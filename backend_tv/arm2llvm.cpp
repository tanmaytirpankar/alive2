#include "backend_tv/arm2llvm.h"

#define AARCH64_MAP_IMPL
#include "aslp/aarch64_map.h"
#include "aslp/aslp_bridge.h"

const bool EXTRA_ABI_CHECKS = false;

using namespace std;

arm2llvm::arm2llvm(Module *LiftedModule, MCFunction &MF, Function &srcFn,
                   MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
                   const MCSubtargetInfo &STI, const MCInstrAnalysis &IA)
    : mc2llvm(LiftedModule, MF, srcFn, InstPrinter, MCE, STI, IA) {
  // sanity checking
  assert(disjoint(instrs_32, instrs_64));
  assert(disjoint(instrs_32, instrs_128));
  assert(disjoint(instrs_64, instrs_128));
  *out << (instrs_32.size() + instrs_64.size() + instrs_128.size())
       << " AArch64 instructions supported\n";
}

// Implemented library pseudocode for signed satuaration from A64 ISA manual
tuple<Value *, bool> arm2llvm::SignedSatQ(Value *i, unsigned bitWidth) {
  auto W = getBitWidth(i);
  assert(bitWidth < W);
  auto max = getSignedIntConst(((uint64_t)1 << (bitWidth - 1)) - 1, W);
  auto min = getSignedIntConst(-((uint64_t)1 << (bitWidth - 1)), W);
  auto *max_bitWidth = getSignedMaxConst(bitWidth);
  auto *min_bitWidth = getSignedMinConst(bitWidth);
  Value *i_bitWidth = createTrunc(i, getIntTy(bitWidth));

  auto sat = createOr(createICmp(ICmpInst::Predicate::ICMP_SGT, i, max),
                      createICmp(ICmpInst::Predicate::ICMP_SLT, i, min));
  auto max_or_min =
      createSelect(createICmp(ICmpInst::Predicate::ICMP_SGT, i, max),
                   max_bitWidth, min_bitWidth);

  auto res = createSelect(sat, max_or_min, i_bitWidth);
  return make_pair(res, sat);
}

// Implemented library pseudocode for unsigned satuaration from A64 ISA manual
tuple<Value *, bool> arm2llvm::UnsignedSatQ(Value *i, unsigned bitWidth) {
  auto W = getBitWidth(i);
  assert(bitWidth < W);
  auto max = getUnsignedIntConst(((uint64_t)1 << bitWidth) - 1, W);
  auto min = getUnsignedIntConst(0, W);
  Value *max_bitWidth = ConstantInt::get(Ctx, APInt::getMaxValue(bitWidth));
  Value *min_bitWidth = ConstantInt::get(Ctx, APInt::getMinValue(bitWidth));
  Value *i_bitWidth = createTrunc(i, getIntTy(bitWidth));

  auto sat = createOr(createICmp(ICmpInst::Predicate::ICMP_UGT, i, max),
                      createICmp(ICmpInst::Predicate::ICMP_ULT, i, min));
  auto max_or_min =
      createSelect(createICmp(ICmpInst::Predicate::ICMP_UGT, i, max),
                   max_bitWidth, min_bitWidth);

  auto res = createSelect(sat, max_or_min, i_bitWidth);
  return make_pair(res, sat);
}

// Implemented library pseudocode for satuaration from A64 ISA manual
tuple<Value *, bool> arm2llvm::SatQ(Value *i, unsigned bitWidth,
                                    bool isSigned) {
  auto [result, sat] =
      isSigned ? SignedSatQ(i, bitWidth) : UnsignedSatQ(i, bitWidth);

  return make_pair(result, sat);
}

bool arm2llvm::isSIMDandFPRegOperand(MCOperand &op) {
  assert(op.isReg() && "[isSIMDandFPRegOperand] expected register operand");
  unsigned reg = op.getReg();
  return (reg >= AArch64::Q0 && reg <= AArch64::Q31) ||
         (reg >= AArch64::D0 && reg <= AArch64::D31) ||
         (reg >= AArch64::S0 && reg <= AArch64::S31);
}

/*
 * the idea here is that if a parameter to the lifted function, or
 * the return value from the lifted function is, for example, 8
 * bits, then we only want to initialize the lower 8 bits of the
 * register or stack slot, with the remaining bits containing junk,
 * in order to detect cases where the compiler incorrectly emits
 * code depending on that junk. on the other hand, if a parameter is
 * signext or zeroext then we have to actually initialize those
 * higher bits.
 *
 * FIXME -- this code was originally developed for scalar parameters
 * and we're mostly sort of hoping it also works for vectors. this
 * should work fine as long as the only vectors we accept are 64 and
 * 128 bits, which seemed (as of Nov 2023) to be the only ones with
 * a stable ABI
 */
Value *arm2llvm::enforceSExtZExt(Value *V, bool isSExt, bool isZExt) {
  auto i8 = getIntTy(8);
  auto i32 = getIntTy(32);
  auto argTy = V->getType();
  unsigned targetWidth;

  // no work needed
  if (argTy->isPointerTy() || argTy->isVoidTy())
    return V;

  if (argTy->isVectorTy() || argTy->isFloatingPointTy()) {
    auto W = getBitWidth(V);
    argTy = getIntTy(W);
    V = createBitCast(V, argTy);
    if (W <= 64)
      targetWidth = 64;
    else
      targetWidth = 128;
  } else {
    targetWidth = 64;
  }

  assert(argTy->isIntegerTy());

  /*
   * i1 has special two ABI rules. first, by default, an i1 is
   * implicitly zero-extended to i8. this is from AAPCS64. second,
   * if the i1 is a signext parameter, then this overrides the
   * zero-extension rule. this is from the LLVM folks.
   */
  if (getBitWidth(V) == 1) {
    if (isSExt)
      V = createSExt(V, i32);
    else
      V = createZExt(V, i8);
  }

  if (isSExt) {
    if (getBitWidth(V) < 32)
      V = createSExt(V, i32);
    else if (getBitWidth(V) > 32 && getBitWidth(V) < targetWidth)
      V = createSExt(V, getIntTy(targetWidth));
  }

  if (isZExt && getBitWidth(V) < targetWidth)
    V = createZExt(V, getIntTy(targetWidth));

  // finally, pad out any remaining bits with junk (frozen poisons)
  auto junkBits = targetWidth - getBitWidth(V);
  if (junkBits > 0) {
    auto junk = createFreeze(PoisonValue::get(getIntTy(junkBits)));
    auto ext1 = createZExt(junk, getIntTy(targetWidth));
    auto shifted =
        createRawShl(ext1, getUnsignedIntConst(getBitWidth(V), targetWidth));
    auto ext2 = createZExt(V, getIntTy(targetWidth));
    V = createOr(shifted, ext2);
  }

  return V;
}

tuple<Value *, int, Value *> arm2llvm::getStoreParams() {
  auto &op0 = CurInst->getOperand(0);
  auto &op1 = CurInst->getOperand(1);
  auto &op2 = CurInst->getOperand(2);
  assert(op0.isReg() && op1.isReg());

  if (op2.isImm()) {
    auto baseReg = op1.getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    auto baseAddr = readPtrFromReg(baseReg);
    return make_tuple(baseAddr, op2.getImm(), readFromRegOld(op0.getReg()));
  } else {
    assert(op2.isExpr());
    auto [globalVar, _] = getExprVar(op2.getExpr());
    return make_tuple(globalVar, 0, readFromRegOld(op0.getReg()));
  }
}

// Creates instructions to store val in memory pointed by base + offset
// offset and size are in bytes
void arm2llvm::storeToMemoryImmOffset(Value *base, uint64_t offset,
                                      uint64_t size, Value *val) {
  // Get offset as a 64-bit LLVM constant
  auto offsetVal = getUnsignedIntConst(offset, 64);

  // Create a GEP instruction based on a byte addressing basis (8 bits)
  // returning pointer to base + offset
  assert(base);
  auto ptr = createGEP(getIntTy(8), base, {offsetVal}, "");

  // Store Value val in the pointer returned by the GEP instruction
  createStore(val, ptr);
}

unsigned arm2llvm::decodeRegSet(unsigned r) {
  switch (r) {
  case AArch64::D0:
  case AArch64::D0_D1:
  case AArch64::D0_D1_D2:
  case AArch64::D0_D1_D2_D3:
    return AArch64::D0;
  case AArch64::D1:
  case AArch64::D1_D2:
  case AArch64::D1_D2_D3:
  case AArch64::D1_D2_D3_D4:
    return AArch64::D1;
  case AArch64::D2:
  case AArch64::D2_D3:
  case AArch64::D2_D3_D4:
  case AArch64::D2_D3_D4_D5:
    return AArch64::D2;
  case AArch64::D3:
  case AArch64::D3_D4:
  case AArch64::D3_D4_D5:
  case AArch64::D3_D4_D5_D6:
    return AArch64::D3;
  case AArch64::D4:
  case AArch64::D4_D5:
  case AArch64::D4_D5_D6:
  case AArch64::D4_D5_D6_D7:
    return AArch64::D4;
  case AArch64::D5:
  case AArch64::D5_D6:
  case AArch64::D5_D6_D7:
  case AArch64::D5_D6_D7_D8:
    return AArch64::D5;
  case AArch64::D6:
  case AArch64::D6_D7:
  case AArch64::D6_D7_D8:
  case AArch64::D6_D7_D8_D9:
    return AArch64::D6;
  case AArch64::D7:
  case AArch64::D7_D8:
  case AArch64::D7_D8_D9:
  case AArch64::D7_D8_D9_D10:
    return AArch64::D7;
  case AArch64::D8:
  case AArch64::D8_D9:
  case AArch64::D8_D9_D10:
  case AArch64::D8_D9_D10_D11:
    return AArch64::D8;
  case AArch64::D9:
  case AArch64::D9_D10:
  case AArch64::D9_D10_D11:
  case AArch64::D9_D10_D11_D12:
    return AArch64::D9;
  case AArch64::D10:
  case AArch64::D10_D11:
  case AArch64::D10_D11_D12:
  case AArch64::D10_D11_D12_D13:
    return AArch64::D10;
  case AArch64::D11:
  case AArch64::D11_D12:
  case AArch64::D11_D12_D13:
  case AArch64::D11_D12_D13_D14:
    return AArch64::D11;
  case AArch64::D12:
  case AArch64::D12_D13:
  case AArch64::D12_D13_D14:
  case AArch64::D12_D13_D14_D15:
    return AArch64::D12;
  case AArch64::D13:
  case AArch64::D13_D14:
  case AArch64::D13_D14_D15:
  case AArch64::D13_D14_D15_D16:
    return AArch64::D13;
  case AArch64::D14:
  case AArch64::D14_D15:
  case AArch64::D14_D15_D16:
  case AArch64::D14_D15_D16_D17:
    return AArch64::D14;
  case AArch64::D15:
  case AArch64::D15_D16:
  case AArch64::D15_D16_D17:
  case AArch64::D15_D16_D17_D18:
    return AArch64::D15;
  case AArch64::D16:
  case AArch64::D16_D17:
  case AArch64::D16_D17_D18:
  case AArch64::D16_D17_D18_D19:
    return AArch64::D16;
  case AArch64::D17:
  case AArch64::D17_D18:
  case AArch64::D17_D18_D19:
  case AArch64::D17_D18_D19_D20:
    return AArch64::D17;
  case AArch64::D18:
  case AArch64::D18_D19:
  case AArch64::D18_D19_D20:
  case AArch64::D18_D19_D20_D21:
    return AArch64::D18;
  case AArch64::D19:
  case AArch64::D19_D20:
  case AArch64::D19_D20_D21:
  case AArch64::D19_D20_D21_D22:
    return AArch64::D19;
  case AArch64::D20:
  case AArch64::D20_D21:
  case AArch64::D20_D21_D22:
  case AArch64::D20_D21_D22_D23:
    return AArch64::D20;
  case AArch64::D21:
  case AArch64::D21_D22:
  case AArch64::D21_D22_D23:
  case AArch64::D21_D22_D23_D24:
    return AArch64::D21;
  case AArch64::D22:
  case AArch64::D22_D23:
  case AArch64::D22_D23_D24:
  case AArch64::D22_D23_D24_D25:
    return AArch64::D22;
  case AArch64::D23:
  case AArch64::D23_D24:
  case AArch64::D23_D24_D25:
  case AArch64::D23_D24_D25_D26:
    return AArch64::D23;
  case AArch64::D24:
  case AArch64::D24_D25:
  case AArch64::D24_D25_D26:
  case AArch64::D24_D25_D26_D27:
    return AArch64::D24;
  case AArch64::D25:
  case AArch64::D25_D26:
  case AArch64::D25_D26_D27:
  case AArch64::D25_D26_D27_D28:
    return AArch64::D25;
  case AArch64::D26:
  case AArch64::D26_D27:
  case AArch64::D26_D27_D28:
  case AArch64::D26_D27_D28_D29:
    return AArch64::D26;
  case AArch64::D27:
  case AArch64::D27_D28:
  case AArch64::D27_D28_D29:
  case AArch64::D27_D28_D29_D30:
    return AArch64::D27;
  case AArch64::D28:
  case AArch64::D28_D29:
  case AArch64::D28_D29_D30:
  case AArch64::D28_D29_D30_D31:
    return AArch64::D28;
  case AArch64::D29:
  case AArch64::D29_D30:
  case AArch64::D29_D30_D31:
  case AArch64::D29_D30_D31_D0:
    return AArch64::D29;
  case AArch64::D30:
  case AArch64::D30_D31:
  case AArch64::D30_D31_D0:
  case AArch64::D30_D31_D0_D1:
    return AArch64::D30;
  case AArch64::D31:
  case AArch64::D31_D0:
  case AArch64::D31_D0_D1:
  case AArch64::D31_D0_D1_D2:
    return AArch64::D31;
  case AArch64::Q0:
  case AArch64::Q0_Q1:
  case AArch64::Q0_Q1_Q2:
  case AArch64::Q0_Q1_Q2_Q3:
    return AArch64::Q0;
  case AArch64::Q1:
  case AArch64::Q1_Q2:
  case AArch64::Q1_Q2_Q3:
  case AArch64::Q1_Q2_Q3_Q4:
    return AArch64::Q1;
  case AArch64::Q2:
  case AArch64::Q2_Q3:
  case AArch64::Q2_Q3_Q4:
  case AArch64::Q2_Q3_Q4_Q5:
    return AArch64::Q2;
  case AArch64::Q3:
  case AArch64::Q3_Q4:
  case AArch64::Q3_Q4_Q5:
  case AArch64::Q3_Q4_Q5_Q6:
    return AArch64::Q3;
  case AArch64::Q4:
  case AArch64::Q4_Q5:
  case AArch64::Q4_Q5_Q6:
  case AArch64::Q4_Q5_Q6_Q7:
    return AArch64::Q4;
  case AArch64::Q5:
  case AArch64::Q5_Q6:
  case AArch64::Q5_Q6_Q7:
  case AArch64::Q5_Q6_Q7_Q8:
    return AArch64::Q5;
  case AArch64::Q6:
  case AArch64::Q6_Q7:
  case AArch64::Q6_Q7_Q8:
  case AArch64::Q6_Q7_Q8_Q9:
    return AArch64::Q6;
  case AArch64::Q7:
  case AArch64::Q7_Q8:
  case AArch64::Q7_Q8_Q9:
  case AArch64::Q7_Q8_Q9_Q10:
    return AArch64::Q7;
  case AArch64::Q8:
  case AArch64::Q8_Q9:
  case AArch64::Q8_Q9_Q10:
  case AArch64::Q8_Q9_Q10_Q11:
    return AArch64::Q8;
  case AArch64::Q9:
  case AArch64::Q9_Q10:
  case AArch64::Q9_Q10_Q11:
  case AArch64::Q9_Q10_Q11_Q12:
    return AArch64::Q9;
  case AArch64::Q10:
  case AArch64::Q10_Q11:
  case AArch64::Q10_Q11_Q12:
  case AArch64::Q10_Q11_Q12_Q13:
    return AArch64::Q10;
  case AArch64::Q11:
  case AArch64::Q11_Q12:
  case AArch64::Q11_Q12_Q13:
  case AArch64::Q11_Q12_Q13_Q14:
    return AArch64::Q11;
  case AArch64::Q12:
  case AArch64::Q12_Q13:
  case AArch64::Q12_Q13_Q14:
  case AArch64::Q12_Q13_Q14_Q15:
    return AArch64::Q12;
  case AArch64::Q13:
  case AArch64::Q13_Q14:
  case AArch64::Q13_Q14_Q15:
  case AArch64::Q13_Q14_Q15_Q16:
    return AArch64::Q13;
  case AArch64::Q14:
  case AArch64::Q14_Q15:
  case AArch64::Q14_Q15_Q16:
  case AArch64::Q14_Q15_Q16_Q17:
    return AArch64::Q14;
  case AArch64::Q15:
  case AArch64::Q15_Q16:
  case AArch64::Q15_Q16_Q17:
  case AArch64::Q15_Q16_Q17_Q18:
    return AArch64::Q15;
  case AArch64::Q16:
  case AArch64::Q16_Q17:
  case AArch64::Q16_Q17_Q18:
  case AArch64::Q16_Q17_Q18_Q19:
    return AArch64::Q16;
  case AArch64::Q17:
  case AArch64::Q17_Q18:
  case AArch64::Q17_Q18_Q19:
  case AArch64::Q17_Q18_Q19_Q20:
    return AArch64::Q17;
  case AArch64::Q18:
  case AArch64::Q18_Q19:
  case AArch64::Q18_Q19_Q20:
  case AArch64::Q18_Q19_Q20_Q21:
    return AArch64::Q18;
  case AArch64::Q19:
  case AArch64::Q19_Q20:
  case AArch64::Q19_Q20_Q21:
  case AArch64::Q19_Q20_Q21_Q22:
    return AArch64::Q19;
  case AArch64::Q20:
  case AArch64::Q20_Q21:
  case AArch64::Q20_Q21_Q22:
  case AArch64::Q20_Q21_Q22_Q23:
    return AArch64::Q20;
  case AArch64::Q21:
  case AArch64::Q21_Q22:
  case AArch64::Q21_Q22_Q23:
  case AArch64::Q21_Q22_Q23_Q24:
    return AArch64::Q21;
  case AArch64::Q22:
  case AArch64::Q22_Q23:
  case AArch64::Q22_Q23_Q24:
  case AArch64::Q22_Q23_Q24_Q25:
    return AArch64::Q22;
  case AArch64::Q23:
  case AArch64::Q23_Q24:
  case AArch64::Q23_Q24_Q25:
  case AArch64::Q23_Q24_Q25_Q26:
    return AArch64::Q23;
  case AArch64::Q24:
  case AArch64::Q24_Q25:
  case AArch64::Q24_Q25_Q26:
  case AArch64::Q24_Q25_Q26_Q27:
    return AArch64::Q24;
  case AArch64::Q25:
  case AArch64::Q25_Q26:
  case AArch64::Q25_Q26_Q27:
  case AArch64::Q25_Q26_Q27_Q28:
    return AArch64::Q25;
  case AArch64::Q26:
  case AArch64::Q26_Q27:
  case AArch64::Q26_Q27_Q28:
  case AArch64::Q26_Q27_Q28_Q29:
    return AArch64::Q26;
  case AArch64::Q27:
  case AArch64::Q27_Q28:
  case AArch64::Q27_Q28_Q29:
  case AArch64::Q27_Q28_Q29_Q30:
    return AArch64::Q27;
  case AArch64::Q28:
  case AArch64::Q28_Q29:
  case AArch64::Q28_Q29_Q30:
  case AArch64::Q28_Q29_Q30_Q31:
    return AArch64::Q28;
  case AArch64::Q29:
  case AArch64::Q29_Q30:
  case AArch64::Q29_Q30_Q31:
  case AArch64::Q29_Q30_Q31_Q0:
    return AArch64::Q29;
  case AArch64::Q30:
  case AArch64::Q30_Q31:
  case AArch64::Q30_Q31_Q0:
  case AArch64::Q30_Q31_Q0_Q1:
    return AArch64::Q30;
  case AArch64::Q31:
  case AArch64::Q31_Q0:
  case AArch64::Q31_Q0_Q1:
  case AArch64::Q31_Q0_Q1_Q2:
    return AArch64::Q31;
  default:
    assert(false && "missing case in decodeRegSet");
    break;
  }
}

Value *arm2llvm::tblHelper2(vector<Value *> &tbl, Value *idx, unsigned i) {
  if (i == tbl.size())
    return getUnsignedIntConst(0, 8);
  auto cond = createICmp(ICmpInst::Predicate::ICMP_ULT, idx,
                         getUnsignedIntConst((i + 1) * 16, 8));
  auto adjIdx = createSub(idx, getUnsignedIntConst(i * 16, 8));
  auto t = createExtractElement(tbl.at(i), adjIdx);
  auto f = tblHelper2(tbl, idx, i + 1);
  return createSelect(cond, t, f);
}

Value *arm2llvm::tblHelper(vector<Value *> &tbl, Value *idx) {
  return tblHelper2(tbl, idx, 0);
}

tuple<Value *, int> arm2llvm::getParamsLoadImmed() {
  auto &op0 = CurInst->getOperand(0);
  auto &op1 = CurInst->getOperand(1);
  auto &op2 = CurInst->getOperand(2);
  assert(op0.isReg() && op1.isReg());
  assert(op2.isImm());
  auto baseReg = op1.getReg();
  assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
         (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
         (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
  auto baseAddr = readPtrFromReg(baseReg);
  return make_pair(baseAddr, op2.getImm());
}

Value *arm2llvm::makeLoadWithOffset(Value *base, Value *offset, int size) {
  // Create a GEP instruction based on a byte addressing basis (8 bits)
  // returning pointer to base + offset
  assert(base);
  auto ptr = createGEP(getIntTy(8), base, {offset}, "");

  // Load Value val in the pointer returned by the GEP instruction
  Value *loaded = createLoad(getIntTy(8 * size), ptr);

  *out << "[makeLoadWithOffset size = " << size << "]\n";

  // FIXME -- this code should not be needed, the ABI guarantees
  // that loading an i1 should have the rest of the bits clear
  if (false && size == 1) {
    *out << "[it's a 1-byte load]\n";
    if (auto llvmInst = getCurLLVMInst()) {
      if (auto LI = dyn_cast<LoadInst>(llvmInst)) {
        if (LI->getType() == getIntTy(1)) {
          *out << "[following ABI rules for i1]\n";
          loaded = createAnd(loaded, getUnsignedIntConst(1, 8));
        } else {
          *out << "[LLVM inst for this ARM load isn't a load i1]\n";
        }
      } else {
        *out << "[LLVM inst for this ARM load is not a load]\n";
      }
    } else {
      *out << "[can't find an LLVM inst for this ARM load]\n";
    }
  }

  return loaded;
}

Value *arm2llvm::makeLoadWithOffset(Value *base, int offset, unsigned size) {
  return makeLoadWithOffset(base, getUnsignedIntConst(offset, 64), size);
}

tuple<Value *, Value *, Value *> arm2llvm::getParamsStoreReg() {
  auto &op0 = CurInst->getOperand(0);
  auto &op1 = CurInst->getOperand(1);
  auto &op2 = CurInst->getOperand(2);
  auto &op3 = CurInst->getOperand(3);
  auto &op4 = CurInst->getOperand(4);
  assert(op0.isReg() && op1.isReg() && op2.isReg());
  assert(op3.isImm() && op4.isImm());

  auto baseReg = op1.getReg();
  auto offsetReg = op2.getReg();
  auto extendTypeVal = op3.getImm();
  auto shiftAmtVal = op4.getImm();

  assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
         (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
         (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
  assert((offsetReg >= AArch64::X0 && offsetReg <= AArch64::X28) ||
         (offsetReg == AArch64::LR) || (offsetReg == AArch64::FP) ||
         (offsetReg == AArch64::XZR) ||
         (offsetReg >= AArch64::W0 && offsetReg <= AArch64::W30) ||
         (offsetReg == AArch64::WZR));

  int extTyp = 0, shiftAmt;
  if ((offsetReg >= AArch64::W0 && offsetReg <= AArch64::W30) ||
      offsetReg == AArch64::WZR) {
    extTyp = extendTypeVal ? SXTW : UXTW;
  } else if ((offsetReg >= AArch64::X0 && offsetReg <= AArch64::X28) ||
             offsetReg == AArch64::FP || offsetReg == AArch64::XZR) {
    // The manual assigns a value LSL to extTyp if extendTypeVal is 1
    // which for a value of 64 bits, is the same as UXTX
    extTyp = extendTypeVal ? SXTX : UXTX;
  }

  switch (CurInst->getOpcode()) {
  case AArch64::STRBBroW:
  case AArch64::STRBBroX:
  case AArch64::STRBroW:
  case AArch64::STRBroX:
    shiftAmt = 0;
    break;
  case AArch64::STRHHroW:
  case AArch64::STRHHroX:
  case AArch64::STRHroW:
  case AArch64::STRHroX:
    shiftAmt = shiftAmtVal ? 1 : 0;
    break;
  case AArch64::STRWroW:
  case AArch64::STRWroX:
  case AArch64::STRSroW:
  case AArch64::STRSroX:
    shiftAmt = shiftAmtVal ? 2 : 0;
    break;
  case AArch64::STRXroW:
  case AArch64::STRXroX:
  case AArch64::STRDroW:
  case AArch64::STRDroX:
    shiftAmt = shiftAmtVal ? 3 : 0;
    break;
  case AArch64::STRQroW:
  case AArch64::STRQroX:
    shiftAmt = shiftAmtVal ? 4 : 0;
    break;
  default:
    *out << "\nError Unknown opcode\n";
    visitError();
  }

  auto baseAddr = readPtrFromReg(baseReg);
  auto offset = extendAndShiftValue(readFromRegOld(offsetReg),
                                    (ExtendType)extTyp, shiftAmt);

  return make_tuple(baseAddr, offset, readFromRegOld(op0.getReg()));
}

void arm2llvm::doIndirectCall() {
  if (auto llvmInst = getCurLLVMInst()) {
    if (auto CI = dyn_cast<CallInst>(llvmInst)) {
      if (!CI->isIndirectCall()) {
        *out << "OOPS: expected BR/BLR to map to an indirect call\n\n";
        exit(-1);
      }
      auto reg = CurInst->getOperand(0).getReg();
      auto fnPtr = readPtrFromReg(reg);
      FunctionCallee FC(CI->getFunctionType(), fnPtr);
      doCall(FC, CI, "");
    } else {
      *out << "OOPS: debuginfo gave us something that's not a callinst\n";
      *out << "Can't process BR/BLR instruction\n\n";
      exit(-1);
    }
  } else {
    *out << "OOPS: no debuginfo mapping exists\n";
    *out << "Can't process BR/BLR instruction\n\n";
    exit(-1);
  }
}

void arm2llvm::doReturn() {
  auto i32 = getIntTy(32);
  auto i64 = getIntTy(64);

  if (EXTRA_ABI_CHECKS) {
    /*
     * ABI stuff: on all return paths, check that callee-saved +
     * other registers have been reset to their previous
     * values. these values were saved at the top of the function so
     * the trivially dominate all returns
     */
    // FIXME: check callee-saved vector registers
    // FIXME: make sure code doesn't touch 16, 17?
    // FIXME: check FP and LR?
    assertSame(initialSP, readFromRegTyped(AArch64::SP, getIntTy(64)));
    for (unsigned r = 19; r <= 28; ++r)
      assertSame(initialReg[r],
                 readFromRegTyped(AArch64::X0 + r, getIntTy(64)));
  }

  auto *retTyp = srcFn.getReturnType();
  if (retTyp->isVoidTy()) {
    createReturn(nullptr);
  } else {
    Value *retVal = nullptr;
    if (retTyp->isVectorTy() || retTyp->isFloatingPointTy()) {
      retVal = readFromRegTyped(AArch64::Q0, retTyp);
    } else {
      retVal = readFromRegOld(AArch64::X0);
    }
    if (retTyp->isPointerTy()) {
      retVal = new IntToPtrInst(retVal, PointerType::get(Ctx, 0), "", LLVMBB);
    } else {
      auto retWidth = DL.getTypeSizeInBits(retTyp);
      auto retValWidth = DL.getTypeSizeInBits(retVal->getType());

      if (retWidth < retValWidth)
        retVal = createTrunc(retVal, getIntTy(retWidth));

      // mask off any don't-care bits
      if (has_ret_attr && (origRetWidth < 32)) {
        assert(retWidth >= origRetWidth);
        assert(retWidth == 64);
        auto trunc = createTrunc(retVal, i32);
        retVal = createZExt(trunc, i64);
      }

      if ((retTyp->isVectorTy() || retTyp->isFloatingPointTy()) &&
          !has_ret_attr)
        retVal = createBitCast(retVal, retTyp);
    }
    createReturn(retVal);
  }
}

tuple<Value *, Value *, Value *, Value *> arm2llvm::FPCompare(Value *a,
                                                              Value *b) {
  return {
      createFCmp(FCmpInst::Predicate::FCMP_OLT, a, b),
      createFCmp(FCmpInst::Predicate::FCMP_OEQ, a, b),
      createFCmp(FCmpInst::Predicate::FCMP_UGT, a, b),
      createFCmp(FCmpInst::Predicate::FCMP_UNO, a, b),
  };
}

tuple<Value *, Value *, Value *, Value *>
arm2llvm::splitImmNZCV(uint64_t imm_flags) {
  assert(imm_flags < 16);
  return {
      getBoolConst((imm_flags & 8) != 0),
      getBoolConst((imm_flags & 4) != 0),
      getBoolConst((imm_flags & 2) != 0),
      getBoolConst((imm_flags & 1) != 0),
  };
}

bool arm2llvm::disjoint(const set<int> &a, const set<int> &b) {
  set<int> i;
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                        std::inserter(i, i.begin()));
  return i.empty();
}

int64_t arm2llvm::getImm(int idx) {
  return CurInst->getOperand(idx).getImm();
}

// Follows the "Library pseudocode for aarch64/instrs/extendreg/ExtendReg"
// from ARM manual
// val is always 64 bits and shiftAmt is always 0-4
Value *arm2llvm::extendAndShiftValue(Value *val, enum ExtendType extType,
                                     int shiftAmt) {
  assert(val->getType()->getIntegerBitWidth() == 64);
  assert(shiftAmt >= 0 && shiftAmt <= 4);

  // size is always 64 for offset shifting instructions
  //    auto size = getInstSize(opcode);
  auto size = 64;
  auto ty = getIntTy(size);
  auto isSigned = (extType & 0x4) != 0x4;

  // extendSize is necessary so that we can start with the word size
  // ARM wants us to (byte, half, full) and then sign extend to a new
  // size. Without extendSize being used for a trunc, a lot of masking
  // and more manual work to sign extend would be necessary
  unsigned extendSize = 8 << (extType & 0x3);

  // Make sure to not trunc to the same size as the parameter.
  if (extendSize != (unsigned)size) {
    auto truncType = getIntTy(extendSize);
    val = createTrunc(val, truncType);
    val = createCast(val, ty, isSigned ? Instruction::SExt : Instruction::ZExt);
  }

  // shift may not be there, it may just be the extend
  if (shiftAmt != 0)
    val = createMaskedShl(val, getUnsignedIntConst(shiftAmt, size));

  return val;
}

tuple<Value *, Value *> arm2llvm::getParamsLoadReg() {
  auto &op0 = CurInst->getOperand(0);
  auto &op1 = CurInst->getOperand(1);
  auto &op2 = CurInst->getOperand(2);
  auto &op3 = CurInst->getOperand(3);
  auto &op4 = CurInst->getOperand(4);
  assert(op0.isReg() && op1.isReg() && op2.isReg());
  assert(op3.isImm() && op4.isImm());

  auto baseReg = op1.getReg();
  auto offsetReg = op2.getReg();
  auto extendTypeVal = op3.getImm();
  auto shiftAmtVal = op4.getImm();

  assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
         (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
         (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
  assert((offsetReg >= AArch64::X0 && offsetReg <= AArch64::X28) ||
         (offsetReg == AArch64::LR) || (offsetReg == AArch64::FP) ||
         (offsetReg == AArch64::SP) || (offsetReg == AArch64::XZR) ||
         (offsetReg >= AArch64::W0 && offsetReg <= AArch64::W30) ||
         (offsetReg == AArch64::WZR));

  int extTyp = -1, shiftAmt;
  if ((offsetReg >= AArch64::W0 && offsetReg <= AArch64::W29) ||
      offsetReg == AArch64::WZR) {
    extTyp = extendTypeVal ? SXTW : UXTW;
  } else if ((offsetReg >= AArch64::X0 && offsetReg <= AArch64::X28) ||
             offsetReg == AArch64::FP || offsetReg == AArch64::XZR) {
    // The manual assigns a value LSL to extTyp if extendTypeVal is 1
    // which for a value of 64 bits, is the same as UXTX
    extTyp = extendTypeVal ? SXTX : UXTX;
  }

  switch (CurInst->getOpcode()) {
  case AArch64::LDRBBroW:
  case AArch64::LDRBBroX:
  case AArch64::LDRBroW:
  case AArch64::LDRBroX:
  case AArch64::LDRSBWroW:
  case AArch64::LDRSBWroX:
  case AArch64::LDRSBXroW:
  case AArch64::LDRSBXroX:
    shiftAmt = 0;
    break;
  case AArch64::LDRHHroW:
  case AArch64::LDRHHroX:
  case AArch64::LDRHroW:
  case AArch64::LDRHroX:
  case AArch64::LDRSHWroW:
  case AArch64::LDRSHWroX:
  case AArch64::LDRSHXroW:
  case AArch64::LDRSHXroX:
    shiftAmt = shiftAmtVal ? 1 : 0;
    break;
  case AArch64::LDRWroW:
  case AArch64::LDRWroX:
  case AArch64::LDRSroW:
  case AArch64::LDRSroX:
  case AArch64::LDRSWroW:
  case AArch64::LDRSWroX:
    shiftAmt = shiftAmtVal ? 2 : 0;
    break;
  case AArch64::LDRXroW:
  case AArch64::LDRXroX:
  case AArch64::LDRDroW:
  case AArch64::LDRDroX:
    shiftAmt = shiftAmtVal ? 3 : 0;
    break;
  case AArch64::LDRQroW:
  case AArch64::LDRQroX:
    shiftAmt = shiftAmtVal ? 4 : 0;
    break;
  default:
    *out << "\nError Unknown opcode\n";
    visitError();
  }

  auto baseAddr = readPtrFromReg(baseReg);
  auto offset = extendAndShiftValue(readFromRegTyped(offsetReg, getIntTy(64)),
                                    (ExtendType)extTyp, shiftAmt);

  return make_pair(baseAddr, offset);
}

// From https://github.com/agustingianni/retools
inline uint64_t arm2llvm::VFPExpandImm(uint64_t imm8, unsigned N) {
  unsigned E = ((N == 32) ? 8 : 11) - 2; // E in {6, 9}
  unsigned F = N - E - 1;                // F in {25, 54}
  uint64_t imm8_6 = (imm8 >> 6) & 1;     // imm8<6>
  uint64_t sign = (imm8 >> 7) & 1;       // imm8<7>
  // NOT(imm8<6>):Replicate(imm8<6>,{5, 8})
  uint64_t exp = ((imm8_6 ^ 1) << (E - 1)) | Replicate(imm8_6, E - 1);
  // imm8<5:0> : Zeros({19, 48})
  uint64_t frac = ((imm8 & 0x3f) << (F - 6)) | Replicate(0, F - 6);
  uint64_t res = (sign << (E + F)) | (exp << F) | frac;
  return res;
}

// From https://github.com/agustingianni/retools
// Implementation of: bits(64) AdvSIMDExpandImm(bit op, bits(4) cmode, bits(8)
// imm8)
inline uint64_t arm2llvm::AdvSIMDExpandImm(unsigned op, unsigned cmode,
                                           unsigned imm8) {
  uint64_t imm64 = 0;

  switch (cmode >> 1) {
  case 0:
    imm64 = Replicate32x2(imm8);
    break;
  case 1:
    imm64 = Replicate32x2(imm8 << 8);
    break;
  case 2:
    imm64 = Replicate32x2(imm8 << 16);
    break;
  case 3:
    imm64 = Replicate32x2(imm8 << 24);
    break;
  case 4:
    imm64 = Replicate16x4(imm8);
    break;
  case 5:
    imm64 = Replicate16x4(imm8 << 8);
    break;
  case 6:
    if ((cmode & 1) == 0)
      imm64 = Replicate32x2((imm8 << 8) | 0xFF);
    else
      imm64 = Replicate32x2((imm8 << 16) | 0xFFFF);
    break;
  case 7:
    if ((cmode & 1) == 0 && op == 0)
      imm64 = Replicate8x8(imm8);

    if ((cmode & 1) == 0 && op == 1) {
      imm64 = 0;
      imm64 |= (imm8 & 0x80) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x40) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x20) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x10) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x08) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x04) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x02) ? 0xFF : 0x00;
      imm64 <<= 8;
      imm64 |= (imm8 & 0x01) ? 0xFF : 0x00;
    }

    if ((cmode & 1) == 1 && op == 0) {
      uint64_t imm8_7 = (imm8 >> 7) & 1;
      uint64_t imm8_6 = (imm8 >> 6) & 1;
      uint64_t imm8_50 = imm8 & 63;
      uint64_t imm32 = (imm8_7 << (1 + 5 + 6 + 19)) |
                       ((imm8_6 ^ 1) << (5 + 6 + 19)) |
                       (Replicate(imm8_6, 5) << (6 + 19)) | (imm8_50 << 19);
      imm64 = Replicate32x2(imm32);
    }

    if ((cmode & 1) == 1 && op == 1) {
      // imm64 =
      // imm8<7>:NOT(imm8<6>):Replicate(imm8<6>,8):imm8<5:0>:Zeros(48);
      uint64_t imm8_7 = (imm8 >> 7) & 1;
      uint64_t imm8_6 = (imm8 >> 6) & 1;
      uint64_t imm8_50 = imm8 & 63;
      imm64 = (imm8_7 << 63) | ((imm8_6 ^ 1) << 62) |
              (Replicate(imm8_6, 8) << 54) | (imm8_50 << 48);
    }
    break;
  default:
    abort();
  }

  return imm64;
}

vector<Value *> arm2llvm::marshallArgs(FunctionType *fTy) {
  *out << "entering marshallArgs()\n";
  assert(fTy);
  if (fTy->getReturnType()->isStructTy()) {
    *out << "\nERROR: we don't support structures in return values yet\n\n";
    exit(-1);
  }
  if (fTy->getReturnType()->isArrayTy()) {
    *out << "\nERROR: we don't support arrays in return values yet\n\n";
    exit(-1);
  }
  unsigned vecArgNum = 0;
  unsigned scalarArgNum = 0;
  unsigned stackSlot = 0;
  vector<Value *> args;
  for (auto arg = fTy->param_begin(); arg != fTy->param_end(); ++arg) {
    Type *argTy = *arg;
    assert(argTy);
    *out << "  vecArgNum = " << vecArgNum << " scalarArgNum = " << scalarArgNum
         << "\n";
    if (argTy->isStructTy()) {
      *out << "\nERROR: we don't support structures in arguments yet\n\n";
      exit(-1);
    }
    if (argTy->isArrayTy()) {
      *out << "\nERROR: we don't support arrays in arguments yet\n\n";
      exit(-1);
    }
    Value *param{nullptr};
    if (argTy->isFloatingPointTy() || argTy->isVectorTy()) {
      if (vecArgNum < 8) {
        param = readFromRegTyped(AArch64::Q0 + vecArgNum, argTy);
        ++vecArgNum;
      } else {
        auto sz = getBitWidth(argTy);
        if (sz > 64 && ((stackSlot % 2) != 0)) {
          ++stackSlot;
          *out << "aligning stack slot for large vector parameter\n";
        }
        *out << "vector parameter going on stack with size = " << sz << "\n";
        auto SP = readPtrFromReg(AArch64::SP);
        auto addr = createGEP(getIntTy(64), SP,
                              {getUnsignedIntConst(stackSlot, 64)}, nextName());
        param = createBitCast(createLoad(getIntTy(sz), addr), argTy);
        ++stackSlot;
        if (sz > 64)
          ++stackSlot;
      }
    } else if (argTy->isIntegerTy() || argTy->isPointerTy()) {
      // FIXME check signext and zeroext
      if (scalarArgNum < 8) {
        param = readFromRegTyped(AArch64::X0 + scalarArgNum, getIntTy(64));
        ++scalarArgNum;
      } else {
        auto SP = readPtrFromReg(AArch64::SP);
        auto addr = createGEP(getIntTy(64), SP,
                              {getUnsignedIntConst(stackSlot, 64)}, nextName());
        param = createLoad(getIntTy(64), addr);
        ++stackSlot;
      }
      if (argTy->isPointerTy()) {
        param = new IntToPtrInst(param, PointerType::get(Ctx, 0), "", LLVMBB);
      } else {
        assert(argTy->getIntegerBitWidth() <= 64);
        if (argTy->getIntegerBitWidth() < 64)
          param = createTrunc(param, getIntTy(argTy->getIntegerBitWidth()));
      }
    } else {
      assert(false && "unknown arg type\n");
    }
    args.push_back(param);
  }
  *out << "marshalled up " << args.size() << " arguments\n";
  return args;
}

void arm2llvm::doCall(FunctionCallee FC, CallInst *llvmCI,
                      const string &calleeName) {
  *out << "entering doCall()\n";

  for (auto &arg : FC.getFunctionType()->params()) {
    if (auto vTy = dyn_cast<VectorType>(arg))
      checkVectorTy(vTy);
  }
  if (auto RT = dyn_cast<VectorType>(FC.getFunctionType()->getReturnType()))
    checkVectorTy(RT);

  auto args = marshallArgs(FC.getFunctionType());

  // ugh -- these functions have an LLVM "immediate" as their last
  // argument; this is not present in the assembly at all, we have
  // to provide it by hand
  if (calleeName == "llvm.memset.p0.i64" ||
      calleeName == "llvm.memset.p0.i32" ||
      calleeName == "llvm.memcpy.p0.p0.i64" ||
      calleeName == "llvm.memmove.p0.p0.i64")
    args[3] = getBoolConst(false);

  auto CI = CallInst::Create(FC, args, "", LLVMBB);

  bool sext{false}, zext{false};

  assert(llvmCI);
  if (llvmCI->hasFnAttr(Attribute::NoReturn)) {
    auto a = CI->getAttributes();
    auto a2 = a.addFnAttribute(Ctx, Attribute::NoReturn);
    CI->setAttributes(a2);
  }
  // NB we have to check for both function attributes and call site
  // attributes
  if (llvmCI->hasRetAttr(Attribute::SExt))
    sext = true;
  if (llvmCI->hasRetAttr(Attribute::ZExt))
    zext = true;
  auto calledFn = llvmCI->getCalledFunction();
  if (calledFn) {
    if (calledFn->hasRetAttribute(Attribute::SExt))
      sext = true;
    if (calledFn->hasRetAttribute(Attribute::ZExt))
      zext = true;
  }

  auto RV = enforceSExtZExt(CI, sext, zext);

  // invalidate machine state that is not guaranteed to be preserved across a
  // call
  invalidateReg(AArch64::N, 1);
  invalidateReg(AArch64::Z, 1);
  invalidateReg(AArch64::C, 1);
  invalidateReg(AArch64::V, 1);
  for (unsigned reg = 9; reg <= 15; ++reg)
    invalidateReg(AArch64::X0 + reg, 64);

  auto retTy = FC.getFunctionType()->getReturnType();
  if (retTy->isIntegerTy() || retTy->isPointerTy()) {
    updateReg(RV, AArch64::X0);
  } else if (retTy->isFloatingPointTy() || retTy->isVectorTy()) {
    updateReg(RV, AArch64::Q0);
  } else {
    assert(retTy->isVoidTy());
  }
}

Value *arm2llvm::conditionHolds(uint64_t cond) {
  assert(cond < 16);

  // cond<0> == '1' && cond != '1111'
  auto invert_bit = (cond & 1) && (cond != 15);

  cond >>= 1;

  auto cur_v = getV();
  auto cur_z = getZ();
  auto cur_n = getN();
  auto cur_c = getC();

  auto falseVal = getBoolConst(false);
  auto trueVal = getBoolConst(true);

  Value *res = nullptr;
  switch (cond) {
  case 0:
    res = cur_z;
    break; // EQ/NE
  case 1:
    res = cur_c;
    break; // CS/CC
  case 2:
    res = cur_n;
    break; // MI/PL
  case 3:
    res = cur_v;
    break; // VS/VC
  case 4: {
    // HI/LS: PSTATE.C == '1' && PSTATE.Z == '0'
    // C == 1
    auto c_cond = createICmp(ICmpInst::Predicate::ICMP_EQ, cur_c, trueVal);
    // Z == 0
    auto z_cond = createICmp(ICmpInst::Predicate::ICMP_EQ, cur_z, falseVal);
    // C == 1 && Z == 0
    res = createAnd(c_cond, z_cond);
    break;
  }
  case 5:
    // GE/LT PSTATE.N == PSTATE.V
    res = createICmp(ICmpInst::Predicate::ICMP_EQ, cur_n, cur_v);
    break;
  case 6: {
    // GT/LE PSTATE.N == PSTATE.V && PSTATE.Z == 0
    auto n_eq_v = createICmp(ICmpInst::Predicate::ICMP_EQ, cur_n, cur_v);
    auto z_cond = createICmp(ICmpInst::Predicate::ICMP_EQ, cur_z, falseVal);
    res = createAnd(n_eq_v, z_cond);
    break;
  }
  case 7:
    res = trueVal;
    break;
  default:
    assert(false && "invalid input to conditionHolds()");
    break;
  }

  assert(res != nullptr && "condition code was not generated");

  if (invert_bit)
    res = createNot(res);

  return res;
}

tuple<Value *, tuple<Value *, Value *, Value *, Value *>>
arm2llvm::addWithCarry(Value *l, Value *r, Value *carryIn) {
  assert(getBitWidth(l) == getBitWidth(r));
  assert(getBitWidth(carryIn) == 1);

  auto size = getBitWidth(l);

  // Deal with size+1 bit integers so that we can easily calculate the c/v
  // PSTATE bits.
  auto ty = l->getType();
  auto tyPlusOne = getIntTy(size + 1);
  auto carry = createZExt(carryIn, tyPlusOne);

  auto uAdd = createAdd(createZExt(l, tyPlusOne), createZExt(r, tyPlusOne));
  auto unsignedSum = createAdd(uAdd, carry);

  auto sAdd = createAdd(createSExt(l, tyPlusOne), createSExt(r, tyPlusOne));
  auto signedSum = createAdd(sAdd, carry);

  auto zero = getUnsignedIntConst(0, size);
  auto res = createTrunc(unsignedSum, ty);

  auto newN = createICmp(ICmpInst::Predicate::ICMP_SLT, res, zero);
  auto newZ = createICmp(ICmpInst::Predicate::ICMP_EQ, res, zero);
  auto newC = createICmp(ICmpInst::Predicate::ICMP_NE, unsignedSum,
                         createZExt(res, tyPlusOne));
  auto newV = createICmp(ICmpInst::Predicate::ICMP_NE, signedSum,
                         createSExt(res, tyPlusOne));

  return {res, {newN, newZ, newC, newV}};
};

void arm2llvm::setV(Value *V) {
  assert(getBitWidth(V) == 1);
  updateReg(V, AArch64::V);
}

void arm2llvm::setZ(Value *V) {
  assert(getBitWidth(V) == 1);
  updateReg(V, AArch64::Z);
}

void arm2llvm::setN(Value *V) {
  assert(getBitWidth(V) == 1);
  updateReg(V, AArch64::N);
}

void arm2llvm::setC(Value *V) {
  assert(getBitWidth(V) == 1);
  updateReg(V, AArch64::C);
}

void arm2llvm::setZUsingResult(Value *V) {
  auto W = getBitWidth(V);
  auto zero = getUnsignedIntConst(0, W);
  auto z = createICmp(ICmpInst::Predicate::ICMP_EQ, V, zero);
  setZ(z);
}

void arm2llvm::setNUsingResult(Value *V) {
  auto W = getBitWidth(V);
  auto zero = getUnsignedIntConst(0, W);
  auto n = createICmp(ICmpInst::Predicate::ICMP_SLT, V, zero);
  setN(n);
}

Value *arm2llvm::getV() {
  return createLoad(getIntTy(1), dealiasReg(AArch64::V));
}

Value *arm2llvm::getZ() {
  return createLoad(getIntTy(1), dealiasReg(AArch64::Z));
}

Value *arm2llvm::getN() {
  return createLoad(getIntTy(1), dealiasReg(AArch64::N));
}

Value *arm2llvm::getC() {
  return createLoad(getIntTy(1), dealiasReg(AArch64::C));
}

// Creates LLVM IR instructions which take two values with the same
// number of bits, bit casting them to vectors of numElts elements
// of size eltSize and doing an operation on them. In cases where
// LLVM does not have an appropriate vector instruction, we perform
// the operation element-wise.
Value *arm2llvm::createVectorOp(function<Value *(Value *, Value *)> op,
                                Value *a, Value *b, unsigned eltSize,
                                unsigned numElts, bool elementWise, extKind ext,
                                bool splatImm2, bool immShift, bool isUpper,
                                bool operandTypesDiffer) {
  if (splatImm2)
    b = splatImm(b, numElts, eltSize, immShift);

  auto vTy = getVecTy(eltSize, numElts);

  if (operandTypesDiffer) {
    if (isUpper) {
      a = createBitCast(a, getVecTy(2 * eltSize, numElts / 2));
    } else {
      a = createBitCast(a, getVecTy(2 * eltSize, numElts));
    }
  } else {
    a = createBitCast(a, vTy);
  }
  b = createBitCast(b, vTy);

  // some instructions double element widths
  if (ext == extKind::ZExt) {
    if (!operandTypesDiffer) {
      a = createZExt(a, getVecTy(2 * eltSize, numElts));
    }
    b = createZExt(b, getVecTy(2 * eltSize, numElts));
  }
  if (ext == extKind::SExt) {
    if (!operandTypesDiffer) {
      a = createSExt(a, getVecTy(2 * eltSize, numElts));
    }
    b = createSExt(b, getVecTy(2 * eltSize, numElts));
  }

  Value *res = nullptr;
  if (elementWise) {
    res = getUndefVec(numElts, eltSize);
    for (unsigned i = 0; i < numElts; ++i) {
      auto aa = createExtractElement(a, i);
      auto bb = createExtractElement(b, i);
      auto cc = op(aa, bb);
      res = createInsertElement(res, cc, i);
    }
  } else {
    // Some instructions use the upper half of the operands.
    if (isUpper) {
      assert(eltSize * numElts == 128);
      if (ext == extKind::ZExt || ext == extKind::SExt) {
        if (!operandTypesDiffer) {
          a = createBitCast(a, getVecTy(128, 2));
        }
        b = createBitCast(b, getVecTy(128, 2));
      } else {
        if (!operandTypesDiffer) {
          a = createBitCast(a, getVecTy(64, 2));
        }
        b = createBitCast(b, getVecTy(64, 2));
      }

      if (!operandTypesDiffer) {
        a = createExtractElement(a, 1);
        a = createBitCast(a, getVecTy(eltSize * 2, numElts / 2));
      }

      b = createExtractElement(b, 1);
      b = createBitCast(b, getVecTy(eltSize * 2, numElts / 2));
    }
    assert(getBitWidth(a) == getBitWidth(b) &&
           "Expected values of same bit width");
    res = op(a, b);
  }
  return res;
}

Value *arm2llvm::getIndexedElement(unsigned idx, unsigned eltSize,
                                   unsigned reg) {
  assert(getRegSize(reg) == 128 && "Expected 128-bit register");
  auto *ty = getVecTy(eltSize, 128 / eltSize);
  auto *r = readFromRegTyped(reg, ty);
  return createExtractElement(r, idx);
}

Value *arm2llvm::getIndexedFPElement(unsigned idx, unsigned eltSize,
                                     unsigned reg) {
  assert(getRegSize(reg) == 128 && "Expected 128-bit register");
  auto *element = getIndexedElement(idx, eltSize, reg);
  return createBitCast(element, getFPType(eltSize));
}

// Returns bitWidth corresponding the registers
unsigned arm2llvm::getRegSize(unsigned Reg) {
  if (Reg == AArch64::N || Reg == AArch64::Z || Reg == AArch64::C ||
      Reg == AArch64::V)
    return 1;
  if (Reg >= AArch64::B0 && Reg <= AArch64::B31)
    return 8;
  if (Reg >= AArch64::H0 && Reg <= AArch64::H31)
    return 16;
  if ((Reg >= AArch64::W0 && Reg <= AArch64::W30) ||
      (Reg >= AArch64::S0 && Reg <= AArch64::S31) || Reg == AArch64::WZR ||
      Reg == AArch64::WSP)
    return 32;
  if ((Reg >= AArch64::X0 && Reg <= AArch64::X28) ||
      (Reg >= AArch64::D0 && Reg <= AArch64::D31) || Reg == AArch64::XZR ||
      Reg == AArch64::SP || Reg == AArch64::FP || Reg == AArch64::LR)
    return 64;
  if (Reg >= AArch64::Q0 && Reg <= AArch64::Q31)
    return 128;
  assert(false && "unhandled register");
}

// Maps ARM registers to backing registers
unsigned arm2llvm::mapRegToBackingReg(unsigned Reg) {
  if (Reg == AArch64::WZR)
    return AArch64::XZR;
  else if (Reg == AArch64::W29)
    return AArch64::FP;
  else if (Reg == AArch64::W30)
    return AArch64::LR;
  else if (Reg == AArch64::WSP)
    return AArch64::SP;
  else if (Reg >= AArch64::W0 && Reg <= AArch64::W30)
    return Reg - AArch64::W0 + AArch64::X0;
  else if (Reg >= AArch64::X0 && Reg <= AArch64::X28)
    return Reg - AArch64::X0 + AArch64::X0;
  // Dealias rules for NEON SIMD/floating-point registers
  // https://developer.arm.com/documentation/den0024/a/AArch64-Floating-point-and-NEON/NEON-and-Floating-Point-architecture/Floating-point
  else if (Reg >= AArch64::B0 && Reg <= AArch64::B31)
    return Reg - AArch64::B0 + AArch64::Q0;
  else if (Reg >= AArch64::H0 && Reg <= AArch64::H31)
    return Reg - AArch64::H0 + AArch64::Q0;
  else if (Reg >= AArch64::S0 && Reg <= AArch64::S31)
    return Reg - AArch64::S0 + AArch64::Q0;
  else if (Reg >= AArch64::D0 && Reg <= AArch64::D31)
    return Reg - AArch64::D0 + AArch64::Q0;
  assert(RegFile[Reg] && "ERROR: Cannot have a register without a backing store"
                         " register corresponding it.");
  return Reg;
}

// return pointer to the backing store for a register, doing the
// necessary de-aliasing
Value *arm2llvm::dealiasReg(unsigned Reg) {
  auto RegAddr = RegFile[mapRegToBackingReg(Reg)];
  assert(RegAddr);
  return RegAddr;
}

// always does a full-width read
//
// TODO eliminate all uses of this and rename readFromRegTyped to readFromReg
Value *arm2llvm::readFromRegOld(unsigned Reg) {
  auto RegAddr = dealiasReg(Reg);
  return createLoad(getIntTy(getRegSize(mapRegToBackingReg(Reg))), RegAddr);
}

Value *arm2llvm::readFromRegTyped(unsigned Reg, Type *ty) {
  auto RegAddr = dealiasReg(Reg);
  return createLoad(ty, RegAddr);
}

Value *arm2llvm::readPtrFromReg(unsigned Reg) {
  auto RegAddr = dealiasReg(Reg);
  return createLoad(PointerType::get(Ctx, 0), RegAddr);
}

void arm2llvm::updateReg(Value *V, uint64_t reg, bool SExt) {
  // important -- squash updates to the zero register
  if (reg == AArch64::WZR || reg == AArch64::XZR)
    return;

  if (V->getType()->isVectorTy())
    V = createBitCast(V, getIntTy(getBitWidth(V)));
  else if (V->getType()->isPointerTy())
    V = createPtrToInt(V, getIntTy(getBitWidth(V)));
  else if (V->getType()->isFloatingPointTy())
    V = createBitCast(V, getIntTy(getBitWidth(V)));

  auto destRegSize = getRegSize(reg);
  auto realRegSize = getRegSize(mapRegToBackingReg(reg));

  // important to chop the value down to the destination register
  // size before extending it again
  if (destRegSize < getBitWidth(V))
    V = createTrunc(V, getIntTy(destRegSize));

  // now sign extend if asked and appropriate
  if (SExt && getBitWidth(V) < 32 && destRegSize == 32)
    V = createSExt(V, getIntTy(32));
  if (SExt && getBitWidth(V) < 64 && destRegSize == 64)
    V = createSExt(V, getIntTy(64));

  // annnnd zero out the rest of the destination reg!
  if (getBitWidth(V) < realRegSize)
    V = createZExt(V, getIntTy(realRegSize));

  assert(getBitWidth(V) == realRegSize &&
         "ERROR: register update should be full width");

  createStore(V, dealiasReg(reg));
}

Value *arm2llvm::readInputReg(int idx) {
  auto op = CurInst->getOperand(idx);
  assert(op.isReg());
  auto reg = op.getReg();
  auto regSize = getRegSize(reg);
  return readFromRegTyped(reg, getIntTy(regSize));
}

// FIXME: stop using this -- instructions should know what they're loading
// FIXME: then remove getInstSize!
// TODO: make it so that lshr generates code on register lookups
// some instructions make use of this, and the semantics need to be
// worked out
Value *arm2llvm::readFromOperand(int idx, unsigned size) {
  auto op = CurInst->getOperand(idx);
  if (!size)
    size = getInstSize(CurInst->getOpcode());
  // Expr operand is required for a combination of ADRP and ADDXri address
  // calculation
  assert(op.isImm() || op.isReg() || op.isExpr());

  if (!(size == 8 || size == 16 || size == 32 || size == 64 || size == 128)) {
    *out << "\nERROR: Registers can be 8, 16, 32, 64 or 128 bits\n\n";
    exit(-1);
  }

  Value *V = nullptr;
  if (op.isImm()) {
    V = getUnsignedIntConst(op.getImm(), size);
  } else if (op.isReg()) {
    V = readFromRegOld(op.getReg());
    if (size == 32) {
      // Always truncate since backing registers are either 64 or 128 bits
      V = createTrunc(V, getIntTy(32));
    } else if (size == 64 && getBitWidth(V) == 128) {
      // Only truncate if the backing register read from is 128 bits
      // i.e. If value V is 128 bits
      V = createTrunc(V, getIntTy(64));
    }
  } else {
    auto [val, _] = getExprVar(op.getExpr());
    V = val;
  }
  return V;
}

Value *arm2llvm::readFromFPOperand(int idx, unsigned size) {
  assert((size == 16 || size == 32 || size == 64) &&
         "Only 16, 32 and 64 bit FP regs");
  auto val = readFromOperand(idx, size);
  return createBitCast(val, getFPType(size));
}

Value *arm2llvm::readFromVecOperand(int idx, unsigned eltSize, unsigned numElts,
                                    bool isUpperHalf, bool isFP) {
  VectorType *ty;
  auto regVal = readFromOperand(idx);
  if (eltSize * numElts < getBitWidth(regVal)) {
    regVal = createTrunc(regVal, getIntTy(eltSize * numElts));
  }

  assert(eltSize * numElts == getBitWidth(regVal));
  if (isUpperHalf) {
    assert(eltSize * numElts == 128);
    auto casted = createBitCast(regVal, getVecTy(64, 2));

    regVal = createExtractElement(casted, 1);
    ty = getVecTy(eltSize, numElts / 2, isFP);
  } else {
    ty = getVecTy(eltSize, numElts, isFP);
  }
  return createBitCast(regVal, ty);
}

void arm2llvm::updateOutputReg(Value *V, bool SExt) {
  auto destReg = CurInst->getOperand(0).getReg();
  updateReg(V, destReg, SExt);
}

Value *arm2llvm::splatImm(Value *v, unsigned numElts, unsigned eltSize,
                          bool shift) {
  if (shift) {
    assert(CurInst->getOperand(3).isImm());
    v = regShift(v, getImm(3));
  }
  if (getBitWidth(v) > eltSize)
    v = createTrunc(v, getIntTy(eltSize));
  Value *res = getUndefVec(numElts, eltSize);
  for (unsigned i = 0; i < numElts; ++i)
    res = createInsertElement(res, v, i);
  return res;
}

const set<int> arm2llvm::s_flag = {
    AArch64::ADDSWri, AArch64::ADDSWrs, AArch64::ADDSWrx, AArch64::ADDSXri,
    AArch64::ADDSXrs, AArch64::ADDSXrx, AArch64::SUBSWri, AArch64::SUBSWrs,
    AArch64::SUBSWrx, AArch64::SUBSXri, AArch64::SUBSXrs, AArch64::SUBSXrx,
    AArch64::ANDSWri, AArch64::ANDSWrr, AArch64::ANDSWrs, AArch64::ANDSXri,
    AArch64::ANDSXrr, AArch64::ANDSXrs, AArch64::BICSWrs, AArch64::BICSXrs,
    AArch64::ADCSWr,  AArch64::ADCSXr,  AArch64::SBCSWr,  AArch64::SBCSXr,
};

const set<int> arm2llvm::instrs_32 = {
    AArch64::TBNZW,
    AArch64::TBZW,
    AArch64::CBZW,
    AArch64::CBNZW,
    AArch64::ADDWrx,
    AArch64::ADDSWrs,
    AArch64::ADDSWri,
    AArch64::ADDWrs,
    AArch64::ADDWri,
    AArch64::ADDSWrx,
    AArch64::ADCWr,
    AArch64::ADCSWr,
    AArch64::ASRVWr,
    AArch64::SBCSWr,
    AArch64::SBCWr,
    AArch64::SUBWri,
    AArch64::SUBWrs,
    AArch64::SUBWrx,
    AArch64::SUBSWrs,
    AArch64::SUBSWri,
    AArch64::SUBSWrx,
    AArch64::SBFMWri,
    AArch64::CSELWr,
    AArch64::ANDWri,
    AArch64::ANDWrr,
    AArch64::ANDWrs,
    AArch64::ANDSWri,
    AArch64::ANDSWrr,
    AArch64::ANDSWrs,
    AArch64::MADDWrrr,
    AArch64::MSUBWrrr,
    AArch64::EORWri,
    AArch64::CSINVWr,
    AArch64::CSINCWr,
    AArch64::MOVZWi,
    AArch64::MOVNWi,
    AArch64::MOVKWi,
    AArch64::LSLVWr,
    AArch64::LSRVWr,
    AArch64::ORNWrs,
    AArch64::UBFMWri,
    AArch64::BFMWri,
    AArch64::ORRWrs,
    AArch64::ORRWri,
    AArch64::SDIVWr,
    AArch64::UDIVWr,
    AArch64::EXTRWrri,
    AArch64::EORWrs,
    AArch64::RORVWr,
    AArch64::RBITWr,
    AArch64::CLZWr,
    AArch64::REVWr,
    AArch64::CSNEGWr,
    AArch64::BICWrs,
    AArch64::BICSWrs,
    AArch64::EONWrs,
    AArch64::REV16Wr,
    AArch64::Bcc,
    AArch64::CCMPWr,
    AArch64::CCMPWi,
    AArch64::LDRWui,
    AArch64::LDRBBroW,
    AArch64::LDRBBroX,
    AArch64::LDRBroW,
    AArch64::LDRBroX,
    AArch64::LDRHHroW,
    AArch64::LDRHHroX,
    AArch64::LDRHroW,
    AArch64::LDRHroX,
    AArch64::LDRWroW,
    AArch64::LDRWroX,
    AArch64::LDRSBWroW,
    AArch64::LDRSBWroX,
    AArch64::LDRSBXroW,
    AArch64::LDRSBXroX,
    AArch64::LDRSHWroW,
    AArch64::LDRSHWroX,
    AArch64::LDRSHXroW,
    AArch64::LDRSHXroX,
    AArch64::LDRSWroW,
    AArch64::LDRSWroX,
    AArch64::LDRSui,
    AArch64::LDRBBui,
    AArch64::LDRBui,
    AArch64::LDRSBWui,
    AArch64::LDRSHWui,
    AArch64::LDRSBWpre,
    AArch64::LDRSHWpre,
    AArch64::LDRSBWpost,
    AArch64::LDRSHWpost,
    AArch64::LDRHHui,
    AArch64::LDRHui,
    AArch64::LDURBi,
    AArch64::LDURBBi,
    AArch64::LDURHi,
    AArch64::LDURHHi,
    AArch64::LDURSi,
    AArch64::LDURWi,
    AArch64::LDURSBWi,
    AArch64::LDURSHWi,
    AArch64::LDRBBpre,
    AArch64::LDRBpre,
    AArch64::LDRHHpre,
    AArch64::LDRHpre,
    AArch64::LDRWpre,
    AArch64::LDRSpre,
    AArch64::LDRBBpost,
    AArch64::LDRBpost,
    AArch64::LDRHHpost,
    AArch64::LDRHpost,
    AArch64::LDRWpost,
    AArch64::LDRSpost,
    AArch64::STRBBpost,
    AArch64::STRBpost,
    AArch64::STRHHpost,
    AArch64::STRHpost,
    AArch64::STRWpost,
    AArch64::STRSpost,
    AArch64::STRWui,
    AArch64::STRBBroW,
    AArch64::STRBBroX,
    AArch64::STRBroW,
    AArch64::STRBroX,
    AArch64::STRHHroW,
    AArch64::STRHHroX,
    AArch64::STRHroW,
    AArch64::STRHroX,
    AArch64::STRWroW,
    AArch64::STRWroX,
    AArch64::CCMNWi,
    AArch64::CCMNWr,
    AArch64::STRBBui,
    AArch64::STRBui,
    AArch64::STPWi,
    AArch64::STPSi,
    AArch64::STPWpre,
    AArch64::STPSpre,
    AArch64::STPWpost,
    AArch64::STPSpost,
    AArch64::STRHHui,
    AArch64::STRHui,
    AArch64::STURBBi,
    AArch64::STURBi,
    AArch64::STURHHi,
    AArch64::STURHi,
    AArch64::STURWi,
    AArch64::STURSi,
    AArch64::STRSui,
    AArch64::LDPWi,
    AArch64::LDPSi,
    AArch64::LDPWpre,
    AArch64::LDPSpre,
    AArch64::LDPWpost,
    AArch64::LDPSpost,
    AArch64::STRBBpre,
    AArch64::STRBpre,
    AArch64::STRHHpre,
    AArch64::STRHpre,
    AArch64::STRWpre,
    AArch64::STRSpre,
    AArch64::FADDSrr,
    AArch64::FSUBSrr,
    AArch64::FCMPSrr,
    AArch64::FCMPESrr,
    AArch64::FCMPSri,
    AArch64::FCMPESri,
    AArch64::FMOVSWr,
    AArch64::INSvi32gpr,
    AArch64::INSvi16gpr,
    AArch64::INSvi8gpr,
    AArch64::FCVTZUUWHr,
    AArch64::FCVTZUUWSr,
    AArch64::FCVTZUUXHr,
    AArch64::FCVTZUUXSr,
    AArch64::FCVTZSUWHr,
    AArch64::FCVTZSUWSr,
    AArch64::FCVTZSUXHr,
    AArch64::FCVTZSUXSr,
    AArch64::FCVTZSv1i32,
    AArch64::FCVTSHr,
    AArch64::FCVTDHr,
    AArch64::FCVTHSr,
    AArch64::FCVTDSr,
    AArch64::FCVTZSUWSr,
    AArch64::FCSELSrrr,
    AArch64::FMULSrr,
    AArch64::FDIVSrr,
    AArch64::FMADDSrrr,
    AArch64::FMSUBSrrr,
    AArch64::FNMADDSrrr,
    AArch64::FNMSUBSrrr,
    AArch64::FNMULSrr,
    AArch64::FSQRTSr,
    AArch64::FMULv1i32_indexed,
    AArch64::FMLAv1i32_indexed,
    AArch64::FMLSv1i32_indexed,
    AArch64::FABSSr,
    AArch64::UQADDv1i32,
    AArch64::SQSUBv1i32,
    AArch64::SQADDv1i32,
    AArch64::FMOVSr,
    AArch64::FNEGSr,
    AArch64::BRK,
    AArch64::UCVTFUWSri,
    AArch64::UCVTFUWDri,
    AArch64::SCVTFUWSri,
    AArch64::SCVTFUWDri,
    AArch64::SCVTFv1i32,
    AArch64::FRINTXSr,
    AArch64::FRINTASr,
    AArch64::FRINTMSr,
    AArch64::FRINTPSr,
    AArch64::UQXTNv1i8,
    AArch64::UQXTNv1i16,
    AArch64::UQXTNv1i32,
    AArch64::SQXTNv1i8,
    AArch64::SQXTNv1i16,
    AArch64::SQXTNv1i32,
    AArch64::FMINSrr,
    AArch64::FMINNMSrr,
    AArch64::FMAXSrr,
    AArch64::FMAXNMSrr,
    AArch64::FCCMPSrr,
};

const set<int> arm2llvm::instrs_64 = {
    AArch64::BL,
    AArch64::BR,
    AArch64::BLR,
    AArch64::ADDXrx,
    AArch64::ADDSXrs,
    AArch64::ADDSXri,
    AArch64::ADDXrs,
    AArch64::ADDXri,
    AArch64::ADDSXrx,
    AArch64::ADDv2i32,
    AArch64::ADDv4i16,
    AArch64::ADDv8i8,
    AArch64::USHLv8i8,
    AArch64::USHLv1i64,
    AArch64::USHLv4i16,
    AArch64::USHLv2i32,
    AArch64::SHLLv8i8,
    AArch64::SHLLv4i16,
    AArch64::SHLLv2i32,
    AArch64::SSHLv8i8,
    AArch64::SSHLv1i64,
    AArch64::SSHLv4i16,
    AArch64::SSHLv2i32,
    AArch64::SUBv1i64,
    AArch64::SUBv2i32,
    AArch64::SUBv4i16,
    AArch64::SUBv8i8,
    AArch64::UABAv8i8,
    AArch64::UABAv4i16,
    AArch64::UABAv2i32,
    AArch64::UABDv8i8,
    AArch64::UABDv4i16,
    AArch64::UABDv2i32,
    AArch64::SABAv8i8,
    AArch64::SABAv4i16,
    AArch64::SABAv2i32,
    AArch64::SABDv8i8,
    AArch64::SABDv4i16,
    AArch64::SABDv2i32,
    AArch64::ADCXr,
    AArch64::ADCSXr,
    AArch64::ASRVXr,
    AArch64::SBCXr,
    AArch64::SBCSXr,
    AArch64::SUBXri,
    AArch64::SUBXrs,
    AArch64::SUBXrx,
    AArch64::SUBXrx64,
    AArch64::SUBSXrs,
    AArch64::SUBSXri,
    AArch64::SUBSXrx,
    AArch64::SBFMXri,
    AArch64::CSELXr,
    AArch64::ANDXri,
    AArch64::ANDXrr,
    AArch64::ANDXrs,
    AArch64::ANDSXri,
    AArch64::ANDSXrr,
    AArch64::ANDSXrs,
    AArch64::MADDXrrr,
    AArch64::MSUBXrrr,
    AArch64::EORXri,
    AArch64::CSINVXr,
    AArch64::CSINCXr,
    AArch64::MOVZXi,
    AArch64::MOVNXi,
    AArch64::MOVKXi,
    AArch64::LSLVXr,
    AArch64::LSRVXr,
    AArch64::ORNXrs,
    AArch64::UBFMXri,
    AArch64::BFMXri,
    AArch64::ORRXrs,
    AArch64::ORRXri,
    AArch64::SDIVXr,
    AArch64::UDIVXr,
    AArch64::EXTRXrri,
    AArch64::EORXrs,
    AArch64::SMADDLrrr,
    AArch64::UMADDLrrr,
    AArch64::RORVXr,
    AArch64::RBITXr,
    AArch64::CLZXr,
    AArch64::REVXr,
    AArch64::CSNEGXr,
    AArch64::BICXrs,
    AArch64::BICSXrs,
    AArch64::EONXrs,
    AArch64::SMULHrr,
    AArch64::UMULHrr,
    AArch64::REV32Xr,
    AArch64::REV16Xr,
    AArch64::SMSUBLrrr,
    AArch64::UMSUBLrrr,
    AArch64::PHI,
    AArch64::TBZX,
    AArch64::TBNZX,
    AArch64::B,
    AArch64::CBZX,
    AArch64::CBNZX,
    AArch64::CCMPXr,
    AArch64::CCMPXi,
    AArch64::LDRXui,
    AArch64::LDRXpre,
    AArch64::LDRDpre,
    AArch64::LDRXpost,
    AArch64::LDRDpost,
    AArch64::LDPXpre,
    AArch64::LDPDpre,
    AArch64::LDPXpost,
    AArch64::LDPDpost,
    AArch64::LDPSWi,
    AArch64::LDPSWpre,
    AArch64::LDPSWpost,
    AArch64::LDPXi,
    AArch64::LDPDi,
    AArch64::LDRDui,
    AArch64::LDRXroW,
    AArch64::LDRXroX,
    AArch64::LDURDi,
    AArch64::LDURXi,
    AArch64::LDURSBXi,
    AArch64::LDURSHXi,
    AArch64::LDURSWi,
    AArch64::STRDui,
    AArch64::MSR,
    AArch64::MRS,
    AArch64::LDRSBXui,
    AArch64::LDRSHXui,
    AArch64::LDRSWui,
    AArch64::LDRSBXpre,
    AArch64::LDRSHXpre,
    AArch64::LDRSWpre,
    AArch64::LDRSBXpost,
    AArch64::LDRSHXpost,
    AArch64::LDRSWpost,
    AArch64::STRXui,
    AArch64::STRXpost,
    AArch64::STRDpost,
    AArch64::STRXroW,
    AArch64::STRXroX,
    AArch64::STPXi,
    AArch64::STPDi,
    AArch64::STPXpre,
    AArch64::STPDpre,
    AArch64::STPXpost,
    AArch64::STPDpost,
    AArch64::CCMNXi,
    AArch64::CCMNXr,
    AArch64::STURXi,
    AArch64::STURDi,
    AArch64::ADRP,
    AArch64::STRXpre,
    AArch64::STRDpre,
    AArch64::FADDDrr,
    AArch64::FMULDrr,
    AArch64::FDIVDrr,
    AArch64::FSQRTDr,
    AArch64::FNMULDrr,
    AArch64::FMADDDrrr,
    AArch64::FMSUBDrrr,
    AArch64::FNMADDDrrr,
    AArch64::FNMSUBDrrr,
    AArch64::FMULv1i64_indexed,
    AArch64::FMLAv1i64_indexed,
    AArch64::FMLSv1i64_indexed,
    AArch64::FABSDr,
    AArch64::FSUBDrr,
    AArch64::FCMPDrr,
    AArch64::FCMPEDrr,
    AArch64::FCMPDri,
    AArch64::FCMPEDri,
    AArch64::NOTv8i8,
    AArch64::CNTv8i8,
    AArch64::ANDv8i8,
    AArch64::EORv8i8,
    AArch64::FMOVDXr,
    AArch64::INSvi64gpr,
    AArch64::MOVID,
    AArch64::FCVTZUUWDr,
    AArch64::FCVTZUUXDr,
    AArch64::FCVTZSUWDr,
    AArch64::FCVTZSUXDr,
    AArch64::FCVTZSv1i64,
    AArch64::FCVTHDr,
    AArch64::FCVTSDr,
    AArch64::FMOVDr,
    AArch64::FMOVv2f32_ns,
    AArch64::DUPv2i32gpr,
    AArch64::UMULLv2i32_v2i64,
    AArch64::USHLLv8i8_shift,
    AArch64::USHLLv2i32_shift,
    AArch64::USHLLv4i16_shift,
    AArch64::SLIv8i8_shift,
    AArch64::SLIv4i16_shift,
    AArch64::SLIv2i32_shift,
    AArch64::SLId,
    AArch64::SRIv8i8_shift,
    AArch64::SRIv4i16_shift,
    AArch64::SRIv2i32_shift,
    AArch64::SRId,
    AArch64::XTNv8i8,
    AArch64::XTNv4i16,
    AArch64::XTNv2i32,
    AArch64::UQXTNv8i8,
    AArch64::UQXTNv4i16,
    AArch64::UQXTNv2i32,
    AArch64::SQXTNv8i8,
    AArch64::SQXTNv4i16,
    AArch64::SQXTNv2i32,
    AArch64::MLSv2i32,
    AArch64::NEGv1i64,
    AArch64::NEGv4i16,
    AArch64::NEGv8i8,
    AArch64::NEGv2i32,
    AArch64::DUPv8i8gpr,
    AArch64::DUPv4i16gpr,
    AArch64::FCSELDrrr,
    AArch64::UADDLVv8i8v,
    AArch64::UADDLVv4i16v,
    AArch64::SADDLVv8i8v,
    AArch64::SADDLVv4i16v,
    AArch64::UADDLPv8i8_v4i16,
    AArch64::UADDLPv4i16_v2i32,
    AArch64::UADDLPv2i32_v1i64,
    AArch64::SADDLPv8i8_v4i16,
    AArch64::SADDLPv4i16_v2i32,
    AArch64::SADDLPv2i32_v1i64,
    AArch64::UABALv8i8_v8i16,
    AArch64::UABALv4i16_v4i32,
    AArch64::UABALv2i32_v2i64,
    AArch64::UABDLv8i8_v8i16,
    AArch64::UABDLv4i16_v4i32,
    AArch64::UABDLv2i32_v2i64,
    AArch64::SABALv8i8_v8i16,
    AArch64::SABALv4i16_v4i32,
    AArch64::SABALv2i32_v2i64,
    AArch64::SABDLv8i8_v8i16,
    AArch64::SABDLv4i16_v4i32,
    AArch64::SABDLv2i32_v2i64,
    AArch64::UADALPv8i8_v4i16,
    AArch64::UADALPv4i16_v2i32,
    AArch64::UADALPv2i32_v1i64,
    AArch64::SADALPv8i8_v4i16,
    AArch64::SADALPv4i16_v2i32,
    AArch64::SADALPv2i32_v1i64,
    AArch64::BIFv8i8,
    AArch64::BSLv8i8,
    AArch64::BICv4i16,
    AArch64::BICv8i8,
    AArch64::BICv2i32,
    AArch64::ADDVv8i8v,
    AArch64::ADDVv4i16v,
    AArch64::SHLv8i8_shift,
    AArch64::SHLv4i16_shift,
    AArch64::SHLv2i32_shift,
    AArch64::SHLd,
    AArch64::SSHRv8i8_shift,
    AArch64::SSHRv4i16_shift,
    AArch64::SSHRv2i32_shift,
    AArch64::SSHRd,
    AArch64::SSRAv8i8_shift,
    AArch64::SSRAv4i16_shift,
    AArch64::SSRAv2i32_shift,
    AArch64::ADDPv8i8,
    AArch64::ADDPv4i16,
    AArch64::ADDPv2i32,
    AArch64::CMEQv2i32,
    AArch64::CMHIv8i8,
    AArch64::CMHIv4i16,
    AArch64::CMHIv1i64,
    AArch64::CMHIv2i32,
    AArch64::CMHIv1i64,
    AArch64::CMGTv1i64,
    AArch64::CMEQv2i32rz,
    AArch64::CMEQv4i16,
    AArch64::CMEQv4i16rz,
    AArch64::CMEQv8i8,
    AArch64::CMEQv8i8rz,
    AArch64::CMGEv2i32,
    AArch64::CMGEv1i64,
    AArch64::CMGEv4i16,
    AArch64::CMGEv8i8,
    AArch64::CMGTv2i32,
    AArch64::CMGTv4i16,
    AArch64::CMGTv8i8,
    AArch64::CMHSv1i64,
    AArch64::CMHSv2i32,
    AArch64::CMHSv4i16,
    AArch64::CMHSv8i8,
    AArch64::CMLTv2i32rz,
    AArch64::CMLTv4i16rz,
    AArch64::CMLTv8i8rz,
    AArch64::CMTSTv8i8,
    AArch64::SSHLLv8i8_shift,
    AArch64::SSHLLv4i16_shift,
    AArch64::SSHLLv2i32_shift,
    AArch64::ZIP2v4i16,
    AArch64::ZIP2v2i32,
    AArch64::ZIP2v8i8,
    AArch64::ZIP1v4i16,
    AArch64::ZIP1v2i32,
    AArch64::ZIP1v8i8,
    AArch64::RBITv8i8,
    AArch64::BITv8i8,
    AArch64::MULv2i32_indexed,
    AArch64::MULv4i16_indexed,
    AArch64::MULv2i32,
    AArch64::MULv8i8,
    AArch64::MULv4i16,
    AArch64::UZP2v8i8,
    AArch64::UZP2v4i16,
    AArch64::UZP1v8i8,
    AArch64::UZP1v4i16,
    AArch64::USHRv8i8_shift,
    AArch64::USHRv4i16_shift,
    AArch64::USHRv2i32_shift,
    AArch64::USHRd,
    AArch64::SMULLv8i8_v8i16,
    AArch64::SMULLv2i32_v2i64,
    AArch64::SMULLv4i16_v4i32,
    AArch64::SMULLv4i16_indexed,
    AArch64::SMULLv2i32_indexed,
    AArch64::USRAv8i8_shift,
    AArch64::USRAv4i16_shift,
    AArch64::USRAv2i32_shift,
    AArch64::USRAd,
    AArch64::SMINv8i8,
    AArch64::SMINv4i16,
    AArch64::SMINv2i32,
    AArch64::SMAXv8i8,
    AArch64::SMAXv4i16,
    AArch64::SMAXv2i32,
    AArch64::UMINv8i8,
    AArch64::UMINv4i16,
    AArch64::UMINv2i32,
    AArch64::UMAXv8i8,
    AArch64::UMAXv4i16,
    AArch64::UMAXv2i32,
    AArch64::SMINPv8i8,
    AArch64::SMINPv4i16,
    AArch64::SMINPv2i32,
    AArch64::SMAXPv8i8,
    AArch64::SMAXPv4i16,
    AArch64::SMAXPv2i32,
    AArch64::UMINPv8i8,
    AArch64::UMINPv4i16,
    AArch64::UMINPv2i32,
    AArch64::UMAXPv8i8,
    AArch64::UMAXPv4i16,
    AArch64::UMAXPv2i32,
    AArch64::UMULLv4i16_indexed,
    AArch64::UMULLv2i32_indexed,
    AArch64::UMULLv8i8_v8i16,
    AArch64::UMULLv4i16_v4i32,
    AArch64::CLZv2i32,
    AArch64::CLZv4i16,
    AArch64::CLZv8i8,
    AArch64::ABSv1i64,
    AArch64::ABSv8i8,
    AArch64::ABSv4i16,
    AArch64::ABSv2i32,
    AArch64::MVNIv2s_msl,
    AArch64::MVNIv2i32,
    AArch64::MVNIv4i16,
    AArch64::SMINVv8i8v,
    AArch64::UMINVv8i8v,
    AArch64::SMAXVv8i8v,
    AArch64::UMAXVv8i8v,
    AArch64::SMINVv4i16v,
    AArch64::UMINVv4i16v,
    AArch64::SMAXVv4i16v,
    AArch64::UMAXVv4i16v,
    AArch64::ORRv8i8,
    AArch64::ORRv2i32,
    AArch64::ORRv4i16,
    AArch64::REV64v2i32,
    AArch64::REV64v4i16,
    AArch64::REV64v8i8,
    AArch64::TRN1v4i16,
    AArch64::TRN1v8i8,
    AArch64::TRN2v8i8,
    AArch64::TRN2v4i16,
    AArch64::UQSUBv8i8,
    AArch64::UQSUBv4i16,
    AArch64::UQSUBv2i32,
    AArch64::TBLv8i8One,
    AArch64::TBLv8i8Two,
    AArch64::TBLv8i8Three,
    AArch64::TBLv8i8Four,
    AArch64::MOVIv2s_msl,
    AArch64::MOVIv8b_ns,
    AArch64::MOVIv4i16,
    AArch64::EXTv8i8,
    AArch64::MLAv2i32_indexed,
    AArch64::MLAv4i16_indexed,
    AArch64::MLAv8i8,
    AArch64::MLAv2i32,
    AArch64::MLAv4i16,
    AArch64::ORNv8i8,
    AArch64::CMTSTv4i16,
    AArch64::CMTSTv2i32,
    AArch64::CMEQv1i64,
    AArch64::CMGTv4i16rz,
    AArch64::CMEQv1i64rz,
    AArch64::CMGTv8i8rz,
    AArch64::CMLEv4i16rz,
    AArch64::CMGEv4i16rz,
    AArch64::CMLEv8i8rz,
    AArch64::CMLTv1i64rz,
    AArch64::CMGTv2i32rz,
    AArch64::CMGEv8i8rz,
    AArch64::CMLEv2i32rz,
    AArch64::CMGEv2i32rz,
    AArch64::UQADDv1i64,
    AArch64::UQADDv8i8,
    AArch64::UQADDv4i16,
    AArch64::UQADDv2i32,
    AArch64::REV16v8i8,
    AArch64::REV32v4i16,
    AArch64::REV32v8i8,
    AArch64::MLSv2i32_indexed,
    AArch64::MLSv4i16_indexed,
    AArch64::MLSv8i8,
    AArch64::MLSv4i16,
    AArch64::SQADDv1i64,
    AArch64::SQADDv8i8,
    AArch64::SQADDv4i16,
    AArch64::SQADDv2i32,
    AArch64::SQSUBv1i64,
    AArch64::SQSUBv8i8,
    AArch64::SQSUBv4i16,
    AArch64::SQSUBv2i32,
    AArch64::ADDv1i64,
    AArch64::URHADDv8i8,
    AArch64::URHADDv4i16,
    AArch64::URHADDv2i32,
    AArch64::UHADDv8i8,
    AArch64::UHADDv4i16,
    AArch64::UHADDv2i32,
    AArch64::SRHADDv8i8,
    AArch64::SRHADDv4i16,
    AArch64::SRHADDv2i32,
    AArch64::SHADDv8i8,
    AArch64::SHADDv4i16,
    AArch64::SHADDv2i32,
    AArch64::UHSUBv8i8,
    AArch64::UHSUBv4i16,
    AArch64::UHSUBv2i32,
    AArch64::SHSUBv8i8,
    AArch64::SHSUBv4i16,
    AArch64::SHSUBv2i32,
    AArch64::UADDLv8i8_v8i16,
    AArch64::UADDLv4i16_v4i32,
    AArch64::UADDLv2i32_v2i64,
    AArch64::UADDWv8i8_v8i16,
    AArch64::UADDWv4i16_v4i32,
    AArch64::UADDWv2i32_v2i64,
    AArch64::SADDLv8i8_v8i16,
    AArch64::SADDLv4i16_v4i32,
    AArch64::SADDLv2i32_v2i64,
    AArch64::SADDWv8i8_v8i16,
    AArch64::SADDWv4i16_v4i32,
    AArch64::SADDWv2i32_v2i64,
    AArch64::USUBLv8i8_v8i16,
    AArch64::USUBLv4i16_v4i32,
    AArch64::USUBLv2i32_v2i64,
    AArch64::USUBWv8i8_v8i16,
    AArch64::USUBWv4i16_v4i32,
    AArch64::USUBWv2i32_v2i64,
    AArch64::SSUBLv8i8_v8i16,
    AArch64::SSUBLv4i16_v4i32,
    AArch64::SSUBLv2i32_v2i64,
    AArch64::SSUBWv8i8_v8i16,
    AArch64::SSUBWv4i16_v4i32,
    AArch64::SSUBWv2i32_v2i64,
    AArch64::FNEGv2f32,
    AArch64::FNEGDr,
    AArch64::UCVTFUXSri,
    AArch64::UCVTFUXDri,
    AArch64::SCVTFUXSri,
    AArch64::SCVTFUXDri,
    AArch64::SCVTFv1i64,
    AArch64::FRINTXDr,
    AArch64::FRINTADr,
    AArch64::FRINTMDr,
    AArch64::FRINTPDr,
    AArch64::PRFMl,
    AArch64::PRFMroW,
    AArch64::PRFMroX,
    AArch64::PRFMui,
    AArch64::PRFUMi,
    AArch64::PACIASP,
    AArch64::PACIBSP,
    AArch64::AUTIASP,
    AArch64::AUTIBSP,
    AArch64::HINT,
    AArch64::FMINDrr,
    AArch64::FMINNMDrr,
    AArch64::FMAXDrr,
    AArch64::FMAXNMDrr,
    AArch64::FCMEQv2f32,
    AArch64::FCMGTv2f32,
    AArch64::FCMGEv2f32,
    AArch64::FCMEQv2i32rz,
    AArch64::FCMGTv2i32rz,
    AArch64::FCMLTv2i32rz,
    AArch64::FCMLEv2i32rz,
    AArch64::FCMGEv2i32rz,
    AArch64::FADDv2f32,
    AArch64::FSUBv2f32,
    AArch64::FMULv2f32,
    AArch64::FCCMPDrr,
};

const set<int> arm2llvm::instrs_128 = {
    AArch64::DUPv8i8lane,
    AArch64::DUPv4i16lane,
    AArch64::DUPv2i32lane,
    AArch64::RADDHNv2i64_v2i32,
    AArch64::RADDHNv2i64_v4i32,
    AArch64::RADDHNv4i32_v4i16,
    AArch64::RADDHNv4i32_v8i16,
    AArch64::RADDHNv8i16_v16i8,
    AArch64::RADDHNv8i16_v8i8,
    AArch64::ADDHNv2i64_v2i32,
    AArch64::ADDHNv2i64_v4i32,
    AArch64::ADDHNv4i32_v4i16,
    AArch64::ADDHNv4i32_v8i16,
    AArch64::ADDHNv8i16_v16i8,
    AArch64::ADDHNv8i16_v8i8,
    AArch64::RSUBHNv2i64_v2i32,
    AArch64::RSUBHNv2i64_v4i32,
    AArch64::RSUBHNv4i32_v4i16,
    AArch64::RSUBHNv4i32_v8i16,
    AArch64::RSUBHNv8i16_v16i8,
    AArch64::RSUBHNv8i16_v8i8,
    AArch64::SUBHNv2i64_v2i32,
    AArch64::SUBHNv2i64_v4i32,
    AArch64::SUBHNv4i32_v4i16,
    AArch64::SUBHNv4i32_v8i16,
    AArch64::SUBHNv8i16_v16i8,
    AArch64::SUBHNv8i16_v8i8,
    AArch64::UADDLv16i8_v8i16,
    AArch64::UADDLv8i16_v4i32,
    AArch64::UADDLv4i32_v2i64,
    AArch64::UADDWv16i8_v8i16,
    AArch64::UADDWv8i16_v4i32,
    AArch64::UADDWv4i32_v2i64,
    AArch64::SADDLv16i8_v8i16,
    AArch64::SADDLv8i16_v4i32,
    AArch64::SADDLv4i32_v2i64,
    AArch64::SADDWv16i8_v8i16,
    AArch64::SADDWv8i16_v4i32,
    AArch64::SADDWv4i32_v2i64,
    AArch64::USUBLv16i8_v8i16,
    AArch64::USUBLv8i16_v4i32,
    AArch64::USUBLv4i32_v2i64,
    AArch64::USUBWv16i8_v8i16,
    AArch64::USUBWv8i16_v4i32,
    AArch64::USUBWv4i32_v2i64,
    AArch64::SSUBLv16i8_v8i16,
    AArch64::SSUBLv8i16_v4i32,
    AArch64::SSUBLv4i32_v2i64,
    AArch64::SSUBWv16i8_v8i16,
    AArch64::SSUBWv8i16_v4i32,
    AArch64::SSUBWv4i32_v2i64,
    AArch64::UMLALv4i16_indexed,
    AArch64::UMLALv8i16_indexed,
    AArch64::UMLALv2i32_indexed,
    AArch64::UMLALv4i32_indexed,
    AArch64::UMLALv8i8_v8i16,
    AArch64::UMLALv4i16_v4i32,
    AArch64::UMLALv2i32_v2i64,
    AArch64::UMLALv16i8_v8i16,
    AArch64::UMLALv8i16_v4i32,
    AArch64::UMLALv4i32_v2i64,
    AArch64::UMLSLv4i16_indexed,
    AArch64::UMLSLv8i16_indexed,
    AArch64::UMLSLv2i32_indexed,
    AArch64::UMLSLv4i32_indexed,
    AArch64::UMLSLv8i8_v8i16,
    AArch64::UMLSLv4i16_v4i32,
    AArch64::UMLSLv2i32_v2i64,
    AArch64::UMLSLv16i8_v8i16,
    AArch64::UMLSLv8i16_v4i32,
    AArch64::UMLSLv4i32_v2i64,
    AArch64::SMLALv4i16_indexed,
    AArch64::SMLALv8i16_indexed,
    AArch64::SMLALv2i32_indexed,
    AArch64::SMLALv4i32_indexed,
    AArch64::SMLALv8i8_v8i16,
    AArch64::SMLALv4i16_v4i32,
    AArch64::SMLALv2i32_v2i64,
    AArch64::SMLALv16i8_v8i16,
    AArch64::SMLALv8i16_v4i32,
    AArch64::SMLALv4i32_v2i64,
    AArch64::SMLSLv4i16_indexed,
    AArch64::SMLSLv8i16_indexed,
    AArch64::SMLSLv2i32_indexed,
    AArch64::SMLSLv4i32_indexed,
    AArch64::SMLSLv8i8_v8i16,
    AArch64::SMLSLv4i16_v4i32,
    AArch64::SMLSLv2i32_v2i64,
    AArch64::SMLSLv16i8_v8i16,
    AArch64::SMLSLv8i16_v4i32,
    AArch64::SMLSLv4i32_v2i64,
    AArch64::SQADDv2i64,
    AArch64::SQADDv4i32,
    AArch64::SQADDv16i8,
    AArch64::SQADDv8i16,
    AArch64::SQSUBv2i64,
    AArch64::SQSUBv4i32,
    AArch64::SQSUBv16i8,
    AArch64::SQSUBv8i16,
    AArch64::MLSv16i8,
    AArch64::MLSv8i16,
    AArch64::MLSv4i32,
    AArch64::MLSv4i32_indexed,
    AArch64::MLSv8i16_indexed,
    AArch64::REV16v16i8,
    AArch64::REV32v8i16,
    AArch64::REV32v16i8,
    AArch64::UQADDv2i64,
    AArch64::UQADDv4i32,
    AArch64::UQADDv16i8,
    AArch64::UQADDv8i16,
    AArch64::CMLEv8i16rz,
    AArch64::CMLEv16i8rz,
    AArch64::CMGTv16i8rz,
    AArch64::CMGTv8i16rz,
    AArch64::CMGTv2i64rz,
    AArch64::CMGEv8i16rz,
    AArch64::CMLEv4i32rz,
    AArch64::CMGEv2i64rz,
    AArch64::CMLEv2i64rz,
    AArch64::CMGEv16i8rz,
    AArch64::CMGEv4i32rz,
    AArch64::ORNv16i8,
    AArch64::MLAv8i16_indexed,
    AArch64::MLAv4i32_indexed,
    AArch64::MLAv16i8,
    AArch64::MLAv8i16,
    AArch64::MLAv4i32,
    AArch64::SHRNv8i8_shift,
    AArch64::SHRNv16i8_shift,
    AArch64::SHRNv4i16_shift,
    AArch64::SHRNv8i16_shift,
    AArch64::SHRNv2i32_shift,
    AArch64::SHRNv4i32_shift,
    AArch64::RSHRNv8i8_shift,
    AArch64::RSHRNv16i8_shift,
    AArch64::RSHRNv4i16_shift,
    AArch64::RSHRNv8i16_shift,
    AArch64::RSHRNv2i32_shift,
    AArch64::RSHRNv4i32_shift,
    AArch64::MOVIv4s_msl,
    AArch64::TBLv16i8One,
    AArch64::TBLv16i8Two,
    AArch64::TBLv16i8Three,
    AArch64::TBLv16i8Four,
    AArch64::UQSUBv2i64,
    AArch64::UQSUBv4i32,
    AArch64::UQSUBv16i8,
    AArch64::UQSUBv8i16,
    AArch64::TRN1v16i8,
    AArch64::TRN1v8i16,
    AArch64::TRN1v4i32,
    AArch64::TRN2v16i8,
    AArch64::TRN2v8i16,
    AArch64::TRN2v4i32,
    AArch64::REV64v8i16,
    AArch64::REV64v16i8,
    AArch64::ORRv16i8,
    AArch64::ORRv8i16,
    AArch64::ORRv4i32,
    AArch64::SMAXVv4i32v,
    AArch64::UMAXVv4i32v,
    AArch64::SMINVv4i32v,
    AArch64::UMINVv4i32v,
    AArch64::UMINVv8i16v,
    AArch64::SMINVv8i16v,
    AArch64::UMAXVv8i16v,
    AArch64::SMAXVv8i16v,
    AArch64::SMINVv16i8v,
    AArch64::UMINVv16i8v,
    AArch64::SMAXVv16i8v,
    AArch64::UMAXVv16i8v,
    AArch64::MVNIv4s_msl,
    AArch64::MVNIv8i16,
    AArch64::MVNIv4i32,
    AArch64::ABSv2i64,
    AArch64::ABSv16i8,
    AArch64::ABSv8i16,
    AArch64::ABSv4i32,
    AArch64::CLZv16i8,
    AArch64::CLZv8i16,
    AArch64::CLZv4i32,
    AArch64::UMULLv16i8_v8i16,
    AArch64::UMULLv8i16_v4i32,
    AArch64::UMULLv4i32_v2i64,
    AArch64::UMULLv8i16_indexed,
    AArch64::UMULLv4i32_indexed,
    AArch64::SMINv16i8,
    AArch64::SMINv8i16,
    AArch64::SMINv4i32,
    AArch64::SMAXv16i8,
    AArch64::SMAXv8i16,
    AArch64::SMAXv4i32,
    AArch64::UMINv16i8,
    AArch64::UMINv8i16,
    AArch64::UMINv4i32,
    AArch64::UMAXv16i8,
    AArch64::UMAXv8i16,
    AArch64::UMAXv4i32,
    AArch64::SMINPv16i8,
    AArch64::SMINPv8i16,
    AArch64::SMINPv4i32,
    AArch64::SMAXPv16i8,
    AArch64::SMAXPv8i16,
    AArch64::SMAXPv4i32,
    AArch64::UMINPv16i8,
    AArch64::UMINPv8i16,
    AArch64::UMINPv4i32,
    AArch64::UMAXPv16i8,
    AArch64::UMAXPv8i16,
    AArch64::UMAXPv4i32,
    AArch64::USRAv16i8_shift,
    AArch64::USRAv8i16_shift,
    AArch64::USRAv4i32_shift,
    AArch64::USRAv2i64_shift,
    AArch64::SMULLv8i16_v4i32,
    AArch64::SMULLv16i8_v8i16,
    AArch64::SMULLv4i32_v2i64,
    AArch64::SMULLv4i32_indexed,
    AArch64::SMULLv8i16_indexed,
    AArch64::USHRv16i8_shift,
    AArch64::USHRv8i16_shift,
    AArch64::USHRv4i32_shift,
    AArch64::UZP2v8i16,
    AArch64::UZP2v16i8,
    AArch64::UZP1v16i8,
    AArch64::UZP2v4i32,
    AArch64::UZP1v8i16,
    AArch64::UZP1v4i32,
    AArch64::MULv8i16_indexed,
    AArch64::MULv4i32_indexed,
    AArch64::MULv16i8,
    AArch64::MULv8i16,
    AArch64::MULv4i32,
    AArch64::RBITv16i8,
    AArch64::BITv16i8,
    AArch64::ZIP2v8i16,
    AArch64::ZIP2v2i64,
    AArch64::ZIP2v16i8,
    AArch64::ZIP2v4i32,
    AArch64::ZIP1v8i16,
    AArch64::ZIP1v16i8,
    AArch64::ZIP1v2i64,
    AArch64::ZIP1v4i32,
    AArch64::SSHLLv4i32_shift,
    AArch64::SSHLLv8i16_shift,
    AArch64::SSHLLv16i8_shift,
    AArch64::ADDPv16i8,
    AArch64::ADDPv4i32,
    AArch64::ADDPv8i16,
    AArch64::ADDPv2i64,
    AArch64::ADDPv2i64p,
    AArch64::SSHRv16i8_shift,
    AArch64::SSHRv2i64_shift,
    AArch64::SSHRv8i16_shift,
    AArch64::SSHRv4i32_shift,
    AArch64::SSRAv16i8_shift,
    AArch64::SSRAv8i16_shift,
    AArch64::SSRAv4i32_shift,
    AArch64::SSRAd,
    AArch64::SSRAv2i64_shift,
    AArch64::SHLv16i8_shift,
    AArch64::SHLv8i16_shift,
    AArch64::SHLv4i32_shift,
    AArch64::SHLv2i64_shift,
    AArch64::USHLLv8i16_shift,
    AArch64::USHLLv16i8_shift,
    AArch64::USHLLv4i32_shift,
    AArch64::SLIv16i8_shift,
    AArch64::SLIv8i16_shift,
    AArch64::SLIv4i32_shift,
    AArch64::SLIv2i64_shift,
    AArch64::SRIv16i8_shift,
    AArch64::SRIv8i16_shift,
    AArch64::SRIv4i32_shift,
    AArch64::SRIv2i64_shift,
    AArch64::DUPi8,
    AArch64::DUPi16,
    AArch64::DUPi64,
    AArch64::DUPi32,
    AArch64::FMOVXDr,
    AArch64::LDPQi,
    AArch64::LDPQpre,
    AArch64::LDPQpost,
    AArch64::LDRQroX,
    AArch64::LDURQi,
    AArch64::LD1i8,
    AArch64::LD1i16,
    AArch64::LD1i32,
    AArch64::LD1i64,
    AArch64::LD1i8_POST,
    AArch64::LD1i16_POST,
    AArch64::LD1i32_POST,
    AArch64::LD1i64_POST,
    AArch64::LD1Rv8b,
    AArch64::LD1Rv16b,
    AArch64::LD1Rv4h,
    AArch64::LD1Rv8h,
    AArch64::LD1Rv2s,
    AArch64::LD1Rv4s,
    AArch64::LD1Rv1d,
    AArch64::LD1Rv2d,
    AArch64::LD1Rv8b_POST,
    AArch64::LD1Rv16b_POST,
    AArch64::LD1Rv4h_POST,
    AArch64::LD1Rv8h_POST,
    AArch64::LD1Rv2s_POST,
    AArch64::LD1Rv4s_POST,
    AArch64::LD1Rv1d_POST,
    AArch64::LD1Rv2d_POST,
    AArch64::LD1Onev8b,
    AArch64::LD1Onev16b,
    AArch64::LD1Onev4h,
    AArch64::LD1Onev8h,
    AArch64::LD1Onev2s,
    AArch64::LD1Onev4s,
    AArch64::LD1Onev1d,
    AArch64::LD1Onev2d,
    AArch64::LD1Onev8b_POST,
    AArch64::LD1Onev16b_POST,
    AArch64::LD1Onev4h_POST,
    AArch64::LD1Onev8h_POST,
    AArch64::LD1Onev2s_POST,
    AArch64::LD1Onev4s_POST,
    AArch64::LD1Onev1d_POST,
    AArch64::LD1Onev2d_POST,
    AArch64::LD1Twov8b,
    AArch64::LD1Twov16b,
    AArch64::LD1Twov4h,
    AArch64::LD1Twov8h,
    AArch64::LD1Twov2s,
    AArch64::LD1Twov4s,
    AArch64::LD1Twov1d,
    AArch64::LD1Twov2d,
    AArch64::LD1Twov8b_POST,
    AArch64::LD1Twov16b_POST,
    AArch64::LD1Twov4h_POST,
    AArch64::LD1Twov8h_POST,
    AArch64::LD1Twov2s_POST,
    AArch64::LD1Twov4s_POST,
    AArch64::LD1Twov1d_POST,
    AArch64::LD1Twov2d_POST,
    AArch64::LD1Threev8b,
    AArch64::LD1Threev16b,
    AArch64::LD1Threev4h,
    AArch64::LD1Threev8h,
    AArch64::LD1Threev2s,
    AArch64::LD1Threev4s,
    AArch64::LD1Threev1d,
    AArch64::LD1Threev2d,
    AArch64::LD1Threev8b_POST,
    AArch64::LD1Threev16b_POST,
    AArch64::LD1Threev4h_POST,
    AArch64::LD1Threev8h_POST,
    AArch64::LD1Threev2s_POST,
    AArch64::LD1Threev4s_POST,
    AArch64::LD1Threev1d_POST,
    AArch64::LD1Threev2d_POST,
    AArch64::LD1Fourv8b,
    AArch64::LD1Fourv16b,
    AArch64::LD1Fourv4h,
    AArch64::LD1Fourv8h,
    AArch64::LD1Fourv2s,
    AArch64::LD1Fourv4s,
    AArch64::LD1Fourv1d,
    AArch64::LD1Fourv2d,
    AArch64::LD1Fourv8b_POST,
    AArch64::LD1Fourv16b_POST,
    AArch64::LD1Fourv4h_POST,
    AArch64::LD1Fourv8h_POST,
    AArch64::LD1Fourv2s_POST,
    AArch64::LD1Fourv4s_POST,
    AArch64::LD1Fourv1d_POST,
    AArch64::LD1Fourv2d_POST,
    AArch64::LD2Twov8b,
    AArch64::LD2Twov16b,
    AArch64::LD2Twov4h,
    AArch64::LD2Twov8h,
    AArch64::LD2Twov2s,
    AArch64::LD2Twov4s,
    AArch64::LD2Twov2d,
    AArch64::LD2Twov8b_POST,
    AArch64::LD2Twov16b_POST,
    AArch64::LD2Twov4h_POST,
    AArch64::LD2Twov8h_POST,
    AArch64::LD2Twov2s_POST,
    AArch64::LD2Twov4s_POST,
    AArch64::LD2Twov2d_POST,
    AArch64::LD3Threev8b,
    AArch64::LD3Threev16b,
    AArch64::LD3Threev4h,
    AArch64::LD3Threev8h,
    AArch64::LD3Threev2s,
    AArch64::LD3Threev4s,
    AArch64::LD3Threev2d,
    AArch64::LD3Threev8b_POST,
    AArch64::LD3Threev16b_POST,
    AArch64::LD3Threev4h_POST,
    AArch64::LD3Threev8h_POST,
    AArch64::LD3Threev2s_POST,
    AArch64::LD3Threev4s_POST,
    AArch64::LD3Threev2d_POST,
    AArch64::LD4Fourv8b,
    AArch64::LD4Fourv16b,
    AArch64::LD4Fourv4h,
    AArch64::LD4Fourv8h,
    AArch64::LD4Fourv2s,
    AArch64::LD4Fourv4s,
    AArch64::LD4Fourv2d,
    AArch64::LD4Fourv8b_POST,
    AArch64::LD4Fourv16b_POST,
    AArch64::LD4Fourv4h_POST,
    AArch64::LD4Fourv8h_POST,
    AArch64::LD4Fourv2s_POST,
    AArch64::LD4Fourv4s_POST,
    AArch64::LD4Fourv2d_POST,
    AArch64::STPQi,
    AArch64::STPQpre,
    AArch64::STPQpost,
    AArch64::STRQroX,
    AArch64::ST1i8,
    AArch64::ST1i16,
    AArch64::ST1i32,
    AArch64::ST1i64,
    AArch64::ST1i8_POST,
    AArch64::ST1i16_POST,
    AArch64::ST1i32_POST,
    AArch64::ST1i64_POST,
    AArch64::ST1Onev8b,
    AArch64::ST1Onev16b,
    AArch64::ST1Onev4h,
    AArch64::ST1Onev8h,
    AArch64::ST1Onev2s,
    AArch64::ST1Onev4s,
    AArch64::ST1Onev1d,
    AArch64::ST1Onev2d,
    AArch64::ST1Onev8b_POST,
    AArch64::ST1Onev16b_POST,
    AArch64::ST1Onev4h_POST,
    AArch64::ST1Onev8h_POST,
    AArch64::ST1Onev2s_POST,
    AArch64::ST1Onev4s_POST,
    AArch64::ST1Onev1d_POST,
    AArch64::ST1Onev2d_POST,
    AArch64::ST1Twov8b,
    AArch64::ST1Twov16b,
    AArch64::ST1Twov4h,
    AArch64::ST1Twov8h,
    AArch64::ST1Twov2s,
    AArch64::ST1Twov4s,
    AArch64::ST1Twov1d,
    AArch64::ST1Twov2d,
    AArch64::ST1Twov8b_POST,
    AArch64::ST1Twov16b_POST,
    AArch64::ST1Twov4h_POST,
    AArch64::ST1Twov8h_POST,
    AArch64::ST1Twov2s_POST,
    AArch64::ST1Twov4s_POST,
    AArch64::ST1Twov1d_POST,
    AArch64::ST1Twov2d_POST,
    AArch64::ST1Threev8b,
    AArch64::ST1Threev16b,
    AArch64::ST1Threev4h,
    AArch64::ST1Threev8h,
    AArch64::ST1Threev2s,
    AArch64::ST1Threev4s,
    AArch64::ST1Threev1d,
    AArch64::ST1Threev2d,
    AArch64::ST1Threev8b_POST,
    AArch64::ST1Threev16b_POST,
    AArch64::ST1Threev4h_POST,
    AArch64::ST1Threev8h_POST,
    AArch64::ST1Threev2s_POST,
    AArch64::ST1Threev4s_POST,
    AArch64::ST1Threev1d_POST,
    AArch64::ST1Threev2d_POST,
    AArch64::ST1Fourv8b,
    AArch64::ST1Fourv16b,
    AArch64::ST1Fourv4h,
    AArch64::ST1Fourv8h,
    AArch64::ST1Fourv2s,
    AArch64::ST1Fourv4s,
    AArch64::ST1Fourv1d,
    AArch64::ST1Fourv2d,
    AArch64::ST1Fourv8b_POST,
    AArch64::ST1Fourv16b_POST,
    AArch64::ST1Fourv4h_POST,
    AArch64::ST1Fourv8h_POST,
    AArch64::ST1Fourv2s_POST,
    AArch64::ST1Fourv4s_POST,
    AArch64::ST1Fourv1d_POST,
    AArch64::ST1Fourv2d_POST,
    AArch64::ST2Twov8b,
    AArch64::ST2Twov16b,
    AArch64::ST2Twov4h,
    AArch64::ST2Twov8h,
    AArch64::ST2Twov2s,
    AArch64::ST2Twov4s,
    AArch64::ST2Twov2d,
    AArch64::ST2Twov8b_POST,
    AArch64::ST2Twov16b_POST,
    AArch64::ST2Twov4h_POST,
    AArch64::ST2Twov8h_POST,
    AArch64::ST2Twov2s_POST,
    AArch64::ST2Twov4s_POST,
    AArch64::ST2Twov2d_POST,
    AArch64::ST3Threev8b,
    AArch64::ST3Threev16b,
    AArch64::ST3Threev4h,
    AArch64::ST3Threev8h,
    AArch64::ST3Threev2s,
    AArch64::ST3Threev4s,
    AArch64::ST3Threev2d,
    AArch64::ST3Threev8b_POST,
    AArch64::ST3Threev16b_POST,
    AArch64::ST3Threev4h_POST,
    AArch64::ST3Threev8h_POST,
    AArch64::ST3Threev2s_POST,
    AArch64::ST3Threev4s_POST,
    AArch64::ST3Threev2d_POST,
    AArch64::ST4Fourv8b,
    AArch64::ST4Fourv16b,
    AArch64::ST4Fourv4h,
    AArch64::ST4Fourv8h,
    AArch64::ST4Fourv2s,
    AArch64::ST4Fourv4s,
    AArch64::ST4Fourv2d,
    AArch64::ST4Fourv8b_POST,
    AArch64::ST4Fourv16b_POST,
    AArch64::ST4Fourv4h_POST,
    AArch64::ST4Fourv8h_POST,
    AArch64::ST4Fourv2s_POST,
    AArch64::ST4Fourv4s_POST,
    AArch64::ST4Fourv2d_POST,
    AArch64::ADDv8i16,
    AArch64::ADDv2i64,
    AArch64::ADDv4i32,
    AArch64::ADDv16i8,
    AArch64::SUBv8i16,
    AArch64::SUBv2i64,
    AArch64::SUBv4i32,
    AArch64::SUBv16i8,
    AArch64::UABAv16i8,
    AArch64::UABAv8i16,
    AArch64::UABAv4i32,
    AArch64::UABDv16i8,
    AArch64::UABDv8i16,
    AArch64::UABDv4i32,
    AArch64::SABAv16i8,
    AArch64::SABAv8i16,
    AArch64::SABAv4i32,
    AArch64::SABDv16i8,
    AArch64::SABDv8i16,
    AArch64::SABDv4i32,
    AArch64::LDRQui,
    AArch64::LDRQpre,
    AArch64::LDRQpost,
    AArch64::STURQi,
    AArch64::STRQui,
    AArch64::STRQpre,
    AArch64::STRQpost,
    AArch64::FMOVDi,
    AArch64::FMOVSi,
    AArch64::FMOVWSr,
    AArch64::FMOVv4f32_ns,
    AArch64::FMOVv2f64_ns,
    AArch64::CNTv16i8,
    AArch64::MOVIv2d_ns,
    AArch64::MOVIv4i32,
    AArch64::EXTv16i8,
    AArch64::MOVIv2i32,
    AArch64::ANDv16i8,
    AArch64::EORv16i8,
    AArch64::UMOVvi32,
    AArch64::UMOVvi8,
    AArch64::UMOVvi8_idx0,
    AArch64::UMOVvi32_idx0,
    AArch64::MOVIv16b_ns,
    AArch64::UMOVvi64,
    AArch64::UMOVvi16,
    AArch64::UMOVvi16_idx0,
    AArch64::SMOVvi8to32_idx0,
    AArch64::SMOVvi8to32,
    AArch64::SMOVvi16to32_idx0,
    AArch64::SMOVvi16to32,
    AArch64::SMOVvi8to64_idx0,
    AArch64::SMOVvi8to64,
    AArch64::SMOVvi16to64_idx0,
    AArch64::SMOVvi16to64,
    AArch64::SMOVvi32to64_idx0,
    AArch64::SMOVvi32to64,
    AArch64::INSvi64lane,
    AArch64::INSvi8lane,
    AArch64::INSvi16lane,
    AArch64::INSvi32lane,
    AArch64::DUPv16i8gpr,
    AArch64::DUPv8i16gpr,
    AArch64::DUPv4i32gpr,
    AArch64::DUPv2i64gpr,
    AArch64::DUPv16i8lane,
    AArch64::DUPv8i16lane,
    AArch64::DUPv4i32lane,
    AArch64::DUPv2i64lane,
    AArch64::MOVIv8i16,
    AArch64::REV64v4i32,
    AArch64::USHRv2i64_shift,
    AArch64::NOTv16i8,
    AArch64::NEGv16i8,
    AArch64::NEGv8i16,
    AArch64::NEGv2i64,
    AArch64::NEGv4i32,
    AArch64::USHLv16i8,
    AArch64::USHLv8i16,
    AArch64::USHLv4i32,
    AArch64::USHLv2i64,
    AArch64::SHLLv16i8,
    AArch64::SHLLv8i16,
    AArch64::SHLLv4i32,
    AArch64::SSHLv16i8,
    AArch64::SSHLv8i16,
    AArch64::SSHLv4i32,
    AArch64::SSHLv2i64,
    AArch64::URHADDv16i8,
    AArch64::URHADDv8i16,
    AArch64::URHADDv4i32,
    AArch64::UHADDv16i8,
    AArch64::UHADDv8i16,
    AArch64::UHADDv4i32,
    AArch64::SRHADDv16i8,
    AArch64::SRHADDv8i16,
    AArch64::SRHADDv4i32,
    AArch64::SHADDv16i8,
    AArch64::SHADDv8i16,
    AArch64::SHADDv4i32,
    AArch64::UHSUBv16i8,
    AArch64::UHSUBv8i16,
    AArch64::UHSUBv4i32,
    AArch64::SHSUBv16i8,
    AArch64::SHSUBv8i16,
    AArch64::SHSUBv4i32,
    AArch64::UADDLVv8i16v,
    AArch64::UADDLVv4i32v,
    AArch64::UADDLVv16i8v,
    AArch64::SADDLVv8i16v,
    AArch64::SADDLVv4i32v,
    AArch64::SADDLVv16i8v,
    AArch64::UADDLPv8i16_v4i32,
    AArch64::UADDLPv4i32_v2i64,
    AArch64::UADDLPv16i8_v8i16,
    AArch64::SADDLPv8i16_v4i32,
    AArch64::SADDLPv4i32_v2i64,
    AArch64::SADDLPv16i8_v8i16,
    AArch64::UABALv16i8_v8i16,
    AArch64::UABALv8i16_v4i32,
    AArch64::UABALv4i32_v2i64,
    AArch64::UABDLv16i8_v8i16,
    AArch64::UABDLv8i16_v4i32,
    AArch64::UABDLv4i32_v2i64,
    AArch64::SABALv16i8_v8i16,
    AArch64::SABALv8i16_v4i32,
    AArch64::SABALv4i32_v2i64,
    AArch64::SABDLv16i8_v8i16,
    AArch64::SABDLv8i16_v4i32,
    AArch64::SABDLv4i32_v2i64,
    AArch64::UADALPv4i32_v2i64,
    AArch64::UADALPv16i8_v8i16,
    AArch64::UADALPv8i16_v4i32,
    AArch64::SADALPv4i32_v2i64,
    AArch64::SADALPv16i8_v8i16,
    AArch64::SADALPv8i16_v4i32,
    AArch64::BIFv16i8,
    AArch64::BSLv16i8,
    AArch64::BICv8i16,
    AArch64::BICv4i32,
    AArch64::BICv16i8,
    AArch64::ADDVv16i8v,
    AArch64::ADDVv8i16v,
    AArch64::ADDVv4i32v,
    AArch64::CMHIv16i8,
    AArch64::CMHIv8i16,
    AArch64::CMHIv4i32,
    AArch64::CMHIv2i64,
    AArch64::CMEQv16i8,
    AArch64::CMEQv16i8rz,
    AArch64::CMEQv2i64,
    AArch64::CMEQv2i64rz,
    AArch64::CMEQv4i32,
    AArch64::CMEQv4i32rz,
    AArch64::CMEQv8i16,
    AArch64::CMEQv8i16rz,
    AArch64::CMGEv16i8,
    AArch64::CMGEv2i64,
    AArch64::CMGEv4i32,
    AArch64::CMGEv8i16,
    AArch64::CMGTv16i8,
    AArch64::CMGTv2i64,
    AArch64::CMGTv4i32,
    AArch64::CMGTv4i32rz,
    AArch64::CMGTv8i16,
    AArch64::CMHSv16i8,
    AArch64::CMHSv2i64,
    AArch64::CMHSv4i32,
    AArch64::CMHSv8i16,
    AArch64::CMLTv16i8rz,
    AArch64::CMLTv2i64rz,
    AArch64::CMLTv4i32rz,
    AArch64::CMLTv8i16rz,
    AArch64::CMTSTv16i8,
    AArch64::CMTSTv2i64,
    AArch64::CMTSTv4i32,
    AArch64::CMTSTv8i16,
    AArch64::CMHIv16i8,
    AArch64::CMHIv8i16,
    AArch64::CMHIv4i32,
    AArch64::CMHIv2i64,
    AArch64::XTNv16i8,
    AArch64::XTNv8i16,
    AArch64::XTNv4i32,
    AArch64::UQXTNv16i8,
    AArch64::UQXTNv8i16,
    AArch64::UQXTNv4i32,
    AArch64::SQXTNv16i8,
    AArch64::SQXTNv8i16,
    AArch64::SQXTNv4i32,
    AArch64::FNEGv4f32,
    AArch64::FNEGv2f64,
    AArch64::FCMEQv4f32,
    AArch64::FCMEQv2f64,
    AArch64::FCMGTv4f32,
    AArch64::FCMGTv2f64,
    AArch64::FCMGEv4f32,
    AArch64::FCMGEv2f64,
    AArch64::FCMLEv2i64rz,
    AArch64::FCMLEv4i32rz,
    AArch64::FCMGEv4i32rz,
    AArch64::FCMGEv2i64rz,
    AArch64::FCMLTv2i64rz,
    AArch64::FCMLTv4i32rz,
    AArch64::FCMGTv4i32rz,
    AArch64::FCMGTv2i64rz,
    AArch64::FCMEQv2i64rz,
    AArch64::FCMEQv4i32rz,
    AArch64::FADDv4f32,
    AArch64::FADDv2f64,
    AArch64::FSUBv4f32,
    AArch64::FSUBv2f64,
    AArch64::FMULv4f32,
    AArch64::FMULv2f64,
};

bool arm2llvm::has_s(int instr) {
  return s_flag.contains(instr);
}

// decodeBitMasks - Decode a logical immediate value in the form
// "N:immr:imms" (where the immr and imms fields are each 6 bits) into the
// integer value it represents with regSize bits. Implementation of the
// DecodeBitMasks function from the ARMv8 manual.
//
// WARNING: tmask is untested
pair<uint64_t, uint64_t> arm2llvm::decodeBitMasks(uint64_t val,
                                                  unsigned regSize) {
  // Extract the N, imms, and immr fields.
  unsigned N = (val >> 12) & 1;
  unsigned immr = (val >> 6) & 0x3f;
  unsigned imms = val & 0x3f;

  assert((regSize == 64 || N == 0) && "undefined logical immediate encoding");
  int len = 31 - llvm::countl_zero((N << 6) | (~imms & 0x3f));
  assert(len >= 0 && len <= 6 && "undefined logical immediate encoding");
  unsigned size = (uint64_t)1 << len;
  unsigned R = immr & (size - 1);
  unsigned S = imms & (size - 1);
  unsigned d = ((S - R) & (size - 1));
  assert(S != size - 1 && d <= size - 1 &&
         "undefined logical immediate encoding");
  uint64_t wmask = (1ULL << (S + 1)) - 1;
  uint64_t tmask = (1ULL << min(d + 1, (unsigned)63)) - 1;
  // Rotate wmask right R times to get wmask
  for (unsigned i = 0; i < R; ++i)
    wmask = ((wmask & 1) << (size - 1)) | (wmask >> 1);

  // Replicate the wmask to fill the regSize.
  while (size != regSize) {
    wmask |= (wmask << size);
    tmask |= (tmask << size);
    size *= 2;
  }

  return make_pair(wmask, tmask);
}

unsigned arm2llvm::getInstSize(int instr) {
  if (instrs_32.contains(instr))
    return 32;
  if (instrs_64.contains(instr))
    return 64;
  if (instrs_128.contains(instr))
    return 128;
  *out << "getInstSize encountered unknown instruction\n";
  visitError();
}

// from getShiftType/getShiftValue:
// https://github.com/llvm/llvm-project/blob/93d1a623cecb6f732db7900baf230a13e6ac6c6a/llvm/lib/Target/AArch64/MCTargetDesc/AArch64AddressingModes.h#L74
Value *arm2llvm::regShift(Value *value, int encodedShift) {
  if (encodedShift == 0)
    return value;

  int shift_type = (encodedShift >> 6) & 0x7;
  auto W = getBitWidth(value);
  auto exp = getUnsignedIntConst(encodedShift & 0x3f, W);

  switch (shift_type) {
  case 0:
    return createMaskedShl(value, exp);
  case 1:
    return createMaskedLShr(value, exp);
  case 2:
    return createMaskedAShr(value, exp);
  case 3:
    // ROR shift
    return createFShr(value, value, exp);
  default:
    // FIXME: handle other case (msl)
    *out << "\nERROR: shift type not supported\n\n";
    exit(-1);
  }
}

llvm::AllocaInst *arm2llvm::get_reg(aslp::reg_t regtype, uint64_t num) {
  using reg_t = aslp::reg_t;
  using pstate_t = aslp::pstate_t;

  uint64_t reg = 0;
  if (regtype == reg_t::X) {
    if (num <= 28)
      reg = llvm::AArch64::X0 + num;
    else if (num == 29)
      reg = llvm::AArch64::FP;
    else if (num == 30)
      reg = llvm::AArch64::LR;
    else if (num == 31)
      reg = llvm::AArch64::SP;
    else
      assert(false && "X register out of range");

  } else if (regtype == reg_t::PSTATE) {

    if (num == (int)pstate_t::N)
      reg = llvm::AArch64::N;
    else if (num == (int)pstate_t::Z)
      reg = llvm::AArch64::Z;
    else if (num == (int)pstate_t::C)
      reg = llvm::AArch64::C;
    else if (num == (int)pstate_t::V)
      reg = llvm::AArch64::V;

  } else if (regtype == reg_t::V) {
    reg = llvm::AArch64::Q0 + num;
  }

  assert(reg && "register not mapped");
  return llvm::cast<llvm::AllocaInst>(RegFile.at(reg));
}

void arm2llvm::lift(MCInst &I) {
  auto entrybb = LLVMBB;
  aslp::bridge bridge{*this, MCE, STI, IA};
  auto opcode = I.getOpcode();

  StringRef instStr = InstPrinter->getOpcodeName(I.getOpcode());
  if (auto a64Opcode = getArmOpcode(I)) {
    auto aslpResult = bridge.run(I, a64Opcode.value());

    if (auto result = std::get_if<aslp::result_t>(&aslpResult)) {

      // branch lifter's entry BB to entry BB in ASLP result,
      // then set ASLP's exit BB to be the next BB.
      LLVMBB = entrybb;
      auto [encoding, stmts] = *result;
      this->createBranch(stmts.first);
      LLVMBB = stmts.second;
      stmts.first->begin()->setMetadata(
          "asm.aslp",
          llvm::MDTuple::get(Ctx, {llvm::MDString::get(Ctx, instStr)}));

      *out << "... lifted via aslp: " << encoding << " - " << instStr.str()
           << std::endl;
      encodingCounts[encoding]++;
      return;

    } else {
      switch (std::get<aslp::err_t>(aslpResult)) {
      case aslp::err_t::missing:
        *out << "... aslp missing! "
             << std::format("0x{:08x}", aslp::get_opnum(a64Opcode.value()))
             << "  " << aslp::format_opcode(a64Opcode.value()) << std::endl;
        if (aslp::bridge::config().fail_if_missing) {
          throw std::runtime_error(
              "missing aslp instruction in debug mode is not allowed!");
        }
        break;
      case aslp::err_t::banned:
        *out << "... aslp banned\n";
        break; // continue with classic.
      }
    }
  } else {
    *out << "... arm opnum failed: "
         << InstPrinter->getOpcodeName(I.getOpcode()).str() << '\n';
    // arm opcode translation failed, possibly SEH_NOP. continue with classic.
  }

  std::string encoding{"classic_" + aslp::aarch64_revmap().at(opcode)};
  encodingCounts[encoding]++;

  // always create new bb per instruction, to match aslp
  auto newbb = BasicBlock::Create(Ctx, "lifter_" + nextName(), liftedFn);
  createBranch(newbb);
  LLVMBB->getTerminator()->setMetadata(
      "asm.classic",
      llvm::MDTuple::get(Ctx, {llvm::MDString::get(Ctx, instStr)}));

  LLVMBB = newbb;

  auto i1 = getIntTy(1);
  auto i8 = getIntTy(8);
  auto i16 = getIntTy(16);
  auto i32 = getIntTy(32);
  auto i64 = getIntTy(64);
  auto i128 = getIntTy(128);

  switch (opcode) {

  // nops
  case AArch64::PRFMl:
  case AArch64::PRFMroW:
  case AArch64::PRFMroX:
  case AArch64::PRFMui:
  case AArch64::PRFUMi:
  case AArch64::PACIASP:
  case AArch64::PACIBSP:
  case AArch64::AUTIASP:
  case AArch64::AUTIBSP:
  case AArch64::HINT:
    break;

  // we're abusing this opcode -- better hope we don't run across these for
  // real
  case AArch64::SEH_Nop:
    break;

  case AArch64::BRK:
    createTrap();
    break;

  case AArch64::BL:
    doDirectCall();
    break;

  case AArch64::B:
    lift_branch();
    break;

  case AArch64::BR:
    doIndirectCall();
    doReturn();
    break;

  case AArch64::BLR:
    doIndirectCall();
    break;

  case AArch64::RET:
    doReturn();
    break;

  case AArch64::Bcc:
    lift_bcc();
    break;

  case AArch64::MRS:
    lift_mrs();
    break;

  case AArch64::MSR:
    lift_msr();
    break;

  case AArch64::ADDWrs:
  case AArch64::ADDWri:
  case AArch64::ADDWrx:
  case AArch64::ADDSWrs:
  case AArch64::ADDSWri:
  case AArch64::ADDSWrx:
  case AArch64::ADDXrs:
  case AArch64::ADDXri:
  case AArch64::ADDXrx:
  case AArch64::ADDSXrs:
  case AArch64::ADDSXri:
  case AArch64::ADDSXrx:
    lift_add(opcode);
    break;

  case AArch64::ADCWr:
  case AArch64::ADCXr:
  case AArch64::ADCSWr:
  case AArch64::ADCSXr:
  case AArch64::SBCWr:
  case AArch64::SBCXr:
  case AArch64::SBCSWr:
  case AArch64::SBCSXr:
    lift_adc_sbc(opcode);
    break;

  case AArch64::ASRVWr:
  case AArch64::ASRVXr: {
    auto size = getInstSize(opcode);
    auto a = readFromOperand(1);
    auto b = readFromOperand(2);

    auto shift_amt =
        createBinop(b, getUnsignedIntConst(size, size), Instruction::URem);
    auto res = createMaskedAShr(a, shift_amt);
    updateOutputReg(res);
    break;
  }

    // SUBrx is a subtract instruction with an extended register.
    // ARM has 8 types of extensions:
    // 000 -> uxtb
    // 001 -> uxth
    // 010 -> uxtw
    // 011 -> uxtx
    // 100 -> sxtb
    // 110 -> sxth
    // 101 -> sxtw
    // 111 -> sxtx
    // To figure out if the extension is signed, we can use (extendType / 4)
    // Since the types repeat byte, half word, word, etc. for signed and
    // unsigned extensions, we can use 8 << (extendType & 0x3) to calculate
    // the extension's byte size
  case AArch64::SUBWri:
  case AArch64::SUBWrs:
  case AArch64::SUBWrx:
  case AArch64::SUBSWrs:
  case AArch64::SUBSWri:
  case AArch64::SUBSWrx:
  case AArch64::SUBXri:
  case AArch64::SUBXrs:
  case AArch64::SUBXrx:
  case AArch64::SUBXrx64:
  case AArch64::SUBSXrs:
  case AArch64::SUBSXri:
  case AArch64::SUBSXrx: {
    auto size = getInstSize(opcode);
    auto ty = getIntTy(size);
    assert(CurInst->getNumOperands() == 4); // dst, lhs, rhs, shift amt
    assert(CurInst->getOperand(3).isImm());

    // convert lhs, rhs operands to IR::Values
    auto a = readFromOperand(1);
    Value *b = nullptr;
    switch (opcode) {
    case AArch64::SUBWrx:
    case AArch64::SUBSWrx:
    case AArch64::SUBXrx:
    case AArch64::SUBXrx64:
    case AArch64::SUBSXrx: {
      auto extendImm = getImm(3);
      auto extendType = (extendImm >> 3) & 0x7;
      auto isSigned = extendType / 4;
      // extendSize is necessary so that we can start with the word size
      // ARM wants us to (byte, half, full) and then sign extend to a new
      // size. Without extendSize being used for a trunc, a lot of masking
      // and more manual work to sign extend would be necessary
      unsigned extendSize = 8 << (extendType & 0x3);
      auto shift = extendImm & 0x7;
      b = readFromOperand(2);

      // Make sure to not to trunc to the same size as the parameter.
      // Sometimes SUBrx is generated using 32 bit registers and "extends" to
      // a 32 bit value. This is seen as a type error by LLVM, but is valid
      // ARM
      if (extendSize != ty->getIntegerBitWidth()) {
        auto truncType = getIntTy(extendSize);
        b = createTrunc(b, truncType);
        b = createCast(b, ty, isSigned ? Instruction::SExt : Instruction::ZExt);
      }

      // shift may not be there, it may just be the extend
      if (shift != 0)
        b = createMaskedShl(b, getUnsignedIntConst(shift, size));
      break;
    }
    default:
      b = readFromOperand(2);
      b = regShift(b, getImm(3));
    }

    // make sure that lhs and rhs conversion succeeded, type lookup succeeded
    if (!ty || !a || !b)
      visitError();

    if (has_s(opcode)) {
      auto ssub = createSSubOverflow(a, b);
      auto result = createExtractValue(ssub, {0});
      auto new_v = createExtractValue(ssub, {1});
      setC(createICmp(ICmpInst::Predicate::ICMP_UGE, a, b));
      setZ(createICmp(ICmpInst::Predicate::ICMP_EQ, a, b));
      setV(new_v);
      setNUsingResult(result);
      updateOutputReg(result);
    } else {
      auto sub = createSub(a, b);
      updateOutputReg(sub);
    }
    break;
  }

  case AArch64::FCSELDrrr:
  case AArch64::FCSELSrrr:
  case AArch64::CSELWr:
  case AArch64::CSELXr: {
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
    break;
  }

  case AArch64::ANDWri:
  case AArch64::ANDWrr:
  case AArch64::ANDWrs:
  case AArch64::ANDSWri:
  case AArch64::ANDSWrr:
  case AArch64::ANDSWrs:
  case AArch64::ANDXri:
  case AArch64::ANDXrr:
  case AArch64::ANDXrs:
  case AArch64::ANDSXri:
  case AArch64::ANDSXrr:
  case AArch64::ANDSXrs: {
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
    break;
  }

  case AArch64::MADDWrrr:
  case AArch64::MADDXrrr: {
    auto mul_lhs = readFromOperand(1);
    auto mul_rhs = readFromOperand(2);
    auto addend = readFromOperand(3);

    auto mul = createMul(mul_lhs, mul_rhs);
    auto add = createAdd(mul, addend);
    updateOutputReg(add);
    break;
  }

  case AArch64::UMADDLrrr: {
    auto size = getInstSize(opcode);
    auto mul_lhs = readFromOperand(1);
    auto mul_rhs = readFromOperand(2);
    auto addend = readFromOperand(3);

    auto lhs_masked =
        createAnd(mul_lhs, getUnsignedIntConst(0xffffffffUL, size));
    auto rhs_masked =
        createAnd(mul_rhs, getUnsignedIntConst(0xffffffffUL, size));
    auto mul = createMul(lhs_masked, rhs_masked);
    auto add = createAdd(mul, addend);
    updateOutputReg(add);
    break;
  }

  case AArch64::SMADDLrrr: {
    // Signed Multiply-Add Long multiplies two 32-bit register values,
    // adds a 64-bit register value, and writes the result to the 64-bit
    // destination register.
    auto mul_lhs = readFromOperand(1);
    auto mul_rhs = readFromOperand(2);
    auto addend = readFromOperand(3);

    // The inputs are automatically zero extended, but we want sign extension,
    // so we need to truncate them back to i32s
    auto lhs_trunc = createTrunc(mul_lhs, i32);
    auto rhs_trunc = createTrunc(mul_rhs, i32);

    // For signed multiplication, must sign extend the lhs and rhs to not
    // overflow
    auto lhs_ext = createSExt(lhs_trunc, i64);
    auto rhs_ext = createSExt(rhs_trunc, i64);

    auto mul = createMul(lhs_ext, rhs_ext);
    auto add = createAdd(mul, addend);
    updateOutputReg(add);
    break;
  }

  case AArch64::SMSUBLrrr:
  case AArch64::UMSUBLrrr: {
    // SMSUBL: Signed Multiply-Subtract Long.
    // UMSUBL: Unsigned Multiply-Subtract Long.
    auto *mul_lhs = readFromOperand(1);
    auto *mul_rhs = readFromOperand(2);
    auto *minuend = readFromOperand(3);

    // The inputs are automatically zero extended, but we want sign
    // extension for signed, so we need to truncate them back to i32s
    auto lhs_trunc = createTrunc(mul_lhs, i32);
    auto rhs_trunc = createTrunc(mul_rhs, i32);

    Value *lhs_extended = nullptr;
    Value *rhs_extended = nullptr;
    if (opcode == AArch64::SMSUBLrrr) {
      // For signed multiplication, must sign extend the lhs and rhs to not
      // overflow
      lhs_extended = createSExt(lhs_trunc, i64);
      rhs_extended = createSExt(rhs_trunc, i64);
    } else {
      lhs_extended = createZExt(lhs_trunc, i64);
      rhs_extended = createZExt(rhs_trunc, i64);
    }

    auto mul = createMul(lhs_extended, rhs_extended);
    auto subtract = createSub(minuend, mul);
    updateOutputReg(subtract);
    break;
  }

  case AArch64::SMULHrr:
  case AArch64::UMULHrr: {
    // SMULH: Signed Multiply High
    // UMULH: Unsigned Multiply High
    auto mul_lhs = readFromOperand(1);
    auto mul_rhs = readFromOperand(2);

    // For unsigned multiplication, must zero extend the lhs and rhs to not
    // overflow For signed multiplication, must sign extend the lhs and rhs to
    // not overflow
    Value *lhs_extended = nullptr, *rhs_extended = nullptr;
    if (opcode == AArch64::UMULHrr) {
      lhs_extended = createZExt(mul_lhs, i128);
      rhs_extended = createZExt(mul_rhs, i128);
    } else {
      lhs_extended = createSExt(mul_lhs, i128);
      rhs_extended = createSExt(mul_rhs, i128);
    }

    auto mul = createMul(lhs_extended, rhs_extended);
    // After multiplying, shift down 64 bits to get the top half of the i128
    // into the bottom half
    auto shift = createMaskedLShr(mul, getUnsignedIntConst(64, 128));

    // Truncate to the proper size:
    auto trunc = createTrunc(shift, i64);
    updateOutputReg(trunc);
    break;
  }

  case AArch64::MSUBWrrr:
  case AArch64::MSUBXrrr: {
    auto mul_lhs = readFromOperand(1);
    auto mul_rhs = readFromOperand(2);
    auto minuend = readFromOperand(3);
    auto mul = createMul(mul_lhs, mul_rhs);
    auto sub = createSub(minuend, mul);
    updateOutputReg(sub);
    break;
  }

  case AArch64::SBFMWri:
  case AArch64::SBFMXri: {
    auto size = getInstSize(opcode);
    auto ty = getIntTy(size);
    auto src = readFromOperand(1);
    auto immr = getImm(2);
    auto imms = getImm(3);

    auto r = getUnsignedIntConst(immr, size);

    // arithmetic shift right (ASR) alias is perferred when:
    // imms == 011111 and size == 32 or when imms == 111111 and size = 64
    if ((size == 32 && imms == 31) || (size == 64 && imms == 63)) {
      auto dst = createMaskedAShr(src, r);
      updateOutputReg(dst);
      break;
    }

    // SXTB
    if (immr == 0 && imms == 7) {
      auto trunc = createTrunc(src, i8);
      auto dst = createSExt(trunc, ty);
      updateOutputReg(dst);
      break;
    }

    // SXTH
    if (immr == 0 && imms == 15) {
      auto trunc = createTrunc(src, i16);
      auto dst = createSExt(trunc, ty);
      updateOutputReg(dst);
      break;
    }

    // SXTW
    if (immr == 0 && imms == 31) {
      auto trunc = createTrunc(src, i32);
      auto dst = createSExt(trunc, ty);
      updateOutputReg(dst);
      break;
    }

    // SBFIZ
    if (imms < immr) {
      auto pos = size - immr;
      auto width = imms + 1;
      assert(width != 64);
      *out << "sbfiz with size = " << size << ", width = " << width << "\n";
      auto mask = ((uint64_t)1 << width) - 1;
      auto mask_comp = ~mask;
      if (size == 32)
        mask_comp &= 0xffffffff;
      auto bitfield_mask = (uint64_t)1 << (width - 1);

      auto masked = createAnd(src, getUnsignedIntConst(mask, size));
      auto bitfield_lsb =
          createAnd(src, getUnsignedIntConst(bitfield_mask, size));
      auto insert_ones = createOr(masked, getUnsignedIntConst(mask_comp, size));
      auto bitfield_lsb_set =
          createICmp(ICmpInst::Predicate::ICMP_NE, bitfield_lsb,
                     getUnsignedIntConst(0, size));
      auto res = createSelect(bitfield_lsb_set, insert_ones, masked);
      auto shifted_res = createMaskedShl(res, getUnsignedIntConst(pos, size));
      updateOutputReg(shifted_res);
      break;
    }

    // FIXME: this requires checking if SBFX is preferred.
    // For now, assume this is always SBFX
    auto width = imms + 1;
    auto mask = ((uint64_t)1 << width) - 1;
    auto pos = immr;

    auto masked = createAnd(src, getUnsignedIntConst(mask, size));
    auto l_shifted =
        createRawShl(masked, getUnsignedIntConst(size - width, size));
    auto shifted_res =
        createRawAShr(l_shifted, getUnsignedIntConst(size - width + pos, size));
    updateOutputReg(shifted_res);
    break;
  }

  case AArch64::CCMPWi:
  case AArch64::CCMPWr:
  case AArch64::CCMPXi:
  case AArch64::CCMPXr: {
    assert(CurInst->getNumOperands() == 4);

    auto lhs = readFromOperand(0);
    auto imm_rhs = readFromOperand(1);

    if (!lhs || !imm_rhs)
      visitError();

    auto [imm_n, imm_z, imm_c, imm_v] = splitImmNZCV(getImm(2));

    auto cond_val_imm = getImm(3);
    auto cond_val = conditionHolds(cond_val_imm);

    auto ssub = createSSubOverflow(lhs, imm_rhs);
    auto result = createExtractValue(ssub, {0});
    auto zero_val = getUnsignedIntConst(0, getBitWidth(result));

    auto new_n = createICmp(ICmpInst::Predicate::ICMP_SLT, result, zero_val);
    auto new_z = createICmp(ICmpInst::Predicate::ICMP_EQ, lhs, imm_rhs);
    auto new_c = createICmp(ICmpInst::Predicate::ICMP_UGE, lhs, imm_rhs);
    auto new_v = createExtractValue(ssub, {1});

    auto new_n_flag = createSelect(cond_val, new_n, imm_n);
    auto new_z_flag = createSelect(cond_val, new_z, imm_z);
    auto new_c_flag = createSelect(cond_val, new_c, imm_c);
    auto new_v_flag = createSelect(cond_val, new_v, imm_v);

    setN(new_n_flag);
    setZ(new_z_flag);
    setC(new_c_flag);
    setV(new_v_flag);
    break;
  }

  case AArch64::EORWri:
  case AArch64::EORXri: {
    auto size = getInstSize(opcode);
    assert(CurInst->getNumOperands() == 3); // dst, src, imm
    assert(CurInst->getOperand(1).isReg() && CurInst->getOperand(2).isImm());

    auto a = readFromOperand(1);
    auto [wmask, _] = decodeBitMasks(getImm(2), size);
    auto imm_val =
        getUnsignedIntConst(wmask,
                            size); // FIXME, need to decode immediate val
    if (!a || !imm_val)
      visitError();

    auto res = createXor(a, imm_val);
    updateOutputReg(res);
    break;
  }
  case AArch64::EORWrs:
  case AArch64::EORXrs: {
    auto lhs = readFromOperand(1);
    auto rhs = readFromOperand(2);
    rhs = regShift(rhs, getImm(3));
    auto result = createXor(lhs, rhs);
    updateOutputReg(result);
    break;
  }

  case AArch64::CCMNWi:
  case AArch64::CCMNWr:
  case AArch64::CCMNXi:
  case AArch64::CCMNXr: {
    auto a = readFromOperand(0);
    auto b = readFromOperand(1);
    auto nzcv = getImm(2);
    auto cond_val_imm = getImm(3);

    auto zero = getBoolConst(false);
    auto one = getBoolConst(true);

    auto [res, flags] = addWithCarry(a, b, zero);
    auto [n, z, c, v] = flags;

    auto cond = conditionHolds(cond_val_imm);
    setN(createSelect(cond, n, (nzcv & 8) ? one : zero));
    setZ(createSelect(cond, z, (nzcv & 4) ? one : zero));
    setC(createSelect(cond, c, (nzcv & 2) ? one : zero));
    setV(createSelect(cond, v, (nzcv & 1) ? one : zero));

    break;
  }

  case AArch64::CSINVWr:
  case AArch64::CSINVXr:
  case AArch64::CSNEGWr:
  case AArch64::CSNEGXr: {
    auto size = getInstSize(opcode);
    // csinv dst, a, b, cond
    // if (cond) a else ~b
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

    auto inverted_b = createNot(b);

    if (opcode == AArch64::CSNEGWr || opcode == AArch64::CSNEGXr) {
      auto negated_b = createAdd(inverted_b, getUnsignedIntConst(1, size));
      auto ret = createSelect(cond_val, a, negated_b);
      updateOutputReg(ret);
    } else {
      auto ret = createSelect(cond_val, a, inverted_b);
      updateOutputReg(ret);
    }
    break;
  }

  case AArch64::CSINCWr:
  case AArch64::CSINCXr: {
    auto size = getInstSize(opcode);
    assert(CurInst->getOperand(1).isReg() && CurInst->getOperand(2).isReg());
    assert(CurInst->getOperand(3).isImm());

    auto a = readFromOperand(1);
    auto b = readFromOperand(2);

    auto cond_val_imm = getImm(3);
    auto cond_val = conditionHolds(cond_val_imm);

    auto inc = createAdd(b, getUnsignedIntConst(1, size));
    auto sel = createSelect(cond_val, a, inc);

    updateOutputReg(sel);
    break;
  }

  case AArch64::MOVZWi:
  case AArch64::MOVZXi: {
    auto size = getInstSize(opcode);
    assert(CurInst->getOperand(0).isReg());
    assert(CurInst->getOperand(1).isImm());
    auto lhs = readFromOperand(1);
    lhs = regShift(lhs, getImm(2));
    auto rhs = getUnsignedIntConst(0, size);
    auto ident = createAdd(lhs, rhs);
    updateOutputReg(ident);
    break;
  }

  case AArch64::MOVNWi:
  case AArch64::MOVNXi: {
    assert(CurInst->getOperand(0).isReg());
    assert(CurInst->getOperand(1).isImm());
    assert(CurInst->getOperand(2).isImm());

    auto lhs = readFromOperand(1);
    lhs = regShift(lhs, getImm(2));
    auto not_lhs = createNot(lhs);

    updateOutputReg(not_lhs);
    break;
  }

  case AArch64::LSLVWr:
  case AArch64::LSLVXr: {
    auto lhs = readFromOperand(1);
    auto rhs = readFromOperand(2);
    auto exp = createMaskedShl(lhs, rhs);
    updateOutputReg(exp);
    break;
  }

  case AArch64::LSRVWr:
  case AArch64::LSRVXr: {
    auto lhs = readFromOperand(1);
    auto rhs = readFromOperand(2);
    auto exp = createMaskedLShr(lhs, rhs);
    updateOutputReg(exp);
    break;
  }

  case AArch64::ORNWrs:
  case AArch64::ORNXrs: {
    auto lhs = readFromOperand(1);
    auto rhs = readFromOperand(2);
    rhs = regShift(rhs, getImm(3));

    auto not_rhs = createNot(rhs);
    auto ident = createOr(lhs, not_rhs);
    updateOutputReg(ident);
    break;
  }

  case AArch64::MOVKWi:
  case AArch64::MOVKXi: {
    auto size = getInstSize(opcode);
    auto dest = readFromOperand(1);
    auto lhs = readFromOperand(2);
    lhs = regShift(lhs, getImm(3));

    uint64_t bitmask;
    auto shift_amt = getImm(3);

    if (opcode == AArch64::MOVKWi) {
      assert(shift_amt == 0 || shift_amt == 16);
      bitmask = (shift_amt == 0) ? 0xffff0000 : 0x0000ffff;
    } else {
      assert(shift_amt == 0 || shift_amt == 16 || shift_amt == 32 ||
             shift_amt == 48);
      bitmask = ~(((uint64_t)0xffff) << shift_amt);
    }

    auto bottom_bits = getUnsignedIntConst(bitmask, size);
    auto cleared = createAnd(dest, bottom_bits);
    auto ident = createOr(cleared, lhs);
    updateOutputReg(ident);
    break;
  }

  case AArch64::UBFMWri:
  case AArch64::UBFMXri: {
    auto size = getInstSize(opcode);
    auto src = readFromOperand(1);
    auto immr = getImm(2);
    auto imms = getImm(3);

    // LSL is preferred when imms != 31 and imms + 1 == immr
    if (size == 32 && imms != 31 && imms + 1 == immr) {
      auto dst = createMaskedShl(src, getUnsignedIntConst(31 - imms, size));
      updateOutputReg(dst);
      break;
    }

    // LSL is preferred when imms != 63 and imms + 1 == immr
    if (size == 64 && imms != 63 && imms + 1 == immr) {
      auto dst = createMaskedShl(src, getUnsignedIntConst(63 - imms, size));
      updateOutputReg(dst);
      break;
    }

    // LSR is preferred when imms == 31 or 63 (size - 1)
    if (imms == size - 1) {
      auto dst = createMaskedLShr(src, getUnsignedIntConst(immr, size));
      updateOutputReg(dst);
      break;
    }

    // UBFIZ
    if (imms < immr) {
      auto pos = size - immr;
      auto width = imms + 1;
      auto mask = ((uint64_t)1 << width) - 1;
      auto masked = createAnd(src, getUnsignedIntConst(mask, size));
      auto shifted = createMaskedShl(masked, getUnsignedIntConst(pos, size));
      updateOutputReg(shifted);
      break;
    }

    // UXTB
    if (immr == 0 && imms == 7) {
      auto mask = ((uint64_t)1 << 8) - 1;
      auto masked = createAnd(src, getUnsignedIntConst(mask, size));
      updateOutputReg(masked);
      break;
    }

    // UXTH
    if (immr == 0 && imms == 15) {
      auto mask = ((uint64_t)1 << 16) - 1;
      auto masked = createAnd(src, getUnsignedIntConst(mask, size));
      updateOutputReg(masked);
      break;
    }

    // UBFX
    // FIXME: this requires checking if UBFX is preferred.
    // For now, assume this is always UBFX
    // we mask from lsb to lsb + width and then perform a logical shift right
    auto width = imms + 1;
    auto mask = ((uint64_t)1 << width) - 1;
    auto pos = immr;

    auto masked = createAnd(src, getUnsignedIntConst(mask, size));
    auto shifted_res = createMaskedLShr(masked, getUnsignedIntConst(pos, size));
    updateOutputReg(shifted_res);
    break;
  }

  case AArch64::BFMWri:
  case AArch64::BFMXri: {
    auto size = getInstSize(opcode);
    auto dst = readFromOperand(1);
    auto src = readFromOperand(2);

    auto immr = getImm(3);
    auto imms = getImm(4);

    // FIXME -- would be better to use decodeBitMasks() here, as
    // shown in the ARM docs

    if (imms >= immr) {
      // BFXIL

      auto bits = (imms - immr + 1);
      auto pos = immr;
      auto mask = (((uint64_t)1 << bits) - 1) << pos;

      auto masked = createAnd(src, getUnsignedIntConst(mask, size));
      auto shifted = createMaskedLShr(masked, getUnsignedIntConst(pos, size));
      auto cleared =
          createAnd(dst, getSignedIntConst((uint64_t)-1 << bits, size));
      auto res = createOr(cleared, shifted);
      updateOutputReg(res);
    } else {
      auto bits = imms + 1;
      auto pos = size - immr;

      // This mask deletes `bits` number of bits starting at `pos`.
      // If the mask is for a 32 bit value, it will chop off the top 32 bits
      // of the 64 bit mask to keep the mask to a size of 32 bits
      auto mask =
          ~((((uint64_t)1 << bits) - 1) << pos) & ((uint64_t)-1 >> (64 - size));

      // get `bits` number of bits from the least significant bits
      auto bitfield =
          createAnd(src, getUnsignedIntConst(~((uint64_t)-1 << bits), size));

      // move the bitfield into position
      auto moved = createMaskedShl(bitfield, getUnsignedIntConst(pos, size));

      // carve out a place for the bitfield
      auto masked = createAnd(dst, getUnsignedIntConst(mask, size));
      // place the bitfield
      auto res = createOr(masked, moved);
      updateOutputReg(res);
    }
    break;
  }

  case AArch64::ORRWri:
  case AArch64::ORRXri: {
    auto size = getInstSize(opcode);
    auto lhs = readFromOperand(1);
    auto imm = getImm(2);
    auto [wmask, _] = decodeBitMasks(imm, size);
    auto result = createOr(lhs, getUnsignedIntConst(wmask, size));
    updateOutputReg(result);
    break;
  }

  case AArch64::ORRWrs:
  case AArch64::ORRXrs: {
    auto lhs = readFromOperand(1);
    auto rhs = readFromOperand(2);
    rhs = regShift(rhs, getImm(3));
    auto result = createOr(lhs, rhs);
    updateOutputReg(result);
    break;
  }

  // can't directly lift ARM sdiv to LLVM sdiv because the latter
  // is UB for divide by zero and INT_MIN / -1
  case AArch64::SDIVWr:
  case AArch64::SDIVXr: {
    auto Size = getInstSize(opcode);
    auto Zero = getUnsignedIntConst(0, Size);
    auto IntMin = createMaskedShl(getUnsignedIntConst(1, Size),
                                  getUnsignedIntConst(Size - 1, Size));
    auto LHS = readFromOperand(1);
    auto RHS = readFromOperand(2);
    auto ResMem = createAlloca(getIntTy(Size), getUnsignedIntConst(1, 64), "");
    createStore(Zero, ResMem);
    auto RHSIsZero = createICmp(ICmpInst::Predicate::ICMP_EQ, RHS, Zero);
    auto LHSIsIntMin = createICmp(ICmpInst::Predicate::ICMP_EQ, LHS, IntMin);
    auto RHSIsAllOnes =
        createICmp(ICmpInst::Predicate::ICMP_EQ, RHS, getAllOnesConst(Size));
    auto IsOverflow = createAnd(LHSIsIntMin, RHSIsAllOnes);
    auto Cond = createOr(RHSIsZero, IsOverflow);
    auto DivBB = BasicBlock::Create(Ctx, "", liftedFn);
    auto ContBB = BasicBlock::Create(Ctx, "", liftedFn);
    createBranch(Cond, ContBB, DivBB);
    LLVMBB = DivBB;
    auto DivResult = createSDiv(LHS, RHS);
    createStore(DivResult, ResMem);
    createBranch(ContBB);
    LLVMBB = ContBB;
    auto result = createLoad(getIntTy(Size), ResMem);
    updateOutputReg(result);
    break;
  }

  // can't directly lift ARM udiv to LLVM udiv because the latter
  // is UB for divide by zero
  case AArch64::UDIVWr:
  case AArch64::UDIVXr: {
    auto size = getInstSize(opcode);
    auto zero = getUnsignedIntConst(0, size);
    auto lhs = readFromOperand(1);
    auto rhs = readFromOperand(2);
    auto A = createAlloca(getIntTy(size), getUnsignedIntConst(1, 64), "");
    createStore(zero, A);
    auto rhsIsZero = createICmp(ICmpInst::Predicate::ICMP_EQ, rhs, zero);
    auto DivBB = BasicBlock::Create(Ctx, "", liftedFn);
    auto ContBB = BasicBlock::Create(Ctx, "", liftedFn);
    createBranch(rhsIsZero, ContBB, DivBB);
    LLVMBB = DivBB;
    auto divResult = createUDiv(lhs, rhs);
    createStore(divResult, A);
    createBranch(ContBB);
    LLVMBB = ContBB;
    auto result = createLoad(getIntTy(size), A);
    updateOutputReg(result);
    break;
  }

  case AArch64::EXTRWrri:
  case AArch64::EXTRXrri: {
    auto op1 = readFromOperand(1);
    auto op2 = readFromOperand(2);
    auto shift = readFromOperand(3);
    auto result = createFShr(op1, op2, shift);
    updateOutputReg(result);
    break;
  }

  case AArch64::RORVWr:
  case AArch64::RORVXr: {
    auto op = readFromOperand(1);
    auto shift = readFromOperand(2);
    auto result = createFShr(op, op, shift);
    updateOutputReg(result);
    break;
  }

  case AArch64::RBITWr:
  case AArch64::RBITXr: {
    auto op = readFromOperand(1);
    auto result = createBitReverse(op);
    updateOutputReg(result);
    break;
  }

  case AArch64::REVWr:
  case AArch64::REVXr:
    updateOutputReg(createBSwap(readFromOperand(1)));
    break;

  case AArch64::CLZWr:
  case AArch64::CLZXr: {
    auto op = readFromOperand(1);
    auto result = createCtlz(op);
    updateOutputReg(result);
    break;
  }

  case AArch64::EONWrs:
  case AArch64::EONXrs:
  case AArch64::BICWrs:
  case AArch64::BICXrs:
  case AArch64::BICSWrs:
  case AArch64::BICSXrs: {
    // BIC:
    // return = op1 AND NOT (optional shift) op2
    // EON:
    // return = op1 XOR NOT (optional shift) op2

    auto op1 = readFromOperand(1);
    auto op2 = readFromOperand(2);

    // If there is a shift to be performed on the second operand
    if (CurInst->getNumOperands() == 4) {
      // the 4th operand (if it exists) must b an immediate
      assert(CurInst->getOperand(3).isImm());
      op2 = regShift(op2, getImm(3));
    }

    auto inverted_op2 = createNot(op2);

    // Perform final Op: AND for BIC, XOR for EON
    Value *ret = nullptr;
    switch (opcode) {
    case AArch64::BICWrs:
    case AArch64::BICXrs:
    case AArch64::BICSXrs:
    case AArch64::BICSWrs:
      ret = createAnd(op1, inverted_op2);
      break;
    case AArch64::EONWrs:
    case AArch64::EONXrs:
      ret = createXor(op1, inverted_op2);
      break;
    default:
      assert(false && "missed case in EON/BIC");
    }

    if (has_s(opcode)) {
      setNUsingResult(ret);
      setZUsingResult(ret);
      setC(getBoolConst(false));
      setV(getBoolConst(false));
    }

    updateOutputReg(ret);
    break;
  }

  case AArch64::REV16Xr: {
    // REV16Xr: Reverse bytes of 64 bit value in 16-bit half-words.
    auto size = getInstSize(opcode);
    auto val = readFromOperand(1);
    auto first_part = createMaskedShl(val, getUnsignedIntConst(8, size));
    auto first_part_and =
        createAnd(first_part, getUnsignedIntConst(0xFF00FF00FF00FF00UL, size));
    auto second_part = createMaskedLShr(val, getUnsignedIntConst(8, size));
    auto second_part_and =
        createAnd(second_part, getUnsignedIntConst(0x00FF00FF00FF00FFUL, size));
    auto combined_val = createOr(first_part_and, second_part_and);
    updateOutputReg(combined_val);
    break;
  }

  case AArch64::REV16Wr:
  case AArch64::REV32Xr: {
    // REV16Wr: Reverse bytes of 32 bit value in 16-bit half-words.
    // REV32Xr: Reverse bytes of 64 bit value in 32-bit words.
    auto size = getInstSize(opcode);
    auto val = readFromOperand(1);

    // Reversing all of the bytes, then performing a rotation by half the
    // width reverses bytes in 16-bit halfwords for a 32 bit int and reverses
    // bytes in a 32-bit word for a 64 bit int
    auto reverse_val = createBSwap(val);
    auto ret = createFShr(reverse_val, reverse_val,
                          getUnsignedIntConst(size / 2, size));
    updateOutputReg(ret);
    break;
  }

  case AArch64::LDRSBXui:
  case AArch64::LDRSBWui:
  case AArch64::LDRSHXui:
  case AArch64::LDRSHWui:
  case AArch64::LDRSWui:
  case AArch64::LDRBBui:
  case AArch64::LDRBui:
  case AArch64::LDRHHui:
  case AArch64::LDRHui:
  case AArch64::LDRWui:
  case AArch64::LDRSui:
  case AArch64::LDRXui:
  case AArch64::LDRDui:
  case AArch64::LDRQui: {
    unsigned size;
    switch (opcode) {
    case AArch64::LDRBBui:
    case AArch64::LDRBui:
    case AArch64::LDRSBXui:
    case AArch64::LDRSBWui:
      size = 1;
      break;
    case AArch64::LDRHHui:
    case AArch64::LDRHui:
    case AArch64::LDRSHXui:
    case AArch64::LDRSHWui:
      size = 2;
      break;
    case AArch64::LDRWui:
    case AArch64::LDRSui:
    case AArch64::LDRSWui:
      size = 4;
      break;
    case AArch64::LDRXui:
    case AArch64::LDRDui:
      size = 8;
      break;
    case AArch64::LDRQui:
      size = 16;
      break;
    default:
      assert(false);
    }

    bool sExt = opcode == AArch64::LDRSBXui || opcode == AArch64::LDRSBWui ||
                opcode == AArch64::LDRSHXui || opcode == AArch64::LDRSHWui ||
                opcode == AArch64::LDRSWui;

    MCOperand &op2 = CurInst->getOperand(2);
    if (op2.isExpr()) {
      *out << "[operand 2 is expr]\n";
      auto [globalVar, storePtr] = getExprVar(op2.getExpr());
      if (storePtr) {
        Value *ptrToInt = createPtrToInt(globalVar, getIntTy(size * 8));
        updateOutputReg(ptrToInt, sExt);
      } else {
        auto loaded = makeLoadWithOffset(globalVar, 0, size);
        updateOutputReg(loaded, sExt);
      }
    } else {
      *out << "[operand 2 is not expr]\n";
      auto [base, imm] = getParamsLoadImmed();
      auto loaded = makeLoadWithOffset(base, imm * size, size);
      updateOutputReg(loaded, sExt);
    }
    break;
  }

  case AArch64::LDURSBWi:
  case AArch64::LDURSBXi:
  case AArch64::LDURSHWi:
  case AArch64::LDURSHXi:
  case AArch64::LDURSWi:
  case AArch64::LDURBi:
  case AArch64::LDURBBi:
  case AArch64::LDURHi:
  case AArch64::LDURHHi:
  case AArch64::LDURSi:
  case AArch64::LDURWi:
  case AArch64::LDURDi:
  case AArch64::LDURXi:
  case AArch64::LDURQi: {
    unsigned size;
    if (opcode == AArch64::LDURBBi || opcode == AArch64::LDURBi ||
        opcode == AArch64::LDURSBWi || opcode == AArch64::LDURSBXi)
      size = 1;
    else if (opcode == AArch64::LDURHHi || opcode == AArch64::LDURHi ||
             opcode == AArch64::LDURSHWi || opcode == AArch64::LDURSHXi)
      size = 2;
    else if (opcode == AArch64::LDURWi || opcode == AArch64::LDURSi ||
             opcode == AArch64::LDURSWi)
      size = 4;
    else if (opcode == AArch64::LDURXi || opcode == AArch64::LDURDi)
      size = 8;
    else if (opcode == AArch64::LDURQi)
      size = 16;
    else
      assert(false);

    bool sExt = opcode == AArch64::LDURSBWi || opcode == AArch64::LDURSBXi ||
                opcode == AArch64::LDURSHWi || opcode == AArch64::LDURSHXi ||
                opcode == AArch64::LDURSWi;
    auto [base, imm] = getParamsLoadImmed();
    // Start offset as a 9-bit signed integer
    assert(imm <= 255 && imm >= -256);
    auto offset = getSignedIntConst(imm, 9);
    Value *offsetVal = createSExt(offset, i64);
    auto loaded = makeLoadWithOffset(base, offsetVal, size);
    updateOutputReg(loaded, sExt);
    break;
  }
  case AArch64::LDRSBWpre:
  case AArch64::LDRSBXpre:
  case AArch64::LDRSHWpre:
  case AArch64::LDRSHXpre:
  case AArch64::LDRSWpre:
  case AArch64::LDRBBpre:
  case AArch64::LDRBpre:
  case AArch64::LDRHHpre:
  case AArch64::LDRHpre:
  case AArch64::LDRWpre:
  case AArch64::LDRSpre:
  case AArch64::LDRXpre:
  case AArch64::LDRDpre:
  case AArch64::LDRQpre:
  case AArch64::LDRSBWpost:
  case AArch64::LDRSBXpost:
  case AArch64::LDRSHWpost:
  case AArch64::LDRSHXpost:
  case AArch64::LDRSWpost:
  case AArch64::LDRBBpost:
  case AArch64::LDRBpost:
  case AArch64::LDRHHpost:
  case AArch64::LDRHpost:
  case AArch64::LDRWpost:
  case AArch64::LDRSpost:
  case AArch64::LDRXpost:
  case AArch64::LDRDpost:
  case AArch64::LDRQpost: {
    unsigned size;
    switch (opcode) {
    case AArch64::LDRSBWpre:
    case AArch64::LDRSBXpre:
    case AArch64::LDRBBpre:
    case AArch64::LDRBpre:
    case AArch64::LDRSBWpost:
    case AArch64::LDRSBXpost:
    case AArch64::LDRBBpost:
    case AArch64::LDRBpost:
      size = 1;
      break;
    case AArch64::LDRSHWpre:
    case AArch64::LDRSHXpre:
    case AArch64::LDRHHpre:
    case AArch64::LDRHpre:
    case AArch64::LDRSHWpost:
    case AArch64::LDRSHXpost:
    case AArch64::LDRHHpost:
    case AArch64::LDRHpost:
      size = 2;
      break;
    case AArch64::LDRSWpre:
    case AArch64::LDRWpre:
    case AArch64::LDRSpre:
    case AArch64::LDRSWpost:
    case AArch64::LDRWpost:
    case AArch64::LDRSpost:
      size = 4;
      break;
    case AArch64::LDRXpre:
    case AArch64::LDRDpre:
    case AArch64::LDRXpost:
    case AArch64::LDRDpost:
      size = 8;
      break;
    case AArch64::LDRQpre:
    case AArch64::LDRQpost:
      size = 16;
      break;
    default:
      assert(false);
    }
    bool sExt = opcode == AArch64::LDRSBWpre || opcode == AArch64::LDRSBXpre ||
                opcode == AArch64::LDRSHWpre || opcode == AArch64::LDRSHXpre ||
                opcode == AArch64::LDRSWpre || opcode == AArch64::LDRSBWpost ||
                opcode == AArch64::LDRSBXpost ||
                opcode == AArch64::LDRSHWpost ||
                opcode == AArch64::LDRSHXpost || opcode == AArch64::LDRSWpost;
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    assert(op0.isReg() && op1.isReg() && op2.isReg());
    assert(op0.getReg() == op2.getReg());
    assert(op3.isImm());

    // For pre and post index memory instructions, the destination register
    // is at position 1
    auto destReg = op1.getReg();
    auto baseReg = op2.getReg();
    auto imm = op3.getImm();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    // Start offset as a 9-bit signed integer
    assert(imm <= 255 && imm >= -256);
    auto offset = getSignedIntConst(imm, 9);
    Value *offsetVal = createSExt(offset, i64);
    Value *zeroVal = getUnsignedIntConst(0, 64);

    bool isPre = opcode == AArch64::LDRBBpre || opcode == AArch64::LDRBpre ||
                 opcode == AArch64::LDRHHpre || opcode == AArch64::LDRHpre ||
                 opcode == AArch64::LDRWpre || opcode == AArch64::LDRSpre ||
                 opcode == AArch64::LDRXpre || opcode == AArch64::LDRDpre ||
                 opcode == AArch64::LDRQpre || opcode == AArch64::LDRSBWpre ||
                 opcode == AArch64::LDRSBXpre || opcode == AArch64::LDRSHWpre ||
                 opcode == AArch64::LDRSHXpre || opcode == AArch64::LDRSWpre;

    auto loaded = makeLoadWithOffset(base, isPre ? offsetVal : zeroVal, size);
    updateReg(loaded, destReg, sExt);

    auto added = createAdd(baseAddr, offsetVal);
    updateOutputReg(added, sExt);
    break;
  }

  case AArch64::LDRBBroW:
  case AArch64::LDRBBroX:
  case AArch64::LDRBroW:
  case AArch64::LDRBroX:
  case AArch64::LDRHHroW:
  case AArch64::LDRHHroX:
  case AArch64::LDRHroW:
  case AArch64::LDRHroX:
  case AArch64::LDRWroW:
  case AArch64::LDRWroX:
  case AArch64::LDRSroW:
  case AArch64::LDRSroX:
  case AArch64::LDRXroW:
  case AArch64::LDRXroX:
  case AArch64::LDRDroW:
  case AArch64::LDRDroX:
  case AArch64::LDRQroW:
  case AArch64::LDRQroX:
  case AArch64::LDRSBWroW:
  case AArch64::LDRSBWroX:
  case AArch64::LDRSBXroW:
  case AArch64::LDRSBXroX:
  case AArch64::LDRSHWroW:
  case AArch64::LDRSHWroX:
  case AArch64::LDRSHXroW:
  case AArch64::LDRSHXroX:
  case AArch64::LDRSWroW:
  case AArch64::LDRSWroX: {
    unsigned size;

    switch (opcode) {
    case AArch64::LDRBBroW:
    case AArch64::LDRBBroX:
    case AArch64::LDRBroW:
    case AArch64::LDRBroX:
    case AArch64::LDRSBWroW:
    case AArch64::LDRSBWroX:
    case AArch64::LDRSBXroW:
    case AArch64::LDRSBXroX:
      size = 1;
      break;
    case AArch64::LDRHHroW:
    case AArch64::LDRHHroX:
    case AArch64::LDRHroW:
    case AArch64::LDRHroX:
    case AArch64::LDRSHWroW:
    case AArch64::LDRSHWroX:
    case AArch64::LDRSHXroW:
    case AArch64::LDRSHXroX:
      size = 2;
      break;
    case AArch64::LDRWroX:
    case AArch64::LDRWroW:
    case AArch64::LDRSroW:
    case AArch64::LDRSroX:
    case AArch64::LDRSWroW:
    case AArch64::LDRSWroX:
      size = 4;
      break;
    case AArch64::LDRXroW:
    case AArch64::LDRXroX:
    case AArch64::LDRDroW:
    case AArch64::LDRDroX:
      size = 8;
      break;
    case AArch64::LDRQroW:
    case AArch64::LDRQroX:
      size = 16;
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }

    bool sExt = opcode == AArch64::LDRSBWroW || opcode == AArch64::LDRSBWroX ||
                opcode == AArch64::LDRSBXroW || opcode == AArch64::LDRSBXroX ||
                opcode == AArch64::LDRSHWroW || opcode == AArch64::LDRSHWroX ||
                opcode == AArch64::LDRSHXroW || opcode == AArch64::LDRSHXroX ||
                opcode == AArch64::LDRSWroW || opcode == AArch64::LDRSWroX;

    auto [base, offset] = getParamsLoadReg();
    auto loaded = makeLoadWithOffset(base, offset, size);
    updateOutputReg(loaded, sExt);
    break;
  }

  case AArch64::LD1i8:
  case AArch64::LD1i16:
  case AArch64::LD1i32:
  case AArch64::LD1i64:
  case AArch64::LD1i8_POST:
  case AArch64::LD1i16_POST:
  case AArch64::LD1i32_POST:
  case AArch64::LD1i64_POST: {
    unsigned numElts, eltSize;

    switch (opcode) {
    case AArch64::LD1i8:
    case AArch64::LD1i8_POST:
      numElts = 16;
      eltSize = 8;
      break;
    case AArch64::LD1i16:
    case AArch64::LD1i16_POST:
      numElts = 8;
      eltSize = 16;
      break;
    case AArch64::LD1i32:
    case AArch64::LD1i32_POST:
      numElts = 4;
      eltSize = 32;
      break;
    case AArch64::LD1i64:
    case AArch64::LD1i64_POST:
      numElts = 2;
      eltSize = 64;
      break;
    default:
      assert(false);
    }
    unsigned nregs = 1;
    bool isPost =
        opcode == AArch64::LD1i8_POST || opcode == AArch64::LD1i16_POST ||
        opcode == AArch64::LD1i32_POST || opcode == AArch64::LD1i64_POST;

    auto regCounter =
        decodeRegSet(CurInst->getOperand(isPost ? 2 : 1).getReg());
    auto index = getImm(isPost ? 3 : 2);
    auto baseReg = CurInst->getOperand(isPost ? 4 : 3).getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    assert((regCounter >= AArch64::Q0 && regCounter <= AArch64::Q31));
    Value *totalOffset{nullptr};
    if (isPost) {
      if (CurInst->getOperand(5).isReg() &&
          CurInst->getOperand(5).getReg() != AArch64::XZR) {
        totalOffset = readFromRegOld(CurInst->getOperand(5).getReg());
      } else {
        totalOffset = getUnsignedIntConst(nregs * (eltSize / 8), 64);
      }
    }

    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    auto offset = 0;
    for (unsigned i = 0; i < nregs; i++, offset += eltSize / 8) {
      auto loaded = makeLoadWithOffset(base, offset, eltSize / 8);
      auto dst = readFromRegTyped(regCounter, getVecTy(eltSize, numElts));
      auto updated = createInsertElement(dst, loaded, index);
      updateReg(updated, regCounter);

      regCounter++;
      if (regCounter > AArch64::Q31)
        regCounter = AArch64::Q0;
    }

    if (isPost) {
      auto added = createAdd(baseAddr, totalOffset);
      updateOutputReg(added);
    }
    break;
  }

  case AArch64::LD1Rv8b:
  case AArch64::LD1Rv16b:
  case AArch64::LD1Rv4h:
  case AArch64::LD1Rv8h:
  case AArch64::LD1Rv2s:
  case AArch64::LD1Rv4s:
  case AArch64::LD1Rv1d:
  case AArch64::LD1Rv2d:
  case AArch64::LD1Rv8b_POST:
  case AArch64::LD1Rv16b_POST:
  case AArch64::LD1Rv4h_POST:
  case AArch64::LD1Rv8h_POST:
  case AArch64::LD1Rv2s_POST:
  case AArch64::LD1Rv4s_POST:
  case AArch64::LD1Rv1d_POST:
  case AArch64::LD1Rv2d_POST: {
    unsigned numElts, eltSize;
    switch (opcode) {
    case AArch64::LD1Rv8b:
    case AArch64::LD1Rv8b_POST:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::LD1Rv16b:
    case AArch64::LD1Rv16b_POST:
      numElts = 16;
      eltSize = 8;
      break;
    case AArch64::LD1Rv4h:
    case AArch64::LD1Rv4h_POST:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::LD1Rv8h:
    case AArch64::LD1Rv8h_POST:
      numElts = 8;
      eltSize = 16;
      break;
    case AArch64::LD1Rv2s:
    case AArch64::LD1Rv2s_POST:
      numElts = 2;
      eltSize = 32;
      break;
    case AArch64::LD1Rv4s:
    case AArch64::LD1Rv4s_POST:
      numElts = 4;
      eltSize = 32;
      break;
    case AArch64::LD1Rv1d:
    case AArch64::LD1Rv1d_POST:
      numElts = 1;
      eltSize = 64;
      break;
    case AArch64::LD1Rv2d:
    case AArch64::LD1Rv2d_POST:
      numElts = 2;
      eltSize = 64;
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }

    bool isPost = false;
    switch (opcode) {
    case AArch64::LD1Rv8b_POST:
    case AArch64::LD1Rv16b_POST:
    case AArch64::LD1Rv4h_POST:
    case AArch64::LD1Rv8h_POST:
    case AArch64::LD1Rv2s_POST:
    case AArch64::LD1Rv4s_POST:
    case AArch64::LD1Rv1d_POST:
    case AArch64::LD1Rv2d_POST:
      isPost = true;
      break;
    default:
      isPost = false;
      break;
    }
    auto dst = readFromVecOperand(isPost ? 1 : 0, eltSize, numElts);
    auto baseReg = CurInst->getOperand(isPost ? 2 : 1).getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));

    Value *offset = nullptr;
    if (isPost) {
      if (CurInst->getOperand(3).isReg() &&
          CurInst->getOperand(3).getReg() != AArch64::XZR) {
        offset = readFromRegOld(CurInst->getOperand(3).getReg());
      } else {
        offset = getUnsignedIntConst((eltSize / 8), 64);
      }
    }

    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);
    auto loaded = makeLoadWithOffset(base, 0, eltSize / 8);
    auto single_inserted = createInsertElement(dst, loaded, 0);
    auto shuffled =
        createShuffleVector(single_inserted, getZeroIntVec(numElts, 32));

    if (isPost) {
      updateReg(shuffled, CurInst->getOperand(1).getReg());
      auto added = createAdd(baseAddr, offset);
      updateOutputReg(added);
    } else {
      updateOutputReg(shuffled);
    }
    break;
  }

  case AArch64::LD1Onev8b:
  case AArch64::LD1Onev16b:
  case AArch64::LD1Onev4h:
  case AArch64::LD1Onev8h:
  case AArch64::LD1Onev2s:
  case AArch64::LD1Onev4s:
  case AArch64::LD1Onev1d:
  case AArch64::LD1Onev2d:
  case AArch64::LD1Onev8b_POST:
  case AArch64::LD1Onev16b_POST:
  case AArch64::LD1Onev4h_POST:
  case AArch64::LD1Onev8h_POST:
  case AArch64::LD1Onev2s_POST:
  case AArch64::LD1Onev4s_POST:
  case AArch64::LD1Onev1d_POST:
  case AArch64::LD1Onev2d_POST:
  case AArch64::LD1Twov1d:
  case AArch64::LD1Twov2d:
  case AArch64::LD1Threev1d:
  case AArch64::LD1Threev2d:
  case AArch64::LD1Fourv1d:
  case AArch64::LD1Fourv2d:
  case AArch64::LD2Twov8b:
  case AArch64::LD2Twov16b:
  case AArch64::LD2Twov4h:
  case AArch64::LD2Twov8h:
  case AArch64::LD2Twov2s:
  case AArch64::LD2Twov4s:
  case AArch64::LD2Twov2d:
  case AArch64::LD2Twov8b_POST:
  case AArch64::LD2Twov16b_POST:
  case AArch64::LD2Twov4h_POST:
  case AArch64::LD2Twov8h_POST:
  case AArch64::LD2Twov2s_POST:
  case AArch64::LD2Twov4s_POST:
  case AArch64::LD2Twov2d_POST:
  case AArch64::LD3Threev8b:
  case AArch64::LD3Threev16b:
  case AArch64::LD3Threev4h:
  case AArch64::LD3Threev8h:
  case AArch64::LD3Threev2s:
  case AArch64::LD3Threev4s:
  case AArch64::LD3Threev2d:
  case AArch64::LD3Threev8b_POST:
  case AArch64::LD3Threev16b_POST:
  case AArch64::LD3Threev4h_POST:
  case AArch64::LD3Threev8h_POST:
  case AArch64::LD3Threev2s_POST:
  case AArch64::LD3Threev4s_POST:
  case AArch64::LD3Threev2d_POST:
  case AArch64::LD4Fourv8b:
  case AArch64::LD4Fourv16b:
  case AArch64::LD4Fourv4h:
  case AArch64::LD4Fourv8h:
  case AArch64::LD4Fourv2s:
  case AArch64::LD4Fourv4s:
  case AArch64::LD4Fourv2d:
  case AArch64::LD4Fourv8b_POST:
  case AArch64::LD4Fourv16b_POST:
  case AArch64::LD4Fourv4h_POST:
  case AArch64::LD4Fourv8h_POST:
  case AArch64::LD4Fourv2s_POST:
  case AArch64::LD4Fourv4s_POST:
  case AArch64::LD4Fourv2d_POST: {
    unsigned numElts, eltSize;
    bool fullWidth;
    switch (opcode) {
    case AArch64::LD1Onev8b:
    case AArch64::LD1Onev8b_POST:
    case AArch64::LD1Twov8b:
    case AArch64::LD1Twov8b_POST:
    case AArch64::LD1Threev8b:
    case AArch64::LD1Threev8b_POST:
    case AArch64::LD1Fourv8b:
    case AArch64::LD1Fourv8b_POST:
    case AArch64::LD2Twov8b:
    case AArch64::LD2Twov8b_POST:
    case AArch64::LD3Threev8b:
    case AArch64::LD3Threev8b_POST:
    case AArch64::LD4Fourv8b:
    case AArch64::LD4Fourv8b_POST:
      numElts = 8;
      eltSize = 8;
      fullWidth = false;
      break;
    case AArch64::LD1Onev16b:
    case AArch64::LD1Onev16b_POST:
    case AArch64::LD1Twov16b:
    case AArch64::LD1Twov16b_POST:
    case AArch64::LD1Threev16b:
    case AArch64::LD1Threev16b_POST:
    case AArch64::LD1Fourv16b:
    case AArch64::LD1Fourv16b_POST:
    case AArch64::LD2Twov16b:
    case AArch64::LD2Twov16b_POST:
    case AArch64::LD3Threev16b:
    case AArch64::LD3Threev16b_POST:
    case AArch64::LD4Fourv16b:
    case AArch64::LD4Fourv16b_POST:
      numElts = 16;
      eltSize = 8;
      fullWidth = true;
      break;
    case AArch64::LD1Onev4h:
    case AArch64::LD1Onev4h_POST:
    case AArch64::LD1Twov4h:
    case AArch64::LD1Twov4h_POST:
    case AArch64::LD1Threev4h:
    case AArch64::LD1Threev4h_POST:
    case AArch64::LD1Fourv4h:
    case AArch64::LD1Fourv4h_POST:
    case AArch64::LD2Twov4h:
    case AArch64::LD2Twov4h_POST:
    case AArch64::LD3Threev4h:
    case AArch64::LD3Threev4h_POST:
    case AArch64::LD4Fourv4h:
    case AArch64::LD4Fourv4h_POST:
      numElts = 4;
      eltSize = 16;
      fullWidth = false;
      break;
    case AArch64::LD1Onev8h:
    case AArch64::LD1Onev8h_POST:
    case AArch64::LD1Twov8h:
    case AArch64::LD1Twov8h_POST:
    case AArch64::LD1Threev8h:
    case AArch64::LD1Threev8h_POST:
    case AArch64::LD1Fourv8h:
    case AArch64::LD1Fourv8h_POST:
    case AArch64::LD2Twov8h:
    case AArch64::LD2Twov8h_POST:
    case AArch64::LD3Threev8h:
    case AArch64::LD3Threev8h_POST:
    case AArch64::LD4Fourv8h:
    case AArch64::LD4Fourv8h_POST:
      numElts = 8;
      eltSize = 16;
      fullWidth = true;
      break;
    case AArch64::LD1Onev2s:
    case AArch64::LD1Onev2s_POST:
    case AArch64::LD1Twov2s:
    case AArch64::LD1Twov2s_POST:
    case AArch64::LD1Threev2s:
    case AArch64::LD1Threev2s_POST:
    case AArch64::LD1Fourv2s:
    case AArch64::LD1Fourv2s_POST:
    case AArch64::LD2Twov2s:
    case AArch64::LD2Twov2s_POST:
    case AArch64::LD3Threev2s:
    case AArch64::LD3Threev2s_POST:
    case AArch64::LD4Fourv2s:
    case AArch64::LD4Fourv2s_POST:
      numElts = 2;
      eltSize = 32;
      fullWidth = false;
      break;
    case AArch64::LD1Onev4s:
    case AArch64::LD1Onev4s_POST:
    case AArch64::LD1Twov4s:
    case AArch64::LD1Twov4s_POST:
    case AArch64::LD1Threev4s:
    case AArch64::LD1Threev4s_POST:
    case AArch64::LD1Fourv4s:
    case AArch64::LD1Fourv4s_POST:
    case AArch64::LD2Twov4s:
    case AArch64::LD2Twov4s_POST:
    case AArch64::LD3Threev4s:
    case AArch64::LD3Threev4s_POST:
    case AArch64::LD4Fourv4s:
    case AArch64::LD4Fourv4s_POST:
      numElts = 4;
      eltSize = 32;
      fullWidth = true;
      break;
    case AArch64::LD1Onev1d:
    case AArch64::LD1Onev1d_POST:
    case AArch64::LD1Twov1d:
    case AArch64::LD1Twov1d_POST:
    case AArch64::LD1Threev1d:
    case AArch64::LD1Threev1d_POST:
    case AArch64::LD1Fourv1d:
    case AArch64::LD1Fourv1d_POST:
      numElts = 1;
      eltSize = 64;
      fullWidth = false;
      break;
    case AArch64::LD1Onev2d:
    case AArch64::LD1Onev2d_POST:
    case AArch64::LD1Twov2d:
    case AArch64::LD1Twov2d_POST:
    case AArch64::LD1Threev2d:
    case AArch64::LD1Threev2d_POST:
    case AArch64::LD1Fourv2d:
    case AArch64::LD1Fourv2d_POST:
    case AArch64::LD2Twov2d:
    case AArch64::LD2Twov2d_POST:
    case AArch64::LD3Threev2d:
    case AArch64::LD3Threev2d_POST:
    case AArch64::LD4Fourv2d:
    case AArch64::LD4Fourv2d_POST:
      numElts = 2;
      eltSize = 64;
      fullWidth = true;
      break;
    default:
      assert(false);
      break;
    }

    unsigned nregs;
    switch (opcode) {
    case AArch64::LD1Onev8b:
    case AArch64::LD1Onev16b:
    case AArch64::LD1Onev4h:
    case AArch64::LD1Onev8h:
    case AArch64::LD1Onev2s:
    case AArch64::LD1Onev4s:
    case AArch64::LD1Onev1d:
    case AArch64::LD1Onev2d:
    case AArch64::LD1Onev8b_POST:
    case AArch64::LD1Onev16b_POST:
    case AArch64::LD1Onev4h_POST:
    case AArch64::LD1Onev8h_POST:
    case AArch64::LD1Onev2s_POST:
    case AArch64::LD1Onev4s_POST:
    case AArch64::LD1Onev1d_POST:
    case AArch64::LD1Onev2d_POST:
      nregs = 1;
      break;
    case AArch64::LD1Twov8b:
    case AArch64::LD1Twov16b:
    case AArch64::LD1Twov4h:
    case AArch64::LD1Twov8h:
    case AArch64::LD1Twov2s:
    case AArch64::LD1Twov4s:
    case AArch64::LD1Twov1d:
    case AArch64::LD1Twov2d:
    case AArch64::LD1Twov8b_POST:
    case AArch64::LD1Twov16b_POST:
    case AArch64::LD1Twov4h_POST:
    case AArch64::LD1Twov8h_POST:
    case AArch64::LD1Twov2s_POST:
    case AArch64::LD1Twov4s_POST:
    case AArch64::LD1Twov1d_POST:
    case AArch64::LD1Twov2d_POST:
    case AArch64::LD2Twov8b:
    case AArch64::LD2Twov16b:
    case AArch64::LD2Twov4h:
    case AArch64::LD2Twov8h:
    case AArch64::LD2Twov2s:
    case AArch64::LD2Twov4s:
    case AArch64::LD2Twov2d:
    case AArch64::LD2Twov8b_POST:
    case AArch64::LD2Twov16b_POST:
    case AArch64::LD2Twov4h_POST:
    case AArch64::LD2Twov8h_POST:
    case AArch64::LD2Twov2s_POST:
    case AArch64::LD2Twov4s_POST:
    case AArch64::LD2Twov2d_POST:
      nregs = 2;
      break;
    case AArch64::LD1Threev8b:
    case AArch64::LD1Threev16b:
    case AArch64::LD1Threev4h:
    case AArch64::LD1Threev8h:
    case AArch64::LD1Threev2s:
    case AArch64::LD1Threev4s:
    case AArch64::LD1Threev1d:
    case AArch64::LD1Threev2d:
    case AArch64::LD1Threev8b_POST:
    case AArch64::LD1Threev16b_POST:
    case AArch64::LD1Threev4h_POST:
    case AArch64::LD1Threev8h_POST:
    case AArch64::LD1Threev2s_POST:
    case AArch64::LD1Threev4s_POST:
    case AArch64::LD1Threev1d_POST:
    case AArch64::LD1Threev2d_POST:
    case AArch64::LD3Threev8b:
    case AArch64::LD3Threev16b:
    case AArch64::LD3Threev4h:
    case AArch64::LD3Threev8h:
    case AArch64::LD3Threev2s:
    case AArch64::LD3Threev4s:
    case AArch64::LD3Threev2d:
    case AArch64::LD3Threev8b_POST:
    case AArch64::LD3Threev16b_POST:
    case AArch64::LD3Threev4h_POST:
    case AArch64::LD3Threev8h_POST:
    case AArch64::LD3Threev2s_POST:
    case AArch64::LD3Threev4s_POST:
    case AArch64::LD3Threev2d_POST:
      nregs = 3;
      break;
    case AArch64::LD1Fourv8b:
    case AArch64::LD1Fourv16b:
    case AArch64::LD1Fourv4h:
    case AArch64::LD1Fourv8h:
    case AArch64::LD1Fourv2s:
    case AArch64::LD1Fourv4s:
    case AArch64::LD1Fourv1d:
    case AArch64::LD1Fourv2d:
    case AArch64::LD1Fourv8b_POST:
    case AArch64::LD1Fourv16b_POST:
    case AArch64::LD1Fourv4h_POST:
    case AArch64::LD1Fourv8h_POST:
    case AArch64::LD1Fourv2s_POST:
    case AArch64::LD1Fourv4s_POST:
    case AArch64::LD1Fourv1d_POST:
    case AArch64::LD1Fourv2d_POST:
    case AArch64::LD4Fourv8b:
    case AArch64::LD4Fourv16b:
    case AArch64::LD4Fourv4h:
    case AArch64::LD4Fourv8h:
    case AArch64::LD4Fourv2s:
    case AArch64::LD4Fourv4s:
    case AArch64::LD4Fourv2d:
    case AArch64::LD4Fourv8b_POST:
    case AArch64::LD4Fourv16b_POST:
    case AArch64::LD4Fourv4h_POST:
    case AArch64::LD4Fourv8h_POST:
    case AArch64::LD4Fourv2s_POST:
    case AArch64::LD4Fourv4s_POST:
    case AArch64::LD4Fourv2d_POST:
      nregs = 4;
      break;
    default:
      assert(false);
      break;
    }
    bool isPost = opcode == AArch64::LD1Onev8b_POST ||
                  opcode == AArch64::LD1Onev16b_POST ||
                  opcode == AArch64::LD1Onev4h_POST ||
                  opcode == AArch64::LD1Onev8h_POST ||
                  opcode == AArch64::LD1Onev2s_POST ||
                  opcode == AArch64::LD1Onev4s_POST ||
                  opcode == AArch64::LD1Onev1d_POST ||
                  opcode == AArch64::LD1Onev2d_POST ||
                  opcode == AArch64::LD1Twov8b_POST ||
                  opcode == AArch64::LD1Twov16b_POST ||
                  opcode == AArch64::LD1Twov4h_POST ||
                  opcode == AArch64::LD1Twov8h_POST ||
                  opcode == AArch64::LD1Twov2s_POST ||
                  opcode == AArch64::LD1Twov4s_POST ||
                  opcode == AArch64::LD1Twov1d_POST ||
                  opcode == AArch64::LD1Twov2d_POST ||
                  opcode == AArch64::LD1Threev8b_POST ||
                  opcode == AArch64::LD1Threev16b_POST ||
                  opcode == AArch64::LD1Threev4h_POST ||
                  opcode == AArch64::LD1Threev8h_POST ||
                  opcode == AArch64::LD1Threev2s_POST ||
                  opcode == AArch64::LD1Threev4s_POST ||
                  opcode == AArch64::LD1Threev1d_POST ||
                  opcode == AArch64::LD1Threev2d_POST ||
                  opcode == AArch64::LD1Fourv8b_POST ||
                  opcode == AArch64::LD1Fourv16b_POST ||
                  opcode == AArch64::LD1Fourv4h_POST ||
                  opcode == AArch64::LD1Fourv8h_POST ||
                  opcode == AArch64::LD1Fourv2s_POST ||
                  opcode == AArch64::LD1Fourv4s_POST ||
                  opcode == AArch64::LD1Fourv1d_POST ||
                  opcode == AArch64::LD1Fourv2d_POST ||
                  opcode == AArch64::LD2Twov8b_POST ||
                  opcode == AArch64::LD2Twov16b_POST ||
                  opcode == AArch64::LD2Twov4h_POST ||
                  opcode == AArch64::LD2Twov8h_POST ||
                  opcode == AArch64::LD2Twov2s_POST ||
                  opcode == AArch64::LD2Twov4s_POST ||
                  opcode == AArch64::LD2Twov2d_POST ||
                  opcode == AArch64::LD3Threev8b_POST ||
                  opcode == AArch64::LD3Threev16b_POST ||
                  opcode == AArch64::LD3Threev4h_POST ||
                  opcode == AArch64::LD3Threev8h_POST ||
                  opcode == AArch64::LD3Threev2s_POST ||
                  opcode == AArch64::LD3Threev4s_POST ||
                  opcode == AArch64::LD3Threev2d_POST ||
                  opcode == AArch64::LD4Fourv8b_POST ||
                  opcode == AArch64::LD4Fourv16b_POST ||
                  opcode == AArch64::LD4Fourv4h_POST ||
                  opcode == AArch64::LD4Fourv8h_POST ||
                  opcode == AArch64::LD4Fourv2s_POST ||
                  opcode == AArch64::LD4Fourv4s_POST ||
                  opcode == AArch64::LD4Fourv2d_POST;

    auto regCounter =
        decodeRegSet(CurInst->getOperand(isPost ? 1 : 0).getReg());
    auto baseReg = CurInst->getOperand(isPost ? 2 : 1).getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    assert((regCounter >= AArch64::Q0 && regCounter <= AArch64::Q31) ||
           (regCounter >= AArch64::D0 && regCounter <= AArch64::D31));
    Value *offset = nullptr;
    if (isPost) {
      if (CurInst->getOperand(3).isReg() &&
          CurInst->getOperand(3).getReg() != AArch64::XZR) {
        offset = readFromRegOld(CurInst->getOperand(3).getReg());
      } else {
        offset = getUnsignedIntConst(nregs * numElts * (eltSize / 8), 64);
      }
    }

    // Load the entire memory block to be stored in all the registers at once
    // [nregs * numElts * (eltSize / 8)] bytes
    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);
    auto loaded = makeLoadWithOffset(base, 0, nregs * numElts * (eltSize / 8));
    auto casted = createBitCast(loaded, getVecTy(eltSize, nregs * numElts));

    // Deinterleave the loaded vector into nregs vectors and store them in the
    // registers
    vector<int> mask(numElts);
    Value *res;

    // Outer loop control for register to store into
    for (unsigned j = 0; j < nregs; j++) {
      for (unsigned i = 0, maskVal = j; i < numElts; i++, maskVal += nregs) {
        assert(maskVal < nregs * numElts);
        mask[i] = maskVal;
      }
      res = createShuffleVector(casted, mask);
      updateReg(res, regCounter);

      regCounter++;
      if (fullWidth && regCounter > AArch64::Q31)
        regCounter = AArch64::Q0;
      else if (!fullWidth && regCounter > AArch64::D31)
        regCounter = AArch64::D0;
    }

    if (isPost) {
      auto added = createAdd(baseAddr, offset);
      updateOutputReg(added);
    }

    break;
  }

  case AArch64::STURBBi:
  case AArch64::STURBi:
  case AArch64::STURHHi:
  case AArch64::STURHi:
  case AArch64::STURWi:
  case AArch64::STURSi:
  case AArch64::STURXi:
  case AArch64::STURDi:
  case AArch64::STURQi:
  case AArch64::STRBBui:
  case AArch64::STRBui:
  case AArch64::STRHHui:
  case AArch64::STRHui:
  case AArch64::STRWui:
  case AArch64::STRSui:
  case AArch64::STRXui:
  case AArch64::STRDui:
  case AArch64::STRQui: {
    auto [base, imm, val] = getStoreParams();

    bool isScaled = opcode == AArch64::STRBBui || opcode == AArch64::STRBui ||
                    opcode == AArch64::STRHHui || opcode == AArch64::STRHui ||
                    opcode == AArch64::STRWui || opcode == AArch64::STRSui ||
                    opcode == AArch64::STRXui || opcode == AArch64::STRDui ||
                    opcode == AArch64::STRQui;

    switch (opcode) {
    case AArch64::STURBBi:
    case AArch64::STURBi:
    case AArch64::STRBBui:
    case AArch64::STRBui: {
      storeToMemoryImmOffset(base, isScaled ? imm * 1 : imm, 1,
                             createTrunc(val, i8));
      break;
    }
    case AArch64::STURHHi:
    case AArch64::STURHi:
    case AArch64::STRHHui:
    case AArch64::STRHui: {
      storeToMemoryImmOffset(base, isScaled ? imm * 2 : imm, 2,
                             createTrunc(val, i16));
      break;
    }
    case AArch64::STURWi:
    case AArch64::STURSi:
    case AArch64::STRWui:
    case AArch64::STRSui: {
      storeToMemoryImmOffset(base, isScaled ? imm * 4 : imm, 4,
                             createTrunc(val, i32));
      break;
    }
    case AArch64::STURXi:
    case AArch64::STRXui: {
      storeToMemoryImmOffset(base, isScaled ? imm * 8 : imm, 8, val);
      break;
    }
    case AArch64::STURDi:
    case AArch64::STRDui: {
      storeToMemoryImmOffset(base, isScaled ? imm * 8 : imm, 8,
                             createTrunc(val, i64));
      break;
    }
    case AArch64::STURQi:
    case AArch64::STRQui: {
      storeToMemoryImmOffset(base, isScaled ? imm * 16 : imm, 16, val);
      break;
    }
    default: {
      assert(false);
      break;
    }
    }
    break;
  }

  case AArch64::STRBBpre:
  case AArch64::STRBpre:
  case AArch64::STRHHpre:
  case AArch64::STRHpre:
  case AArch64::STRWpre:
  case AArch64::STRSpre:
  case AArch64::STRXpre:
  case AArch64::STRDpre:
  case AArch64::STRQpre:
  case AArch64::STRBBpost:
  case AArch64::STRBpost:
  case AArch64::STRHHpost:
  case AArch64::STRHpost:
  case AArch64::STRWpost:
  case AArch64::STRSpost:
  case AArch64::STRXpost:
  case AArch64::STRDpost:
  case AArch64::STRQpost: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    assert(op0.isReg() && op1.isReg() && op2.isReg());
    assert(op0.getReg() == op2.getReg());
    assert(op3.isImm());

    // For pre and post index memory instructions, the destination register
    // is at position 1
    auto srcReg = op1.getReg();
    auto baseReg = op2.getReg();
    auto imm = op3.getImm();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    unsigned size;
    Value *loaded = nullptr;
    switch (opcode) {
    case AArch64::STRBBpre:
    case AArch64::STRBpre:
    case AArch64::STRBBpost:
    case AArch64::STRBpost:
      size = 1;
      loaded = readFromRegTyped(srcReg, i8);
      break;
    case AArch64::STRHHpre:
    case AArch64::STRHpre:
    case AArch64::STRHHpost:
    case AArch64::STRHpost:
      size = 2;
      loaded = readFromRegTyped(srcReg, i16);
      break;
    case AArch64::STRWpre:
    case AArch64::STRSpre:
    case AArch64::STRWpost:
    case AArch64::STRSpost:
      size = 4;
      loaded = readFromRegTyped(srcReg, i32);
      break;
    case AArch64::STRXpre:
    case AArch64::STRXpost:
      size = 8;
      loaded = readFromRegTyped(srcReg, i64);
      break;
    case AArch64::STRDpre:
    case AArch64::STRDpost:
      size = 8;
      loaded = readFromRegTyped(srcReg, i64);
      break;
    case AArch64::STRQpre:
    case AArch64::STRQpost:
      size = 16;
      loaded = readFromRegTyped(srcReg, getIntTy(128));
      break;
    default:
      assert(false);
    }

    // Start offset as a 9-bit signed integer
    assert(imm <= 255 && imm >= -256);
    auto offset = getSignedIntConst(imm, 9);
    Value *offsetVal = createSExt(offset, i64);
    Value *zeroVal = getUnsignedIntConst(0, 64);

    bool isPre = opcode == AArch64::STRBBpre || opcode == AArch64::STRBpre ||
                 opcode == AArch64::STRHHpre || opcode == AArch64::STRHpre ||
                 opcode == AArch64::STRWpre || opcode == AArch64::STRSpre ||
                 opcode == AArch64::STRXpre || opcode == AArch64::STRDpre ||
                 opcode == AArch64::STRQpre;

    storeToMemoryValOffset(base, isPre ? offsetVal : zeroVal, size, loaded);

    auto added = createAdd(baseAddr, offsetVal);
    updateOutputReg(added);
    break;
  }

  case AArch64::STRBBroW:
  case AArch64::STRBBroX:
  case AArch64::STRBroW:
  case AArch64::STRBroX:
  case AArch64::STRHHroW:
  case AArch64::STRHHroX:
  case AArch64::STRHroW:
  case AArch64::STRHroX:
  case AArch64::STRWroW:
  case AArch64::STRWroX:
  case AArch64::STRSroW:
  case AArch64::STRSroX:
  case AArch64::STRXroW:
  case AArch64::STRXroX:
  case AArch64::STRDroW:
  case AArch64::STRDroX:
  case AArch64::STRQroW:
  case AArch64::STRQroX: {
    auto [base, offset, val] = getParamsStoreReg();

    switch (opcode) {
    case AArch64::STRBBroW:
    case AArch64::STRBBroX:
    case AArch64::STRBroW:
    case AArch64::STRBroX:
      storeToMemoryValOffset(base, offset, 1, createTrunc(val, i8));
      break;
    case AArch64::STRHHroW:
    case AArch64::STRHHroX:
    case AArch64::STRHroW:
    case AArch64::STRHroX:
      storeToMemoryValOffset(base, offset, 2, createTrunc(val, i16));
      break;
    case AArch64::STRWroW:
    case AArch64::STRWroX:
    case AArch64::STRSroW:
    case AArch64::STRSroX:
      storeToMemoryValOffset(base, offset, 4, createTrunc(val, i32));
      break;
    case AArch64::STRXroW:
    case AArch64::STRXroX:
      storeToMemoryValOffset(base, offset, 8, val);
      break;
    case AArch64::STRDroW:
    case AArch64::STRDroX:
      storeToMemoryValOffset(base, offset, 8, createTrunc(val, i64));
      break;
    case AArch64::STRQroW:
    case AArch64::STRQroX:
      storeToMemoryValOffset(base, offset, 16, val);
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
    }
    break;
  }

  case AArch64::ST1i8:
  case AArch64::ST1i16:
  case AArch64::ST1i32:
  case AArch64::ST1i64:
  case AArch64::ST1i8_POST:
  case AArch64::ST1i16_POST:
  case AArch64::ST1i32_POST:
  case AArch64::ST1i64_POST: {
    unsigned numElts, eltSize;

    switch (opcode) {
    case AArch64::ST1i8:
    case AArch64::ST1i8_POST:
      numElts = 16;
      eltSize = 8;
      break;
    case AArch64::ST1i16:
    case AArch64::ST1i16_POST:
      numElts = 8;
      eltSize = 16;
      break;
    case AArch64::ST1i32:
    case AArch64::ST1i32_POST:
      numElts = 4;
      eltSize = 32;
      break;
    case AArch64::ST1i64:
    case AArch64::ST1i64_POST:
      numElts = 2;
      eltSize = 64;
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }

    unsigned nregs;
    switch (opcode) {
    case AArch64::ST1i8:
    case AArch64::ST1i16:
    case AArch64::ST1i32:
    case AArch64::ST1i64:
    case AArch64::ST1i8_POST:
    case AArch64::ST1i16_POST:
    case AArch64::ST1i32_POST:
    case AArch64::ST1i64_POST:
      nregs = 1;
      break;
    default:
      assert(false);
      break;
    }
    bool isPost =
        opcode == AArch64::ST1i8_POST || opcode == AArch64::ST1i16_POST ||
        opcode == AArch64::ST1i32_POST || opcode == AArch64::ST1i64_POST;

    auto regCounter =
        decodeRegSet(CurInst->getOperand(isPost ? 1 : 0).getReg());
    auto index = getImm(isPost ? 2 : 1);
    auto baseReg = CurInst->getOperand(isPost ? 3 : 2).getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    assert((regCounter >= AArch64::Q0 && regCounter <= AArch64::Q31));

    Value *offset = nullptr;
    if (isPost) {
      if (CurInst->getOperand(4).isReg() &&
          CurInst->getOperand(4).getReg() != AArch64::XZR) {
        offset = readFromRegOld(CurInst->getOperand(4).getReg());
      } else {
        offset = getUnsignedIntConst(nregs * (eltSize / 8), 64);
      }
    }

    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    Value *valueToStore = getUndefVec(nregs, eltSize);
    // Outer loop control for register to load from
    for (unsigned j = 0; j < nregs; j++) {
      Value *registerjValue = readFromRegOld(regCounter);

      auto casted = createBitCast(registerjValue, getVecTy(eltSize, numElts));
      auto loaded = createExtractElement(casted, index);

      valueToStore = createInsertElement(valueToStore, loaded, j);

      regCounter++;
      if (regCounter > AArch64::Q31)
        regCounter = AArch64::Q0;
    }

    storeToMemoryImmOffset(base, 0, nregs * eltSize / 8, valueToStore);

    if (isPost) {
      auto added = createAdd(baseAddr, offset);
      updateOutputReg(added);
    }
    break;
  }

  case AArch64::ST1Onev8b:
  case AArch64::ST1Onev16b:
  case AArch64::ST1Onev4h:
  case AArch64::ST1Onev8h:
  case AArch64::ST1Onev2s:
  case AArch64::ST1Onev4s:
  case AArch64::ST1Onev1d:
  case AArch64::ST1Onev2d:
  case AArch64::ST1Onev8b_POST:
  case AArch64::ST1Onev16b_POST:
  case AArch64::ST1Onev4h_POST:
  case AArch64::ST1Onev8h_POST:
  case AArch64::ST1Onev2s_POST:
  case AArch64::ST1Onev4s_POST:
  case AArch64::ST1Onev1d_POST:
  case AArch64::ST1Onev2d_POST:
  case AArch64::ST1Twov1d:
  case AArch64::ST1Threev1d:
  case AArch64::ST1Threev2d:
  case AArch64::ST1Twov2d:
  case AArch64::ST1Fourv1d:
  case AArch64::ST1Fourv2d:
  case AArch64::ST2Twov8b:
  case AArch64::ST2Twov16b:
  case AArch64::ST2Twov4h:
  case AArch64::ST2Twov8h:
  case AArch64::ST2Twov2s:
  case AArch64::ST2Twov4s:
  case AArch64::ST2Twov2d:
  case AArch64::ST2Twov8b_POST:
  case AArch64::ST2Twov16b_POST:
  case AArch64::ST2Twov4h_POST:
  case AArch64::ST2Twov8h_POST:
  case AArch64::ST2Twov2s_POST:
  case AArch64::ST2Twov4s_POST:
  case AArch64::ST2Twov2d_POST:
  case AArch64::ST3Threev8b:
  case AArch64::ST3Threev16b:
  case AArch64::ST3Threev4h:
  case AArch64::ST3Threev8h:
  case AArch64::ST3Threev2s:
  case AArch64::ST3Threev4s:
  case AArch64::ST3Threev2d:
  case AArch64::ST3Threev8b_POST:
  case AArch64::ST3Threev16b_POST:
  case AArch64::ST3Threev4h_POST:
  case AArch64::ST3Threev8h_POST:
  case AArch64::ST3Threev2s_POST:
  case AArch64::ST3Threev4s_POST:
  case AArch64::ST3Threev2d_POST:
  case AArch64::ST4Fourv8b:
  case AArch64::ST4Fourv16b:
  case AArch64::ST4Fourv4h:
  case AArch64::ST4Fourv8h:
  case AArch64::ST4Fourv2s:
  case AArch64::ST4Fourv4s:
  case AArch64::ST4Fourv2d:
  case AArch64::ST4Fourv8b_POST:
  case AArch64::ST4Fourv16b_POST:
  case AArch64::ST4Fourv4h_POST:
  case AArch64::ST4Fourv8h_POST:
  case AArch64::ST4Fourv2s_POST:
  case AArch64::ST4Fourv4s_POST:
  case AArch64::ST4Fourv2d_POST: {
    unsigned numElts, eltSize;
    bool fullWidth;
    switch (opcode) {
    case AArch64::ST1Onev8b:
    case AArch64::ST1Onev8b_POST:
    case AArch64::ST1Twov8b:
    case AArch64::ST1Twov8b_POST:
    case AArch64::ST1Threev8b:
    case AArch64::ST1Threev8b_POST:
    case AArch64::ST1Fourv8b:
    case AArch64::ST1Fourv8b_POST:
    case AArch64::ST2Twov8b:
    case AArch64::ST2Twov8b_POST:
    case AArch64::ST3Threev8b:
    case AArch64::ST3Threev8b_POST:
    case AArch64::ST4Fourv8b:
    case AArch64::ST4Fourv8b_POST:
      numElts = 8;
      eltSize = 8;
      fullWidth = false;
      break;
    case AArch64::ST1Onev16b:
    case AArch64::ST1Onev16b_POST:
    case AArch64::ST1Twov16b:
    case AArch64::ST1Twov16b_POST:
    case AArch64::ST1Threev16b:
    case AArch64::ST1Threev16b_POST:
    case AArch64::ST1Fourv16b:
    case AArch64::ST1Fourv16b_POST:
    case AArch64::ST2Twov16b:
    case AArch64::ST2Twov16b_POST:
    case AArch64::ST3Threev16b:
    case AArch64::ST3Threev16b_POST:
    case AArch64::ST4Fourv16b:
    case AArch64::ST4Fourv16b_POST:
      numElts = 16;
      eltSize = 8;
      fullWidth = true;
      break;
    case AArch64::ST1Onev4h:
    case AArch64::ST1Onev4h_POST:
    case AArch64::ST1Twov4h:
    case AArch64::ST1Twov4h_POST:
    case AArch64::ST1Threev4h:
    case AArch64::ST1Threev4h_POST:
    case AArch64::ST1Fourv4h:
    case AArch64::ST1Fourv4h_POST:
    case AArch64::ST2Twov4h:
    case AArch64::ST2Twov4h_POST:
    case AArch64::ST3Threev4h:
    case AArch64::ST3Threev4h_POST:
    case AArch64::ST4Fourv4h:
    case AArch64::ST4Fourv4h_POST:
      numElts = 4;
      eltSize = 16;
      fullWidth = false;
      break;
    case AArch64::ST1Onev8h:
    case AArch64::ST1Onev8h_POST:
    case AArch64::ST1Twov8h:
    case AArch64::ST1Twov8h_POST:
    case AArch64::ST1Threev8h:
    case AArch64::ST1Threev8h_POST:
    case AArch64::ST1Fourv8h:
    case AArch64::ST1Fourv8h_POST:
    case AArch64::ST2Twov8h:
    case AArch64::ST2Twov8h_POST:
    case AArch64::ST3Threev8h:
    case AArch64::ST3Threev8h_POST:
    case AArch64::ST4Fourv8h:
    case AArch64::ST4Fourv8h_POST:
      numElts = 8;
      eltSize = 16;
      fullWidth = true;
      break;
    case AArch64::ST1Onev2s:
    case AArch64::ST1Onev2s_POST:
    case AArch64::ST1Twov2s:
    case AArch64::ST1Twov2s_POST:
    case AArch64::ST1Threev2s:
    case AArch64::ST1Threev2s_POST:
    case AArch64::ST1Fourv2s:
    case AArch64::ST1Fourv2s_POST:
    case AArch64::ST2Twov2s:
    case AArch64::ST2Twov2s_POST:
    case AArch64::ST3Threev2s:
    case AArch64::ST3Threev2s_POST:
    case AArch64::ST4Fourv2s:
    case AArch64::ST4Fourv2s_POST:
      numElts = 2;
      eltSize = 32;
      fullWidth = false;
      break;
    case AArch64::ST1Onev4s:
    case AArch64::ST1Onev4s_POST:
    case AArch64::ST1Twov4s:
    case AArch64::ST1Twov4s_POST:
    case AArch64::ST1Threev4s:
    case AArch64::ST1Threev4s_POST:
    case AArch64::ST1Fourv4s:
    case AArch64::ST1Fourv4s_POST:
    case AArch64::ST2Twov4s:
    case AArch64::ST2Twov4s_POST:
    case AArch64::ST3Threev4s:
    case AArch64::ST3Threev4s_POST:
    case AArch64::ST4Fourv4s:
    case AArch64::ST4Fourv4s_POST:
      numElts = 4;
      eltSize = 32;
      fullWidth = true;
      break;
    case AArch64::ST1Onev1d:
    case AArch64::ST1Onev1d_POST:
    case AArch64::ST1Twov1d:
    case AArch64::ST1Twov1d_POST:
    case AArch64::ST1Threev1d:
    case AArch64::ST1Threev1d_POST:
    case AArch64::ST1Fourv1d:
    case AArch64::ST1Fourv1d_POST:
      numElts = 1;
      eltSize = 64;
      fullWidth = false;
      break;
    case AArch64::ST1Onev2d:
    case AArch64::ST1Onev2d_POST:
    case AArch64::ST1Twov2d:
    case AArch64::ST1Twov2d_POST:
    case AArch64::ST1Threev2d:
    case AArch64::ST1Threev2d_POST:
    case AArch64::ST1Fourv2d:
    case AArch64::ST1Fourv2d_POST:
    case AArch64::ST2Twov2d:
    case AArch64::ST2Twov2d_POST:
    case AArch64::ST3Threev2d:
    case AArch64::ST3Threev2d_POST:
    case AArch64::ST4Fourv2d:
    case AArch64::ST4Fourv2d_POST:
      numElts = 2;
      eltSize = 64;
      fullWidth = true;
      break;
    default:
      assert(false);
      break;
    }

    unsigned nregs;
    switch (opcode) {
    case AArch64::ST1Onev8b:
    case AArch64::ST1Onev16b:
    case AArch64::ST1Onev4h:
    case AArch64::ST1Onev8h:
    case AArch64::ST1Onev2s:
    case AArch64::ST1Onev4s:
    case AArch64::ST1Onev1d:
    case AArch64::ST1Onev2d:
    case AArch64::ST1Onev8b_POST:
    case AArch64::ST1Onev16b_POST:
    case AArch64::ST1Onev4h_POST:
    case AArch64::ST1Onev8h_POST:
    case AArch64::ST1Onev2s_POST:
    case AArch64::ST1Onev4s_POST:
    case AArch64::ST1Onev1d_POST:
    case AArch64::ST1Onev2d_POST:
      nregs = 1;
      break;
    case AArch64::ST1Twov8b:
    case AArch64::ST1Twov16b:
    case AArch64::ST1Twov4h:
    case AArch64::ST1Twov8h:
    case AArch64::ST1Twov2s:
    case AArch64::ST1Twov4s:
    case AArch64::ST1Twov1d:
    case AArch64::ST1Twov2d:
    case AArch64::ST1Twov8b_POST:
    case AArch64::ST1Twov16b_POST:
    case AArch64::ST1Twov4h_POST:
    case AArch64::ST1Twov8h_POST:
    case AArch64::ST1Twov2s_POST:
    case AArch64::ST1Twov4s_POST:
    case AArch64::ST1Twov1d_POST:
    case AArch64::ST1Twov2d_POST:
    case AArch64::ST2Twov8b:
    case AArch64::ST2Twov16b:
    case AArch64::ST2Twov4h:
    case AArch64::ST2Twov8h:
    case AArch64::ST2Twov2s:
    case AArch64::ST2Twov4s:
    case AArch64::ST2Twov2d:
    case AArch64::ST2Twov8b_POST:
    case AArch64::ST2Twov16b_POST:
    case AArch64::ST2Twov4h_POST:
    case AArch64::ST2Twov8h_POST:
    case AArch64::ST2Twov2s_POST:
    case AArch64::ST2Twov4s_POST:
    case AArch64::ST2Twov2d_POST:
      nregs = 2;
      break;
    case AArch64::ST1Threev8b:
    case AArch64::ST1Threev16b:
    case AArch64::ST1Threev4h:
    case AArch64::ST1Threev8h:
    case AArch64::ST1Threev2s:
    case AArch64::ST1Threev4s:
    case AArch64::ST1Threev1d:
    case AArch64::ST1Threev2d:
    case AArch64::ST1Threev8b_POST:
    case AArch64::ST1Threev16b_POST:
    case AArch64::ST1Threev4h_POST:
    case AArch64::ST1Threev8h_POST:
    case AArch64::ST1Threev2s_POST:
    case AArch64::ST1Threev4s_POST:
    case AArch64::ST1Threev1d_POST:
    case AArch64::ST1Threev2d_POST:
    case AArch64::ST3Threev8b:
    case AArch64::ST3Threev16b:
    case AArch64::ST3Threev4h:
    case AArch64::ST3Threev8h:
    case AArch64::ST3Threev2s:
    case AArch64::ST3Threev4s:
    case AArch64::ST3Threev2d:
    case AArch64::ST3Threev8b_POST:
    case AArch64::ST3Threev16b_POST:
    case AArch64::ST3Threev4h_POST:
    case AArch64::ST3Threev8h_POST:
    case AArch64::ST3Threev2s_POST:
    case AArch64::ST3Threev4s_POST:
    case AArch64::ST3Threev2d_POST:
      nregs = 3;
      break;
    case AArch64::ST1Fourv8b:
    case AArch64::ST1Fourv16b:
    case AArch64::ST1Fourv4h:
    case AArch64::ST1Fourv8h:
    case AArch64::ST1Fourv2s:
    case AArch64::ST1Fourv4s:
    case AArch64::ST1Fourv1d:
    case AArch64::ST1Fourv2d:
    case AArch64::ST1Fourv8b_POST:
    case AArch64::ST1Fourv16b_POST:
    case AArch64::ST1Fourv4h_POST:
    case AArch64::ST1Fourv8h_POST:
    case AArch64::ST1Fourv2s_POST:
    case AArch64::ST1Fourv4s_POST:
    case AArch64::ST1Fourv1d_POST:
    case AArch64::ST1Fourv2d_POST:
    case AArch64::ST4Fourv8b:
    case AArch64::ST4Fourv16b:
    case AArch64::ST4Fourv4h:
    case AArch64::ST4Fourv8h:
    case AArch64::ST4Fourv2s:
    case AArch64::ST4Fourv4s:
    case AArch64::ST4Fourv2d:
    case AArch64::ST4Fourv8b_POST:
    case AArch64::ST4Fourv16b_POST:
    case AArch64::ST4Fourv4h_POST:
    case AArch64::ST4Fourv8h_POST:
    case AArch64::ST4Fourv2s_POST:
    case AArch64::ST4Fourv4s_POST:
    case AArch64::ST4Fourv2d_POST:
      nregs = 4;
      break;
    default:
      assert(false);
      break;
    }
    bool isPost = opcode == AArch64::ST1Onev8b_POST ||
                  opcode == AArch64::ST1Onev16b_POST ||
                  opcode == AArch64::ST1Onev4h_POST ||
                  opcode == AArch64::ST1Onev8h_POST ||
                  opcode == AArch64::ST1Onev2s_POST ||
                  opcode == AArch64::ST1Onev4s_POST ||
                  opcode == AArch64::ST1Onev1d_POST ||
                  opcode == AArch64::ST1Onev2d_POST ||
                  opcode == AArch64::ST1Twov8b_POST ||
                  opcode == AArch64::ST1Twov16b_POST ||
                  opcode == AArch64::ST1Twov4h_POST ||
                  opcode == AArch64::ST1Twov8h_POST ||
                  opcode == AArch64::ST1Twov2s_POST ||
                  opcode == AArch64::ST1Twov4s_POST ||
                  opcode == AArch64::ST1Twov1d_POST ||
                  opcode == AArch64::ST1Twov2d_POST ||
                  opcode == AArch64::ST1Threev8b_POST ||
                  opcode == AArch64::ST1Threev16b_POST ||
                  opcode == AArch64::ST1Threev4h_POST ||
                  opcode == AArch64::ST1Threev8h_POST ||
                  opcode == AArch64::ST1Threev2s_POST ||
                  opcode == AArch64::ST1Threev4s_POST ||
                  opcode == AArch64::ST1Threev1d_POST ||
                  opcode == AArch64::ST1Threev2d_POST ||
                  opcode == AArch64::ST1Fourv8b_POST ||
                  opcode == AArch64::ST1Fourv16b_POST ||
                  opcode == AArch64::ST1Fourv4h_POST ||
                  opcode == AArch64::ST1Fourv8h_POST ||
                  opcode == AArch64::ST1Fourv2s_POST ||
                  opcode == AArch64::ST1Fourv4s_POST ||
                  opcode == AArch64::ST1Fourv1d_POST ||
                  opcode == AArch64::ST1Fourv2d_POST ||
                  opcode == AArch64::ST2Twov8b_POST ||
                  opcode == AArch64::ST2Twov16b_POST ||
                  opcode == AArch64::ST2Twov4h_POST ||
                  opcode == AArch64::ST2Twov8h_POST ||
                  opcode == AArch64::ST2Twov2s_POST ||
                  opcode == AArch64::ST2Twov4s_POST ||
                  opcode == AArch64::ST2Twov2d_POST ||
                  opcode == AArch64::ST3Threev8b_POST ||
                  opcode == AArch64::ST3Threev16b_POST ||
                  opcode == AArch64::ST3Threev4h_POST ||
                  opcode == AArch64::ST3Threev8h_POST ||
                  opcode == AArch64::ST3Threev2s_POST ||
                  opcode == AArch64::ST3Threev4s_POST ||
                  opcode == AArch64::ST3Threev2d_POST ||
                  opcode == AArch64::ST4Fourv8b_POST ||
                  opcode == AArch64::ST4Fourv16b_POST ||
                  opcode == AArch64::ST4Fourv4h_POST ||
                  opcode == AArch64::ST4Fourv8h_POST ||
                  opcode == AArch64::ST4Fourv2s_POST ||
                  opcode == AArch64::ST4Fourv4s_POST ||
                  opcode == AArch64::ST4Fourv2d_POST;

    auto regCounter =
        decodeRegSet(CurInst->getOperand(isPost ? 1 : 0).getReg());
    auto baseReg = CurInst->getOperand(isPost ? 2 : 1).getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    assert((regCounter >= AArch64::Q0 && regCounter <= AArch64::Q31) ||
           (regCounter >= AArch64::D0 && regCounter <= AArch64::D31));
    Value *offset = nullptr;
    if (isPost) {
      if (CurInst->getOperand(3).isReg() &&
          CurInst->getOperand(3).getReg() != AArch64::XZR) {
        offset = readFromRegOld(CurInst->getOperand(3).getReg());
      } else {
        offset = getUnsignedIntConst(nregs * numElts * (eltSize / 8), 64);
      }
    }

    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    vector<int> mask(nregs * numElts);

    Value *valueToStore = getUndefVec(nregs, numElts * eltSize);
    // Outer loop control for register to load from
    for (unsigned j = 0; j < nregs; j++) {
      Value *registerjValue = readFromRegOld(regCounter);
      if (!fullWidth) {
        registerjValue = createTrunc(registerjValue, i64);
      }

      valueToStore = createInsertElement(valueToStore, registerjValue, j);

      // Creating mask for the shufflevector
      for (unsigned i = 0, index = j; i < numElts; i++, index += nregs) {
        assert(index < nregs * numElts);
        mask[index] = j * numElts + i;
      }

      regCounter++;
      if (fullWidth && regCounter > AArch64::Q31)
        regCounter = AArch64::Q0;
      else if (!fullWidth && regCounter > AArch64::D31)
        regCounter = AArch64::D0;
    }

    // Shuffle the vector to interleave the nregs vectors
    auto casted =
        createBitCast(valueToStore, getVecTy(eltSize, nregs * numElts));
    Value *res = createShuffleVector(casted, mask);

    storeToMemoryImmOffset(base, 0, nregs * numElts * (eltSize / 8), res);

    if (isPost) {
      auto added = createAdd(baseAddr, offset);
      updateOutputReg(added);
    }

    break;
  }

  case AArch64::LDPSWi:
  case AArch64::LDPWi:
  case AArch64::LDPSi:
  case AArch64::LDPXi:
  case AArch64::LDPDi:
  case AArch64::LDPQi: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    assert(op0.isReg() && op1.isReg() && op2.isReg());
    assert(op3.isImm());

    auto baseReg = op2.getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::XZR) || (baseReg == AArch64::FP));
    auto baseAddr = readPtrFromReg(baseReg);

    int size = 0;
    switch (opcode) {
    case AArch64::LDPSWi:
    case AArch64::LDPWi:
    case AArch64::LDPSi: {
      size = 4;
      break;
    }
    case AArch64::LDPXi:
    case AArch64::LDPDi: {
      size = 8;
      break;
    }
    case AArch64::LDPQi: {
      size = 16;
      break;
    }
    default: {
      *out << "\nError Unknown opcode\n";
      visitError();
    }
    }
    assert(size != 0);

    bool SExt = opcode == AArch64::LDPSWi;

    auto imm = op3.getImm();
    auto out1 = op0.getReg();
    auto out2 = op1.getReg();

    updateReg(makeLoadWithOffset(baseAddr, imm * size, size), out1, SExt);
    updateReg(makeLoadWithOffset(baseAddr, (imm + 1) * size, size), out2, SExt);
    break;
  }

  case AArch64::STPWi:
  case AArch64::STPSi:
  case AArch64::STPXi:
  case AArch64::STPDi:
  case AArch64::STPQi: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    assert(op0.isReg() && op1.isReg() && op2.isReg());
    assert(op3.isImm());

    auto baseReg = op2.getReg();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP));
    auto baseAddr = readPtrFromReg(baseReg);
    auto val1 = readFromRegOld(op0.getReg());
    auto val2 = readFromRegOld(op1.getReg());

    auto imm = op3.getImm();

    uint64_t size = 0;
    switch (opcode) {
    case AArch64::STPWi:
    case AArch64::STPSi: {
      size = 4;
      val1 = createTrunc(val1, i32);
      val2 = createTrunc(val2, i32);
      break;
    }
    case AArch64::STPXi: {
      size = 8;
      break;
    }
    case AArch64::STPDi: {
      size = 8;
      val1 = createTrunc(val1, i64);
      val2 = createTrunc(val2, i64);
      break;
    }
    case AArch64::STPQi: {
      size = 16;
      break;
    }
    default: {
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }
    }
    assert(size != 0);

    storeToMemoryImmOffset(baseAddr, imm * size, size, val1);
    storeToMemoryImmOffset(baseAddr, (imm + 1) * size, size, val2);
    break;
  }

  case AArch64::LDPSWpre:
  case AArch64::LDPWpre:
  case AArch64::LDPSpre:
  case AArch64::LDPXpre:
  case AArch64::LDPDpre:
  case AArch64::LDPQpre:
  case AArch64::LDPSWpost:
  case AArch64::LDPWpost:
  case AArch64::LDPSpost:
  case AArch64::LDPXpost:
  case AArch64::LDPDpost:
  case AArch64::LDPQpost: {
    unsigned scale;
    switch (opcode) {
    case AArch64::LDPSWpre:
    case AArch64::LDPWpre:
    case AArch64::LDPSpre:
    case AArch64::LDPSWpost:
    case AArch64::LDPWpost:
    case AArch64::LDPSpost: {
      scale = 2;
      break;
    }
    case AArch64::LDPXpre:
    case AArch64::LDPDpre:
    case AArch64::LDPXpost:
    case AArch64::LDPDpost: {
      scale = 3;
      break;
    }
    case AArch64::LDPQpre:
    case AArch64::LDPQpost: {
      scale = 4;
      break;
    }
    default: {
      *out << "\nError Unknown opcode\n";
      visitError();
    }
    }

    bool sExt = false;
    switch (opcode) {
    case AArch64::LDPSWpre:
    case AArch64::LDPSWpost:
      sExt = true;
      break;
    default:
      sExt = false;
      break;
    }
    unsigned size = pow(2, scale);
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    auto &op4 = CurInst->getOperand(4);
    assert(op0.isReg() && op1.isReg() && op2.isReg() && op3.isReg());
    assert(op0.getReg() == op3.getReg());
    assert(op4.isImm());

    auto destReg1 = op1.getReg();
    auto destReg2 = op2.getReg();
    auto baseReg = op3.getReg();
    auto imm = op4.getImm();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    // Start offset as 7-bit signed integer
    assert(imm <= 63 && imm >= -64);
    auto offset = getSignedIntConst(imm, 7);
    Value *offsetVal1 = createMaskedShl(createSExt(offset, i64),
                                        getUnsignedIntConst(scale, 64));
    Value *offsetVal2 = createAdd(offsetVal1, getUnsignedIntConst(size, 64));

    bool isPre = opcode == AArch64::LDPWpre || opcode == AArch64::LDPSpre ||
                 opcode == AArch64::LDPXpre || opcode == AArch64::LDPDpre ||
                 opcode == AArch64::LDPQpre;

    Value *loaded1, *loaded2;
    if (isPre) {
      loaded1 = makeLoadWithOffset(base, offsetVal1, size);
      loaded2 = makeLoadWithOffset(base, offsetVal2, size);
    } else {
      loaded1 = makeLoadWithOffset(base, getUnsignedIntConst(0, 64), size);
      loaded2 = makeLoadWithOffset(base, getUnsignedIntConst(size, 64), size);
    }
    updateReg(loaded1, destReg1, sExt);
    updateReg(loaded2, destReg2, sExt);

    auto added = createAdd(baseAddr, offsetVal1);
    updateOutputReg(added);
    break;
  }

  case AArch64::STPWpre:
  case AArch64::STPSpre:
  case AArch64::STPXpre:
  case AArch64::STPDpre:
  case AArch64::STPQpre:
  case AArch64::STPWpost:
  case AArch64::STPSpost:
  case AArch64::STPXpost:
  case AArch64::STPDpost:
  case AArch64::STPQpost: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    auto &op4 = CurInst->getOperand(4);
    assert(op0.isReg() && op1.isReg() && op2.isReg() && op3.isReg());
    assert(op0.getReg() == op3.getReg());
    assert(op4.isImm());

    auto srcReg1 = op1.getReg();
    auto srcReg2 = op2.getReg();
    auto baseReg = op3.getReg();
    auto imm = op4.getImm();
    assert((baseReg >= AArch64::X0 && baseReg <= AArch64::X28) ||
           (baseReg == AArch64::SP) || (baseReg == AArch64::LR) ||
           (baseReg == AArch64::FP) || (baseReg == AArch64::XZR));
    auto base = readPtrFromReg(baseReg);
    auto baseAddr = createPtrToInt(base, i64);

    unsigned scale;
    Value *loaded1, *loaded2;

    switch (opcode) {
    case AArch64::STPWpre:
    case AArch64::STPSpre:
    case AArch64::STPWpost:
    case AArch64::STPSpost: {
      scale = 2;
      loaded1 = createTrunc(readFromRegOld(srcReg1), i32);
      loaded2 = createTrunc(readFromRegOld(srcReg2), i32);
      break;
    }
    case AArch64::STPXpre:
    case AArch64::STPXpost: {
      scale = 3;
      loaded1 = readFromRegOld(srcReg1);
      loaded2 = readFromRegOld(srcReg2);
      break;
    }
    case AArch64::STPDpre:
    case AArch64::STPDpost: {
      scale = 3;
      loaded1 = createTrunc(readFromRegOld(srcReg1), i64);
      loaded2 = createTrunc(readFromRegOld(srcReg2), i64);
      break;
    }
    case AArch64::STPQpre:
    case AArch64::STPQpost: {
      scale = 4;
      loaded1 = readFromRegOld(srcReg1);
      loaded2 = readFromRegOld(srcReg2);
      break;
    }
    default: {
      *out << "\nError Unknown opcode\n";
      visitError();
    }
    }
    unsigned size = pow(2, scale);

    // Start offset as 7-bit signed integer
    assert(imm <= 63 && imm >= -64);
    auto offset = getSignedIntConst(imm, 7);
    Value *offsetVal1 = createMaskedShl(createSExt(offset, i64),
                                        getUnsignedIntConst(scale, 64));
    Value *offsetVal2 = createAdd(offsetVal1, getUnsignedIntConst(size, 64));

    bool isPre = opcode == AArch64::STPWpre || opcode == AArch64::STPSpre ||
                 opcode == AArch64::STPXpre || opcode == AArch64::STPDpre ||
                 opcode == AArch64::STPQpre;

    if (isPre) {
      storeToMemoryValOffset(base, offsetVal1, size, loaded1);
      storeToMemoryValOffset(base, offsetVal2, size, loaded2);
    } else {
      storeToMemoryValOffset(base, getUnsignedIntConst(0, 64), size, loaded1);
      storeToMemoryValOffset(base, getUnsignedIntConst(size, 64), size,
                             loaded2);
    }

    auto added = createAdd(baseAddr, offsetVal1);
    updateOutputReg(added);
    break;
  }

  case AArch64::ADRP: {
    assert(CurInst->getOperand(0).isReg());
    mapExprVar(CurInst->getOperand(1).getExpr());
    break;
  }

  case AArch64::CBZW:
  case AArch64::CBZX: {
    auto operand = readFromOperand(0);
    assert(operand != nullptr && "operand is null");
    auto cond_val = createICmp(ICmpInst::Predicate::ICMP_EQ, operand,
                               getUnsignedIntConst(0, getBitWidth(operand)));
    auto dst_true = getBB(CurInst->getOperand(1));
    assert(dst_true);
    assert(MCBB->getSuccs().size() == 1 || MCBB->getSuccs().size() == 2);

    BasicBlock *dst_false{nullptr};
    if (MCBB->getSuccs().size() == 1) {
      dst_false = dst_true;
    } else {
      const string *dst_false_name = nullptr;
      for (auto &succ : MCBB->getSuccs()) {
        if (succ->getName() != dst_true->getName()) {
          dst_false_name = &succ->getName();
          break;
        }
      }
      assert(dst_false_name != nullptr);
      dst_false = getBBByName(*dst_false_name);
    }
    createBranch(cond_val, dst_true, dst_false);
    break;
  }

  case AArch64::CBNZW:
  case AArch64::CBNZX: {
    auto operand = readFromOperand(0);
    assert(operand != nullptr && "operand is null");
    auto cond_val = createICmp(ICmpInst::Predicate::ICMP_NE, operand,
                               getUnsignedIntConst(0, getBitWidth(operand)));

    auto dst_true = getBB(CurInst->getOperand(1));
    assert(dst_true);
    auto succs = MCBB->getSuccs().size();

    if (succs == 2) {
      const string *dst_false_name = nullptr;
      for (auto &succ : MCBB->getSuccs()) {
        if (succ->getName() != dst_true->getName()) {
          dst_false_name = &succ->getName();
          break;
        }
      }
      assert(dst_false_name != nullptr);
      auto *dst_false = getBBByName(*dst_false_name);
      createBranch(cond_val, dst_true, dst_false);
    } else {
      assert(succs == 1);
    }
    break;
  }

  case AArch64::TBZW:
  case AArch64::TBZX:
  case AArch64::TBNZW:
  case AArch64::TBNZX: {
    auto size = getInstSize(opcode);
    auto operand = readFromOperand(0);
    assert(operand != nullptr && "operand is null");
    auto bit_pos = getImm(1);
    auto shift = createMaskedLShr(operand, getUnsignedIntConst(bit_pos, size));
    auto cond_val = createTrunc(shift, i1);

    auto &jmp_tgt_op = CurInst->getOperand(2);
    assert(jmp_tgt_op.isExpr() && "expected expression");
    assert((jmp_tgt_op.getExpr()->getKind() == MCExpr::ExprKind::SymbolRef) &&
           "expected symbol ref as bcc operand");
    const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*jmp_tgt_op.getExpr());
    const MCSymbol &Sym =
        SRE.getSymbol(); // FIXME refactor this into a function
    auto *dst_false = getBBByName(Sym.getName());

    assert(MCBB->getSuccs().size() == 1 || MCBB->getSuccs().size() == 2);

    const string *dst_true_name = nullptr;
    for (auto &succ : MCBB->getSuccs()) {
      if (succ->getName() != Sym.getName()) {
        dst_true_name = &succ->getName();
        break;
      }
    }
    auto *dst_true =
        getBBByName(dst_true_name ? *dst_true_name : Sym.getName());

    if (opcode == AArch64::TBNZW || opcode == AArch64::TBNZX)
      createBranch(cond_val, dst_false, dst_true);
    else
      createBranch(cond_val, dst_true, dst_false);
    break;
  }

  case AArch64::INSvi8lane:
  case AArch64::INSvi16lane:
  case AArch64::INSvi32lane:
  case AArch64::INSvi64lane: {
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
    break;
  }

  case AArch64::INSvi8gpr:
  case AArch64::INSvi16gpr:
  case AArch64::INSvi32gpr:
  case AArch64::INSvi64gpr: {
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
    break;
  }

  case AArch64::FNEGSr:
  case AArch64::FNEGDr: {
    auto operandSize = getRegSize(CurInst->getOperand(1).getReg());
    auto fVal = readFromFPOperand(1, operandSize);
    auto res = createFNeg(fVal);
    updateOutputReg(res);
    break;
  }

  case AArch64::FNEGv2f32:
  case AArch64::FNEGv4f32:
  case AArch64::FNEGv2f64: {
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
    break;
  }

  case AArch64::FCVTZUUWHr:
  case AArch64::FCVTZUUWSr:
  case AArch64::FCVTZUUWDr:
  case AArch64::FCVTZUUXHr:
  case AArch64::FCVTZUUXSr:
  case AArch64::FCVTZUUXDr:
  case AArch64::FCVTZSUWHr:
  case AArch64::FCVTZSUWSr:
  case AArch64::FCVTZSUWDr:
  case AArch64::FCVTZSUXHr:
  case AArch64::FCVTZSUXSr:
  case AArch64::FCVTZSUXDr: {
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
    break;
  }

  case AArch64::FCVTZSv1i32:
  case AArch64::FCVTZSv1i64: {
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
    break;
  }

  case AArch64::FCVTSHr:
  case AArch64::FCVTDHr:
  case AArch64::FCVTHSr:
  case AArch64::FCVTHDr: {
    *out << "\nERROR: only float and double supported (not  bfloat, half, "
            "fp128, etc.)\n\n";
    exit(-1);
  }

  case AArch64::FCVTDSr:
  case AArch64::FCVTSDr: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    assert(op0.isReg() && op1.isReg());

    auto op0Size = getRegSize(op0.getReg());
    auto op1Size = getRegSize(op1.getReg());

    auto fTy = getFPType(op0Size);
    auto fp_val = readFromFPOperand(1, op1Size);

    auto converted = op0Size < op1Size ? createFPTrunc(fp_val, fTy)
                                       : createFPExt(fp_val, fTy);

    updateOutputReg(converted);
    break;
  }

  case AArch64::FRINTXSr:
  case AArch64::FRINTXDr:
  case AArch64::FRINTASr:
  case AArch64::FRINTADr:
  case AArch64::FRINTMSr:
  case AArch64::FRINTMDr:
  case AArch64::FRINTPSr:
  case AArch64::FRINTPDr: {
    auto &op1 = CurInst->getOperand(1);
    assert(op1.isReg());
    auto *v = readFromFPOperand(1, getRegSize(op1.getReg()));
    auto md = MDString::get(Ctx, "fpexcept.strict");
    Value *converted{nullptr};
    switch (opcode) {
    case AArch64::FRINTXSr:
    case AArch64::FRINTXDr: {
      converted = createRound(v);
      break;
    }
    case AArch64::FRINTASr:
    case AArch64::FRINTADr: {
      converted = createConstrainedRound(v, md);
      break;
    }
    case AArch64::FRINTMSr:
    case AArch64::FRINTMDr: {
      converted = createConstrainedFloor(v, md);
      break;
    }
    case AArch64::FRINTPSr:
    case AArch64::FRINTPDr: {
      converted = createConstrainedCeil(v, md);
      break;
    }
    default:
      assert(false);
    }
    updateOutputReg(converted);
    break;
  }

  case AArch64::UCVTFUWSri:
  case AArch64::UCVTFUWDri:
  case AArch64::UCVTFUXSri:
  case AArch64::UCVTFUXDri:
  case AArch64::SCVTFUWSri:
  case AArch64::SCVTFUWDri:
  case AArch64::SCVTFUXSri:
  case AArch64::SCVTFUXDri: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    assert(op0.isReg() && op1.isReg());

    auto isSigned =
        opcode == AArch64::SCVTFUWSri || opcode == AArch64::SCVTFUWDri ||
        opcode == AArch64::SCVTFUXSri || opcode == AArch64::SCVTFUXDri;

    auto fTy = getFPType(getRegSize(op0.getReg()));
    auto val = readFromOperand(1, getRegSize(op1.getReg()));
    auto converted = isSigned ? createSIToFP(val, fTy) : createUIToFP(val, fTy);

    updateOutputReg(converted);
    break;
  }

  case AArch64::SCVTFv1i32:
  case AArch64::SCVTFv1i64: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    assert(op0.isReg() && op1.isReg());

    auto isSigned =
        opcode == AArch64::SCVTFv1i32 || opcode == AArch64::SCVTFv1i64;

    auto fTy = getFPType(getRegSize(op0.getReg()));
    auto val = readFromOperand(1, getRegSize(op1.getReg()));
    auto converted = isSigned ? createSIToFP(val, fTy) : createUIToFP(val, fTy);

    updateOutputReg(converted);
    break;
  }

  case AArch64::FMOVSWr:
  case AArch64::FMOVDXr:
  case AArch64::FMOVWSr:
  case AArch64::FMOVXDr:
  case AArch64::FMOVDr:
  case AArch64::FMOVSr: {
    auto v = readFromOperand(1);
    updateOutputReg(v);
    break;
  }

  case AArch64::FMOVSi:
  case AArch64::FMOVDi: {
    auto imm = getImm(1);
    assert(imm <= 256);
    int w = (opcode == AArch64::FMOVSi) ? 32 : 64;
    auto floatVal = getUnsignedIntConst(VFPExpandImm(imm, w), 64);
    updateOutputReg(floatVal);
    break;
  }

  case AArch64::FMOVv2f32_ns:
  case AArch64::FMOVv4f32_ns:
  case AArch64::FMOVv2f64_ns: {
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

    auto result =
        createBitCast(expandedImmVal, getVecTy(eltSize, numElts, true));
    updateOutputReg(result);

    break;
  }

  case AArch64::FABSSr:
  case AArch64::FABSDr: {
    auto a = readFromFPOperand(1, getRegSize(CurInst->getOperand(1).getReg()));
    auto res = createFAbs(a);
    updateOutputReg(res);
    break;
  }

  case AArch64::FMINSrr:
  case AArch64::FMINNMSrr:
  case AArch64::FMAXSrr:
  case AArch64::FMAXNMSrr:
  case AArch64::FMINDrr:
  case AArch64::FMINNMDrr:
  case AArch64::FMAXDrr:
  case AArch64::FMAXNMDrr: {

    auto a = readFromFPOperand(1, getRegSize(CurInst->getOperand(1).getReg()));
    auto b = readFromFPOperand(2, getRegSize(CurInst->getOperand(2).getReg()));

    Function *decl{nullptr};
    switch (opcode) {
    case AArch64::FMINSrr:
    case AArch64::FMINDrr:
      decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::minimum,
                                               a->getType());
      break;
    case AArch64::FMAXSrr:
    case AArch64::FMAXDrr:
      decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::maximum,
                                               a->getType());
      break;
    case AArch64::FMINNMSrr:
    case AArch64::FMINNMDrr:
      decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::minnum,
                                               a->getType());
      break;
    case AArch64::FMAXNMSrr:
    case AArch64::FMAXNMDrr:
      decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::maxnum,
                                               a->getType());
      break;
    default:
      assert(false);
    }
    assert(decl);

    Value *res = CallInst::Create(decl, {a, b}, nextName(), LLVMBB);
    updateOutputReg(res);
    break;
  }

  case AArch64::FMULSrr:
  case AArch64::FMULDrr:
  case AArch64::FNMULSrr:
  case AArch64::FNMULDrr:
  case AArch64::FDIVSrr:
  case AArch64::FDIVDrr:
  case AArch64::FADDSrr:
  case AArch64::FADDDrr:
  case AArch64::FSUBSrr:
  case AArch64::FSUBDrr: {
    Instruction::BinaryOps op;
    if (opcode == AArch64::FADDSrr || opcode == AArch64::FADDDrr) {
      op = Instruction::FAdd;
    } else if (opcode == AArch64::FMULSrr || opcode == AArch64::FMULDrr ||
               opcode == AArch64::FNMULSrr || opcode == AArch64::FNMULDrr) {
      op = Instruction::FMul;
    } else if (opcode == AArch64::FDIVSrr || opcode == AArch64::FDIVDrr) {
      op = Instruction::FDiv;
    } else if (opcode == AArch64::FSUBSrr || opcode == AArch64::FSUBDrr) {
      op = Instruction::FSub;
    } else {
      assert(false && "missed a case");
    }

    auto a = readFromFPOperand(1, getRegSize(CurInst->getOperand(1).getReg()));
    auto b = readFromFPOperand(2, getRegSize(CurInst->getOperand(2).getReg()));

    Value *res = createBinop(a, b, op);
    if (opcode == AArch64::FNMULSrr || opcode == AArch64::FNMULDrr) {
      res = createFNeg(res);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::FMULv1i32_indexed:
  case AArch64::FMULv1i64_indexed: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    assert(op0.isReg() && op1.isReg() && op2.isReg() && op3.isImm());
    Instruction::BinaryOps op;
    switch (opcode) {
    case AArch64::FMULv1i32_indexed:
    case AArch64::FMULv1i64_indexed: {
      op = Instruction::FMul;
      break;
    }
    default: {
      assert(false && "missed a case");
    }
    }

    auto eltSize = getRegSize(op1.getReg());
    auto a = readFromFPOperand(1, eltSize);
    auto b = getIndexedFPElement(op3.getImm(), eltSize, op2.getReg());
    auto res = createBinop(a, b, op);
    updateOutputReg(res);
    break;
  }

  case AArch64::FMLAv1i32_indexed:
  case AArch64::FMLAv1i64_indexed:
  case AArch64::FMLSv1i32_indexed:
  case AArch64::FMLSv1i64_indexed: {
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    auto &op4 = CurInst->getOperand(4);
    assert(op1.isReg() && op2.isReg() && op3.isReg() && op4.isImm());

    bool negateProduct = false;
    switch (opcode) {
    case AArch64::FMLSv1i32_indexed:
    case AArch64::FMLSv1i64_indexed: {
      negateProduct = true;
      break;
    }
    }

    auto eltSize = getRegSize(op2.getReg());
    auto c = readFromFPOperand(1, getRegSize(op1.getReg()));
    auto a = readFromFPOperand(2, eltSize);
    auto b = getIndexedFPElement(op4.getImm(), eltSize, op3.getReg());

    if (negateProduct) {
      a = createFNeg(a);
    }

    auto res = createFusedMultiplyAdd(a, b, c);

    updateOutputReg(res);
    break;
  }

  case AArch64::FMADDSrrr:
  case AArch64::FMADDDrrr:
  case AArch64::FMSUBSrrr:
  case AArch64::FMSUBDrrr:
  case AArch64::FNMADDSrrr:
  case AArch64::FNMADDDrrr:
  case AArch64::FNMSUBSrrr:
  case AArch64::FNMSUBDrrr: {
    auto &op0 = CurInst->getOperand(0);
    auto &op1 = CurInst->getOperand(1);
    auto &op2 = CurInst->getOperand(2);
    auto &op3 = CurInst->getOperand(3);
    assert(op0.isReg() && op1.isReg() && op2.isReg() && op3.isReg());

    bool negateProduct = false, negateAddend = false;
    switch (opcode) {
    case AArch64::FMSUBSrrr:
    case AArch64::FMSUBDrrr: {
      negateProduct = true;
      break;
    }
    case AArch64::FNMADDSrrr:
    case AArch64::FNMADDDrrr: {
      negateAddend = true;
      negateProduct = true;
      break;
    }
    case AArch64::FNMSUBSrrr:
    case AArch64::FNMSUBDrrr: {
      negateAddend = true;
      break;
    }
    }

    auto a = readFromFPOperand(1, getRegSize(op1.getReg()));
    auto b = readFromFPOperand(2, getRegSize(op2.getReg()));
    auto c = readFromFPOperand(3, getRegSize(op3.getReg()));

    if (negateProduct) {
      a = createFNeg(a);
    }
    if (negateAddend) {
      c = createFNeg(c);
    }

    auto res = createFusedMultiplyAdd(a, b, c);

    updateOutputReg(res);
    break;
  }

  case AArch64::FSQRTSr:
  case AArch64::FSQRTDr: {
    auto operandSize = getRegSize(CurInst->getOperand(1).getReg());
    auto a = readFromFPOperand(1, operandSize);
    auto res = createSQRT(a);
    updateOutputReg(res);
    break;
  }

  case AArch64::FADDv2f32:
  case AArch64::FADDv4f32:
  case AArch64::FADDv2f64:
  case AArch64::FSUBv2f32:
  case AArch64::FSUBv4f32:
  case AArch64::FSUBv2f64:
  case AArch64::FMULv2f32:
  case AArch64::FMULv4f32:
  case AArch64::FMULv2f64: {
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
    break;
  }

  case AArch64::FCMPSri:
  case AArch64::FCMPDri:
  case AArch64::FCMPESri:
  case AArch64::FCMPEDri:
  case AArch64::FCMPSrr:
  case AArch64::FCMPDrr:
  case AArch64::FCMPESrr:
  case AArch64::FCMPEDrr: {
    auto operandSize = getRegSize(CurInst->getOperand(0).getReg());
    auto a = readFromFPOperand(0, operandSize);
    Value *b;
    if (opcode == AArch64::FCMPSri || opcode == AArch64::FCMPDri ||
        opcode == AArch64::FCMPESri || opcode == AArch64::FCMPEDri) {
      b = ConstantFP::get(getFPType(operandSize), 0.0);
    } else {
      b = readFromFPOperand(1, operandSize);
    }
    auto [n, z, c, v] = FPCompare(a, b);
    setN(n);
    setZ(z);
    setC(c);
    setV(v);
    break;
  }

  case AArch64::FCCMPSrr:
  case AArch64::FCCMPDrr: {
    auto operandSize = getRegSize(CurInst->getOperand(0).getReg());
    auto a = readFromFPOperand(0, operandSize);
    auto b = readFromFPOperand(1, operandSize);

    auto [imm_n, imm_z, imm_c, imm_v] = splitImmNZCV(getImm(2));
    auto [n, z, c, v] = FPCompare(a, b);

    auto cond = conditionHolds(getImm(3));

    auto new_n = createSelect(cond, n, imm_n);
    auto new_z = createSelect(cond, z, imm_z);
    auto new_c = createSelect(cond, c, imm_c);
    auto new_v = createSelect(cond, v, imm_v);

    setN(new_n);
    setZ(new_z);
    setC(new_c);
    setV(new_v);
    break;
  }

  case AArch64::SMOVvi8to32_idx0:
  case AArch64::SMOVvi8to32:
  case AArch64::SMOVvi16to32_idx0:
  case AArch64::SMOVvi16to32:
  case AArch64::SMOVvi8to64_idx0:
  case AArch64::SMOVvi8to64:
  case AArch64::SMOVvi16to64_idx0:
  case AArch64::SMOVvi16to64:
  case AArch64::SMOVvi32to64_idx0:
  case AArch64::SMOVvi32to64: {
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
    break;
  }

  case AArch64::UMOVvi8:
  case AArch64::UMOVvi8_idx0:
  case AArch64::UMOVvi16:
  case AArch64::UMOVvi16_idx0:
  case AArch64::UMOVvi32:
  case AArch64::UMOVvi32_idx0:
  case AArch64::UMOVvi64: {
    unsigned sz;
    if (opcode == AArch64::UMOVvi8 || opcode == AArch64::UMOVvi8_idx0) {
      sz = 8;
    } else if (opcode == AArch64::UMOVvi16 ||
               opcode == AArch64::UMOVvi16_idx0) {
      sz = 16;
    } else if (opcode == AArch64::UMOVvi32 ||
               opcode == AArch64::UMOVvi32_idx0) {
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
    break;
  }

  case AArch64::MVNIv8i16:
  case AArch64::MVNIv4i32:
  case AArch64::MVNIv4i16:
  case AArch64::MVNIv2i32: {
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
    break;
  }

  case AArch64::MVNIv2s_msl:
  case AArch64::MVNIv4s_msl: {
    auto imm1 = getUnsignedIntConst(getImm(1), 32);
    auto imm2 = getImm(2) & ~0x100;
    auto v = createNot(createMSL(imm1, imm2));
    int numElts = (opcode == AArch64::MVNIv2s_msl) ? 2 : 4;
    updateOutputReg(dupElts(v, numElts, 32));
    break;
  }

  case AArch64::MOVIv2s_msl:
  case AArch64::MOVIv4s_msl: {
    auto imm1 = getUnsignedIntConst(getImm(1), 32);
    auto imm2 = getImm(2) & ~0x100;
    auto v = createMSL(imm1, imm2);
    int numElts = (opcode == AArch64::MOVIv2s_msl) ? 2 : 4;
    updateOutputReg(dupElts(v, numElts, 32));
    break;
  }

  case AArch64::MOVID:
  case AArch64::MOVIv2d_ns: {
    auto imm = getUnsignedIntConst(replicate8to64(getImm(1)), 64);
    updateOutputReg(dupElts(imm, 2, 64));
    break;
  }

  case AArch64::MOVIv8b_ns: {
    auto v = getUnsignedIntConst(getImm(1), 8);
    updateOutputReg(dupElts(v, 8, 8));
    break;
  }

  case AArch64::MOVIv16b_ns: {
    auto v = getUnsignedIntConst(getImm(1), 8);
    updateOutputReg(dupElts(v, 16, 8));
    break;
  }

  case AArch64::MOVIv4i16: {
    auto imm1 = getImm(1);
    auto imm2 = getImm(2);
    auto val = getUnsignedIntConst(imm1 << imm2, 16);
    updateOutputReg(dupElts(val, 4, 16));
    break;
  }

  case AArch64::MOVIv8i16: {
    auto imm1 = getImm(1);
    auto imm2 = getImm(2);
    auto val = getUnsignedIntConst(imm1 << imm2, 16);
    updateOutputReg(dupElts(val, 8, 16));
    break;
  }

  case AArch64::MOVIv2i32: {
    auto imm1 = getImm(1);
    auto imm2 = getImm(2);
    auto val = getUnsignedIntConst(imm1 << imm2, 32);
    updateOutputReg(dupElts(val, 2, 32));
    break;
  }

  case AArch64::MOVIv4i32: {
    auto imm1 = getImm(1);
    auto imm2 = getImm(2);
    auto val = getUnsignedIntConst(imm1 << imm2, 32);
    updateOutputReg(dupElts(val, 4, 32));
    break;
  }

  case AArch64::EXTv8i8: {
    auto a = readFromOperand(1);
    auto b = readFromOperand(2);
    auto imm = getImm(3);
    auto both = concat(b, a);
    auto shifted = createRawLShr(both, getUnsignedIntConst(8 * imm, 128));
    updateOutputReg(createTrunc(shifted, i64));
    break;
  }

  case AArch64::EXTv16i8: {
    auto a = readFromOperand(1);
    auto b = readFromOperand(2);
    auto imm = getImm(3);
    auto both = concat(b, a);
    auto shifted = createRawLShr(both, getUnsignedIntConst(8 * imm, 256));
    updateOutputReg(createTrunc(shifted, i128));
    break;
  }

  case AArch64::REV64v4i32: {
    auto v = rev(readFromOperand(1), 32, 64);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV64v2i32: {
    auto v = rev(readFromOperand(1), 32, 64);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV64v4i16: {
    auto v = rev(readFromOperand(1), 16, 64);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV64v8i8: {
    auto v = rev(readFromOperand(1), 8, 64);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV64v8i16: {
    auto v = rev(readFromOperand(1), 16, 64);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV64v16i8: {
    auto v = rev(readFromOperand(1), 8, 64);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV16v8i8: {
    auto v = rev(readFromOperand(1), 8, 16);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV16v16i8: {
    auto v = rev(readFromOperand(1), 8, 16);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV32v4i16: {
    auto v = rev(readFromOperand(1), 16, 32);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV32v8i8: {
    auto v = rev(readFromOperand(1), 8, 32);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV32v8i16: {
    auto v = rev(readFromOperand(1), 16, 32);
    updateOutputReg(v);
    break;
  }

  case AArch64::REV32v16i8: {
    auto v = rev(readFromOperand(1), 8, 32);
    updateOutputReg(v);
    break;
  }

  case AArch64::DUPi8: {
    auto in = readFromVecOperand(1, 8, 16);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(ext);
    break;
  }

  case AArch64::DUPi16: {
    auto in = readFromVecOperand(1, 16, 8);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(ext);
    break;
  }

  case AArch64::DUPi32: {
    auto in = readFromVecOperand(1, 32, 4);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(ext);
    break;
  }

  case AArch64::DUPi64: {
    auto in = readFromVecOperand(1, 64, 2);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(ext);
    break;
  }

  case AArch64::DUPv8i8gpr: {
    auto t = createTrunc(readFromOperand(1), i8);
    updateOutputReg(dupElts(t, 8, 8));
    break;
  }

  case AArch64::DUPv16i8gpr: {
    auto t = createTrunc(readFromOperand(1), i8);
    updateOutputReg(dupElts(t, 16, 8));
    break;
  }

  case AArch64::DUPv8i16gpr: {
    auto t = createTrunc(readFromOperand(1), i16);
    updateOutputReg(dupElts(t, 8, 16));
    break;
  }

  case AArch64::DUPv4i16gpr: {
    auto t = createTrunc(readFromOperand(1), i16);
    updateOutputReg(dupElts(t, 4, 16));
    break;
  }

  case AArch64::DUPv4i32gpr: {
    auto t = createTrunc(readFromOperand(1), i32);
    updateOutputReg(dupElts(t, 4, 32));
    break;
  }

  case AArch64::DUPv2i32gpr: {
    auto t = createTrunc(readFromOperand(1), i32);
    updateOutputReg(dupElts(t, 2, 32));
    break;
  }

  case AArch64::DUPv2i64gpr: {
    updateOutputReg(dupElts(readFromOperand(1), 2, 64));
    break;
  }

  case AArch64::DUPv2i32lane: {
    auto in = readFromVecOperand(1, 32, 4);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 2, 32));
    break;
  }

  case AArch64::DUPv2i64lane: {
    auto in = readFromVecOperand(1, 64, 2);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 2, 64));
    break;
  }

  case AArch64::DUPv4i16lane: {
    auto in = readFromVecOperand(1, 16, 8);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 4, 16));
    break;
  }

  case AArch64::DUPv4i32lane: {
    auto in = readFromVecOperand(1, 32, 4);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 4, 32));
    break;
  }

  case AArch64::DUPv8i8lane: {
    auto in = readFromVecOperand(1, 8, 16);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 8, 8));
    break;
  }

  case AArch64::DUPv8i16lane: {
    auto in = readFromVecOperand(1, 16, 8);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 8, 16));
    break;
  }

  case AArch64::DUPv16i8lane: {
    auto in = readFromVecOperand(1, 8, 16);
    auto ext = createExtractElement(in, getImm(2));
    updateOutputReg(dupElts(ext, 16, 8));
    break;
  }

  case AArch64::BIFv8i8:
  case AArch64::BIFv16i8: {
    auto op1 = readFromOperand(1);
    auto op4 = readFromOperand(2);
    auto op3 = createNot(readFromOperand(3));
    auto res = createXor(op1, createAnd(createXor(op1, op4), op3));
    updateOutputReg(res);
    break;
  }

  case AArch64::BITv16i8:
  case AArch64::BITv8i8: {
    auto op4 = readFromOperand(2);
    auto op1 = readFromOperand(1);
    auto op3 = readFromOperand(3);
    auto res = createXor(op1, createAnd(createXor(op1, op4), op3));
    updateOutputReg(res);
    break;
  }

  case AArch64::BSLv8i8:
  case AArch64::BSLv16i8: {
    auto op1 = readFromOperand(3);
    auto op4 = readFromOperand(2);
    auto op3 = readFromOperand(1);
    auto res = createXor(op1, createAnd(createXor(op1, op4), op3));
    updateOutputReg(res);
    break;
  }

    /*
case AArch64::FCMEQ64:
case AArch64::FCMGT64:
case AArch64::FCMGE64:
    */

  case AArch64::FCMLEv2i64rz:
  case AArch64::FCMLEv4i32rz:
  case AArch64::FCMLEv2i32rz:
  case AArch64::FCMLTv2i32rz:
  case AArch64::FCMLTv2i64rz:
  case AArch64::FCMLTv4i32rz:
  case AArch64::FCMEQv2i32rz:
  case AArch64::FCMEQv2i64rz:
  case AArch64::FCMEQv4i32rz:
  case AArch64::FCMGTv4i32rz:
  case AArch64::FCMGTv2i64rz:
  case AArch64::FCMGTv2i32rz:
  case AArch64::FCMGEv2i32rz:
  case AArch64::FCMGEv4i32rz:
  case AArch64::FCMGEv2i64rz:
  case AArch64::FCMEQv2f32:
  case AArch64::FCMEQv4f32:
  case AArch64::FCMEQv2f64:
  case AArch64::FCMGTv2f32:
  case AArch64::FCMGTv4f32:
  case AArch64::FCMGTv2f64:
  case AArch64::FCMGEv2f32:
  case AArch64::FCMGEv4f32:
  case AArch64::FCMGEv2f64: {
    FCmpInst::Predicate pred;
    switch (opcode) {
    case AArch64::FCMLEv2i64rz:
    case AArch64::FCMLEv4i32rz:
    case AArch64::FCMLEv2i32rz:
      pred = FCmpInst::Predicate::FCMP_OLE;
      break;
    case AArch64::FCMLTv2i32rz:
    case AArch64::FCMLTv2i64rz:
    case AArch64::FCMLTv4i32rz:
      pred = FCmpInst::Predicate::FCMP_OLT;
      break;
    case AArch64::FCMEQv2f32:
    case AArch64::FCMEQv4f32:
    case AArch64::FCMEQv2f64:
    case AArch64::FCMEQv2i32rz:
    case AArch64::FCMEQv2i64rz:
    case AArch64::FCMEQv4i32rz:
      pred = FCmpInst::Predicate::FCMP_OEQ;
      break;
    case AArch64::FCMGTv2f32:
    case AArch64::FCMGTv4f32:
    case AArch64::FCMGTv2f64:
    case AArch64::FCMGTv4i32rz:
    case AArch64::FCMGTv2i64rz:
    case AArch64::FCMGTv2i32rz:
      pred = FCmpInst::Predicate::FCMP_OGT;
      break;
    case AArch64::FCMGEv2f32:
    case AArch64::FCMGEv4f32:
    case AArch64::FCMGEv2f64:
    case AArch64::FCMGEv2i32rz:
    case AArch64::FCMGEv4i32rz:
    case AArch64::FCMGEv2i64rz:
      pred = FCmpInst::Predicate::FCMP_OGE;
      break;
    default:
      assert(false);
    }

    int eltSize = -1;
    int numElts = -1;
    switch (opcode) {
    case AArch64::FCMEQv2i32rz:
    case AArch64::FCMLEv2i32rz:
    case AArch64::FCMGEv2i32rz:
    case AArch64::FCMLTv2i32rz:
    case AArch64::FCMGTv2i32rz:
    case AArch64::FCMEQv2f32:
    case AArch64::FCMGTv2f32:
    case AArch64::FCMGEv2f32:
      eltSize = 32;
      numElts = 2;
      break;
    case AArch64::FCMEQv4i32rz:
    case AArch64::FCMLEv4i32rz:
    case AArch64::FCMGEv4i32rz:
    case AArch64::FCMLTv4i32rz:
    case AArch64::FCMGTv4i32rz:
    case AArch64::FCMEQv4f32:
    case AArch64::FCMGTv4f32:
    case AArch64::FCMGEv4f32:
      eltSize = 32;
      numElts = 4;
      break;
    case AArch64::FCMEQv2f64:
    case AArch64::FCMGTv2f64:
    case AArch64::FCMGEv2f64:
    case AArch64::FCMEQv2i64rz:
    case AArch64::FCMLEv2i64rz:
    case AArch64::FCMGEv2i64rz:
    case AArch64::FCMLTv2i64rz:
    case AArch64::FCMGTv2i64rz:
      eltSize = 64;
      numElts = 2;
      break;
    default:
      assert(false);
    }
    auto *vTy = getVecTy(eltSize, numElts, /*FP=*/true);
    auto *vIntTy = getVecTy(eltSize, numElts, /*FP=*/false);
    auto a = readFromRegTyped(CurInst->getOperand(1).getReg(), vTy);
    Value *b{nullptr};
    switch (opcode) {
    case AArch64::FCMEQv2i32rz:
    case AArch64::FCMEQv2i64rz:
    case AArch64::FCMEQv4i32rz:
    case AArch64::FCMLEv2i64rz:
    case AArch64::FCMLEv4i32rz:
    case AArch64::FCMLEv2i32rz:
    case AArch64::FCMGEv2i32rz:
    case AArch64::FCMGEv4i32rz:
    case AArch64::FCMGEv2i64rz:
    case AArch64::FCMLTv2i32rz:
    case AArch64::FCMLTv2i64rz:
    case AArch64::FCMLTv4i32rz:
    case AArch64::FCMGTv4i32rz:
    case AArch64::FCMGTv2i64rz:
    case AArch64::FCMGTv2i32rz:
      b = getZeroFPVec(numElts, eltSize);
      break;
    case AArch64::FCMEQv2f32:
    case AArch64::FCMGTv2f32:
    case AArch64::FCMGEv2f32:
    case AArch64::FCMEQv4f32:
    case AArch64::FCMGTv4f32:
    case AArch64::FCMGEv4f32:
    case AArch64::FCMEQv2f64:
    case AArch64::FCMGTv2f64:
    case AArch64::FCMGEv2f64:
      b = readFromRegTyped(CurInst->getOperand(2).getReg(), vTy);
      break;
    default:
      assert(false);
    }
    auto res1 = createFCmp(pred, a, b);
    auto res2 = createSExt(res1, vIntTy);
    updateOutputReg(res2);
    break;
  }

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
  case AArch64::CMGTv4i32rz:
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
  case AArch64::CMLTv16i8rz:
  case AArch64::CMLTv2i32rz:
  case AArch64::CMLTv2i64rz:
  case AArch64::CMLTv4i16rz:
  case AArch64::CMLTv4i32rz:
  case AArch64::CMLTv8i16rz:
  case AArch64::CMLTv8i8rz:
  case AArch64::CMTSTv4i16:
  case AArch64::CMTSTv2i32:
  case AArch64::CMTSTv16i8:
  case AArch64::CMTSTv2i64:
  case AArch64::CMTSTv4i32:
  case AArch64::CMTSTv8i16:
  case AArch64::CMTSTv8i8: {
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
    break;
  }

  case AArch64::TBLv8i8One:
  case AArch64::TBLv8i8Two:
  case AArch64::TBLv8i8Three:
  case AArch64::TBLv8i8Four:
  case AArch64::TBLv16i8One:
  case AArch64::TBLv16i8Two:
  case AArch64::TBLv16i8Three:
  case AArch64::TBLv16i8Four: {
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
    break;
  }

  case AArch64::RADDHNv2i64_v2i32:
  case AArch64::RADDHNv2i64_v4i32:
  case AArch64::RADDHNv4i32_v4i16:
  case AArch64::RADDHNv4i32_v8i16:
  case AArch64::RADDHNv8i16_v8i8:
  case AArch64::RADDHNv8i16_v16i8:
  case AArch64::ADDHNv2i64_v2i32:
  case AArch64::ADDHNv2i64_v4i32:
  case AArch64::ADDHNv4i32_v4i16:
  case AArch64::ADDHNv4i32_v8i16:
  case AArch64::ADDHNv8i16_v8i8:
  case AArch64::ADDHNv8i16_v16i8:
  case AArch64::RSUBHNv2i64_v2i32:
  case AArch64::RSUBHNv2i64_v4i32:
  case AArch64::RSUBHNv4i32_v4i16:
  case AArch64::RSUBHNv4i32_v8i16:
  case AArch64::RSUBHNv8i16_v8i8:
  case AArch64::RSUBHNv8i16_v16i8:
  case AArch64::SUBHNv2i64_v2i32:
  case AArch64::SUBHNv2i64_v4i32:
  case AArch64::SUBHNv4i32_v4i16:
  case AArch64::SUBHNv4i32_v8i16:
  case AArch64::SUBHNv8i16_v8i8:
  case AArch64::SUBHNv8i16_v16i8: {
    unsigned eltSize = -1, numElts = -1;
    switch (opcode) {
    case AArch64::RADDHNv2i64_v2i32:
    case AArch64::RADDHNv2i64_v4i32:
    case AArch64::ADDHNv2i64_v2i32:
    case AArch64::ADDHNv2i64_v4i32:
    case AArch64::RSUBHNv2i64_v2i32:
    case AArch64::RSUBHNv2i64_v4i32:
    case AArch64::SUBHNv2i64_v2i32:
    case AArch64::SUBHNv2i64_v4i32:
      eltSize = 32;
      numElts = 2;
      break;
    case AArch64::RADDHNv4i32_v4i16:
    case AArch64::RADDHNv4i32_v8i16:
    case AArch64::ADDHNv4i32_v4i16:
    case AArch64::ADDHNv4i32_v8i16:
    case AArch64::RSUBHNv4i32_v4i16:
    case AArch64::RSUBHNv4i32_v8i16:
    case AArch64::SUBHNv4i32_v4i16:
    case AArch64::SUBHNv4i32_v8i16:
      eltSize = 16;
      numElts = 4;
      break;
    case AArch64::RADDHNv8i16_v8i8:
    case AArch64::RADDHNv8i16_v16i8:
    case AArch64::ADDHNv8i16_v8i8:
    case AArch64::ADDHNv8i16_v16i8:
    case AArch64::RSUBHNv8i16_v8i8:
    case AArch64::RSUBHNv8i16_v16i8:
    case AArch64::SUBHNv8i16_v8i8:
    case AArch64::SUBHNv8i16_v16i8:
      eltSize = 8;
      numElts = 8;
      break;
    default:
      assert(false);
      break;
    }

    Instruction::BinaryOps op;
    bool addRoundingConst = false;
    switch (opcode) {
    case AArch64::RADDHNv2i64_v2i32:
    case AArch64::RADDHNv4i32_v4i16:
    case AArch64::RADDHNv8i16_v8i8:
    case AArch64::RADDHNv2i64_v4i32:
    case AArch64::RADDHNv4i32_v8i16:
    case AArch64::RADDHNv8i16_v16i8:
      addRoundingConst = true;
    case AArch64::ADDHNv2i64_v2i32:
    case AArch64::ADDHNv4i32_v4i16:
    case AArch64::ADDHNv8i16_v8i8:
    case AArch64::ADDHNv2i64_v4i32:
    case AArch64::ADDHNv4i32_v8i16:
    case AArch64::ADDHNv8i16_v16i8:
      op = Instruction::BinaryOps::Add;
      break;
    case AArch64::RSUBHNv2i64_v2i32:
    case AArch64::RSUBHNv4i32_v4i16:
    case AArch64::RSUBHNv8i16_v8i8:
    case AArch64::RSUBHNv2i64_v4i32:
    case AArch64::RSUBHNv4i32_v8i16:
    case AArch64::RSUBHNv8i16_v16i8:
      addRoundingConst = true;
    case AArch64::SUBHNv2i64_v2i32:
    case AArch64::SUBHNv4i32_v4i16:
    case AArch64::SUBHNv8i16_v8i8:
    case AArch64::SUBHNv2i64_v4i32:
    case AArch64::SUBHNv4i32_v8i16:
    case AArch64::SUBHNv8i16_v16i8:
      op = Instruction::BinaryOps::Sub;
      break;
    default:
      assert(false);
      break;
    }

    bool isUpper = false;
    switch (opcode) {
    // RADDHN2 variants
    case AArch64::RADDHNv2i64_v4i32:
    case AArch64::RADDHNv4i32_v8i16:
    case AArch64::RADDHNv8i16_v16i8:
    // ADDHN2 variants
    case AArch64::ADDHNv2i64_v4i32:
    case AArch64::ADDHNv4i32_v8i16:
    case AArch64::ADDHNv8i16_v16i8:
    // RSUBHN2 variants
    case AArch64::RSUBHNv2i64_v4i32:
    case AArch64::RSUBHNv4i32_v8i16:
    case AArch64::RSUBHNv8i16_v16i8:
    // SUBHN2 variants
    case AArch64::SUBHNv2i64_v4i32:
    case AArch64::SUBHNv4i32_v8i16:
    case AArch64::SUBHNv8i16_v16i8:
      isUpper = true;
      break;
    default:
      break;
    }

    auto a = readFromVecOperand(isUpper ? 2 : 1, 2 * eltSize, numElts);
    auto b = readFromVecOperand(isUpper ? 3 : 2, 2 * eltSize, numElts);

    Value *res = createBinop(a, b, op);

    if (addRoundingConst) {
      auto roundingConst =
          getElemSplat(numElts, 2 * eltSize, (uint64_t)1 << (eltSize - 1));
      res = createBinop(res, roundingConst, Instruction::BinaryOps::Add);
    }

    Value *shifted =
        createRawLShr(res, getElemSplat(numElts, 2 * eltSize, eltSize));
    res = createTrunc(shifted, getVecTy(eltSize, numElts));
    if (isUpper) {
      // Preserve the lower 64 bits so, read from destination register
      // and insert to the upper 64 bits
      Value *dest = readFromVecOperand(1, 64, 2);
      Value *element = createBitCast(res, i64);
      res = createInsertElement(dest, element, 1);
    }

    updateOutputReg(res);
    break;
  }

    // lane-wise binary vector instructions
  case AArch64::ORNv8i8:
  case AArch64::ORNv16i8:
  case AArch64::SMINv8i8:
  case AArch64::SMINv4i16:
  case AArch64::SMINv2i32:
  case AArch64::SMINv16i8:
  case AArch64::SMINv8i16:
  case AArch64::SMINv4i32:
  case AArch64::SMAXv8i8:
  case AArch64::SMAXv4i16:
  case AArch64::SMAXv2i32:
  case AArch64::SMAXv16i8:
  case AArch64::SMAXv8i16:
  case AArch64::SMAXv4i32:
  case AArch64::UMINv8i8:
  case AArch64::UMINv4i16:
  case AArch64::UMINv2i32:
  case AArch64::UMINv16i8:
  case AArch64::UMINv8i16:
  case AArch64::UMINv4i32:
  case AArch64::UMAXv8i8:
  case AArch64::UMAXv4i16:
  case AArch64::UMAXv2i32:
  case AArch64::UMAXv16i8:
  case AArch64::UMAXv8i16:
  case AArch64::UMAXv4i32:
  case AArch64::SMINPv8i8:
  case AArch64::SMINPv4i16:
  case AArch64::SMINPv2i32:
  case AArch64::SMINPv16i8:
  case AArch64::SMINPv8i16:
  case AArch64::SMINPv4i32:
  case AArch64::SMAXPv8i8:
  case AArch64::SMAXPv4i16:
  case AArch64::SMAXPv2i32:
  case AArch64::SMAXPv16i8:
  case AArch64::SMAXPv8i16:
  case AArch64::SMAXPv4i32:
  case AArch64::UMINPv8i8:
  case AArch64::UMINPv4i16:
  case AArch64::UMINPv2i32:
  case AArch64::UMINPv16i8:
  case AArch64::UMINPv8i16:
  case AArch64::UMINPv4i32:
  case AArch64::UMAXPv8i8:
  case AArch64::UMAXPv4i16:
  case AArch64::UMAXPv2i32:
  case AArch64::UMAXPv16i8:
  case AArch64::UMAXPv8i16:
  case AArch64::UMAXPv4i32:
  case AArch64::SMULLv8i8_v8i16:
  case AArch64::SMULLv2i32_v2i64:
  case AArch64::SMULLv4i16_v4i32:
  case AArch64::SMULLv8i16_v4i32:
  case AArch64::SMULLv16i8_v8i16:
  case AArch64::SMULLv4i32_v2i64:
  case AArch64::USHRv8i8_shift:
  case AArch64::USHRv4i16_shift:
  case AArch64::USHRv2i32_shift:
  case AArch64::USHRv16i8_shift:
  case AArch64::USHRv8i16_shift:
  case AArch64::USHRv4i32_shift:
  case AArch64::USHRd:
  case AArch64::USHRv2i64_shift:
  case AArch64::MULv2i32:
  case AArch64::MULv8i8:
  case AArch64::MULv4i16:
  case AArch64::MULv16i8:
  case AArch64::MULv8i16:
  case AArch64::MULv4i32:
  case AArch64::SSHRv8i8_shift:
  case AArch64::SSHRv16i8_shift:
  case AArch64::SSHRv4i16_shift:
  case AArch64::SSHRv8i16_shift:
  case AArch64::SSHRv2i32_shift:
  case AArch64::SSHRv4i32_shift:
  case AArch64::SSHRd:
  case AArch64::SSHRv2i64_shift:
  case AArch64::SHLv16i8_shift:
  case AArch64::SHLv8i16_shift:
  case AArch64::SHLv4i32_shift:
  case AArch64::SHLv2i64_shift:
  case AArch64::SHLv8i8_shift:
  case AArch64::SHLv4i16_shift:
  case AArch64::SHLd:
  case AArch64::SHLv2i32_shift:
  case AArch64::BICv4i16:
  case AArch64::BICv8i8:
  case AArch64::BICv2i32:
  case AArch64::BICv8i16:
  case AArch64::BICv4i32:
  case AArch64::BICv16i8:
  case AArch64::ADDv2i32:
  case AArch64::ADDv1i64:
  case AArch64::ADDv2i64:
  case AArch64::ADDv4i16:
  case AArch64::ADDv4i32:
  case AArch64::ADDv8i8:
  case AArch64::ADDv8i16:
  case AArch64::ADDv16i8:
  case AArch64::UADDLv8i8_v8i16:
  case AArch64::UADDLv16i8_v8i16:
  case AArch64::UADDLv4i16_v4i32:
  case AArch64::UADDLv8i16_v4i32:
  case AArch64::UADDLv2i32_v2i64:
  case AArch64::UADDLv4i32_v2i64:
  case AArch64::UADDWv8i8_v8i16:
  case AArch64::UADDWv16i8_v8i16:
  case AArch64::UADDWv4i16_v4i32:
  case AArch64::UADDWv8i16_v4i32:
  case AArch64::UADDWv2i32_v2i64:
  case AArch64::UADDWv4i32_v2i64:
  case AArch64::SADDLv8i8_v8i16:
  case AArch64::SADDLv16i8_v8i16:
  case AArch64::SADDLv4i16_v4i32:
  case AArch64::SADDLv8i16_v4i32:
  case AArch64::SADDLv2i32_v2i64:
  case AArch64::SADDLv4i32_v2i64:
  case AArch64::SADDWv8i8_v8i16:
  case AArch64::SADDWv16i8_v8i16:
  case AArch64::SADDWv4i16_v4i32:
  case AArch64::SADDWv8i16_v4i32:
  case AArch64::SADDWv2i32_v2i64:
  case AArch64::SADDWv4i32_v2i64:
  case AArch64::USUBLv8i8_v8i16:
  case AArch64::USUBLv16i8_v8i16:
  case AArch64::USUBLv4i16_v4i32:
  case AArch64::USUBLv8i16_v4i32:
  case AArch64::USUBLv2i32_v2i64:
  case AArch64::USUBLv4i32_v2i64:
  case AArch64::USUBWv8i8_v8i16:
  case AArch64::USUBWv16i8_v8i16:
  case AArch64::USUBWv4i16_v4i32:
  case AArch64::USUBWv8i16_v4i32:
  case AArch64::USUBWv2i32_v2i64:
  case AArch64::USUBWv4i32_v2i64:
  case AArch64::SSUBLv8i8_v8i16:
  case AArch64::SSUBLv16i8_v8i16:
  case AArch64::SSUBLv4i16_v4i32:
  case AArch64::SSUBLv8i16_v4i32:
  case AArch64::SSUBLv2i32_v2i64:
  case AArch64::SSUBLv4i32_v2i64:
  case AArch64::SSUBWv8i8_v8i16:
  case AArch64::SSUBWv16i8_v8i16:
  case AArch64::SSUBWv4i16_v4i32:
  case AArch64::SSUBWv8i16_v4i32:
  case AArch64::SSUBWv2i32_v2i64:
  case AArch64::SSUBWv4i32_v2i64:
  case AArch64::SUBv1i64:
  case AArch64::SUBv2i32:
  case AArch64::SUBv2i64:
  case AArch64::SUBv4i16:
  case AArch64::SUBv4i32:
  case AArch64::SUBv8i8:
  case AArch64::SUBv8i16:
  case AArch64::SUBv16i8:
  case AArch64::EORv8i8:
  case AArch64::EORv16i8:
  case AArch64::ANDv8i8:
  case AArch64::ANDv16i8:
  case AArch64::ORRv8i8:
  case AArch64::ORRv16i8:
  case AArch64::ORRv2i32:
  case AArch64::ORRv4i16:
  case AArch64::ORRv8i16:
  case AArch64::ORRv4i32:
  case AArch64::UMULLv2i32_v2i64:
  case AArch64::UMULLv8i8_v8i16:
  case AArch64::UMULLv4i16_v4i32:
  case AArch64::UMULLv16i8_v8i16:
  case AArch64::UMULLv8i16_v4i32:
  case AArch64::UMULLv4i32_v2i64:
  case AArch64::USHLv1i64:
  case AArch64::USHLv4i16:
  case AArch64::USHLv16i8:
  case AArch64::USHLv8i16:
  case AArch64::USHLv2i32:
  case AArch64::USHLv4i32:
  case AArch64::USHLv8i8:
  case AArch64::USHLv2i64:
  case AArch64::SSHLv1i64:
  case AArch64::SSHLv4i16:
  case AArch64::SSHLv16i8:
  case AArch64::SSHLv8i16:
  case AArch64::SSHLv2i32:
  case AArch64::SSHLv4i32:
  case AArch64::SSHLv8i8:
  case AArch64::SSHLv2i64:
  case AArch64::SSHLLv8i8_shift:
  case AArch64::SSHLLv4i16_shift:
  case AArch64::SSHLLv2i32_shift:
  case AArch64::SSHLLv4i32_shift:
  case AArch64::SSHLLv8i16_shift:
  case AArch64::SSHLLv16i8_shift:
  case AArch64::USHLLv8i8_shift:
  case AArch64::USHLLv4i32_shift:
  case AArch64::USHLLv8i16_shift:
  case AArch64::USHLLv16i8_shift:
  case AArch64::USHLLv4i16_shift:
  case AArch64::USHLLv2i32_shift: {
    unsigned op1Size = 0;
    switch (opcode) {
    case AArch64::UADDWv8i8_v8i16:
    case AArch64::UADDWv4i16_v4i32:
    case AArch64::UADDWv2i32_v2i64:
    case AArch64::USUBWv8i8_v8i16:
    case AArch64::USUBWv4i16_v4i32:
    case AArch64::USUBWv2i32_v2i64:
    case AArch64::SADDWv8i8_v8i16:
    case AArch64::SADDWv4i16_v4i32:
    case AArch64::SADDWv2i32_v2i64:
    case AArch64::SSUBWv8i8_v8i16:
    case AArch64::SSUBWv4i16_v4i32:
    case AArch64::SSUBWv2i32_v2i64: {
      op1Size = 128;
      break;
    }
    }
    auto a = readFromOperand(1, op1Size);
    auto b = readFromOperand(2);
    bool elementWise = false;
    bool splatImm2 = false;
    extKind ext = extKind::None;
    bool immShift = false;
    bool isUpper = false;
    function<Value *(Value *, Value *)> op;
    switch (opcode) {
    case AArch64::SMINv8i8:
    case AArch64::SMINv4i16:
    case AArch64::SMINv2i32:
    case AArch64::SMINv16i8:
    case AArch64::SMINv8i16:
    case AArch64::SMINv4i32:
    case AArch64::SMINPv8i8:
    case AArch64::SMINPv4i16:
    case AArch64::SMINPv2i32:
    case AArch64::SMINPv16i8:
    case AArch64::SMINPv8i16:
    case AArch64::SMINPv4i32:
      op = [&](Value *a, Value *b) { return createSMin(a, b); };
      break;
    case AArch64::SMAXv8i8:
    case AArch64::SMAXv4i16:
    case AArch64::SMAXv2i32:
    case AArch64::SMAXv16i8:
    case AArch64::SMAXv8i16:
    case AArch64::SMAXv4i32:
    case AArch64::SMAXPv8i8:
    case AArch64::SMAXPv4i16:
    case AArch64::SMAXPv2i32:
    case AArch64::SMAXPv16i8:
    case AArch64::SMAXPv8i16:
    case AArch64::SMAXPv4i32:
      op = [&](Value *a, Value *b) { return createSMax(a, b); };
      break;
    case AArch64::UMINv8i8:
    case AArch64::UMINv4i16:
    case AArch64::UMINv2i32:
    case AArch64::UMINv16i8:
    case AArch64::UMINv8i16:
    case AArch64::UMINv4i32:
    case AArch64::UMINPv8i8:
    case AArch64::UMINPv4i16:
    case AArch64::UMINPv2i32:
    case AArch64::UMINPv16i8:
    case AArch64::UMINPv8i16:
    case AArch64::UMINPv4i32:
      op = [&](Value *a, Value *b) { return createUMin(a, b); };
      break;
    case AArch64::UMAXv8i8:
    case AArch64::UMAXv4i16:
    case AArch64::UMAXv2i32:
    case AArch64::UMAXv16i8:
    case AArch64::UMAXv8i16:
    case AArch64::UMAXv4i32:
    case AArch64::UMAXPv8i8:
    case AArch64::UMAXPv4i16:
    case AArch64::UMAXPv2i32:
    case AArch64::UMAXPv16i8:
    case AArch64::UMAXPv8i16:
    case AArch64::UMAXPv4i32:
      op = [&](Value *a, Value *b) { return createUMax(a, b); };
      break;
    case AArch64::UMULLv16i8_v8i16:
    case AArch64::UMULLv8i16_v4i32:
    case AArch64::UMULLv4i32_v2i64:
      // these three cases are UMULL2
      a = createRawLShr(a, getUnsignedIntConst(64, 128));
      b = createRawLShr(b, getUnsignedIntConst(64, 128));
    case AArch64::UMULLv2i32_v2i64:
    case AArch64::UMULLv8i8_v8i16:
    case AArch64::UMULLv4i16_v4i32:
      ext = extKind::ZExt;
      op = [&](Value *a, Value *b) { return createMul(a, b); };
      break;
    case AArch64::SMULLv16i8_v8i16:
    case AArch64::SMULLv4i32_v2i64:
    case AArch64::SMULLv8i16_v4i32:
      // these three cases are SMULL2
      a = createRawLShr(a, getUnsignedIntConst(64, 128));
      b = createRawLShr(b, getUnsignedIntConst(64, 128));
    case AArch64::SMULLv8i8_v8i16:
    case AArch64::SMULLv2i32_v2i64:
    case AArch64::SMULLv4i16_v4i32:
      ext = extKind::SExt;
      op = [&](Value *a, Value *b) { return createMul(a, b); };
      break;
    case AArch64::USHRv8i8_shift:
    case AArch64::USHRv4i16_shift:
    case AArch64::USHRv2i32_shift:
    case AArch64::USHRv16i8_shift:
    case AArch64::USHRv8i16_shift:
    case AArch64::USHRv4i32_shift:
    case AArch64::USHRd:
      splatImm2 = true;
      op = [&](Value *a, Value *b) { return createMaskedLShr(a, b); };
      break;
    case AArch64::MULv2i32:
    case AArch64::MULv8i8:
    case AArch64::MULv4i16:
    case AArch64::MULv16i8:
    case AArch64::MULv8i16:
    case AArch64::MULv4i32:
      op = [&](Value *a, Value *b) { return createMul(a, b); };
      break;
    case AArch64::SSHRv8i8_shift:
    case AArch64::SSHRv16i8_shift:
    case AArch64::SSHRv4i16_shift:
    case AArch64::SSHRv8i16_shift:
    case AArch64::SSHRv2i32_shift:
    case AArch64::SSHRv4i32_shift:
    case AArch64::SSHRd:
    case AArch64::SSHRv2i64_shift:
      splatImm2 = true;
      op = [&](Value *a, Value *b) { return createMaskedAShr(a, b); };
      break;
    case AArch64::SHLv16i8_shift:
    case AArch64::SHLv8i16_shift:
    case AArch64::SHLv4i32_shift:
    case AArch64::SHLv2i64_shift:
    case AArch64::SHLv8i8_shift:
    case AArch64::SHLv4i16_shift:
    case AArch64::SHLd:
    case AArch64::SHLv2i32_shift:
      splatImm2 = true;
      op = [&](Value *a, Value *b) { return createMaskedShl(a, b); };
      break;
    case AArch64::BICv4i16:
    case AArch64::BICv8i8:
    case AArch64::BICv2i32:
    case AArch64::BICv8i16:
    case AArch64::BICv4i32:
    case AArch64::BICv16i8:
      if (CurInst->getOperand(2).isImm()) {
        splatImm2 = true;
        immShift = true;
      }
      op = [&](Value *a, Value *b) { return createAnd(a, createNot(b)); };
      break;
    case AArch64::USHLv1i64:
    case AArch64::USHLv4i16:
    case AArch64::USHLv16i8:
    case AArch64::USHLv8i16:
    case AArch64::USHLv2i32:
    case AArch64::USHLv4i32:
    case AArch64::USHLv8i8:
    case AArch64::USHLv2i64:
      op = [&](Value *a, Value *b) { return createUSHL(a, b); };
      elementWise = true;
      break;
    case AArch64::SSHLv1i64:
    case AArch64::SSHLv4i16:
    case AArch64::SSHLv16i8:
    case AArch64::SSHLv8i16:
    case AArch64::SSHLv2i32:
    case AArch64::SSHLv4i32:
    case AArch64::SSHLv8i8:
    case AArch64::SSHLv2i64:
      op = [&](Value *a, Value *b) { return createSSHL(a, b); };
      elementWise = true;
      break;
    case AArch64::ADDv2i32:
    case AArch64::ADDv1i64:
    case AArch64::ADDv2i64:
    case AArch64::ADDv4i16:
    case AArch64::ADDv4i32:
    case AArch64::ADDv8i8:
    case AArch64::ADDv8i16:
    case AArch64::ADDv16i8:
      op = [&](Value *a, Value *b) { return createAdd(a, b); };
      break;
    case AArch64::UADDLv16i8_v8i16:
    case AArch64::UADDLv8i16_v4i32:
    case AArch64::UADDLv4i32_v2i64:
    case AArch64::UADDWv16i8_v8i16:
    case AArch64::UADDWv8i16_v4i32:
    case AArch64::UADDWv4i32_v2i64:
    case AArch64::SADDLv16i8_v8i16:
    case AArch64::SADDLv8i16_v4i32:
    case AArch64::SADDLv4i32_v2i64:
    case AArch64::SADDWv16i8_v8i16:
    case AArch64::SADDWv8i16_v4i32:
    case AArch64::SADDWv4i32_v2i64:
      // These cases are UADDL2, UADDW2, SADDL2, SADDW2
      isUpper = true;
    case AArch64::UADDLv8i8_v8i16:
    case AArch64::UADDLv4i16_v4i32:
    case AArch64::UADDLv2i32_v2i64:
    case AArch64::UADDWv8i8_v8i16:
    case AArch64::UADDWv4i16_v4i32:
    case AArch64::UADDWv2i32_v2i64:
    case AArch64::SADDLv8i8_v8i16:
    case AArch64::SADDLv4i16_v4i32:
    case AArch64::SADDLv2i32_v2i64:
    case AArch64::SADDWv8i8_v8i16:
    case AArch64::SADDWv4i16_v4i32:
    case AArch64::SADDWv2i32_v2i64:
      if (opcode == AArch64::SADDLv8i8_v8i16 ||
          opcode == AArch64::SADDLv16i8_v8i16 ||
          opcode == AArch64::SADDLv4i16_v4i32 ||
          opcode == AArch64::SADDLv8i16_v4i32 ||
          opcode == AArch64::SADDLv2i32_v2i64 ||
          opcode == AArch64::SADDLv4i32_v2i64 ||
          opcode == AArch64::SADDWv8i8_v8i16 ||
          opcode == AArch64::SADDWv16i8_v8i16 ||
          opcode == AArch64::SADDWv4i16_v4i32 ||
          opcode == AArch64::SADDWv8i16_v4i32 ||
          opcode == AArch64::SADDWv2i32_v2i64 ||
          opcode == AArch64::SADDWv4i32_v2i64) {
        ext = extKind::SExt;
      } else {
        ext = extKind::ZExt;
      }
      op = [&](Value *a, Value *b) { return createAdd(a, b); };
      break;
    case AArch64::USUBLv16i8_v8i16:
    case AArch64::USUBLv8i16_v4i32:
    case AArch64::USUBLv4i32_v2i64:
    case AArch64::USUBWv16i8_v8i16:
    case AArch64::USUBWv8i16_v4i32:
    case AArch64::USUBWv4i32_v2i64:
    case AArch64::SSUBLv16i8_v8i16:
    case AArch64::SSUBLv8i16_v4i32:
    case AArch64::SSUBLv4i32_v2i64:
    case AArch64::SSUBWv16i8_v8i16:
    case AArch64::SSUBWv8i16_v4i32:
    case AArch64::SSUBWv4i32_v2i64:
      // These three cases are USUBL2, SSUBL2, USUBW2 and SSUBW2
      isUpper = true;
    case AArch64::USUBLv8i8_v8i16:
    case AArch64::USUBLv4i16_v4i32:
    case AArch64::USUBLv2i32_v2i64:
    case AArch64::USUBWv8i8_v8i16:
    case AArch64::USUBWv4i16_v4i32:
    case AArch64::USUBWv2i32_v2i64:
    case AArch64::SSUBLv8i8_v8i16:
    case AArch64::SSUBLv4i16_v4i32:
    case AArch64::SSUBLv2i32_v2i64:
    case AArch64::SSUBWv8i8_v8i16:
    case AArch64::SSUBWv4i16_v4i32:
    case AArch64::SSUBWv2i32_v2i64:
      if (opcode == AArch64::SSUBLv8i8_v8i16 ||
          opcode == AArch64::SSUBLv16i8_v8i16 ||
          opcode == AArch64::SSUBLv4i16_v4i32 ||
          opcode == AArch64::SSUBLv8i16_v4i32 ||
          opcode == AArch64::SSUBLv2i32_v2i64 ||
          opcode == AArch64::SSUBLv4i32_v2i64 ||
          opcode == AArch64::SSUBWv8i8_v8i16 ||
          opcode == AArch64::SSUBWv16i8_v8i16 ||
          opcode == AArch64::SSUBWv4i16_v4i32 ||
          opcode == AArch64::SSUBWv8i16_v4i32 ||
          opcode == AArch64::SSUBWv2i32_v2i64 ||
          opcode == AArch64::SSUBWv4i32_v2i64) {
        ext = extKind::SExt;
      } else {
        ext = extKind::ZExt;
      }
      op = [&](Value *a, Value *b) { return createSub(a, b); };
      break;
    case AArch64::SUBv1i64:
    case AArch64::SUBv2i32:
    case AArch64::SUBv2i64:
    case AArch64::SUBv4i16:
    case AArch64::SUBv4i32:
    case AArch64::SUBv8i8:
    case AArch64::SUBv8i16:
    case AArch64::SUBv16i8:
      op = [&](Value *a, Value *b) { return createSub(a, b); };
      break;
    case AArch64::EORv8i8:
    case AArch64::EORv16i8:
      op = [&](Value *a, Value *b) { return createXor(a, b); };
      break;
    case AArch64::ANDv8i8:
    case AArch64::ANDv16i8:
      op = [&](Value *a, Value *b) { return createAnd(a, b); };
      break;
    case AArch64::ORNv8i8:
    case AArch64::ORNv16i8:
      op = [&](Value *a, Value *b) { return createOr(a, createNot(b)); };
      break;
    case AArch64::ORRv8i8:
    case AArch64::ORRv16i8:
    case AArch64::ORRv2i32:
    case AArch64::ORRv4i16:
    case AArch64::ORRv8i16:
    case AArch64::ORRv4i32:
      if (CurInst->getOperand(2).isImm()) {
        splatImm2 = true;
        immShift = true;
      }
      op = [&](Value *a, Value *b) { return createOr(a, b); };
      break;
    case AArch64::SSHLLv4i32_shift:
    case AArch64::SSHLLv8i16_shift:
    case AArch64::SSHLLv16i8_shift:
      // these three cases are SSHLL2
      a = createRawLShr(a, getUnsignedIntConst(64, 128));
    case AArch64::SSHLLv8i8_shift:
    case AArch64::SSHLLv4i16_shift:
    case AArch64::SSHLLv2i32_shift:
      ext = extKind::SExt;
      splatImm2 = true;
      op = [&](Value *a, Value *b) { return createRawShl(a, b); };
      break;
    case AArch64::USHLLv4i32_shift:
    case AArch64::USHLLv8i16_shift:
    case AArch64::USHLLv16i8_shift:
      // these three cases are USHLL2
      a = createRawLShr(a, getUnsignedIntConst(64, 128));
    case AArch64::USHLLv4i16_shift:
    case AArch64::USHLLv2i32_shift:
    case AArch64::USHLLv8i8_shift:
      ext = extKind::ZExt;
      splatImm2 = true;
      op = [&](Value *a, Value *b) { return createRawShl(a, b); };
      break;
    case AArch64::USHRv2i64_shift:
      splatImm2 = true;
      op = [&](Value *a, Value *b) { return createRawLShr(a, b); };
      break;
    default:
      assert(false && "missed a case");
    }

    unsigned eltSize, numElts;
    switch (opcode) {
    case AArch64::USHLv1i64:
    case AArch64::USHRd:
    case AArch64::SSHRd:
    case AArch64::SSHLv1i64:
    case AArch64::SHLd:
    case AArch64::ADDv1i64:
    case AArch64::SUBv1i64:
      numElts = 1;
      eltSize = 64;
      break;
    case AArch64::ORRv2i32:
    case AArch64::UMULLv2i32_v2i64:
    case AArch64::SMINv2i32:
    case AArch64::SMAXv2i32:
    case AArch64::UMINv2i32:
    case AArch64::UMAXv2i32:
    case AArch64::SMINPv2i32:
    case AArch64::SMAXPv2i32:
    case AArch64::UMINPv2i32:
    case AArch64::UMAXPv2i32:
    case AArch64::SMULLv2i32_v2i64:
    case AArch64::USHRv2i32_shift:
    case AArch64::MULv2i32:
    case AArch64::SSHLLv2i32_shift:
    case AArch64::SSHRv2i32_shift:
    case AArch64::SHLv2i32_shift:
    case AArch64::SUBv2i32:
    case AArch64::ADDv2i32:
    case AArch64::USHLv2i32:
    case AArch64::SSHLv2i32:
    case AArch64::BICv2i32:
    case AArch64::USHLLv2i32_shift:
    case AArch64::UADDLv2i32_v2i64:
    case AArch64::UADDWv2i32_v2i64:
    case AArch64::SADDLv2i32_v2i64:
    case AArch64::SADDWv2i32_v2i64:
    case AArch64::USUBLv2i32_v2i64:
    case AArch64::USUBWv2i32_v2i64:
    case AArch64::SSUBLv2i32_v2i64:
    case AArch64::SSUBWv2i32_v2i64:
      numElts = 2;
      eltSize = 32;
      break;
    case AArch64::SSHRv2i64_shift:
    case AArch64::SHLv2i64_shift:
    case AArch64::ADDv2i64:
    case AArch64::SUBv2i64:
    case AArch64::USHLv2i64:
    case AArch64::SSHLv2i64:
    case AArch64::USHRv2i64_shift:
      numElts = 2;
      eltSize = 64;
      break;
    case AArch64::ORRv4i16:
    case AArch64::UMULLv4i16_v4i32:
    case AArch64::SMINv4i16:
    case AArch64::SMAXv4i16:
    case AArch64::UMINv4i16:
    case AArch64::UMAXv4i16:
    case AArch64::SMINPv4i16:
    case AArch64::SMAXPv4i16:
    case AArch64::UMINPv4i16:
    case AArch64::UMAXPv4i16:
    case AArch64::SMULLv4i16_v4i32:
    case AArch64::USHRv4i16_shift:
    case AArch64::SSHLLv4i16_shift:
    case AArch64::SSHRv4i16_shift:
    case AArch64::ADDv4i16:
    case AArch64::SUBv4i16:
    case AArch64::USHLv4i16:
    case AArch64::SSHLv4i16:
    case AArch64::BICv4i16:
    case AArch64::USHLLv4i16_shift:
    case AArch64::SHLv4i16_shift:
    case AArch64::MULv4i16:
    case AArch64::UADDLv4i16_v4i32:
    case AArch64::UADDWv4i16_v4i32:
    case AArch64::SADDLv4i16_v4i32:
    case AArch64::SADDWv4i16_v4i32:
    case AArch64::USUBLv4i16_v4i32:
    case AArch64::USUBWv4i16_v4i32:
    case AArch64::SSUBLv4i16_v4i32:
    case AArch64::SSUBWv4i16_v4i32:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::UMULLv4i32_v2i64:
    case AArch64::SMINv4i32:
    case AArch64::SMAXv4i32:
    case AArch64::UMINv4i32:
    case AArch64::UMAXv4i32:
    case AArch64::SMINPv4i32:
    case AArch64::SMAXPv4i32:
    case AArch64::UMINPv4i32:
    case AArch64::UMAXPv4i32:
    case AArch64::SMULLv4i32_v2i64:
    case AArch64::USHRv4i32_shift:
    case AArch64::MULv4i32:
    case AArch64::SSHLLv4i32_shift:
    case AArch64::SSHRv4i32_shift:
    case AArch64::SHLv4i32_shift:
    case AArch64::ADDv4i32:
    case AArch64::SUBv4i32:
    case AArch64::USHLv4i32:
    case AArch64::SSHLv4i32:
    case AArch64::BICv4i32:
    case AArch64::USHLLv4i32_shift:
    case AArch64::ORRv4i32:
    case AArch64::UADDLv4i32_v2i64:
    case AArch64::UADDWv4i32_v2i64:
    case AArch64::SADDLv4i32_v2i64:
    case AArch64::SADDWv4i32_v2i64:
    case AArch64::USUBLv4i32_v2i64:
    case AArch64::USUBWv4i32_v2i64:
    case AArch64::SSUBLv4i32_v2i64:
    case AArch64::SSUBWv4i32_v2i64:
      numElts = 4;
      eltSize = 32;
      break;
    case AArch64::ORNv8i8:
    case AArch64::UMULLv8i8_v8i16:
    case AArch64::SMINv8i8:
    case AArch64::SMAXv8i8:
    case AArch64::UMINv8i8:
    case AArch64::UMAXv8i8:
    case AArch64::SMINPv8i8:
    case AArch64::SMAXPv8i8:
    case AArch64::UMINPv8i8:
    case AArch64::UMAXPv8i8:
    case AArch64::SMULLv8i8_v8i16:
    case AArch64::MULv8i8:
    case AArch64::SSHLLv8i8_shift:
    case AArch64::SSHRv8i8_shift:
    case AArch64::SHLv8i8_shift:
    case AArch64::ADDv8i8:
    case AArch64::SUBv8i8:
    case AArch64::EORv8i8:
    case AArch64::ANDv8i8:
    case AArch64::ORRv8i8:
    case AArch64::USHLv8i8:
    case AArch64::SSHLv8i8:
    case AArch64::BICv8i8:
    case AArch64::USHRv8i8_shift:
    case AArch64::USHLLv8i8_shift:
    case AArch64::UADDLv8i8_v8i16:
    case AArch64::UADDWv8i8_v8i16:
    case AArch64::SADDLv8i8_v8i16:
    case AArch64::SADDWv8i8_v8i16:
    case AArch64::USUBLv8i8_v8i16:
    case AArch64::USUBWv8i8_v8i16:
    case AArch64::SSUBLv8i8_v8i16:
    case AArch64::SSUBWv8i8_v8i16:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::ORRv8i16:
    case AArch64::UMULLv8i16_v4i32:
    case AArch64::SMULLv8i16_v4i32:
    case AArch64::USHRv8i16_shift:
    case AArch64::MULv8i16:
    case AArch64::SSHLLv8i16_shift:
    case AArch64::ADDv8i16:
    case AArch64::SUBv8i16:
    case AArch64::USHLv8i16:
    case AArch64::SSHLv8i16:
    case AArch64::BICv8i16:
    case AArch64::USHLLv8i16_shift:
    case AArch64::SHLv8i16_shift:
    case AArch64::SSHRv8i16_shift:
    case AArch64::SMINv8i16:
    case AArch64::SMAXv8i16:
    case AArch64::UMINv8i16:
    case AArch64::UMAXv8i16:
    case AArch64::SMINPv8i16:
    case AArch64::SMAXPv8i16:
    case AArch64::UMINPv8i16:
    case AArch64::UMAXPv8i16:
    case AArch64::UADDLv8i16_v4i32:
    case AArch64::UADDWv8i16_v4i32:
    case AArch64::SADDLv8i16_v4i32:
    case AArch64::SADDWv8i16_v4i32:
    case AArch64::USUBLv8i16_v4i32:
    case AArch64::USUBWv8i16_v4i32:
    case AArch64::SSUBLv8i16_v4i32:
    case AArch64::SSUBWv8i16_v4i32:
      numElts = 8;
      eltSize = 16;
      break;
    case AArch64::ORNv16i8:
    case AArch64::UMULLv16i8_v8i16:
    case AArch64::SMINv16i8:
    case AArch64::SMAXv16i8:
    case AArch64::UMINv16i8:
    case AArch64::UMAXv16i8:
    case AArch64::SMINPv16i8:
    case AArch64::SMAXPv16i8:
    case AArch64::UMINPv16i8:
    case AArch64::UMAXPv16i8:
    case AArch64::SMULLv16i8_v8i16:
    case AArch64::USHRv16i8_shift:
    case AArch64::MULv16i8:
    case AArch64::SSHLLv16i8_shift:
    case AArch64::ADDv16i8:
    case AArch64::SUBv16i8:
    case AArch64::EORv16i8:
    case AArch64::ANDv16i8:
    case AArch64::ORRv16i8:
    case AArch64::USHLv16i8:
    case AArch64::SSHLv16i8:
    case AArch64::BICv16i8:
    case AArch64::USHLLv16i8_shift:
    case AArch64::SHLv16i8_shift:
    case AArch64::SSHRv16i8_shift:
    case AArch64::UADDLv16i8_v8i16:
    case AArch64::UADDWv16i8_v8i16:
    case AArch64::SADDLv16i8_v8i16:
    case AArch64::SADDWv16i8_v8i16:
    case AArch64::USUBLv16i8_v8i16:
    case AArch64::USUBWv16i8_v8i16:
    case AArch64::SSUBLv16i8_v8i16:
    case AArch64::SSUBWv16i8_v8i16:
      numElts = 16;
      eltSize = 8;
      break;
    default:
      assert(false && "missed case");
      break;
    }

    // Preprocessing the two operands
    switch (opcode) {
    case AArch64::SMINPv8i8:
    case AArch64::SMINPv4i16:
    case AArch64::SMINPv2i32:
    case AArch64::SMINPv16i8:
    case AArch64::SMINPv8i16:
    case AArch64::SMINPv4i32:
    case AArch64::SMAXPv8i8:
    case AArch64::SMAXPv4i16:
    case AArch64::SMAXPv2i32:
    case AArch64::SMAXPv16i8:
    case AArch64::SMAXPv8i16:
    case AArch64::SMAXPv4i32:
    case AArch64::UMINPv8i8:
    case AArch64::UMINPv4i16:
    case AArch64::UMINPv2i32:
    case AArch64::UMINPv16i8:
    case AArch64::UMINPv8i16:
    case AArch64::UMINPv4i32:
    case AArch64::UMAXPv8i8:
    case AArch64::UMAXPv4i16:
    case AArch64::UMAXPv2i32:
    case AArch64::UMAXPv16i8:
    case AArch64::UMAXPv8i16:
    case AArch64::UMAXPv4i32: {
      vector<int> mask1(numElts), mask2(numElts);
      for (unsigned i = 0; i < numElts; i++) {
        mask1[i] = 2 * i;
        mask2[i] = 2 * i + 1;
      }
      auto vector_a = createBitCast(a, getVecTy(eltSize, numElts));
      auto vector_b = createBitCast(b, getVecTy(eltSize, numElts));

      auto new_a = createShuffleVector(vector_b, vector_a, mask1);
      auto new_b = createShuffleVector(vector_b, vector_a, mask2);

      a = createBitCast(new_a, getIntTy(eltSize * numElts));
      b = createBitCast(new_b, getIntTy(eltSize * numElts));
      break;
    }
    }

    // Some instructions have first operand of a different type than the
    // second operand (Twice the eltSize. Also half the number of elements if
    // isUpper)
    bool operandTypesDiffer = false;
    switch (opcode) {
    case AArch64::UADDWv8i8_v8i16:
    case AArch64::UADDWv16i8_v8i16:
    case AArch64::UADDWv4i16_v4i32:
    case AArch64::UADDWv8i16_v4i32:
    case AArch64::UADDWv2i32_v2i64:
    case AArch64::UADDWv4i32_v2i64:
    case AArch64::SADDWv8i8_v8i16:
    case AArch64::SADDWv16i8_v8i16:
    case AArch64::SADDWv4i16_v4i32:
    case AArch64::SADDWv8i16_v4i32:
    case AArch64::SADDWv2i32_v2i64:
    case AArch64::SADDWv4i32_v2i64:
    case AArch64::USUBWv8i8_v8i16:
    case AArch64::USUBWv16i8_v8i16:
    case AArch64::USUBWv4i16_v4i32:
    case AArch64::USUBWv8i16_v4i32:
    case AArch64::USUBWv2i32_v2i64:
    case AArch64::USUBWv4i32_v2i64:
    case AArch64::SSUBWv8i8_v8i16:
    case AArch64::SSUBWv16i8_v8i16:
    case AArch64::SSUBWv4i16_v4i32:
    case AArch64::SSUBWv8i16_v4i32:
    case AArch64::SSUBWv2i32_v2i64:
    case AArch64::SSUBWv4i32_v2i64:
      operandTypesDiffer = true;
      break;
    }

    auto res = createVectorOp(op, a, b, eltSize, numElts, elementWise, ext,
                              splatImm2, immShift, isUpper, operandTypesDiffer);
    updateOutputReg(res);
    break;
  }

  case AArch64::ADDPv2i64p: {
    auto vTy = getVecTy(64, 2);
    auto v = createBitCast(readFromOperand(1), vTy);
    auto res =
        createAdd(createExtractElement(v, 0), createExtractElement(v, 1));
    updateOutputReg(res);
    break;
  }

  case AArch64::UZP1v8i8:
  case AArch64::UZP1v4i16:
  case AArch64::UZP1v16i8:
  case AArch64::UZP1v8i16:
  case AArch64::UZP1v4i32:
  case AArch64::UZP2v8i8:
  case AArch64::UZP2v4i16:
  case AArch64::UZP2v8i16:
  case AArch64::UZP2v16i8:
  case AArch64::UZP2v4i32: {
    int which;
    switch (opcode) {
    case AArch64::UZP1v8i8:
    case AArch64::UZP1v4i16:
    case AArch64::UZP1v16i8:
    case AArch64::UZP1v8i16:
    case AArch64::UZP1v4i32:
      which = 0;
      break;
    case AArch64::UZP2v8i8:
    case AArch64::UZP2v4i16:
    case AArch64::UZP2v8i16:
    case AArch64::UZP2v16i8:
    case AArch64::UZP2v4i32:
      which = 1;
      break;
    default:
      assert(false);
    }
    int numElts, eltSize;
    if (opcode == AArch64::UZP1v8i8 || opcode == AArch64::UZP2v8i8) {
      numElts = 8;
      eltSize = 8;
    } else if (opcode == AArch64::UZP1v4i16 || opcode == AArch64::UZP2v4i16) {
      numElts = 4;
      eltSize = 16;
    } else if (opcode == AArch64::UZP1v16i8 || opcode == AArch64::UZP2v16i8) {
      numElts = 16;
      eltSize = 8;
    } else if (opcode == AArch64::UZP1v8i16 || opcode == AArch64::UZP2v8i16) {
      numElts = 8;
      eltSize = 16;
    } else if (opcode == AArch64::UZP1v4i32 || opcode == AArch64::UZP2v4i32) {
      numElts = 4;
      eltSize = 32;
    } else {
      assert(false);
    }
    auto vTy = getVecTy(eltSize, numElts);
    auto a = createBitCast(readFromOperand(1), vTy);
    auto b = createBitCast(readFromOperand(2), vTy);
    Value *res = getUndefVec(numElts, eltSize);
    for (int i = 0; i < numElts / 2; ++i) {
      auto e1 = createExtractElement(a, (i * 2) + which);
      auto e2 = createExtractElement(b, (i * 2) + which);
      res = createInsertElement(res, e1, i);
      res = createInsertElement(res, e2, i + (numElts / 2));
    }
    updateOutputReg(res);
    break;
  }

#define GET_SIZES4(INSN, SUFF)                                                 \
  if (opcode == AArch64::INSN##v2i32##SUFF) {                                  \
    numElts = 2;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v4i16##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v4i32##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v8i16##SUFF) {                           \
    numElts = 8;                                                               \
    eltSize = 16;                                                              \
  } else {                                                                     \
    assert(false);                                                             \
  }

  case AArch64::MULv8i16_indexed:
  case AArch64::MULv4i32_indexed:
  case AArch64::MULv2i32_indexed:
  case AArch64::MULv4i16_indexed: {
    unsigned eltSize, numElts;
    GET_SIZES4(MUL, _indexed);
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto e2 =
        getIndexedElement(getImm(3), eltSize, CurInst->getOperand(2).getReg());
    Value *res = getUndefVec(numElts, eltSize);
    for (unsigned i = 0; i < numElts; ++i) {
      auto e1 = createExtractElement(a, i);
      res = createInsertElement(res, createMul(e1, e2), i);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::MLSv2i32_indexed:
  case AArch64::MLSv4i16_indexed:
  case AArch64::MLSv8i16_indexed:
  case AArch64::MLSv4i32_indexed: {
    int numElts, eltSize;
    GET_SIZES4(MLS, _indexed);
    auto vTy = getVecTy(eltSize, numElts);
    auto a = createBitCast(readFromOperand(1), vTy);
    auto b = createBitCast(readFromOperand(2), vTy);
    auto e =
        getIndexedElement(getImm(4), eltSize, CurInst->getOperand(3).getReg());
    auto spl = splat(e, numElts, eltSize);
    auto mul = createMul(b, spl);
    auto sum = createSub(a, mul);
    updateOutputReg(sum);
    break;
  }

  case AArch64::MLAv2i32_indexed:
  case AArch64::MLAv4i16_indexed:
  case AArch64::MLAv8i16_indexed:
  case AArch64::MLAv4i32_indexed: {
    int numElts, eltSize;
    GET_SIZES4(MLA, _indexed);
    auto vTy = getVecTy(eltSize, numElts);
    auto a = createBitCast(readFromOperand(1), vTy);
    auto b = createBitCast(readFromOperand(2), vTy);
    auto e =
        getIndexedElement(getImm(4), eltSize, CurInst->getOperand(3).getReg());
    auto spl = splat(e, numElts, eltSize);
    auto mul = createMul(b, spl);
    auto sum = createAdd(mul, a);
    updateOutputReg(sum);
    break;
  }

#define GET_SIZES5(INSN, SUFF)                                                 \
  if (opcode == AArch64::INSN##v8i8##SUFF) {                                   \
    numElts = 8;                                                               \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i16##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v16i8##SUFF) {                           \
    numElts = 16;                                                              \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i32##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v8i16##SUFF) {                           \
    numElts = 8;                                                               \
    eltSize = 16;                                                              \
  }

  case AArch64::TRN1v16i8:
  case AArch64::TRN1v8i16:
  case AArch64::TRN1v4i32:
  case AArch64::TRN1v4i16:
  case AArch64::TRN1v8i8:
  case AArch64::TRN2v8i8:
  case AArch64::TRN2v4i16:
  case AArch64::TRN2v16i8:
  case AArch64::TRN2v8i16:
  case AArch64::TRN2v4i32: {
    int numElts = -1, eltSize = -1;
    GET_SIZES5(TRN1, );
    GET_SIZES5(TRN2, );
    assert(numElts != -1 && eltSize != -1);
    int part;
    switch (opcode) {
    case AArch64::TRN1v16i8:
    case AArch64::TRN1v8i16:
    case AArch64::TRN1v4i32:
    case AArch64::TRN1v4i16:
    case AArch64::TRN1v8i8:
      part = 0;
      break;
    case AArch64::TRN2v8i8:
    case AArch64::TRN2v4i16:
    case AArch64::TRN2v16i8:
    case AArch64::TRN2v8i16:
    case AArch64::TRN2v4i32:
      part = 1;
      break;
    default:
      assert(false);
    }
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto b = readFromVecOperand(2, eltSize, numElts);
    Value *res = getUndefVec(numElts, eltSize);
    for (int p = 0; p < numElts / 2; ++p) {
      auto *e1 = createExtractElement(a, (2 * p) + part);
      auto *e2 = createExtractElement(b, (2 * p) + part);
      res = createInsertElement(res, e1, 2 * p);
      res = createInsertElement(res, e2, (2 * p) + 1);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::SMINVv8i8v:
  case AArch64::UMINVv8i8v:
  case AArch64::SMAXVv8i8v:
  case AArch64::UMAXVv8i8v:
  case AArch64::SMINVv4i16v:
  case AArch64::UMINVv4i16v:
  case AArch64::SMAXVv4i16v:
  case AArch64::UMAXVv4i16v:
  case AArch64::SMAXVv4i32v:
  case AArch64::UMAXVv4i32v:
  case AArch64::SMINVv4i32v:
  case AArch64::UMINVv4i32v:
  case AArch64::UMINVv8i16v:
  case AArch64::SMINVv8i16v:
  case AArch64::UMAXVv8i16v:
  case AArch64::SMAXVv8i16v:
  case AArch64::SMINVv16i8v:
  case AArch64::UMINVv16i8v:
  case AArch64::SMAXVv16i8v:
  case AArch64::UMAXVv16i8v: {
    int numElts = -1, eltSize = -1;
    GET_SIZES5(SMINV, v);
    GET_SIZES5(SMAXV, v);
    GET_SIZES5(UMINV, v);
    GET_SIZES5(UMAXV, v);
    assert(numElts != -1 && eltSize != -1);
    auto v = readFromVecOperand(1, eltSize, numElts);
    Value *min = createExtractElement(v, 0);
    for (int i = 1; i < numElts; ++i) {
      auto e = createExtractElement(v, i);
      auto p{ICmpInst::Predicate::BAD_ICMP_PREDICATE};
      switch (opcode) {
      case AArch64::SMINVv8i8v:
      case AArch64::SMINVv4i16v:
      case AArch64::SMINVv8i16v:
      case AArch64::SMINVv16i8v:
      case AArch64::SMINVv4i32v:
        p = ICmpInst::Predicate::ICMP_SLT;
        break;
      case AArch64::UMINVv8i8v:
      case AArch64::UMINVv4i16v:
      case AArch64::UMINVv4i32v:
      case AArch64::UMINVv8i16v:
      case AArch64::UMINVv16i8v:
        p = ICmpInst::Predicate::ICMP_ULT;
        break;
      case AArch64::SMAXVv8i8v:
      case AArch64::SMAXVv4i16v:
      case AArch64::SMAXVv8i16v:
      case AArch64::SMAXVv16i8v:
      case AArch64::SMAXVv4i32v:
        p = ICmpInst::Predicate::ICMP_SGT;
        break;
      case AArch64::UMAXVv4i16v:
      case AArch64::UMAXVv4i32v:
      case AArch64::UMAXVv8i16v:
      case AArch64::UMAXVv8i8v:
      case AArch64::UMAXVv16i8v:
        p = ICmpInst::Predicate::ICMP_UGT;
        break;
      default:
        assert(false);
      }
      auto c = createICmp(p, min, e);
      min = createSelect(c, min, e);
    }
    updateOutputReg(min);
    break;
  }

#define GET_SIZES6(INSN, SUFF)                                                 \
  if (opcode == AArch64::INSN##v8i8##SUFF) {                                   \
    numElts = 8;                                                               \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i16##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v2i32##SUFF) {                           \
    numElts = 2;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v16i8##SUFF) {                           \
    numElts = 16;                                                              \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i32##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v8i16##SUFF) {                           \
    numElts = 8;                                                               \
    eltSize = 16;                                                              \
  }

  case AArch64::SHLLv8i8:
  case AArch64::SHLLv16i8:
  case AArch64::SHLLv4i16:
  case AArch64::SHLLv8i16:
  case AArch64::SHLLv2i32:
  case AArch64::SHLLv4i32: {
    unsigned eltSize = -1, numElts = -1;
    GET_SIZES6(SHLL, )

    bool isUpper = false;
    switch (opcode) {
    case AArch64::SHLLv16i8:
    case AArch64::SHLLv8i16:
    case AArch64::SHLLv4i32:
      isUpper = true;
      break;
    }

    auto a = readFromVecOperand(1, eltSize, numElts, isUpper);
    Value *extended_a, *b;
    if (isUpper) {
      numElts /= 2;
      extended_a = createZExt(a, getVecTy(eltSize * 2, numElts));
      b = getElemSplat(numElts, 2 * eltSize, eltSize);
    } else {
      extended_a = createZExt(a, getVecTy(eltSize * 2, numElts));
      b = getElemSplat(numElts, 2 * eltSize, eltSize);
    }

    auto res = createMaskedShl(extended_a, b);
    updateOutputReg(res);
    break;
  }

  case AArch64::SHRNv8i8_shift:
  case AArch64::SHRNv16i8_shift:
  case AArch64::SHRNv4i16_shift:
  case AArch64::SHRNv8i16_shift:
  case AArch64::SHRNv2i32_shift:
  case AArch64::SHRNv4i32_shift:
  case AArch64::RSHRNv8i8_shift:
  case AArch64::RSHRNv16i8_shift:
  case AArch64::RSHRNv4i16_shift:
  case AArch64::RSHRNv8i16_shift:
  case AArch64::RSHRNv2i32_shift:
  case AArch64::RSHRNv4i32_shift: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES6(SHRN, _shift)
    GET_SIZES6(RSHRN, _shift)

    bool isUpper = false;
    switch (opcode) {
    case AArch64::SHRNv16i8_shift:
    case AArch64::SHRNv8i16_shift:
    case AArch64::SHRNv4i32_shift:
    case AArch64::RSHRNv16i8_shift:
    case AArch64::RSHRNv8i16_shift:
    case AArch64::RSHRNv4i32_shift:
      isUpper = true;
      break;
    }

    bool addRoundingConst = false;
    switch (opcode) {
    case AArch64::RSHRNv8i8_shift:
    case AArch64::RSHRNv16i8_shift:
    case AArch64::RSHRNv4i16_shift:
    case AArch64::RSHRNv8i16_shift:
    case AArch64::RSHRNv2i32_shift:
    case AArch64::RSHRNv4i32_shift:
      addRoundingConst = true;
      break;
    }

    Value *a, *b, *roundingConst;
    if (isUpper) {
      numElts /= 2;
      a = readFromVecOperand(2, eltSize * 2, numElts);
      roundingConst =
          getElemSplat(numElts, eltSize * 2, (uint64_t)1 << (getImm(3) - 1));
      b = getElemSplat(numElts, eltSize * 2, getImm(3));
    } else {
      a = readFromVecOperand(1, eltSize * 2, numElts);
      roundingConst =
          getElemSplat(numElts, eltSize * 2, (uint64_t)1 << (getImm(2) - 1));
      b = getElemSplat(numElts, eltSize * 2, getImm(2));
    }

    if (addRoundingConst) {
      a = createAdd(a, roundingConst);
    }

    auto shiftedVec = createMaskedLShr(a, b);
    Value *res = createTrunc(shiftedVec, getVecTy(eltSize, numElts));
    if (isUpper) {
      Value *dest = readFromVecOperand(1, 64, 2);
      Value *element = createBitCast(res, i64);
      res = createInsertElement(dest, element, 1);
    }

    updateOutputReg(res);
    break;
  }

  case AArch64::SLIv8i8_shift:
  case AArch64::SLIv16i8_shift:
  case AArch64::SLIv4i16_shift:
  case AArch64::SLIv8i16_shift:
  case AArch64::SLIv2i32_shift:
  case AArch64::SLIv4i32_shift:
  case AArch64::SLId:
  case AArch64::SLIv2i64_shift:
  case AArch64::SRIv8i8_shift:
  case AArch64::SRIv16i8_shift:
  case AArch64::SRIv4i16_shift:
  case AArch64::SRIv8i16_shift:
  case AArch64::SRIv2i32_shift:
  case AArch64::SRIv4i32_shift:
  case AArch64::SRId:
  case AArch64::SRIv2i64_shift: {
    unsigned numElts = -1, eltSize = -1;
    switch (opcode) {
    case AArch64::SLIv8i8_shift:
    case AArch64::SRIv8i8_shift:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::SLIv16i8_shift:
    case AArch64::SRIv16i8_shift:
      numElts = 16;
      eltSize = 8;
      break;
    case AArch64::SLIv4i16_shift:
    case AArch64::SRIv4i16_shift:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::SLIv8i16_shift:
    case AArch64::SRIv8i16_shift:
      numElts = 8;
      eltSize = 16;
      break;
    case AArch64::SLIv2i32_shift:
    case AArch64::SRIv2i32_shift:
      numElts = 2;
      eltSize = 32;
      break;
    case AArch64::SLIv4i32_shift:
    case AArch64::SRIv4i32_shift:
      numElts = 4;
      eltSize = 32;
      break;
    case AArch64::SLId:
    case AArch64::SRId:
      numElts = 1;
      eltSize = 64;
      break;
    case AArch64::SLIv2i64_shift:
    case AArch64::SRIv2i64_shift:
      numElts = 2;
      eltSize = 64;
      break;
    default:
      assert(false && "Invalid opcode for SLI");
      break;
    }

    auto shiftAmt = getImm(3);
    auto a = readFromVecOperand(2, eltSize, numElts);
    auto shiftVec = getElemSplat(numElts, eltSize, shiftAmt);
    Value *shifted_a;
    uint64_t maskAmt;
    switch (opcode) {
    case AArch64::SLIv8i8_shift:
    case AArch64::SLIv16i8_shift:
    case AArch64::SLIv4i16_shift:
    case AArch64::SLIv8i16_shift:
    case AArch64::SLIv2i32_shift:
    case AArch64::SLIv4i32_shift:
    case AArch64::SLId:
    case AArch64::SLIv2i64_shift:
      shifted_a = createMaskedShl(a, shiftVec);
      maskAmt = (1ULL << shiftAmt) - 1;
      break;
    case AArch64::SRIv8i8_shift:
    case AArch64::SRIv16i8_shift:
    case AArch64::SRIv4i16_shift:
    case AArch64::SRIv8i16_shift:
    case AArch64::SRIv2i32_shift:
    case AArch64::SRIv4i32_shift:
    case AArch64::SRId:
    case AArch64::SRIv2i64_shift:
      shifted_a = createMaskedLShr(a, shiftVec);
      maskAmt = ((1ULL << shiftAmt) - 1) << (eltSize - shiftAmt);
      break;
    default:
      assert(false && "Invalid opcode for SLI");
      break;
    }

    auto res = readFromVecOperand(1, eltSize, numElts);
    auto mask = getElemSplat(numElts, eltSize, maskAmt);
    auto masked_res = createAnd(res, mask);

    updateOutputReg(createOr(masked_res, shifted_a));
    break;
  }

  case AArch64::MLSv8i8:
  case AArch64::MLSv2i32:
  case AArch64::MLSv4i16:
  case AArch64::MLSv16i8:
  case AArch64::MLSv8i16:
  case AArch64::MLSv4i32: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES6(MLS, )
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto b = readFromVecOperand(2, eltSize, numElts);
    auto c = readFromVecOperand(3, eltSize, numElts);
    auto mul = createMul(b, c);
    auto sum = createSub(a, mul);
    updateOutputReg(sum);
    break;
  }

  case AArch64::MLAv8i8:
  case AArch64::MLAv2i32:
  case AArch64::MLAv4i16:
  case AArch64::MLAv16i8:
  case AArch64::MLAv8i16:
  case AArch64::MLAv4i32: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES6(MLA, )
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto b = readFromVecOperand(2, eltSize, numElts);
    auto c = readFromVecOperand(3, eltSize, numElts);
    auto mul = createMul(b, c);
    auto sum = createAdd(mul, a);
    updateOutputReg(sum);
    break;
  }

  case AArch64::UABALv8i8_v8i16:
  case AArch64::UABALv16i8_v8i16:
  case AArch64::UABALv4i16_v4i32:
  case AArch64::UABALv8i16_v4i32:
  case AArch64::UABALv2i32_v2i64:
  case AArch64::UABALv4i32_v2i64:
  case AArch64::UABDLv8i8_v8i16:
  case AArch64::UABDLv16i8_v8i16:
  case AArch64::UABDLv4i16_v4i32:
  case AArch64::UABDLv8i16_v4i32:
  case AArch64::UABDLv2i32_v2i64:
  case AArch64::UABDLv4i32_v2i64:
  case AArch64::SABALv8i8_v8i16:
  case AArch64::SABALv16i8_v8i16:
  case AArch64::SABALv4i16_v4i32:
  case AArch64::SABALv8i16_v4i32:
  case AArch64::SABALv2i32_v2i64:
  case AArch64::SABALv4i32_v2i64:
  case AArch64::SABDLv8i8_v8i16:
  case AArch64::SABDLv16i8_v8i16:
  case AArch64::SABDLv4i16_v4i32:
  case AArch64::SABDLv8i16_v4i32:
  case AArch64::SABDLv2i32_v2i64:
  case AArch64::SABDLv4i32_v2i64: {
    unsigned numElts, eltSize;
    bool isUpper = false;
    switch (opcode) {
    case AArch64::UABALv8i8_v8i16:
    case AArch64::UABDLv8i8_v8i16:
    case AArch64::SABALv8i8_v8i16:
    case AArch64::SABDLv8i8_v8i16:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::UABALv16i8_v8i16:
    case AArch64::UABDLv16i8_v8i16:
    case AArch64::SABALv16i8_v8i16:
    case AArch64::SABDLv16i8_v8i16:
      numElts = 16;
      eltSize = 8;
      isUpper = true;
      break;
    case AArch64::UABALv4i16_v4i32:
    case AArch64::UABDLv4i16_v4i32:
    case AArch64::SABALv4i16_v4i32:
    case AArch64::SABDLv4i16_v4i32:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::UABALv8i16_v4i32:
    case AArch64::UABDLv8i16_v4i32:
    case AArch64::SABALv8i16_v4i32:
    case AArch64::SABDLv8i16_v4i32:
      numElts = 8;
      eltSize = 16;
      isUpper = true;
      break;
    case AArch64::UABALv2i32_v2i64:
    case AArch64::UABDLv2i32_v2i64:
    case AArch64::SABALv2i32_v2i64:
    case AArch64::SABDLv2i32_v2i64:
      numElts = 2;
      eltSize = 32;
      break;
    case AArch64::UABALv4i32_v2i64:
    case AArch64::UABDLv4i32_v2i64:
    case AArch64::SABALv4i32_v2i64:
    case AArch64::SABDLv4i32_v2i64:
      numElts = 4;
      eltSize = 32;
      isUpper = true;
      break;
    default:
      assert(false);
    }

    bool isSigned = false;
    switch (opcode) {
    case AArch64::SABALv8i8_v8i16:
    case AArch64::SABALv16i8_v8i16:
    case AArch64::SABALv4i16_v4i32:
    case AArch64::SABALv8i16_v4i32:
    case AArch64::SABALv2i32_v2i64:
    case AArch64::SABALv4i32_v2i64:
    case AArch64::SABDLv8i8_v8i16:
    case AArch64::SABDLv16i8_v8i16:
    case AArch64::SABDLv4i16_v4i32:
    case AArch64::SABDLv8i16_v4i32:
    case AArch64::SABDLv2i32_v2i64:
    case AArch64::SABDLv4i32_v2i64:
      isSigned = true;
      break;
    }

    bool accumulate = false;
    switch (opcode) {
    case AArch64::UABALv8i8_v8i16:
    case AArch64::UABALv16i8_v8i16:
    case AArch64::UABALv4i16_v4i32:
    case AArch64::UABALv8i16_v4i32:
    case AArch64::UABALv2i32_v2i64:
    case AArch64::UABALv4i32_v2i64:
    case AArch64::SABALv8i8_v8i16:
    case AArch64::SABALv16i8_v8i16:
    case AArch64::SABALv4i16_v4i32:
    case AArch64::SABALv8i16_v4i32:
    case AArch64::SABALv2i32_v2i64:
    case AArch64::SABALv4i32_v2i64:
      accumulate = true;
      break;
    }

    auto a = readFromVecOperand(accumulate ? 2 : 1, eltSize, numElts, isUpper);
    auto b = readFromVecOperand(accumulate ? 3 : 2, eltSize, numElts, isUpper);

    Value *extended_a, *extended_b;

    if (isSigned) {
      extended_a =
          createSExt(a, getVecTy(2 * eltSize, isUpper ? numElts / 2 : numElts));
      extended_b =
          createSExt(b, getVecTy(2 * eltSize, isUpper ? numElts / 2 : numElts));
    } else {
      extended_a =
          createZExt(a, getVecTy(2 * eltSize, isUpper ? numElts / 2 : numElts));
      extended_b =
          createZExt(b, getVecTy(2 * eltSize, isUpper ? numElts / 2 : numElts));
    }

    auto difference = createSub(extended_a, extended_b);
    Value *res = createAbs(difference);

    if (accumulate) {
      auto c =
          readFromVecOperand(1, 2 * eltSize, isUpper ? numElts / 2 : numElts);
      res = createAdd(c, res);
    }

    updateOutputReg(res);
    break;
  }

  case AArch64::UMLALv4i16_indexed:
  case AArch64::UMLALv8i16_indexed:
  case AArch64::UMLALv2i32_indexed:
  case AArch64::UMLALv4i32_indexed:
  case AArch64::UMLSLv4i16_indexed:
  case AArch64::UMLSLv8i16_indexed:
  case AArch64::UMLSLv2i32_indexed:
  case AArch64::UMLSLv4i32_indexed:
  case AArch64::SMLALv4i16_indexed:
  case AArch64::SMLALv8i16_indexed:
  case AArch64::SMLALv2i32_indexed:
  case AArch64::SMLALv4i32_indexed:
  case AArch64::SMLSLv4i16_indexed:
  case AArch64::SMLSLv8i16_indexed:
  case AArch64::SMLSLv2i32_indexed:
  case AArch64::SMLSLv4i32_indexed: {
    unsigned numElts, eltSize;
    switch (opcode) {
    case AArch64::UMLALv2i32_indexed:
    case AArch64::UMLSLv2i32_indexed:
    case AArch64::SMLALv2i32_indexed:
    case AArch64::SMLSLv2i32_indexed:
      numElts = 2;
      eltSize = 32;
      break;
    case AArch64::UMLALv4i16_indexed:
    case AArch64::UMLSLv4i16_indexed:
    case AArch64::SMLALv4i16_indexed:
    case AArch64::SMLSLv4i16_indexed:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::UMLALv4i32_indexed:
    case AArch64::UMLSLv4i32_indexed:
    case AArch64::SMLALv4i32_indexed:
    case AArch64::SMLSLv4i32_indexed:
      numElts = 4;
      eltSize = 32;
      break;
    case AArch64::UMLALv8i16_indexed:
    case AArch64::UMLSLv8i16_indexed:
    case AArch64::SMLALv8i16_indexed:
    case AArch64::SMLSLv8i16_indexed:
      numElts = 8;
      eltSize = 16;
      break;
    default:
      assert(false);
    }
    auto isUpper = opcode == AArch64::UMLALv8i16_indexed ||
                   opcode == AArch64::UMLSLv8i16_indexed ||
                   opcode == AArch64::SMLALv8i16_indexed ||
                   opcode == AArch64::SMLSLv8i16_indexed ||
                   opcode == AArch64::UMLALv4i32_indexed ||
                   opcode == AArch64::UMLSLv4i32_indexed ||
                   opcode == AArch64::SMLALv4i32_indexed ||
                   opcode == AArch64::SMLSLv4i32_indexed;
    auto isSigned = opcode == AArch64::SMLALv4i16_indexed ||
                    opcode == AArch64::SMLSLv4i16_indexed ||
                    opcode == AArch64::SMLALv8i16_indexed ||
                    opcode == AArch64::SMLSLv8i16_indexed ||
                    opcode == AArch64::SMLALv2i32_indexed ||
                    opcode == AArch64::SMLSLv2i32_indexed ||
                    opcode == AArch64::SMLALv4i32_indexed ||
                    opcode == AArch64::SMLSLv4i32_indexed;
    auto isSub = opcode == AArch64::UMLSLv4i16_indexed ||
                 opcode == AArch64::UMLSLv8i16_indexed ||
                 opcode == AArch64::UMLSLv2i32_indexed ||
                 opcode == AArch64::UMLSLv4i32_indexed ||
                 opcode == AArch64::SMLSLv4i16_indexed ||
                 opcode == AArch64::SMLSLv8i16_indexed ||
                 opcode == AArch64::SMLSLv2i32_indexed ||
                 opcode == AArch64::SMLSLv4i32_indexed;
    assert(isSIMDandFPRegOperand(CurInst->getOperand(0)) &&
           isSIMDandFPRegOperand(CurInst->getOperand(1)) &&
           CurInst->getOperand(0).getReg() == CurInst->getOperand(1).getReg());
    assert(CurInst->getOperand(4).isImm());

    auto numEltsAdj = isUpper ? numElts / 2 : numElts;

    auto destReg = readFromVecOperand(1, 2 * eltSize, numEltsAdj);
    auto a = readFromVecOperand(2, eltSize, numElts, isUpper);
    auto element =
        getIndexedElement(getImm(4), eltSize, CurInst->getOperand(3).getReg());
    auto splatElement = splat(element, numEltsAdj, eltSize);
    auto *eTy = getVecTy(2 * eltSize, numEltsAdj);
    auto extended_a = isSigned ? createSExt(a, eTy) : createZExt(a, eTy);
    auto extended_b = isSigned ? createSExt(splatElement, eTy)
                               : createZExt(splatElement, eTy);
    auto mul = createMul(extended_a, extended_b);
    auto sum = isSub ? createSub(destReg, mul) : createAdd(destReg, mul);

    updateOutputReg(sum);
    break;
  }

  case AArch64::UMLALv8i8_v8i16:
  case AArch64::UMLALv16i8_v8i16:
  case AArch64::UMLALv4i16_v4i32:
  case AArch64::UMLALv8i16_v4i32:
  case AArch64::UMLALv2i32_v2i64:
  case AArch64::UMLALv4i32_v2i64:
  case AArch64::UMLSLv8i8_v8i16:
  case AArch64::UMLSLv16i8_v8i16:
  case AArch64::UMLSLv4i16_v4i32:
  case AArch64::UMLSLv8i16_v4i32:
  case AArch64::UMLSLv2i32_v2i64:
  case AArch64::UMLSLv4i32_v2i64:
  case AArch64::SMLALv8i8_v8i16:
  case AArch64::SMLALv16i8_v8i16:
  case AArch64::SMLALv4i16_v4i32:
  case AArch64::SMLALv8i16_v4i32:
  case AArch64::SMLALv2i32_v2i64:
  case AArch64::SMLALv4i32_v2i64:
  case AArch64::SMLSLv8i8_v8i16:
  case AArch64::SMLSLv16i8_v8i16:
  case AArch64::SMLSLv4i16_v4i32:
  case AArch64::SMLSLv8i16_v4i32:
  case AArch64::SMLSLv2i32_v2i64:
  case AArch64::SMLSLv4i32_v2i64: {
    unsigned numElts, eltSize;
    bool isUpper = false;
    switch (opcode) {
    case AArch64::UMLALv8i8_v8i16:
    case AArch64::UMLSLv8i8_v8i16:
    case AArch64::SMLALv8i8_v8i16:
    case AArch64::SMLSLv8i8_v8i16:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::UMLALv16i8_v8i16:
    case AArch64::UMLSLv16i8_v8i16:
    case AArch64::SMLALv16i8_v8i16:
    case AArch64::SMLSLv16i8_v8i16:
      numElts = 16;
      eltSize = 8;
      isUpper = true;
      break;
    case AArch64::UMLALv4i16_v4i32:
    case AArch64::UMLSLv4i16_v4i32:
    case AArch64::SMLALv4i16_v4i32:
    case AArch64::SMLSLv4i16_v4i32:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::UMLALv8i16_v4i32:
    case AArch64::UMLSLv8i16_v4i32:
    case AArch64::SMLALv8i16_v4i32:
    case AArch64::SMLSLv8i16_v4i32:
      numElts = 8;
      eltSize = 16;
      isUpper = true;
      break;
    case AArch64::UMLALv2i32_v2i64:
    case AArch64::UMLSLv2i32_v2i64:
    case AArch64::SMLALv2i32_v2i64:
    case AArch64::SMLSLv2i32_v2i64:
      numElts = 2;
      eltSize = 32;
      break;
    case AArch64::UMLALv4i32_v2i64:
    case AArch64::UMLSLv4i32_v2i64:
    case AArch64::SMLALv4i32_v2i64:
    case AArch64::SMLSLv4i32_v2i64:
      numElts = 4;
      eltSize = 32;
      isUpper = true;
      break;
    default:
      assert(false);
    }
    auto isSigned = opcode == AArch64::SMLALv8i8_v8i16 ||
                    opcode == AArch64::SMLSLv8i8_v8i16 ||
                    opcode == AArch64::SMLALv16i8_v8i16 ||
                    opcode == AArch64::SMLSLv16i8_v8i16 ||
                    opcode == AArch64::SMLALv4i16_v4i32 ||
                    opcode == AArch64::SMLSLv4i16_v4i32 ||
                    opcode == AArch64::SMLALv8i16_v4i32 ||
                    opcode == AArch64::SMLSLv8i16_v4i32 ||
                    opcode == AArch64::SMLALv2i32_v2i64 ||
                    opcode == AArch64::SMLSLv2i32_v2i64 ||
                    opcode == AArch64::SMLALv4i32_v2i64 ||
                    opcode == AArch64::SMLSLv4i32_v2i64;
    auto isSub = opcode == AArch64::UMLSLv8i8_v8i16 ||
                 opcode == AArch64::UMLSLv16i8_v8i16 ||
                 opcode == AArch64::UMLSLv4i16_v4i32 ||
                 opcode == AArch64::UMLSLv8i16_v4i32 ||
                 opcode == AArch64::UMLSLv2i32_v2i64 ||
                 opcode == AArch64::UMLSLv4i32_v2i64 ||
                 opcode == AArch64::SMLSLv8i8_v8i16 ||
                 opcode == AArch64::SMLSLv16i8_v8i16 ||
                 opcode == AArch64::SMLSLv4i16_v4i32 ||
                 opcode == AArch64::SMLSLv8i16_v4i32 ||
                 opcode == AArch64::SMLSLv2i32_v2i64 ||
                 opcode == AArch64::SMLSLv4i32_v2i64;
    assert(isSIMDandFPRegOperand(CurInst->getOperand(0)) &&
           isSIMDandFPRegOperand(CurInst->getOperand(1)) &&
           CurInst->getOperand(0).getReg() == CurInst->getOperand(1).getReg());
    auto destReg =
        readFromVecOperand(1, 2 * eltSize, isUpper ? numElts / 2 : numElts);
    auto a = readFromVecOperand(2, eltSize, numElts, isUpper);
    auto b = readFromVecOperand(3, eltSize, numElts, isUpper);

    auto extended_a =
        isSigned ? createSExt(a, getVecTy(2 * eltSize,
                                          isUpper ? numElts / 2 : numElts))
                 : createZExt(a, getVecTy(2 * eltSize,
                                          isUpper ? numElts / 2 : numElts));
    auto extended_b =
        isSigned ? createSExt(b, getVecTy(2 * eltSize,
                                          isUpper ? numElts / 2 : numElts))
                 : createZExt(b, getVecTy(2 * eltSize,
                                          isUpper ? numElts / 2 : numElts));

    auto mul = createMul(extended_a, extended_b);
    auto res = isSub ? createSub(destReg, mul) : createAdd(destReg, mul);

    updateOutputReg(res);
    break;
  }

  case AArch64::UABAv8i8:
  case AArch64::UABAv16i8:
  case AArch64::UABAv4i16:
  case AArch64::UABAv8i16:
  case AArch64::UABAv2i32:
  case AArch64::UABAv4i32:
  case AArch64::UABDv8i8:
  case AArch64::UABDv16i8:
  case AArch64::UABDv4i16:
  case AArch64::UABDv8i16:
  case AArch64::UABDv2i32:
  case AArch64::UABDv4i32:
  case AArch64::SABAv8i8:
  case AArch64::SABAv16i8:
  case AArch64::SABAv4i16:
  case AArch64::SABAv8i16:
  case AArch64::SABAv2i32:
  case AArch64::SABAv4i32:
  case AArch64::SABDv8i8:
  case AArch64::SABDv16i8:
  case AArch64::SABDv4i16:
  case AArch64::SABDv8i16:
  case AArch64::SABDv2i32:
  case AArch64::SABDv4i32: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES6(UABA, )
    GET_SIZES6(UABD, )
    GET_SIZES6(SABA, )
    GET_SIZES6(SABD, )

    bool isSigned = false;
    switch (opcode) {
    case AArch64::SABAv8i8:
    case AArch64::SABAv16i8:
    case AArch64::SABAv4i16:
    case AArch64::SABAv8i16:
    case AArch64::SABAv2i32:
    case AArch64::SABAv4i32:
    case AArch64::SABDv8i8:
    case AArch64::SABDv16i8:
    case AArch64::SABDv4i16:
    case AArch64::SABDv8i16:
    case AArch64::SABDv2i32:
    case AArch64::SABDv4i32: {
      isSigned = true;
      break;
    }
    }

    bool accumulate = false;
    switch (opcode) {
    case AArch64::UABAv8i8:
    case AArch64::UABAv16i8:
    case AArch64::UABAv4i16:
    case AArch64::UABAv8i16:
    case AArch64::UABAv2i32:
    case AArch64::UABAv4i32:
    case AArch64::SABAv8i8:
    case AArch64::SABAv16i8:
    case AArch64::SABAv4i16:
    case AArch64::SABAv8i16:
    case AArch64::SABAv2i32:
    case AArch64::SABAv4i32:
      accumulate = true;
      break;
    }

    auto a = readFromVecOperand(accumulate ? 2 : 1, eltSize, numElts);
    auto b = readFromVecOperand(accumulate ? 3 : 2, eltSize, numElts);

    Value *extended_a;
    Value *extended_b;

    if (isSigned) {
      extended_a = createSExt(a, getVecTy(2 * eltSize, numElts));
      extended_b = createSExt(b, getVecTy(2 * eltSize, numElts));
    } else {
      extended_a = createZExt(a, getVecTy(2 * eltSize, numElts));
      extended_b = createZExt(b, getVecTy(2 * eltSize, numElts));
    }

    auto difference = createSub(extended_a, extended_b);
    auto abs = createAbs(difference);
    Value *res = createTrunc(abs, getVecTy(eltSize, numElts));
    if (accumulate) {
      auto c = readFromVecOperand(1, eltSize, numElts);
      res = createAdd(c, res);
    }

    updateOutputReg(res);
    break;
  }

#define GET_SIZES7(INSN, SUFF)                                                 \
  if (opcode == AArch64::INSN##v8i8##SUFF) {                                   \
    numElts = 8;                                                               \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i16##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v2i32##SUFF) {                           \
    numElts = 2;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v16i8##SUFF) {                           \
    numElts = 16;                                                              \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i32##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v8i16##SUFF) {                           \
    numElts = 8;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v2i64##SUFF) {                           \
    numElts = 2;                                                               \
    eltSize = 64;                                                              \
  }

  case AArch64::SSRAv8i8_shift:
  case AArch64::SSRAv16i8_shift:
  case AArch64::SSRAv4i16_shift:
  case AArch64::SSRAv8i16_shift:
  case AArch64::SSRAv2i32_shift:
  case AArch64::SSRAv4i32_shift:
  case AArch64::SSRAd:
  case AArch64::SSRAv2i64_shift: {
    unsigned numElts = -1, eltSize = -1;
    switch (opcode) {
    case AArch64::SSRAv8i8_shift:
      eltSize = 8;
      numElts = 8;
      break;
    case AArch64::SSRAv16i8_shift:
      eltSize = 8;
      numElts = 16;
      break;
    case AArch64::SSRAv4i16_shift:
      eltSize = 16;
      numElts = 4;
      break;
    case AArch64::SSRAv8i16_shift:
      eltSize = 16;
      numElts = 8;
      break;
    case AArch64::SSRAv2i32_shift:
      eltSize = 32;
      numElts = 2;
      break;
    case AArch64::SSRAv4i32_shift:
      eltSize = 32;
      numElts = 4;
      break;
    case AArch64::SSRAd:
      eltSize = 64;
      numElts = 1;
      break;
    case AArch64::SSRAv2i64_shift:
      eltSize = 64;
      numElts = 2;
      break;
    default:
      assert(false);
    }

    Value *a, *b, *c;
    a = readFromVecOperand(2, eltSize, numElts);
    b = getElemSplat(numElts, eltSize, getImm(3));
    c = readFromVecOperand(1, eltSize, numElts);

    auto shiftedVec = createMaskedAShr(a, b);
    auto res = createAdd(shiftedVec, c);

    updateOutputReg(res);
    break;
  }

  case AArch64::USRAv8i8_shift:
  case AArch64::USRAv16i8_shift:
  case AArch64::USRAv4i16_shift:
  case AArch64::USRAv8i16_shift:
  case AArch64::USRAv2i32_shift:
  case AArch64::USRAv4i32_shift:
  case AArch64::USRAv2i64_shift:
  case AArch64::USRAd: {
    unsigned numElts, eltSize;
    GET_SIZES7(USRA, _shift);
    if (opcode == AArch64::USRAd) {
      numElts = 1;
      eltSize = 64;
    }
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto b = readFromVecOperand(2, eltSize, numElts);
    auto exp = getImm(3);
    Value *res = getUndefVec(numElts, eltSize);
    for (unsigned i = 0; i < numElts; ++i) {
      auto e1 = createExtractElement(a, i);
      auto e2 = createExtractElement(b, i);
      auto shift = createMaskedLShr(e2, getUnsignedIntConst(exp, eltSize));
      auto sum = createAdd(e1, shift);
      res = createInsertElement(res, sum, i);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::ZIP1v4i16:
  case AArch64::ZIP1v2i32:
  case AArch64::ZIP1v8i8:
  case AArch64::ZIP1v8i16:
  case AArch64::ZIP1v16i8:
  case AArch64::ZIP1v2i64:
  case AArch64::ZIP1v4i32: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES7(ZIP1, );
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto b = readFromVecOperand(2, eltSize, numElts);
    Value *res = getUndefVec(numElts, eltSize);
    for (unsigned i = 0; i < numElts / 2; ++i) {
      auto e1 = createExtractElement(a, i);
      auto e2 = createExtractElement(b, i);
      res = createInsertElement(res, e1, 2 * i);
      res = createInsertElement(res, e2, (2 * i) + 1);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::ZIP2v4i16:
  case AArch64::ZIP2v2i32:
  case AArch64::ZIP2v8i8:
  case AArch64::ZIP2v8i16:
  case AArch64::ZIP2v2i64:
  case AArch64::ZIP2v16i8:
  case AArch64::ZIP2v4i32: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES7(ZIP2, );
    auto a = readFromVecOperand(1, eltSize, numElts);
    auto b = readFromVecOperand(2, eltSize, numElts);
    Value *res = getUndefVec(numElts, eltSize);
    for (unsigned i = 0; i < numElts / 2; ++i) {
      auto e1 = createExtractElement(a, (numElts / 2) + i);
      auto e2 = createExtractElement(b, (numElts / 2) + i);
      res = createInsertElement(res, e1, 2 * i);
      res = createInsertElement(res, e2, (2 * i) + 1);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::ADDPv8i8:
  case AArch64::ADDPv4i16:
  case AArch64::ADDPv2i32:
  case AArch64::ADDPv16i8:
  case AArch64::ADDPv4i32:
  case AArch64::ADDPv8i16:
  case AArch64::ADDPv2i64: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES7(ADDP, );
    auto x = readFromOperand(1);
    auto y = readFromOperand(2);
    auto conc = concat(y, x);
    auto concTy = getVecTy(eltSize, numElts * 2);
    auto concV = createBitCast(conc, concTy);
    Value *res = getUndefVec(numElts, eltSize);
    for (unsigned e = 0; e < numElts; ++e) {
      *out << "e = " << e << "\n";
      auto elt1 = createExtractElement(concV, 2 * e);
      auto elt2 = createExtractElement(concV, (2 * e) + 1);
      auto sum = createAdd(elt1, elt2);
      res = createInsertElement(res, sum, e);
    }
    updateOutputReg(res);
    break;
  }

#define GET_SIZES9(INSN, SUFF)                                                 \
  unsigned numElts, eltSize;                                                   \
  if (opcode == AArch64::INSN##v8i8##SUFF) {                                   \
    numElts = 8;                                                               \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i16##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v1i32##SUFF) {                           \
    numElts = 1;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v2i32##SUFF) {                           \
    numElts = 2;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v16i8##SUFF) {                           \
    numElts = 16;                                                              \
    eltSize = 8;                                                               \
  } else if (opcode == AArch64::INSN##v4i32##SUFF) {                           \
    numElts = 4;                                                               \
    eltSize = 32;                                                              \
  } else if (opcode == AArch64::INSN##v8i16##SUFF) {                           \
    numElts = 8;                                                               \
    eltSize = 16;                                                              \
  } else if (opcode == AArch64::INSN##v1i64##SUFF) {                           \
    numElts = 1;                                                               \
    eltSize = 64;                                                              \
  } else if (opcode == AArch64::INSN##v2i64##SUFF) {                           \
    numElts = 2;                                                               \
    eltSize = 64;                                                              \
  } else {                                                                     \
    assert(false);                                                             \
  }

    // FIXME: we're not doing this "If saturation occurs, the
    // cumulative saturation bit FPSR.QC is set." (same applies to
    // UQSUB)
  case AArch64::UQADDv1i32:
  case AArch64::UQADDv1i64:
  case AArch64::UQADDv8i8:
  case AArch64::UQADDv4i16:
  case AArch64::UQADDv2i32:
  case AArch64::UQADDv2i64:
  case AArch64::UQADDv4i32:
  case AArch64::UQADDv16i8:
  case AArch64::UQADDv8i16: {
    GET_SIZES9(UQADD, );
    auto x = readFromVecOperand(1, eltSize, numElts);
    auto y = readFromVecOperand(2, eltSize, numElts);
    auto res = createUAddSat(x, y);
    updateOutputReg(res);
    break;
  }

  case AArch64::UQSUBv1i32:
  case AArch64::UQSUBv1i64:
  case AArch64::UQSUBv8i8:
  case AArch64::UQSUBv4i16:
  case AArch64::UQSUBv2i32:
  case AArch64::UQSUBv2i64:
  case AArch64::UQSUBv4i32:
  case AArch64::UQSUBv16i8:
  case AArch64::UQSUBv8i16: {
    GET_SIZES9(UQSUB, );
    auto x = readFromVecOperand(1, eltSize, numElts);
    auto y = readFromVecOperand(2, eltSize, numElts);
    auto res = createUSubSat(x, y);
    updateOutputReg(res);
    break;
  }

  case AArch64::SQADDv1i32:
  case AArch64::SQADDv1i64:
  case AArch64::SQADDv8i8:
  case AArch64::SQADDv4i16:
  case AArch64::SQADDv2i32:
  case AArch64::SQADDv2i64:
  case AArch64::SQADDv4i32:
  case AArch64::SQADDv16i8:
  case AArch64::SQADDv8i16: {
    GET_SIZES9(SQADD, );
    auto x = readFromVecOperand(1, eltSize, numElts);
    auto y = readFromVecOperand(2, eltSize, numElts);
    auto res = createSAddSat(x, y);
    updateOutputReg(res);
    break;
  }

  case AArch64::SQSUBv1i32:
  case AArch64::SQSUBv1i64:
  case AArch64::SQSUBv8i8:
  case AArch64::SQSUBv4i16:
  case AArch64::SQSUBv2i32:
  case AArch64::SQSUBv2i64:
  case AArch64::SQSUBv4i32:
  case AArch64::SQSUBv16i8:
  case AArch64::SQSUBv8i16: {
    GET_SIZES9(SQSUB, );
    auto x = readFromVecOperand(1, eltSize, numElts);
    auto y = readFromVecOperand(2, eltSize, numElts);
    auto res = createSSubSat(x, y);
    updateOutputReg(res);
    break;
  }

  case AArch64::UMULLv4i16_indexed:
  case AArch64::UMULLv2i32_indexed:
  case AArch64::UMULLv8i16_indexed:
  case AArch64::UMULLv4i32_indexed:
  case AArch64::SMULLv4i32_indexed:
  case AArch64::SMULLv8i16_indexed:
  case AArch64::SMULLv4i16_indexed:
  case AArch64::SMULLv2i32_indexed: {
    int eltSize, numElts1, numElts2;
    if (opcode == AArch64::SMULLv8i16_indexed ||
        opcode == AArch64::UMULLv8i16_indexed) {
      numElts1 = 8;
      numElts2 = 8;
      eltSize = 16;
    } else if (opcode == AArch64::SMULLv4i32_indexed ||
               opcode == AArch64::UMULLv4i32_indexed) {
      numElts1 = 4;
      numElts2 = 4;
      eltSize = 32;
    } else if (opcode == AArch64::SMULLv2i32_indexed ||
               opcode == AArch64::UMULLv2i32_indexed) {
      numElts1 = 2;
      numElts2 = 4;
      eltSize = 32;
    } else if (opcode == AArch64::SMULLv4i16_indexed ||
               opcode == AArch64::UMULLv4i16_indexed) {
      numElts1 = 4;
      numElts2 = 8;
      eltSize = 16;
    } else {
      assert(false);
    }
    auto vTy1 = getVecTy(eltSize, numElts1);
    auto vTy2 = getVecTy(eltSize, numElts2);
    auto vBigTy1 = getVecTy(2 * eltSize, numElts1);
    auto vBigTy2 = getVecTy(2 * eltSize, numElts2);
    Value *a, *b;
    switch (opcode) {
    case AArch64::UMULLv4i16_indexed:
    case AArch64::UMULLv2i32_indexed:
    case AArch64::UMULLv8i16_indexed:
    case AArch64::UMULLv4i32_indexed:
      a = createZExt(createBitCast(readFromOperand(1), vTy1), vBigTy1);
      b = createZExt(createBitCast(readFromOperand(2, 128), vTy2), vBigTy2);
      break;
    case AArch64::SMULLv4i32_indexed:
    case AArch64::SMULLv8i16_indexed:
    case AArch64::SMULLv4i16_indexed:
    case AArch64::SMULLv2i32_indexed:
      a = createSExt(createBitCast(readFromOperand(1), vTy1), vBigTy1);
      b = createSExt(createBitCast(readFromOperand(2, 128), vTy2), vBigTy2);
      break;
    default:
      assert(false);
    }
    auto idx = getImm(3);
    Value *res = getUndefVec(numElts1, 2 * eltSize);
    // offset is nonzero when we're dealing with SMULL2/UMULL2
    int offset = (opcode == AArch64::SMULLv4i32_indexed ||
                  opcode == AArch64::SMULLv8i16_indexed ||
                  opcode == AArch64::UMULLv4i32_indexed ||
                  opcode == AArch64::UMULLv8i16_indexed)
                     ? (numElts1 / 2)
                     : 0;
    auto e2 = createExtractElement(b, idx);
    for (int i = 0; i < numElts1; ++i) {
      auto e1 = createExtractElement(a, i + offset);
      res = createInsertElement(res, createMul(e1, e2), i);
    }
    updateOutputReg(res);
    break;
  }

  case AArch64::URHADDv8i8:
  case AArch64::URHADDv16i8:
  case AArch64::URHADDv4i16:
  case AArch64::URHADDv8i16:
  case AArch64::URHADDv2i32:
  case AArch64::URHADDv4i32:
  case AArch64::UHADDv8i8:
  case AArch64::UHADDv16i8:
  case AArch64::UHADDv4i16:
  case AArch64::UHADDv8i16:
  case AArch64::UHADDv2i32:
  case AArch64::UHADDv4i32:
  case AArch64::SRHADDv8i8:
  case AArch64::SRHADDv16i8:
  case AArch64::SRHADDv4i16:
  case AArch64::SRHADDv8i16:
  case AArch64::SRHADDv2i32:
  case AArch64::SRHADDv4i32:
  case AArch64::SHADDv8i8:
  case AArch64::SHADDv16i8:
  case AArch64::SHADDv4i16:
  case AArch64::SHADDv8i16:
  case AArch64::SHADDv2i32:
  case AArch64::SHADDv4i32:
  case AArch64::UHSUBv8i8:
  case AArch64::UHSUBv16i8:
  case AArch64::UHSUBv4i16:
  case AArch64::UHSUBv8i16:
  case AArch64::UHSUBv2i32:
  case AArch64::UHSUBv4i32:
  case AArch64::SHSUBv8i8:
  case AArch64::SHSUBv16i8:
  case AArch64::SHSUBv4i16:
  case AArch64::SHSUBv8i16:
  case AArch64::SHSUBv2i32:
  case AArch64::SHSUBv4i32: {
    unsigned numElts = -1, eltSize = -1;
    GET_SIZES6(URHADD, )
    GET_SIZES6(UHADD, )
    GET_SIZES6(SRHADD, )
    GET_SIZES6(SHADD, )
    GET_SIZES6(UHSUB, )
    GET_SIZES6(SHSUB, )

    bool isSigned = false;
    switch (opcode) {
    case AArch64::SRHADDv8i8:
    case AArch64::SRHADDv16i8:
    case AArch64::SRHADDv4i16:
    case AArch64::SRHADDv8i16:
    case AArch64::SRHADDv2i32:
    case AArch64::SRHADDv4i32:
    case AArch64::SHADDv8i8:
    case AArch64::SHADDv16i8:
    case AArch64::SHADDv4i16:
    case AArch64::SHADDv8i16:
    case AArch64::SHADDv2i32:
    case AArch64::SHADDv4i32:
    case AArch64::SHSUBv8i8:
    case AArch64::SHSUBv16i8:
    case AArch64::SHSUBv4i16:
    case AArch64::SHSUBv8i16:
    case AArch64::SHSUBv2i32:
    case AArch64::SHSUBv4i32:
      isSigned = true;
      break;
    }

    Instruction::BinaryOps op;
    bool addRoundingConst = false;
    switch (opcode) {
    case AArch64::URHADDv8i8:
    case AArch64::URHADDv16i8:
    case AArch64::URHADDv4i16:
    case AArch64::URHADDv8i16:
    case AArch64::URHADDv2i32:
    case AArch64::URHADDv4i32:
    case AArch64::SRHADDv8i8:
    case AArch64::SRHADDv16i8:
    case AArch64::SRHADDv4i16:
    case AArch64::SRHADDv8i16:
    case AArch64::SRHADDv2i32:
    case AArch64::SRHADDv4i32:
      addRoundingConst = true;
    case AArch64::UHADDv8i8:
    case AArch64::UHADDv16i8:
    case AArch64::UHADDv4i16:
    case AArch64::UHADDv8i16:
    case AArch64::UHADDv2i32:
    case AArch64::UHADDv4i32:
    case AArch64::SHADDv8i8:
    case AArch64::SHADDv16i8:
    case AArch64::SHADDv4i16:
    case AArch64::SHADDv8i16:
    case AArch64::SHADDv2i32:
    case AArch64::SHADDv4i32:
      op = Instruction::Add;
      break;
    case AArch64::UHSUBv8i8:
    case AArch64::UHSUBv16i8:
    case AArch64::UHSUBv4i16:
    case AArch64::UHSUBv8i16:
    case AArch64::UHSUBv2i32:
    case AArch64::UHSUBv4i32:
    case AArch64::SHSUBv8i8:
    case AArch64::SHSUBv16i8:
    case AArch64::SHSUBv4i16:
    case AArch64::SHSUBv8i16:
    case AArch64::SHSUBv2i32:
    case AArch64::SHSUBv4i32:
      op = Instruction::Sub;
      break;
    default:
      assert(false);
    }

    Type *vecTy = getVecTy(eltSize, numElts);
    Type *vecTyDoubledEltSize = getVecTy(2 * eltSize, numElts);

    auto a = isSigned ? createSExt(readFromVecOperand(1, eltSize, numElts),
                                   vecTyDoubledEltSize)
                      : createZExt(readFromVecOperand(1, eltSize, numElts),
                                   vecTyDoubledEltSize);
    auto b = isSigned ? createSExt(readFromVecOperand(2, eltSize, numElts),
                                   vecTyDoubledEltSize)
                      : createZExt(readFromVecOperand(2, eltSize, numElts),
                                   vecTyDoubledEltSize);

    auto res = createBinop(a, b, op);
    if (addRoundingConst) {
      auto roundingConst = getElemSplat(numElts, 2 * eltSize, 1);
      res = createAdd(res, roundingConst);
    }

    auto onesVec = getElemSplat(numElts, 2 * eltSize, 1);
    auto shifted = createRawAShr(res, onesVec);

    updateOutputReg(createTrunc(shifted, vecTy));
    break;
  }

  case AArch64::XTNv2i32:
  case AArch64::XTNv4i32:
  case AArch64::XTNv4i16:
  case AArch64::XTNv8i16:
  case AArch64::XTNv8i8:
  case AArch64::XTNv16i8: {
    auto &op0 = CurInst->getOperand(0);
    uint64_t srcReg = opcode == AArch64::XTNv2i32 ||
                              opcode == AArch64::XTNv4i16 ||
                              opcode == AArch64::XTNv8i8
                          ? 1
                          : 2;
    auto &op1 = CurInst->getOperand(srcReg);
    assert(isSIMDandFPRegOperand(op0) && isSIMDandFPRegOperand(op0));

    Value *src = readFromRegOld(op1.getReg());
    assert(getBitWidth(src) == 128 &&
           "Source value is not a vector with 128 bits");

    uint64_t eltSize, numElts, part;
    part = opcode == AArch64::XTNv2i32 || opcode == AArch64::XTNv4i16 ||
                   opcode == AArch64::XTNv8i8
               ? 0
               : 1;
    switch (opcode) {
    case AArch64::XTNv8i8:
    case AArch64::XTNv16i8:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::XTNv4i16:
    case AArch64::XTNv8i16:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::XTNv2i32:
    case AArch64::XTNv4i32:
      numElts = 2;
      eltSize = 32;
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }

    // BitCast src to a vector of numElts x (2*eltSize) for narrowing
    assert(numElts * (2 * eltSize) == 128 && "BitCasting to wrong type");
    Value *src_vector = createBitCast(src, getVecTy(2 * eltSize, numElts));
    Value *narrowed_vector =
        createTrunc(src_vector, getVecTy(eltSize, numElts));

    Value *final_vector = narrowed_vector;
    // For XTN2 - insertion to upper half
    if (part) {
      // Preserve the lower 64 bits so, read from destination register
      // and insert to the upper 64 bits
      Value *dest = readFromRegOld(op0.getReg());
      Value *original_dest_vector = createBitCast(dest, getVecTy(64, 2));

      Value *element = createBitCast(narrowed_vector, i64);

      final_vector = createInsertElement(original_dest_vector, element, 1);
    }

    // Write 64 bits for XTN or 128 bits XTN2 to output register
    updateOutputReg(final_vector);
    break;
  }

    //    case AArch64::UQXTNv1i8:
    //    case AArch64::UQXTNv1i16:
    //    case AArch64::UQXTNv1i32:
  case AArch64::UQXTNv8i8:
  case AArch64::UQXTNv4i16:
  case AArch64::UQXTNv2i32:
  case AArch64::UQXTNv16i8:
  case AArch64::UQXTNv8i16:
  case AArch64::UQXTNv4i32:
    //    case AArch64::SQXTNv1i8:
    //    case AArch64::SQXTNv1i16:
    //    case AArch64::SQXTNv1i32:
  case AArch64::SQXTNv8i8:
  case AArch64::SQXTNv4i16:
  case AArch64::SQXTNv2i32:
  case AArch64::SQXTNv16i8:
  case AArch64::SQXTNv8i16:
  case AArch64::SQXTNv4i32: {
    auto &op0 = CurInst->getOperand(0);

    uint64_t srcReg, part;
    switch (opcode) {
    case AArch64::UQXTNv1i8:
    case AArch64::UQXTNv1i16:
    case AArch64::UQXTNv1i32:
    case AArch64::UQXTNv8i8:
    case AArch64::UQXTNv4i16:
    case AArch64::UQXTNv2i32:
    case AArch64::SQXTNv1i8:
    case AArch64::SQXTNv1i16:
    case AArch64::SQXTNv1i32:
    case AArch64::SQXTNv8i8:
    case AArch64::SQXTNv4i16:
    case AArch64::SQXTNv2i32:
      srcReg = 1;
      part = 0;
      break;
    case AArch64::UQXTNv16i8:
    case AArch64::UQXTNv8i16:
    case AArch64::UQXTNv4i32:
    case AArch64::SQXTNv16i8:
    case AArch64::SQXTNv8i16:
    case AArch64::SQXTNv4i32:
      srcReg = 2;
      part = 1;
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }

    auto &op1 = CurInst->getOperand(srcReg);
    assert(isSIMDandFPRegOperand(op0) && isSIMDandFPRegOperand(op0));

    Value *src = readFromRegOld(op1.getReg());
    assert(getBitWidth(src) == 128 &&
           "Source value is not a vector with 128 bits");

    uint64_t eltSize, numElts;
    switch (opcode) {
    case AArch64::UQXTNv1i8:
    case AArch64::UQXTNv16i8:
    case AArch64::UQXTNv8i8:
    case AArch64::SQXTNv1i8:
    case AArch64::SQXTNv16i8:
    case AArch64::SQXTNv8i8:
      numElts = 8;
      eltSize = 8;
      break;
    case AArch64::UQXTNv1i16:
    case AArch64::UQXTNv8i16:
    case AArch64::UQXTNv4i16:
    case AArch64::SQXTNv1i16:
    case AArch64::SQXTNv8i16:
    case AArch64::SQXTNv4i16:
      numElts = 4;
      eltSize = 16;
      break;
    case AArch64::UQXTNv1i32:
    case AArch64::UQXTNv4i32:
    case AArch64::UQXTNv2i32:
    case AArch64::SQXTNv1i32:
    case AArch64::SQXTNv2i32:
    case AArch64::SQXTNv4i32:
      numElts = 2;
      eltSize = 32;
      break;
    default:
      *out << "\nError Unknown opcode\n";
      visitError();
      break;
    }

    if (opcode == AArch64::UQXTNv1i8 || opcode == AArch64::UQXTNv1i16 ||
        opcode == AArch64::UQXTNv1i32 || opcode == AArch64::SQXTNv1i8 ||
        opcode == AArch64::SQXTNv1i16 || opcode == AArch64::SQXTNv1i32) {
      numElts = 1;
    }

    bool isSigned = false;
    switch (opcode) {
    case AArch64::SQXTNv1i8:
    case AArch64::SQXTNv1i16:
    case AArch64::SQXTNv1i32:
    case AArch64::SQXTNv8i8:
    case AArch64::SQXTNv4i16:
    case AArch64::SQXTNv2i32:
    case AArch64::SQXTNv16i8:
    case AArch64::SQXTNv8i16:
    case AArch64::SQXTNv4i32:
      isSigned = true;
      break;
    }

    // BitCast src to a vector of numElts x (2*eltSize) for narrowing
    assert(numElts * (2 * eltSize) == 128 && "BitCasting to wrong type");
    Value *src_vector = createBitCast(src, getVecTy(2 * eltSize, numElts));

    Value *final_vector = getUndefVec(numElts, eltSize);
    // Perform element-wise saturating narrow using SatQ
    for (unsigned i = 0; i < numElts; ++i) {
      auto element = createExtractElement(src_vector, i);
      auto [narrowed_element, sat] = SatQ(element, eltSize, isSigned);
      final_vector = createInsertElement(final_vector, narrowed_element, i);
    }

    // For UQXTN2, SQXTN2 - insertion to upper half
    if (part) {
      // Preserve the lower 64 bits so, read from destination register
      // and insert to the upper 64 bits
      Value *dest = readFromRegOld(op0.getReg());
      Value *original_dest_vector = createBitCast(dest, getVecTy(64, 2));

      Value *element = createBitCast(final_vector, i64);

      final_vector = createInsertElement(original_dest_vector, element, 1);
    }

    // Write 64 bits for UQXTN, SQXTN or 128 bits for UQXTN2, SQXTN2 to output
    // register
    updateOutputReg(final_vector);

    break;
  }

    // unary vector instructions
  case AArch64::ABSv1i64:
  case AArch64::ABSv8i8:
  case AArch64::ABSv4i16:
  case AArch64::ABSv2i32:
  case AArch64::ABSv2i64:
  case AArch64::ABSv16i8:
  case AArch64::ABSv8i16:
  case AArch64::ABSv4i32:
  case AArch64::CLZv2i32:
  case AArch64::CLZv4i16:
  case AArch64::CLZv8i8:
  case AArch64::CLZv16i8:
  case AArch64::CLZv8i16:
  case AArch64::CLZv4i32:
  case AArch64::UADALPv8i8_v4i16:
  case AArch64::UADALPv4i16_v2i32:
  case AArch64::UADALPv2i32_v1i64:
  case AArch64::UADALPv4i32_v2i64:
  case AArch64::UADALPv16i8_v8i16:
  case AArch64::UADALPv8i16_v4i32:
  case AArch64::SADALPv8i8_v4i16:
  case AArch64::SADALPv4i16_v2i32:
  case AArch64::SADALPv2i32_v1i64:
  case AArch64::SADALPv4i32_v2i64:
  case AArch64::SADALPv16i8_v8i16:
  case AArch64::SADALPv8i16_v4i32:
  case AArch64::UADDLPv4i16_v2i32:
  case AArch64::UADDLPv2i32_v1i64:
  case AArch64::UADDLPv8i8_v4i16:
  case AArch64::UADDLPv8i16_v4i32:
  case AArch64::UADDLPv4i32_v2i64:
  case AArch64::UADDLPv16i8_v8i16:
  case AArch64::SADDLPv8i8_v4i16:
  case AArch64::SADDLPv4i16_v2i32:
  case AArch64::SADDLPv2i32_v1i64:
  case AArch64::SADDLPv16i8_v8i16:
  case AArch64::SADDLPv8i16_v4i32:
  case AArch64::SADDLPv4i32_v2i64:
  case AArch64::UADDLVv8i16v:
  case AArch64::UADDLVv4i32v:
  case AArch64::UADDLVv8i8v:
  case AArch64::UADDLVv4i16v:
  case AArch64::UADDLVv16i8v:
  case AArch64::SADDLVv8i8v:
  case AArch64::SADDLVv16i8v:
  case AArch64::SADDLVv4i16v:
  case AArch64::SADDLVv8i16v:
  case AArch64::SADDLVv4i32v:
  case AArch64::NEGv1i64:
  case AArch64::NEGv4i16:
  case AArch64::NEGv8i8:
  case AArch64::NEGv16i8:
  case AArch64::NEGv2i32:
  case AArch64::NEGv8i16:
  case AArch64::NEGv2i64:
  case AArch64::NEGv4i32:
  case AArch64::NOTv8i8:
  case AArch64::NOTv16i8:
  case AArch64::CNTv8i8:
  case AArch64::CNTv16i8:
  case AArch64::ADDVv16i8v:
  case AArch64::ADDVv8i16v:
  case AArch64::ADDVv4i32v:
  case AArch64::ADDVv8i8v:
  case AArch64::ADDVv4i16v:
  case AArch64::RBITv8i8:
  case AArch64::RBITv16i8: {
    auto src = readFromOperand(1);
    uint64_t eltSize, numElts;

    switch (opcode) {
    case AArch64::ABSv1i64:
    case AArch64::NEGv1i64:
      eltSize = 64;
      numElts = 1;
      break;
    case AArch64::ABSv4i16:
    case AArch64::CLZv4i16:
    case AArch64::NEGv4i16:
    case AArch64::UADDLVv4i16v:
    case AArch64::SADDLVv4i16v:
    case AArch64::UADDLPv4i16_v2i32:
    case AArch64::SADDLPv4i16_v2i32:
    case AArch64::UADALPv4i16_v2i32:
    case AArch64::SADALPv4i16_v2i32:
    case AArch64::ADDVv4i16v:
      eltSize = 16;
      numElts = 4;
      break;
    case AArch64::ABSv2i32:
    case AArch64::CLZv2i32:
    case AArch64::NEGv2i32:
    case AArch64::UADDLPv2i32_v1i64:
    case AArch64::SADDLPv2i32_v1i64:
    case AArch64::UADALPv2i32_v1i64:
    case AArch64::SADALPv2i32_v1i64:
      eltSize = 32;
      numElts = 2;
      break;
    case AArch64::ABSv8i16:
    case AArch64::CLZv8i16:
    case AArch64::NEGv8i16:
    case AArch64::UADDLVv8i16v:
    case AArch64::SADDLVv8i16v:
    case AArch64::UADDLPv8i16_v4i32:
    case AArch64::SADDLPv8i16_v4i32:
    case AArch64::UADALPv8i16_v4i32:
    case AArch64::SADALPv8i16_v4i32:
    case AArch64::ADDVv8i16v:
      eltSize = 16;
      numElts = 8;
      break;
    case AArch64::ABSv2i64:
    case AArch64::NEGv2i64:
      eltSize = 64;
      numElts = 2;
      break;
    case AArch64::ABSv4i32:
    case AArch64::CLZv4i32:
    case AArch64::NEGv4i32:
    case AArch64::UADDLVv4i32v:
    case AArch64::SADDLVv4i32v:
    case AArch64::UADDLPv4i32_v2i64:
    case AArch64::SADDLPv4i32_v2i64:
    case AArch64::UADALPv4i32_v2i64:
    case AArch64::SADALPv4i32_v2i64:
    case AArch64::ADDVv4i32v:
      eltSize = 32;
      numElts = 4;
      break;
    case AArch64::ABSv8i8:
    case AArch64::CLZv8i8:
    case AArch64::UADDLVv8i8v:
    case AArch64::SADDLVv8i8v:
    case AArch64::UADDLPv8i8_v4i16:
    case AArch64::SADDLPv8i8_v4i16:
    case AArch64::UADALPv8i8_v4i16:
    case AArch64::SADALPv8i8_v4i16:
    case AArch64::NEGv8i8:
    case AArch64::NOTv8i8:
    case AArch64::CNTv8i8:
    case AArch64::ADDVv8i8v:
    case AArch64::RBITv8i8:
      eltSize = 8;
      numElts = 8;
      break;
    case AArch64::ABSv16i8:
    case AArch64::CLZv16i8:
    case AArch64::RBITv16i8:
    case AArch64::ADDVv16i8v:
    case AArch64::NEGv16i8:
    case AArch64::UADDLVv16i8v:
    case AArch64::SADDLVv16i8v:
    case AArch64::UADDLPv16i8_v8i16:
    case AArch64::SADDLPv16i8_v8i16:
    case AArch64::UADALPv16i8_v8i16:
    case AArch64::SADALPv16i8_v8i16:
    case AArch64::NOTv16i8:
    case AArch64::CNTv16i8:
      eltSize = 8;
      numElts = 16;
      break;
    default:
      assert(false);
    }

    auto *vTy = getVecTy(eltSize, numElts);

    // Perform the operation
    switch (opcode) {
    case AArch64::ABSv1i64:
    case AArch64::ABSv8i8:
    case AArch64::ABSv4i16:
    case AArch64::ABSv2i32:
    case AArch64::ABSv2i64:
    case AArch64::ABSv16i8:
    case AArch64::ABSv8i16:
    case AArch64::ABSv4i32: {
      auto src_vector = createBitCast(src, vTy);
      auto res = createAbs(src_vector);
      updateOutputReg(res);
      break;
    }
    case AArch64::CLZv2i32:
    case AArch64::CLZv4i16:
    case AArch64::CLZv8i8:
    case AArch64::CLZv16i8:
    case AArch64::CLZv8i16:
    case AArch64::CLZv4i32: {
      auto src_vector = createBitCast(src, vTy);
      auto res = createCtlz(src_vector);
      updateOutputReg(res);
      break;
    }
    case AArch64::ADDVv16i8v:
    case AArch64::ADDVv8i16v:
    case AArch64::ADDVv4i32v:
    case AArch64::ADDVv8i8v:
    case AArch64::ADDVv4i16v: {
      auto src_vector = createBitCast(src, vTy);
      Value *sum = getUnsignedIntConst(0, eltSize);
      for (unsigned i = 0; i < numElts; ++i) {
        auto elt = createExtractElement(src_vector, i);
        sum = createAdd(sum, elt);
      }
      // sum goes into the bottom lane, all others are zeroed out
      auto zero = getZeroIntVec(numElts, eltSize);
      auto res = createInsertElement(zero, sum, 0);
      updateOutputReg(res);
      break;
    }
    case AArch64::UADDLPv4i16_v2i32:
    case AArch64::UADDLPv2i32_v1i64:
    case AArch64::UADDLPv8i8_v4i16:
    case AArch64::UADDLPv8i16_v4i32:
    case AArch64::UADDLPv4i32_v2i64:
    case AArch64::UADDLPv16i8_v8i16: {
      auto src_vector = createBitCast(src, vTy);
      auto res = addPairs(src_vector, eltSize, numElts, extKind::ZExt);
      updateOutputReg(res);
      break;
    }
    case AArch64::SADDLPv4i16_v2i32:
    case AArch64::SADDLPv2i32_v1i64:
    case AArch64::SADDLPv8i8_v4i16:
    case AArch64::SADDLPv8i16_v4i32:
    case AArch64::SADDLPv4i32_v2i64:
    case AArch64::SADDLPv16i8_v8i16: {
      auto src_vector = createBitCast(src, vTy);
      auto res = addPairs(src_vector, eltSize, numElts, extKind::SExt);
      updateOutputReg(res);
      break;
    }
    case AArch64::UADALPv4i16_v2i32:
    case AArch64::UADALPv2i32_v1i64:
    case AArch64::UADALPv8i8_v4i16:
    case AArch64::UADALPv8i16_v4i32:
    case AArch64::UADALPv4i32_v2i64:
    case AArch64::UADALPv16i8_v8i16:
    case AArch64::SADALPv4i16_v2i32:
    case AArch64::SADALPv2i32_v1i64:
    case AArch64::SADALPv8i8_v4i16:
    case AArch64::SADALPv8i16_v4i32:
    case AArch64::SADALPv4i32_v2i64:
    case AArch64::SADALPv16i8_v8i16: {
      auto ext = opcode == AArch64::SADALPv4i16_v2i32 ||
                         opcode == AArch64::SADALPv2i32_v1i64 ||
                         opcode == AArch64::SADALPv8i8_v4i16 ||
                         opcode == AArch64::SADALPv8i16_v4i32 ||
                         opcode == AArch64::SADALPv4i32_v2i64 ||
                         opcode == AArch64::SADALPv16i8_v8i16
                     ? extKind::SExt
                     : extKind::ZExt;
      auto src2 = readFromOperand(2);
      auto src2_vector = createBitCast(src2, vTy);
      auto sum = addPairs(src2_vector, eltSize, numElts, ext);
      auto *bigTy = getVecTy(2 * eltSize, numElts / 2);
      Value *res = getUndefVec(numElts / 2, 2 * eltSize);
      auto src_vector = createBitCast(src, bigTy);
      for (unsigned i = 0; i < numElts / 2; ++i) {
        auto elt1 = createExtractElement(src_vector, i);
        auto elt2 = createExtractElement(sum, i);
        auto add = createAdd(elt1, elt2);
        res = createInsertElement(res, add, i);
      }
      updateOutputReg(res);
      break;
    }
    case AArch64::UADDLVv8i16v:
    case AArch64::UADDLVv4i32v:
    case AArch64::UADDLVv8i8v:
    case AArch64::UADDLVv4i16v:
    case AArch64::UADDLVv16i8v:
    case AArch64::SADDLVv8i8v:
    case AArch64::SADDLVv16i8v:
    case AArch64::SADDLVv4i16v:
    case AArch64::SADDLVv8i16v:
    case AArch64::SADDLVv4i32v: {
      bool isSigned =
          opcode == AArch64::SADDLVv8i8v || opcode == AArch64::SADDLVv16i8v ||
          opcode == AArch64::SADDLVv4i16v || opcode == AArch64::SADDLVv8i16v ||
          opcode == AArch64::SADDLVv4i32v;
      auto src_vector = createBitCast(src, vTy);
      auto bigTy = getIntTy(2 * eltSize);
      Value *sum = getUnsignedIntConst(0, 2 * eltSize);
      for (unsigned i = 0; i < numElts; ++i) {
        auto elt = createExtractElement(src_vector, i);
        auto ext = isSigned ? createSExt(elt, bigTy) : createZExt(elt, bigTy);
        sum = createAdd(sum, ext);
      }
      updateOutputReg(sum);
      break;
    }
    case AArch64::NOTv8i8:
    case AArch64::NOTv16i8: {
      auto src_vector = createBitCast(src, vTy);
      updateOutputReg(createNot(src_vector));
      break;
    }
    case AArch64::RBITv8i8:
    case AArch64::RBITv16i8: {
      auto src_vector = createBitCast(src, vTy);
      auto result = createBitReverse(src_vector);
      updateOutputReg(result);
      break;
    }
    case AArch64::CNTv8i8:
    case AArch64::CNTv16i8: {
      auto src_vector = createBitCast(src, vTy);
      updateOutputReg(createCtPop(src_vector));
      break;
    }
    case AArch64::NEGv1i64:
    case AArch64::NEGv4i16:
    case AArch64::NEGv8i8:
    case AArch64::NEGv16i8:
    case AArch64::NEGv2i32:
    case AArch64::NEGv8i16:
    case AArch64::NEGv2i64:
    case AArch64::NEGv4i32: {
      auto src_vector = createBitCast(src, vTy);
      auto zeroes = ConstantInt::get(vTy, APInt::getZero(eltSize));
      updateOutputReg(createSub(zeroes, src_vector));
      break;
    }
    default:
      assert(false);
    }
    break;
  }
  default:
    *out << funcToString(liftedFn);
    *out << "\nError "
            "detected----------partially-lifted-arm-target----------\n";
    visitError();
  }
}

Value *arm2llvm::createRegFileAndStack() {
  auto i8 = getIntTy(8);

  // allocate storage for the main register file
  for (unsigned Reg = AArch64::X0; Reg <= AArch64::X28; ++Reg) {
    stringstream Name;
    Name << "X" << Reg - AArch64::X0;
    createRegStorage(Reg, 64, Name.str());
    initialReg[Reg - AArch64::X0] = readFromRegOld(Reg);
  }

  // Allocating storage for thirty-two 128 bit NEON registers
  // https://developer.arm.com/documentation/den0024/a/AArch64-Floating-point-and-NEON?lang=en
  for (unsigned Reg = AArch64::Q0; Reg <= AArch64::Q31; ++Reg) {
    stringstream Name;
    Name << "Q" << Reg - AArch64::Q0;
    createRegStorage(Reg, 128, Name.str());
  }

  createRegStorage(AArch64::SP, 64, "SP");
  // load the base address for the stack memory; FIXME: this works
  // for accessing parameters but it doesn't support the general
  // case
  auto paramBase =
      createGEP(i8, stackMem, {getUnsignedIntConst(stackBytes, 64)}, "");
  createStore(paramBase, RegFile[AArch64::SP]);
  initialSP = readFromRegOld(AArch64::SP);

  // FP is X29; we'll initialize it later
  createRegStorage(AArch64::FP, 64, "FP");

  // LR is X30; FIXME initialize this
  createRegStorage(AArch64::LR, 64, "LR");

  // initializing to zero makes loads from XZR work; stores are
  // handled in updateReg()
  createRegStorage(AArch64::XZR, 64, "XZR");
  createStore(getUnsignedIntConst(0, 64), RegFile[AArch64::XZR]);

  // allocate storage for PSTATE
  createRegStorage(AArch64::N, 1, "N");
  createRegStorage(AArch64::Z, 1, "Z");
  createRegStorage(AArch64::C, 1, "C");
  createRegStorage(AArch64::V, 1, "V");

  return paramBase;
}
