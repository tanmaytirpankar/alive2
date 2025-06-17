#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
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
#include "backend_tv/lifter.h"
#include "backend_tv/mc2llvm.h"
#include "backend_tv/riscv2llvm.h"
#include "backend_tv/streamerwrapper.h"

using namespace std;
using namespace llvm;
using namespace lifter;

// do not delete this line
mc::RegisterMCTargetOptionsFlags MOF;

namespace lifter {

// FIXME get rid of these globals
std::ostream *out;
unsigned origRetWidth;
bool has_ret_attr;
const Target *Targ;
Function *myAlloc;
Constant *stackSize;
std::string DefaultBackend;
llvm::Triple DefaultTT;
const char *DefaultDL;
const char *DefaultCPU;
const char *DefaultFeatures;

void addDebugInfo(Function *srcFn,
                  unordered_map<unsigned, Instruction *> &lineMap) {
  auto &M = *srcFn->getParent();

  // start with a clean slate
  StripDebugInfo(M);

  M.setModuleFlag(Module::Warning, "Dwarf Version", dwarf::DWARF_VERSION);
  M.setModuleFlag(Module::Warning, "Debug Info Version",
                  DEBUG_METADATA_VERSION);

  auto &Ctx = srcFn->getContext();

  DIBuilder DBuilder(M);
  auto DIF = DBuilder.createFile("foo.ll", ".");
  auto CU = DBuilder.createCompileUnit(dwarf::DW_LANG_C, DIF, "backend-tv",
                                       false, "", 0);
  auto Ty = DBuilder.createSubroutineType(DBuilder.getOrCreateTypeArray({}));
  auto SP = DBuilder.createFunction(CU, srcFn->getName(), StringRef(), DIF, 0,
                                    Ty, 0, DINode::FlagPrototyped,
                                    DISubprogram::SPFlagDefinition);
  srcFn->setSubprogram(SP);
  unsigned line = 0;
  for (auto &bb : *srcFn) {
    for (auto &i : bb) {
      lineMap[line] = &i;
      i.setDebugLoc(DILocation::get(Ctx, line, 0, SP));
      ++line;
    }
  }

  if (false) {
    M.dump();
    *out << "\n\n\n";
  }

  DBuilder.finalize();
  verifyModule(M);
  *out << "\n\n\n";
}

void init(std::string &backend) {
  DefaultBackend = backend;
  auto TripleStr = DefaultTT.getTriple();
  assert(TripleStr == Triple::normalize(TripleStr));
  /*
   * FIXME we probably want to ask the client to run these
   * initializers
   */
  if (DefaultBackend == "aarch64") {
    LLVMInitializeAArch64TargetInfo();
    LLVMInitializeAArch64Target();
    LLVMInitializeAArch64TargetMC();
    LLVMInitializeAArch64AsmParser();
    LLVMInitializeAArch64AsmPrinter();
  } else if (DefaultBackend == "riscv64") {
    LLVMInitializeRISCVTargetInfo();
    LLVMInitializeRISCVTarget();
    LLVMInitializeRISCVTargetMC();
    LLVMInitializeRISCVAsmParser();
    LLVMInitializeRISCVAsmPrinter();
  } else {
    assert(false);
  }
  string Error;
  Targ = TargetRegistry::lookupTarget(DefaultTT, Error);
  if (!Targ) {
    *out << Error;
    exit(-1);
  }
  origRetWidth = 64;
  has_ret_attr = false;
}

void fixupOptimizedTgt(llvm::Function *tgt) {
  /*
   * these attributes can be soundly removed, and a good thing too
   * since they cause spurious TV failures in ASM memory mode
   */
  for (auto arg = tgt->arg_begin(); arg != tgt->arg_end(); ++arg) {
    arg->removeAttr(llvm::Attribute::Captures);
    arg->removeAttr(llvm::Attribute::ReadNone);
    arg->removeAttr(llvm::Attribute::ReadOnly);
    arg->removeAttr(llvm::Attribute::WriteOnly);
  }

  /*
   * when we originally generated the target function, we allocated
   * its stack memory using a custom allocation function; this is to
   * keep LLVM from making unwarranted assumptions about that memory
   * and optimizing it in undesirable ways. however, Alive doesn't
   * want to see the custom allocator. so, here, before passing target
   * to Alive, we replace it with a regular old alloc
   */
  Instruction *myAllocCall{nullptr};
  for (auto &bb : *tgt) {
    for (auto &i : bb) {
      if (auto *ci = dyn_cast<CallInst>(&i)) {
        if (auto callee = ci->getCalledFunction()) {
          if (callee == myAlloc) {
            IRBuilder<> B(&i);
            auto *i8Ty = Type::getInt8Ty(ci->getContext());
            auto *alloca = B.CreateAlloca(i8Ty, 0, stackSize, "stack");
            alloca->setAlignment(Align(16));
            i.replaceAllUsesWith(alloca);
            assert(myAllocCall == nullptr);
            myAllocCall = &i;
          }
        }
      }
    }
  }
  if (myAllocCall)
    myAllocCall->eraseFromParent();
}

pair<Function *, Function *>
liftFunc(Function *srcFn, unique_ptr<MemoryBuffer> MB,
         std::unordered_map<unsigned, llvm::Instruction *> &lineMap) {
  unique_ptr<mc2llvm> lifter;
  if (DefaultBackend == "aarch64") {
    lifter = make_unique<arm2llvm>(srcFn, std::move(MB), lineMap);
  } else if (DefaultBackend == "riscv64") {
    lifter = make_unique<riscv2llvm>(srcFn, std::move(MB), lineMap);
  } else {
    *out << "ERROR: Nonexistent backend\n";
    exit(-1);
  }

  return lifter->run();
}

} // namespace lifter
