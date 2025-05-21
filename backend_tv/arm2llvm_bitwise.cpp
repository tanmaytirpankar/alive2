#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

void arm2llvm::lift_and(unsigned opcode) {
  auto size = getInstSize(opcode);
  Value *rhs = nullptr;
  if (CurInst->getOperand(2).isImm()) {
    auto [wmask, _] = decodeBitMasks(getImm(2), size);
    rhs = getUnsignedIntConst(wmask, size);
  } else {
    rhs = readFromOperand(2);
  }

  // We are in a ANDrs case. We need to handle a shift
  if (CurInst->getNumOperands() == 4) {
    // the 4th operand (if it exists) must be an immediate
    assert(CurInst->getOperand(3).isImm());
    rhs = regShift(rhs, getImm(3));
  }

  auto and_op = createAnd(readFromOperand(1), rhs);

  if (has_s(opcode)) {
    setNUsingResult(and_op);
    setZUsingResult(and_op);
    setC(getBoolConst(false));
    setV(getBoolConst(false));
  }

  updateOutputReg(and_op);
}

void arm2llvm::lift_asrv(unsigned opcode) {
  auto size = getInstSize(opcode);
  auto a = readFromOperand(1);
  auto b = readFromOperand(2);

  auto shift_amt =
      createBinop(b, getUnsignedIntConst(size, size), Instruction::URem);
  auto res = createMaskedAShr(a, shift_amt);
  updateOutputReg(res);
}
