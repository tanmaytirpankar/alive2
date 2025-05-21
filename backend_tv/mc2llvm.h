#pragma once

#include "backend_tv/bitutils.h"
#include "backend_tv/lifter.h"

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

#include "aslp/aslp_bridge.h"

/* FIXME remove these ugh */
using namespace llvm;

namespace lifter {

class MCFunction;
class MCBasicBlock;

class mc2llvm : public aslp::lifter_interface_llvm {
public:
  Module *LiftedModule{nullptr};
  LLVMContext &Ctx = LiftedModule->getContext();
  MCFunction &MF;
  Function &srcFn;
  Function *liftedFn{nullptr};
  MCBasicBlock *MCBB{nullptr};
  BasicBlock *LLVMBB{nullptr};
  MCInstPrinter *InstPrinter{nullptr};
  MCInst *CurInst{nullptr}, *PrevInst{nullptr};
  unsigned armInstNum{0}, llvmInstNum{0};
  std::map<unsigned, Value *> RegFile;
  Value *stackMem{nullptr};
  std::unordered_map<std::string, Constant *> LLVMglobals;
  Value *initialSP, *initialReg[32];
  Function *assertDecl;
  const MCCodeEmitter &MCE;
  const MCSubtargetInfo &STI;
  const MCInstrAnalysis &IA;

  mc2llvm(Module *LiftedModule, MCFunction &MF, Function &srcFn,
          MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
          const MCSubtargetInfo &STI, const MCInstrAnalysis &IA)
      : LiftedModule(LiftedModule), MF(MF), srcFn(srcFn),
        InstPrinter(InstPrinter), MCE{MCE}, STI{STI}, IA{IA},
        DL(srcFn.getParent()->getDataLayout()) {}

  // these are ones that the backend adds to tgt, even when they don't
  // appear at all in src
  const std::unordered_map<std::string, FunctionType *> implicit_intrinsics = {
      {"llvm.memset.p0.i64", FunctionType::get(Type::getVoidTy(Ctx),
                                               {
                                                   PointerType::get(Ctx, 0),
                                                   Type::getInt8Ty(Ctx),
                                                   Type::getInt64Ty(Ctx),
                                                   Type::getInt1Ty(Ctx),
                                               },
                                               false)},
  };

  // intrinsics get their names mangled by LLVM, we need to reverse
  // that, and there's no way to do it other than hard-coding this
  // mapping
  const std::unordered_map<std::string, std::string> intrinsic_names = {
      // memory
      {"memcpy", "llvm.memcpy.p0.p0.i64"},
      {"memset", "llvm.memset.p0.i64"},
      {"memmove", "llvm.memmove.p0.p0.i64"},
      {"__llvm_memset_element_unordered_atomic_16",
       "llvm.memset.element.unordered.atomic.p0.i32"}, // FIXME

      // FP
      {"cosf", "llvm.cos.f32"},
      {"coshf", "llvm.cosh.f32"},
      {"powf", "llvm.pow.f32"},
      {"pow", "llvm.pow.f64"},
      {"expf", "llvm.exp.f32"},
      {"exp", "llvm.exp.f64"},
      {"exp10f", "llvm.exp10.f32"},
      {"exp10", "llvm.exp10.f64"},

      {"ldexpf", "llvm.ldexp.f32.i32"},
      {"sincos", "llvm.cos.f64"},
  };

  std::map<std::string, unsigned> encodingCounts;

  // Map of ADRP MCInsts to the string representations of the operand variable
  // names
  std::unordered_map<MCInst *, std::string> instExprVarMap;
  const DataLayout &DL;

  struct deferredGlobal {
    std::string name;
    GlobalVariable *val;
  };
  std::vector<deferredGlobal> deferredGlobs;

  Function *copyFunctionToTarget(Function *f, const Twine &name);
  Constant *lazyAddGlobal(std::string newGlobal);

  // create lifted globals only on demand -- saves time and clutter for
  // large modules
  Constant *lookupGlobal(const std::string &nm);

  BasicBlock *getBB(MCOperand &jmp_tgt) {
    assert(jmp_tgt.isExpr() && "[getBB] expected expression operand");
    assert((jmp_tgt.getExpr()->getKind() == MCExpr::ExprKind::SymbolRef) &&
           "[getBB] expected symbol ref as jump operand");
    const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*jmp_tgt.getExpr());
    const MCSymbol &Sym = SRE.getSymbol();
    StringRef name = Sym.getName();
    for (auto &bb : *liftedFn) {
      if (bb.getName() == name)
        return &bb;
    }
    return nullptr;
  }

  // FIXME -- do this without the strings, just keep a map or something
  BasicBlock *getBBByName(StringRef name) {
    for (auto &bb : *liftedFn) {
      if (bb.getName() == name)
        return &bb;
    }
    assert(false && "basic block not found in getBBByName()");
  }

  Constant *getUnsignedIntConst(uint64_t val, uint64_t bits) override {
    return ConstantInt::get(Ctx, llvm::APInt(bits, val));
  }

  Constant *getAllOnesConst(uint64_t bits) {
    return ConstantInt::get(Ctx, llvm::APInt::getAllOnes(bits));
  }

  Constant *getSignedMaxConst(uint64_t bits) {
    return ConstantInt::get(Ctx, llvm::APInt::getSignedMaxValue(bits));
  }

  Constant *getSignedMinConst(uint64_t bits) {
    return ConstantInt::get(Ctx, llvm::APInt::getSignedMinValue(bits));
  }

  Constant *getAllOnesConst(Type *t) {
    int w = -1;
    if (auto *vTy = dyn_cast<VectorType>(t)) {
      auto *eTy = vTy->getElementType();
      w = getBitWidth(eTy);
    } else {
      w = getBitWidth(t);
    }
    return ConstantInt::get(t, llvm::APInt::getAllOnes(w));
  }

  Constant *getBoolConst(bool val) {
    return ConstantInt::get(Ctx, llvm::APInt(1, val ? 1 : 0));
  }

  Constant *getSignedIntConst(int64_t val, uint64_t bits) {
    return ConstantInt::get(Ctx, llvm::APInt(bits, val, /*signed=*/true));
  }

  VectorType *getVecTy(unsigned eltSize, unsigned numElts,
                       bool isFP = false) override {
    Type *eTy;
    if (isFP) {
      eTy = getFPType(eltSize);
    } else {
      eTy = getIntTy(eltSize);
    }
    auto ec = ElementCount::getFixed(numElts);
    return VectorType::get(eTy, ec);
  }

  Constant *getUndefVec(unsigned numElts, unsigned eltSize) override {
    auto eTy = getIntTy(eltSize);
    auto ec = ElementCount::getFixed(numElts);
    return ConstantVector::getSplat(ec, UndefValue::get(eTy));
  }

  Constant *getElemSplat(unsigned numElts, unsigned eltSize, uint64_t val,
                         bool isSigned = false) {
    auto ec = ElementCount::getFixed(numElts);
    return ConstantVector::getSplat(
        ec, ConstantInt::get(Ctx, APInt(eltSize, val, isSigned)));
  }

  Constant *getZeroFPVec(unsigned numElts, unsigned eltSize) {
    assert(eltSize == 32 || eltSize == 64);
    auto ec = ElementCount::getFixed(numElts);
    auto &sem = (eltSize == 64) ? APFloat::IEEEdouble() : APFloat::IEEEsingle();
    auto z = APFloat::getZero(sem);
    return ConstantVector::getSplat(ec, ConstantFP::get(Ctx, z));
  }

  Constant *getZeroIntVec(unsigned numElts, unsigned eltSize) {
    return getElemSplat(numElts, eltSize, 0);
  }

  Type *getIntTy(unsigned bits) override {
    return Type::getIntNTy(Ctx, bits);
  }

  Type *getFPType(unsigned bits) override {
    if (bits == 16)
      return Type::getHalfTy(Ctx);
    else if (bits == 32)
      return Type::getFloatTy(Ctx);
    else if (bits == 64)
      return Type::getDoubleTy(Ctx);
    else
      assert(false && "unsupported floating point type");
  }

  // Create and return a ConstantVector out of the vector of Constant vals
  Value *getVectorConst(const std::vector<Constant *> &vals) {
    return ConstantVector::get(vals);
  }

  // Takes an LLVM Type*, constructs a mask value of this type with
  // mask value = W - 1 where W is the bitwidth of the element type if a vector
  // type or the bitwidth of the type if an integer
  Value *getMaskByType(Type *llvm_ty);

  [[noreturn]] void visitError() {
    out->flush();
    std::string str(InstPrinter->getOpcodeName(CurInst->getOpcode()));
    *out << "\nERROR: Unsupported AArch64 instruction: " << str << "\n";
    out->flush();
    exit(-1);
  }

  void set_bb(llvm::BasicBlock *bb) override {
    LLVMBB = bb;
  }

  llvm::BasicBlock *get_bb() override {
    return LLVMBB;
  }

  llvm::Function &ll_function() override {
    return *liftedFn;
  }

  // lifted instructions are named using the number of the ARM
  // instruction they come from
  std::string nextName() override {
    std::stringstream ss;
    ss << "a" << armInstNum << "_" << llvmInstNum++;
    return ss.str();
  }

  AllocaInst *createAlloca(Type *ty, Value *sz,
                           const std::string &NameStr) override {
    return new AllocaInst(ty, 0, sz, NameStr, LLVMBB);
  }

  GetElementPtrInst *createGEP(Type *ty, Value *v, ArrayRef<Value *> idxlist,
                               const std::string &NameStr) override {
    return GetElementPtrInst::Create(ty, v, idxlist, NameStr, LLVMBB);
  }

  void createBranch(Value *c, BasicBlock *t, BasicBlock *f) {
    BranchInst::Create(t, f, c, LLVMBB);
  }

  void createBranch(Value *c, stmt_t t, stmt_t f) override {
    createBranch(c, t.first, f.first);
  }

  void createBranch(BasicBlock *dst) {
    BranchInst::Create(dst, LLVMBB);
  }

  void createBranch(stmt_t dst) override {
    createBranch(dst.first);
  }

  LoadInst *createLoad(Type *ty, Value *ptr) override {
    return new LoadInst(ty, ptr, nextName(), false, Align(1), LLVMBB);
  }

  void createStore(Value *v, Value *ptr) override {
    new StoreInst(v, ptr, false, Align(1), LLVMBB);
  }

  Value *createTrap() override {
    auto decl =
        Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::trap);
    return CallInst::Create(decl, "", LLVMBB);
  }

  Value *createSMin(Value *a, Value *b) override {
    auto decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::smin,
                                                  a->getType());
    return CallInst::Create(decl, {a, b}, nextName(), LLVMBB);
  }

  Value *createSMax(Value *a, Value *b) override {
    auto decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::smax,
                                                  a->getType());
    return CallInst::Create(decl, {a, b}, nextName(), LLVMBB);
  }

  Value *createUMin(Value *a, Value *b) override {
    auto decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::umin,
                                                  a->getType());
    return CallInst::Create(decl, {a, b}, nextName(), LLVMBB);
  }

  Value *createUMax(Value *a, Value *b) override {
    auto decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::umax,
                                                  a->getType());
    return CallInst::Create(decl, {a, b}, nextName(), LLVMBB);
  }

  Value *createFNeg(Value *v) override {
    return UnaryOperator::CreateFNeg(v, nextName(), LLVMBB);
  }

  Value *createFAbs(Value *v) override {
    auto fabs_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::fabs, v->getType());
    return CallInst::Create(fabs_decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createSSubOverflow(Value *a, Value *b) override {
    auto ssub_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::ssub_with_overflow, a->getType());
    return CallInst::Create(ssub_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createSAddOverflow(Value *a, Value *b) override {
    auto sadd_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::sadd_with_overflow, a->getType());
    return CallInst::Create(sadd_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createUSubOverflow(Value *a, Value *b) override {
    auto usub_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::usub_with_overflow, a->getType());
    return CallInst::Create(usub_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createUAddOverflow(Value *a, Value *b) override {
    auto uadd_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::uadd_with_overflow, a->getType());
    return CallInst::Create(uadd_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createUAddSat(Value *a, Value *b) override {
    auto uadd_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::uadd_sat, a->getType());
    return CallInst::Create(uadd_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createUSubSat(Value *a, Value *b) override {
    auto usub_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::usub_sat, a->getType());
    return CallInst::Create(usub_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createSAddSat(Value *a, Value *b) override {
    auto sadd_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::sadd_sat, a->getType());
    return CallInst::Create(sadd_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createSSubSat(Value *a, Value *b) override {
    auto ssub_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::ssub_sat, a->getType());
    return CallInst::Create(ssub_decl, {a, b}, nextName(), LLVMBB);
  }

  CallInst *createCtPop(Value *v) override {
    auto decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::ctpop, v->getType());
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  // first argument is an i16
  CallInst *createConvertFromFP16(Value *v, Type *ty) override {
    auto cvt_decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::convert_from_fp16, ty);
    return CallInst::Create(cvt_decl, {v}, nextName(), LLVMBB);
  }

  CastInst *createConvertFPToSI(Value *v, Type *ty) override {
    return new FPToSIInst(v, ty, nextName(), LLVMBB);
  }

  CastInst *createConvertFPToUI(Value *v, Type *ty) override {
    return new FPToUIInst(v, ty, nextName(), LLVMBB);
  }

  CastInst *createPtrToInt(Value *v, Type *ty) override {
    return new PtrToIntInst(v, ty, nextName(), LLVMBB);
  }

  InsertElementInst *createInsertElement(Value *vec, Value *val,
                                         int idx) override {
    auto idxv = getUnsignedIntConst(idx, 32);
    return InsertElementInst::Create(vec, val, idxv, nextName(), LLVMBB);
  }

  InsertElementInst *createInsertElement(Value *vec, Value *val,
                                         Value *idx) override {
    return InsertElementInst::Create(vec, val, idx, nextName(), LLVMBB);
  }

  ExtractElementInst *createExtractElement(Value *v, Value *idx) override {
    return ExtractElementInst::Create(v, idx, nextName(), LLVMBB);
  }

  ExtractElementInst *createExtractElement(Value *v, int idx) override {
    auto idxv = getUnsignedIntConst(idx, 32);
    return ExtractElementInst::Create(v, idxv, nextName(), LLVMBB);
  }

  ShuffleVectorInst *createShuffleVector(Value *v,
                                         ArrayRef<int> mask) override {
    return new ShuffleVectorInst(v, mask, nextName(), LLVMBB);
  }

  ShuffleVectorInst *createShuffleVector(Value *v, Value *mask) {
    return new ShuffleVectorInst(v, mask, nextName(), LLVMBB);
  }

  ShuffleVectorInst *createShuffleVector(Value *v, Value *x,
                                         ArrayRef<int> mask) override {
    return new ShuffleVectorInst(v, x, mask, nextName(), LLVMBB);
  }

  ShuffleVectorInst *createShuffleVector(Value *v1, Value *v2, Value *mask) {
    return new ShuffleVectorInst(v1, v2, mask, nextName(), LLVMBB);
  }

  ExtractValueInst *createExtractValue(Value *v,
                                       ArrayRef<unsigned> idxs) override {
    return ExtractValueInst::Create(v, idxs, nextName(), LLVMBB);
  }

  ReturnInst *createReturn(Value *v) override {
    return ReturnInst::Create(Ctx, v, LLVMBB);
  }

  CallInst *createFShr(Value *a, Value *b, Value *c) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::fshr, a->getType());
    return CallInst::Create(decl, {a, b, c}, nextName(), LLVMBB);
  }

  CallInst *createFShl(Value *a, Value *b, Value *c) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::fshl, a->getType());
    return CallInst::Create(decl, {a, b, c}, nextName(), LLVMBB);
  }

  CallInst *createBitReverse(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::bitreverse, v->getType());
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createAbs(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::abs,
                                                   v->getType());
    return CallInst::Create(decl, {v, getBoolConst(false)}, nextName(), LLVMBB);
  }

  CallInst *createCtlz(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::ctlz, v->getType());
    return CallInst::Create(decl, {v, getBoolConst(false)}, nextName(), LLVMBB);
  }

  CallInst *createBSwap(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::bswap, v->getType());
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createVectorReduceAdd(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::vector_reduce_add, v->getType());
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createFusedMultiplyAdd(Value *a, Value *b, Value *c) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(LiftedModule, Intrinsic::fma,
                                                   a->getType());
    return CallInst::Create(decl, {a, b, c}, nextName(), LLVMBB);
  }

  CallInst *createSQRT(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::sqrt, v->getType());
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createRound(Value *v) override {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::rint, v->getType());
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createConstrainedRound(Value *v, Metadata *md) {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::experimental_constrained_round, v->getType());
    return CallInst::Create(decl, {v, MetadataAsValue::get(Ctx, md)},
                            nextName(), LLVMBB);
  }

  CallInst *createConstrainedFloor(Value *v, Metadata *md) {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::experimental_constrained_floor, v->getType());
    return CallInst::Create(decl, {v, MetadataAsValue::get(Ctx, md)},
                            nextName(), LLVMBB);
  }

  CallInst *createConstrainedCeil(Value *v, Metadata *md) {
    auto *decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::experimental_constrained_ceil, v->getType());
    return CallInst::Create(decl, {v, MetadataAsValue::get(Ctx, md)},
                            nextName(), LLVMBB);
  }

  CallInst *createConstrainedRound(Value *v) override {
    return createConstrainedRound(v, MDString::get(Ctx, "fpexcept.strict"));
  }

  CallInst *createConstrainedFloor(Value *v) override {
    return createConstrainedFloor(v, MDString::get(Ctx, "fpexcept.strict"));
  }

  CallInst *createConstrainedCeil(Value *v) override {
    return createConstrainedCeil(v, MDString::get(Ctx, "fpexcept.strict"));
  }

  SelectInst *createSelect(Value *cond, Value *a, Value *b) override {
    return SelectInst::Create(cond, a, b, nextName(), LLVMBB);
  }

  ICmpInst *createICmp(ICmpInst::Predicate p, Value *a, Value *b) override {
    return new ICmpInst(LLVMBB, p, a, b, nextName());
  }

  FCmpInst *createFCmp(FCmpInst::Predicate p, Value *a, Value *b) override {
    return new FCmpInst(LLVMBB, p, a, b, nextName());
  }

  BinaryOperator *createBinop(Value *a, Value *b,
                              Instruction::BinaryOps op) override {
    return BinaryOperator::Create(op, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createUDiv(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::UDiv, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createSDiv(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::SDiv, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createMul(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::Mul, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createAdd(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::Add, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createFAdd(Value *a, Value *b) {
    return BinaryOperator::Create(Instruction::FAdd, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createSub(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::Sub, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createFSub(Value *a, Value *b) {
    return BinaryOperator::Create(Instruction::FSub, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createFMul(Value *a, Value *b) {
    return BinaryOperator::Create(Instruction::FMul, a, b, nextName(), LLVMBB);
  }

  Value *createRawLShr(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::LShr, a, b, nextName(), LLVMBB);
  }

  Value *createMaskedLShr(Value *a, Value *b) override {
    assert(a->getType() == b->getType() && "Expected values of same type");

    // Get an LLVM mask for b to get shift value less than bit width of a
    // In LLVM shift >= bitwidth -> poison
    auto mask = getMaskByType(a->getType());
    assert(a->getType() == mask->getType() && "Expected values of same type");

    auto masked =
        BinaryOperator::Create(Instruction::And, mask, b, nextName(), LLVMBB);
    return BinaryOperator::Create(Instruction::LShr, a, masked, nextName(),
                                  LLVMBB);
  }

  Value *createRawAShr(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::AShr, a, b, nextName(), LLVMBB);
  }

  Value *createMaskedAShr(Value *a, Value *b) override {
    assert(a->getType() == b->getType() && "Expected values of same type");

    // Get an LLVM mask for b to get shift value less than bit width of a
    // In LLVM shift >= bitwidth -> poison
    auto mask = getMaskByType(a->getType());
    assert(a->getType() == mask->getType() && "Expected values of same type");

    auto masked =
        BinaryOperator::Create(Instruction::And, mask, b, nextName(), LLVMBB);
    return BinaryOperator::Create(Instruction::AShr, a, masked, nextName(),
                                  LLVMBB);
  }

  Value *createRawShl(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::Shl, a, b, nextName(), LLVMBB);
  }

  Value *createMaskedShl(Value *a, Value *b) override {
    assert(a->getType() == b->getType() && "Expected values of same type");

    // Get an LLVM mask for b to get shift value less than bit width of a
    // In LLVM shift >= bitwidth -> poison
    auto mask = getMaskByType(a->getType());
    assert(a->getType() == mask->getType() && "Expected values of same type");

    auto masked =
        BinaryOperator::Create(Instruction::And, mask, b, nextName(), LLVMBB);
    return BinaryOperator::Create(Instruction::Shl, a, masked, nextName(),
                                  LLVMBB);
  }

  Value *getLowOnes(int ones, int w) override {
    auto zero = getUnsignedIntConst(0, ones);
    auto one = getUnsignedIntConst(1, ones);
    auto minusOne = createSub(zero, one);
    return createZExt(minusOne, getIntTy(w));
  }

  Value *createMSL(Value *a, int b) override {
    auto v = BinaryOperator::Create(Instruction::Shl, a,
                                    getUnsignedIntConst(b, getBitWidth(a)),
                                    nextName(), LLVMBB);
    auto ones = getLowOnes(b, getBitWidth(a));
    return createOr(v, ones);
  }

  BinaryOperator *createAnd(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::And, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createOr(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::Or, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createXor(Value *a, Value *b) override {
    return BinaryOperator::Create(Instruction::Xor, a, b, nextName(), LLVMBB);
  }

  BinaryOperator *createNot(Value *a) override {
    auto NegOne = getAllOnesConst(a->getType());
    return BinaryOperator::Create(Instruction::Xor, a, NegOne, nextName(),
                                  LLVMBB);
  }

  FreezeInst *createFreeze(Value *v) override {
    return new FreezeInst(v, nextName(), LLVMBB);
  }

  Value *createTrunc(Value *v, Type *t) override {
    if (v->getType() == t)
      return v;
    return CastInst::Create(Instruction::Trunc, v, t, nextName(), LLVMBB);
  }

  CastInst *createSExt(Value *v, Type *t) override {
    return CastInst::Create(Instruction::SExt, v, t, nextName(), LLVMBB);
  }

  CastInst *createZExt(Value *v, Type *t) override {
    return CastInst::Create(Instruction::ZExt, v, t, nextName(), LLVMBB);
  }

  CallInst *createFPToUI_sat(Value *v, Type *t) override {
    auto decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::fptoui_sat, {t, v->getType()});
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CallInst *createFPToSI_sat(Value *v, Type *t) override {
    auto decl = Intrinsic::getOrInsertDeclaration(
        LiftedModule, Intrinsic::fptosi_sat, {t, v->getType()});
    return CallInst::Create(decl, {v}, nextName(), LLVMBB);
  }

  CastInst *createUIToFP(Value *v, Type *t) override {
    return CastInst::Create(Instruction::UIToFP, v, t, nextName(), LLVMBB);
  }

  CastInst *createSIToFP(Value *v, Type *t) override {
    return CastInst::Create(Instruction::SIToFP, v, t, nextName(), LLVMBB);
  }

  CastInst *createFPTrunc(Value *v, Type *t) override {
    return CastInst::Create(Instruction::FPTrunc, v, t, nextName(), LLVMBB);
  }

  CastInst *createFPExt(Value *v, Type *t) override {
    return CastInst::Create(Instruction::FPExt, v, t, nextName(), LLVMBB);
  }

  CastInst *createBitCast(Value *v, Type *t) override {
    return CastInst::Create(Instruction::BitCast, v, t, nextName(), LLVMBB);
  }

  CastInst *createCast(Value *v, Type *t, Instruction::CastOps op) override {
    return CastInst::Create(op, v, t, nextName(), LLVMBB);
  }

  Value *splat(Value *v, unsigned numElts, unsigned eltSize) {
    assert(getBitWidth(v) == eltSize);
    Value *res = getUndefVec(numElts, eltSize);
    for (unsigned i = 0; i < numElts; ++i)
      res = createInsertElement(res, v, i);
    return res;
  }

  enum class extKind { SExt, ZExt, None };

  Value *addPairs(Value *src, unsigned eltSize, unsigned numElts, extKind ext) {
    auto bigEltTy = getIntTy(2 * eltSize);
    Value *res = getUndefVec(numElts / 2, 2 * eltSize);
    for (unsigned i = 0; i < numElts; i += 2) {
      auto elt1 = createExtractElement(src, i);
      auto elt2 = createExtractElement(src, i + 1);
      auto ext1 = ext == extKind::SExt ? createSExt(elt1, bigEltTy)
                                       : createZExt(elt1, bigEltTy);
      auto ext2 = ext == extKind::SExt ? createSExt(elt2, bigEltTy)
                                       : createZExt(elt2, bigEltTy);
      auto sum = createAdd(ext1, ext2);
      res = createInsertElement(res, sum, i / 2);
    }
    return res;
  }

  unsigned getBitWidth(Type *ty) {
    if (auto vTy = dyn_cast<VectorType>(ty)) {
      return vTy->getScalarSizeInBits() *
             vTy->getElementCount().getFixedValue();
    } else if (ty->isIntegerTy()) {
      return ty->getIntegerBitWidth();
    } else if (ty->isHalfTy()) {
      return 16;
    } else if (ty->isFloatTy()) {
      return 32;
    } else if (ty->isDoubleTy()) {
      return 64;
    } else if (ty->isPointerTy()) {
      return 64;
    } else {
      ty->dump();
      assert(false && "Unhandled type");
    }
  }

  unsigned getBitWidth(Value *V) {
    return getBitWidth(V->getType());
  }

  std::tuple<std::string, long> getOffset(const std::string &var);
  // Reads an Expr and maps containing string variable to a global variable
  std::string mapExprVar(const MCExpr *expr);
  std::string demangle(const std::string &name);
  // Reads an Expr and gets the global variable corresponding the containing
  // string variable. Assuming the Expr consists of a single global variable.
  std::pair<Value *, bool> getExprVar(const MCExpr *expr);
  // negative shift exponents go the other direction
  Value *createUSHL(Value *a, Value *b);
  // negative shift exponents go the other direction
  Value *createSSHL(Value *a, Value *b);
  Value *rev(Value *in, unsigned eltSize, unsigned amt);
  Value *dupElts(Value *v, unsigned numElts, unsigned eltSize);
  Value *concat(Value *a, Value *b);
  void assertSame(Value *a, Value *b);
  void doDirectCall();
  Instruction *getCurLLVMInst();
  std::optional<aslp::opcode_t> getArmOpcode(const MCInst &I);
  void liftInst(MCInst &I);
  void invalidateReg(unsigned Reg, unsigned Width);
  void createRegStorage(unsigned Reg, unsigned Width, const std::string &Name);
  Function *run();

  /*
   * shared with the aslp lifter
   */
  llvm::Value *lookupExprVar(const llvm::MCExpr &expr) override {
    return lookupGlobal(mapExprVar(&expr));
  }
  void assertTrue(Value *cond) override;
  void storeToMemoryValOffset(Value *base, Value *offset, uint64_t size,
                              Value *val) override;

  /*
   * per-backend functionality goes here
   */
  virtual void doCall(FunctionCallee FC, CallInst *llvmCI,
                      const std::string &calleeName) = 0;
  virtual void lift(MCInst &I) = 0;
  virtual Value *enforceSExtZExt(Value *V, bool isSExt, bool isZExt) = 0;
  virtual Value *createRegFileAndStack() = 0;
  virtual void doReturn() = 0;
};

} // end namespace lifter
