#pragma once

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

#include "backend_tv/arm2llvm.h"
#include "backend_tv/bitutils.h"
#include "backend_tv/lifter.h"
#include "backend_tv/mc2llvm.h"
#include "backend_tv/mcutils.h"
#include "backend_tv/riscv2llvm.h"
#include "backend_tv/streamerwrapper.h"

#include <cmath>
#include <vector>

using namespace std;
using namespace llvm;
using namespace lifter;

class riscv2llvm final : public mc2llvm {
  Value *enforceSExtZExt(Value *V, bool isSExt, bool isZExt) override;

  llvm::AllocaInst *get_reg(aslp::reg_t regtype, uint64_t num) override;

  void updateOutputReg(Value *V, bool SExt = false) override;

  Value *makeLoadWithOffset(Value *base, Value *offset, int size) override;

  Value *getIndexedElement(unsigned idx, unsigned eltSize,
                           unsigned reg) override;

  void doCall(FunctionCallee FC, CallInst *llvmCI,
              const string &calleeName) override;

  void lift(MCInst &I) override;

  Value *createRegFileAndStack() override;

  void doReturn() override;

public:
  riscv2llvm(Module *LiftedModule, MCFunction &MF, Function &srcFn,
             MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
             const MCSubtargetInfo &STI, const MCInstrAnalysis &IA);
};
