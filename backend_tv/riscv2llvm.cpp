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
 * this thread has some useful details about setting up a RISCV
 * execution environment:
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
    : mc2llvm(LiftedModule, MF, srcFn, InstPrinter, MCE, STI, IA, SentinelNOP) {
}

Value *riscv2llvm::enforceSExtZExt(Value *V, bool isSExt, bool isZExt) {
  // FIXME
  return V;
}

llvm::AllocaInst *riscv2llvm::get_reg(aslp::reg_t regtype, uint64_t num) {
  assert(false);
  return nullptr;
}

Value *riscv2llvm::lookupReg(unsigned Reg) {
  assert(Reg >= RISCV::X0 && Reg <= RISCV::X31);
  return RegFile[Reg];
}

void riscv2llvm::updateReg(Value *V, uint64_t Reg) {
  // important -- squash updates to the zero register
  if (Reg == RISCV::X0)
    return;
  createStore(V, lookupReg(Reg));
}

void riscv2llvm::updateOutputReg(Value *V, bool SExt) {
  assert(!SExt); // FIXME implement
  auto destReg = CurInst->getOperand(0).getReg();
  updateReg(V, destReg);
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

Value *riscv2llvm::readFromRegOperand(int idx) {
  auto op = CurInst->getOperand(idx);
  assert(op.isReg());
  return readFromReg(op.getReg());
}

Value *riscv2llvm::readFromReg(unsigned Reg) {
  auto addr = lookupReg(Reg);
  *out << "addr = " << addr << "\n";
  auto i64ty = getIntTy(64);
  return createLoad(i64ty, addr);
}

void riscv2llvm::doReturn() {
  auto i32ty = getIntTy(32);
  auto i64ty = getIntTy(64);

  // FIXME add ABI checks

  auto *retTyp = srcFn.getReturnType();
  if (retTyp->isVoidTy()) {
    createReturn(nullptr);
  } else {
    Value *retVal = nullptr;
    // FIXME handle vectors and FP
    retVal = readFromReg(RISCV::X10);
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
        auto trunc = createTrunc(retVal, i32ty);
        retVal = createZExt(trunc, i64ty);
      }

      if ((retTyp->isVectorTy() || retTyp->isFloatingPointTy()) &&
          !has_ret_attr)
        retVal = createBitCast(retVal, retTyp);
    }
    createReturn(retVal);
  }
}

void riscv2llvm::platformInit() {
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
  initialSP = readFromReg(RISCV::X2);

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

void riscv2llvm::lift(MCInst &I) {
  auto opcode = I.getOpcode();
  // StringRef instStr = InstPrinter->getOpcodeName(opcode);
  auto newbb = BasicBlock::Create(Ctx, "lifter_" + nextName(), liftedFn);
  createBranch(newbb);
  LLVMBB = newbb;

  // auto i1ty = getIntTy(1);
  // auto i8ty = getIntTy(8);
  // auto i16ty = getIntTy(16);
  auto i32ty = getIntTy(32);
  auto i64ty = getIntTy(64);
  // auto i128ty = getIntTy(128);

  switch (opcode) {

  case RISCV::C_NOP_HINT:
    break;

  case RISCV::C_J: {
    /*
     * copied from ARM -- maybe move to portable code
     */
    BasicBlock *dst{nullptr};
    // JDR: I don't understand this
    if (CurInst->getOperand(0).isImm()) {
      // handles the case when we add an entry block with no predecessors
      auto &dst_name = MF.BBs[getImm(0)].getName();
      dst = getBBByName(dst_name);
    } else {
      dst = getBB(CurInst->getOperand(0));
    }
    if (dst) {
      createBranch(dst);
    } else {
      // ok, if we don't have a destination block then we left this
      // dangling on purpose, with the assumption that it's a tail
      // call
      doDirectCall();
      doReturn();
    }
    break;
  }

  case RISCV::C_SUBW: {
    auto a = readFromRegOperand(1);
    auto b = readFromRegOperand(2);
    auto a32 = createTrunc(a, i32ty);
    auto b32 = createTrunc(b, i32ty);
    auto sub = createSub(a32, b32);
    auto res = createSExt(sub, i64ty);
    updateOutputReg(res);
    break;
  }

  case RISCV::C_JR:
    doReturn();
    break;

  default:
    *out << "unhandled instruction\n";
    exit(-1);
  }
}

