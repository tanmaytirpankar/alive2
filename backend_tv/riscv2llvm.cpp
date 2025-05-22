#include "backend_tv/riscv2llvm.h"

#include <cmath>
#include <vector>

/*
 * primary references for this file are the user mode ISA reference:
 *
 * https://drive.google.com/file/d/1uviu1nH-tScFfgrovvFCrj7Omv8tFtkp/view
 *
 * and the ABI document:
 *
 * https://drive.google.com/file/d/1Ja_Tpp_5Me583CGVD-BIZMlgGBnlKU4R/view
 *
 * this thread has some good details on setting up a RISCV execution
 * environment:
 *
 * https://www.reddit.com/r/RISCV/comments/10k805c/how_can_i_buildrun_riscv_assembly_on_macos/
 */

#define GET_INSTRINFO_ENUM
#include "Target/RISCV/RISCVGenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/RISCV/RISCVGenRegisterInfo.inc"

using namespace std;
using namespace lifter;
using namespace llvm;

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

Value *riscv2llvm::readFromReg(unsigned Reg, Type *ty) {
  assert(Reg >= RISCV::X0 && Reg <= RISCV::X31);
  return createLoad(ty, RegFile[Reg - RISCV::X0]);
}

Value *riscv2llvm::createRegFileAndStack() {
  auto i64ty = getIntTy(64);
  auto i8ty = getIntTy(8);

  // allocate storage for the main register file
  for (unsigned Reg = RISCV::X0; Reg <= RISCV::X31; ++Reg) {
    stringstream Name;
    Name << "X" << Reg - RISCV::X0;
    createRegStorage(Reg, 64, Name.str());
    initialReg[Reg - RISCV::X0] = readFromReg(Reg, i64ty);
  }

  // TODO vector registers

  auto paramBase =
      createGEP(i8ty, stackMem, {getUnsignedIntConst(stackBytes, 64)}, "");
  createStore(paramBase, RegFile[RISCV::X2]);
  initialSP = readFromReg(RISCV::X2, i64ty);

  // initializing to zero makes loads from XZR work; stores are
  // handled in updateReg()
  createStore(getUnsignedIntConst(0, 64), RegFile[RISCV::X0]);

  return paramBase;
}

void riscv2llvm::doReturn() {
  assert(false);
}
