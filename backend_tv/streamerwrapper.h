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

#include "backend_tv/mc2llvm.h"
#include "backend_tv/mcutils.h"

namespace lifter {

// We're overriding MCStreamerWrapper to generate an MCFunction
// from the arm assembly. MCStreamerWrapper provides callbacks to handle
// different parts of the assembly file. The callbacks that we're
// using right now are all emit* functions.
class MCStreamerWrapper final : public llvm::MCStreamer {
  enum ASMLine { none = 0, label = 1, non_term_instr = 2, terminator = 3 };

private:
  llvm::MCInstrAnalysis *IA;
  MCBasicBlock *curBB{nullptr};
  unsigned prev_line{0};
  llvm::Align curAlign;
  std::string curSym;
  std::string curSec;
  bool FunctionEnded = false;
  unsigned curDebugLine = 0;
  std::vector<RODataItem> curROData;

public:
  MCFunction MF;
  unsigned cnt{0};

  MCStreamerWrapper(llvm::MCContext &Context, llvm::MCInstrAnalysis *IA,
                    llvm::MCInstPrinter *InstPrinter, llvm::MCRegisterInfo *MRI)
      : MCStreamer(Context), IA(IA) {
    MF.IA = IA;
    MF.InstPrinter = InstPrinter;
    MF.MRI = MRI;
  }

  void addConstant() {
    if (curROData.empty())
      return;

    MCGlobal g{
        .name = curSym,
        .align = curAlign,
        .section = curSec,
        .data = curROData,
    };
    MF.MCglobals.emplace_back(g);
    curROData.clear();
    *out << "created constant: " << curSym << "\n";
  }

  // We only want to intercept the emission of new instructions.
  virtual void
  emitInstruction(const llvm::MCInst &Inst,
                  const llvm::MCSubtargetInfo & /* unused */) override;

  std::string attrName(llvm::MCSymbolAttr A) {
    switch (A) {
    case llvm::MCSA_ELF_TypeFunction:
      return "ELF function";
    case llvm::MCSA_ELF_TypeObject:
      return "ELF object";
    case llvm::MCSA_Global:
      return "global";
    default:
      assert(false && "unknown symbol attribute");
    }
  }

  virtual bool emitSymbolAttribute(llvm::MCSymbol *Symbol,
                                   llvm::MCSymbolAttr Attribute) override {
    *out << "[emitSymbolAttribute '" << Symbol->getName().str() << "']\n";
    if (false) {
      *out << "  Common? " << Symbol->isCommon() << "\n";
      *out << "  Variable? " << Symbol->isVariable() << "\n";
    }
    return true;
  }

  virtual void emitSymbolDesc(llvm::MCSymbol *Symbol,
                              unsigned DescValue) override {
    *out << "[emitSymbolDesc '" << Symbol->getName().str() << "']\n";
  }

  virtual void emitCommonSymbol(llvm::MCSymbol *Symbol, uint64_t Size,
                                llvm::Align ByteAlignment) override;

  virtual void emitBytes(llvm::StringRef Data) override;

  virtual void emitFill(const llvm::MCExpr &NumBytes, uint64_t FillValue,
                        llvm::SMLoc Loc) override;

  virtual void emitZerofill(llvm::MCSection *Section,
                            llvm::MCSymbol *Symbol = nullptr, uint64_t Size = 0,
                            llvm::Align ByteAlignment = llvm::Align(1),
                            llvm::SMLoc Loc = llvm::SMLoc()) override {
    *out << "[emitZerofill " << Size << " bytes]\n";
  }

  virtual void emitELFSize(llvm::MCSymbol *Symbol,
                           const llvm::MCExpr *Value) override {
    *out << "[emitELFSize '" << Symbol->getName().str() << "']\n";
    addConstant();
  }

  // FIXME -- we probably need a proper recursive descent parser for
  // MCExprs here
  virtual void emitValueImpl(const llvm::MCExpr *Value, unsigned Size,
                             llvm::SMLoc Loc = llvm::SMLoc()) override;

  virtual void emitValueToAlignment(llvm::Align Alignment, int64_t Value = 0,
                                    unsigned ValueSize = 1,
                                    unsigned MaxBytesToEmit = 0) override {
    *out << "[emitValueToAlignment= " << Alignment.value() << "]\n";
    curAlign = Alignment;
  }

  virtual void emitAssignment(llvm::MCSymbol *Symbol,
                              const llvm::MCExpr *Value) override {
    *out << "[emitAssignment]\n";
  }

  virtual void emitDwarfLocDirective(unsigned FileNo, unsigned Line,
                                     unsigned Column, unsigned Flags,
                                     unsigned Isa, unsigned Discriminator,
                                     llvm::StringRef FileName,
                                     llvm::StringRef Comment) override {
    *out << "[dwarf loc directive: line = " << Line << "]\n";
    curDebugLine = Line;
  }

  virtual void emitLabel(llvm::MCSymbol *Symbol, llvm::SMLoc Loc) override;

  std::string findTargetLabel(llvm::MCInst &Inst) {
    auto num_operands = Inst.getNumOperands();
    for (unsigned i = 0; i < num_operands; ++i) {
      auto op = Inst.getOperand(i);
      if (op.isExpr()) {
        auto expr = op.getExpr();
        if (expr->getKind() == llvm::MCExpr::ExprKind::SymbolRef) {
          const llvm::MCSymbolRefExpr &SRE = cast<llvm::MCSymbolRefExpr>(*expr);
          const llvm::MCSymbol &Sym = SRE.getSymbol();
          return Sym.getName().str();
        }
      }
    }
    assert(false && "could not find target label in arm branch instruction");
  }

  // Make sure that we have an entry label with no predecessors
  void checkEntryBlock() {
    MF.checkEntryBlock();
  }

  // Fill in the CFG
  void generateSuccessors();

  // Remove empty basic blocks, including .Lfunc_end
  void removeEmptyBlocks() {
    erase_if(MF.BBs, [](MCBasicBlock bb) { return bb.empty(); });
  }
};

} // end namespace lifter
