#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/NZCV--Condition-Flags
void arm2llvm::lift_mrs() {
  auto imm = getImm(1);
  if (imm != 55824) {
    *out << "\nERROR: NZCV is the only supported case for MRS\n\n";
    exit(-1);
  }

  auto i64 = getIntTy(64);
  auto N = createZExt(getN(), i64);
  auto Z = createZExt(getZ(), i64);
  auto C = createZExt(getC(), i64);
  auto V = createZExt(getV(), i64);

  auto newN = createMaskedShl(N, getUnsignedIntConst(31, 64));
  auto newZ = createMaskedShl(Z, getUnsignedIntConst(30, 64));
  auto newC = createMaskedShl(C, getUnsignedIntConst(29, 64));
  auto newV = createMaskedShl(V, getUnsignedIntConst(28, 64));

  Value *res = getUnsignedIntConst(0, 64);
  res = createOr(res, newN);
  res = createOr(res, newZ);
  res = createOr(res, newC);
  res = createOr(res, newV);
  updateOutputReg(res);
}

// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/NZCV--Condition-Flags
void arm2llvm::lift_msr() {
  auto imm = getImm(0);
  if (imm != 55824) {
    *out << "\nERROR: NZCV is the only supported case for MSR\n\n";
    exit(-1);
  }

  auto i64_0 = getUnsignedIntConst(0, 64);
  auto i64_1 = getUnsignedIntConst(1, 64);

  auto Nmask = createMaskedShl(i64_1, getUnsignedIntConst(31, 64));
  auto Zmask = createMaskedShl(i64_1, getUnsignedIntConst(30, 64));
  auto Cmask = createMaskedShl(i64_1, getUnsignedIntConst(29, 64));
  auto Vmask = createMaskedShl(i64_1, getUnsignedIntConst(28, 64));

  auto reg = readFromOperand(1);
  auto Nval = createAnd(Nmask, reg);
  auto Zval = createAnd(Zmask, reg);
  auto Cval = createAnd(Cmask, reg);
  auto Vval = createAnd(Vmask, reg);

  setN(createICmp(ICmpInst::Predicate::ICMP_NE, Nval, i64_0));
  setZ(createICmp(ICmpInst::Predicate::ICMP_NE, Zval, i64_0));
  setC(createICmp(ICmpInst::Predicate::ICMP_NE, Cval, i64_0));
  setV(createICmp(ICmpInst::Predicate::ICMP_NE, Vval, i64_0));
}
