#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

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
