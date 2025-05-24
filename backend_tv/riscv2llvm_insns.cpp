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

  case RISCV::BLT:
  case RISCV::BLTU: {
    auto a = readFromRegOperand(0);
    auto b = readFromRegOperand(1);
    auto &jmp_tgt_op = CurInst->getOperand(2);

    assert(jmp_tgt_op.isExpr() && "expected expression");
    assert((jmp_tgt_op.getExpr()->getKind() == MCExpr::ExprKind::SymbolRef) &&
           "expected symbol ref as bcc operand");
    const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*jmp_tgt_op.getExpr());
    const MCSymbol &Sym = SRE.getSymbol();

    auto *dst_true = getBBByName(Sym.getName());

    assert(MCBB->getSuccs().size() == 1 || MCBB->getSuccs().size() == 2);
    const string *dst_false_name = nullptr;
    for (auto &succ : MCBB->getSuccs()) {
      if (succ->getName() != Sym.getName()) {
        dst_false_name = &succ->getName();
        break;
      }
    }
    auto *dst_false =
        getBBByName(dst_false_name ? *dst_false_name : Sym.getName());

    Value *cond;
    switch (opcode) {
    case RISCV::BLT:
      cond = createICmp(ICmpInst::Predicate::ICMP_SLT, a, b);
      break;
    case RISCV::BLTU:
      cond = createICmp(ICmpInst::Predicate::ICMP_ULT, a, b);
      break;
    default:
      assert(false);
    }
    createBranch(cond, dst_true, dst_false);
    break;
  }

  case RISCV::ADD:
  case RISCV::SUB:
  case RISCV::C_ADD:
  case RISCV::C_SUB: {
    auto a = readFromRegOperand(1);
    auto b = readFromRegOperand(2);
    Value *res;
    switch (opcode) {
    case RISCV::ADD:
    case RISCV::C_ADD:
      res = createAdd(a, b);
      break;
    case RISCV::SUB:
    case RISCV::C_SUB:
      res = createSub(a, b);
      break;
    default:
      assert(false);
    }
    updateOutputReg(res);
    break;
  }

  case RISCV::C_ADDW:
  case RISCV::C_SUBW: {
    auto a = readFromRegOperand(1);
    auto b = readFromRegOperand(2);
    auto a32 = createTrunc(a, i32ty);
    auto b32 = createTrunc(b, i32ty);
    Value *res;
    switch (opcode) {
    case RISCV::C_ADDW:
      res = createAdd(a32, b32);
      break;
    case RISCV::C_SUBW:
      res = createSub(a32, b32);
      break;
    default:
      assert(false);
    }
    auto resExt = createSExt(res, i64ty);
    updateOutputReg(resExt);
    break;
  }

  case RISCV::C_ADDI:
  case RISCV::ADDI: {
    auto a = readFromRegOperand(1);
    auto b = readFromImmOperand(2, 64);
    auto res = createAdd(a, b);
    updateOutputReg(res);
    break;
  }

  case RISCV::C_ADDIW:
  case RISCV::ADDIW: {
    auto a = readFromRegOperand(1);
    auto b = readFromImmOperand(2, 32);
    auto a32 = createTrunc(a, i32ty);
    auto res = createAdd(a32, b);
    auto resExt = createSExt(res, i64ty);
    updateOutputReg(resExt);
    break;
  }

  case RISCV::SLTU:
  case RISCV::SLT: {
    auto a = readFromRegOperand(1);
    auto b = readFromRegOperand(2);
    Value *res;
    switch (opcode) {
    case RISCV::SLT:
      res = createICmp(ICmpInst::Predicate::ICMP_SLT, a, b);
      break;
    case RISCV::SLTU:
      res = createICmp(ICmpInst::Predicate::ICMP_ULT, a, b);
      break;
    default:
      assert(false);
    }
    auto resExt = createZExt(res, i64ty);
    updateOutputReg(resExt);
    break;
  }

  case RISCV::SLTI:
  case RISCV::SLTIU: {
    auto a = readFromRegOperand(1);
    auto b = readFromImmOperand(2, 64);
    Value *res;
    switch (opcode) {
    case RISCV::SLTI:
      res = createICmp(ICmpInst::Predicate::ICMP_SLT, a, b);
      break;
    case RISCV::SLTIU:
      res = createICmp(ICmpInst::Predicate::ICMP_ULT, a, b);
      break;
    default:
      assert(false);
    }
    auto resExt = createZExt(res, i64ty);
    updateOutputReg(resExt);
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
