#include "llvm/MC/MCAsmInfo.h" // include first to avoid ambiguity for comparison operator from util/spaceship.h

#include "cache/cache.h"
#include "ir/instr.h"
#include "ir/type.h"
#include "llvm_util/llvm2alive.h"
#include "llvm_util/llvm_optimizer.h"
#include "llvm_util/utils.h"
#include "smt/smt.h"
#include "tools/transform.h"
#include "util/sort.h"
#include "util/version.h"
#include "backend_tv/mc.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCTargetOptionsCommandFlags.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#define GET_INSTRINFO_ENUM
#include "Target/AArch64/AArch64GenInstrInfo.inc"

#define GET_REGINFO_ENUM
#include "Target/AArch64/AArch64GenRegisterInfo.inc"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace tools;
using namespace util;
using namespace std;
using namespace llvm_util;
using namespace llvm;

size_t MCOperandHash::operator()(const MCOperand &op) const {
  unsigned prefix;
  unsigned id;

  if (op.isReg()) {
    prefix = Kind::reg;
    id = op.getReg();
  } else if (op.isImm()) {
    prefix = Kind::immedidate;
    id = op.getImm();
  } else if (op.isExpr()) {
    prefix = Kind::symbol;
    auto expr = op.getExpr();
    if (expr->getKind() == MCExpr::ExprKind::SymbolRef) {
      const MCSymbolRefExpr &SRE = cast<MCSymbolRefExpr>(*expr);
      const MCSymbol &Sym = SRE.getSymbol();
      errs() << "label : " << Sym.getName() << '\n'; // FIXME remove when done
      id = Sym.getOffset();
    } else {
      assert("unsupported mcExpr" && false);
    }
  } else {
    assert("no" && false);
  }

  return std::hash<unsigned long>()(prefix * id);
}


