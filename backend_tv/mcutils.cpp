#include "backend_tv/mcutils.h"

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

using namespace llvm;
using namespace lifter;

void MCFunction::checkEntryBlock(unsigned jumpOpcode) {
  // LLVM doesn't let the entry block be a jump target, but assembly
  // does; we can fix that up by adding an extra block at the start
  // of the function. simplifyCFG will clean this up when it's not
  // needed.
  BBs.emplace(BBs.begin(), "arm_tv_entry");
  MCInst jmp_instr;
  jmp_instr.setOpcode(jumpOpcode);
  jmp_instr.addOperand(MCOperand::createImm(1));
  BBs[0].addInstBegin(std::move(jmp_instr));
}
