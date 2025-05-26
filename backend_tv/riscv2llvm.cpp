#include "backend_tv/riscv2llvm.h"

#include <cmath>
#include <vector>

#define GET_INSTRINFO_ENUM
#include "Target/RISCV/RISCVGenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/RISCV/RISCVGenRegisterInfo.inc"

using namespace std;
using namespace lifter;
using namespace llvm;

riscv2llvm::riscv2llvm(Module *LiftedModule, MCStreamerWrapper &Str, Function &srcFn,
                       MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
                       const MCSubtargetInfo &STI, const MCInstrAnalysis &IA,
                       unsigned SentinelNOP)
    : mc2llvm(LiftedModule, Str, srcFn, InstPrinter, MCE, STI, IA, SentinelNOP) {
}

// TODO -- move this up to mc2llvm so the ARM lifter can use it too
tuple<BasicBlock *, BasicBlock *> riscv2llvm::getBranchTargetsOperand(int op) {
  auto &jmp_tgt_op = CurInst->getOperand(op);
  assert(jmp_tgt_op.isExpr() && "expected expression");
  assert((jmp_tgt_op.getExpr()->getKind() == MCExpr::ExprKind::SymbolRef) &&
         "expected symbol ref as bcc operand");
  const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*jmp_tgt_op.getExpr());
  const MCSymbol &Sym = SRE.getSymbol();
  auto dst_true = getBBByName(Sym.getName());
  assert(MCBB->getSuccs().size() == 1 || MCBB->getSuccs().size() == 2);
  const string *dst_false_name = nullptr;
  for (auto &succ : MCBB->getSuccs()) {
    if (succ->getName() != Sym.getName()) {
      dst_false_name = &succ->getName();
      break;
    }
  }
  auto dst_false =
      getBBByName(dst_false_name ? *dst_false_name : Sym.getName());
  return make_pair(dst_true, dst_false);
}

Value *riscv2llvm::enforceSExtZExt(Value *V, bool isSExt, bool isZExt) {
  assert(!(isSExt && isZExt));

  auto i8 = getIntTy(8);
  auto i64 = getIntTy(64);
  auto argTy = V->getType();
  unsigned targetWidth = 64;

  // no work needed
  if (argTy->isPointerTy() || argTy->isVoidTy())
    return V;

  if (argTy->isVectorTy() || argTy->isFloatingPointTy()) {
    assert(false && "to be implemented!");
  }

  assert(argTy->isIntegerTy());

  /*
   * the ABI states that a Boolean value is a byte containing value 0
   * or 1. as we saw in AArch64, this raises the question of whether a
   * signext parameter of this type is sign-extended from a width of 1
   * or 8 bits. the answer there was 1, so we'll do the same thing here
   */
  if (getBitWidth(V) == 1) {
    if (isSExt)
      V = createSExt(V, i64);
    else
      V = createZExt(V, i8);
  }

  if (isZExt && getBitWidth(V) < targetWidth)
    V = createZExt(V, getIntTy(targetWidth));

  if (isSExt && getBitWidth(V) < targetWidth)
    V = createSExt(V, getIntTy(targetWidth));

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

Value *riscv2llvm::readFromImmOperand(int idx, unsigned immed_width,
                                      unsigned result_width) {
  assert(result_width >= immed_width);
  auto op = CurInst->getOperand(idx);
  assert(op.isImm());
  Value *imm = getSignedIntConst(op.getImm(), immed_width);
  if (result_width > immed_width)
    imm = createSExt(imm, getIntTy(result_width));
  return imm;
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
