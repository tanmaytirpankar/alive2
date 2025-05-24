#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_fcvt_1(unsigned opcode) {
  auto &op0 = CurInst->getOperand(0);
  auto &op1 = CurInst->getOperand(1);
  assert(op0.isReg() && op1.isReg());

  auto isSigned =
      opcode == AArch64::FCVTZSUWHr || opcode == AArch64::FCVTZSUWSr ||
      opcode == AArch64::FCVTZSUWDr || opcode == AArch64::FCVTZSUXHr ||
      opcode == AArch64::FCVTZSUXSr || opcode == AArch64::FCVTZSUXDr;

  auto op0Size = getRegSize(op0.getReg());
  auto op1Size = getRegSize(op1.getReg());

  auto fp_val = readFromFPOperand(1, op1Size);
  auto converted = isSigned ? createFPToSI_sat(fp_val, getIntTy(op0Size))
                            : createFPToUI_sat(fp_val, getIntTy(op0Size));
  updateOutputReg(converted);
}

void arm2llvm::lift_fneg() {
  auto operandSize = getRegSize(CurInst->getOperand(1).getReg());
  auto fVal = readFromFPOperand(1, operandSize);
  auto res = createFNeg(fVal);
  updateOutputReg(res);
}
