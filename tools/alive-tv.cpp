// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

#include "llvm_util/llvm2alive.h"
#include "llvm_util/utils.h"
#include "smt/smt.h"
#include "tools/transform.h"
#include "util/version.h"
#include "util/sort.h"
#include "ir/instr.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCTargetOptionsCommandFlags.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <unordered_map>
#include <ranges>

using namespace tools;
using namespace util;
using namespace std;
using namespace llvm_util;
using namespace llvm;

#define LLVM_ARGS_PREFIX ""
#define ARGS_SRC_TGT
#define ARGS_REFINEMENT
#include "llvm_util/cmd_args_list.h"

namespace {

llvm::cl::opt<string> opt_file1(llvm::cl::Positional,
  llvm::cl::desc("first_bitcode_file"),
  llvm::cl::Required, llvm::cl::value_desc("filename"),
  llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<string> opt_file2(llvm::cl::Positional,
  llvm::cl::desc("[second_bitcode_file]"),
  llvm::cl::Optional, llvm::cl::value_desc("filename"),
  llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<std::string> opt_src_fn(LLVM_ARGS_PREFIX "src-fn",
  llvm::cl::desc("Name of src function (without @)"),
  llvm::cl::cat(alive_cmdargs), llvm::cl::init("src"));

llvm::cl::opt<std::string> opt_tgt_fn(LLVM_ARGS_PREFIX"tgt-fn",
  llvm::cl::desc("Name of tgt function (without @)"),
  llvm::cl::cat(alive_cmdargs), llvm::cl::init("tgt"));

llvm::cl::opt<bool> opt_backend_tv(LLVM_ARGS_PREFIX "backend-tv",
  llvm::cl::desc("Verify operation of a backend (default=false)"),
  llvm::cl::init(false), llvm::cl::cat(alive_cmdargs));


llvm::ExitOnError ExitOnErr;

// adapted from llvm-dis.cpp
std::unique_ptr<llvm::Module> openInputFile(llvm::LLVMContext &Context,
                                            const string &InputFilename) {
  auto MB =
    ExitOnErr(errorOrToExpected(llvm::MemoryBuffer::getFile(InputFilename)));
  llvm::SMDiagnostic Diag;
  auto M = getLazyIRModule(move(MB), Diag, Context,
                           /*ShouldLazyLoadMetadata=*/true);
  if (!M) {
    Diag.print("", llvm::errs(), false);
    return 0;
  }
  ExitOnErr(M->materializeAll());
  return M;
}

optional<smt::smt_initializer> smt_init;

struct Results {
  Transform t;
  string error;
  Errors errs;
  enum {
    ERROR,
    TYPE_CHECKER_FAILED,
    SYNTACTIC_EQ,
    CORRECT,
    UNSOUND,
    FAILED_TO_PROVE
  } status;

  static Results Error(string &&err) {
    Results r;
    r.status = ERROR;
    r.error = move(err);
    return r;
  }
};

Results verify(llvm::Function &F1, llvm::Function &F2,
               llvm::TargetLibraryInfoWrapperPass &TLI,
               bool print_transform = false,
               bool always_verify = false) {
  auto fn1 = llvm2alive(F1, TLI.getTLI(F1));
  if (!fn1)
    return Results::Error("Could not translate '" + F1.getName().str() +
                          "' to Alive IR\n");

  auto fn2 = llvm2alive(F2, TLI.getTLI(F2), fn1->getGlobalVarNames());
  if (!fn2)
    return Results::Error("Could not translate '" + F2.getName().str() +
                          "' to Alive IR\n");

  Results r;
  r.t.src = move(*fn1);
  r.t.tgt = move(*fn2);

  if (!always_verify) {
    stringstream ss1, ss2;
    r.t.src.print(ss1);
    r.t.tgt.print(ss2);
    if (ss1.str() == ss2.str()) {
      if (print_transform)
        r.t.print(*out, {});
      r.status = Results::SYNTACTIC_EQ;
      return r;
    }
  }

  smt_init->reset();
  r.t.preprocess();
  TransformVerify verifier(r.t, false);

  if (print_transform)
    r.t.print(*out, {});

  {
    auto types = verifier.getTypings();
    if (!types) {
      r.status = Results::TYPE_CHECKER_FAILED;
      return r;
    }
    assert(types.hasSingleTyping());
  }

  r.errs = verifier.verify();
  if (r.errs) {
    r.status = r.errs.isUnsound() ? Results::UNSOUND : Results::FAILED_TO_PROVE;
  } else {
    r.status = Results::CORRECT;
  }
  return r;
}

unsigned num_correct = 0;
unsigned num_unsound = 0;
unsigned num_failed = 0;
unsigned num_errors = 0;

bool compareFunctions(llvm::Function &F1, llvm::Function &F2,
                      llvm::TargetLibraryInfoWrapperPass &TLI) {
  auto r = verify(F1, F2, TLI, !opt_quiet, opt_always_verify);
  if (r.status == Results::ERROR) {
    *out << "ERROR: " << r.error;
    ++num_errors;
    return true;
  }

  if (opt_print_dot) {
    r.t.src.writeDot("src");
    r.t.tgt.writeDot("tgt");
  }

  switch (r.status) {
  case Results::ERROR:
    UNREACHABLE();
    break;

  case Results::SYNTACTIC_EQ:
    *out << "Transformation seems to be correct! (syntactically equal)\n\n";
    ++num_correct;
    break;

  case Results::CORRECT:
    *out << "Transformation seems to be correct!\n\n";
    ++num_correct;
    break;

  case Results::TYPE_CHECKER_FAILED:
    *out << "Transformation doesn't verify!\n"
            "ERROR: program doesn't type check!\n\n";
    ++num_errors;
    return true;

  case Results::UNSOUND:
    *out << "Transformation doesn't verify!\n\n";
    if (!opt_quiet)
      *out << r.errs << endl;
    ++num_unsound;
    return false;

  case Results::FAILED_TO_PROVE:
    *out << r.errs << endl;
    ++num_failed;
    return true;
  }

  if (opt_bidirectional) {
    r = verify(F2, F1, TLI, false, opt_always_verify);
    switch (r.status) {
    case Results::ERROR:
    case Results::TYPE_CHECKER_FAILED:
      UNREACHABLE();
      break;

    case Results::SYNTACTIC_EQ:
    case Results::CORRECT:
      *out << "These functions seem to be equivalent!\n\n";
      return true;

    case Results::FAILED_TO_PROVE:
      *out << "Failed to verify the reverse transformation\n\n";
      if (!opt_quiet)
        *out << r.errs << endl;
      return true;

    case Results::UNSOUND:
      *out << "Reverse transformation doesn't verify!\n\n";
      if (!opt_quiet)
        *out << r.errs << endl;
      return false;
    }
  }
  return true;
}

void optimizeModule(llvm::Module *M) {
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  llvm::FunctionPassManager FPM = PB.buildFunctionSimplificationPipeline(
      llvm::OptimizationLevel::O2, llvm::ThinOrFullLTOPhase::None);
  llvm::ModulePassManager MPM;
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  MPM.run(*M, MAM);
}

llvm::Function *findFunction(llvm::Module &M, const string &FName) {
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    if (FName.compare(F.getName()) != 0)
      continue;
    return &F;
  }
  return 0;
}
}

static llvm::mc::RegisterMCTargetOptionsFlags MOF;

class MCInstWrapper{
  
public:
  llvm::MCInst instr;
  MCInstWrapper(llvm::MCInst _instr) : instr(_instr) {}
  llvm::MCInst& getMCInst() {return instr;}
  unsigned getOpcode() const {
    return instr.getOpcode(); 
  }
  void print() const {
    cout << "< MCInstWrapper " << getOpcode() << " ";
    for (auto it=instr.begin(); it!=instr.end(); ++it) {
      if (it->isReg()) {
        cout << "<MCOperand Reg:" << it->getReg() << ">";
      }
      else if (it->isImm()) {
        cout << "<MCOperand Imm:" << it->getImm() << ">";
      }
      else {
        assert("MCInstWrapper printing an unsupported operand" && false);
      }
      cout << " ";
      // TODO for other types
    }
    cout << ">\n"; 
  }
  friend auto operator<=>(const MCInstWrapper&, const MCInstWrapper&) = default;

};

class MCBasicBlock {
  std::string Name;
  using SetTy = llvm::DenseSet<MCBasicBlock*>;
  SetTy Succs;
public:
  std::vector<MCInstWrapper> Instrs;
  MCBasicBlock(std::string _Name) : Name(_Name) {}
  const std::string& getName() const{return Name;}
  auto& getInstrs() { return Instrs;}
  void addInst(MCInstWrapper& inst) { Instrs.push_back(inst); }
  void addSucc(MCBasicBlock* succ_block) { Succs.insert(succ_block); }
  auto succBegin() const { return Succs.begin(); }
  auto succEnd() const { return Succs.end(); }
  auto size() const { return Instrs.size(); }
  void print() const {
    for (auto& inst: Instrs) {
      inst.print();
    }
  }

};

class MCFunction {
  std::string Name;
  unsigned label_cnt{0};
public:
  std::vector<MCBasicBlock> BBs;
  MCFunction() {}
  MCFunction(std::string _Name) : Name(_Name) {}
  void setName(std::string _Name) {
    Name = _Name;
  }
  MCBasicBlock* addBlock(std::string b_name) {
    return &BBs.emplace_back(b_name);
  }
  std::string getName() {
    return Name;
  }
  std::string getLabel() {
    return Name + std::to_string(++label_cnt);
  }
  MCBasicBlock* findBlockByName(std::string b_name) {
    for (auto& bb : BBs) {
      if (bb.getName() == b_name) {
        return &bb;
      }
    }
    return nullptr;
  }
  bool isVarArg() {
    return false;
  }
};

struct MCOperandHash {
  enum Kind{reg=(1<<2)-1, immedidate=(1<<3)-1, symbol=(1<<4)-1};
  size_t operator()(const MCOperand& op) const
  {  
      unsigned prefix;
      unsigned id;
      if (op.isReg()){
        prefix = Kind::reg;
        id = op.getReg();
      }
      else if (op.isImm()){
        prefix = Kind::immedidate;
        id = op.getImm();
      }
      else if (op.isExpr()) {
        prefix = Kind::symbol;
        auto expr = op.getExpr();
        if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
          const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
          const MCSymbol &Sym = SRE.getSymbol();
          errs() << "label : " << Sym.getName() << '\n'; // FIXME remove when done
          id = Sym.getOffset();
        }
        else {
          assert("unsupported mcExpr" && false);
        } 
      }
      else {
        assert("no" && false);
      }
      return std::hash<unsigned long>() (prefix * id);
  } 
};

struct MCOperandEqual {
  enum Kind{reg=(1<<2)-1, immedidate=(1<<3)-1};
  bool operator()(const MCOperand& lhs, const MCOperand& rhs) const
  {  
      if ((lhs.isReg() && rhs.isReg() && (lhs.getReg() == rhs.getReg()))  
      ||  (lhs.isImm() && rhs.isImm() && (lhs.getImm() == rhs.getImm()))
      ||  (lhs.isExpr() && rhs.isExpr() && (lhs.getExpr() == rhs.getExpr()))){ //FIXME this is just comparing ptrs
          return true;
      }
      return false;
  } 
};

std::unordered_map<llvm::MCOperand, IR::Value*, MCOperandHash, MCOperandEqual> mc_value_cache;

// Should eventually be moved to utils.h
// Will need more parameters to identify the exact type of oprand.
// For now, we're just returning 32 bit ints
IR::Type* arm_type2alive(MCOperand ty){
  if (ty.isReg()) {
    return &get_int_type(32);
  }
  else if(ty.isImm()) {
    return &get_int_type(32);
  }
  return nullptr;
}

void mc_add_identifier(const llvm::MCOperand &mc_val, IR::Value& v) {
  mc_value_cache.emplace(mc_val, &v);
}

IR::Value* mc_get_operand(llvm::MCOperand mc_val) {
  if (auto I = mc_value_cache.find(mc_val);
      I != mc_value_cache.end())
    return I->second;

  auto ty = &get_int_type(32); // FIXME
  if (!ty)
    return nullptr;

  // TODO
  if (mc_val.isImm()){

  }

  assert("Unsupported operand" && false);

  return nullptr;
}

// Visit a Vector MCInstWrapper
class MCInstVisitor{
  enum ARM_Instruction {Add=883, Ret=3663};
public:
  static std::unique_ptr<IR::Instr> visit_error(MCInstWrapper& I) {
    cout << "ERROR: Unsupported arm instruction: " ;
    exit(1); // for now lets exit the program if the arm instruction is not supported
    I.print();
    return {};
  }
  // TODO add support for other arm instructions
  static std::unique_ptr<IR::Instr> mc_visit(MCInstWrapper& I) {
    // IR::BinOp::Op alive_op;
    auto opcode = I.getOpcode();
    auto& mc_inst = I.getMCInst();
    if (opcode == ARM_Instruction::Add) {
      assert(mc_inst.getNumOperands() == 4); // dst, lhs, rhs, shift amt
      //for now only support adds with no shift
      assert(mc_inst.getOperand(3).isImm() && (mc_inst.getOperand(3).getImm() == 0));
      auto alive_op = IR::BinOp::Add;
      auto ty = &get_int_type(32); // FIXME
      auto a = mc_get_operand(mc_inst.getOperand(1));
      auto b = mc_get_operand(mc_inst.getOperand(2));
      if (!ty || !a || !b)
        return visit_error(I);
      assert(mc_inst.getOperand(0).isReg());
      std::string operand_name = "%" + std::to_string(mc_inst.getOperand(0).getReg());
      auto ret = make_unique<IR::BinOp>(*ty, move(operand_name), *a, *b, alive_op);
      mc_add_identifier(mc_inst.getOperand(0),*ret.get());
      return ret;
    }
    else if (opcode == ARM_Instruction::Ret) {
      // for now we're assuming that the function returns an integer value
      assert(mc_inst.getNumOperands() == 1); 
      auto ty = &get_int_type(32); // FIXME
      auto val = mc_get_operand(mc_inst.getOperand(0));
      if (!ty || !val)
        return visit_error(I);
      return make_unique<IR::Return>(*ty, *val);
    }
    else {
      return visit_error(I);
    }
    return nullptr;

  }

};

// std::unique_ptr<IR::Instr> visit_error(MCInstWrapper& I) {
//   cout << "ERROR: Unsupported arm instruction: " << I.print() << '\n';
//   return {};
// }
// // TODO add support for other arm instructions
// // FIXME the code will need to become more modular
// 
// std::unique_ptr<IR::Instr> mc_visit(MCInstWrapper& I) {
//   IR::BinOp::Op alive_op;
//   auto opcode = I.getOpcode();
//   return nullptr;
// }



// FIXME evantually this should take the entire arm SSA function as an input. i.e., ArmFunction class
// For now we pass a vector of MCInstWrapper which represents a single basicblock in SSA form 
std::optional<IR::Function> arm2alive(MCFunction &MF, const llvm::DataLayout &DL) {
  // only work with 

  // for now assume that return type is 32-bit integer
  auto func_return_type = &get_int_type(32);
  if (!func_return_type) 
    return {};
  
  IR::Function Fn(*func_return_type, MF.getName());
  reset_state(Fn);
  
  // FIXME infer funciton attributes 
  // Most likely need to emit and read the debug info
  
  auto& first_BB = MF.BBs[0];
  auto& BB_mcinstrs = first_BB.getInstrs();
  MCInst& first_instr = BB_mcinstrs[0].getMCInst();
  // FIXME for now assuming arm function takes two args
  // from the first 2 arguments of the first instruction in the MCFunction
  for (unsigned idx = 1; idx < 3; ++idx) {
    auto& operand = first_instr.getOperand(idx);
    auto ty = arm_type2alive(operand);
    if (!ty) 
      return {};
    assert(operand.isReg());
    std::string operand_name = "%" + std::to_string(operand.getReg());
    auto val = make_unique<IR::Input>(*ty, move(operand_name));
    mc_add_identifier(operand, *val.get());
    Fn.addInput(move(val));
  }

  // Create Fn's BBs
  vector<pair<IR::BasicBlock*, MCBasicBlock*>> sorted_bbs;
  
  {
    util::edgesTy edges;
    vector<MCBasicBlock*> bbs;
    unordered_map<MCBasicBlock*, unsigned> bb_map;
    
    auto bb_num = [&](MCBasicBlock* bb) {
      auto[I, inserted] = bb_map.emplace(bb, bbs.size());
      if (inserted) {
        bbs.emplace_back(bb);
        edges.emplace_back();
      }
      return I->second;
    };

    for (auto& bb : MF.BBs) {
      auto n = bb_num(&bb);
      for (auto it=bb.succBegin(); it!=bb.succEnd(); ++it) {
        auto succ_ptr = *it;
        auto n_dst = bb_num(succ_ptr);
        edges[n].emplace(n_dst);
      }
    }

    for (auto v : top_sort(edges)) {
        sorted_bbs.emplace_back(&Fn.getBB(bbs[v]->getName()), bbs[v]);
    }
  }

  for (auto& [alive_bb, mc_bb] : sorted_bbs) {
    auto mc_instrs = mc_bb->getInstrs();
    for (auto& mc_instr : mc_instrs) {
      if (auto I = MCInstVisitor::mc_visit(mc_instr)) {
        // auto alive_i = I.get();
        alive_bb->addInstr(move(I));
      }
      else {
        return {};
      }
    }    
  }
    
  return move(Fn);
}

// TODO for now we're using this class to generate the arm assembly
// cfg. might want to move this implementation somewhere else
class MCStreamerWrapper final : public llvm::MCStreamer {
enum ASMLine{none=0, label=1, non_term_instr=2, terminator=3};
private:
  MCBasicBlock* CurBlock{nullptr};
  std::string CurLabel;
  bool first_label{true};
  unsigned prev_line{0};
  std::map<std::string,std::vector<MCInstWrapper>*> Label2Block;
  std::vector<std::string> LabelNames;
  llvm::MCInstrAnalysis* Ana_ptr;
  llvm::MCInstPrinter* IP_ptr;
  llvm::MCRegisterInfo* MRI_ptr;
public:
  MCFunction MF;
  unsigned cnt{0};
  std::vector<llvm::MCInst> Insts;
  std::vector<MCInstWrapper> W_Insts;
  std::vector<std::vector<MCInstWrapper>> Blocks;
  
  MCStreamerWrapper(llvm::MCContext &Context, llvm::MCInstrAnalysis* _Ana_ptr, 
                    llvm::MCInstPrinter* _IP_ptr, llvm::MCRegisterInfo* _MRI_ptr)
      : MCStreamer(Context), Ana_ptr(_Ana_ptr) , IP_ptr(_IP_ptr), MRI_ptr(_MRI_ptr) {}

  // We only want to intercept the emission of new instructions.
  virtual void emitInstruction(const llvm::MCInst &Inst,
                               const llvm::MCSubtargetInfo & /* unused */) override {
    assert(prev_line != ASMLine::none);
    if (prev_line == ASMLine::terminator) {
      CurBlock = MF.addBlock(MF.getLabel());
    }
    MCInstWrapper Cur_Inst(Inst);
    CurBlock->addInst(Cur_Inst);
    Insts.push_back(Inst);
    
    if (Ana_ptr->isTerminator(Inst)) {
      prev_line = ASMLine::terminator;
    }
    else {
      prev_line = ASMLine::non_term_instr;
    }
    auto& inst_ref = Cur_Inst.getMCInst();
    auto num_operands = inst_ref.getNumOperands();
    for (unsigned i=0; i < num_operands; ++i ) {
      auto op = inst_ref.getOperand(i);
      if (op.isExpr()) {
        auto expr = op.getExpr();
        if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
          const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
          const MCSymbol &Sym = SRE.getSymbol();
          errs() << "target label : " << Sym.getName() << ", offset=" << Sym.getOffset() << '\n'; // FIXME remove when done
        } 
      }
    }
    
    errs() << cnt++ << "  : ";
    Inst.dump_pretty(llvm::errs(), IP_ptr, " ", MRI_ptr);
    if (Ana_ptr->isBranch(Inst)) 
      errs() << ": branch ";
    if (Ana_ptr->isConditionalBranch(Inst)) 
      errs() << ": conditional branch ";
    if (Ana_ptr->isUnconditionalBranch(Inst)) 
      errs() << ": unconditional branch ";
    if (Ana_ptr->isTerminator(Inst)) 
      errs() << ": terminator ";
    errs() << "\n";
  }

  bool emitSymbolAttribute(llvm::MCSymbol *Symbol, llvm::MCSymbolAttr Attribute) override {
    return true;
  }

  void emitCommonSymbol(llvm::MCSymbol *Symbol, uint64_t Size,
                        unsigned ByteAlignment) override {}
  void emitZerofill(llvm::MCSection *Section, llvm::MCSymbol *Symbol = nullptr,
                    uint64_t Size = 0, unsigned ByteAlignment = 0,
                    llvm::SMLoc Loc = llvm::SMLoc()) override {}
  void emitGPRel32Value(const llvm::MCExpr *Value) override {}
  void BeginCOFFSymbolDef(const llvm::MCSymbol *Symbol) override {}
  void EmitCOFFSymbolStorageClass(int StorageClass) override {}
  void EmitCOFFSymbolType(int Type) override {}
  void EndCOFFSymbolDef() override {}
  virtual void emitLabel(MCSymbol *Symbol, SMLoc Loc) override {
    // Assuming the first label encountered is the function's name
    // Need to figure out if there is a better name to get access to the function's name
    if (first_label) {
      MF.setName(Symbol->getName().str() + "-tgt");
      first_label = false;
    }
    CurLabel = Symbol->getName().str();
    CurBlock = MF.addBlock(CurLabel);
    prev_line = ASMLine::label;
    errs() << cnt++ << "  : ";
    errs() << "inside Emit Label: symbol=" << Symbol->getName() << '\n'; 
  }

  std::string findTargetLabel(MCInst& inst_ref) {
    auto num_operands = inst_ref.getNumOperands();
    for (unsigned i=0; i < num_operands; ++i ) {
      auto op = inst_ref.getOperand(i);
      if (op.isExpr()) {
        auto expr = op.getExpr();
        if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
          const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
          const MCSymbol &Sym = SRE.getSymbol();
          return Sym.getName().str();
        } 
      }
    }
    UNREACHABLE();
  }
  // call after MF with basic block is constructed to generate the successors for 
  // each basic block
  void generateSuccessors() {
    cout << "generating basic block successors" << '\n';
    for (unsigned i=0; i < MF.BBs.size()-1; ++i) {
      auto& cur_bb = MF.BBs[i];
      auto next_bb_ptr = &MF.BBs[i+1];
      if (cur_bb.Instrs.empty()) {
        cout << "generateSuccessors, encountered basic block with 0 instructions" << '\n';
        continue;
      }
      auto& last_mc_instr = cur_bb.Instrs.back().getMCInst();
      if (Ana_ptr->isConditionalBranch(last_mc_instr)) {
        std::string target = findTargetLabel(last_mc_instr);
        auto target_bb = MF.findBlockByName(target);
        cur_bb.addSucc(target_bb);
        cur_bb.addSucc(next_bb_ptr);        
      }
      else if (Ana_ptr->isUnconditionalBranch(last_mc_instr)) {
        std::string target = findTargetLabel(last_mc_instr);
        auto target_bb = MF.findBlockByName(target);
        cur_bb.addSucc(target_bb);
      }
      else if (Ana_ptr->isReturn(last_mc_instr)) {
        continue;
      }
      else { // add edge to next block
        cur_bb.addSucc(next_bb_ptr);
      }
    }

  }

  void printBlocks() {
    cout << "#of Blocks = " << MF.BBs.size() << '\n';
    cout << "-------------\n";
    int i=0;
    for (auto& block: MF.BBs) {
      errs() << "block " << i << ", name= " << block.getName() <<'\n';
      for (auto& inst: block.Instrs) {
        inst.instr.dump_pretty(llvm::errs(), IP_ptr, " ", MRI_ptr);
        errs() << '\n';
      }
      i++;
    }
  }

  void printCFG() {
    cout << "printing arm function CFG" << '\n';
    for (auto& block: MF.BBs) {
      cout << block.getName() << ": [";
      for (auto it=block.succBegin(); it!=block.succEnd(); ++it) {
        
        auto successor = *it;
        cout << successor->getName() << ", ";
        //cout << it->getName() << ", ";
      }
      cout << "]\n";
    }
  }
  /*
  ArrayRef<llvm::MCInst> GetInstructionSequence(unsigned Index) const {
    return Regions.getInstructionSequence(Index);
  }
  */
};



struct CanonVal{
  static unsigned id_;
  unsigned id{0};
  std::string prefix;
  void assignId() {
    id = ++id_;
  }
};

static unsigned id_{0};
unsigned getId() {
  return ++id_;
}

unsigned InsertAndFind(std::unordered_map<unsigned, unsigned>& var2num, unsigned var){
  if (var2num.count(var) == 0) {
    auto new_value_num = getId();
    var2num.emplace(var, new_value_num);
    return new_value_num;
  }
  return 0;
}

struct SymValue{
  unsigned opcode{0};
  //std::vector<MCOperand> operands;
  std::vector<unsigned> operands;
  //SymValue()
  //:id(++id_) {}
  
};

struct SymValueHash {
  size_t operator()(const SymValue& op) const
  {  
    //auto op_hasher = MCOperandHash();
    // Is this the right way to go about this
    unsigned combined_val = 41; // start with some prime number
    // Is this too expensive?
    for (auto& e : op.operands) {
      combined_val += e;
    }
    return std::hash<unsigned long>() (op.opcode + combined_val);
  } 
};

struct SymValueEqual {
  enum Kind{reg=(1<<2)-1, immedidate=(1<<3)-1};
  bool operator()(const SymValue& lhs, const SymValue& rhs) const
  { 
    // auto mcop_eq = MCOperandEqual();
    if (lhs.opcode != rhs.opcode || lhs.operands.size() != rhs.operands.size()) {
      return false;
    } 
    for (unsigned i=0; i < rhs.operands.size(); ++i) {
      //if (!mcop_eq(lhs.operands[i], rhs.operands[i]))
      if (lhs.operands[i] != rhs.operands[i])
        return false;
    } 
    return true;
  } 
};

// Return variable that are read before being written in the basic block
auto FindReadBeforeWritten(std::vector<MCInst>& instrs, llvm::MCInstrAnalysis* Ana_ptr) {
  std::unordered_set<MCOperand,MCOperandHash,MCOperandEqual> reads;
  std::unordered_set<MCOperand,MCOperandHash,MCOperandEqual> writes;
  // TODO for writes, should only apply to instructions that update a destination register
  for (auto& I : instrs) {
    if (Ana_ptr->isReturn(I))
      continue;
    assert(I.getNumOperands() > 0 && "MCInst with zero operands");
    for (unsigned j = 1; j < I.getNumOperands(); ++j) {
      if (!writes.contains(I.getOperand(j))) 
        reads.insert(I.getOperand(j));
    }
    writes.insert(I.getOperand(0));
  }

  return reads;
}

// Return variable that are read before being written in the basic block
auto FindReadBeforeWritten(MCBasicBlock& block, llvm::MCInstrAnalysis* Ana_ptr) {
  auto mcInstrs = block.getInstrs();
  std::unordered_set<MCOperand,MCOperandHash,MCOperandEqual> reads;
  std::unordered_set<MCOperand,MCOperandHash,MCOperandEqual> writes;
  // TODO for writes, should only apply to instructions that update a destination register
  for (auto& WI : mcInstrs) {
    auto& I = WI.getMCInst();
    if (Ana_ptr->isReturn(I))
      continue;
    assert(I.getNumOperands() > 0 && "MCInst with zero operands");
    for (unsigned j = 1; j < I.getNumOperands(); ++j) {
      if (!writes.contains(I.getOperand(j))) 
        reads.insert(I.getOperand(j));
    }
    writes.insert(I.getOperand(0));
  }

  return reads;
}




std::vector<bool> LastWrites(std::vector<MCInst>& instrs) {
  // TODO need to check for size of instrs in backend-tv and return error
  // otherwise these helper functions will fail in an ugly manner
  auto last_write = std::vector<bool>(instrs.size(), false);
  std::unordered_set<MCOperand,MCOperandHash,MCOperandEqual> willOverwrite;
  unsigned index = instrs.size() - 1;
  // TODO should only apply to instructions that update a destination register
  for (auto& r_I : ranges::views::reverse(instrs)) {
    // TODO Check for dest reg 
    // for now assume operand 0 is dest register 
    if (!willOverwrite.contains(r_I.getOperand(0))) {
      last_write[index] = true;
      willOverwrite.insert(r_I.getOperand(0));
    }
    index--;
  }
  return last_write;
}

std::vector<bool> LastWrites(MCBasicBlock& block) {
  // TODO need to check for size of instrs in backend-tv and return error
  // otherwise these helper functions will fail in an ugly manner
  auto mcInstrs = block.getInstrs();
  auto last_write = std::vector<bool>(mcInstrs.size(), false);
  std::unordered_set<MCOperand,MCOperandHash,MCOperandEqual> willOverwrite;
  unsigned index = mcInstrs.size() - 1;
  // TODO should only apply to instructions that update a destination register
  for (auto& r_WI : ranges::views::reverse(mcInstrs)) {
    auto& r_I = r_WI.getMCInst();
    // TODO Check for dest reg 
    // for now assume operand 0 is dest register 
    if (!willOverwrite.contains(r_I.getOperand(0))) {
      last_write[index] = true;
      willOverwrite.insert(r_I.getOperand(0));
    }
    index--;
  }
  return last_write;
}
Results backend_verify(std::optional<IR::Function>& fn1, 
                       std::optional<IR::Function>& fn2,
                       llvm::TargetLibraryInfoWrapperPass &TLI,
                       bool print_transform = false,
                       bool always_verify = false) {
  Results r;
  r.t.src = move(*fn1);
  r.t.tgt = move(*fn2);

  if (!always_verify) {
    stringstream ss1, ss2;
    r.t.src.print(ss1);
    r.t.tgt.print(ss2);
    if (ss1.str() == ss2.str()) {
      if (print_transform)
        r.t.print(*out, {});
      r.status = Results::SYNTACTIC_EQ;
      return r;
    }
  }

  smt_init->reset();
  r.t.preprocess();
  TransformVerify verifier(r.t, false);

  if (print_transform)
    r.t.print(*out, {});

  {
    auto types = verifier.getTypings();
    if (!types) {
      r.status = Results::TYPE_CHECKER_FAILED;
      return r;
    }
    assert(types.hasSingleTyping());
  }

  r.errs = verifier.verify();
  if (r.errs) {
    r.status = r.errs.isUnsound() ? Results::UNSOUND : Results::FAILED_TO_PROVE;
  } else {
    r.status = Results::CORRECT;
  }
  return r;


}
bool backendTV() {
  if (!opt_file2.empty()) {
    cerr << "Please only specify one bitcode file when validating a backend\n";
    exit(-1);    
  }

  llvm::LLVMContext Context;
  auto M1 = openInputFile(Context, opt_file1);
  if (!M1.get()) {
    cerr << "Could not read bitcode from '" << opt_file1 << "'\n";
    exit(-1);
  }

#define ARGS_MODULE_VAR M1
# include "llvm_util/cmd_args_def.h"

  auto &DL = M1.get()->getDataLayout();
  llvm::Triple targetTriple(M1.get()->getTargetTriple());
  llvm::TargetLibraryInfoWrapperPass TLI(targetTriple);

  llvm_util::initializer llvm_util_init(*out, DL);
  smt_init.emplace();

  LLVMInitializeAArch64TargetInfo();
  LLVMInitializeAArch64Target();
  LLVMInitializeAArch64TargetMC();
  LLVMInitializeAArch64AsmParser();
  LLVMInitializeAArch64AsmPrinter();
 
  std::string Error;
  const char *TripleName = "aarch64-arm-none-eabi";
  auto Target = llvm::TargetRegistry::lookupTarget(TripleName, Error);
  if (!Target) {
    cerr << Error;
    exit(-1);
  }
  llvm::TargetOptions Opt;
  const char *CPU = "apple-a12";
  auto RM = llvm::Optional<llvm::Reloc::Model>();
  auto TM = Target->createTargetMachine(TripleName, CPU, "", Opt, RM);

  llvm::SmallString<1024> Asm;
  llvm::raw_svector_ostream Dest(Asm);

  llvm::legacy::PassManager pass;
  if (TM->addPassesToEmitFile(pass, Dest, nullptr, llvm::CGFT_AssemblyFile)) {
    cerr << "Failed to generate assembly";
    exit(-1);
  }
  pass.run(*M1);

  // FIXME only do this in verbose mode, or something
  for (size_t i=0; i<Asm.size(); ++i)
    cout << Asm[i];
  cout << "-------------\n";
  cout << "\n\n";
  llvm::Triple TheTriple(TripleName);

  auto MCOptions = llvm::mc::InitMCTargetOptionsFromFlags();
  std::unique_ptr<llvm::MCRegisterInfo> MRI(Target->createMCRegInfo(TripleName));
  assert(MRI && "Unable to create target register info!");

  std::unique_ptr<llvm::MCAsmInfo> MAI(
      Target->createMCAsmInfo(*MRI, TripleName, MCOptions));
  assert(MAI && "Unable to create MC asm info!");

  std::unique_ptr<llvm::MCSubtargetInfo> STI(
      Target->createMCSubtargetInfo(TripleName, CPU, ""));
  assert(STI && "Unable to create subtarget info!");
  assert(STI->isCPUStringValid(CPU) && "Invalid CPU!");

  llvm::MCContext Ctx(TheTriple, MAI.get(), MRI.get(), STI.get());

  llvm::SourceMgr SrcMgr;
  auto Buf = llvm::MemoryBuffer::getMemBuffer(Asm.c_str());
  assert(Buf);
  SrcMgr.AddNewSourceBuffer(std::move(Buf), llvm::SMLoc());

  std::unique_ptr<llvm::MCInstrInfo> MCII(Target->createMCInstrInfo());
  assert(MCII && "Unable to create instruction info!");

  std::unique_ptr<llvm::MCInstPrinter> IPtemp(Target->createMCInstPrinter(
      TheTriple, 0, *MAI, *MCII, *MRI));
  
  auto Ana = std::make_unique<MCInstrAnalysis>(MCII.get());

  MCStreamerWrapper Str(Ctx, Ana.get(), IPtemp.get(), MRI.get());
  
  std::unique_ptr<llvm::MCAsmParser> Parser(
      llvm::createMCAsmParser(SrcMgr, Ctx, Str, *MAI));
  assert(Parser);
  
  llvm::MCTargetOptions Opts;
  Opts.PreserveAsmComments = false;
  std::unique_ptr<llvm::MCTargetAsmParser> TAP(
      Target->createMCAsmParser(*STI, *Parser, *MCII, Opts));
  assert(TAP);
  Parser->setTargetParser(*TAP);
  Parser->Run(true); // ??

  // FIXME Nader your code here
  cout << "\n\nPretty Parsed MCInsts:\n";
  for (auto I : Str.Insts) {
    I.dump_pretty(llvm::errs(), IPtemp.get(), " ", MRI.get());
    llvm::errs() << '\n';
  }

  cout << "\n\nParsed MCInsts:\n";
  for (auto I : Str.Insts) {
    I.dump_pretty(llvm::errs());
    llvm::errs() << '\n';
  } 
  
  cout << "\n\n";
  
  Str.MF.BBs.pop_back(); // remove the last basic block corresponding to .Lfunc_end 
  Str.printBlocks();
  Str.generateSuccessors();
  Str.printCFG();
  // FIXME for now, exit if the function has more than 2 blocks
  if (Str.MF.BBs.size() > 2) {
    cout << "ERROR: we don't generate SSA for this type of arm function yet" << '\n';
    return false;
  }
  
  std::unordered_map<MCOperand,unsigned,MCOperandHash,MCOperandEqual> svar2num; 
  std::unordered_map<unsigned,MCOperand> num2cvar; 
  std::unordered_map<SymValue,unsigned,SymValueHash,SymValueEqual> value2num;
   
  cout << "finding readers and updating maps\n";
  auto& MF = Str.MF;
  auto& first_BB = MF.BBs[0];
  // FIXME for now we process first basic block only
  auto first_reads = FindReadBeforeWritten(first_BB, Ana.get());
  for (auto& read_op : first_reads) {
    auto new_num = getId();
    svar2num.emplace(read_op, new_num);
    num2cvar.emplace(new_num, read_op);
  }
  cout << "--------svar2num-----------\n";
  for (auto& [key,val]: svar2num) {
    key.dump();
    errs() << ": " << val << '\n';
  }
  cout << "--------num2cvar----------\n";
  for (auto& [key,val]: num2cvar) {
    errs() << key << ": ";
    val.dump();
    errs() << '\n';
  }
  cout << "----------------------\n";
  auto last_writes = LastWrites(first_BB);
  for (unsigned i = 0; i < first_BB.Instrs.size(); ++i) {
    auto& cur_w_instr = first_BB.getInstrs()[i];
    auto& cur_instr = cur_w_instr.getMCInst();
    // llvm::errs() << "<" << last_writes[i] << "> ";
    // cur_instr.dump_pretty(llvm::errs());
    // llvm::errs() << '\n';
    auto sym_val = SymValue();
    // TODO move this to SymValue CTOR and also distinguish operand selection based on opcode
    // Right now we assume operand 0 is destination and operands 1..n are used
    sym_val.opcode = cur_instr.getOpcode();
    assert(cur_instr.getNumOperands() > 0 && "MCInst with zero operands");
    for (unsigned j = 1; j < cur_instr.getNumOperands(); ++j) {
        sym_val.operands.push_back(svar2num[cur_instr.getOperand(j)]);
    }
    if (!value2num.contains(sym_val)) {
     // TODO 
    }

    // Only do the following if opcode writes to a variable
    auto dst = cur_instr.getOperand(0);
    auto new_num = getId();
    svar2num.emplace(dst, new_num);

    MCOperand new_dst;
    // if (last_writes[i]) { //no need to add version numbering to dst
    //   new_dst = dst;
    // }
    // else {
      new_dst = dst;
      assert(new_dst.isReg());
      new_dst.setReg(1000 + new_num); // FIXME
    // }

    num2cvar.emplace(new_num, new_dst);
    //dst.setReg(new_dst.getReg());
    cur_instr.getOperand(0).setReg(new_dst.getReg());

    if (!sym_val.operands.empty()) {
      value2num.emplace(sym_val, new_num);
    }
    // Update MCInstrs operands with 

    for (unsigned j = 1; j < cur_instr.getNumOperands(); ++j) {
      auto& inst_operand = cur_instr.getOperand(j);
      if (inst_operand.isReg()) {
        inst_operand.setReg(num2cvar[sym_val.operands[j-1]].getReg());
      }
    }
  } 

  cout << "\n\nAfter LVN MCInsts:\n";
  
  first_BB.print();
  // Adjust the ret instruction's operand
  // FIXME for now we're doing something pretty naive/wrong except in the simplest cases
  auto& BB_mcinstrs = first_BB.getInstrs();
  auto BB_size = first_BB.size();
  assert(BB_size > 1);
  MCInst& last_instr = BB_mcinstrs[BB_size-1].getMCInst();
  MCInst& sec_last_instr = BB_mcinstrs[BB_size-2].getMCInst();
  auto& dest_operand = sec_last_instr.getOperand(0);
  auto& ret_operand = last_instr.getOperand(0);
  assert(dest_operand.isReg());
  ret_operand.setReg(dest_operand.getReg());

  cout << "\n\nAfter adjusting return instruction:\n";
  first_BB.print();
  
  cout << "\n\nConverting source llvm function to alive ir\n"; 
  std::optional<IR::Function> AF;
  // Only try to verify the first function in the module
  for (auto &F : *M1.get()) {
    if (F.isDeclaration())
      continue;
    if (!func_names.empty() && !func_names.count(F.getName().str()))
      continue;
    AF = llvm2alive(F,TLI.getTLI(F));
    break;
  }
  AF->print(cout << "\n----------alive-ir-src.ll-file----------\n");

  auto TF = arm2alive(MF, DL);
  if (TF) 
    TF->print(cout << "\n----------alive-lift-arm-target----------\n");

  auto r = backend_verify(AF,TF, TLI);

  if (r.status == Results::ERROR) {
    *out << "ERROR: " << r.error;
    ++num_errors;
    return true;
  }

  if (opt_print_dot) {
    r.t.src.writeDot("src");
    r.t.tgt.writeDot("tgt");
  }

  switch (r.status) {
  case Results::ERROR:
    UNREACHABLE();
    break;

  case Results::SYNTACTIC_EQ:
    *out << "Transformation seems to be correct! (syntactically equal)\n\n";
    ++num_correct;
    break;

  case Results::CORRECT:
    *out << "Transformation seems to be correct!\n\n";
    ++num_correct;
    break;

  case Results::TYPE_CHECKER_FAILED:
    *out << "Transformation doesn't verify!\n"
            "ERROR: program doesn't type check!\n\n";
    ++num_errors;
    return true;

  case Results::UNSOUND:
    *out << "Transformation doesn't verify!\n\n";
    if (!opt_quiet)
      *out << r.errs << endl;
    ++num_unsound;
    return false;

  case Results::FAILED_TO_PROVE:
    *out << r.errs << endl;
    ++num_failed;
    return true;
  }
  return false;
/*
  auto SRC = findFunction(*M1, opt_src_fn);
  auto TGT = findFunction(*M1, opt_tgt_fn);
  if (SRC && TGT) {
    compareFunctions(*SRC, *TGT, TLI);
    return;
  } else {
    M2 = CloneModule(*M1);
    optimizeModule(M2.get());
  }

  // FIXME: quadratic, may not be suitable for very large modules
  // emitted by opt-fuzz
  for (auto &F1 : *M1.get()) {
    if (F1.isDeclaration())
      continue;
    if (!func_names.empty() && !func_names.count(F1.getName().str()))
      continue;
    for (auto &F2 : *M2.get()) {
      if (F2.isDeclaration() || F1.getName() != F2.getName())
        continue;
      if (!compareFunctions(F1, F2, TLI))
        if (opt_error_fatal)
          return;
      break;
    }
  }

  */
}

void bitcodeTV() {
  llvm::LLVMContext Context;
  auto M1 = openInputFile(Context, opt_file1);
  if (!M1.get()) {
    cerr << "Could not read bitcode from '" << opt_file1 << "'\n";
    exit(-1);
  }

#define ARGS_MODULE_VAR M1
# include "llvm_util/cmd_args_def.h"

  auto &DL = M1.get()->getDataLayout();
  llvm::Triple targetTriple(M1.get()->getTargetTriple());
  llvm::TargetLibraryInfoWrapperPass TLI(targetTriple);

  llvm_util::initializer llvm_util_init(*out, DL);
  smt_init.emplace();

  unique_ptr<llvm::Module> M2;
  if (opt_file2.empty()) {
    auto SRC = findFunction(*M1, opt_src_fn);
    auto TGT = findFunction(*M1, opt_tgt_fn);
    if (SRC && TGT) {
      compareFunctions(*SRC, *TGT, TLI);
      return;
    } else {
      M2 = CloneModule(*M1);
      optimizeModule(M2.get());
    }
  } else {
    M2 = openInputFile(Context, opt_file2);
    if (!M2.get()) {
      *out << "Could not read bitcode from '" << opt_file2 << "'\n";
      exit(-1);
    }
  }

  if (M1.get()->getTargetTriple() != M2.get()->getTargetTriple()) {
    *out << "Modules have different target triples\n";
    exit(-1);
  }

  // FIXME: quadratic, may not be suitable for very large modules
  // emitted by opt-fuzz
  for (auto &F1 : *M1.get()) {
    if (F1.isDeclaration())
      continue;
    if (!func_names.empty() && !func_names.count(F1.getName().str()))
      continue;
    for (auto &F2 : *M2.get()) {
      if (F2.isDeclaration() || F1.getName() != F2.getName())
        continue;
      if (!compareFunctions(F1, F2, TLI))
        if (opt_error_fatal)
          return;
      break;
    }
  }
}

int main(int argc, char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);
  llvm::EnableDebugBuffering = true;
  llvm::llvm_shutdown_obj llvm_shutdown; // Call llvm_shutdown() on exit.

  std::string Usage =
      R"EOF(Alive2 stand-alone translation validator:
version )EOF";
  Usage += alive_version;
  Usage += R"EOF(
see alive-tv --version  for LLVM version info,

This program takes either one or two LLVM IR files files as
command-line arguments. Both .bc and .ll files are supported.

If two files are provided, alive-tv checks that functions in the
second file refine functions in the first file, matching up functions
by name. Functions not found in both files are ignored. It is an error
for a function to be found in both files unless they have the same
signature.

If one file is provided, there are two possibilities. If the file
contains a function called "src" and also a function called "tgt",
then alive-tv will determine whether src is refined by tgt. It is an
error if src and tgt do not have the same signature. Otherwise,
alive-tv will optimize the entire module using an optimization
pipeline similar to -O2, and then verify that functions in the
optimized module refine those in the original one. This provides a
convenient way to demonstrate an existing optimizer bug.
)EOF";

  llvm::cl::HideUnrelatedOptions(alive_cmdargs);
  llvm::cl::ParseCommandLineOptions(argc, argv, Usage);

  if (opt_backend_tv) {
    backendTV();
  } else {
    bitcodeTV();
  }
  
  *out << "Summary:\n"
          "  " << num_correct << " correct transformations\n"
          "  " << num_unsound << " incorrect transformations\n"
          "  " << num_failed  << " failed-to-prove transformations\n"
          "  " << num_errors << " Alive2 errors\n";

  if (opt_smt_stats)
    smt::solver_print_stats(*out);

  smt_init.reset();

  if (opt_alias_stats)
    IR::Memory::printAliasStats(*out);

  return num_errors > 0;
}
