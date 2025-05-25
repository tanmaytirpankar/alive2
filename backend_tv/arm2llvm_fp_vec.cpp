#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_vec_fpbinop(unsigned opcode) {
  int eltSize = -1, numElts = -1;
  switch (opcode) {
  case AArch64::FMULv2f32:
  case AArch64::FADDv2f32:
  case AArch64::FSUBv2f32:
    eltSize = 32;
    numElts = 2;
    break;
  case AArch64::FMULv4f32:
  case AArch64::FADDv4f32:
  case AArch64::FSUBv4f32:
    eltSize = 32;
    numElts = 4;
    break;
  case AArch64::FADDv2f64:
  case AArch64::FSUBv2f64:
  case AArch64::FMULv2f64:
    eltSize = 64;
    numElts = 2;
    break;
  default:
    assert(false);
  }
  auto a = readFromVecOperand(1, eltSize, numElts, /*isUpperHalf=*/false,
                              /*isFP=*/true);
  auto b = readFromVecOperand(2, eltSize, numElts, /*isUpperHalf=*/false,
                              /*isFP=*/true);
  Value *res{nullptr};
  switch (opcode) {
  case AArch64::FADDv2f32:
  case AArch64::FADDv4f32:
  case AArch64::FADDv2f64:
    res = createFAdd(a, b);
    break;
  case AArch64::FSUBv2f32:
  case AArch64::FSUBv4f32:
  case AArch64::FSUBv2f64:
    res = createFSub(a, b);
    break;
  case AArch64::FMULv2f32:
  case AArch64::FMULv4f32:
  case AArch64::FMULv2f64:
    res = createFMul(a, b);
    break;
  default:
    assert(false);
  };
  updateOutputReg(res);
}

void arm2llvm::lift_fmov_3(unsigned opcode) {
  bool bitWidth128 = false;
  unsigned numElts, eltSize, op;
  switch (opcode) {
  case AArch64::FMOVv2f32_ns: {
    numElts = 2;
    eltSize = 32;
    bitWidth128 = false;
    op = 0;
    break;
  }
  case AArch64::FMOVv4f32_ns: {
    numElts = 4;
    eltSize = 32;
    bitWidth128 = true;
    op = 0;
    break;
  }
  case AArch64::FMOVv2f64_ns: {
    numElts = 2;
    eltSize = 64;
    bitWidth128 = true;
    op = 1;
    break;
  }
  default:
    assert(false);
  }
  unsigned cmode = 15;
  auto imm = getImm(1);
  assert(imm <= 256);
  auto expandedImm = AdvSIMDExpandImm(op, cmode, imm);
  Constant *expandedImmVal = getUnsignedIntConst(expandedImm, 64);
  if (bitWidth128) {
    // Create a 128-bit vector with the expanded immediate
    expandedImmVal = getElemSplat(2, 64, expandedImm);
  }

  auto result = createBitCast(expandedImmVal, getVecTy(eltSize, numElts, true));
  updateOutputReg(result);
}

void arm2llvm::lift_cvtf_2(unsigned opcode) {
  auto &op0 = CurInst->getOperand(0);
  auto &op1 = CurInst->getOperand(1);
  assert(op0.isReg() && op1.isReg());

  auto isSigned =
      opcode == AArch64::SCVTFv1i32 || opcode == AArch64::SCVTFv1i64;

  auto fTy = getFPType(getRegSize(op0.getReg()));
  auto val = readFromOperand(1, getRegSize(op1.getReg()));
  auto converted = isSigned ? createSIToFP(val, fTy) : createUIToFP(val, fTy);

  updateOutputReg(converted);
}

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
