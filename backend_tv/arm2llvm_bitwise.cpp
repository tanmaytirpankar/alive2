#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

void arm2llvm::lift_eorr() {
  auto lhs = readFromOperand(1);
  auto rhs = readFromOperand(2);
  rhs = regShift(rhs, getImm(3));
  auto result = createXor(lhs, rhs);
  updateOutputReg(result);
}

void arm2llvm::lift_eori(unsigned opcode) {
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
}

void arm2llvm::lift_sbfm(unsigned opcode) {
  auto i8 = getIntTy(8);
  auto i16 = getIntTy(16);
  auto i32 = getIntTy(32);
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
    return;
  }

  // SXTB
  if (immr == 0 && imms == 7) {
    auto trunc = createTrunc(src, i8);
    auto dst = createSExt(trunc, ty);
    updateOutputReg(dst);
    return;
  }

  // SXTH
  if (immr == 0 && imms == 15) {
    auto trunc = createTrunc(src, i16);
    auto dst = createSExt(trunc, ty);
    updateOutputReg(dst);
    return;
  }

  // SXTW
  if (immr == 0 && imms == 31) {
    auto trunc = createTrunc(src, i32);
    auto dst = createSExt(trunc, ty);
    updateOutputReg(dst);
    return;
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
    return;
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
}

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
