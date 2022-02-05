// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

#include "ir/instr.h"
#include "ir/type.h"
#include "llvm_util/llvm2alive.h"
#include "llvm_util/utils.h"
#include "smt/smt.h"
#include "tools/transform.h"
#include "util/sort.h"
#include "util/version.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCTargetOptionsCommandFlags.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

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
                                llvm::cl::Required,
                                llvm::cl::value_desc("filename"),
                                llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<string> opt_file2(llvm::cl::Positional,
                                llvm::cl::desc("[second_bitcode_file]"),
                                llvm::cl::Optional,
                                llvm::cl::value_desc("filename"),
                                llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<std::string>
    opt_src_fn(LLVM_ARGS_PREFIX "src-fn",
               llvm::cl::desc("Name of src function (without @)"),
               llvm::cl::cat(alive_cmdargs), llvm::cl::init("src"));

llvm::cl::opt<std::string>
    opt_tgt_fn(LLVM_ARGS_PREFIX "tgt-fn",
               llvm::cl::desc("Name of tgt function (without @)"),
               llvm::cl::cat(alive_cmdargs), llvm::cl::init("tgt"));

llvm::cl::opt<bool> opt_backend_tv(
    LLVM_ARGS_PREFIX "backend-tv",
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
               bool print_transform = false, bool always_verify = false) {
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
} // namespace

static llvm::mc::RegisterMCTargetOptionsFlags MOF;

// Class that wraps the underlying MCInst instruction
// FIXME a lot has to change in this class since write
// now it doens't really wrap the instruction well enough
// and it doesn't add additional functionality compared
// to MCInst
class MCInstWrapper {
public:
  llvm::MCInst instr;
  MCInstWrapper(llvm::MCInst _instr) : instr(_instr) {}
  llvm::MCInst &getMCInst() {
    return instr;
  }
  unsigned getOpcode() const {
    return instr.getOpcode();
  }
  void print() const {
    cout << "< MCInstWrapper " << getOpcode() << " ";
    for (auto it = instr.begin(); it != instr.end(); ++it) {
      if (it->isReg()) {
        cout << "<MCOperand Reg:" << it->getReg() << ">";
      } else if (it->isImm()) {
        cout << "<MCOperand Imm:" << it->getImm() << ">";
      } else {
        assert("MCInstWrapper printing an unsupported operand" && false);
      }
      cout << " ";
      // TODO for other types
    }
    cout << ">\n";
  }
  friend auto operator<=>(const MCInstWrapper &,
                          const MCInstWrapper &) = default;
};

// Class to represent a basic block of machine instructions
class MCBasicBlock {
  std::string Name;
  using SetTy = llvm::DenseSet<MCBasicBlock *>;
  SetTy Succs;
  SetTy Preds;

public:
  std::vector<MCInstWrapper> Instrs;
  MCBasicBlock(std::string _Name) : Name(_Name) {}
  const std::string &getName() const {
    return Name;
  }

  auto &getInstrs() {
    return Instrs;
  }

  void addInst(MCInstWrapper &inst) {
    Instrs.push_back(inst);
  }

  void addSucc(MCBasicBlock *succ_block) {
    Succs.insert(succ_block);
  }

  void addPred(MCBasicBlock *pred_block) {
    Preds.insert(pred_block);
  }

  auto predBegin() {
    return Preds.begin();
  }

  auto predEnd() {
    return Preds.end();
  }

  auto succBegin() const {
    return Succs.begin();
  }

  auto succEnd() const {
    return Succs.end();
  }

  auto size() const {
    return Instrs.size();
  }

  void print() const {
    for (auto &inst : Instrs) {
      inst.print();
    }
  }
};

// Class to represent a machine fucntion
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
  MCBasicBlock *addBlock(std::string b_name) {
    return &BBs.emplace_back(b_name);
  }
  std::string getName() {
    return Name;
  }
  std::string getLabel() {
    return Name + std::to_string(++label_cnt);
  }
  MCBasicBlock *findBlockByName(std::string b_name) {
    for (auto &bb : BBs) {
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
  enum Kind {
    reg = (1 << 2) - 1,
    immedidate = (1 << 3) - 1,
    symbol = (1 << 4) - 1
  };
  size_t operator()(const MCOperand &op) const {
    unsigned prefix;
    unsigned id;
    if (op.isReg()) {
      prefix = Kind::reg;
      id = op.getReg();
    } else if (op.isImm()) {
      prefix = Kind::immedidate;
      id = op.getImm();
    } else if (op.isExpr()) {
      prefix = Kind::symbol;
      auto expr = op.getExpr();
      if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
        const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
        const MCSymbol &Sym = SRE.getSymbol();
        errs() << "label : " << Sym.getName() << '\n'; // FIXME remove when done
        id = Sym.getOffset();
      } else {
        assert("unsupported mcExpr" && false);
      }
    } else {
      assert("no" && false);
    }
    return std::hash<unsigned long>()(prefix * id);
  }
};

struct MCOperandEqual {
  enum Kind { reg = (1 << 2) - 1, immedidate = (1 << 3) - 1 };
  bool operator()(const MCOperand &lhs, const MCOperand &rhs) const {
    if ((lhs.isReg() && rhs.isReg() && (lhs.getReg() == rhs.getReg())) ||
        (lhs.isImm() && rhs.isImm() && (lhs.getImm() == rhs.getImm())) ||
        (lhs.isExpr() && rhs.isExpr() &&
         (lhs.getExpr() ==
          rhs.getExpr()))) { // FIXME this is just comparing ptrs
      return true;
    }
    return false;
  }
};

class AMCValue {
  llvm::MCOperand operand;
  unsigned id;

public:
  AMCValue(llvm::MCOperand _operand, unsigned _id)
      : operand(_operand), id(_id) {}
  unsigned get_id() const {
    return id;
  }
  auto &get_operand() const {
    return operand;
  }
};

struct AMCValueHash {
  size_t operator()(const AMCValue &op) const {
    MCOperandHash h;
    auto op_hash = h(op.get_operand());
    return std::hash<unsigned long>()(op_hash + op.get_id());
  }
};

struct AMCValueEqual {
  bool operator()(const AMCValue &lhs, const AMCValue &rhs) const {
    MCOperandEqual op_eq;
    return op_eq(lhs.get_operand(), rhs.get_operand()) &&
           (lhs.get_id() == rhs.get_id());
  }
};

// Some variables that we need to maintain as we're performing arm-tv
std::unordered_map<llvm::MCOperand, unsigned, MCOperandHash, MCOperandEqual>
    mc_operand_id;

// Mapping between machine value and IR::value used when translating asm to
// Alive IR
std::unordered_map<AMCValue, IR::Value *, AMCValueHash, AMCValueEqual>
    mc_value_cache;

unsigned type_id_counter{0};

// Generate the required struct type for an alive2 *_overflow instructions
// FIXME: @ryan-berger padding may change, hardcoding 24 bits is a bad idea
// FIXME these type object generators should be either grouped in one class or
// be refactored in some other way.
// We should also pass something more useful than just one operand that can be
// used as a key to cache complex types as right now this function leaks memory
// This function should also be moved to utils.cpp as it will need to use
// objects that are defined there further I'm not sure if the padding matters at
// this point but the code is based on utils.cpp llvm_type2alive function that
// uses padding for struct types
auto sadd_overflow_type(MCOperand op, int size) {
  vector<IR::Type *> elems;
  vector<bool> is_padding{false, false, true};

  assert(op.isReg());
  auto add_res_ty = &get_int_type(size);
  auto add_ov_ty = &get_int_type(1);
  auto padding_ty = &get_int_type(24);
  elems.push_back(add_res_ty);
  elems.push_back(add_ov_ty);
  elems.push_back(padding_ty);
  auto ty = new IR::StructType("ty_" + to_string(type_id_counter++),
                               move(elems), move(is_padding));
  return ty;
}

auto uadd_overflow_type(MCOperand op, int size) {
  vector<IR::Type *> elems;
  vector<bool> is_padding{false, false, true};

  assert(op.isReg());
  auto add_res_ty = &get_int_type(size);
  auto add_ov_ty = &get_int_type(1);
  auto padding_ty = &get_int_type(24);
  elems.push_back(add_res_ty);
  elems.push_back(add_ov_ty);
  elems.push_back(padding_ty);
  auto ty = new IR::StructType("ty_" + to_string(type_id_counter++),
                               move(elems), move(is_padding));
  return ty;
}

// get_cur_op_id takes in an MCOperand, and returns the id for the
// given operand.
//
// The operand is constructed using the name of a non-special purpose register
// used in any given instruction.
//
// A previous instruction will associate its registers to operand ids in order
// for later lookup by this function. All lookups must succeed, or else
// translation has gone very wrong.
unsigned get_cur_op_id(llvm::MCOperand &mc_op) {
  if (mc_op.isImm()) {
    return 0;
  } else if (mc_op.isReg()) {
    auto I = mc_operand_id.find(mc_op);

    // the operand id is not found
    assert(I != mc_operand_id.end());
    return mc_operand_id[mc_op];
  } else {
    cout << "ERROR: [get_cur_op_id] unsupported MCOperand type" << '\n';
    exit(0);
  }
}

unsigned get_new_op_id(llvm::MCOperand &mc_op) {
  if (mc_op.isImm()) {
    return 0;
  } else if (mc_op.isReg()) {
    auto I = mc_operand_id.find(mc_op);
    if (I == mc_operand_id.end()) {
      mc_operand_id.emplace(mc_op, 0);
      return 0;
    } else {
      mc_operand_id[mc_op] += 1;
      return mc_operand_id[mc_op];
    }
  } else {
    cout << "ERROR: [get_new_op_id] unsupported MCOperand type" << '\n';
    exit(0);
  }
}

void mc_add_identifier(llvm::MCOperand &mc_op, unsigned op_id, IR::Value &v) {
  assert(mc_op.isReg()); // FIXME
  auto mc_val = AMCValue(mc_op, op_id);
  mc_value_cache.emplace(mc_val, &v);
}

IR::Value *mc_get_operand(AMCValue mc_val) {
  if (auto I = mc_value_cache.find(mc_val); I != mc_value_cache.end())
    return I->second;
  return nullptr;
}

// Code taken from llvm. This should be okay for now. But we generally
// don't want to trust the llvm implementation so we need to complete my
// implementation at function decode_bit_mask
static inline uint64_t ror(uint64_t elt, unsigned size) {
  return ((elt & 1) << (size - 1)) | (elt >> 1);
}

/// decodeLogicalImmediate - Decode a logical immediate value in the form
/// "N:immr:imms" (where the immr and imms fields are each 6 bits) into the
/// integer value it represents with regSize bits.
static inline uint64_t decodeLogicalImmediate(uint64_t val, unsigned regSize) {
  // Extract the N, imms, and immr fields.
  unsigned N = (val >> 12) & 1;
  unsigned immr = (val >> 6) & 0x3f;
  unsigned imms = val & 0x3f;

  assert((regSize == 64 || N == 0) && "undefined logical immediate encoding");
  int len = 31 - llvm::countLeadingZeros((N << 6) | (~imms & 0x3f));
  assert(len >= 0 && "undefined logical immediate encoding");
  unsigned size = (1 << len);
  unsigned R = immr & (size - 1);
  unsigned S = imms & (size - 1);
  assert(S != size - 1 && "undefined logical immediate encoding");
  uint64_t pattern = (1ULL << (S + 1)) - 1;
  for (unsigned i = 0; i < R; ++i)
    pattern = ror(pattern, size);

  // Replicate the pattern to fill the regSize.
  while (size != regSize) {
    pattern |= (pattern << size);
    size *= 2;
  }
  return pattern;
}

// adapted from the arm ISA
// Decode AArch64 bitfield and logical immediate masks which use a similar
// encoding structure
std::tuple<uint64_t, uint64_t> decode_bit_mask(bool immNBit, uint32_t _imms,
                                               uint32_t _immr, bool immediate,
                                               int M) {
  llvm::APInt imms(6, _imms);
  llvm::APInt immr(6, _immr);

  auto notImm = APInt(6, _imms);
  notImm.flipAllBits();

  auto concatted = APInt(1, (immNBit ? 1 : 0)).concat(notImm);
  auto len = concatted.getBitWidth() - concatted.countLeadingZeros() - 1;

  // Undefined behavior
  assert(len >= 1);
  assert(M >= (1 << len));

  auto levels = llvm::APInt::getAllOnes(len).zextOrSelf(6);

  auto S = (imms & levels);
  auto R = (immr & levels);

  auto diff = S - R;
  auto esize = (1 << len);

  auto d = diff.ashr(len - 1);

  auto welem = llvm::APInt::getAllOnes(S.getZExtValue() + 1).zextOrSelf(esize);
  auto telem = llvm::APInt::getAllOnes(d.getZExtValue() + 1).zextOrSelf(esize);

  return {welem.rotr(R).getZExtValue(), telem.getZExtValue()};
}

// Values currently holding ZNCV bits, respectively
IR::Value *cur_v{nullptr};
IR::Value *cur_z{nullptr};
IR::Value *cur_n{nullptr};
IR::Value *cur_c{nullptr};

// DISABLE_REGISTER_LOOKUP will disable code that resolves 32/64 bit registers
// to the same use.
// FIXME make SSA to support 32/64 bit registers as same use
#define DISABLE_REGISTER_LOOKUP true

set<int> s_flag = {
    AArch64::ADDSWrs, AArch64::ADDSWri, AArch64::ADDSXrs, AArch64::ADDSXri,
    AArch64::SUBSWrs, AArch64::SUBSWri, AArch64::SUBSXrs, AArch64::SUBXri,
};

set<int> instrs_32 = {
    AArch64::ADDSWrs,  AArch64::ADDSWri, AArch64::ADDWrs,  AArch64::ADDWri,
    AArch64::SUBWri,   AArch64::SUBWrs,  AArch64::SUBSWrs, AArch64::SUBSWri,
    AArch64::SBFMWri,  AArch64::CSELWr,  AArch64::ANDWri,  AArch64::ANDWrr,
    AArch64::MADDWrrr, AArch64::EORWri,  AArch64::CSINVWr, AArch64::CSINCWr,
    AArch64::MOVZWi,   AArch64::MOVNWi,  AArch64::MOVKWi,  AArch64::LSLVWr,
    AArch64::LSRVWr,   AArch64::ORNWrs,  AArch64::UBFMWri, AArch64::BFMWri,
    AArch64::ORRWrs,   AArch64::SDIVWr,  AArch64::UDIVWr,
};

set<int> instrs_64 = {
    AArch64::ADDSXrs,  AArch64::ADDSXri, AArch64::ADDXrs,  AArch64::ADDXri,
    AArch64::SUBXri,   AArch64::SUBXrs,  AArch64::SUBSXrs, AArch64::SUBSXri,
    AArch64::SBFMXri,  AArch64::CSELXr,  AArch64::ANDXri,  AArch64::ANDXrr,
    AArch64::MADDXrrr, AArch64::EORXri,  AArch64::CSINVXr, AArch64::CSINCXr,
    AArch64::MOVZXi,   AArch64::MOVNXi,  AArch64::MOVKXi,  AArch64::LSLVXr,
    AArch64::LSRVXr,   AArch64::ORNXrs,  AArch64::UBFMXri, AArch64::BFMXri,
    AArch64::ORRXrs,   AArch64::SDIVXr,  AArch64::UDIVXr,
};

int get_size(int instr) {
  if (instrs_32.contains(instr)) {
    return 32;
  }

  if (instrs_64.contains(instr)) {
    return 64;
  }

  assert(instr == AArch64::RET && "instruction not found");
  return 0;
}

bool has_s(int instr) {
  return s_flag.contains(instr);
}

class arm2alive_ {
  MCFunction &MF;
  const llvm::DataLayout &DL;
  std::optional<IR::Function> &srcFn;
  IR::BasicBlock *BB;

  MCInstPrinter *instrPrinter;
  MCRegisterInfo *registerInfo;

  MCInst curInst;
  unsigned int curId;

  std::vector<std::unique_ptr<IR::Instr>> visit_error(MCInstWrapper &I) {
    llvm::errs() << "ERROR: Unsupported arm instruction: "
                 << instrPrinter->getOpcodeName(I.instr.getOpcode());
    exit(1); // for now lets exit the program if the arm instruction is not
             // supported
  }

  IR::Value *get_identifier(MCOperand &op) {
    auto search_val = AMCValue(op, get_cur_op_id(op));
    auto val = mc_get_operand(search_val);

    assert(val != NULL);

    return val;
  }

  // TODO: figure out how to deal with mixed-width function arguments.
  //  This function is buggy and won't handle support it. If we have anything
  //  other than 64 bit arguments, we may want to do some sort of conversion
  // TODO: make it so that lshr generates code on register lookups
  //  some instructions make use of this, and the semantics need to be worked
  //  out
  IR::Value *get_value(MCOperand &op, int lshr = 0, size_t size = 32) {
    assert(op.isImm() || op.isReg());
    if (op.isImm()) {
      // FIXME, figure out immediate size
      return make_intconst(op.getImm() << lshr, size);
    }

    if (op.getReg() == AArch64::WZR) {
      return make_intconst(0, 32);
    }

    if (op.getReg() == AArch64::XZR) {
      return make_intconst(0, 64);
    }

    auto val = get_identifier(op);
    if (size == 64 || DISABLE_REGISTER_LOOKUP) {
      return val;
    }

    auto ty = &get_int_type(32);
    auto trunc = make_unique<IR::ConversionOp>(*ty, move(next_name()), *val,
                                               IR::ConversionOp::Trunc);
    val = trunc.get();

    BB->addInstr(move(trunc));
    return val;
  }

  unsigned long next_id() {
    curId = get_new_op_id(curInst.getOperand(0));
    return curId;
  }

  std::string next_name() {
    return "%" + std::to_string(curInst.getOperand(0).getReg()) + "_" +
           std::to_string(next_id());
  }

  void add_identifier(IR::Value &v) {
    mc_add_identifier(curInst.getOperand(0), curId, v);
  }

  // store will store an IR value using the current instruction's destination
  // register.
  // All values are kept track of in their full-width counterparts to simulate
  // registers. For example, a x0 register would be kept track of in the bottom
  // bits of w0. Optionally, there is an "s" or signed flag that can be used
  // when writing smaller bit-width values to half or full-width registers which
  // will perform a small sign extension procedure.
  void store(IR::Value &v, bool s = false) {
    // if v.bits() == 64, regSize == 64 because of above assertion
    if (v.bits() == 64 || DISABLE_REGISTER_LOOKUP) {
      add_identifier(v);
      return;
    }

    // FIXME: remove assert
    assert(false && "unimplemented & buggy code path");

    // FIXME: get register bit width for further computation
    size_t regSize = 32;

    // regSize should only be 32/64
    assert(regSize == 32 || regSize == 64);

    // if the s flag is set, the value is smaller than 32 bits,
    // and the register we are storing it in _is_ 32 bits, we sign extend
    // to 32 bits before zero-extending to 64
    if (s && regSize == 32 && v.bits() < 32) {
      auto ty = &get_int_type(32);
      auto sext32 = make_unique<IR::ConversionOp>(*ty, move(next_name()), v,
                                                  IR::ConversionOp::SExt);

      ty = &get_int_type(64);
      auto zext64 = make_unique<IR::ConversionOp>(
          *ty, move(next_name()), *sext32.get(), IR::ConversionOp::ZExt);

      BB->addInstr(move(sext32));

      add_identifier(*zext64.get());
      BB->addInstr(move(zext64));
      return;
    }

    auto op = s ? IR::ConversionOp::SExt : IR::ConversionOp::ZExt;
    auto ty = &get_int_type(64);
    auto new_val = make_unique<IR::ConversionOp>(*ty, move(next_name()), v, op);

    add_identifier(*new_val.get());
    BB->addInstr(move(new_val));
  }

  IR::Value *evaluate_condition(uint64_t cond) {
    // cond<0> == '1' && cond != '1111'
    auto invert_bit = (cond & 1) && (cond != 15);

    cond >>= 1;

    IR::Value *res = nullptr;
    switch (cond) {
    case 0:
      res = cur_z;
      break; // EQ/NE
    case 1:
      res = cur_c;
      break; // CS/CC
    case 2:
      res = cur_n;
      break; // MI/PL
    case 3:
      res = cur_v;
      break; // VS/VC
    case 4:  // HI/LS: PSTATE.C == '1' && PSTATE.Z == '0'
    {
      assert(cur_c != nullptr && cur_z != nullptr &&
             "HI/LS requires C and Z bits to be generated");
      auto ty = &get_int_type(1);

      // C == 1
      auto c_cond = add_instr<IR::ICmp>(*ty, move(next_name()), IR::ICmp::EQ,
                                        *cur_c, *make_intconst(1, 1));
      // Z == 0
      auto z_cond = add_instr<IR::ICmp>(*ty, move(next_name()), IR::ICmp::EQ,
                                        *cur_z, *make_intconst(0, 1));
      // C == 1 && Z == 0
      res = add_instr<IR::BinOp>(*ty, move(next_name()), *c_cond, *z_cond,
                                 IR::BinOp::And);
    }
    case 5: // GE/LT PSTATE.N == PSTATE.V
    {
      assert(cur_n != nullptr && cur_v != nullptr &&
             "GE/LT requires N and V bits to be generated");
      auto ty = &get_int_type(1);

      res = add_instr<IR::ICmp>(*ty, move(next_name()), IR::ICmp::EQ, *cur_n,
                                *cur_v);
      break;
    }
    case 6: // GT/LE PSTATE.N == PSTATE.V && PSTATE.Z == 0
    {
      assert(cur_n != nullptr && cur_v != nullptr && cur_z != nullptr &&
             "GT/LE requires N, V and Z bits to be generated");
      auto ty = &get_int_type(1);

      auto n_eq_v = add_instr<IR::ICmp>(*ty, move(next_name()), IR::ICmp::EQ,
                                        *cur_n, *cur_v);
      auto z_cond = add_instr<IR::ICmp>(*ty, move(next_name()), IR::ICmp::EQ,
                                        *cur_z, *make_intconst(0, 1));

      res = add_instr<IR::BinOp>(*ty, move(next_name()), *n_eq_v, *z_cond,
                                 IR::BinOp::And);
      break;
    }
    case 7: {
      res = make_intconst(1, 1);
      break;
    }
    default:
      assert(false && "invalid condition code");
      break;
    }

    assert(res != nullptr && "condition code was not generated");
    if (invert_bit) {
      auto ty = &get_int_type(1);
      auto one = make_intconst(1, 1);
      auto inverted = make_unique<IR::BinOp>(*ty, move(next_name()), *res, *one,
                                             IR::BinOp::Xor);
      res = inverted.get();
      BB->addInstr(move(inverted));
    }

    return res;
  }

  void set_z(IR::Value *val) {
    auto typ = &get_int_type(1);
    auto zero = make_intconst(0, val->bits());

    auto z = make_unique<IR::ICmp>(*typ, move(next_name()), IR::ICmp::Cond::EQ,
                                   *val, *zero);

    cur_z = z.get();
    BB->addInstr(move(z));
  }

  void set_n(IR::Value *val) {
    cout << val->bits() << "\n";
    auto typ = &get_int_type(1);
    auto zero = make_intconst(0, val->bits());

    auto n = make_unique<IR::ICmp>(*typ, move(next_name()), IR::ICmp::Cond::SLT,
                                   *val, *zero);

    cur_n = n.get();
    BB->addInstr(move(n));
  }

  // add_instr is a thin wrapper around make_unique which adds an instruction to
  // the current basic block, and returns a pointer to the value.
  template <typename _Tp, typename... _Args> _Tp *add_instr(_Args &&...__args) {
    assert(BB != nullptr);
    auto instr = make_unique<_Tp>(std::forward<_Args>(__args)...);
    auto ret = instr.get();

    BB->addInstr(move(instr));

    return ret;
  }

public:
  arm2alive_(MCFunction &MF, const llvm::DataLayout &DL,
             std::optional<IR::Function> &srcFn, MCInstPrinter *instrPrinter,
             MCRegisterInfo *registerInfo)
      : MF(MF), DL(DL), srcFn(srcFn), instrPrinter(instrPrinter),
        registerInfo(registerInfo) {}

  // Rudimentary function to visit an MCInstWrapper instructions and convert it
  // to alive IR Ideally would want a nicer designed interface, but I opted for
  // simplicity to get the initial prototype.
  // FIXME add support for more arm instructions
  // FIXME generate code for setting NZCV flags and other changes to arm PSTATE
  void mc_visit(MCInstWrapper &I) {
    std::vector<std::unique_ptr<IR::Instr>> res;
    auto opcode = I.getOpcode();
    auto &mc_inst = I.getMCInst();
    curInst = mc_inst;

    auto size = get_size(opcode);
    auto ty = &get_int_type(size);

    switch (opcode) {
    case AArch64::ADDWrs:
    case AArch64::ADDWri:
    case AArch64::ADDSWrs:
    case AArch64::ADDSWri:
    case AArch64::ADDXrs:
    case AArch64::ADDXri:
    case AArch64::ADDSXrs:
    case AArch64::ADDSXri: {
      auto a = get_value(mc_inst.getOperand(1), size = size);
      auto b = get_value(mc_inst.getOperand(2), mc_inst.getOperand(3).getImm(),
                         size);

      auto overflow_type = sadd_overflow_type(mc_inst.getOperand(1), size);
      auto sadd = add_instr<IR::BinOp>(*overflow_type, move(next_name()), *a,
                                       *b, IR::BinOp::SAdd_Overflow);

      auto result = add_instr<IR::ExtractValue>(*ty, move(next_name()), *sadd);
      result->addIdx(0);

      if (has_s(opcode)) {
        auto i1 = &get_int_type(1);
        // generate v flag from SAdd result
        auto new_v = add_instr<IR::ExtractValue>(*i1, move(next_name()), *sadd);
        new_v->addIdx(1);

        // generate c flag from UAdd result
        auto uadd = add_instr<IR::BinOp>(*overflow_type, move(next_name()), *a,
                                         *b, IR::BinOp::UAdd_Overflow);
        auto new_c = add_instr<IR::ExtractValue>(*i1, move(next_name()), *uadd);
        new_c->addIdx(1);

        cur_v = new_v;
        cur_c = new_c;
        set_n(result);
        set_z(result);
      }

      store(*result);
      break;
    }
    case AArch64::SUBWri:
    case AArch64::SUBWrs:
    case AArch64::SUBSWrs:
    case AArch64::SUBSWri:
    case AArch64::SUBXri:
    case AArch64::SUBXrs:
    case AArch64::SUBSXrs:
    case AArch64::SUBSXri: {
      assert(mc_inst.getNumOperands() == 4); // dst, lhs, rhs, shift amt
      assert(mc_inst.getOperand(3).isImm());

      auto ty_ptr = sadd_overflow_type(mc_inst.getOperand(1), size);

      // convert lhs, rhs operands to IR::Values
      auto a = get_value(mc_inst.getOperand(1));
      auto b = get_value(mc_inst.getOperand(2), mc_inst.getOperand(3).getImm());

      // make sure that lhs and rhs conversion succeeded, type lookup succeeded
      if (!ty || !a || !b)
        visit_error(I);

      // SSub_Overflow returns: {iN (result), i1 (overflow), i24 (padding)}
      auto ssub = add_instr<IR::BinOp>(*ty_ptr, move(next_name()), *a, *b,
                                       IR::BinOp::SSub_Overflow);
      auto result = add_instr<IR::ExtractValue>(*ty, move(next_name()), *ssub);
      result->addIdx(0);

      if (has_s(opcode)) {
        auto ty_i1 = &get_int_type(1);

        auto new_v =
            add_instr<IR::ExtractValue>(*ty_i1, move(next_name()), *ssub);
        new_v->addIdx(1);

        auto uadd_typ = uadd_overflow_type(mc_inst.getOperand(1), size);

        // generate code for subtract borrow flag. This can be formulated as
        // a + not(b), or a + (-b)
        auto not_b = add_instr<IR::BinOp>(
            *ty, move(next_name()), *b, *make_intconst(-1, 32), IR::BinOp::Xor);

        auto uadd = add_instr<IR::BinOp>(*uadd_typ, move(next_name()), *a,
                                         *not_b, IR::BinOp::UAdd_Overflow);

        // extract the c flag from UAdd_Overflow result
        auto new_c =
            add_instr<IR::ExtractValue>(*ty_i1, move(next_name()), *uadd);
        new_c->addIdx(1);

        cur_v = new_v;
        cur_c = new_c;
        set_z(result);
        set_n(result);
      }

      store(*result);
      break;
    }
    case AArch64::CSELWr:
    case AArch64::CSELXr: {
      assert(mc_inst.getNumOperands() == 4); // dst, lhs, rhs, cond
      // TODO decode condition and find the approprate cond val
      assert(mc_inst.getOperand(1).isReg() && mc_inst.getOperand(2).isReg());
      assert(mc_inst.getOperand(3).isImm());

      auto a = get_value(mc_inst.getOperand(1));
      auto b = get_value(mc_inst.getOperand(2));

      auto cond_val_imm = mc_inst.getOperand(3).getImm();
      auto cond_val = evaluate_condition(cond_val_imm);

      if (!ty || !a || !b)
        visit_error(I);

      auto ret =
          add_instr<IR::Select>(*ty, move(next_name()), *cond_val, *a, *b);
      store(*ret);
      break;
    }
    case AArch64::ANDWri:
    case AArch64::ANDWrr:
    case AArch64::ANDXri:
    case AArch64::ANDXrr: {
      auto and_op = add_instr<IR::BinOp>(
          *ty, next_name(), *get_value(mc_inst.getOperand(1), 0, size),
          *get_value(mc_inst.getOperand(2), mc_inst.getOperand(3).getImm(),
                     size),
          IR::BinOp::And);

      store(*and_op);
      break;
    }
    case AArch64::MADDWrrr:
    case AArch64::MADDXrrr: {
      auto mul_lhs = get_value(mc_inst.getOperand(1), 0, size);
      auto mul_rhs = get_value(mc_inst.getOperand(2), 0, size);
      auto addend = get_value(mc_inst.getOperand(3), 0, size);

      auto mul = add_instr<IR::BinOp>(*ty, move(next_name()), *mul_lhs,
                                      *mul_rhs, IR::BinOp::Mul);
      auto add = add_instr<IR::BinOp>(*ty, move(next_name()), *mul, *addend,
                                      IR::BinOp::Add);
      store(*add);
      break;
    }
    case AArch64::SBFMWri:
    case AArch64::SBFMXri: {
      assert(mc_inst.getNumOperands() == 4); // dst, src, imm1, imm2
      assert(mc_inst.getOperand(2).isImm() && mc_inst.getOperand(3).isImm());

      auto a = get_value(mc_inst.getOperand(1));

      auto shift_amt = make_intconst(mc_inst.getOperand(2).getImm(), size);
      if (!ty || !a || !shift_amt)
        visit_error(I);

      auto res = add_instr<IR::BinOp>(*ty, move(next_name()), *a, *shift_amt,
                                      IR::BinOp::AShr);
      store(*res);
      break;
    }
    case AArch64::EORWri:
    case AArch64::EORXri: {
      assert(mc_inst.getNumOperands() == 3); // dst, src, imm
      assert(mc_inst.getOperand(1).isReg() && mc_inst.getOperand(2).isImm());

      auto a = get_value(mc_inst.getOperand(1));
      auto decoded_immediate =
          decodeLogicalImmediate(mc_inst.getOperand(2).getImm(), size);
      auto imm_val = make_intconst(decoded_immediate,
                                   size); // FIXME, need to decode immediate val
      if (!ty || !a || !imm_val)
        visit_error(I);

      auto res = add_instr<IR::BinOp>(*ty, move(next_name()), *a, *imm_val,
                                      IR::BinOp::Xor);
      store(*res);
      break;
    }
    case AArch64::CSINVWr:
    case AArch64::CSINVXr: {
      // csinv dst, a, b, cond
      // if (cond) a else ~b
      assert(mc_inst.getNumOperands() == 4); // dst, lhs, rhs, cond
      // TODO decode condition and find the approprate cond val
      assert(mc_inst.getOperand(1).isReg() && mc_inst.getOperand(2).isReg());
      assert(mc_inst.getOperand(3).isImm());

      auto a = get_value(mc_inst.getOperand(1));
      auto b = get_value(mc_inst.getOperand(2));

      auto cond_val_imm = mc_inst.getOperand(3).getImm();
      auto cond_val = evaluate_condition(cond_val_imm);

      if (!ty || !a || !b)
        visit_error(I);

      auto neg_one = make_intconst(-1, size);
      auto negated_b = add_instr<IR::BinOp>(*ty, move(next_name()), *b,
                                            *neg_one, IR::BinOp::Xor);

      auto ret = add_instr<IR::Select>(*ty, move(next_name()), *cond_val, *a,
                                       *negated_b);
      store(*ret);
      break;
    }
    case AArch64::CSINCWr:
    case AArch64::CSINCXr: {
      assert(mc_inst.getOperand(1).isReg() && mc_inst.getOperand(2).isReg());
      assert(mc_inst.getOperand(3).isImm());

      auto a = get_value(mc_inst.getOperand(1));
      auto b = get_value(mc_inst.getOperand(2));

      auto cond_val_imm = mc_inst.getOperand(3).getImm();
      auto cond_val = evaluate_condition(cond_val_imm);

      auto inc =
          add_instr<IR::BinOp>(*ty, move(next_name()), *b,
                               *make_intconst(1, ty->bits()), IR::BinOp::Add);
      auto sel =
          add_instr<IR::Select>(*ty, move(next_name()), *cond_val, *a, *inc);

      store(*sel);
      break;
    }
    case AArch64::MOVZWi:
    case AArch64::MOVZXi: {
      assert(mc_inst.getOperand(0).isReg());
      assert(mc_inst.getOperand(1).isImm());

      auto lhs = get_value(mc_inst.getOperand(1),
                           mc_inst.getOperand(2).getImm(), size);

      auto rhs = make_intconst(0, size);
      auto ident =
          add_instr<IR::BinOp>(*ty, next_name(), *lhs, *rhs, IR::BinOp::Add);

      store(*ident);
      break;
    }
    case AArch64::MOVNWi:
    case AArch64::MOVNXi: {
      assert(mc_inst.getOperand(0).isReg());
      assert(mc_inst.getOperand(1).isImm());
      assert(mc_inst.getOperand(2).isImm());

      auto lhs =
          get_value(mc_inst.getOperand(1), mc_inst.getOperand(2).getImm());

      auto neg_one = make_intconst(-1, size);
      auto not_lhs = add_instr<IR::BinOp>(*ty, move(next_name()), *lhs,
                                          *neg_one, IR::BinOp::Xor);

      auto rhs = make_intconst(0, size);
      auto ident = add_instr<IR::BinOp>(*ty, move(next_name()), *not_lhs, *rhs,
                                        IR::BinOp::Add);
      store(*ident);
      break;
    }
    case AArch64::LSLVWr:
    case AArch64::LSLVXr: {

      auto zero = make_intconst(0, size);
      auto lhs = get_value(mc_inst.getOperand(1));
      auto rhs = get_value(mc_inst.getOperand(2));

      auto exp = add_instr<IR::TernaryOp>(*ty, move(next_name()), *lhs, *zero,
                                          *rhs, IR::TernaryOp::FShl);
      store(*exp);
      break;
    }
    case AArch64::LSRVWr:
    case AArch64::LSRVXr: {

      auto zero = make_intconst(0, size);
      auto lhs = get_value(mc_inst.getOperand(1));
      auto rhs = get_value(mc_inst.getOperand(2));

      auto exp = add_instr<IR::TernaryOp>(*ty, move(next_name()), *zero, *lhs,
                                          *rhs, IR::TernaryOp::FShr);
      store(*exp);
      break;
    }
    case AArch64::ORNWrs:
    case AArch64::ORNXrs: {
      auto lhs = get_value(mc_inst.getOperand(1));
      auto rhs = get_value(mc_inst.getOperand(2));

      auto neg_one = make_intconst(-1, 32);
      auto not_rhs = make_unique<IR::BinOp>(*ty, move(next_name()), *rhs,
                                            *neg_one, IR::BinOp::Xor);

      auto ident = make_unique<IR::BinOp>(*ty, move(next_name()), *lhs,
                                          *not_rhs, IR::BinOp::Or);
      auto ident_ptr = ident.get();

      BB->addInstr(move(not_rhs));
      BB->addInstr(move(ident));
      store(*ident_ptr);
      break;
    }
      // TODO: support MOVKW alonside our MOVKW support
    case AArch64::MOVKWi: {
      auto dest = get_value(mc_inst.getOperand(1));
      auto lhs =
          get_value(mc_inst.getOperand(2), mc_inst.getOperand(3).getImm());

      auto bottom_bits =
          make_intconst(~(0xFFFF << mc_inst.getOperand(3).getImm()), 32);
      auto cleared = add_instr<IR::BinOp>(*ty, move(next_name()), *dest,
                                            *bottom_bits, IR::BinOp::And);

      auto ident = add_instr<IR::BinOp>(*ty, move(next_name()), *cleared,
                                          *lhs, IR::BinOp::Or);
      store(*ident);
      break;
    }
    case AArch64::UBFMWri:
    case AArch64::UBFMXri: {
      auto src = get_value(mc_inst.getOperand(1));
      auto immr = mc_inst.getOperand(2).getImm();
      auto imms = mc_inst.getOperand(3).getImm();
      auto r = make_intconst(immr, size);

      auto [wmaskInt, tmaskInt] =
          decode_bit_mask(false, imms, immr, false, size);
      auto wmask = make_intconst(wmaskInt, size);
      auto tmask = make_intconst(tmaskInt, size);

      auto ror = add_instr<IR::TernaryOp>(*ty, move(next_name()), *src, *src,
                                          *r, IR::TernaryOp::FShr);
      auto bot = add_instr<IR::BinOp>(*ty, move(next_name()), *ror, *wmask,
                                      IR::BinOp::And);
      auto dst = add_instr<IR::BinOp>(*ty, move(next_name()), *bot, *tmask,
                                      IR::BinOp::And);
      store(*dst);
      break;
    }
    case AArch64::BFMWri:
    case AArch64::BFMXri: {
      auto dst = get_value(mc_inst.getOperand(1));
      auto src = get_value(mc_inst.getOperand(2));

      auto immr = mc_inst.getOperand(3).getImm();
      auto imms = mc_inst.getOperand(4).getImm();
      auto r = make_intconst(immr, size);

      auto [wmaskInt, tmaskInt] =
          decode_bit_mask(false, imms, immr, false, size);
      auto wmask = make_intconst(wmaskInt, size);
      auto tmask = make_intconst(tmaskInt, size);

      auto not_wmask = make_intconst(~wmaskInt, size);
      auto not_tmask = make_intconst(~tmaskInt, size);

      auto bot_lhs = add_instr<IR::BinOp>(*ty, move(next_name()), *dst,
                                          *not_wmask, IR::BinOp::And);

      auto bot_ror = add_instr<IR::TernaryOp>(*ty, move(next_name()), *src,
                                              *src, *r, IR::TernaryOp::FShr);

      auto bot_rhs = add_instr<IR::BinOp>(*ty, move(next_name()), *bot_ror,
                                          *wmask, IR::BinOp::And);

      auto bot = add_instr<IR::BinOp>(*ty, move(next_name()), *bot_lhs,
                                      *bot_rhs, IR::BinOp::Or);

      auto res_lhs = add_instr<IR::BinOp>(*ty, move(next_name()), *dst,
                                          *not_tmask, IR::BinOp::And);

      auto res_rhs = add_instr<IR::BinOp>(*ty, move(next_name()), *bot, *tmask,
                                          IR::BinOp::And);

      auto result = add_instr<IR::BinOp>(*ty, move(next_name()), *res_lhs,
                                         *res_rhs, IR::BinOp::Or);

      store(*result);
      break;
    }
    case AArch64::ORRWrs:
    case AArch64::ORRXrs: {
      // don't support shifts because I'm lazy
      assert(mc_inst.getOperand(3).getImm() == 0);

      auto lhs = get_value(mc_inst.getOperand(1));
      auto rhs = get_value(mc_inst.getOperand(2));

      auto result = add_instr<IR::BinOp>(*ty, move(next_name()), *lhs, *rhs,
                                         IR::BinOp::Or);
      store(*result);
      break;
    }
    case AArch64::SDIVWr:
    case AArch64::SDIVXr: {
      auto lhs = get_value(mc_inst.getOperand(1));
      auto rhs = get_value(mc_inst.getOperand(2));

      auto result = add_instr<IR::BinOp>(*ty, move(next_name()), *lhs, *rhs,
                                         IR::BinOp::SDiv);
      store(*result);
      break;
    }
    case AArch64::UDIVWr:
    case AArch64::UDIVXr: {
      auto lhs = get_value(mc_inst.getOperand(1));
      auto rhs = get_value(mc_inst.getOperand(2));

      auto result = add_instr<IR::BinOp>(*ty, move(next_name()), *lhs, *rhs,
                                         IR::BinOp::SDiv);
      store(*result);
      break;
    }
    case AArch64::RET: {
      // for now we're assuming that the function returns an integer value
      assert(mc_inst.getNumOperands() == 1);
      // TODO: this seems like a hack. Change maybe?
      auto ty = &get_int_type(srcFn->getType().bits());
      // TODO: make sure we are getting the correct bit width as well
      auto val = get_value(mc_inst.getOperand(0));

      BB->addInstr(make_unique<IR::Return>(*ty, *val));
      break;
    }
    default:
      visit_error(I);
    }
  }

  std::optional<IR::Function> run() {
    // for now assume that return type is 32-bit integer
    assert(srcFn->getType().isIntType());
    auto func_return_type = &get_int_type(srcFn->getType().bits());
    if (!func_return_type)
      return {};

    IR::Function Fn(*func_return_type, MF.getName());
    reset_state(Fn);

    // set function attribute to include noundef
    // Fn.getFnAttrs().set(IR::FnAttrs::NoUndef);
    // TODO need to disable poison values as well. Figure how to do so

    // FIXME infer function attributes if any
    // Most likely need to emit and read the debug info from the MCStreamer

    int argNum = 0;
    for (auto &v : srcFn->getInputs()) {
      auto &typ = v.getType();
      assert(typ.isIntType());

      // FIXME. Do a switch statement to figure out which register to start from
      auto start = typ.bits() == 32 ? AArch64::W0 : AArch64::X0;
      auto operand = MCOperand::createReg(start + (argNum++));

      std::string operand_name = "%" + std::to_string(operand.getReg());
      IR::ParamAttrs attrs;
      attrs.set(IR::ParamAttrs::NoUndef);

      auto val = make_unique<IR::Input>(typ, move(operand_name), move(attrs));
      mc_add_identifier(operand, get_new_op_id(operand), *val.get());
      Fn.addInput(move(val));
    }

    // Create Fn's BBs
    vector<pair<IR::BasicBlock *, MCBasicBlock *>> sorted_bbs;
    {
      util::edgesTy edges;
      vector<MCBasicBlock *> bbs;
      unordered_map<MCBasicBlock *, unsigned> bb_map;

      auto bb_num = [&](MCBasicBlock *bb) {
        auto [I, inserted] = bb_map.emplace(bb, bbs.size());
        if (inserted) {
          bbs.emplace_back(bb);
          edges.emplace_back();
        }
        return I->second;
      };

      for (auto &bb : MF.BBs) {
        auto n = bb_num(&bb);
        for (auto it = bb.succBegin(); it != bb.succEnd(); ++it) {
          auto succ_ptr = *it;
          auto n_dst = bb_num(succ_ptr);
          edges[n].emplace(n_dst);
        }
      }

      for (auto v : top_sort(edges)) {
        sorted_bbs.emplace_back(&Fn.getBB(bbs[v]->getName()), bbs[v]);
      }
    }

    for (auto &[alive_bb, mc_bb] : sorted_bbs) {
      BB = alive_bb;
      auto mc_instrs = mc_bb->getInstrs();
      for (auto &mc_instr : mc_instrs) {
        mc_visit(mc_instr);
      }

      auto instrs = BB->instrs();
      if (instrs.begin() != instrs.end()) {
        // if there are any instructions in the BB, keep processing
        continue;
      }

      Fn.print(cout << "\n----------partially-lifted-arm-target----------\n");
      return {};
    }

    return move(Fn);
  }
};

// Convert an MCFucntion to IR::Function
// Adapted from llvm2alive_ in llvm2alive.cpp with some simplifying assumptions
// FIXME for now, we are making a lot of simplifying assumptions like assuming
// types of arguments.
std::optional<IR::Function> arm2alive(MCFunction &MF,
                                      const llvm::DataLayout &DL,
                                      std::optional<IR::Function> &srcFn,
                                      MCInstPrinter *instrPrinter,
                                      MCRegisterInfo *registerInfo) {
  return arm2alive_(MF, DL, srcFn, instrPrinter, registerInfo).run();
}

// We're overriding MCStreamerWrapper to generate an MCFunction
// from the arm assembly. MCStreamerWrapper provides callbacks to handle
// different parts of the assembly file. The main callbacks that we're
// using right now are emitInstruction and emitLabel to access the
// instruction and labels in the arm assembly.
//
// FIXME for now, we're using this class to generate the MCFunction and
// also print the MCFunction. we should move this implementation somewhere else
// TODO we'll need to implement some the other callbacks to extract more
// information from the asm file. For example, it would be useful to extract
// debug info to determine the number of function parameters.
class MCStreamerWrapper final : public llvm::MCStreamer {
  enum ASMLine { none = 0, label = 1, non_term_instr = 2, terminator = 3 };

private:
  MCBasicBlock *CurBlock{nullptr};
  std::string CurLabel;
  bool first_label{true};
  unsigned prev_line{0};
  std::map<std::string, std::vector<MCInstWrapper> *> Label2Block;
  std::vector<std::string> LabelNames;
  llvm::MCInstrAnalysis *Ana_ptr;
  llvm::MCInstPrinter *IP_ptr;
  llvm::MCRegisterInfo *MRI_ptr;

public:
  MCFunction MF;
  unsigned cnt{0};
  std::vector<llvm::MCInst> Insts;
  std::vector<MCInstWrapper> W_Insts;
  std::vector<std::vector<MCInstWrapper>> Blocks;

  MCStreamerWrapper(llvm::MCContext &Context, llvm::MCInstrAnalysis *_Ana_ptr,
                    llvm::MCInstPrinter *_IP_ptr,
                    llvm::MCRegisterInfo *_MRI_ptr)
      : MCStreamer(Context), Ana_ptr(_Ana_ptr), IP_ptr(_IP_ptr),
        MRI_ptr(_MRI_ptr) {}

  // We only want to intercept the emission of new instructions.
  virtual void
  emitInstruction(const llvm::MCInst &Inst,
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
    } else {
      prev_line = ASMLine::non_term_instr;
    }
    auto &inst_ref = Cur_Inst.getMCInst();
    auto num_operands = inst_ref.getNumOperands();
    for (unsigned i = 0; i < num_operands; ++i) {
      auto op = inst_ref.getOperand(i);
      if (op.isExpr()) {
        auto expr = op.getExpr();
        if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
          const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
          const MCSymbol &Sym = SRE.getSymbol();
          errs() << "target label : " << Sym.getName()
                 << ", offset=" << Sym.getOffset()
                 << '\n'; // FIXME remove when done
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

  bool emitSymbolAttribute(llvm::MCSymbol *Symbol,
                           llvm::MCSymbolAttr Attribute) override {
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
    // Need to figure out if there is a better name to get access to the
    // function's name
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

  std::string findTargetLabel(MCInst &inst_ref) {
    auto num_operands = inst_ref.getNumOperands();
    for (unsigned i = 0; i < num_operands; ++i) {
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

  // Only call after MF with Basicblocks is constructed to generate the
  // successors for each basic block
  void generateSuccessors() {
    cout << "generating basic block successors" << '\n';
    for (unsigned i = 0; i < MF.BBs.size() - 1; ++i) {
      auto &cur_bb = MF.BBs[i];
      auto next_bb_ptr = &MF.BBs[i + 1];
      if (cur_bb.Instrs.empty()) {
        cout
            << "generateSuccessors, encountered basic block with 0 instructions"
            << '\n';
        continue;
      }
      auto &last_mc_instr = cur_bb.Instrs.back().getMCInst();
      if (Ana_ptr->isConditionalBranch(last_mc_instr)) {
        std::string target = findTargetLabel(last_mc_instr);
        auto target_bb = MF.findBlockByName(target);
        cur_bb.addSucc(target_bb);
        cur_bb.addSucc(next_bb_ptr);
      } else if (Ana_ptr->isUnconditionalBranch(last_mc_instr)) {
        std::string target = findTargetLabel(last_mc_instr);
        auto target_bb = MF.findBlockByName(target);
        cur_bb.addSucc(target_bb);
      } else if (Ana_ptr->isReturn(last_mc_instr)) {
        continue;
      } else { // add edge to next block
        cur_bb.addSucc(next_bb_ptr);
      }
    }
  }

  // Remove empty basic blocks from the machine function
  void removeEmptyBlocks() {
    cout << "removing empty basic blocks" << '\n';
    std::erase_if(MF.BBs, [](MCBasicBlock b) { return b.size() == 0; });
  }

  // Only call after generateSucessors() has been called
  // generate predecessors for each basic block in a MCFunction
  void generatePredecessors() {
    cout << "generating basic block predecessors" << '\n';
    for (auto &block : MF.BBs) {
      for (auto it = block.succBegin(); it != block.succEnd(); ++it) {
        auto successor = *it;
        successor->addPred(&block);
      }
    }
  }

  void printBlocks() {
    cout << "#of Blocks = " << MF.BBs.size() << '\n';
    cout << "-------------\n";
    int i = 0;
    for (auto &block : MF.BBs) {
      errs() << "block " << i << ", name= " << block.getName() << '\n';
      for (auto &inst : block.Instrs) {
        inst.instr.dump_pretty(llvm::errs(), IP_ptr, " ", MRI_ptr);
        errs() << '\n';
      }
      i++;
    }
  }

  void printCFG() {
    cout << "printing arm function CFG" << '\n';
    cout << "successors" << '\n';
    for (auto &block : MF.BBs) {
      cout << block.getName() << ": [";
      for (auto it = block.succBegin(); it != block.succEnd(); ++it) {
        auto successor = *it;
        cout << successor->getName() << ", ";
      }
      cout << "]\n";
    }

    cout << "predecessors" << '\n';
    for (auto &block : MF.BBs) {
      cout << block.getName() << ": [";
      for (auto it = block.predBegin(); it != block.predEnd(); ++it) {
        auto predecessor = *it;
        cout << predecessor->getName() << ", ";
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

static unsigned id_{0};
unsigned getId() {
  return ++id_;
}

// Used for performing LVN
struct SymValue {
  unsigned opcode{0};
  // std::vector<MCOperand> operands;
  std::vector<unsigned> operands;
};

// FIXME we should probably remove these and go with the default <=>
// implementation as this implementation is neither nice nor performant
struct SymValueHash {
  size_t operator()(const SymValue &op) const {
    // auto op_hasher = MCOperandHash();
    // Is this the right way to go about this
    unsigned combined_val = 41; // start with some prime number
    // Is this too expensive?
    for (auto &e : op.operands) {
      combined_val += e;
    }
    return std::hash<unsigned long>()(op.opcode + combined_val);
  }
};

struct SymValueEqual {
  enum Kind { reg = (1 << 2) - 1, immedidate = (1 << 3) - 1 };
  bool operator()(const SymValue &lhs, const SymValue &rhs) const {
    if (lhs.opcode != rhs.opcode ||
        lhs.operands.size() != rhs.operands.size()) {
      return false;
    }
    for (unsigned i = 0; i < rhs.operands.size(); ++i) {
      if (lhs.operands[i] != rhs.operands[i])
        return false;
    }
    return true;
  }
};

// Return variables that are read before being written in the basic block
auto FindReadBeforeWritten(std::vector<MCInst> &instrs,
                           llvm::MCInstrAnalysis *Ana_ptr) {
  std::unordered_set<MCOperand, MCOperandHash, MCOperandEqual> reads;
  std::unordered_set<MCOperand, MCOperandHash, MCOperandEqual> writes;
  // TODO for writes, should only apply to instructions that update a
  // destination register
  for (auto &I : instrs) {
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

// Return variable that are read before being written in the basicblock
auto FindReadBeforeWritten(MCBasicBlock &block,
                           llvm::MCInstrAnalysis *Ana_ptr) {
  auto mcInstrs = block.getInstrs();
  std::unordered_set<MCOperand, MCOperandHash, MCOperandEqual> reads;
  std::unordered_set<MCOperand, MCOperandHash, MCOperandEqual> writes;
  // TODO for writes, should only apply to instructions that update a
  // destination register
  for (auto &WI : mcInstrs) {
    auto &I = WI.getMCInst();
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

std::vector<bool> LastWrites(std::vector<MCInst> &instrs) {
  // TODO need to check for size of instrs in backend-tv and return error
  // otherwise these helper functions will fail in an ugly manner
  auto last_write = std::vector<bool>(instrs.size(), false);
  std::unordered_set<MCOperand, MCOperandHash, MCOperandEqual> willOverwrite;
  unsigned index = instrs.size() - 1;
  // TODO should only apply to instructions that update a destination register
  for (auto &r_I : ranges::views::reverse(instrs)) {
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

std::vector<bool> LastWrites(MCBasicBlock &block) {
  // TODO need to check for size of instrs in backend-tv and return error
  // otherwise these helper functions will fail in an ugly manner
  auto mcInstrs = block.getInstrs();
  auto last_write = std::vector<bool>(mcInstrs.size(), false);
  std::unordered_set<MCOperand, MCOperandHash, MCOperandEqual> willOverwrite;
  unsigned index = mcInstrs.size() - 1;
  // TODO should only apply to instructions that update a destination register
  for (auto &r_WI : ranges::views::reverse(mcInstrs)) {
    auto &r_I = r_WI.getMCInst();
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

// Perform verification on two alive functions
Results backend_verify(std::optional<IR::Function> &fn1,
                       std::optional<IR::Function> &fn2,
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
#include "llvm_util/cmd_args_def.h"

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
  for (size_t i = 0; i < Asm.size(); ++i)
    cout << Asm[i];
  cout << "-------------\n";
  cout << "\n\n";
  llvm::Triple TheTriple(TripleName);

  auto MCOptions = llvm::mc::InitMCTargetOptionsFromFlags();
  std::unique_ptr<llvm::MCRegisterInfo> MRI(
      Target->createMCRegInfo(TripleName));
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

  std::unique_ptr<llvm::MCInstPrinter> IPtemp(
      Target->createMCInstPrinter(TheTriple, 0, *MAI, *MCII, *MRI));

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

  // FIXME remove printing of the mcInsts
  // For now, print the parsed instructions for debug puropses
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

  Str.printBlocks();
  Str.removeEmptyBlocks(); // remove empty basic blocks, including .Lfunc_end
  Str.printBlocks();
  Str.generateSuccessors();
  Str.generatePredecessors();
  Str.printCFG();

  // In this part, we want to use lvn on each basic block and use
  // the SSA construction algorithm described in
  // https://link.springer.com/chapter/10.1007/978-3-642-37051-9_6
  // to generate SSA form that can be converted into alive IR
  // FIXME For now, lvn is implemented, but the SSA construction algorithm is
  // not.
  // Hence, for now, we exit if the function has more than 1 block
  if (Str.MF.BBs.size() > 1) {
    cout << "ERROR: we don't generate SSA for this type of arm function yet"
         << '\n';
    return false;
  }

  std::unordered_map<MCOperand, unsigned, MCOperandHash, MCOperandEqual>
      svar2num;
  std::unordered_map<unsigned, MCOperand> num2cvar;
  std::unordered_map<SymValue, unsigned, SymValueHash, SymValueEqual> value2num;

  cout << "finding readers and updating maps\n";
  auto &MF = Str.MF;
  auto &first_BB = MF.BBs[0];
  // FIXME for now we process first basic block only
  auto first_reads = FindReadBeforeWritten(first_BB, Ana.get());
  for (auto &read_op : first_reads) {
    auto new_num = getId();
    svar2num.emplace(read_op, new_num);
    num2cvar.emplace(new_num, read_op);
  }
  cout << "--------svar2num-----------\n";
  for (auto &[key, val] : svar2num) {
    key.dump();
    errs() << ": " << val << '\n';
  }
  cout << "--------num2cvar----------\n";
  for (auto &[key, val] : num2cvar) {
    errs() << key << ": ";
    val.dump();
    errs() << '\n';
  }
  cout << "----------------------\n";
  auto last_writes = LastWrites(first_BB);
  for (unsigned i = 0; i < first_BB.Instrs.size(); ++i) {
    auto &cur_w_instr = first_BB.getInstrs()[i];
    auto &cur_instr = cur_w_instr.getMCInst();
    auto sym_val = SymValue();
    // TODO move this to SymValue CTOR and also distinguish operand selection
    // based on opcode Right now we assume operand 0 is destination and
    // operands 1..n are used
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
    svar2num.insert_or_assign(dst, new_num);

    MCOperand new_dst;
    // if (last_writes[i]) { //no need to add version numbering to dst
    //   new_dst = dst;
    // }
    // else {
    new_dst = dst;
    assert(new_dst.isReg());
    // FIXME I'm offsetting the register number by 1000
    // to use new values. this has to be fixed by updating MCInstWrapper
    new_dst.setReg(1000 + new_num);
    // }

    num2cvar.emplace(new_num, new_dst);
    cur_instr.getOperand(0).setReg(new_dst.getReg());

    if (!sym_val.operands.empty()) {
      value2num.emplace(sym_val, new_num);
    }
    // Update MCInstrs operands with
    for (unsigned j = 1; j < cur_instr.getNumOperands(); ++j) {
      auto &inst_operand = cur_instr.getOperand(j);
      if (inst_operand.isReg()) {
        inst_operand.setReg(num2cvar[sym_val.operands[j - 1]].getReg());
      }
    }
  }

  cout << "\n\nAfter performing LVN:\n";

  first_BB.print();
  // Adjust the ret instruction's operand
  // FIXME for now, we're doing something pretty naive/wrong except in the
  // simplest cases I'm assuming that the destination for the second to last
  // instruction is the return value of the function.
  auto &BB_mcinstrs = first_BB.getInstrs();
  auto BB_size = first_BB.size();
  assert(BB_size > 1);
  MCInst &last_instr = BB_mcinstrs[BB_size - 1].getMCInst();
  MCInst &sec_last_instr = BB_mcinstrs[BB_size - 2].getMCInst();
  auto &dest_operand = sec_last_instr.getOperand(0);
  auto &ret_operand = last_instr.getOperand(0);
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
    AF = llvm2alive(F, TLI.getTLI(F));
    break;
  }

  AF->print(cout << "\n----------alive-ir-src.ll-file----------\n");

  auto TF = arm2alive(MF, DL, AF, IPtemp.get(), MRI.get());
  if (TF)
    TF->print(cout << "\n----------alive-lift-arm-target----------\n");

  auto r = backend_verify(AF, TF, TLI, true);

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
#include "llvm_util/cmd_args_def.h"

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

// arm util functions

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
    backendTV(); // this is the function we use to perform arm translation
                 // validation
  } else {
    bitcodeTV();
  }

  *out << "Summary:\n"
          "  "
       << num_correct
       << " correct transformations\n"
          "  "
       << num_unsound
       << " incorrect transformations\n"
          "  "
       << num_failed
       << " failed-to-prove transformations\n"
          "  "
       << num_errors << " Alive2 errors\n";

  if (opt_smt_stats)
    smt::solver_print_stats(*out);

  smt_init.reset();

  if (opt_alias_stats)
    IR::Memory::printAliasStats(*out);

  return num_errors > 0;
}
