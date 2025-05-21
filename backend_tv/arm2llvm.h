#pragma once

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCTargetOptionsCommandFlags.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "backend_tv/arm2llvm.h"
#include "backend_tv/bitutils.h"
#include "backend_tv/lifter.h"
#include "backend_tv/mc2llvm.h"
#include "backend_tv/mcutils.h"
#include "backend_tv/riscv2llvm.h"
#include "backend_tv/streamerwrapper.h"

#include <cmath>
#include <vector>

// avoid collisions with the upstream AArch64 namespace
namespace llvm::AArch64 {
const unsigned N = 100000000;
const unsigned Z = 100000001;
const unsigned C = 100000002;
const unsigned V = 100000003;
} // namespace llvm::AArch64

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

class arm2llvm final : public mc2llvm {
public:
  arm2llvm(Module *LiftedModule, MCFunction &MF, Function &srcFn,
           MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
           const MCSubtargetInfo &STI, const MCInstrAnalysis &IA);

  // Implemented library pseudocode for signed satuaration from A64 ISA manual
  std::tuple<Value *, bool> SignedSatQ(Value *i, unsigned bitWidth);

  // Implemented library pseudocode for unsigned satuaration from A64 ISA manual
  std::tuple<Value *, bool> UnsignedSatQ(Value *i, unsigned bitWidth);

  // Implemented library pseudocode for satuaration from A64 ISA manual
  std::tuple<Value *, bool> SatQ(Value *i, unsigned bitWidth, bool isSigned);

  bool isSIMDandFPRegOperand(MCOperand &op);

  /*
   * the idea here is that if a parameter to the lifted function, or
   * the return value from the lifted function is, for example, 8
   * bits, then we only want to initialize the lower 8 bits of the
   * register or stack slot, with the remaining bits containing junk,
   * in order to detect cases where the compiler incorrectly emits
   * code depending on that junk. on the other hand, if a parameter is
   * signext or zeroext then we have to actually initialize those
   * higher bits.
   *
   * FIXME -- this code was originally developed for scalar parameters
   * and we're mostly sort of hoping it also works for vectors. this
   * should work fine as long as the only vectors we accept are 64 and
   * 128 bits, which seemed (as of Nov 2023) to be the only ones with
   * a stable ABI
   */
  Value *enforceSExtZExt(Value *V, bool isSExt, bool isZExt) override;

  std::tuple<Value *, int, Value *> getStoreParams();

  // Creates instructions to store val in memory pointed by base + offset
  // offset and size are in bytes
  void storeToMemoryImmOffset(Value *base, uint64_t offset, uint64_t size,
                              Value *val);

  unsigned decodeRegSet(unsigned r);

  Value *tblHelper2(std::vector<Value *> &tbl, Value *idx, unsigned i);

  Value *tblHelper(std::vector<Value *> &tbl, Value *idx);

  std::tuple<Value *, int> getParamsLoadImmed();

  Value *makeLoadWithOffset(Value *base, Value *offset, int size) override;

  Value *makeLoadWithOffset(Value *base, int offset, unsigned size);

  std::tuple<Value *, Value *, Value *> getParamsStoreReg();

  void doIndirectCall();

  void doReturn() override;

  std::tuple<Value *, Value *, Value *, Value *> FPCompare(Value *a, Value *b);

  std::tuple<Value *, Value *, Value *, Value *>
  splitImmNZCV(uint64_t imm_flags);

  bool disjoint(const std::set<int> &a, const std::set<int> &b);

  int64_t getImm(int idx);

  enum ExtendType { SXTB, SXTH, SXTW, SXTX, UXTB, UXTH, UXTW, UXTX };

  // Follows the "Library pseudocode for aarch64/instrs/extendreg/ExtendReg"
  // from ARM manual
  // val is always 64 bits and shiftAmt is always 0-4
  Value *extendAndShiftValue(Value *val, enum ExtendType extType, int shiftAmt);

  std::tuple<Value *, Value *> getParamsLoadReg();

  // From https://github.com/agustingianni/retools
  uint64_t VFPExpandImm(uint64_t imm8, unsigned N);

  // From https://github.com/agustingianni/retools
  // Implementation of: bits(64) AdvSIMDExpandImm(bit op, bits(4) cmode, bits(8)
  // imm8)
  uint64_t AdvSIMDExpandImm(unsigned op, unsigned cmode, unsigned imm8);

  std::vector<Value *> marshallArgs(FunctionType *fTy);

  void doCall(FunctionCallee FC, CallInst *llvmCI,
              const std::string &calleeName) override;

  Value *conditionHolds(uint64_t cond);

  std::tuple<Value *, std::tuple<Value *, Value *, Value *, Value *>>
  addWithCarry(Value *l, Value *r, Value *carryIn);

  void setV(Value *V);

  void setZ(Value *V);

  void setN(Value *V);

  void setC(Value *V);

  void setZUsingResult(Value *V);

  void setNUsingResult(Value *V);

  Value *getV();

  Value *getZ();

  Value *getN();

  Value *getC();

  // Creates LLVM IR instructions which take two values with the same
  // number of bits, bit casting them to vectors of numElts elements
  // of size eltSize and doing an operation on them. In cases where
  // LLVM does not have an appropriate vector instruction, we perform
  // the operation element-wise.
  Value *createVectorOp(std::function<Value *(Value *, Value *)> op, Value *a,
                        Value *b, unsigned eltSize, unsigned numElts,
                        bool elementWise, extKind ext, bool splatImm2,
                        bool immShift, bool isUpper, bool operandTypesDiffer);

  Value *getIndexedElement(unsigned idx, unsigned eltSize,
                           unsigned reg) override;

  Value *getIndexedFPElement(unsigned idx, unsigned eltSize, unsigned reg);

  // Returns bitWidth corresponding the registers
  unsigned getRegSize(unsigned Reg);

  // Maps ARM registers to backing registers
  unsigned mapRegToBackingReg(unsigned Reg);

  // return pointer to the backing store for a register, doing the
  // necessary de-aliasing
  Value *dealiasReg(unsigned Reg);

  // always does a full-width read
  //
  // TODO eliminate all uses of this and rename readFromRegTyped to readFromReg
  Value *readFromRegOld(unsigned Reg);

  Value *readFromRegTyped(unsigned Reg, Type *ty);

  Value *readPtrFromReg(unsigned Reg);

  void updateReg(Value *V, uint64_t reg, bool SExt = false);

  Value *readInputReg(int idx);

  // FIXME: stop using this -- instructions should know what they're loading
  // FIXME: then remove getInstSize!
  // TODO: make it so that lshr generates code on register lookups
  // some instructions make use of this, and the semantics need to be
  // worked out
  Value *readFromOperand(int idx, unsigned size = 0);

  Value *readFromFPOperand(int idx, unsigned size);

  Value *readFromVecOperand(int idx, unsigned eltSize, unsigned numElts,
                            bool isUpperHalf = false, bool isFP = false);

  void updateOutputReg(Value *V, bool SExt = false) override;

  Value *splatImm(Value *v, unsigned numElts, unsigned eltSize, bool shift);

  static const std::set<int> s_flag;

  static const std::set<int> instrs_32;

  static const std::set<int> instrs_64;

  static const std::set<int> instrs_128;

  bool has_s(int instr);

  // decodeBitMasks - Decode a logical immediate value in the form
  // "N:immr:imms" (where the immr and imms fields are each 6 bits) into the
  // integer value it represents with regSize bits. Implementation of the
  // DecodeBitMasks function from the ARMv8 manual.
  //
  // WARNING: tmask is untested
  std::pair<uint64_t, uint64_t> decodeBitMasks(uint64_t val, unsigned regSize);

  unsigned getInstSize(int instr);

  // from getShiftType/getShiftValue:
  // https://github.com/llvm/llvm-project/blob/93d1a623cecb6f732db7900baf230a13e6ac6c6a/llvm/lib/Target/AArch64/MCTargetDesc/AArch64AddressingModes.h#L74
  Value *regShift(Value *value, int encodedShift);

  llvm::AllocaInst *get_reg(aslp::reg_t regtype, uint64_t num) override;

  void lift(MCInst &I) override;
  void lift_add(unsigned opcode);
  void lift_adc_sbc(unsigned opcode);
  void lift_branch();
  void lift_bcc();
  void lift_mrs();
  void lift_msr();
  void lift_asrv(unsigned opcode);
  void lift_sub(unsigned opcode);

  Value *createRegFileAndStack() override;
};
