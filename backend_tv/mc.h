#include "llvm/MC/MCInst.h"

struct MCOperandHash {

  enum Kind {
    reg = (1 << 2) - 1,
    immedidate = (1 << 3) - 1,
    symbol = (1 << 4) - 1
  };

  size_t operator()(const llvm::MCOperand &op) const; 
};

