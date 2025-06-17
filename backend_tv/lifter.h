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

// FIXME eliminate all globals

// TODO -- let users change these?
extern llvm::Triple DefaultTT;
extern const char *DefaultDL;
extern const char *DefaultCPU;
extern const char *DefaultFeatures;

void init(const llvm::Target *Targ, std::ostream *out);

void nameGlobals(llvm::Module *);
llvm::Function *adjustSrc(llvm::Function *);
void addDebugInfo(llvm::Function *);
void checkSupport(llvm::Function *);
void fixupOptimizedTgt(llvm::Function *);

std::unique_ptr<llvm::MemoryBuffer> generateAsm(llvm::Module &,
                                                const llvm::Target *Targ);
void addDebugInfo(llvm::Function *srcFn,
                  std::unordered_map<unsigned, llvm::Instruction *> &lineMap);

std::pair<llvm::Function *, llvm::Function *>
liftFunc(llvm::Function *, std::unique_ptr<llvm::MemoryBuffer>,
         std::unordered_map<unsigned, llvm::Instruction *> &lineMap,
         std::string optimize_tgt, std::ostream *out, const llvm::Target *Targ);

inline std::string moduleToString(llvm::Module *M) {
  std::string sss;
  llvm::raw_string_ostream ss(sss);
  M->print(ss, nullptr);
  return sss;
}

} // namespace lifter
