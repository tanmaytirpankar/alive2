#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

void arm2llvm::lift_bfm(unsigned opcode) {
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
}

void arm2llvm::lift_ubfm(unsigned opcode) {
  auto size = getInstSize(opcode);
  auto src = readFromOperand(1);
  auto immr = getImm(2);
  auto imms = getImm(3);

  // LSL is preferred when imms != 31 and imms + 1 == immr
  if (size == 32 && imms != 31 && imms + 1 == immr) {
    auto dst = createMaskedShl(src, getUnsignedIntConst(31 - imms, size));
    updateOutputReg(dst);
    return;
  }

  // LSL is preferred when imms != 63 and imms + 1 == immr
  if (size == 64 && imms != 63 && imms + 1 == immr) {
    auto dst = createMaskedShl(src, getUnsignedIntConst(63 - imms, size));
    updateOutputReg(dst);
    return;
  }

  // LSR is preferred when imms == 31 or 63 (size - 1)
  if (imms == size - 1) {
    auto dst = createMaskedLShr(src, getUnsignedIntConst(immr, size));
    updateOutputReg(dst);
    return;
  }

  // UBFIZ
  if (imms < immr) {
    auto pos = size - immr;
    auto width = imms + 1;
    auto mask = ((uint64_t)1 << width) - 1;
    auto masked = createAnd(src, getUnsignedIntConst(mask, size));
    auto shifted = createMaskedShl(masked, getUnsignedIntConst(pos, size));
    updateOutputReg(shifted);
    return;
  }

  // UXTB
  if (immr == 0 && imms == 7) {
    auto mask = ((uint64_t)1 << 8) - 1;
    auto masked = createAnd(src, getUnsignedIntConst(mask, size));
    updateOutputReg(masked);
    return;
  }

  // UXTH
  if (immr == 0 && imms == 15) {
    auto mask = ((uint64_t)1 << 16) - 1;
    auto masked = createAnd(src, getUnsignedIntConst(mask, size));
    updateOutputReg(masked);
    return;
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
}

void arm2llvm::lift_orn() {
  auto lhs = readFromOperand(1);
  auto rhs = readFromOperand(2);
  rhs = regShift(rhs, getImm(3));
  auto not_rhs = createNot(rhs);
  auto ident = createOr(lhs, not_rhs);
  updateOutputReg(ident);
}

void arm2llvm::lift_lsrv() {
  auto lhs = readFromOperand(1);
  auto rhs = readFromOperand(2);
  auto exp = createMaskedLShr(lhs, rhs);
  updateOutputReg(exp);
}

void arm2llvm::lift_lslv() {
  auto lhs = readFromOperand(1);
  auto rhs = readFromOperand(2);
  auto exp = createMaskedShl(lhs, rhs);
  updateOutputReg(exp);
}

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
