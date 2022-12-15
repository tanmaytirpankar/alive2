#include "ir/function.h"

#include "llvm/IR/Module.h"

llvm::Function *lift_func(llvm::Module &ArmModule, llvm::Module &LiftedModule, bool asm_input,
                          std::string opt_file2, bool opt_asm_only,
                          IR::Function &AF, llvm::Function *F);
