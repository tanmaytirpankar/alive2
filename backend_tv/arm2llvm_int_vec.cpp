#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_ext_2() {
  auto i128 = getIntTy(128);
  auto a = readFromOperand(1);
  auto b = readFromOperand(2);
  auto imm = getImm(3);
  auto both = concat(b, a);
  auto shifted = createRawLShr(both, getUnsignedIntConst(8 * imm, 256));
  updateOutputReg(createTrunc(shifted, i128));
}

void arm2llvm::lift_ext_1() {
  auto i64 = getIntTy(64);
  auto a = readFromOperand(1);
  auto b = readFromOperand(2);
  auto imm = getImm(3);
  auto both = concat(b, a);
  auto shifted = createRawLShr(both, getUnsignedIntConst(8 * imm, 128));
  updateOutputReg(createTrunc(shifted, i64));
}

void arm2llvm::lift_movi_7() {
  auto imm1 = getImm(1);
  auto imm2 = getImm(2);
  auto val = getUnsignedIntConst(imm1 << imm2, 32);
  updateOutputReg(dupElts(val, 4, 32));
}

void arm2llvm::lift_movi_6() {
  auto imm1 = getImm(1);
  auto imm2 = getImm(2);
  auto val = getUnsignedIntConst(imm1 << imm2, 32);
  updateOutputReg(dupElts(val, 2, 32));
}

void arm2llvm::lift_movi_5() {
  auto imm1 = getImm(1);
  auto imm2 = getImm(2);
  auto val = getUnsignedIntConst(imm1 << imm2, 16);
  updateOutputReg(dupElts(val, 8, 16));
}

void arm2llvm::lift_movi_4() {
  auto imm1 = getImm(1);
  auto imm2 = getImm(2);
  auto val = getUnsignedIntConst(imm1 << imm2, 16);
  updateOutputReg(dupElts(val, 4, 16));
}

void arm2llvm::lift_movi_3() {
  auto v = getUnsignedIntConst(getImm(1), 8);
  updateOutputReg(dupElts(v, 16, 8));
}

void arm2llvm::lift_movi_2() {
  auto v = getUnsignedIntConst(getImm(1), 8);
  updateOutputReg(dupElts(v, 8, 8));
}

void arm2llvm::lift_movi_1() {
  auto imm = getUnsignedIntConst(replicate8to64(getImm(1)), 64);
  updateOutputReg(dupElts(imm, 2, 64));
}

void arm2llvm::lift_movi_msl(unsigned opcode) {
  auto imm1 = getUnsignedIntConst(getImm(1), 32);
  auto imm2 = getImm(2) & ~0x100;
  auto v = createMSL(imm1, imm2);
  int numElts = (opcode == AArch64::MOVIv2s_msl) ? 2 : 4;
  updateOutputReg(dupElts(v, numElts, 32));
}

void arm2llvm::lift_mvni_msl(unsigned opcode) {
  auto imm1 = getUnsignedIntConst(getImm(1), 32);
  auto imm2 = getImm(2) & ~0x100;
  auto v = createNot(createMSL(imm1, imm2));
  int numElts = (opcode == AArch64::MVNIv2s_msl) ? 2 : 4;
  updateOutputReg(dupElts(v, numElts, 32));
}

void arm2llvm::lift_mvni(unsigned opcode) {
  int numElts, eltSize;
  switch (opcode) {
  case AArch64::MVNIv8i16:
    numElts = 8;
    eltSize = 16;
    break;
  case AArch64::MVNIv4i32:
    numElts = 4;
    eltSize = 32;
    break;
  case AArch64::MVNIv4i16:
    numElts = 4;
    eltSize = 16;
    break;
  case AArch64::MVNIv2i32:
    numElts = 2;
    eltSize = 32;
    break;
  default:
    assert(false);
  }
  auto imm1 = getUnsignedIntConst(getImm(1), eltSize);
  auto imm2 = getUnsignedIntConst(getImm(2), eltSize);
  auto v = createNot(createRawShl(imm1, imm2));
  updateOutputReg(dupElts(v, numElts, eltSize));
}

void arm2llvm::lift_umov_vi(unsigned opcode) {
  unsigned sz;
  if (opcode == AArch64::UMOVvi8 || opcode == AArch64::UMOVvi8_idx0) {
    sz = 8;
  } else if (opcode == AArch64::UMOVvi16 || opcode == AArch64::UMOVvi16_idx0) {
    sz = 16;
  } else if (opcode == AArch64::UMOVvi32 || opcode == AArch64::UMOVvi32_idx0) {
    sz = 32;
  } else if (opcode == AArch64::UMOVvi64) {
    sz = 64;
  } else {
    assert(false);
  }
  unsigned idx;
  if (opcode == AArch64::UMOVvi8_idx0 || opcode == AArch64::UMOVvi16_idx0 ||
      opcode == AArch64::UMOVvi32_idx0) {
    idx = 0;
  } else {
    idx = getImm(2);
  }
  auto vTy = getVecTy(sz, 128 / sz);
  auto reg = createBitCast(readFromOperand(1), vTy);
  auto val = createExtractElement(reg, idx);
  updateOutputReg(val);
}

void arm2llvm::lift_smov_vi(unsigned opcode) {
  auto i8 = getIntTy(8);
  auto i16 = getIntTy(16);
  auto i32 = getIntTy(32);
  auto val = readFromOperand(1);
  auto index = getImm(2);
  int64_t eltSizeLog2;
  Type *truncSize;

  switch (opcode) {
  case AArch64::SMOVvi8to32_idx0:
  case AArch64::SMOVvi8to32:
    eltSizeLog2 = 3;
    truncSize = i8;
    break;
  case AArch64::SMOVvi16to32_idx0:
  case AArch64::SMOVvi16to32:
    eltSizeLog2 = 4;
    truncSize = i16;
    break;
  case AArch64::SMOVvi8to64_idx0:
  case AArch64::SMOVvi8to64:
    eltSizeLog2 = 3;
    truncSize = i8;
    break;
  case AArch64::SMOVvi16to64_idx0:
  case AArch64::SMOVvi16to64:
    eltSizeLog2 = 4;
    truncSize = i16;
    break;
  case AArch64::SMOVvi32to64_idx0:
  case AArch64::SMOVvi32to64:
    eltSizeLog2 = 5;
    truncSize = i32;
    break;
  default:
    assert(false && "error");
  }
  auto shiftAmt = getUnsignedIntConst(index << eltSizeLog2, 128);
  auto shifted = createRawLShr(val, shiftAmt);
  auto trunced = createTrunc(shifted, truncSize);
  updateOutputReg(trunced, true);
}

void arm2llvm::lift_ins_lane(unsigned opcode) {
  unsigned w;
  if (opcode == AArch64::INSvi8lane) {
    w = 8;
  } else if (opcode == AArch64::INSvi16lane) {
    w = 16;
  } else if (opcode == AArch64::INSvi32lane) {
    w = 32;
  } else if (opcode == AArch64::INSvi64lane) {
    w = 64;
  } else {
    assert(false);
  }
  auto in = readFromVecOperand(3, w, 128 / w);
  auto out = readFromVecOperand(1, w, 128 / w);
  auto ext = createExtractElement(in, getImm(4));
  auto ins = createInsertElement(out, ext, getImm(2));
  updateOutputReg(ins);
}

void arm2llvm::lift_ins_gpr(unsigned opcode) {
  unsigned w;
  if (opcode == AArch64::INSvi8gpr) {
    w = 8;
  } else if (opcode == AArch64::INSvi16gpr) {
    w = 16;
  } else if (opcode == AArch64::INSvi32gpr) {
    w = 32;
  } else if (opcode == AArch64::INSvi64gpr) {
    w = 64;
  } else {
    assert(false);
  }
  auto val = readFromOperand(3);
  // need to clear extraneous bits
  if (w < 32)
    val = createTrunc(val, getIntTy(w));
  auto lane = getImm(2);
  auto ty = getVecTy(w, 128 / w);
  auto vec = readFromRegTyped(CurInst->getOperand(1).getReg(), ty);
  auto inserted = createInsertElement(vec, val, lane);
  updateOutputReg(inserted);
}
