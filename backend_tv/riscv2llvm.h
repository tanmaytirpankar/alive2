class riscv2llvm : public mc2llvm {
  Value *enforceSExtZExt(Value *V, bool isSExt, bool isZExt) override {
    return nullptr;
  }

  llvm::AllocaInst *get_reg(aslp::reg_t regtype, uint64_t num) override {
    return nullptr;
  }

  void updateOutputReg(Value *V, bool SExt = false) override {}

  Value *makeLoadWithOffset(Value *base, Value *offset, int size) override {
    return nullptr;
  }

  Value *getIndexedElement(unsigned idx, unsigned eltSize,
                           unsigned reg) override {
    return nullptr;
  }

  void doCall(FunctionCallee FC, CallInst *llvmCI,
              const string &calleeName) override {}

  void lift(MCInst &I) override {}

  Value *createRegFileAndStack() override {
    return nullptr;
  }

  void doReturn() override {}

public:
  riscv2llvm(Module *LiftedModule, MCFunction &MF, Function &srcFn,
             MCInstPrinter *InstPrinter, const MCCodeEmitter &MCE,
             const MCSubtargetInfo &STI, const MCInstrAnalysis &IA)
      : mc2llvm(LiftedModule, MF, srcFn, InstPrinter, MCE, STI, IA) {}
};

