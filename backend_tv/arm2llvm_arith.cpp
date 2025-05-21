#include "backend_tv/arm2llvm.h"

void arm2llvm::lift_add(unsigned opcode) {
  Value *a = nullptr;
  Value *b = nullptr;
  bool break_outer_switch = false;

  switch (opcode) {
  case AArch64::ADDWrx:
  case AArch64::ADDSWrx:
  case AArch64::ADDXrx:
  case AArch64::ADDSXrx: {
    auto size = getInstSize(opcode);
    auto ty = getIntTy(size);
    auto extendImm = getImm(3);
    auto extendType = ((extendImm >> 3) & 0x7);
    auto isSigned = (extendType & 0x4) != 0;

    // extendSize is necessary so that we can start with the word size
    // ARM wants us to (byte, half, full) and then sign extend to a new
    // size. Without extendSize being used for a trunc, a lot of masking
    // and more manual work to sign extend would be necessary
    unsigned extendSize = 8 << (extendType & 0x3);
    auto shift = extendImm & 0x7;

    b = readFromOperand(2);

    // Make sure to not to trunc to the same size as the parameter.
    // Sometimes ADDrx is generated using 32 bit registers and "extends" to
    // a 32 bit value. This is seen as a type error by LLVM, but is valid
    // ARM
    if (extendSize != (unsigned)size) {
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
    if (b->getType()->isPointerTy()) {
      // This control path is for PC-Relative addressing.
      auto reg = CurInst->getOperand(0).getReg();
      updateReg(b, reg);
      break_outer_switch = true;
      break;
    }
    break;
  }
  // Let the PC-Relative addressing control path break here instead of the
  // end of the case as we do not want any more instructions created.
  if (break_outer_switch)
    return;

  a = readFromOperand(1);

  if (has_s(opcode)) {
    auto sadd = createSAddOverflow(a, b);
    auto result = createExtractValue(sadd, {0});
    auto new_v = createExtractValue(sadd, {1});

    auto uadd = createUAddOverflow(a, b);
    auto new_c = createExtractValue(uadd, {1});

    setV(new_v);
    setC(new_c);
    setNUsingResult(result);
    setZUsingResult(result);
    updateOutputReg(result);
  }

  updateOutputReg(createAdd(a, b));
}

void arm2llvm::lift_adc_sbc(unsigned opcode) {
  auto a = readFromOperand(1);
  auto b = readFromOperand(2);
  
  switch (opcode) {
  case AArch64::SBCWr:
  case AArch64::SBCXr:
  case AArch64::SBCSWr:
  case AArch64::SBCSXr:
    b = createNot(b);
    return;
  }
  
  auto [res, flags] = addWithCarry(a, b, getC());
  updateOutputReg(res);
  
  if (has_s(opcode)) {
    auto [n, z, c, v] = flags;
    setN(n);
    setZ(z);
    setC(c);
    setV(v);
  }
}

