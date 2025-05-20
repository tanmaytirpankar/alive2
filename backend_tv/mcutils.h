#pragma once

#include "llvm/ADT/SetVector.h"
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

#include <string>
#include <vector>

using namespace llvm;

// Represents a basic block of machine instructions
class MCBasicBlock {
private:
  std::string name;
  std::vector<MCInst> Instrs;
  llvm::SetVector<MCBasicBlock *> Succs;

public:
  MCBasicBlock(const std::string &name) : name(name) {}

  const std::string &getName() const {
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
  std::string sym;
  long offset;
};

typedef std::variant<OffsetSym, char> RODataItem;

struct MCGlobal {
  std::string name;
  Align align;
  std::string section;
  std::vector<RODataItem> data;
};

class MCFunction {
  std::string name;
  unsigned label_cnt{0};

public:
  MCInstrAnalysis *IA;
  MCInstPrinter *InstPrinter;
  MCRegisterInfo *MRI;
  std::vector<MCBasicBlock> BBs;
  std::vector<MCGlobal> MCglobals;

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

  void setName(const std::string &_name) {
    name = _name;
  }

  MCBasicBlock *addBlock(const std::string &b_name) {
    return &BBs.emplace_back(b_name);
  }

  std::string getName() {
    return name;
  }

  std::string getLabel() {
    return name + std::to_string(++label_cnt);
  }

  MCBasicBlock *findBlockByName(const std::string &b_name) {
    for (auto &bb : BBs)
      if (bb.getName() == b_name)
        return &bb;
    return nullptr;
  }

  void checkEntryBlock();
};

