#include "backend_tv/arm2llvm.h"

using namespace lifter;
using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

void arm2llvm::lift_frint(unsigned opcode) {
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
}

void arm2llvm::lift_fcvt_4() {
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
}

void arm2llvm::lift_fcvt_3() {
  // TODO
    *out << "\nERROR: only float and double supported (not bfloat, half, "
            "fp128, etc.)\n\n";
    exit(-1);
}

void arm2llvm::lift_fcvt_1(unsigned opcode) {
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
}

void arm2llvm::lift_fneg() {
  auto operandSize = getRegSize(CurInst->getOperand(1).getReg());
  auto fVal = readFromFPOperand(1, operandSize);
  auto res = createFNeg(fVal);
  updateOutputReg(res);
}
