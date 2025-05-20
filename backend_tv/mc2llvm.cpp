#include "backend_tv/bitutils.h"
#include "backend_tv/lifter.h"
#include "backend_tv/mc2llvm.h"
#include "backend_tv/mcutils.h"

#include <regex>

using namespace std;
using namespace llvm;
using namespace lifter;

Function *mc2llvm::copyFunctionToTarget(Function *f, const Twine &name) {
  auto newF = Function::Create(f->getFunctionType(),
                               GlobalValue::LinkageTypes::ExternalLinkage, name,
                               LiftedModule);
  if (f->hasFnAttribute(Attribute::NoReturn))
    newF->addFnAttr(Attribute::NoReturn);
  if (f->hasFnAttribute(Attribute::WillReturn))
    newF->addFnAttr(Attribute::WillReturn);
  if (f->hasRetAttribute(Attribute::NoAlias))
    newF->addRetAttr(Attribute::NoAlias);
  if (f->hasRetAttribute(Attribute::SExt))
    newF->addRetAttr(Attribute::SExt);
  if (f->hasRetAttribute(Attribute::ZExt))
    newF->addRetAttr(Attribute::ZExt);
  auto attr = f->getFnAttribute(Attribute::Memory);
  if (attr.hasAttribute(Attribute::Memory))
    newF->addFnAttr(attr);
  return newF;
}

Constant *mc2llvm::lazyAddGlobal(string newGlobal) {
  *out << "  lazyAddGlobal '" << newGlobal << "'\n";

  {
    auto got = intrinsic_names.find(newGlobal);
    if (got != intrinsic_names.end())
      newGlobal = got->second;
  }

  if (newGlobal == "__stack_chk_fail") {
    return new GlobalVariable(*LiftedModule, getIntTy(8), false,
                              GlobalValue::LinkageTypes::ExternalLinkage,
                              nullptr, "__stack_chk_fail");
  }

  // is the global the address of a function?
  if (newGlobal == liftedFn->getName()) {
    // pointer to the function we're lifting into is a special case
    *out << "  yay it's me!\n";
    return liftedFn;
  }
  for (auto &f : *srcFn.getParent()) {
    auto name = f.getName();
    if (name != newGlobal)
      continue;
    *out << "  creating function '" << newGlobal << "'\n";
    return copyFunctionToTarget(&f, newGlobal);
  }

  // globals that are definitions in the assembly can be lifted
  // directly. here we have to deal with a mix of literal and
  // symbolic data, and also deal with globals whose initializers
  // reference each other. we support the latter by creating an
  // initializer-free dummy variable right now, and then later
  // creating the actual thing we need
  for (const auto &g : MF.MCglobals) {
    string name{demangle(g.name)};
    if (name != newGlobal)
      continue;
    *out << "creating lifted global " << name << " from the assembly\n";
    auto size = g.data.size();
    *out << "  section = " << g.section << "\n";

    vector<Type *> tys;
    vector<Constant *> vals;
    for (unsigned i = 0; i < size; ++i) {
      auto &data = g.data[i];
      if (holds_alternative<char>(data)) {
        tys.push_back(getIntTy(8));
        vals.push_back(ConstantInt::get(getIntTy(8), get<char>(data)));
      } else if (holds_alternative<OffsetSym>(data)) {
        auto s = get<OffsetSym>(data);
        *out << "  it's a symbol named " << s.sym << "\n";
        auto res = LLVMglobals.find(s.sym);
        Constant *con;
        if (res == LLVMglobals.end()) {
          // ok, this global hasn't been lifted yet. but it might
          // already be in the deferred queue, in which case we can
          // use its dummy version.
          bool found = false;
          for (auto [name, _con] : deferredGlobs) {
            if (name == s.sym) {
              found = true;
              con = _con;
              break;
            }
          }
          if (!found) {
            *out << "  breaking recursive loop by deferring " << s.sym << "\n";
            auto *dummy =
                new GlobalVariable(*LiftedModule, getIntTy(8), false,
                                   GlobalValue::LinkageTypes::ExternalLinkage,
                                   nullptr, s.sym + "_tmp");
            deferredGlobs.push_back({.name = demangle(s.sym), .val = dummy});
            con = dummy;
          }
        } else {
          *out << "  found it using regular lookup\n";
          con = res->second;
        }
        Constant *offset = getUnsignedIntConst(s.offset, 64);
        Constant *ptr =
            ConstantExpr::getGetElementPtr(getIntTy(8), con, offset);
        tys.push_back(PointerType::get(Ctx, 0));
        vals.push_back(ptr);
      } else {
        assert(false);
      }
    }
    auto *ty = StructType::create(tys);
    auto initializer = ConstantStruct::get(ty, vals);
    bool isConstant = g.section.starts_with(".rodata") || g.section == ".text";
    auto *glob = new GlobalVariable(*LiftedModule, ty, isConstant,
                                    GlobalValue::LinkageTypes::ExternalLinkage,
                                    initializer, name);
    glob->setAlignment(g.align);
    return glob;
  }

  // globals that are only declarations have to be registered in
  // LLVM IR, but they don't show up anywhere in the assembly -- the
  // declaration is implicit. so here we find those and create them
  // in the lifted module
  for (auto &srcFnGlobal : srcFn.getParent()->globals()) {
    if (!srcFnGlobal.isDeclaration())
      continue;
    string name{srcFnGlobal.getName()};
    if (name != newGlobal)
      continue;
    *out << "creating declaration for global variable " << name << "\n";
    *out << "  linkage = " << srcFnGlobal.getLinkage() << "\n";
    auto *glob = new GlobalVariable(*LiftedModule, srcFnGlobal.getValueType(),
                                    srcFnGlobal.isConstant(),
                                    GlobalValue::LinkageTypes::ExternalLinkage,
                                    /*initializer=*/nullptr, name);
    glob->setAlignment(srcFnGlobal.getAlign());
    return glob;
  }

  {
    /*
     * sometimes an intrinsic appears in tgt, but not src. there's
     * no principled way to deal with these, we'll special case them
     * here
     */
    auto got = implicit_intrinsics.find(newGlobal);
    if (got != implicit_intrinsics.end()) {
      auto newF = Function::Create(got->second,
                                   GlobalValue::LinkageTypes::ExternalLinkage,
                                   newGlobal, LiftedModule);
      return newF;
    }
  }

  *out << "ERROR: global symbol '" << newGlobal << "' not found\n";
  exit(-1);
}

Constant *mc2llvm::lookupGlobal(const string &nm) {
  auto name = demangle(nm);
  *out << "lookupGlobal '" << name << "'\n";

  auto glob = LLVMglobals.find(name);
  if (glob != LLVMglobals.end())
    return glob->second;

  auto *g = lazyAddGlobal(name);
  assert(g);
  LLVMglobals[name] = g;

  // lifting this one may have necessitated lifting other variables,
  // which we deferred, and created placeholders instead. fill those
  // in now. this may keep cascading for a while, no problem! our
  // overall goal here is to end up lifting the transitive closure
  // of stuff reachable from the function we're lifting, and nothing
  // else.
  while (!deferredGlobs.empty()) {
    *out << "  processing a deferred global\n";
    auto def = deferredGlobs.at(0);
    deferredGlobs.erase(deferredGlobs.begin());
    auto g2 = lazyAddGlobal(def.name);
    def.val->replaceAllUsesWith(g2);
    def.val->eraseFromParent();
    LLVMglobals[def.name] = g2;
  }

  return g;
}

Value *mc2llvm::getMaskByType(Type *llvm_ty) {
  assert((llvm_ty->isIntegerTy() || llvm_ty->isVectorTy()) &&
         "getMaskByType only handles integer or vector type right now\n");
  Value *mask_value;

  if (llvm_ty->isIntegerTy()) {
    auto W = llvm_ty->getIntegerBitWidth();
    mask_value = getUnsignedIntConst(W - 1, W);
  } else if (llvm_ty->isVectorTy()) {
    VectorType *shift_value_type = ((VectorType *)llvm_ty);
    auto W_element = shift_value_type->getScalarSizeInBits();
    auto numElements = shift_value_type->getElementCount().getFixedValue();
    vector<Constant *> widths;

    // Push numElements x (W_element-1)'s to the vector widths
    for (unsigned i = 0; i < numElements; i++) {
      widths.push_back(
          ConstantInt::get(Ctx, llvm::APInt(W_element, W_element - 1)));
    }

    // Get a ConstantVector of the widths
    mask_value = getVectorConst(widths);
  } else {
    *out << "ERROR: getMaskByType encountered unhandled/unknown type\n";
    exit(-1);
  }

  return mask_value;
}

std::tuple<string, long> mc2llvm::getOffset(const string &var) {
  std::smatch m;
  std::regex offset("^(.*)\\+([0-9]+)$");
  if (std::regex_search(var, m, offset)) {
    auto root = m[1];
    auto offset = m[2];
    *out << "  root = " << root << "\n";
    *out << "  offset = " << offset << "\n";
    return make_tuple(root, stol(offset));
  } else {
    return make_tuple(var, 0);
  }
}

string mc2llvm::demangle(const string &name) {
  if (name.rfind(".L.", 0) == 0) {
    // the assembler has mangled local symbols, which start with a
    // dot, by prefixing them with ".L"; here we demangle
    return name.substr(2);
  } else {
    return name;
  }
}

std::string mc2llvm::mapExprVar(const MCExpr *expr) {
  std::string name;
  llvm::raw_string_ostream ss(name);
  expr->print(ss, nullptr);

  // If the expression starts with a relocation specifier, strip it and map
  // the rest to a string name of the global variable. Assuming there is only
  // one relocation specifier, and it is at the beginning
  // (std::regex_constants::match_continuous).
  // eg: ":lo12:a" becomes  "a"
  std::smatch sm1;
  std::regex reloc("^:[a-z0-9_]+:");
  if (std::regex_search(name, sm1, reloc)) {
    name = sm1.suffix();
  }

  name = demangle(name);
  auto [root, offset] = getOffset(name);
  name = root;

  if (!lookupGlobal(name)) {
    *out << "\ncan't find global '" << name << "'\n";
    *out << "ERROR: Unknown global in ADRP\n\n";
    exit(-1);
  }

  instExprVarMap[CurInst] = name;
  return name;
}

pair<Value *, bool> mc2llvm::getExprVar(const MCExpr *expr) {
  Value *globalVar;
  // Default to true meaning store the ptr global value rather than loading
  // the value from the global
  bool storePtr = true;
  std::string sss;

  // Matched strings
  std::smatch sm;

  // Regex to match relocation specifiers
  std::regex re(":[a-z0-9_]+:");

  llvm::raw_string_ostream ss(sss);
  expr->print(ss, nullptr);

  auto [root, offset] = getOffset(sss);
  sss = root;

  // If the expression starts with a relocation specifier, strip it and look
  // for the rest (variable in the Expr) in the instExprVarMap and globals.
  // Assuming there is only one relocation specifier, and it is at the
  // beginning (std::regex_constants::match_continuous).
  if (std::regex_search(sss, sm, re, std::regex_constants::match_continuous)) {
    string stringVar = sm.suffix();
    // Check the relocation specifiers to determine whether to store ptr
    // global value in the register or load the value from the global
    if (!sm.empty() && (sm[0] == ":lo12:")) {
      storePtr = false;
    }

    stringVar = demangle(stringVar);

    if (!lookupGlobal(stringVar)) {
      *out << "\nERROR: instruction mentions '" << stringVar << "'\n";
      *out << "which is not a global variable we know about\n\n";
      exit(-1);
    }

    // Look through all visited ADRP instructions to find one in which
    // stringVar was the operand used.
    bool foundStringVar = false;
    for (const auto &exprVar : instExprVarMap) {
      if (exprVar.second == stringVar) {
        foundStringVar = true;
        break;
      }
    }

    if (!foundStringVar) {
      *out << "\nERROR: Did not use \"" << stringVar
           << "\" in an ADRP "
              "instruction\n\n";
      exit(-1);
    }

    globalVar = lookupGlobal(stringVar);
    if (!globalVar) {
      *out << "\nERROR: global not found\n\n";
      exit(-1);
    }
  } else {
    globalVar = lookupGlobal(demangle(sss));
    if (!globalVar) {
      *out << "\nERROR: global not found\n\n";
      exit(-1);
    }
  }

  assert(globalVar);
  if (offset != 0) {
    // FIXME -- would be better to return the root symbol and the
    // offset separately, and let the caller do the pointer
    // arithmetic
    globalVar = createGEP(getIntTy(8), globalVar,
                          {getUnsignedIntConst(offset, 64)}, "");
  }
  return make_pair(globalVar, storePtr);
}

Value *mc2llvm::createUSHL(Value *a, Value *b) {
  auto zero = getUnsignedIntConst(0, getBitWidth(b));
  auto c = createICmp(ICmpInst::Predicate::ICMP_SGT, b, zero);
  auto neg = createSub(zero, b);
  auto posRes = createMaskedShl(a, b);
  auto negRes = createMaskedLShr(a, neg);
  return createSelect(c, posRes, negRes);
}

Value *mc2llvm::createSSHL(Value *a, Value *b) {
  auto zero = getUnsignedIntConst(0, getBitWidth(b));
  auto c = createICmp(ICmpInst::Predicate::ICMP_SGT, b, zero);
  auto neg = createSub(zero, b);
  auto posRes = createMaskedShl(a, b);
  auto negRes = createMaskedAShr(a, neg);
  return createSelect(c, posRes, negRes);
}

Value *mc2llvm::rev(Value *in, unsigned eltSize, unsigned amt) {
  assert(eltSize == 8 || eltSize == 16 || eltSize == 32);
  assert(getBitWidth(in) == 64 || getBitWidth(in) == 128);
  if (getBitWidth(in) == 64)
    in = createZExt(in, getIntTy(128));
  Value *rev = getUndefVec(128 / eltSize, eltSize);
  in = createBitCast(in, getVecTy(eltSize, 128 / eltSize));
  for (unsigned i = 0; i < (128 / amt); ++i) {
    auto innerCount = amt / eltSize;
    for (unsigned j = 0; j < innerCount; j++) {
      auto elt = createExtractElement(in, (i * innerCount) + j);
      rev =
          createInsertElement(rev, elt, (i * innerCount) + innerCount - j - 1);
    }
  }
  return rev;
}

Value *mc2llvm::dupElts(Value *v, unsigned numElts, unsigned eltSize) {
  unsigned w = numElts * eltSize;
  assert(w == 64 || w == 128);
  assert(getBitWidth(v) == eltSize);
  Value *res = getUndefVec(numElts, eltSize);
  for (unsigned i = 0; i < numElts; i++)
    res = createInsertElement(res, v, i);
  return res;
}

Value *mc2llvm::concat(Value *a, Value *b) {
  int wa = getBitWidth(a);
  int wb = getBitWidth(b);
  auto wide_a = createZExt(a, getIntTy(wa + wb));
  auto wide_b = createZExt(b, getIntTy(wa + wb));
  auto shifted_a = createRawShl(wide_a, getUnsignedIntConst(wb, wa + wb));
  return createOr(shifted_a, wide_b);
}

void mc2llvm::assertTrue(Value *cond) {
  assert(cond->getType()->getIntegerBitWidth() == 1 && "assert requires i1");
  CallInst::Create(assertDecl, {cond}, "", LLVMBB);
}

void mc2llvm::assertSame(Value *a, Value *b) {
  auto *c = createICmp(ICmpInst::Predicate::ICMP_EQ, a, b);
  assertTrue(c);
}

void mc2llvm::doDirectCall() {
  auto &op0 = CurInst->getOperand(0);
  assert(op0.isExpr());
  auto [expr, _] = getExprVar(op0.getExpr());
  assert(expr);
  string calleeName = (string)expr->getName();

  if (calleeName == "__stack_chk_fail") {
    createTrap();
    return;
  }

  *out << "lifting a direct call, callee is: '" << calleeName << "'\n";
  auto *callee = dyn_cast<Function>(expr);
  assert(callee);

  if (false && callee == liftedFn) {
    *out << "Recursion currently not supported\n\n";
    exit(-1);
  }

  auto llvmInst = getCurLLVMInst();
  CallInst *llvmCI{nullptr};
  if (!llvmInst) {
    *out << "oops, no debuginfo mapping exists\n";
  } else {
    llvmCI = dyn_cast<CallInst>(llvmInst);
    if (!llvmCI)
      *out << "oops, debuginfo gave us something that's not a callinst\n";
  }
  if (!llvmCI) {
    *out << "error: can't locate corresponding source-side call instruction\n";
    exit(-1);
  }

  FunctionCallee FC{callee};
  doCall(FC, llvmCI, calleeName);
}

Instruction *mc2llvm::getCurLLVMInst() {
  return (Instruction *)(CurInst->getLoc().getPointer());
}

void mc2llvm::storeToMemoryValOffset(Value *base, Value *offset, uint64_t size,
                                     Value *val) {
  // Create a GEP instruction based on a byte addressing basis (8 bits)
  // returning pointer to base + offset
  assert(base);
  auto ptr = createGEP(getIntTy(8), base, {offset}, "");

  // Store Value val in the pointer returned by the GEP instruction
  createStore(val, ptr);
}

std::optional<aslp::opcode_t> mc2llvm::getArmOpcode(const MCInst &I) {
  SmallVector<MCFixup> Fixups{};
  SmallVector<char> Code{};

  if (I.getOpcode() == AArch64::SEH_Nop)
    return std::nullopt;

  MCE.encodeInstruction(I, Code, Fixups, STI);
  for (auto x : Fixups) {
    // std::cerr << "fixup: " << x.getKind() << ' ' << x.getTargetKind() << '
    // ' << x.getOffset() << ' ' << std::flush; x.getValue()->dump();
    // std::cout << std::endl;
    (void)x;
  }

  // do not hand any instructions with relocation fixups to aslp
  if (Fixups.size() != 0)
    return std::nullopt;

  aslp::opcode_t ret;
  unsigned i = 0;
  for (const char &x : Code) {
    ret.at(i++) = x;
  }
  return ret;
}

void mc2llvm::liftInst(MCInst &I) {
  *out << "mcinst: " << I.getOpcode() << " = ";
  PrevInst = CurInst;
  CurInst = &I;

  std::string sss;
  llvm::raw_string_ostream ss{sss};
  I.dump_pretty(ss, InstPrinter);
  *out << sss << " = " << std::flush;
  if (I.getOpcode() != AArch64::SEH_Nop) {
    InstPrinter->printInst(&I, 100, "", STI, outs());
    outs().flush();
  }
  *out << std::endl;

  lift(I);
}

void mc2llvm::invalidateReg(unsigned Reg, unsigned Width) {
  auto F = createFreeze(PoisonValue::get(getIntTy(Width)));
  createStore(F, RegFile[Reg]);
}

// create the actual storage associated with a register -- all of its
// asm-level aliases will get redirected here
void mc2llvm::createRegStorage(unsigned Reg, unsigned Width,
                               const string &Name) {
  auto A = createAlloca(getIntTy(Width), getUnsignedIntConst(1, 64), Name);
  auto F = createFreeze(PoisonValue::get(getIntTy(Width)));
  createStore(F, A);
  RegFile[Reg] = A;
}

Function *mc2llvm::run() {
  // we'll want this later
  vector<Type *> args{getIntTy(1)};
  FunctionType *assertTy = FunctionType::get(Type::getVoidTy(Ctx), args, false);
  assertDecl = Function::Create(assertTy, Function::ExternalLinkage,
                                "llvm.assert", LiftedModule);
  auto i64 = getIntTy(64);

  // create a fresh function
  liftedFn =
      Function::Create(srcFn.getFunctionType(), GlobalValue::ExternalLinkage, 0,
                       srcFn.getName(), LiftedModule);
  liftedFn->copyAttributesFrom(&srcFn);

  // create LLVM-side basic blocks
  vector<pair<BasicBlock *, MCBasicBlock *>> BBs;
  {
    long insts = 0;
    for (auto &mbb : MF.BBs) {
      for (auto &inst [[maybe_unused]] : mbb.getInstrs())
        ++insts;
      auto bb = BasicBlock::Create(Ctx, mbb.getName(), liftedFn);
      BBs.push_back(make_pair(bb, &mbb));
    }
    *out << insts << " assembly instructions\n";
    out->flush();
  }

  // default to adding instructions to the entry block
  LLVMBB = BBs[0].first;

  // number of 8-byte stack slots for parameters
  const int numStackSlots = 32;

  auto *allocTy =
      FunctionType::get(PointerType::get(Ctx, 0), {i64, i64}, false);
  myAlloc = Function::Create(allocTy, GlobalValue::ExternalLinkage, 0,
                             "myalloc", LiftedModule);
  myAlloc->addRetAttr(Attribute::NonNull);
  AttrBuilder B1(Ctx);
  B1.addAllocKindAttr(AllocFnKind::Alloc);
  B1.addAllocSizeAttr(0, {});
  B1.addAttribute(Attribute::WillReturn);
  myAlloc->addFnAttrs(B1);
  myAlloc->addParamAttr(1, Attribute::AllocAlign);
  myAlloc->addFnAttr("alloc-family", "backend-tv-alloc");
  stackSize = getUnsignedIntConst(stackBytes + (8 * numStackSlots), 64);

  stackMem = CallInst::Create(myAlloc, {stackSize, getUnsignedIntConst(16, 64)},
                              "stack", LLVMBB);

  auto paramBase = createRegFileAndStack();

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
      auto Reg = AArch64::X0 + scalarArgNum;
      createStore(val, RegFile[Reg]);
      ++scalarArgNum;
      goto end;
    }

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

  end:
    *out << "\n";
  }

  *out << "done with callee-side ABI stuff\n";

  // initialize the frame pointer
  auto initFP =
      createGEP(i64, paramBase, {getUnsignedIntConst(stackSlot, 64)}, "");
  createStore(initFP, RegFile[AArch64::FP]);

  *out << "\n\nlifting assembly instructions to LLVM\n";

  for (auto &[llvm_bb, mc_bb] : BBs) {
    LLVMBB = llvm_bb;
    MCBB = mc_bb;
    auto &mc_instrs = mc_bb->getInstrs();

    for (auto &inst : mc_instrs) {
      llvmInstNum = 0;
      *out << armInstNum << " : about to lift "
           << (string)InstPrinter->getOpcodeName(inst.getOpcode()) << "\n";
      liftInst(inst);
      *out << "    lifted\n";
      ++armInstNum;
    }

    // machine code falls through but LLVM isn't allowed to
    if (!LLVMBB->getTerminator()) {
      auto succs = MCBB->getSuccs().size();
      if (succs == 0) {
        // this should only happen when we have a function with a
        // single, empty basic block, which should only when we
        // started with an LLVM function whose body is something
        // like UNREACHABLE
        doReturn();
      } else if (succs == 1) {
        auto *dst = getBBByName(MCBB->getSuccs()[0]->getName());
        createBranch(dst);
      }
    }
  }
  *out << armInstNum << " assembly instructions\n";

  *out << "encoding counts: ";
  for (auto &[enc, count] : encodingCounts) {
    *out << enc << '=' << count << ',';
  }
  *out << '\n';
  return liftedFn;
}

