#include <optional>

#include "ir/function.h"

#include "llvm/IR/Module.h"

std::optional<IR::Function> lift_func(llvm::Module &M, bool asm_input,
                                      std::string opt_file2, bool opt_asm_only,
                                      IR::Function &AF);
