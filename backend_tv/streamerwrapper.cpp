#include "backend_tv/streamerwrapper.h"

using namespace std;
using namespace llvm;
using namespace lifter;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void MCStreamerWrapper::emitInstruction(const MCInst &Inst,
                               const MCSubtargetInfo & /* unused */) {

    assert(prev_line != ASMLine::none);

    if (prev_line == ASMLine::terminator)
      curBB = MF.addBlock(MF.getLabel());

    Instruction *LLVMInst = lineMap[curDebugLine];
    SMLoc Loc = SMLoc::getFromPointer((char *)LLVMInst);
    MCInst i(Inst);
    i.setLoc(Loc);
    curBB->addInst(i);

    prev_line =
        IA->isTerminator(Inst) ? ASMLine::terminator : ASMLine::non_term_instr;
    auto num_operands = i.getNumOperands();
    for (unsigned idx = 0; idx < num_operands; ++idx) {
      auto op = i.getOperand(idx);
      if (op.isExpr()) {
        auto expr = op.getExpr();
        if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
          const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
          const MCSymbol &Sym = SRE.getSymbol();
          *out << "target label : " << (string)Sym.getName()
               << ", offset=" << to_string(Sym.getOffset())
               << '\n'; // FIXME remove when done
        }
      }
    }

    *out << cnt++ << "  : ";
    std::string sss;
    llvm::raw_string_ostream ss(sss);
    // Inst.dump_pretty(ss, IP, " ", MRI);
    *out << sss;
    if (IA->isBranch(Inst))
      *out << ": branch ";
    if (IA->isConditionalBranch(Inst))
      *out << ": conditional branch ";
    if (IA->isUnconditionalBranch(Inst))
      *out << ": unconditional branch ";
    if (IA->isTerminator(Inst))
      *out << ": terminator ";
    *out << "\n";
  }

void MCStreamerWrapper::emitValueImpl(const MCExpr *Value, unsigned Size,
                             SMLoc Loc) {
    if (curSec.starts_with(".debug")) {
      *out << "skipping emitValue in debug section\n";
      return;
    }
    if (auto SR = dyn_cast<MCSymbolRefExpr>(Value)) {
      const MCSymbol &Sym = SR->getSymbol();
      std::string name{Sym.getName()};
      *out << "[emitValue MCSymbolRefExpr= " << name << "]\n";
      OffsetSym s{name, 0};
      curROData.push_back(RODataItem{s});
    } else if (auto CE = dyn_cast<MCConstantExpr>(Value)) {
      *out << "[emitValue MCConstantExpr]\n";
      CE->dump();
      assert(false && "handle this");
    } else if (auto UE = dyn_cast<MCUnaryExpr>(Value)) {
      *out << "[emitValue MCUnaryExpr]\n";
      UE->dump();
      assert(false && "handle this");
    } else if (auto BE = dyn_cast<MCBinaryExpr>(Value)) {
      *out << "[emitValue MCBinaryExpr]\n";
      auto *LHS = dyn_cast<MCSymbolRefExpr>(BE->getLHS());
      assert(LHS);
      auto *RHS = dyn_cast<MCConstantExpr>(BE->getRHS());
      assert(RHS);
      OffsetSym s{(std::string)LHS->getSymbol().getName(), RHS->getValue()};
      curROData.push_back(RODataItem{s});
    } else if (auto TE = dyn_cast<MCTargetExpr>(Value)) {
      *out << "[emitValue MCTargetExpr]\n";
      TE->dump();
      assert(false && "handle this");
    } else {
      assert(false && "unexpected MCExpr type");
    }
  }

void MCStreamerWrapper::emitLabel(MCSymbol *Symbol, SMLoc Loc) {
    auto sp = getCurrentSection();
    std::string secName = sp.first->getName().str();
    std::string lab = Symbol->getName().str();
    *out << "[emitLabel '" << lab << "' in section '" << secName << "']\n";

    if (secName.starts_with(".debug")) {
      *out << "  skipping debug stuff\n";
      curSec = (std::string)sp.first->getName();
      return;
    }

    addConstant();

    curSym = Symbol->getName().str();
    curSec = (std::string)sp.first->getName();

    if (lab == ".Lfunc_end0")
      FunctionEnded = true;
    if (!FunctionEnded) {
      curBB = MF.addBlock(lab);
      MCInst nop;
      // subsequent logic can be a bit simpler if we assume each BB
      // contains at least one instruction. might need to revisit this
      // later on.
      nop.setOpcode(AArch64::SEH_Nop);
      curBB->addInstBegin(std::move(nop));
      prev_line = ASMLine::label;
    }
  }

void MCStreamerWrapper::generateSuccessors() {
    *out << "generating basic block successors" << '\n';
    for (unsigned i = 0; i < MF.BBs.size(); ++i) {
      auto &cur_bb = MF.BBs[i];
      MCBasicBlock *next_bb_ptr = nullptr;
      if (i < MF.BBs.size() - 1)
        next_bb_ptr = &MF.BBs[i + 1];

      if (cur_bb.empty()) {
        *out
            << "generateSuccessors, encountered basic block with 0 instructions"
            << '\n';
        continue;
      }
      auto &last_mc_instr = cur_bb.getInstrs().back();
      // handle the special case of adding where we have added a new entry block
      // with no predecessors. This is hacky because I don't know the API to
      // create an MCExpr and have to create a branch with an immediate operand
      // instead
      if (i == 0 && (IA->isUnconditionalBranch(last_mc_instr)) &&
          last_mc_instr.getOperand(0).isImm()) {
        cur_bb.addSucc(next_bb_ptr);
        continue;
      }
      if (IA->isConditionalBranch(last_mc_instr)) {
        std::string target = findTargetLabel(last_mc_instr);
        auto target_bb = MF.findBlockByName(target);
        assert(target_bb);
        cur_bb.addSucc(target_bb);
        if (next_bb_ptr)
          cur_bb.addSucc(next_bb_ptr);
      } else if (IA->isUnconditionalBranch(last_mc_instr)) {
        std::string target = findTargetLabel(last_mc_instr);
        auto target_bb = MF.findBlockByName(target);
        if (target_bb) {
          cur_bb.addSucc(target_bb);
        } else {
          *out << "looks like a tail call to " << target << "\n";
        }
      } else if (IA->isReturn(last_mc_instr)) {
        continue;
      } else if (next_bb_ptr) {
        // add edge to next block
        cur_bb.addSucc(next_bb_ptr);
      }
    }
  }
