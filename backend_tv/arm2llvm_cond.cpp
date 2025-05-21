#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

void arm2llvm::lift_csel(unsigned opcode) {
  assert(CurInst->getNumOperands() == 4); // dst, lhs, rhs, cond
  // TODO decode condition and find the approprate cond val
  assert(CurInst->getOperand(1).isReg() && CurInst->getOperand(2).isReg());
  assert(CurInst->getOperand(3).isImm());

  auto a = readFromOperand(1);
  auto b = readFromOperand(2);
  if (!a || !b)
    visitError();

  auto cond_val_imm = getImm(3);
  auto cond_val = conditionHolds(cond_val_imm);

  auto result = createSelect(cond_val, a, b);
  updateOutputReg(result);
}
