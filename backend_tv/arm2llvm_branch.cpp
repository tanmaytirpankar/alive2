#include "backend_tv/arm2llvm.h"

void arm2llvm::lift_branch() {
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
}
