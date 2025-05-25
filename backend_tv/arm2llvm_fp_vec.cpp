#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_fcvt_2(unsigned opcode) {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    assert(op0.isReg() && op1.isReg());

    auto isSigned =
        opcode == AArch64::FCVTZSv1i32 || opcode == AArch64::FCVTZSv1i64;

    auto op0Size = getRegSize(op0.getReg());
    auto op1Size = getRegSize(op1.getReg());

    auto fp_val = readFromFPOperand(1, op1Size);
    auto converted = isSigned ? createFPToSI_sat(fp_val, getIntTy(op0Size))
                              : createFPToUI_sat(fp_val, getIntTy(op0Size));
    updateOutputReg(converted);
}

void arm2llvm::lift_vec_fneg(unsigned opcode) {
  unsigned eltSize, numElts;
  switch (opcode) {
  case AArch64::FNEGv2f32: {
    eltSize = 32;
    numElts = 2;
    break;
  }
  case AArch64::FNEGv4f16: {
    eltSize = 16;
    numElts = 4;
    break;
  }
  case AArch64::FNEGv4f32: {
    eltSize = 32;
    numElts = 4;
    break;
  }
  case AArch64::FNEGv2f64: {
    eltSize = 64;
    numElts = 2;
    break;
  }
  case AArch64::FNEGv8f16: {
    eltSize = 16;
    numElts = 8;
    break;
  }
  default: {
    assert(false);
    break;
  }
  }
  auto v = readFromVecOperand(1, eltSize, numElts, false, true);
  auto res = createFNeg(v);
  updateOutputReg(res);
}
