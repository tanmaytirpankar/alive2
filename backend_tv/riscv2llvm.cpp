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
                       const MCSubtargetInfo &STI, const MCInstrAnalysis &IA,
		       unsigned SentinelNOP)
  : mc2llvm(LiftedModule, MF, srcFn, InstPrinter, MCE, STI, IA, SentinelNOP) {}

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
  Value *addr = RegFile[Reg];
  *out << "addr = " << addr << "\n";
  return createLoad(ty, addr);
}

void riscv2llvm::platformInit() {
  auto i64ty = getIntTy(64);
  auto i8ty = getIntTy(8);

  // allocate storage for the main register file
  for (unsigned Reg = RISCV::X0; Reg <= RISCV::X31; ++Reg) {
    stringstream Name;
    Name << "X" << Reg - RISCV::X0;
    createRegStorage(Reg, 64, Name.str());
    // initialReg[Reg - RISCV::X0] = readFromReg(Reg, i64ty);
  }

  *out << "created scalar registers\n";
  
  // TODO vector registers

  auto paramBase =
      createGEP(i8ty, stackMem, {getUnsignedIntConst(stackBytes, 64)}, "");
  createStore(paramBase, RegFile[RISCV::X2]);
  initialSP = readFromReg(RISCV::X2, i64ty);

  // initializing to zero makes loads from XZR work; stores are
  // handled in updateReg()
  createStore(getUnsignedIntConst(0, 64), RegFile[RISCV::X0]);

  *out << "about to do callee-side ABI stuff\n";

  // implement the callee side of the ABI; FIXME -- this code only
  // supports integer parameters <= 64 bits and will require
  // significant generalization to handle large parameters
  unsigned vecArgNum = 0;
  unsigned scalarArgNum = 0;
  unsigned stackSlot = 0;

  for (Function::arg_iterator arg = liftedFn->arg_begin(),
                              E = liftedFn->arg_end(),
                              srcArg = srcFn.arg_begin();
       arg != E; ++arg, ++srcArg) {
    *out << "  processing " << getBitWidth(arg)
         << "-bit arg with vecArgNum = " << vecArgNum
         << ", scalarArgNum = " << scalarArgNum
         << ", stackSlot = " << stackSlot;
    auto *argTy = arg->getType();
    auto *val =
        enforceSExtZExt(arg, srcArg->hasSExtAttr(), srcArg->hasZExtAttr());

    // first 8 integer parameters go in the first 8 integer registers
    if ((argTy->isIntegerTy() || argTy->isPointerTy()) && scalarArgNum < 8) {
      auto Reg = RISCV::X10 + scalarArgNum;
      createStore(val, RegFile[Reg]);
      ++scalarArgNum;
      goto end;
    }

    assert(false && "unimplemented");
#if 0
    
    // first 8 vector/FP parameters go in the first 8 vector registers
    if ((argTy->isVectorTy() || argTy->isFloatingPointTy()) && vecArgNum < 8) {
      auto Reg = AArch64::Q0 + vecArgNum;
      createStore(val, RegFile[Reg]);
      ++vecArgNum;
      goto end;
    }

    // anything else goes onto the stack
    {
      // 128-bit alignment required for 128-bit arguments
      if ((getBitWidth(val) == 128) && ((stackSlot % 2) != 0)) {
        ++stackSlot;
        *out << " (actual stack slot = " << stackSlot << ")";
      }

      if (stackSlot >= numStackSlots) {
        *out << "\nERROR: maximum stack slots for parameter values "
                "exceeded\n\n";
        exit(-1);
      }

      auto addr =
          createGEP(i64, paramBase, {getUnsignedIntConst(stackSlot, 64)}, "");
      createStore(val, addr);

      if (getBitWidth(val) == 64) {
        stackSlot += 1;
      } else if (getBitWidth(val) == 128) {
        stackSlot += 2;
      } else {
        assert(false);
      }
    }
#endif

  end:
    *out << "\n";
  }

  *out << "done with callee-side ABI stuff\n";
}

void riscv2llvm::doReturn() {
  assert(false);
}
