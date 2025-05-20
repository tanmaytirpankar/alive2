// Represents a basic block of machine instructions
class MCBasicBlock {
private:
  string name;
  vector<MCInst> Instrs;
  SetVector<MCBasicBlock *> Succs;

public:
  MCBasicBlock(const string &name) : name(name) {}

  const string &getName() const {
    return name;
  }

  auto &getInstrs() {
    return Instrs;
  }

  bool empty() const {
    return Instrs.size() == 0;
  }

  auto &getSuccs() {
    return Succs;
  }

  void addInst(MCInst &inst) {
    Instrs.push_back(inst);
  }

  void addInstBegin(MCInst &&inst) {
    Instrs.insert(Instrs.begin(), std::move(inst));
  }

  void addSucc(MCBasicBlock *succ_block) {
    Succs.insert(succ_block);
  }
};

struct OffsetSym {
  string sym;
  long offset;
};

typedef variant<OffsetSym, char> RODataItem;

struct MCGlobal {
  string name;
  Align align;
  string section;
  vector<RODataItem> data;
};

class MCFunction {
  string name;
  unsigned label_cnt{0};

public:
  MCInstrAnalysis *IA;
  MCInstPrinter *InstPrinter;
  MCRegisterInfo *MRI;
  vector<MCBasicBlock> BBs;
  vector<MCGlobal> MCglobals;

  MCFunction() {
    MCGlobal g{
        .name = "__stack_chk_guard",
        .align = Align(8),
        .section = ".rodata",
        // FIXME -- use symbolic data here? does this matter?
        .data = {'7', '7', '7', '7', '7', '7', '7', '7'},
    };
    MCglobals.push_back(g);
  }

  void setName(const string &_name) {
    name = _name;
  }

  MCBasicBlock *addBlock(const string &b_name) {
    return &BBs.emplace_back(b_name);
  }

  string getName() {
    return name;
  }

  string getLabel() {
    return name + to_string(++label_cnt);
  }

  MCBasicBlock *findBlockByName(const string &b_name) {
    for (auto &bb : BBs)
      if (bb.getName() == b_name)
        return &bb;
    return nullptr;
  }

  void checkEntryBlock() {
    // LLVM doesn't let the entry block be a jump target, but assembly
    // does; we can fix that up by adding an extra block at the start
    // of the function. simplifyCFG will clean this up when it's not
    // needed.
    BBs.emplace(BBs.begin(), "arm_tv_entry");
    MCInst jmp_instr;
    jmp_instr.setOpcode(AArch64::B);
    jmp_instr.addOperand(MCOperand::createImm(1));
    BBs[0].addInstBegin(std::move(jmp_instr));
  }
};

