#include "backend_tv/arm2llvm.h"

#include <vector>

using namespace lifter;
using namespace llvm;
using namespace std;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_tbl(unsigned opcode) {
  int lanes;
  switch (opcode) {
  case AArch64::TBLv8i8One:
  case AArch64::TBLv8i8Two:
  case AArch64::TBLv8i8Three:
  case AArch64::TBLv8i8Four:
    lanes = 8;
    break;
  case AArch64::TBLv16i8One:
  case AArch64::TBLv16i8Two:
  case AArch64::TBLv16i8Three:
  case AArch64::TBLv16i8Four:
    lanes = 16;
    break;
  default:
    assert(false);
  }
  int nregs;
  switch (opcode) {
  case AArch64::TBLv8i8One:
  case AArch64::TBLv16i8One:
    nregs = 1;
    break;
  case AArch64::TBLv8i8Two:
  case AArch64::TBLv16i8Two:
    nregs = 2;
    break;
  case AArch64::TBLv8i8Three:
  case AArch64::TBLv16i8Three:
    nregs = 3;
    break;
  case AArch64::TBLv8i8Four:
  case AArch64::TBLv16i8Four:
    nregs = 4;
    break;
  default:
    assert(false);
  }
  auto vTy = getVecTy(8, lanes);
  auto fullTy = getVecTy(8, 16);
  auto baseReg = decodeRegSet(CurInst->getOperand(1).getReg());
  vector<Value *> regs;
  for (int i = 0; i < nregs; ++i) {
    regs.push_back(createBitCast(readFromRegOld(baseReg), fullTy));
    baseReg++;
    if (baseReg > AArch64::Q31)
      baseReg = AArch64::Q0;
  }
  auto src = createBitCast(readFromOperand(2), vTy);
  Value *res = getUndefVec(lanes, 8);
  for (int i = 0; i < lanes; ++i) {
    auto idx = createExtractElement(src, i);
    auto entry = tblHelper(regs, idx);
    res = createInsertElement(res, entry, i);
  }
  updateOutputReg(res);
}

void arm2llvm::lift_cm(unsigned opcode) {
  auto a = readFromOperand(1);
  Value *b;
  switch (opcode) {
  case AArch64::CMGTv4i16rz:
  case AArch64::CMEQv1i64rz:
  case AArch64::CMGTv8i8rz:
  case AArch64::CMLEv4i16rz:
  case AArch64::CMGEv4i16rz:
  case AArch64::CMLEv8i8rz:
  case AArch64::CMLTv1i64rz:
  case AArch64::CMGTv2i32rz:
  case AArch64::CMGEv8i8rz:
  case AArch64::CMLEv2i32rz:
  case AArch64::CMGEv2i32rz:
  case AArch64::CMLEv8i16rz:
  case AArch64::CMLEv16i8rz:
  case AArch64::CMGTv16i8rz:
  case AArch64::CMGTv8i16rz:
  case AArch64::CMGTv2i64rz:
  case AArch64::CMGEv8i16rz:
  case AArch64::CMLEv4i32rz:
  case AArch64::CMGEv2i64rz:
  case AArch64::CMLEv2i64rz:
  case AArch64::CMGEv16i8rz:
  case AArch64::CMGEv4i32rz:
  case AArch64::CMEQv16i8rz:
  case AArch64::CMEQv2i32rz:
  case AArch64::CMEQv2i64rz:
  case AArch64::CMEQv4i16rz:
  case AArch64::CMEQv4i32rz:
  case AArch64::CMEQv8i16rz:
  case AArch64::CMEQv8i8rz:
  case AArch64::CMGTv4i32rz:
  case AArch64::CMLTv16i8rz:
  case AArch64::CMLTv2i32rz:
  case AArch64::CMLTv2i64rz:
  case AArch64::CMLTv4i16rz:
  case AArch64::CMLTv4i32rz:
  case AArch64::CMLTv8i16rz:
  case AArch64::CMLTv8i8rz:
    b = getUnsignedIntConst(0, getInstSize(opcode));
    break;
  case AArch64::CMEQv1i64:
  case AArch64::CMEQv16i8:
  case AArch64::CMEQv2i32:
  case AArch64::CMEQv2i64:
  case AArch64::CMEQv4i16:
  case AArch64::CMEQv4i32:
  case AArch64::CMEQv8i16:
  case AArch64::CMEQv8i8:
  case AArch64::CMGEv16i8:
  case AArch64::CMGEv2i32:
  case AArch64::CMGEv1i64:
  case AArch64::CMGEv2i64:
  case AArch64::CMGEv4i16:
  case AArch64::CMGEv4i32:
  case AArch64::CMGEv8i16:
  case AArch64::CMGEv8i8:
  case AArch64::CMGTv16i8:
  case AArch64::CMGTv2i32:
  case AArch64::CMGTv2i64:
  case AArch64::CMGTv4i16:
  case AArch64::CMGTv4i32:
  case AArch64::CMGTv8i16:
  case AArch64::CMGTv8i8:
  case AArch64::CMHIv16i8:
  case AArch64::CMGTv1i64:
  case AArch64::CMHIv1i64:
  case AArch64::CMHIv2i32:
  case AArch64::CMHIv2i64:
  case AArch64::CMHIv4i16:
  case AArch64::CMHIv4i32:
  case AArch64::CMHIv8i16:
  case AArch64::CMHIv8i8:
  case AArch64::CMHSv16i8:
  case AArch64::CMHSv1i64:
  case AArch64::CMHSv2i32:
  case AArch64::CMHSv2i64:
  case AArch64::CMHSv4i16:
  case AArch64::CMHSv4i32:
  case AArch64::CMHSv8i16:
  case AArch64::CMHSv8i8:
  case AArch64::CMTSTv4i16:
  case AArch64::CMTSTv2i32:
  case AArch64::CMTSTv16i8:
  case AArch64::CMTSTv2i64:
  case AArch64::CMTSTv4i32:
  case AArch64::CMTSTv8i16:
  case AArch64::CMTSTv8i8:
    b = readFromOperand(2);
    break;
  default:
    assert(false);
  }

  int numElts, eltSize;
  switch (opcode) {
  case AArch64::CMEQv1i64rz:
  case AArch64::CMLTv1i64rz:
  case AArch64::CMHIv1i64:
  case AArch64::CMGTv1i64:
  case AArch64::CMEQv1i64:
  case AArch64::CMGEv1i64:
  case AArch64::CMHSv1i64:
    numElts = 1;
    eltSize = 64;
    break;
  case AArch64::CMLEv16i8rz:
  case AArch64::CMGTv16i8rz:
  case AArch64::CMGEv16i8rz:
  case AArch64::CMEQv16i8:
  case AArch64::CMEQv16i8rz:
  case AArch64::CMGEv16i8:
  case AArch64::CMGTv16i8:
  case AArch64::CMHIv16i8:
  case AArch64::CMHSv16i8:
  case AArch64::CMLTv16i8rz:
  case AArch64::CMTSTv16i8:
    numElts = 16;
    eltSize = 8;
    break;
  case AArch64::CMTSTv2i32:
  case AArch64::CMEQv2i32:
  case AArch64::CMGTv2i32rz:
  case AArch64::CMLEv2i32rz:
  case AArch64::CMGEv2i32rz:
  case AArch64::CMEQv2i32rz:
  case AArch64::CMGEv2i32:
  case AArch64::CMGTv2i32:
  case AArch64::CMHIv2i32:
  case AArch64::CMHSv2i32:
  case AArch64::CMLTv2i32rz:
    numElts = 2;
    eltSize = 32;
    break;
  case AArch64::CMEQv2i64:
  case AArch64::CMEQv2i64rz:
  case AArch64::CMGEv2i64:
  case AArch64::CMGTv2i64:
  case AArch64::CMHIv2i64:
  case AArch64::CMHSv2i64:
  case AArch64::CMGTv2i64rz:
  case AArch64::CMGEv2i64rz:
  case AArch64::CMLEv2i64rz:
  case AArch64::CMLTv2i64rz:
  case AArch64::CMTSTv2i64:
    numElts = 2;
    eltSize = 64;
    break;
  case AArch64::CMLEv4i16rz:
  case AArch64::CMGEv4i16rz:
  case AArch64::CMGTv4i16rz:
  case AArch64::CMTSTv4i16:
  case AArch64::CMEQv4i16:
  case AArch64::CMEQv4i16rz:
  case AArch64::CMGEv4i16:
  case AArch64::CMGTv4i16:
  case AArch64::CMHIv4i16:
  case AArch64::CMHSv4i16:
  case AArch64::CMLTv4i16rz:
    numElts = 4;
    eltSize = 16;
    break;
  case AArch64::CMLEv4i32rz:
  case AArch64::CMGEv4i32rz:
  case AArch64::CMEQv4i32:
  case AArch64::CMEQv4i32rz:
  case AArch64::CMGEv4i32:
  case AArch64::CMGTv4i32:
  case AArch64::CMGTv4i32rz:
  case AArch64::CMHIv4i32:
  case AArch64::CMHSv4i32:
  case AArch64::CMLTv4i32rz:
  case AArch64::CMTSTv4i32:
    numElts = 4;
    eltSize = 32;
    break;
  case AArch64::CMLEv8i16rz:
  case AArch64::CMGTv8i16rz:
  case AArch64::CMGEv8i16rz:
  case AArch64::CMEQv8i16:
  case AArch64::CMEQv8i16rz:
  case AArch64::CMGEv8i16:
  case AArch64::CMGTv8i16:
  case AArch64::CMHIv8i16:
  case AArch64::CMHSv8i16:
  case AArch64::CMLTv8i16rz:
  case AArch64::CMTSTv8i16:
    numElts = 8;
    eltSize = 16;
    break;
  case AArch64::CMGTv8i8rz:
  case AArch64::CMLEv8i8rz:
  case AArch64::CMGEv8i8rz:
  case AArch64::CMEQv8i8:
  case AArch64::CMEQv8i8rz:
  case AArch64::CMGEv8i8:
  case AArch64::CMGTv8i8:
  case AArch64::CMHIv8i8:
  case AArch64::CMHSv8i8:
  case AArch64::CMLTv8i8rz:
  case AArch64::CMTSTv8i8:
    numElts = 8;
    eltSize = 8;
    break;
  default:
    assert(false);
  }

  auto vTy = getVecTy(eltSize, numElts);
  a = createBitCast(a, vTy);
  b = createBitCast(b, vTy);
  Value *res;

  switch (opcode) {
  case AArch64::CMLEv16i8rz:
  case AArch64::CMLEv2i32rz:
  case AArch64::CMLEv2i64rz:
  case AArch64::CMLEv4i16rz:
  case AArch64::CMLEv4i32rz:
  case AArch64::CMLEv8i16rz:
  case AArch64::CMLEv8i8rz:
    res = createICmp(ICmpInst::Predicate::ICMP_SLE, a, b);
    break;
  case AArch64::CMEQv1i64rz:
  case AArch64::CMEQv1i64:
  case AArch64::CMEQv16i8:
  case AArch64::CMEQv16i8rz:
  case AArch64::CMEQv2i32:
  case AArch64::CMEQv2i32rz:
  case AArch64::CMEQv2i64:
  case AArch64::CMEQv2i64rz:
  case AArch64::CMEQv4i16:
  case AArch64::CMEQv4i16rz:
  case AArch64::CMEQv4i32:
  case AArch64::CMEQv4i32rz:
  case AArch64::CMEQv8i16:
  case AArch64::CMEQv8i16rz:
  case AArch64::CMEQv8i8:
  case AArch64::CMEQv8i8rz:
    res = createICmp(ICmpInst::Predicate::ICMP_EQ, a, b);
    break;
  case AArch64::CMGEv16i8rz:
  case AArch64::CMGEv2i32rz:
  case AArch64::CMGEv2i64rz:
  case AArch64::CMGEv4i16rz:
  case AArch64::CMGEv4i32rz:
  case AArch64::CMGEv8i16rz:
  case AArch64::CMGEv8i8rz:
  case AArch64::CMGEv16i8:
  case AArch64::CMGEv2i32:
  case AArch64::CMGEv1i64:
  case AArch64::CMGEv2i64:
  case AArch64::CMGEv4i16:
  case AArch64::CMGEv4i32:
  case AArch64::CMGEv8i16:
  case AArch64::CMGEv8i8:
    res = createICmp(ICmpInst::Predicate::ICMP_SGE, a, b);
    break;
  case AArch64::CMGTv16i8rz:
  case AArch64::CMGTv2i32rz:
  case AArch64::CMGTv2i64rz:
  case AArch64::CMGTv4i16rz:
  case AArch64::CMGTv8i16rz:
  case AArch64::CMGTv8i8rz:
  case AArch64::CMGTv1i64:
  case AArch64::CMGTv16i8:
  case AArch64::CMGTv2i32:
  case AArch64::CMGTv2i64:
  case AArch64::CMGTv4i16:
  case AArch64::CMGTv4i32:
  case AArch64::CMGTv4i32rz:
  case AArch64::CMGTv8i16:
  case AArch64::CMGTv8i8:
    res = createICmp(ICmpInst::Predicate::ICMP_SGT, a, b);
    break;
  case AArch64::CMHIv16i8:
  case AArch64::CMHIv1i64:
  case AArch64::CMHIv2i32:
  case AArch64::CMHIv2i64:
  case AArch64::CMHIv4i16:
  case AArch64::CMHIv4i32:
  case AArch64::CMHIv8i16:
  case AArch64::CMHIv8i8:
    res = createICmp(ICmpInst::Predicate::ICMP_UGT, a, b);
    break;
  case AArch64::CMHSv16i8:
  case AArch64::CMHSv1i64:
  case AArch64::CMHSv2i32:
  case AArch64::CMHSv2i64:
  case AArch64::CMHSv4i16:
  case AArch64::CMHSv4i32:
  case AArch64::CMHSv8i16:
  case AArch64::CMHSv8i8:
    res = createICmp(ICmpInst::Predicate::ICMP_UGE, a, b);
    break;
  case AArch64::CMLTv1i64rz:
  case AArch64::CMLTv16i8rz:
  case AArch64::CMLTv2i32rz:
  case AArch64::CMLTv2i64rz:
  case AArch64::CMLTv4i16rz:
  case AArch64::CMLTv4i32rz:
  case AArch64::CMLTv8i16rz:
  case AArch64::CMLTv8i8rz:
    res = createICmp(ICmpInst::Predicate::ICMP_SLT, a, b);
    break;
  case AArch64::CMTSTv2i32:
  case AArch64::CMTSTv4i16:
  case AArch64::CMTSTv16i8:
  case AArch64::CMTSTv2i64:
  case AArch64::CMTSTv4i32:
  case AArch64::CMTSTv8i16:
  case AArch64::CMTSTv8i8: {
    auto *tmp = createAnd(a, b);
    auto *zero =
        createBitCast(getUnsignedIntConst(0, getInstSize(opcode)), vTy);
    res = createICmp(ICmpInst::Predicate::ICMP_NE, tmp, zero);
    break;
  }
  default:
    assert(false);
  }

  updateOutputReg(createSExt(res, vTy));
}

void arm2llvm::lift_bif1() {
  auto op1 = readFromOperand(1);
  auto op4 = readFromOperand(2);
  auto op3 = createNot(readFromOperand(3));
  auto res = createXor(op1, createAnd(createXor(op1, op4), op3));
  updateOutputReg(res);
}

void arm2llvm::lift_bif2() {
  auto op4 = readFromOperand(2);
  auto op1 = readFromOperand(1);
  auto op3 = readFromOperand(3);
  auto res = createXor(op1, createAnd(createXor(op1, op4), op3));
  updateOutputReg(res);
}

void arm2llvm::lift_bif3() {
  auto op1 = readFromOperand(3);
  auto op4 = readFromOperand(2);
  auto op3 = readFromOperand(1);
  auto res = createXor(op1, createAnd(createXor(op1, op4), op3));
  updateOutputReg(res);
}

void arm2llvm::lift_dup88() {
  auto i8 = getIntTy(8);
  auto t = createTrunc(readFromOperand(1), i8);
  updateOutputReg(dupElts(t, 8, 8));
}

void arm2llvm::lift_dup168() {
  auto i8 = getIntTy(8);
  auto t = createTrunc(readFromOperand(1), i8);
  updateOutputReg(dupElts(t, 16, 8));
}

void arm2llvm::lift_dup816() {
  auto i16 = getIntTy(16);
  auto t = createTrunc(readFromOperand(1), i16);
  updateOutputReg(dupElts(t, 8, 16));
}

void arm2llvm::lift_dup416() {
  auto i16 = getIntTy(16);
  auto t = createTrunc(readFromOperand(1), i16);
  updateOutputReg(dupElts(t, 4, 16));
}

void arm2llvm::lift_dup432() {
  auto i32 = getIntTy(32);
  auto t = createTrunc(readFromOperand(1), i32);
  updateOutputReg(dupElts(t, 4, 32));
}

void arm2llvm::lift_dup232() {
  auto i32 = getIntTy(32);
  auto t = createTrunc(readFromOperand(1), i32);
  updateOutputReg(dupElts(t, 2, 32));
}

void arm2llvm::lift_dup264() {
  updateOutputReg(dupElts(readFromOperand(1), 2, 64));
}

void arm2llvm::lift_dup232lane() {
  auto in = readFromVecOperand(1, 32, 4);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 2, 32));
}

void arm2llvm::lift_dup264lane() {
  auto in = readFromVecOperand(1, 64, 2);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 2, 64));
}

void arm2llvm::lift_dup416lane() {
  auto in = readFromVecOperand(1, 16, 8);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 4, 16));
}

void arm2llvm::lift_dup432lane() {
  auto in = readFromVecOperand(1, 32, 4);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 4, 32));
}

void arm2llvm::lift_dup88lane() {
  auto in = readFromVecOperand(1, 8, 16);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 8, 8));
}

void arm2llvm::lift_dup816lane() {
  auto in = readFromVecOperand(1, 16, 8);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 8, 16));
}

void arm2llvm::lift_dup168lane() {
  auto in = readFromVecOperand(1, 8, 16);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(dupElts(ext, 16, 8));
}

void arm2llvm::lift_dup8() {
  auto in = readFromVecOperand(1, 8, 16);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(ext);
}

void arm2llvm::lift_dup16() {
  auto in = readFromVecOperand(1, 16, 8);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(ext);
}

void arm2llvm::lift_dup32() {
  auto in = readFromVecOperand(1, 32, 4);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(ext);
}

void arm2llvm::lift_dup64() {
  auto in = readFromVecOperand(1, 64, 2);
  auto ext = createExtractElement(in, getImm(2));
  updateOutputReg(ext);
}

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
