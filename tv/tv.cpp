<<<<<<< HEAD
// Copyright (c) 2018-present The Alive2 Authors.
// Distributed under the MIT license that can be found in the LICENSE file.

#include "cache/cache.h"
#include "ir/memory.h"
#include "llvm_util/llvm2alive.h"
#include "llvm_util/utils.h"
#include "smt/smt.h"
#include "smt/solver.h"
#include "tools/transform.h"
#include "util/parallel.h"
#include "util/stopwatch.h"
#include "util/version.h"
=======
>>>>>>> 9021866161d283a2df4d0f86dab32174bba46a48
#include "llvm/ADT/Any.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <signal.h>
#include <sstream>
#include <unistd.h>
#include <unordered_map>
#include <utility>

using namespace std;

<<<<<<< HEAD
#define LLVM_ARGS_PREFIX "tv-"
#define ARGS_SRC_TGT
#define ARGS_REFINEMENT
#include "llvm_util/cmd_args_list.h"

namespace {

llvm::cl::opt<string> parallel_tv("tv-parallel",
  llvm::cl::desc("Parallelization mode. Accepted values:"
                  " unrestricted (no throttling)"
                  ", fifo (use Alive2's job server)"
                  ", null (developer mode)"),
  llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<int> max_subprocesses("max-subprocesses",
  llvm::cl::desc("Maximum children any single clang instance will have at one "
                 "time (default=128)"),
  llvm::cl::init(128), llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<long> subprocess_timeout("tv-subprocess-timeout",
  llvm::cl::desc("Maximum time, in seconds, that a parallel TV call "
                 "will be allowed to execeute (default=infinite)"),
  llvm::cl::init(-1), llvm::cl::cat(alive_cmdargs));

llvm::cl::opt<bool> batch_opts("tv-batch-opts",
  llvm::cl::desc("Batch optimizations (clang plugin only)"),
  llvm::cl::cat(alive_cmdargs));


struct FnInfo {
  Function fn;
  string fn_tostr;
  unsigned n = 0;
};

optional<smt::smt_initializer> smt_init;
optional<llvm_util::initializer> llvm_util_init;
TransformPrintOpts print_opts;
unordered_map<string, FnInfo> fns;
unsigned initialized = 0;
bool showed_stats = false;
bool has_failure = false;
// If is_clangtv is true, tv should exit with zero
bool is_clangtv = false;
unique_ptr<Cache> cache;
unique_ptr<parallel> parallelMgr;
stringstream parent_ss;
std::unique_ptr<llvm::Module> MClone;
string pass_name;

void sigalarm_handler(int) {
  parallelMgr->finishChild(/*is_timeout=*/true);
  // this is a fully asynchronous exit, skip destructors and such
  _Exit(0);
}

void printDot(const Function &tgt, int n) {
  if (opt_print_dot) {
    string prefix = to_string(n);
    tgt.writeDot(prefix.c_str());
  }
}

string toString(const Function &fn) {
  stringstream ss;
  fn.print(ss);
  return std::move(ss).str();
}

static void showStats() {
  if (opt_smt_stats)
    smt::solver_print_stats(*out);
  if (opt_alias_stats)
    IR::Memory::printAliasStats(*out);
}

static void writeBitcodeAtomically(const fs::path report_filename) {
  fs::path tmp_path;
  do {
    auto newname = report_filename.stem();
    newname += "_" + get_random_str(8) + ".bc";
    tmp_path.replace_filename(newname);
  } while (fs::exists(tmp_path));

  std::error_code EC;
  llvm::raw_fd_ostream tmp_file(tmp_path.string(), EC);
  if (EC) {
    cerr << "Alive2: Couldn't open temporary bitcode file" << endl;
    exit(1);
  }
  llvm::WriteBitcodeToFile(*MClone, tmp_file);
  tmp_file.close();

  fs::path bc_filename = tmp_path;
  if (!report_filename.empty()) {
    bc_filename = report_filename;
    bc_filename.replace_extension(".bc");
    std::rename(tmp_path.c_str(), bc_filename.c_str());
  }
  *out << "Wrote bitcode to: " << bc_filename << '\n';
}

static void emitCommandLine(ostream *out) {
#ifdef __linux__
  ifstream cmd_args("/proc/self/cmdline");
  if (!cmd_args.is_open()) {
    return;
  }
  *out << "Command line:";
  std::string arg;
  while (std::getline(cmd_args, arg, '\0'))
    *out << " '" << arg << "'";
  *out << "\n";
#endif
}

=======
>>>>>>> 9021866161d283a2df4d0f86dab32174bba46a48
struct TVLegacyPass final : public llvm::ModulePass {
  static char ID;

  TVLegacyPass() : ModulePass(ID) {}

  bool runOnModule(llvm::Module &M) override {
    llvm::outs() << "hello from tv plugin!\n";
    for (auto &F : M) {
      if (!F.isDeclaration())
	llvm::outs() << F.getName() << " ";
    }
    return false;
  }

<<<<<<< HEAD
  static void verify(Transform &t, int n, const string &src_tostr) {
    printDot(t.tgt, n);

    auto tgt_tostr = toString(t.tgt);
    if (!opt_always_verify) {
      // Compare Alive2 IR and skip if syntactically equal
      if (src_tostr == tgt_tostr) {
        if (!opt_quiet)
          t.print(*out, print_opts);
        *out << "Transformation seems to be correct! (syntactically equal)\n\n";
        return;
      }
    }

    // Since we have an open connection to the Redis server, we have
    // to do this before forking. Anyway, this is fast.
    if (cache && cache->lookup(src_tostr + "===\n" + tgt_tostr)) {
      *out << "Skipping repeated query\n\n";
      return;
    }

    if (parallelMgr) {
      auto [pid, osp, index] = parallelMgr->limitedFork();

      if (pid == -1) {
        perror("fork() failed");
        exit(-1);
      }

      if (pid != 0) {
        /*
         * parent returns to LLVM immediately; leave a placeholder in
         * the output that we'll patch up later
         */
        *out << "include(" << index << ")\n";
        /*
         * Tell the caller that tgt should be regenerated via llvm2alive.
         * TODO: this llvm2alive() call isn't needed for correctness,
         * but only to make parallel output match sequential
         * output. we can remove it later if we want.
         */
        return;
      }

      if (subprocess_timeout != -1) {
        ENSURE(signal(SIGALRM, sigalarm_handler) == nullptr);
        alarm(subprocess_timeout);
      }

      /*
       * child now writes to a stringstream provided by the parallel
       * manager, its output will get pushed to the parent via a pipe
       * later on
       */
      out = osp;
      set_outs(*out);
    }

    /*
     * from here, we must not return back to LLVM if parallelMgr
     * is non-null; instead we call parallelMgr->finishChild()
     */

    smt_init->reset();
    t.preprocess();
    TransformVerify verifier(t, false);
    if (!opt_quiet)
      t.print(*out, print_opts);

    {
      auto types = verifier.getTypings();
      if (!types) {
        *out << "Transformation doesn't verify!\n"
                "ERROR: program doesn't type check!\n\n";
        goto done;
      }
      assert(types.hasSingleTyping());
    }

    if (Errors errs = verifier.verify()) {
      *out << "Transformation doesn't verify!" <<
              (errs.isUnsound() ? " (unsound)\n" : " (not unsound)\n")
           << errs;
      if (errs.isUnsound()) {
        has_failure = true;
        *out << "\nPass: " << pass_name << '\n';
        emitCommandLine(out);
        if (MClone)
          writeBitcodeAtomically(report_filename);
        *out << "\n";
      }
      if (opt_error_fatal && has_failure)
        finalize();
    } else {
      *out << "Transformation seems to be correct!\n\n";
    }

  done:
    if (parallelMgr) {
      showStats();
      signal(SIGALRM, SIG_IGN);
      llvm_util_init.reset();
      smt_init.reset();
      parallelMgr->finishChild(/*is_timeout=*/false);
      exit(0);
    }
  }

  static void initialize(llvm::Module &module) {
    if (initialized++)
      return;

#define ARGS_MODULE_VAR (&module)
#   include "llvm_util/cmd_args_def.h"

    if (parallel_tv == "unrestricted") {
      parallelMgr = make_unique<unrestricted>(max_subprocesses, parent_ss,
                                              *out);
    } else if (parallel_tv == "fifo") {
      parallelMgr = make_unique<fifo>(max_subprocesses, parent_ss, *out);
    } else if (parallel_tv == "null") {
      parallelMgr = make_unique<null>(max_subprocesses, parent_ss, *out);
    } else if (!parallel_tv.empty()) {
      *out << "Alive2: Unknown parallelization mode: " << parallel_tv << endl;
      exit(1);
    }

    if (parallelMgr) {
      if (parallelMgr->init()) {
        out = &parent_ss;
        set_outs(*out);
      } else {
        *out << "WARNING: Parallel execution of Alive2 Clang plugin is "
                "unavailable, sorry\n";
        parallelMgr.reset();
      }
    }

    showed_stats = false;
    llvm_util_init.emplace(*out, module.getDataLayout());
    smt_init.emplace();
    return;
  }

  bool doFinalization(llvm::Module&) override {
    finalize();
    return false;
  }

  static void finalize() {
    MClone = nullptr;
    if (parallelMgr) {
      parallelMgr->finishParent();
      out = out_file.is_open() ? &out_file : &cout;
      set_outs(*out);
    }

    // If it is run in parallel, stats are shown by children
    if (!showed_stats && !parallelMgr) {
      showed_stats = true;
      showStats();
      if (has_failure && !report_filename.empty())
        cerr << "Report written to " << report_filename << endl;
    }

    llvm_util_init.reset();
    smt_init.reset();
    --initialized;

    if (has_failure) {
      if (opt_error_fatal)
        *out << "Alive2: Transform doesn't verify; aborting!" << endl;
      else
        *out << "Alive2: Transform doesn't verify!" << endl;

      if (!is_clangtv)
        exit(1);
    }
  }

=======
>>>>>>> 9021866161d283a2df4d0f86dab32174bba46a48
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<llvm::TargetLibraryInfoWrapperPass>();
    AU.setPreservesAll();
  }
};

char TVLegacyPass::ID = 0;
llvm::RegisterPass<TVLegacyPass> X("tv", "Translation Validator", false, false);

