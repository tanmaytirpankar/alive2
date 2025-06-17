#pragma once

#include <ostream>
#include <string>

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"

namespace llvm {
class Constant;
class Function;
class VectorType;
} // namespace llvm

// FIXME -- cleanup lots of dead stuff below

namespace lifter {

// amount of stack available for use by the lifted function, in bytes
#define stackBytes 1024

// number of 8-byte stack slots for parameters
#define numStackSlots 32

void checkArguments(llvm::CallInst *ci, llvm::Value *val);
void checkVectorTy(llvm::VectorType *Ty);

// FIXME we'd rather not have these globals shared across files

// details from rewriting the src function
extern unsigned origRetWidth;
extern bool has_ret_attr;
extern const llvm::Target *Targ;
extern llvm::Function *myAlloc;
extern llvm::Constant *stackSize;

// TODO -- let users change these?
extern llvm::Triple DefaultTT;
extern const char *DefaultDL;
extern const char *DefaultCPU;
extern const char *DefaultFeatures;

void init(std::string &backend, std::ostream *out);

void nameGlobals(llvm::Module *);
llvm::Function *adjustSrc(llvm::Function *);
void addDebugInfo(llvm::Function *);
void checkSupport(llvm::Function *);
void fixupOptimizedTgt(llvm::Function *);

std::unique_ptr<llvm::MemoryBuffer> generateAsm(llvm::Module &);
void addDebugInfo(llvm::Function *srcFn,
                  std::unordered_map<unsigned, llvm::Instruction *> &lineMap);

std::pair<llvm::Function *, llvm::Function *>
liftFunc(llvm::Function *, std::unique_ptr<llvm::MemoryBuffer>,
         std::unordered_map<unsigned, llvm::Instruction *> &lineMap,
         std::string optimize_tgt, std::ostream *out);

inline std::string moduleToString(llvm::Module *M) {
  std::string sss;
  llvm::raw_string_ostream ss(sss);
  M->print(ss, nullptr);
  return sss;
}

} // namespace lifter
