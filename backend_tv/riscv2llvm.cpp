#include "backend_tv/riscv2llvm.h"

#define GET_INSTRINFO_ENUM
#include "Target/RISCV/RISCVGenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/RISCV/RISCVGenRegisterInfo.inc"

using namespace std;

riscv2llvm::riscv2llvm(Module *LiftedModule, MCFunction &MF, Function &srcFn,
                       MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
                       const MCSubtargetInfo &STI, const MCInstrAnalysis &IA)
    : mc2llvm(LiftedModule, MF, srcFn, InstPrinter, MCE, STI, IA) {}

Value *riscv2llvm::enforceSExtZExt(Value *V, bool isSExt, bool isZExt) {
  assert(false);
  return nullptr;
}

llvm::AllocaInst *riscv2llvm::get_reg(aslp::reg_t regtype, uint64_t num) {
  assert(false);
  return nullptr;
}

void riscv2llvm::updateOutputReg(Value *V, bool SExt) {
  assert(false);
}

Value *riscv2llvm::makeLoadWithOffset(Value *base, Value *offset, int size) {
  assert(false);
  return nullptr;
}

Value *riscv2llvm::getIndexedElement(unsigned idx, unsigned eltSize,
                                     unsigned reg) {
  assert(false);
  return nullptr;
}

void riscv2llvm::doCall(FunctionCallee FC, CallInst *llvmCI,
                        const string &calleeName) {
  assert(false);
}

void riscv2llvm::lift(MCInst &I) {
  assert(false);
}

Value *riscv2llvm::createRegFileAndStack() {
  assert(false);
  return nullptr;
}

void riscv2llvm::doReturn() {
  assert(false);
}
