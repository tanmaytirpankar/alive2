#include "backend_tv/arm2llvm.h"

using namespace std;
using namespace lifter;
using namespace llvm;

void arm2llvm::lift_branch() {
  BasicBlock *dst{nullptr};
  // JDR: I don't understand this
  if (CurInst->getOperand(0).isImm()) {
    // handles the case when we add an entry block with no predecessors
    auto &dst_name = MF.BBs[getImm(0)].getName();
    dst = getBBByName(dst_name);
  } else {
    dst = getBB(CurInst->getOperand(0));
  }
  if (dst) {
    createBranch(dst);
  } else {
    // ok, if we don't have a destination block then we left this
    // dangling on purpose, with the assumption that it's a tail
    // call
    doDirectCall();
    doReturn();
  }
}

void arm2llvm::lift_bcc() {
  auto cond_val_imm = getImm(0);
  auto cond_val = conditionHolds(cond_val_imm);

  auto &jmp_tgt_op = CurInst->getOperand(1);
  assert(jmp_tgt_op.isExpr() && "expected expression");
  assert((jmp_tgt_op.getExpr()->getKind() == MCExpr::ExprKind::SymbolRef) &&
         "expected symbol ref as bcc operand");
  const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*jmp_tgt_op.getExpr());
  const MCSymbol &Sym = SRE.getSymbol();

  auto *dst_true = getBBByName(Sym.getName());

  assert(MCBB->getSuccs().size() == 1 || MCBB->getSuccs().size() == 2);
  const string *dst_false_name = nullptr;
  for (auto &succ : MCBB->getSuccs()) {
    if (succ->getName() != Sym.getName()) {
      dst_false_name = &succ->getName();
      return;
    }
  }
  auto *dst_false =
      getBBByName(dst_false_name ? *dst_false_name : Sym.getName());

  createBranch(cond_val, dst_true, dst_false);
}
