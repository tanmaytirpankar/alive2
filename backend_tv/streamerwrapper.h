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

using namespace llvm;

// We're overriding MCStreamerWrapper to generate an MCFunction
// from the arm assembly. MCStreamerWrapper provides callbacks to handle
// different parts of the assembly file. The callbacks that we're
// using right now are all emit* functions.
class MCStreamerWrapper final : public MCStreamer {
  enum ASMLine { none = 0, label = 1, non_term_instr = 2, terminator = 3 };

private:
  MCInstrAnalysis *IA;
  MCBasicBlock *curBB{nullptr};
  unsigned prev_line{0};
  Align curAlign;
  std::string curSym;
  std::string curSec;
  bool FunctionEnded = false;
  unsigned curDebugLine = 0;

  vector<RODataItem> curROData;

public:
  MCFunction MF;
  unsigned cnt{0};

  MCStreamerWrapper(MCContext &Context, MCInstrAnalysis *IA,
                    MCInstPrinter *InstPrinter, MCRegisterInfo *MRI)
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
  virtual void emitInstruction(const MCInst &Inst,
                               const MCSubtargetInfo & /* unused */) override;

  std::string attrName(MCSymbolAttr A) {
    switch (A) {
    case MCSA_ELF_TypeFunction:
      return "ELF function";
    case MCSA_ELF_TypeObject:
      return "ELF object";
    case MCSA_Global:
      return "global";
    default:
      assert(false && "unknown symbol attribute");
    }
  }

  virtual bool emitSymbolAttribute(MCSymbol *Symbol,
                                   MCSymbolAttr Attribute) override {
    *out << "[emitSymbolAttribute '" << Symbol->getName().str() << "']\n";
    if (false) {
      *out << "  Common? " << Symbol->isCommon() << "\n";
      *out << "  Variable? " << Symbol->isVariable() << "\n";
    }
    return true;
  }

  virtual void emitSymbolDesc(MCSymbol *Symbol, unsigned DescValue) override {
    *out << "[emitSymbolDesc '" << Symbol->getName().str() << "']\n";
  }

  virtual void emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                Align ByteAlignment) override {
    vector<RODataItem> data;
    std::string name{Symbol->getName()};
    for (uint64_t i = 0; i < Size; ++i)
      data.push_back(RODataItem{'0'});
    MCGlobal g{
        .name = name,
        .align = ByteAlignment,
        .section = "common",
        .data = data,
    };
    MF.MCglobals.emplace_back(g);
    *out << "[emitCommonSymbol = " << name << "]\n";
  }

  virtual void emitBytes(StringRef Data) override {
    auto len = Data.size();
    *out << "[emitBytes " << len << " bytes]\n";
    for (unsigned i = 0; i < len; ++i) {
      auto rod = RODataItem{Data[i]};
      curROData.push_back(rod);
    }
  }

  virtual void emitFill(const MCExpr &NumBytes, uint64_t FillValue,
                        SMLoc Loc) override {
    auto ce = dyn_cast<MCConstantExpr>(&NumBytes);
    if (ce) {
      assert(FillValue < 256);
      auto bytes = ce->getValue();
      *out << "[emitFill value = " << FillValue << ", size = " << bytes
           << "]\n";
      for (int i = 0; i < bytes; ++i) {
        auto rod = RODataItem{(char)FillValue};
        curROData.push_back(rod);
      }
    } else {
      *out << "[emitFill is unknown!]\n";
    }
  }

  virtual void emitZerofill(MCSection *Section, MCSymbol *Symbol = nullptr,
                            uint64_t Size = 0, Align ByteAlignment = Align(1),
                            SMLoc Loc = SMLoc()) override {
    *out << "[emitZerofill " << Size << " bytes]\n";
  }

  virtual void emitELFSize(MCSymbol *Symbol, const MCExpr *Value) override {
    *out << "[emitELFSize '" << Symbol->getName().str() << "']\n";
    addConstant();
  }

  // FIXME -- we probably need a proper recursive descent parser for
  // MCExprs here
  virtual void emitValueImpl(const MCExpr *Value, unsigned Size,
                             SMLoc Loc = SMLoc()) override;

  virtual void emitValueToAlignment(Align Alignment, int64_t Value = 0,
                                    unsigned ValueSize = 1,
                                    unsigned MaxBytesToEmit = 0) override {
    *out << "[emitValueToAlignment= " << Alignment.value() << "]\n";
    curAlign = Alignment;
  }

  virtual void emitAssignment(MCSymbol *Symbol, const MCExpr *Value) override {
    *out << "[emitAssignment]\n";
  }

  virtual void emitDwarfLocDirective(unsigned FileNo, unsigned Line,
                                     unsigned Column, unsigned Flags,
                                     unsigned Isa, unsigned Discriminator,
                                     StringRef FileName,
                                     StringRef Comment) override {
    *out << "[dwarf loc directive: line = " << Line << "]\n";
    curDebugLine = Line;
  }

  virtual void emitLabel(MCSymbol *Symbol, SMLoc Loc) override;

  std::string findTargetLabel(MCInst &Inst) {
    auto num_operands = Inst.getNumOperands();
    for (unsigned i = 0; i < num_operands; ++i) {
      auto op = Inst.getOperand(i);
      if (op.isExpr()) {
        auto expr = op.getExpr();
        if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
          const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
          const MCSymbol &Sym = SRE.getSymbol();
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
