#include "backend_tv/arm2llvm.h"

#include <vector>

using namespace lifter;
using namespace llvm;
using namespace std;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_stp(unsigned opcode) {
  auto i32 = getIntTy(32);
  auto i64 = getIntTy(64);
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
}

void arm2llvm::lift_str_5(unsigned opcode) {
  auto i64 = getIntTy(64);
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
  bool isPost =
      opcode == AArch64::ST1Onev8b_POST || opcode == AArch64::ST1Onev16b_POST ||
      opcode == AArch64::ST1Onev4h_POST || opcode == AArch64::ST1Onev8h_POST ||
      opcode == AArch64::ST1Onev2s_POST || opcode == AArch64::ST1Onev4s_POST ||
      opcode == AArch64::ST1Onev1d_POST || opcode == AArch64::ST1Onev2d_POST ||
      opcode == AArch64::ST1Twov8b_POST || opcode == AArch64::ST1Twov16b_POST ||
      opcode == AArch64::ST1Twov4h_POST || opcode == AArch64::ST1Twov8h_POST ||
      opcode == AArch64::ST1Twov2s_POST || opcode == AArch64::ST1Twov4s_POST ||
      opcode == AArch64::ST1Twov1d_POST || opcode == AArch64::ST1Twov2d_POST ||
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
      opcode == AArch64::ST1Fourv2d_POST || opcode == AArch64::ST2Twov8b_POST ||
      opcode == AArch64::ST2Twov16b_POST || opcode == AArch64::ST2Twov4h_POST ||
      opcode == AArch64::ST2Twov8h_POST || opcode == AArch64::ST2Twov2s_POST ||
      opcode == AArch64::ST2Twov4s_POST || opcode == AArch64::ST2Twov2d_POST ||
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
      opcode == AArch64::ST4Fourv4s_POST || opcode == AArch64::ST4Fourv2d_POST;

  auto regCounter = decodeRegSet(CurInst->getOperand(isPost ? 1 : 0).getReg());
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
  auto casted = createBitCast(valueToStore, getVecTy(eltSize, nregs * numElts));
  Value *res = createShuffleVector(casted, mask);

  storeToMemoryImmOffset(base, 0, nregs * numElts * (eltSize / 8), res);

  if (isPost) {
    auto added = createAdd(baseAddr, offset);
    updateOutputReg(added);
  }
}

void arm2llvm::lift_str_4(unsigned opcode) {
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

  auto regCounter = decodeRegSet(CurInst->getOperand(isPost ? 1 : 0).getReg());
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
  auto i64 = getIntTy(64);
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
}

void arm2llvm::lift_str_3(unsigned opcode) {
  auto i8 = getIntTy(8);
  auto i16 = getIntTy(16);
  auto i32 = getIntTy(32);
  auto i64 = getIntTy(64);
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
}

void arm2llvm::lift_str_2(unsigned opcode) {
  auto i8 = getIntTy(8);
  auto i16 = getIntTy(16);
  auto i32 = getIntTy(32);
  auto i64 = getIntTy(64);
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
}

void arm2llvm::lift_str_1(unsigned opcode) {
  auto i8 = getIntTy(8);
  auto i16 = getIntTy(16);
  auto i32 = getIntTy(32);
  auto i64 = getIntTy(64);
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
}
