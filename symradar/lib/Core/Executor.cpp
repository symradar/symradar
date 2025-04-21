//===-- Executor.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Executor.h"
#include "Context.h"
#include "CoreStats.h"
#include "ExecutorTimerInfo.h"
#include "ExternalDispatcher.h"
#include "ImpliedValue.h"
#include "MemoryManager.h"
#include "PTree.h"
#include "Searcher.h"
#include "SeedInfo.h"
#include "SpecialFunctionHandler.h"
#include "StatsTracker.h"
#include "TimingSolver.h"
#include "UserSearcher.h"

#include "external/json/json.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"

#include "klee/Common.h"
#include "klee/Config/Version.h"
#include "klee/ExecutionState.h"
#include "klee/Expr.h"
#include "klee/Internal/ADT/KTest.h"
#include "klee/Internal/ADT/RNG.h"
#include "klee/Internal/Module/Cell.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Module/SlicingInfo.h"
#include "klee/Internal/Module/Snapshot.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/Support/FileHandling.h"
#include "klee/Internal/Support/FloatEvaluation.h"
#include "klee/Internal/Support/ModuleUtil.h"
#include "klee/Internal/System/Memory.h"
#include "klee/Internal/System/MemoryUsage.h"
#include "klee/Internal/System/Time.h"
#include "klee/Interpreter.h"
#include "klee/SlicingHandler.h"
#include "klee/SolverCmdLine.h"
#include "klee/SolverStats.h"
#include "klee/TimerStatIncrementer.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/util/ExprSMTLIBPrinter.h"
#include "klee/util/ExprUtil.h"
#include "klee/util/GetElementPtrTypeIterator.h"

#include "../Expr/ArrayExprOptimizer.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Support/CallSite.h"
#elif LLVM_VERSION_CODE < LLVM_VERSION(8, 0)
#include "llvm/IR/CallSite.h"
#else
#include "llvm/IR/InstrTypes.h"
#endif

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <sys/mman.h>

#include <cxxabi.h>
#include <errno.h>

using namespace llvm;
using namespace klee;

namespace {
cl::opt<bool> DumpStatesOnHalt(
    "dump-states-on-halt", cl::init(true),
    cl::desc("Dump test cases for all active states on exit (default=on)"));

int *A_data, *A_data_stat;
std::set<std::string> hit_list;
std::string trace_filter;
std::map<std::string, int *> var_map;
std::map<std::string, int *> arg_map;

/// The different query logging solvers that can switched on/off
enum PrintDebugInstructionsType {
  STDERR_ALL, ///
  STDERR_SRC,
  STDERR_COMPACT,
  FILE_ALL,    ///
  FILE_SRC,    ///
  FILE_COMPACT ///
};

llvm::cl::bits<PrintDebugInstructionsType> DebugPrintInstructions(
    "debug-print-instructions",
    llvm::cl::desc("Log instructions during execution."),
    llvm::cl::values(
        clEnumValN(STDERR_ALL, "all:stderr",
                   "Log all instructions to stderr "
                   "in format [src, inst_id, "
                   "llvm_inst]"),
        clEnumValN(STDERR_SRC, "src:stderr",
                   "Log all instructions to stderr in format [src, inst_id]"),
        clEnumValN(STDERR_COMPACT, "compact:stderr",
                   "Log all instructions to stderr in format [inst_id]"),
        clEnumValN(FILE_ALL, "all:file",
                   "Log all instructions to file "
                   "instructions.txt in format [src, "
                   "inst_id, llvm_inst]"),
        clEnumValN(FILE_SRC, "src:file",
                   "Log all instructions to file "
                   "instructions.txt in format [src, "
                   "inst_id]"),
        clEnumValN(FILE_COMPACT, "compact:file",
                   "Log all instructions to file instructions.txt in format "
                   "[inst_id]") KLEE_LLVM_CL_VAL_END),
    llvm::cl::CommaSeparated);
#ifdef HAVE_ZLIB_H
cl::opt<bool> DebugCompressInstructions(
    "debug-compress-instructions", cl::init(false),
    cl::desc("Compress the logged instructions in gzip format."));
#endif

cl::opt<bool> DebugCheckForImpliedValues("debug-check-for-implied-values");

cl::opt<bool> DisableMemoryCheck(
    "dis-mem-check", cl::init(false),
    cl::desc("Switch off memory violation checking (default=off)"));

cl::opt<bool> PrintTrace(
    "print-trace", cl::init(false),
    cl::desc(
        "Output source location for each instruction executed (default=off)"));

cl::opt<bool> NoExitOnError(
    "no-exit-on-error", cl::init(false),
    cl::desc("Continue execution even after finding an error (default=off)"));

cl::opt<bool> PrintStack(
    "print-stack", cl::init(false),
    cl::desc("Output stack information on error exit (default=off)"));

cl::opt<bool>
    PrintPath("print-path", cl::init(false),
              cl::desc("Output path condition along with source location as "
                       "and when it's updated (default=off)"));

cl::opt<bool>
    LogPPC("log-ppc", cl::init(false),
           cl::desc("Log partial path condition along with source location as "
                    "and when it's updated (default=off)"));

cl::opt<bool> LogTrace("log-trace", cl::init(false),
                       cl::desc("Log instruction trace with source location as "
                                "and when it's executed (default=off)"));

cl::opt<std::string> LocHit("hit-locations", cl::init(""),
                            cl::desc("Log given locations in trace.log if its "
                                     "witnessed (default=log everything)"));

cl::opt<std::string>
    TraceFilter("trace-filter", cl::init(""),
                cl::desc("filter criteria for the trace log (default=None)"));

cl::opt<bool> ResolvePath(
    "resolve-path", cl::init(false),
    cl::desc("In seed mode resolve path using seed values (default=off)"));

cl::opt<bool>
    SimplifySymIndices("simplify-sym-indices", cl::init(true),
                       cl::desc("Simplify symbolic accesses using equalities "
                                "from other constraints (default=on)"));

cl::opt<bool>
    EqualitySubstitution("equality-substitution", cl::init(true),
                         cl::desc("Simplify equality expressions before "
                                  "querying the solver (default=on)."));

cl::opt<unsigned> MaxSymArraySize("max-sym-array-size", cl::init(0));

cl::opt<bool> MakeAllParameterSymbolic(
    "make-all-parameter-symbolic", cl::init(false),
    cl::desc("Make all value typed arguments symbolic"));

cl::list<int> PatchID("patch-id",
                      cl::desc("Patch ID to be used for the execution"),
                      cl::value_desc("0,1,3"), cl::CommaSeparated);

cl::opt<bool> LazyPatch("lazy-patch", cl::init(false),
                        cl::desc("Use lazy patch"));

cl::opt<bool> DumpSnapshot("dump-snapshot", cl::init(false),
                           cl::desc("Dump snapshot"));

llvm::cl::list<int>
    MaxForksPerPhases("max-forks-per-phases",
                      llvm::cl::desc("Maximum number of forks per phases"),
                      llvm::cl::value_desc("base,after,crash"),
                      llvm::cl::CommaSeparated);

cl::opt<bool> SuppressExternalWarnings(
    "suppress-external-warnings", cl::init(false),
    cl::desc("Supress warnings about calling external functions."));

cl::opt<bool> AllExternalWarnings(
    "all-external-warnings", cl::init(false),
    cl::desc("Issue an warning everytime an external call is made,"
             "as opposed to once per function (default=off)"));

cl::opt<bool> OnlyOutputStatesCoveringNew(
    "only-output-states-covering-new", cl::init(false),
    cl::desc("Only output test cases covering new code (default=off)."));

cl::opt<bool> EmitAllErrors(
    "emit-all-errors", cl::init(true),
    cl::desc(
        "Generate tests cases for all errors "
        "(default=on, if set off, emit once per (error,instruction) pair)"));

enum class ExternalCallPolicy {
  None,     // No external calls allowed
  Concrete, // Only external calls with concrete arguments allowed
  All,      // All external calls allowed
};

cl::opt<ExternalCallPolicy> ExternalCalls(
    "external-calls", cl::desc("Specify the external call policy"),
    cl::values(
        clEnumValN(
            ExternalCallPolicy::None, "none",
            "No external function calls are allowed.  Note that KLEE always "
            "allows some external calls with concrete arguments to go through "
            "(in particular printf and puts), regardless of this option."),
        clEnumValN(ExternalCallPolicy::Concrete, "concrete",
                   "Only external function calls with concrete arguments are "
                   "allowed (default)"),
        clEnumValN(ExternalCallPolicy::All, "all",
                   "All external function calls are allowed.  This concretizes "
                   "any symbolic arguments in calls to external functions.")
            KLEE_LLVM_CL_VAL_END),
    cl::init(ExternalCallPolicy::Concrete));

enum SymbolicJumpPolicyType {
  SJP_EarlyExit,
  SJP_FollowBuggyPath,
  SJP_GreedySearch,
};

llvm::cl::opt<SymbolicJumpPolicyType> SymbolicJumpPolicy(
    "symbolic-jump-policy", llvm::cl::desc("Policy for symbolic jumps."),
    llvm::cl::values(
        clEnumValN(SymbolicJumpPolicyType::SJP_EarlyExit, "early-exit",
                   "Exit as soon as a symbolic jump is encountered"),
        clEnumValN(SymbolicJumpPolicyType::SJP_FollowBuggyPath,
                   "follow-buggy-path",
                   "Follow the path that leads to the bug"),
        clEnumValN(SymbolicJumpPolicyType::SJP_GreedySearch, "greedy-search",
                   "Follow the path that leads to the bug, but try to "
                   "minimize the number of symbolic jumps")
            KLEE_LLVM_CL_VAL_END),
    llvm::cl::init(SymbolicJumpPolicyType::SJP_EarlyExit));

enum SymbolizeLevelEnum {
  NoSymbolization = 0,
  LowSymbolization = 1,
  MediumSymbolization = 2,
  HighSymbolization = 3,
  ExtremeSymbolization = 4,
  AllSymbolization = 5,
};

llvm::cl::opt<SymbolizeLevelEnum> SymbolizeLevel(
    "symbolize-level",
    llvm::cl::desc("Level of struct symbolization (default=0), max=3"),
    llvm::cl::values(
        clEnumValN(NoSymbolization, "none", "Do not symbolize"),
        clEnumValN(LowSymbolization, "low", "Symbolize something"),
        clEnumValN(MediumSymbolization, "medium",
                   "Symbolize only part of the memory"),
        clEnumValN(HighSymbolization, "high", "Symbolize every primitive type"),
        clEnumValN(ExtremeSymbolization, "extreme",
                   "Symbolize every primitive type + use less bound")),
    llvm::cl::init(NoSymbolization));

llvm::cl::opt<int> SymbolizeBound(
    "symbolize-bound", llvm::cl::init(1),
    llvm::cl::desc("Bound for symbolization (default=1), max=3"));

cl::opt<bool> AlwaysOutputSeeds("always-output-seeds", cl::init(true));

cl::opt<bool> OnlyReplaySeeds(
    "only-replay-seeds", cl::init(false),
    cl::desc("Discard states that do not have a seed (default=off)."));

cl::opt<bool> OnlySeed("only-seed", cl::init(false),
                       cl::desc("Stop execution after seeding is done without "
                                "doing regular search (default=off)."));

cl::opt<bool>
    AllowSeedExtension("allow-seed-extension", cl::init(false),
                       cl::desc("Allow extra (unbound) values to become "
                                "symbolic during seeding (default=false)."));

cl::opt<bool> ZeroSeedExtension("zero-seed-extension", cl::init(false),
                                cl::desc("(default=off)"));

cl::opt<bool> AllowSeedTruncation(
    "allow-seed-truncation", cl::init(false),
    cl::desc("Allow smaller buffers than in seeds (default=off)."));

cl::opt<bool> NamedSeedMatching(
    "named-seed-matching", cl::init(false),
    cl::desc("Use names to match symbolic objects to inputs (default=off)."));

cl::opt<double> MaxStaticForkPct("max-static-fork-pct", cl::init(1.),
                                 cl::desc("(default=1.0)"));

cl::opt<double> MaxStaticSolvePct("max-static-solve-pct", cl::init(1.),
                                  cl::desc("(default=1.0)"));

cl::opt<double> MaxStaticCPForkPct("max-static-cpfork-pct", cl::init(1.),
                                   cl::desc("(default=1.0)"));

cl::opt<double> MaxStaticCPSolvePct("max-static-cpsolve-pct", cl::init(1.),
                                    cl::desc("(default=1.0)"));

cl::opt<std::string> MaxInstructionTime(
    "max-instruction-time",
    cl::desc("Allow a single instruction to take only this much time "
             "(default=0s (off)). Enables --use-forked-solver"));

cl::opt<std::string>
    SeedTime("seed-time", cl::desc("Amount of time to dedicate to seeds, "
                                   "before normal search (default=0s (off))"));

cl::list<Executor::TerminateReason> ExitOnErrorType(
    "exit-on-error-type",
    cl::desc(
        "Stop execution after reaching a specified condition.  (default=off)"),
    cl::values(
        clEnumValN(Executor::Abort, "Abort", "The program crashed"),
        clEnumValN(Executor::Assert, "Assert", "An assertion was hit"),
        clEnumValN(Executor::BadVectorAccess, "BadVectorAccess",
                   "Vector accessed out of bounds"),
        clEnumValN(Executor::Exec, "Exec",
                   "Trying to execute an unexpected instruction"),
        clEnumValN(Executor::External, "External",
                   "External objects referenced"),
        clEnumValN(Executor::Free, "Free", "Freeing invalid memory"),
        clEnumValN(Executor::Model, "Model", "Memory model limit hit"),
        clEnumValN(Executor::Overflow, "Overflow", "An overflow occurred"),
        clEnumValN(Executor::Ptr, "Ptr", "Pointer error"),
        clEnumValN(Executor::ReadOnly, "ReadOnly", "Write to read-only memory"),
        clEnumValN(Executor::ReportError, "ReportError",
                   "klee_report_error called"),
        clEnumValN(Executor::User, "User", "Wrong klee_* functions invocation"),
        clEnumValN(Executor::Unhandled, "Unhandled",
                   "Unhandled instruction hit") KLEE_LLVM_CL_VAL_END),
    cl::ZeroOrMore);

cl::opt<unsigned long long>
    StopAfterNInstructions("stop-after-n-instructions",
                           cl::desc("Stop execution after specified number of "
                                    "instructions (default=0 (off))"),
                           cl::init(0));

cl::opt<unsigned>
    MaxForks("max-forks",
             cl::desc("Only fork this many times (default=-1 (off))"),
             cl::init(~0u));

cl::opt<unsigned> MaxDepth(
    "max-depth",
    cl::desc("Only allow this many symbolic branches (default=0 (off))"),
    cl::init(0));

cl::opt<unsigned> MaxMemory("max-memory",
                            cl::desc("Refuse to fork when above this amount of "
                                     "memory (in MB, default=2000)"),
                            cl::init(2000));

cl::opt<bool> MaxMemoryInhibit(
    "max-memory-inhibit",
    cl::desc(
        "Inhibit forking at memory cap (vs. random terminate) (default=on)"),
    cl::init(true));

cl::opt<unsigned> RuntimeMaxStackFrames(
    "max-stack-frames",
    cl::desc("Terminates a state after the limit of stack frames is reached "
             "(default=8192). Disable check with 0."),
    cl::init(8192));

cl::opt<bool> UseCFG("use-cfg",
                     cl::desc("Use CFG to guide search (default=on)"),
                     cl::init(true));

cl::opt<bool> UseNullArg("use-null-arg",
                         cl::desc("Use null argument for target function"),
                         cl::init(false));

cl::opt<bool> UseUniKleeMakeSym("use-uni-klee-make-sym", cl::init(true),
                                cl::desc("Use uniklee_make_symbolic"));

cl::opt<int> CrashLoc("crash-loc", cl::init(0),
                      cl::desc("Crash line in assembly.ll"));
cl::opt<bool> UseExtractFix("use-extractfix", cl::init(false),
                            cl::desc("Use extractfix"));

cl::opt<int> MaxLazyDepth("max-lazy-depth", cl::init(3),
                          cl::desc("Max depth for lazy initialization"));

cl::opt<bool> PatchFiltering("patch-filtering", cl::init(false),
                             cl::desc("Use patch filtering"));
} // namespace

namespace klee {
cl::opt<std::string> UniKleeReplay("uni-klee-replay",
                                   cl::desc("Replay with symbolic input"),
                                   cl::init(""));
cl::opt<std::string>
    TargetFunction("target-function",
                   cl::desc("Execute until target function (default=\"\")"),
                   cl::init(""));
RNG theRNG;
} // namespace klee

const char *Executor::TerminateReasonNames[] = {
    [Abort] = "abort",
    [Assert] = "assert",
    [BadVectorAccess] = "bad_vector_access",
    [Exec] = "exec",
    [External] = "external",
    [Free] = "free",
    [Model] = "model",
    [Overflow] = "overflow",
    [Ptr] = "ptr",
    [ReadOnly] = "readonly",
    [ReportError] = "reporterror",
    [User] = "user",
    [Unhandled] = "xxx",
};

bool hasPostfix(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return false;
  }
}

Executor::StateGroup::StateGroup(Executor *executor) : executor(executor) {
  continuePopInput = false;
  maxBaseForks = -1;
  maxBaseAfterForks = -1;
  maxCrashTestForks = -1;
  inputGenerationTime = time::getWallTime();
  if (MaxForksPerPhases.size() < 3) {
    SPDLOG_WARN("max-forks-per-phases size is less than 3 - not set");
  } else {
    if (MaxForksPerPhases[0] > 0)
      maxBaseForks = MaxForksPerPhases[0];
    if (MaxForksPerPhases[1] > 0)
      maxBaseAfterForks = MaxForksPerPhases[1];
    if (MaxForksPerPhases[2] > 0)
      maxCrashTestForks = MaxForksPerPhases[2];
  }
};

Executor::StateGroup::~StateGroup() {
  for (auto &patchState : patchStates) {
    delete patchState.second;
  }
}

void Executor::StateGroup::addPatchState(ExecutionState *state,
                                         int baseStateID) {
  if (!state->metaData.isType(StateType::ST_patch)) {
    SPDLOG_WARN("[state {}] should be a patch state but {}", state->getID(),
                state->metaData.getStateType());
    return;
  }
  if (executor->executionType == ExecutionType::MakeSnapshot) {
    return;
  }
  if (patchStates.count(baseStateID) == 0) {
    patchStates[baseStateID] = new std::vector<ExecutionState *>();
  }
  patchStates[baseStateID]->push_back(state);
  executor->pauseState(*state);
}

void Executor::StateGroup::addAndPauseState(ExecutionState *state) {
  if (state->metaData.isType(StateType::ST_patch)) {
    return;
  }
  if (state->metaData.isType(StateType::ST_crash_test)) {
    SPDLOG_DEBUG("[state {}] [anp] skip crash test state", state->getID());
  }
  if (state->metaData.getIsPaused()) {
    SPDLOG_DEBUG("[state {}] [anp] skip paused state", state->getID());
  }
  executor->pauseState(*state);
}

// Pause all running states: base or base_after
void Executor::StateGroup::pauseAll(ExecutionState *state) {
  for (ExecutionState *s : executor->states) {
    if (s != state && s->ptreeNode->validChildren > 0) {
      addAndPauseState(s);
    }
  }
}

// Resume paused states: base_after, base
void Executor::StateGroup::continueAll() {
  StateType st = StateType::ST_base;
  for (ExecutionState *s : executor->states) {
    if (s->metaData.isType(ST_base_after)) {
      st = ST_base_after;
      break;
    }
  }
  SPDLOG_DEBUG("[pig] continueAll: {}", st);
  for (ExecutionState *s : executor->states) {
    if (s->metaData.isType(st) && s->metaData.getIsPaused() &&
        s->ptreeNode->validChildren == 0) {
      executor->continueState(*s);
    }
  }
}

bool Executor::StateGroup::shouldPopInput() {
  if (executor->isAllStatePaused() && executor->addedStates.empty()) {
    SPDLOG_DEBUG("[pig] pop due to all states are paused, states {} + added {} "
                 "- removed {} + paused {} - continued {}",
                 executor->states.size(), executor->addedStates.size(),
                 executor->removedStates.size(), executor->pausedStates.size(),
                 executor->continuedStates.size());
    return true;
  }
  if (inputQueue.size() >= 16) {
    SPDLOG_DEBUG("[pig] pop due to inputQueue size {} >= 16",
                 inputQueue.size());
    return true;
  }
  if (inputQueue.empty()) {
    SPDLOG_TRACE("[pig] do not pop due to inputQueue empty");
    return false;
  }
  // If crash_test exists, do not pop
  for (ExecutionState *s : executor->states) {
    if (s->metaData.isType(StateType::ST_crash_test)) {
      SPDLOG_TRACE("[pig] do not pop due to crash_test [state {}] {}",
                   s->getID(), inputQueue.size());
      return false;
    }
  }
  // If pop started, continue pop
  if (continuePopInput) {
    SPDLOG_DEBUG("[pig] pop due to continuePopInput {}", inputQueue.size());
    return true;
  }
  if (checkInputGenerationTime()) {
    SPDLOG_DEBUG("[pig] pop due to inputGenerationTime {}", inputQueue.size());
    inputGenerationTime = time::getWallTime();
    return true;
  }
  SPDLOG_TRACE("[pig] do not pop?");
  return false;
}

void Executor::StateGroup::pushInput(ExecutionState *state) {
  state->setCrashID();
  std::vector<ExecutionState *>::iterator it = std::find(
      executor->addedStates.begin(), executor->addedStates.end(), state);
  state->pc = state->prevPC;
  bool add2removed = false;
  if (it != executor->addedStates.end()) {
    // auto rit = std::remove(executor->addedStates.begin(),
    //                        executor->addedStates.end(), state);
    // executor->addedStates.erase(rit);
    add2removed = true;
  }
  doNotDeleteStates.insert(state);
  inputQueue.push(state);
  SPDLOG_INFO(
      "[state {}] [crash {}] state: push to input queue {}, add2removed {}",
      state->getID(), state->metaData.getCrashID(), inputQueue.size(),
      add2removed);
}

void Executor::StateGroup::popInput() {
  // Check if all crash_test is done
  if (inputQueue.empty()) {
    // This should not happen
    SPDLOG_DEBUG("[pig] popInput: inputQueue {}", inputQueue.size());
    continuePopInput = false;
    return;
  }
  continuePopInput = true;
  ExecutionState *state = inputQueue.front();
  inputQueue.pop();
  if (inputQueue.empty()) {
    continuePopInput = false;
    inputGenerationTime = time::getWallTime();
  }
  handleCrashState(state);
  SPDLOG_DEBUG("[state {}] Crash state: pop from input queue {}",
               state->getID(), inputQueue.size());
  doNotDeleteStates.erase(state);
  // Remove from removedStates, if exists
}

void Executor::StateGroup::handleCrashState(ExecutionState *state) {
  std::string constraintsStr;
  llvm::raw_string_ostream os(constraintsStr);
  ExprPPrinter::printConstraints(os, state->shadowConstraints);
  SPDLOG_DEBUG(
      "[state {}] handle crash state {} (isCrash: {}) (shadow m: {}, c: {}) {}",
      state->getID(), state->getCrashID(), state->metaData.getIsCrash(),
      state->shadowMemory.size(), state->shadowConstraints.size(), os.str());
  std::string constraintsStr2;
  llvm::raw_string_ostream os2(constraintsStr2);
  ExprPPrinter::printConstraints(os2, state->constraints);
  SPDLOG_DEBUG("[state {}] constraints c: {} {}", state->getID(),
               state->constraints.size(), os2.str());
  pauseAll(state);
  std::vector<ExecutionState *> *currentPatchStates =
      getPatchStates(state->getID());
  if (currentPatchStates == nullptr) {
    SPDLOG_WARN("[state {}] currentPatchStates is nullptr", state->getID());
    return;
  }
  for (ExecutionState *s : *currentPatchStates) {
    ExecutionState *ns = executor->cloneState(s);
    ns->metaData.handleCrashState(&state->metaData);
    klee_log_data(
        fmt::format("[fork-map] [merge] [state {}] -> [state {}] [patch {}]",
                    state->getID(), ns->getID(), ns->metaData.getPatchID()));
    addForkParent(ns);
    // addFork(state, ns);
    std::vector<ref<Expr>> shadowConstraints;
    for (auto &shadowConstraint : state->shadowConstraints) {
      shadowConstraints.push_back(shadowConstraint);
    }
    for (auto &op : state->shadowMemory) {
      if (op.first && op.second) {
        executor->bindObjectInState(*ns, op.first, false, op.second);
        SPDLOG_INFO("[state {}] [shadow] [mo name={} addr={}]", state->getID(),
                    op.first->name, op.first->address);
        //        const ObjectState *os = ns->addressSpace.findObject(op.first);
        //        std::vector<ref<ReadExpr>> reads;
        //        ReadExprCollector rec(op.second, reads);
        //        ReadExprChecker checker(op.second);
        //        for (size_t i = 0; i < shadowConstraints.size(); i++) {
        //          // Collect ReadExpr that contains op
        //          ref<Expr> shadowConstraint = shadowConstraints[i];
        //          ref<Expr> constraint = rec.getReads(shadowConstraint);
        //          for (auto &read : reads) {
        //            SPDLOG_DEBUG("[state {}] [shadow] [read={}] [mo name={}
        //            addr={}] "
        //                         "[constraint={}]",
        //                         state->getID(), read->str(), op.first->name,
        //                         op.first->address, constraint->str());
        //          }
        //          if (checker.containsReadFromObject(shadowConstraint)) {
        //            ReadExprReplacer rep(op.second, os);
        //            ref<Expr> fixedConstraint =
        //            rep.replcaeReadExpr(shadowConstraint);
        //            SPDLOG_DEBUG("[state {}] [shadow] [mo name={} addr={}] "
        //                         "[constraint={}] [fixed={}]",
        //                         state->getID(), op.first->name,
        //                         op.first->address, shadowConstraint->str(),
        //                         fixedConstraint->str());
        //            shadowConstraints[i] = fixedConstraint;
        //          }
        //        }
      }
    }
    std::string constraintsStr3;
    llvm::raw_string_ostream os3(constraintsStr3);
    ExprPPrinter::printConstraints(os3, ns->constraints);
    SPDLOG_DEBUG("[state {}] [before] [constraints c: {}] {}", ns->getID(),
                 ns->constraints.size(), os3.str());
    for (auto &shadowConstraint : shadowConstraints) {
      ns->addConstraint(shadowConstraint);
      SPDLOG_DEBUG("[state {}] [add-shadow] [constraint={}]", ns->getID(),
                   shadowConstraint->str());
    }
    std::string constraintsStr4;
    llvm::raw_string_ostream os4(constraintsStr4);
    ExprPPrinter::printConstraints(os4, ns->constraints);
    SPDLOG_DEBUG("[state {}] [after] [constraints c: {}] {}", ns->getID(),
                 ns->constraints.size(), os4.str());
  }
}

std::vector<ExecutionState *> *
Executor::StateGroup::getPatchStates(int stateID) {
  // 1. Find stateID's fork parent
  // std::pair<int, StateType> key = {, StateType::ST_base_after};
  if (forkMap.count(stateID) == 0) {
    SPDLOG_WARN("[state {}] forkMap miss", stateID);
    return nullptr;
  }
  int parentState = forkMap[stateID];
  // 2. Return patchStates[parentState]
  if (patchStates.count(parentState) == 0) {
    SPDLOG_WARN("[state {}] patchStates miss", stateID);
    return nullptr;
  }
  return patchStates[parentState];
}

std::size_t Executor::StateGroup::getPatchedStatesSize() {
  std::size_t size = 0;
  for (auto &patchState : patchStates) {
    size += patchState.second->size();
  }
  return size;
}

int Executor::StateGroup::getMaxForkValue(StateType type) {
  switch (type) {
  case StateType::ST_base:
    return maxBaseForks;
  case StateType::ST_base_after:
    return maxBaseAfterForks;
  case StateType::ST_crash_test:
    return maxCrashTestForks;
  default:
    return -1;
  }
}

void Executor::StateGroup::addForkParent(ExecutionState *state) {
  auto parentState = state->getID();
  // std::make_pair(state->getID(), state->metaData.getStateType());
  stateMetaDataMap[state->getID()] = state->metaData;
  forkMap[parentState] = parentState;
  forkCountMap[parentState] = 1;
  klee_log_data(fmt::format("[fork-map] [fork-parent] [state {}] [type {}]",
                            state->getID(), state->metaData.getStateType()));
}

ExecutionState *Executor::StateGroup::getForkParent(ExecutionState *state) {
  if (forkMap.count(state->getID()) == 0) {
    SPDLOG_DEBUG("[state {}] {} forkMap miss", state->getID(),
                 state->metaData.getStateType());
    return nullptr;
  }
  int baseId = forkMap[state->getID()];
  for (ExecutionState *s : executor->states) {
    if (s->getID() == baseId) {
      return s;
    }
  }
  return nullptr;
}

bool Executor::StateGroup::addFork(ExecutionState *state,
                                   ExecutionState *forkedState) {
  if (forkMap.count(state->getID()) == 0) {
    SPDLOG_DEBUG("[state {}] {} forkMap miss... failed to addFork",
                 state->getID(), state->metaData.getStateType());
    return false;
  }
  if (state->metaData.isType(StateType::ST_crash_test)) {
    if (state->metaData.getPatchID() == 0) {
      SPDLOG_WARN("[state {}] is crash test with patchID is 0, but forked to "
                  "[state {}]",
                  state->getID(), forkedState->getID());
    }
  }
  auto baseState = forkMap[state->getID()];
  forkMap[forkedState->getID()] = baseState;
  stateMetaDataMap[forkedState->getID()] = forkedState->metaData;
  if (forkCountMap.count(baseState) == 0) {
    return false;
  }
  forkCountMap[baseState]++;
  klee_log_data(fmt::format(
      "[fork-map] [fork] [state {}] [type {}] [base {}] [base-type "
      "{}] [state {}] [type {}] "
      "[fork-count {}/{}]",
      state->getID(), state->metaData.getStateType(), baseState,
      stateMetaDataMap[baseState].getStateType(), forkedState->getID(),
      forkedState->metaData.getStateType(), forkCountMap[baseState],
      getMaxForkValue(state->metaData.getStateType())));
  return true;
}

bool Executor::StateGroup::checkMaxFork(ExecutionState *state, int add,
                                        int &remain) {
  remain = add;
  int maxFork = getMaxForkValue(state->metaData.getStateType());
  if (maxFork < 0)
    return false;
  if (forkMap.count(state->getID()) == 0) {
    SPDLOG_INFO("[state {}] {} forkMap miss", state->getID(),
                state->metaData.getStateType());
    return false;
  }
  auto baseState = forkMap[state->getID()];
  if (forkCountMap.count(baseState) == 0) {
    SPDLOG_INFO("[state {}] {} forkCountMap miss", state->getID(),
                state->metaData.getStateType());
    return false;
  }
  if (MaxForksPerPhases.size() == 4) {
    // Print fork count
    SPDLOG_INFO("[state {}] {} forkCount: {}", state->getID(),
                state->metaData.getStateType(), forkCountMap[baseState]);
    for (auto &forkCount : forkCountMap) {
      SPDLOG_INFO("[state {}] forkCount: {}", forkCount.first,
                  forkCount.second);
    }
  }
  int current = forkCountMap[baseState];
  if (current + add - 1 > maxFork) {
    remain = maxFork - current + 1;
    if (remain <= 0)
      remain = 1;
    SPDLOG_INFO(
        "[max-fork] [state {}] [type {}] [base {}] [fork-count {}/{}] [add {}] "
        "[remain {}]",
        state->getID(), state->metaData.getStateType(), baseState, current,
        maxFork, add, remain);
    return true;
  }
  return false;
}

int Executor::StateGroup::getRemainFork(ExecutionState *state) {
  int maxFork = getMaxForkValue(state->metaData.getStateType());
  int remain;
  bool toFork = checkMaxFork(state, maxFork, remain);
  return remain;
}

bool Executor::StateGroup::checkInputGenerationTime() {
  time::Span elapsed = time::getWallTime() - inputGenerationTime;
  if (elapsed > time::minutes(20)) {
    SPDLOG_INFO("[pig] checkInputGenerationTime: elapsed {} seconds",
                elapsed.toSeconds());
    return true;
  }
  return false;
}

bool Executor::StateGroup::checkDeleteState(ExecutionState *state) {
  return doNotDeleteStates.count(state) == 0;
}

Executor::Executor(LLVMContext &ctx, const InterpreterOptions &opts,
                   InterpreterHandler *ih)
    : Interpreter(opts), interpreterHandler(ih), searcher(0), cfg(0),
      externalDispatcher(new ExternalDispatcher(ctx)), statsTracker(0),
      pathWriter(0), symPathWriter(0), specialFunctionHandler(0),
      processTree(0), replayKTest(0), replayPath(0), usingSeeds(0),
      atMemoryLimit(false), inhibitForking(false), haltExecution(false),
      ivcEnabled(false), debugLogBuffer(debugBufferString), fromSnapshot(false),
      executionType(ExecutionType::Normal), baseSnapshot(nullptr),
      taintMarker(false), stateGroup(this) {

  const time::Span maxCoreSolverTime(MaxCoreSolverTime);
  maxInstructionTime = time::Span(MaxInstructionTime);
  coreSolverTimeout = maxCoreSolverTime && maxInstructionTime
                          ? std::min(maxCoreSolverTime, maxInstructionTime)
                          : std::max(maxCoreSolverTime, maxInstructionTime);

  if (coreSolverTimeout)
    UseForkedCoreSolver = true;
  Solver *coreSolver = klee::createCoreSolver(CoreSolverToUse);
  if (!coreSolver) {
    klee_error("Failed to create core solver\n");
  }

  Solver *solver = constructSolverChain(
      coreSolver,
      interpreterHandler->getOutputFilename(ALL_QUERIES_SMT2_FILE_NAME),
      interpreterHandler->getOutputFilename(SOLVER_QUERIES_SMT2_FILE_NAME),
      interpreterHandler->getOutputFilename(ALL_QUERIES_KQUERY_FILE_NAME),
      interpreterHandler->getOutputFilename(SOLVER_QUERIES_KQUERY_FILE_NAME));

  this->solver = new TimingSolver(solver, EqualitySubstitution);
  memory = new MemoryManager(&arrayCache);

  initializeSearchOptions();
  // initializeSlicing(this);

  if (OnlyOutputStatesCoveringNew && !StatsTracker::useIStats())
    klee_error("To use --only-output-states-covering-new, you need to enable "
               "--output-istats.");

  if (DebugPrintInstructions.isSet(FILE_ALL) ||
      DebugPrintInstructions.isSet(FILE_COMPACT) ||
      DebugPrintInstructions.isSet(FILE_SRC)) {
    std::string debug_file_name =
        interpreterHandler->getOutputFilename("instructions.txt");
    std::string error;
#ifdef HAVE_ZLIB_H
    if (!DebugCompressInstructions) {
#endif
      debugInstFile = klee_open_output_file(debug_file_name, error);
#ifdef HAVE_ZLIB_H
    } else {
      debug_file_name.append(".gz");
      debugInstFile = klee_open_compressed_output_file(debug_file_name, error);
    }
#endif
    if (!debugInstFile) {
      klee_error("Could not open file %s : %s", debug_file_name.c_str(),
                 error.c_str());
    }
  }
}

llvm::Module *
Executor::setModule(std::vector<std::unique_ptr<llvm::Module>> &modules,
                    const ModuleOptions &opts) {
  assert(!kmodule && !modules.empty() &&
         "can only register one module"); // XXX gross

  kmodule = std::unique_ptr<KModule>(new KModule());

  // Preparing the final module happens in multiple stages

  // Link with KLEE intrinsics library before running any optimizations
  SmallString<128> LibPath(opts.LibraryDir);
  llvm::sys::path::append(LibPath, "libkleeRuntimeIntrinsic.bca");
  std::string error;
  if (!klee::loadFile(LibPath.c_str(), modules[0]->getContext(), modules,
                      error)) {
    klee_error("Could not load KLEE intrinsic file %s", LibPath.c_str());
  }
  // #ifdef ENABLE_DG
  // std::unique_ptr<Module> slicedM = slice(std::move(modules[0]));
  // if (slicedM) {
  //   slicingInfo->load(slicedM.get());
  // }
  // #endif

  // 1.) Link the modules together
  while (kmodule->link(modules, opts.EntryPoint)) {
    // 2.) Apply different instrumentation
    kmodule->instrument(opts);
  }

  // Create a list of functions that should be preserved if used
  std::vector<const char *> preservedFunctions;
  specialFunctionHandler = new SpecialFunctionHandler(*this);
  specialFunctionHandler->prepare(preservedFunctions);

  preservedFunctions.push_back(opts.EntryPoint.c_str());

  // Preserve the free-standing library calls
  preservedFunctions.push_back("memset");
  preservedFunctions.push_back("memcpy");
  preservedFunctions.push_back("memcmp");
  preservedFunctions.push_back("memmove");

  kmodule->optimiseAndPrepare(opts, preservedFunctions);

  // 4.) Manifest the module
  kmodule->manifest(interpreterHandler, StatsTracker::useStatistics());
  specialFunctionHandler->bind();
  if (StatsTracker::useStatistics() || userSearcherRequiresMD2U()) {
    statsTracker = new StatsTracker(
        *this, interpreterHandler->getOutputFilename("assembly.ll"),
        userSearcherRequiresMD2U());
  }

  // Initialize the context.
  DataLayout *TD = kmodule->targetData.get();
  Context::initialize(TD->isLittleEndian(),
                      (Expr::Width)TD->getPointerSizeInBits());

  Module *finalModule = kmodule->module.get();
  finder.processModule(*finalModule);

  if (TargetFunction.empty() != true) {
    if (TargetFunction == "main") {
      targetFunctionAddr =
          finalModule->getFunction("__klee_posix_wrapped_main");
    } else {
      targetFunctionAddr = finalModule->getFunction(TargetFunction);
    }
    patchLocationAddr = finalModule->getFunction("__cpr_choice");
  }

  return finalModule;
}

Executor::~Executor() {
  delete memory;
  delete externalDispatcher;
  delete processTree;
  delete specialFunctionHandler;
  delete statsTracker;
  delete solver;
  while (!timers.empty()) {
    delete timers.back();
    timers.pop_back();
  }
}

/***/

void Executor::initializeGlobalObject(ExecutionState &state, ObjectState *os,
                                      const Constant *c, unsigned offset) {
  const auto targetData = kmodule->targetData.get();
  if (const ConstantVector *cp = dyn_cast<ConstantVector>(c)) {
    unsigned elementSize =
        targetData->getTypeStoreSize(cp->getType()->getElementType());
    for (unsigned i = 0, e = cp->getNumOperands(); i != e; ++i)
      initializeGlobalObject(state, os, cp->getOperand(i),
                             offset + i * elementSize);
  } else if (isa<ConstantAggregateZero>(c)) {
    unsigned i, size = targetData->getTypeStoreSize(c->getType());
    for (i = 0; i < size; i++)
      os->write8(offset + i, (uint8_t)0);
  } else if (const ConstantArray *ca = dyn_cast<ConstantArray>(c)) {
    unsigned elementSize =
        targetData->getTypeStoreSize(ca->getType()->getElementType());
    for (unsigned i = 0, e = ca->getNumOperands(); i != e; ++i)
      initializeGlobalObject(state, os, ca->getOperand(i),
                             offset + i * elementSize);
  } else if (const ConstantStruct *cs = dyn_cast<ConstantStruct>(c)) {
    const StructLayout *sl =
        targetData->getStructLayout(cast<StructType>(cs->getType()));
    for (unsigned i = 0, e = cs->getNumOperands(); i != e; ++i)
      initializeGlobalObject(state, os, cs->getOperand(i),
                             offset + sl->getElementOffset(i));
  } else if (const ConstantDataSequential *cds =
                 dyn_cast<ConstantDataSequential>(c)) {
    unsigned elementSize = targetData->getTypeStoreSize(cds->getElementType());
    for (unsigned i = 0, e = cds->getNumElements(); i != e; ++i)
      initializeGlobalObject(state, os, cds->getElementAsConstant(i),
                             offset + i * elementSize);
  } else if (!isa<UndefValue>(c)) {
    unsigned StoreBits = targetData->getTypeStoreSizeInBits(c->getType());
    ref<ConstantExpr> C = evalConstant(c);

    // Extend the constant if necessary;
    assert(StoreBits >= C->getWidth() && "Invalid store size!");
    if (StoreBits > C->getWidth())
      C = C->ZExt(StoreBits);

    os->write(offset, C);
  }
}

MemoryObject *Executor::addExternalObject(ExecutionState &state, void *addr,
                                          unsigned size, bool isReadOnly,
                                          std::string name) {
  auto mo = memory->allocateFixed(reinterpret_cast<std::uint64_t>(addr), size,
                                  nullptr);
  ObjectState *os = bindObjectInState(state, mo, false);
  mo->setName(name);
  externalObjects.insert(std::make_pair(name, (uint64_t)addr));
  for (unsigned i = 0; i < size; i++)
    os->write8(i, ((uint8_t *)addr)[i]);
  if (isReadOnly)
    os->setReadOnly(true);
  return mo;
}

extern void *__dso_handle __attribute__((__weak__));

void Executor::initializeGlobals(ExecutionState &state) {
  Module *m = kmodule->module.get();

  if (m->getModuleInlineAsm() != "")
    klee_warning("executable has module level assembly (ignoring)");
  // represent function globals using the address of the actual llvm function
  // object. given that we use malloc to allocate memory in states this also
  // ensures that we won't conflict. we don't need to allocate a memory object
  // since reading/writing via a function pointer is unsupported anyway.
  for (Module::iterator i = m->begin(), ie = m->end(); i != ie; ++i) {
    Function *f = &*i;
    ref<ConstantExpr> addr(0);

    // If the symbol has external weak linkage then it is implicitly
    // not defined in this module; if it isn't resolvable then it
    // should be null.
    if (f->hasExternalWeakLinkage() &&
        !externalDispatcher->resolveSymbol(f->getName().str())) {
      addr = Expr::createPointer(0);
    } else {
      addr = Expr::createPointer(reinterpret_cast<std::uint64_t>(f));
      legalFunctions.insert(reinterpret_cast<std::uint64_t>(f));
    }

    globalAddresses.insert(std::make_pair(f, addr));
  }

#ifndef WINDOWS
  int *errno_addr = getErrnoLocation(state);

  MemoryObject *errnoObj = addExternalObject(
      state, (void *)errno_addr, sizeof *errno_addr, false, "ext_errno_loc");
  // Copy values from and to program space explicitly
  errnoObj->isUserSpecified = true;
#endif

  // Disabled, we don't want to promote use of live externals.
#ifdef HAVE_CTYPE_EXTERNALS
#ifndef WINDOWS
#ifndef DARWIN
  /* from /usr/include/ctype.h:
       These point into arrays of 384, so they can be indexed by any `unsigned
       char' value [0,255]; by EOF (-1); or by any `signed char' value
       [-128,-1).  ISO C requires that the ctype functions work for `unsigned */
  const uint16_t **addr = __ctype_b_loc();
  addExternalObject(state, const_cast<uint16_t *>(*addr - 128),
                    384 * sizeof **addr, true, "ext__ctype_b_loc");
  addExternalObject(state, addr, sizeof(*addr), true, "ext__ctype_b_loc_ptr");

  const int32_t **lower_addr = __ctype_tolower_loc();
  addExternalObject(state, const_cast<int32_t *>(*lower_addr - 128),
                    384 * sizeof **lower_addr, true, "ext__ctype_tolower_loc");
  addExternalObject(state, lower_addr, sizeof(*lower_addr), true,
                    "ext__ctype_tolower_loc_ptr");

  const int32_t **upper_addr = __ctype_toupper_loc();
  addExternalObject(state, const_cast<int32_t *>(*upper_addr - 128),
                    384 * sizeof **upper_addr, true, "ext__ctype_toupper_loc");
  addExternalObject(state, upper_addr, sizeof(*upper_addr), true,
                    "ext__ctype_toupper_loc_ptr");
#endif
#endif
#endif

  // allocate and initialize globals, done in two passes since we may
  // need address of a global in order to initialize some other one.

  // allocate memory objects for all globals
  for (Module::const_global_iterator i = m->global_begin(), e = m->global_end();
       i != e; ++i) {
    const GlobalVariable *v = &*i;
    size_t globalObjectAlignment = getAllocationAlignment(v);
    if (i->isDeclaration()) {
      // FIXME: We have no general way of handling unknown external
      // symbols. If we really cared about making external stuff work
      // better we could support user definition, or use the EXE style
      // hack where we check the object file information.

      Type *ty = i->getType()->getElementType();
      uint64_t size = 0;
      if (ty->isSized()) {
        size = kmodule->targetData->getTypeStoreSize(ty);
      } else {
        klee_warning("Type for %.*s is not sized", (int)i->getName().size(),
                     i->getName().data());
      }

      // XXX - DWD - hardcode some things until we decide how to fix.
#ifndef WINDOWS
      if (i->getName() == "_ZTVN10__cxxabiv117__class_type_infoE") {
        size = 0x2C;
      } else if (i->getName() == "_ZTVN10__cxxabiv120__si_class_type_infoE") {
        size = 0x2C;
      } else if (i->getName() == "_ZTVN10__cxxabiv121__vmi_class_type_infoE") {
        size = 0x2C;
      }
#endif

      if (size == 0) {
        klee_warning("Unable to find size for global variable: %.*s (use will "
                     "result in out of bounds access)",
                     (int)i->getName().size(), i->getName().data());
      }

      MemoryObject *mo = memory->allocate(size, /*isLocal=*/false,
                                          /*isGlobal=*/true, /*allocSite=*/v,
                                          /*alignment=*/globalObjectAlignment);
      ObjectState *os = bindObjectInState(state, mo, false);
      std::string gvName = v->getName().str();
      mo->setName(gvName); // global variable name is unique
      mo->type = ty;
      globalObjects.insert(std::make_pair(v, mo));
      globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));
      globalNames.insert(std::make_pair(gvName, v));
      // Program already running = object already initialized.  Read
      // concrete value and write it to our copy.
      if (size) {
        void *addr;
        if (i->getName() == "__dso_handle") {
          addr = &__dso_handle; // wtf ?
        } else {
          addr = externalDispatcher->resolveSymbol(i->getName().str());
        }
        if (!addr)
          klee_error("unable to load symbol(%s) while initializing globals.",
                     i->getName().data());

        for (unsigned offset = 0; offset < mo->size; offset++)
          os->write8(offset, ((unsigned char *)addr)[offset]);
      }
    } else {
      Type *ty = i->getType()->getElementType();
      uint64_t size = kmodule->targetData->getTypeStoreSize(ty);
      MemoryObject *mo = memory->allocate(size, /*isLocal=*/false,
                                          /*isGlobal=*/true, /*allocSite=*/v,
                                          /*alignment=*/globalObjectAlignment);
      if (!mo)
        llvm::report_fatal_error("out of memory");
      std::string gvName = v->getName().str();
      mo->setName(gvName); // global variable name is unique
      mo->type = ty;
      ObjectState *os = bindObjectInState(state, mo, false);
      globalObjects.insert(std::make_pair(v, mo));
      globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));
      globalNames.insert(std::make_pair(gvName, v));

      if (!i->hasInitializer())
        os->initializeToRandom();
    }
  }

  // link aliases to their definitions (if bound)
  for (Module::alias_iterator i = m->alias_begin(), ie = m->alias_end();
       i != ie; ++i) {
    // Map the alias to its aliasee's address. This works because we have
    // addresses for everything, even undefined functions.
    globalAddresses.insert(std::make_pair(&*i, evalConstant(i->getAliasee())));
  }

  // once all objects are allocated, do the actual initialization
  for (Module::const_global_iterator i = m->global_begin(), e = m->global_end();
       i != e; ++i) {
    if (i->hasInitializer()) {
      const GlobalVariable *v = &*i;
      MemoryObject *mo = globalObjects.find(v)->second;
      const ObjectState *os = state.addressSpace.findObject(mo);
      assert(os);
      ObjectState *wos = state.addressSpace.getWriteable(mo, os);
      specialFunctionHandler->trackMemory(state, v->getType(),
                                          mo->getBaseExpr(), mo->getSizeExpr());
      initializeGlobalObject(state, wos, i->getInitializer(), 0);
      // if (i->isConstant())
      //   wos->setReadOnly(true);
    }
  }
}

void Executor::initializeUnderConstrained(ExecutionState &state) {
  Module *m = kmodule->module.get();

  if (m->getModuleInlineAsm() != "")
    klee_warning("executable has module level assembly (ignoring)");
  // represent function globals using the address of the actual llvm function
  // object. given that we use malloc to allocate memory in states this also
  // ensures that we won't conflict. we don't need to allocate a memory object
  // since reading/writing via a function pointer is unsupported anyway.
  for (Module::iterator i = m->begin(), ie = m->end(); i != ie; ++i) {
    Function *f = &*i;
    ref<ConstantExpr> addr(0);

    // If the symbol has external weak linkage then it is implicitly
    // not defined in this module; if it isn't resolvable then it
    // should be null.
    if (f->hasExternalWeakLinkage() &&
        !externalDispatcher->resolveSymbol(f->getName().str())) {
      addr = Expr::createPointer(0);
    } else {
      addr = Expr::createPointer(reinterpret_cast<std::uint64_t>(f));
      legalFunctions.insert(reinterpret_cast<std::uint64_t>(f));
    }

    globalAddresses.insert(std::make_pair(f, addr));
  }

#ifndef WINDOWS
  int *errno_addr = getErrnoLocation(state);

  MemoryObject *errnoObj = addExternalObject(
      state, (void *)errno_addr, sizeof *errno_addr, false, "ext_errno_loc");
  // Copy values from and to program space explicitly
  errnoObj->isUserSpecified = true;
#endif

  // Disabled, we don't want to promote use of live externals.
#ifdef HAVE_CTYPE_EXTERNALS
#ifndef WINDOWS
#ifndef DARWIN
  /* from /usr/include/ctype.h:
       These point into arrays of 384, so they can be indexed by any `unsigned
       char' value [0,255]; by EOF (-1); or by any `signed char' value
       [-128,-1).  ISO C requires that the ctype functions work for `unsigned */
  const uint16_t **addr = __ctype_b_loc();
  addExternalObject(state, const_cast<uint16_t *>(*addr - 128),
                    384 * sizeof **addr, true, "ext__ctype_b_loc");
  addExternalObject(state, addr, sizeof(*addr), true, "ext__ctype_b_loc_ptr");

  const int32_t **lower_addr = __ctype_tolower_loc();
  addExternalObject(state, const_cast<int32_t *>(*lower_addr - 128),
                    384 * sizeof **lower_addr, true, "ext__ctype_tolower_loc");
  addExternalObject(state, lower_addr, sizeof(*lower_addr), true,
                    "ext__ctype_tolower_loc_ptr");

  const int32_t **upper_addr = __ctype_toupper_loc();
  addExternalObject(state, const_cast<int32_t *>(*upper_addr - 128),
                    384 * sizeof **upper_addr, true, "ext__ctype_toupper_loc");
  addExternalObject(state, upper_addr, sizeof(*upper_addr), true,
                    "ext__ctype_toupper_loc_ptr");
#endif
#endif
#endif

  // allocate and initialize globals, done in two passes since we may
  // need address of a global in order to initialize some other one.

  // allocate memory objects for all globals
  for (Module::const_global_iterator i = m->global_begin(), e = m->global_end();
       i != e; ++i) {
    const GlobalVariable *v = &*i;
    size_t globalObjectAlignment = getAllocationAlignment(v);
    if (i->isDeclaration()) {
      // FIXME: We have no general way of handling unknown external
      // symbols. If we really cared about making external stuff work
      // better we could support user definition, or use the EXE style
      // hack where we check the object file information.

      Type *ty = i->getType()->getElementType();
      uint64_t size = 0;
      if (ty->isSized()) {
        size = kmodule->targetData->getTypeStoreSize(ty);
      } else {
        klee_warning("Type for %.*s is not sized", (int)i->getName().size(),
                     i->getName().data());
      }

      // XXX - DWD - hardcode some things until we decide how to fix.
#ifndef WINDOWS
      if (i->getName() == "_ZTVN10__cxxabiv117__class_type_infoE") {
        size = 0x2C;
      } else if (i->getName() == "_ZTVN10__cxxabiv120__si_class_type_infoE") {
        size = 0x2C;
      } else if (i->getName() == "_ZTVN10__cxxabiv121__vmi_class_type_infoE") {
        size = 0x2C;
      }
#endif

      if (size == 0) {
        klee_warning("Unable to find size for global variable: %.*s (use will "
                     "result in out of bounds access)",
                     (int)i->getName().size(), i->getName().data());
      }

      MemoryObject *mo = memory->allocate(size, /*isLocal=*/false,
                                          /*isGlobal=*/true, /*allocSite=*/v,
                                          /*alignment=*/globalObjectAlignment);
      ObjectState *os = bindObjectInState(state, mo, false);
      std::string gvName = v->getName().str();
      mo->setName(gvName); // global variable name is unique
      mo->type = ty;
      globalObjects.insert(std::make_pair(v, mo));
      globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));
      globalNames.insert(std::make_pair(gvName, v));
      // Program already running = object already initialized.  Read
      // concrete value and write it to our copy.
      if (size) {
        void *addr;
        if (i->getName() == "__dso_handle") {
          addr = &__dso_handle; // wtf ?
        } else {
          addr = externalDispatcher->resolveSymbol(i->getName().str());
        }
        if (!addr)
          klee_error("unable to load symbol(%s) while initializing globals.",
                     i->getName().data());

        for (unsigned offset = 0; offset < mo->size; offset++)
          os->write8(offset, ((unsigned char *)addr)[offset]);
      }
    } else {
      Type *ty = i->getType()->getElementType();
      uint64_t size = kmodule->targetData->getTypeStoreSize(ty);
      MemoryObject *mo = memory->allocate(size, /*isLocal=*/false,
                                          /*isGlobal=*/true, /*allocSite=*/v,
                                          /*alignment=*/globalObjectAlignment);
      if (!mo)
        llvm::report_fatal_error("out of memory");
      std::string gvName = v->getName().str();
      mo->setName(gvName); // global variable name is unique
      mo->type = ty;
      ObjectState *os = bindObjectInState(state, mo, false);
      globalObjects.insert(std::make_pair(v, mo));
      globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));
      globalNames.insert(std::make_pair(gvName, v));

      if (!i->hasInitializer())
        os->initializeToRandom();
    }
  }

  // link aliases to their definitions (if bound)
  for (Module::alias_iterator i = m->alias_begin(), ie = m->alias_end();
       i != ie; ++i) {
    // Map the alias to its aliasee's address. This works because we have
    // addresses for everything, even undefined functions.
    globalAddresses.insert(std::make_pair(&*i, evalConstant(i->getAliasee())));
  }

  // once all objects are allocated, do the actual initialization
  for (Module::const_global_iterator i = m->global_begin(), e = m->global_end();
       i != e; ++i) {
    if (i->hasInitializer()) {
      const GlobalVariable *v = &*i;
      MemoryObject *mo = globalObjects.find(v)->second;
      const ObjectState *os = state.addressSpace.findObject(mo);
      assert(os);
      ObjectState *wos = state.addressSpace.getWriteable(mo, os);
      initializeGlobalObject(state, wos, i->getInitializer(), 0);
      if (i->isConstant()) {
        // wos->setReadOnly(true);
      } else {
        executeMakeSymbolic(state, mo, v->getName().str());
      }
    }
  }
}

// Load functions and external objects
void Executor::initializeFunctions(ExecutionState &state) {
  Module *m = kmodule->module.get();

  if (m->getModuleInlineAsm() != "")
    klee_warning("executable has module level assembly (ignoring)");
  // represent function globals using the address of the actual llvm function
  // object. given that we use malloc to allocate memory in states this also
  // ensures that we won't conflict. we don't need to allocate a memory object
  // since reading/writing via a function pointer is unsupported anyway.
  for (Module::iterator i = m->begin(), ie = m->end(); i != ie; ++i) {
    Function *f = &*i;
    ref<ConstantExpr> addr(0);

    // If the symbol has external weak linkage then it is implicitly
    // not defined in this module; if it isn't resolvable then it
    // should be null.
    if (f->hasExternalWeakLinkage() &&
        !externalDispatcher->resolveSymbol(f->getName().str())) {
      addr = Expr::createPointer(0);
    } else {
      addr = Expr::createPointer(reinterpret_cast<std::uint64_t>(f));
      legalFunctions.insert(reinterpret_cast<std::uint64_t>(f));
    }

    globalAddresses.insert(std::make_pair(f, addr));
  }

#ifndef WINDOWS
  int *errno_addr = getErrnoLocation(state);

  MemoryObject *errnoObj = addExternalObject(
      state, (void *)errno_addr, sizeof *errno_addr, false, "errno_loc");
  // Copy values from and to program space explicitly
  errnoObj->isUserSpecified = true;
#endif

  // Disabled, we don't want to promote use of live externals.
#ifdef HAVE_CTYPE_EXTERNALS
#ifndef WINDOWS
#ifndef DARWIN
  /* from /usr/include/ctype.h:
       These point into arrays of 384, so they can be indexed by any `unsigned
       char' value [0,255]; by EOF (-1); or by any `signed char' value
       [-128,-1).  ISO C requires that the ctype functions work for `unsigned */
  const uint16_t **addr = __ctype_b_loc();
  addExternalObject(state, const_cast<uint16_t *>(*addr - 128),
                    384 * sizeof **addr, true, "__ctype_b_loc");
  addExternalObject(state, addr, sizeof(*addr), true, "__ctype_b_loc_ptr");

  const int32_t **lower_addr = __ctype_tolower_loc();
  addExternalObject(state, const_cast<int32_t *>(*lower_addr - 128),
                    384 * sizeof **lower_addr, true, "__ctype_tolower_loc");
  addExternalObject(state, lower_addr, sizeof(*lower_addr), true,
                    "__ctype_tolower_loc_ptr");

  const int32_t **upper_addr = __ctype_toupper_loc();
  addExternalObject(state, const_cast<int32_t *>(*upper_addr - 128),
                    384 * sizeof **upper_addr, true, "__ctype_toupper_loc");
  addExternalObject(state, upper_addr, sizeof(*upper_addr), true,
                    "__ctype_toupper_loc_ptr");
#endif
#endif
#endif
}

/**
 * Used in switch, indirectbr, and getValue
 */
void Executor::branch(ExecutionState &state,
                      const std::vector<ref<Expr>> &conditions,
                      std::vector<ExecutionState *> &result) {
  TimerStatIncrementer timer(stats::forkTime);
  unsigned N = conditions.size();
  assert(N);
  int remain;
  if (stateGroup.checkMaxFork(&state, N, remain) ||
      (MaxForks != ~0u && stats::forks >= MaxForks)) {
    // Random select remain
    std::vector<unsigned> remainIdx;
    for (unsigned i = 0; i < N; ++i) {
      remainIdx.push_back(i);
    }
    std::random_shuffle(remainIdx.begin(), remainIdx.end());
    std::set<unsigned> remainSet;
    for (unsigned i = 0; i < remain; ++i) {
      remainSet.insert(remainIdx[i]);
    }
    std::vector<ExecutionState *> remainStates;
    remainStates.push_back(&state);
    for (unsigned i = 1; i < remain; ++i) {
      ExecutionState *es = remainStates[theRNG.getInt32() % i];
      ExecutionState *ns = es->branch();
      addedStates.push_back(ns);
      stateGroup.addFork(&state, ns);
      es->ptreeNode->data = 0;
      std::pair<PTree::Node *, PTree::Node *> res =
          processTree->split(es->ptreeNode, ns, es);
      ns->ptreeNode = res.first;
      es->ptreeNode = res.second;
      remainStates.push_back(ns);
    }
    unsigned remainCount = 0;
    for (unsigned i = 0; i < N; ++i) {
      if (remainSet.count(i)) {
        result.push_back(remainStates[remainCount++]);
      } else {
        result.push_back(NULL);
      }
    }
  } else {
    stats::forks += N - 1;

    // XXX do proper balance or keep random?
    result.push_back(&state);
    for (unsigned i = 1; i < N; ++i) {
      ExecutionState *es = result[theRNG.getInt32() % i];
      ExecutionState *ns = es->branch();
      addedStates.push_back(ns);
      stateGroup.addFork(&state, ns);
      result.push_back(ns);
      es->ptreeNode->data = 0;
      std::pair<PTree::Node *, PTree::Node *> res =
          processTree->split(es->ptreeNode, ns, es);
      ns->ptreeNode = res.first;
      es->ptreeNode = res.second;
    }
  }

  // If necessary redistribute seeds to match conditions, killing
  // states if necessary due to OnlyReplaySeeds (inefficient but
  // simple).

  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap.find(&state);
  if (it != seedMap.end()) {
    std::vector<SeedInfo> seeds = it->second;
    seedMap.erase(it);

    // Assume each seed only satisfies one condition (necessarily true
    // when conditions are mutually exclusive and their conjunction is
    // a tautology).
    for (std::vector<SeedInfo>::iterator siit = seeds.begin(),
                                         siie = seeds.end();
         siit != siie; ++siit) {
      unsigned i;
      for (i = 0; i < N; ++i) {
        ref<ConstantExpr> res;
        bool success = solver->getValue(
            state, siit->assignment.evaluate(conditions[i]), res);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (res->isTrue())
          break;
      }

      // If we didn't find a satisfying condition randomly pick one
      // (the seed will be patched).
      if (i == N)
        i = theRNG.getInt32() % N;

      // Extra check in case we're replaying seeds with a max-fork
      if (result[i])
        seedMap[result[i]].push_back(*siit);
    }

    if (OnlyReplaySeeds) {
      for (unsigned i = 0; i < N; ++i) {
        if (result[i] && !seedMap.count(result[i])) {
          terminateState(*result[i], "Seed did not satisfy condition", "seed");
          result[i] = NULL;
        }
      }
    }
  }

  for (unsigned i = 0; i < N; ++i)
    if (result[i])
      addConstraint(*result[i], conditions[i]);
}

Executor::StatePair Executor::fork(ExecutionState &current, ref<Expr> condition,
                                   bool isInternal, bool countFork) {
  Solver::Validity res;
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap.find(&current);
  bool isSeeding = it != seedMap.end();
  SPDLOG_TRACE("[state {}] fork: {}", current.getID(), condition->str());

  if (!isSeeding && !isa<ConstantExpr>(condition) &&
      (MaxStaticForkPct != 1. || MaxStaticSolvePct != 1. ||
       MaxStaticCPForkPct != 1. || MaxStaticCPSolvePct != 1.) &&
      statsTracker->elapsed() > time::seconds(60)) {
    StatisticManager &sm = *theStatisticManager;
    CallPathNode *cpn = current.stack.back().callPathNode;
    int remain;
    if ((countFork && stateGroup.checkMaxFork(&current, 2, remain)) ||
        (MaxStaticForkPct < 1. &&
         sm.getIndexedValue(stats::forks, sm.getIndex()) >
             stats::forks * MaxStaticForkPct) ||
        (MaxStaticCPForkPct < 1. && cpn &&
         (cpn->statistics.getValue(stats::forks) >
          stats::forks * MaxStaticCPForkPct)) ||
        (MaxStaticSolvePct < 1 &&
         sm.getIndexedValue(stats::solverTime, sm.getIndex()) >
             stats::solverTime * MaxStaticSolvePct) ||
        (MaxStaticCPForkPct < 1. && cpn &&
         (cpn->statistics.getValue(stats::solverTime) >
          stats::solverTime * MaxStaticCPSolvePct))) {
      ref<ConstantExpr> value;
      bool success = solver->getValue(current, condition, value);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      SPDLOG_INFO("[state {}] Max fork reached, fork to true {}",
                  current.getID(), condition->str());
      addConstraint(current, EqExpr::create(value, condition));
      condition = value;
    }
  }
  time::Span timeout = coreSolverTimeout;
  if (isSeeding)
    timeout *= it->second.size();
  solver->setTimeout(timeout);
  bool success;
  if (usingSeeds) {
    ref<Expr> clone_cond = cloneTree(condition);
    ref<Expr> conc_cond = concretizeExpr(current, clone_cond);
    success = solver->evaluate(current, conc_cond, res);
    if (!(dyn_cast<ConstantExpr>(condition))) {
      if (res == Solver::True) {
        addConstraint(current, condition);
      } else {
        addConstraint(current, Expr::createIsZero(condition));
      }
    }
  } else {
    success = solver->evaluate(current, condition, res);
  }
  solver->setTimeout(time::Span());
  if (!success) {
    current.pc = current.prevPC;
    terminateStateEarly(current, "Query timed out (fork).");
    SPDLOG_DEBUG("[state {}] fork: query timed out", current.getID());
    return StatePair(0, 0);
  }
  if (!isSeeding) {
    if (replayPath && !isInternal) {
      assert(replayPosition < replayPath->size() &&
             "ran out of branches in replay path mode");
      bool branch = (*replayPath)[replayPosition++];

      if (res == Solver::True) {
        assert(branch && "hit invalid branch in replay path mode");
      } else if (res == Solver::False) {
        assert(!branch && "hit invalid branch in replay path mode");
      } else {
        // add constraints
        if (branch) {
          res = Solver::True;
          addConstraint(current, condition);
        } else {
          res = Solver::False;
          addConstraint(current, Expr::createIsZero(condition));
        }
      }
    } else if (res == Solver::Unknown) {
      assert(!replayKTest && "in replay mode, only one branch can be true.");
      int remain;
      if ((MaxMemoryInhibit && atMemoryLimit) || current.forkDisabled ||
          inhibitForking || (MaxForks != ~0u && stats::forks >= MaxForks) ||
          stateGroup.checkMaxFork(&current, 2, remain)) {

        if (MaxMemoryInhibit && atMemoryLimit)
          klee_warning_once(0, "skipping fork (memory cap exceeded)");
        else if (current.forkDisabled)
          klee_warning_once(0, "skipping fork (fork disabled on current path)");
        else if (inhibitForking)
          klee_warning_once(0, "skipping fork (fork disabled globally)");
        else
          klee_warning_once(0, "skipping fork (max-forks reached)");
        SPDLOG_INFO("[state {}] fork: max-forks reached", current.getID());
        TimerStatIncrementer timer(stats::forkTime);
        if (theRNG.getBool()) {
          addConstraint(current, condition);
          res = Solver::True;
        } else {
          addConstraint(current, Expr::createIsZero(condition));
          res = Solver::False;
        }
      }
    }
  }
  // Fix branch in only-replay-seed mode, if we don't have both true
  // and false seeds.
  if (isSeeding && (current.forkDisabled || OnlyReplaySeeds) &&
      res == Solver::Unknown) {
    bool trueSeed = false, falseSeed = false;
    // Is seed extension still ok here?
    for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                         siie = it->second.end();
         siit != siie; ++siit) {
      ref<ConstantExpr> res;
      bool success =
          solver->getValue(current, siit->assignment.evaluate(condition), res);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      if (res->isTrue()) {
        trueSeed = true;
      } else {
        falseSeed = true;
      }
      if (trueSeed && falseSeed)
        break;
    }
    if (!(trueSeed && falseSeed)) {
      assert(trueSeed || falseSeed);

      res = trueSeed ? Solver::True : Solver::False;
      addConstraint(current,
                    trueSeed ? condition : Expr::createIsZero(condition));
    }
  }

  // XXX - even if the constraint is provable one way or the other we
  // can probably benefit by adding this constraint and allowing it to
  // reduce the other constraints. For example, if we do a binary
  // search on a particular value, and then see a comparison against
  // the value it has been fixed at, we should take this as a nice
  // hint to just use the single constraint instead of all the binary
  // search ones. If that makes sense.
  if (res == Solver::True) {
    if (!isInternal) {
      if (pathWriter) {
        current.pathOS << "1";
      }
    }
    return StatePair(&current, 0);
  } else if (res == Solver::False) {
    if (!isInternal) {
      if (pathWriter) {
        current.pathOS << "0";
      }
    }
    return StatePair(0, &current);
  } else {
    TimerStatIncrementer timer(stats::forkTime);
    ExecutionState *falseState, *trueState = &current;
    ++stats::forks;

    falseState = trueState->branch();
    addedStates.push_back(falseState);
    if (countFork) {
      stateGroup.addFork(trueState, falseState);
    }
    SPDLOG_DEBUG("[state {}] -> [state {}] and [state {}] fork: true = {}, "
                 "false = {}, total = {}",
                 current.getID(), trueState->getID(), falseState->getID(),
                 trueState->getID(), falseState->getID(), addedStates.size());

    if (it != seedMap.end()) {
      std::vector<SeedInfo> seeds = it->second;
      it->second.clear();
      std::vector<SeedInfo> &trueSeeds = seedMap[trueState];
      std::vector<SeedInfo> &falseSeeds = seedMap[falseState];
      for (std::vector<SeedInfo>::iterator siit = seeds.begin(),
                                           siie = seeds.end();
           siit != siie; ++siit) {
        ref<ConstantExpr> res;
        bool success = solver->getValue(
            current, siit->assignment.evaluate(condition), res);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (res->isTrue()) {
          trueSeeds.push_back(*siit);
        } else {
          falseSeeds.push_back(*siit);
        }
      }

      bool swapInfo = false;
      if (trueSeeds.empty()) {
        if (&current == trueState)
          swapInfo = true;
        seedMap.erase(trueState);
      }
      if (falseSeeds.empty()) {
        if (&current == falseState)
          swapInfo = true;
        seedMap.erase(falseState);
      }
      if (swapInfo) {
        std::swap(trueState->coveredNew, falseState->coveredNew);
        std::swap(trueState->coveredLines, falseState->coveredLines);
      }
    }

    current.ptreeNode->data = 0;
    std::pair<PTree::Node *, PTree::Node *> res =
        processTree->split(current.ptreeNode, falseState, trueState);
    falseState->ptreeNode = res.first;
    trueState->ptreeNode = res.second;

    if (pathWriter) {
      // Need to update the pathOS.id field of falseState, otherwise the same id
      // is used for both falseState and trueState.
      falseState->pathOS = pathWriter->open(current.pathOS);
      if (!isInternal) {
        trueState->pathOS << "1";
        falseState->pathOS << "0";
      }
    }
    if (symPathWriter) {
      falseState->symPathOS = symPathWriter->open(current.symPathOS);
      if (!isInternal) {
        trueState->symPathOS << "1";
        falseState->symPathOS << "0";
      }
    }
    addConstraint(*trueState, condition);
    addConstraint(*falseState, Expr::createIsZero(condition));
    // Kinda gross, do we even really still want this option?
    if (MaxDepth && MaxDepth <= trueState->depth) {
      terminateStateEarly(*trueState, "max-depth exceeded.");
      terminateStateEarly(*falseState, "max-depth exceeded.");
      return StatePair(0, 0);
    }

    return StatePair(trueState, falseState);
  }
}

void Executor::addConstraint(ExecutionState &state, ref<Expr> condition) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(condition)) {
    if (!CE->isTrue())
      llvm::report_fatal_error("attempt to add invalid constraint");
    return;
  }

  // Check to see if this constraint violates seeds.
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap.find(&state);
  bool res;
  if (it != seedMap.end()) {
    bool warn = false;
    for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                         siie = it->second.end();
         siit != siie; ++siit) {

      bool success =
          solver->mustBeFalse(state, siit->assignment.evaluate(condition), res);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      if (res) {
        warn = true;
      }
    }
    if (warn)
      klee_warning("seeds patched for violating constraint");
  }
  std::string sourceLoc = "unknown";
  SPDLOG_TRACE("[state {}] add constraint: {}", state.getID(),
               condition->str());
  if (state.prevPC)
    sourceLoc = state.prevPC->getSourceLocation(false);
  std::string constraintsStr;
  llvm::raw_string_ostream os(constraintsStr);
  ExprPPrinter::printConstraints(os, state.constraints);
  SPDLOG_DEBUG(
      "[state {}] type: {} constraints: {} \n add constraint(taintMarker: "
      "{}): {} at {}",
      state.getID(), state.metaData.getStateType(), os.str(), taintMarker,
      condition->str(), sourceLoc);
  state.setTaintMarker(taintMarker);
  state.addConstraint(condition);
  state.setTaintMarker(false);

  if (PrintPath && state.prevPC) {
    std::string constraints;
    getConstraintLog(state, constraints, Interpreter::SMTLIB2);
    SPDLOG_DEBUG("[path:condition] {} : {}", state.prevPC->getSourceLocation(),
                 condition->str());
    SPDLOG_DEBUG("[path:ppc] {} : {}", state.prevPC->getSourceLocation(),
                 constraints);
  }
  if (LogPPC) {
    std::string constraints;
    getConstraintLog(state, constraints, Interpreter::SMTLIB2);

    if (sourceLoc.find("klee") == std::string::npos) {
      std::string log_message =
          "\n[path:ppc] " + sourceLoc + " : " + constraints;
      klee_log_ppc(log_message.c_str());
    }
  }
  if (LogTrace) {
    if (!TraceFilter.empty()) {
      if (TraceFilter == "control-loc" && state.prevPC) {
        std::string log_message =
            "[klee:trace] " + state.prevPC->getSourceLocation(true);
        klee_log_trace(log_message.c_str());
      }
    }
  }
  if (ivcEnabled)
    doImpliedValueConcretization(state, condition,
                                 ConstantExpr::alloc(1, Expr::Bool));
}

const Cell &Executor::eval(KInstruction *ki, unsigned index,
                           ExecutionState &state) const {
  assert(index < ki->inst->getNumOperands());
  int vnumber = ki->operands[index];

  assert(vnumber != -1 &&
         "Invalid operand to eval(), not a value or constant!");

  // Determine if this is a constant or not.
  if (vnumber < 0) {
    unsigned index = -vnumber - 2;
    return kmodule->constantTable[index];
  } else {
    unsigned index = vnumber;
    StackFrame &sf = state.stack.back();
    return sf.locals[index];
  }
}

void Executor::bindLocal(KInstruction *target, ExecutionState &state,
                         ref<Expr> value) {
  // specialFunctionHandler->trackTaint(state, target, value);
  getDestCell(state, target).value = value;
}

void Executor::bindLocal(KInstruction *target, ExecutionState &state,
                         ref<Expr> value, MemRecord *mr) {
  getDestCell(state, target).value = value;
}

void Executor::bindArgument(KFunction *kf, unsigned index,
                            ExecutionState &state, ref<Expr> value) {
  getArgumentCell(state, kf, index).value = value;
}

ref<Expr> Executor::toUnique(const ExecutionState &state, ref<Expr> &e) {
  ref<Expr> result = e;
  if (!isa<ConstantExpr>(e)) {
    if (usingSeeds) {
      result = concretizeExpr(state, result);
    } else {
      ref<ConstantExpr> value;
      bool isTrue = false;
      e = optimizer.optimizeExpr(e, true);
      solver->setTimeout(coreSolverTimeout);
      if (solver->getValue(state, e, value)) {
        ref<Expr> cond = EqExpr::create(e, value);
        cond = optimizer.optimizeExpr(cond, false);
        if (solver->mustBeTrue(state, cond, isTrue) && isTrue)
          result = value;
      }
      solver->setTimeout(time::Span());
    }
  }

  return result;
}

/* Concretize the given expression, and return a possible constant value.
   'reason' is just a documentation string stating the reason for
   concretization. */
ref<klee::ConstantExpr> Executor::toConstant(ExecutionState &state, ref<Expr> e,
                                             const char *reason) {
  e = state.constraints.simplifyExpr(e);
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e))
    return CE;

  ref<ConstantExpr> value;

  if (usingSeeds)
    e = concretizeExpr(state, e);

  bool success = solver->getValue(state, e, value);
  assert(success && "FIXME: Unhandled solver failure");
  (void)success;

  std::string str;
  llvm::raw_string_ostream os(str);
  os << "silently concretizing (reason: " << reason << ") expression " << e
     << " to value " << value << " (" << (*(state.pc)).info->file << ":"
     << (*(state.pc)).info->line << ")";

  if (AllExternalWarnings)
    klee_warning("%s", os.str().c_str());
  else
    klee_warning_once(reason, "%s", os.str().c_str());

  SPDLOG_DEBUG("[state {}] {}", state.getID(), os.str());
  addConstraint(state, EqExpr::create(e, value));

  return value;
}

void Executor::executeGetValue(ExecutionState &state, ref<Expr> e,
                               KInstruction *target) {
  e = state.constraints.simplifyExpr(e);
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
      seedMap.find(&state);
  if (it == seedMap.end() || isa<ConstantExpr>(e)) {
    ref<ConstantExpr> value;
    //    e = concretizeExpr(state, e);
    e = optimizer.optimizeExpr(e, true);
    bool success = solver->getValue(state, e, value);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    bindLocal(target, state, value);
  } else {
    std::set<ref<Expr>> values;
    for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                         siie = it->second.end();
         siit != siie; ++siit) {
      ref<Expr> cond = siit->assignment.evaluate(e);
      //      cond = concretizeExpr(state, cond);
      cond = optimizer.optimizeExpr(cond, true);
      ref<ConstantExpr> value;
      bool success = solver->getValue(state, cond, value);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      values.insert(value);
    }

    std::vector<ref<Expr>> conditions;
    for (std::set<ref<Expr>>::iterator vit = values.begin(), vie = values.end();
         vit != vie; ++vit)
      conditions.push_back(EqExpr::create(e, *vit));

    std::vector<ExecutionState *> branches;
    branch(state, conditions, branches);

    std::vector<ExecutionState *>::iterator bit = branches.begin();
    for (std::set<ref<Expr>>::iterator vit = values.begin(), vie = values.end();
         vit != vie; ++vit) {
      ExecutionState *es = *bit;
      if (es)
        bindLocal(target, *es, *vit);
      ++bit;
    }
  }
}

void Executor::printDebugInstructions(ExecutionState &state) {
  // check do not print
  if (DebugPrintInstructions.getBits() == 0)
    return;

  llvm::raw_ostream *stream = 0;
  if (DebugPrintInstructions.isSet(STDERR_ALL) ||
      DebugPrintInstructions.isSet(STDERR_SRC) ||
      DebugPrintInstructions.isSet(STDERR_COMPACT))
    stream = &llvm::errs();
  else
    stream = &debugLogBuffer;

  if (!DebugPrintInstructions.isSet(STDERR_COMPACT) &&
      !DebugPrintInstructions.isSet(FILE_COMPACT)) {
    (*stream) << "     " << state.pc->getSourceLocation() << ":";
  }

  (*stream) << state.pc->info->assemblyLine;

  if (DebugPrintInstructions.isSet(STDERR_ALL) ||
      DebugPrintInstructions.isSet(FILE_ALL))
    (*stream) << ":" << *(state.pc->inst);
  (*stream) << "\n";

  if (DebugPrintInstructions.isSet(FILE_ALL) ||
      DebugPrintInstructions.isSet(FILE_COMPACT) ||
      DebugPrintInstructions.isSet(FILE_SRC)) {
    debugLogBuffer.flush();
    (*debugInstFile) << debugLogBuffer.str();
    debugBufferString = "";
  }
}

void Executor::stepInstruction(ExecutionState &state) {
  printDebugInstructions(state);
  if (statsTracker)
    statsTracker->stepInstruction(state);

  ++stats::instructions;
  ++state.steppedInstructions;
  state.prevPC = state.pc;
  ++state.pc;

  if (stats::instructions == StopAfterNInstructions) {
    SPDLOG_WARN("STOPPING AFTER {} / {} INSTRUCTIONS", stats::instructions,
                StopAfterNInstructions);
    haltExecution = true;
  }
}

static inline const llvm::fltSemantics *fpWidthToSemantics(unsigned width) {
  switch (width) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
  case Expr::Int32:
    return &llvm::APFloat::IEEEsingle();
  case Expr::Int64:
    return &llvm::APFloat::IEEEdouble();
  case Expr::Fl80:
    return &llvm::APFloat::x87DoubleExtended();
#else
  case Expr::Int32:
    return &llvm::APFloat::IEEEsingle;
  case Expr::Int64:
    return &llvm::APFloat::IEEEdouble;
  case Expr::Fl80:
    return &llvm::APFloat::x87DoubleExtended;
#endif
  default:
    return 0;
  }
}

void Executor::initUniKleeAtEntry(ExecutionState &state, const KFunction *kf,
                                  Snapshot *snapshot) {
  llvm::Function *uf = kmodule->module->getFunction("uni_klee_make_symbolic");
  llvm::Function *ef = kmodule->module->getFunction("extractfix_make_symbolic");
  if (executionType == ExecutionType::MakeSnapshot) {
    if (uf != nullptr)
      functionBlackList.insert(uf);
    if (ef != nullptr)
      functionBlackList.insert(ef);
  } else {
    if (UseExtractFix) {
      if (uf != nullptr)
        functionBlackList.insert(uf);
    } else {
      if (ef != nullptr)
        functionBlackList.insert(ef);
    }
    state.metaData.setStateType(StateType::ST_base);
    stateGroup.addForkParent(&state);
  }
  dump_cfg(kmodule.get(), interpreterHandler->getOutputFilename("cfg.sbsv"));
  cfg = new ControlFlowGraph(kf);
  KInstruction *kinst = cfg->findTargetInst("__cpr_choice");
  if (kinst == nullptr || !UseCFG)
    return;
  cfg->markTargetKInst(kinst);
  cfg->markBasicBlocks();
}

void Executor::executeCall(ExecutionState &state, KInstruction *ki, Function *f,
                           std::vector<ref<Expr>> &arguments) {
  Instruction *i = ki->inst;
  if (functionBlackList.contains(f)) {
    if (f && f->getName().str() == "uni_klee_make_symbolic" &&
        executionType == ExecutionType::MakeSnapshot) {
      // Add the input memory address and their parents to the blacklist
      // Get address from argument
      if (arguments.size() != 3 || state.targetStack.empty()) {
        SPDLOG_ERROR(
            "[state {}] Wrong number of arguments for uni_klee_make_symbolic",
            state.getID());
        return;
      }
      Snapshot *snapshot = state.targetStack.back().snapshot;
      if (snapshot == nullptr) {
        SPDLOG_ERROR("[state {}] No snapshot found for target function",
                     state.getID());
        return;
      }
      ref<Expr> address = arguments[0];
      ref<Expr> size = arguments[1];
      std::string name =
          specialFunctionHandler->readStringAtAddress(state, arguments[2]);
      SPDLOG_INFO("[state {}] in make_snapshot, record input of "
                  "uni_klee_make_symbolic {}",
                  state.getID(), name);
      // Prepare endpoints from target function arguments
      std::set<uint64_t> endpoints;
      for (auto &arg : snapshot->arguments) {
        ConstantExpr *argCE = dyn_cast<ConstantExpr>(arg);
        if (argCE != nullptr && argCE->getWidth() == Expr::Int64) {
          uint64_t argValue = argCE->getZExtValue();
          ObjectPair op;
          bool success = state.addressSpace.resolveOne(argCE, op);
          if (!success) {
            SPDLOG_ERROR("[state {}] Cannot resolve address {}", state.getID(),
                         argValue);
            continue;
          }
          endpoints.insert(argCE->getZExtValue());
          endpoints.insert(op.first->address);
        }
      }
      // Get list of parents
      std::set<uint64_t> filter;
      std::vector<MemRecord> parents;
      std::queue<uint64_t> q;
      ref<ConstantExpr> addressCE = dyn_cast<ConstantExpr>(address);
      q.push(addressCE->getZExtValue());
      while (!q.empty()) {
        uint64_t addr = q.front();
        filter.insert(addr);
        q.pop();
        // auto &incomings = snapshot->memPtrGraph[q.front()].second;
        if (!memory->isValidAddress(addr))
          continue;
        ObjectPair op;
        bool success = state.addressSpace.resolveOne(
            ConstantExpr::create(addr, Expr::Int64), op);
        if (!success) {
          SPDLOG_DEBUG("[state {}] Cannot resolve address {}", state.getID(),
                       addr);
          continue;
        }
        uint64_t base = op.first->address;
        MemNode *mn = memGraph.getMemNode(base);
        if (mn == nullptr) {
          SPDLOG_DEBUG("[state {}] Cannot find memnode for address {}",
                       state.getID(), base);
          continue;
        }
        for (auto incoming : memGraph.getIncomingMemEdges(base)) {
          uint64_t incomingAddr = incoming->getFrom()->getAddress();
          if (filter.find(incomingAddr) == filter.end()) {
            ObjectPair op;
            bool success = state.addressSpace.resolveOne(
                ConstantExpr::create(incomingAddr, Expr::Int64), op);
            if (!success) {
              SPDLOG_TRACE("[state {}] Cannot resolve address {}",
                           state.getID(), incomingAddr);
              continue;
            }
            SPDLOG_DEBUG("[state {}] Add blacklisted address {}, base {}",
                         state.getID(), incomingAddr, op.first->address);
            q.push(incomingAddr);
            parents.push_back(MemRecord(incomingAddr, op.first->address, true,
                                        8, MemRecordType::UniKleeMakeSymbolic));
            filter.insert(incomingAddr);
            filter.insert(op.first->address);
          }
        }
      }
      // Add to snapshot
      SPDLOG_DEBUG("[state {}] Total {} found", state.getID(), parents.size());
      for (auto &parent : parents) {
        if (UseUniKleeMakeSym)
          accessedByExternalCall.insert(parent);
      }
    }
    SPDLOG_DEBUG("[state {}] Skipping function {} due to blacklisting",
                 state.getID(), f ? f->getName().str() : "null ptr");
    return;
  }
  if (f && f->isDeclaration()) {
    switch (f->getIntrinsicID()) {
    case Intrinsic::not_intrinsic:
      // state may be destroyed by this call, cannot touch
      callExternalFunction(state, ki, f, arguments);
      break;

    case Intrinsic::fabs: {
      ref<ConstantExpr> arg = toConstant(state, arguments[0], "floating point");
      if (!fpWidthToSemantics(arg->getWidth())) {
        return terminateStateOnExecError(
            state, "Unsupported intrinsic llvm.fabs call");
      }
      llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()),
                        arg->getAPValue());
      Res = llvm::abs(Res);
      bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
      break;
    }

      // case Intrinsic::rint: {
      //   //
      //   break;
      // }

      // va_arg is handled by caller and intrinsic lowering, see comment for
      // ExecutionState::varargs
    case Intrinsic::vastart: {
      StackFrame &sf = state.stack.back();
      // varargs can be zero if no varargs were provided
      if (!sf.varargs)
        return;

      // FIXME: This is really specific to the architecture, not the pointer
      // size. This happens to work for x86-32 and x86-64, however.
      Expr::Width WordSize = Context::get().getPointerWidth();
      if (WordSize == Expr::Int32) {
        executeMemoryOperation(state, true, arguments[0],
                               sf.varargs->getBaseExpr(), 0);
      } else {
        assert(WordSize == Expr::Int64 && "Unknown word size!");

        // x86-64 has quite complicated calling convention. However,
        // instead of implementing it, we can do a simple hack: just
        // make a function believe that all varargs are on stack.
        executeMemoryOperation(state, true, arguments[0],
                               ConstantExpr::create(48, 32), 0); // gp_offset
        executeMemoryOperation(
            state, true,
            AddExpr::create(arguments[0], ConstantExpr::create(4, 64)),
            ConstantExpr::create(304, 32), 0); // fp_offset
        executeMemoryOperation(
            state, true,
            AddExpr::create(arguments[0], ConstantExpr::create(8, 64)),
            sf.varargs->getBaseExpr(), 0); // overflow_arg_area
        executeMemoryOperation(
            state, true,
            AddExpr::create(arguments[0], ConstantExpr::create(16, 64)),
            ConstantExpr::create(0, 64), 0); // reg_save_area
      }
      break;
    }
    case Intrinsic::vaend:
      // va_end is a noop for the interpreter.
      //
      // FIXME: We should validate that the target didn't do something bad
      // with va_end, however (like call it twice).
      break;

    case Intrinsic::vacopy:
      // va_copy should have been lowered.
      //
      // FIXME: It would be nice to check for errors in the usage of this as
      // well.

#if LLVM_VERSION_CODE >= LLVM_VERSION(12, 0)
    case Intrinsic::abs: {
      if (isa<VectorType>(i->getOperand(0)->getType()))
        return terminateStateOnExecError(
            state, "llvm.abs with vectors is not supported");

      ref<Expr> op = eval(ki, 1, state).value;
      ref<Expr> poison = eval(ki, 2, state).value;

      assert(poison->getWidth() == 1 && "Second argument is not an i1");
      unsigned bw = op->getWidth();

      uint64_t moneVal = APInt(bw, -1, true).getZExtValue();
      uint64_t sminVal = APInt::getSignedMinValue(bw).getZExtValue();

      ref<ConstantExpr> zero = ConstantExpr::create(0, bw);
      ref<ConstantExpr> mone = ConstantExpr::create(moneVal, bw);
      ref<ConstantExpr> smin = ConstantExpr::create(sminVal, bw);

      if (poison->isTrue()) {
        ref<Expr> issmin = EqExpr::create(op, smin);
        if (issmin->isTrue())
          return terminateStateOnExecError(
              state, "llvm.abs called with poison and INT_MIN");
      }

      // conditions to flip the sign: INT_MIN < op < 0
      ref<Expr> negative = SltExpr::create(op, zero);
      ref<Expr> notsmin = NeExpr::create(op, smin);
      ref<Expr> cond = AndExpr::create(negative, notsmin);

      // flip and select the result
      ref<Expr> flip = MulExpr::create(op, mone);
      ref<Expr> result = SelectExpr::create(cond, flip, op);

      bindLocal(ki, state, result);
      break;
    }

    case Intrinsic::smax:
    case Intrinsic::smin:
    case Intrinsic::umax:
    case Intrinsic::umin: {
      if (isa<VectorType>(i->getOperand(0)->getType()) ||
          isa<VectorType>(i->getOperand(1)->getType()))
        return terminateStateOnExecError(
            state, "llvm.{s,u}{max,min} with vectors is not supported");

      ref<Expr> op1 = eval(ki, 1, state).value;
      ref<Expr> op2 = eval(ki, 2, state).value;

      ref<Expr> cond = nullptr;
      if (f->getIntrinsicID() == Intrinsic::smax)
        cond = SgtExpr::create(op1, op2);
      else if (f->getIntrinsicID() == Intrinsic::smin)
        cond = SltExpr::create(op1, op2);
      else if (f->getIntrinsicID() == Intrinsic::umax)
        cond = UgtExpr::create(op1, op2);
      else // (f->getIntrinsicID() == Intrinsic::umin)
        cond = UltExpr::create(op1, op2);

      ref<Expr> result = SelectExpr::create(cond, op1, op2);
      bindLocal(ki, state, result);
      break;
    }
#endif

    default:
      klee_error("unknown intrinsic: %s", f->getName().data());
    }

    if (InvokeInst *ii = dyn_cast<InvokeInst>(i))
      transferToBasicBlock(ii->getNormalDest(), i->getParent(), state);
  } else {
    // Check if maximum stack size was reached.
    // We currently only count the number of stack frames
    if (RuntimeMaxStackFrames && state.stack.size() > RuntimeMaxStackFrames) {
      terminateStateEarly(state, "Maximum stack size reached.");
      klee_warning("Maximum stack size reached.");
      return;
    }

    // FIXME: I'm not really happy about this reliance on prevPC but it is ok, I
    // guess. This just done to avoid having to pass KInstIterator everywhere
    // instead of the actual instruction, since we can't make a KInstIterator
    // from just an instruction (unlike LLVM).
    KFunction *kf = kmodule->functionMap[f];

    state.pushFrame(state.prevPC, kf);
    state.pc = kf->instructions;
    // Reached target function: make snapshot
    if ((executionType == ExecutionType::MakeSnapshot || DumpSnapshot) &&
        targetFunctionAddr == f) {
      SPDLOG_INFO("Target function {} found at {}", TargetFunction,
                  (uint64_t)targetFunctionAddr);
      int snapshotId = interpreterHandler->getSnapshotNumber();
      std::string snapshotName = interpreterHandler->getSnapshotName();
      state.stack.back().snapshot =
          getSnapshot(state, f, ki, arguments, snapshotId, snapshotName);
      if (executionType == ExecutionType::MakeSnapshot) {
        state.targetStack.push_back(state.stack.back());
        initUniKleeAtEntry(state, kf, nullptr);
      }
    }
    std::string TmpStr;
    llvm::raw_string_ostream os(TmpStr);
    os << "calling function: " << f->getName().str() << "(";
    for (unsigned i = 0; i < arguments.size(); i++) {
      if (isa<ConstantExpr>(arguments[i])) {
        os << arguments[i];
      } else {
        os << "symbolic";
      }
      if (i != arguments.size() - 1)
        os << ", ";
    }
    os << ") at " << state.pc->getSourceLocation();
    SPDLOG_DEBUG("[state {}] {} from {}", state.getID(), os.str(),
                 state.prevPC->getSourceLocation());
    if (f == patchLocationAddr) {
      ref<ConstantExpr> values = dyn_cast<ConstantExpr>(arguments[2]);
      ref<Expr> names = arguments[3];
      ref<Expr> nums = arguments[4];
      uint64_t num = dyn_cast<ConstantExpr>(nums)->getZExtValue();
      ObjectPair op;
      bool res = state.addressSpace.resolveOne(values, op);
      std::string tmpStr;
      llvm::raw_string_ostream os(tmpStr);
      if (res) {
        ref<Expr> offset = op.first->getOffsetExpr(values);
        os << "__cpr_choice (";
        for (uint64_t i = 0; i < num; i++) {
          ref<Expr> offsetExpr = AddExpr::create(
              offset, ConstantExpr::create(i * 8, offset->getWidth()));
          ref<Expr> value = op.second->read(offsetExpr, Expr::Int64);
          os << value << ", ";
        }
        os << ")";
        SPDLOG_INFO("[state {}] [show-values] {}", state.getID(), os.str());
      }
    }
    if (statsTracker)
      statsTracker->framePushed(state, &state.stack[state.stack.size() - 2]);

    // TODO: support "byval" parameter attribute
    // TODO: support zeroext, signext, sret attributes

    unsigned callingArgs = arguments.size();
    unsigned funcArgs = f->arg_size();
    if (!f->isVarArg()) {
      if (callingArgs > funcArgs) {
        klee_warning_once(f, "calling %s with extra arguments.",
                          f->getName().data());
      } else if (callingArgs < funcArgs) {
        terminateStateOnError(state, "calling function with too few arguments",
                              User);
        return;
      }
    } else {
      Expr::Width WordSize = Context::get().getPointerWidth();

      if (callingArgs < funcArgs) {
        terminateStateOnError(
            state, "calling function with too few arguments va", User);
        return;
      }

      StackFrame &sf = state.stack.back();
      unsigned size = 0;
      bool requires16ByteAlignment = false;
      for (unsigned i = funcArgs; i < callingArgs; i++) {
        // FIXME: This is really specific to the architecture, not the pointer
        // size. This happens to work for x86-32 and x86-64, however.
        if (WordSize == Expr::Int32) {
          size += Expr::getMinBytesForWidth(arguments[i]->getWidth());
        } else {
          Expr::Width argWidth = arguments[i]->getWidth();
          // AMD64-ABI 3.5.7p5: Step 7. Align l->overflow_arg_area upwards to a
          // 16 byte boundary if alignment needed by type exceeds 8 byte
          // boundary.
          //
          // Alignment requirements for scalar types is the same as their size
          if (argWidth > Expr::Int64) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
            size = llvm::alignTo(size, 16);
#else
            size = llvm::RoundUpToAlignment(size, 16);
#endif
            requires16ByteAlignment = true;
          }
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
          size += llvm::alignTo(argWidth, WordSize) / 8;
#else
          size += llvm::RoundUpToAlignment(argWidth, WordSize) / 8;
#endif
        }
      }

      MemoryObject *mo = sf.varargs =
          memory->allocate(size, true, false, state.prevPC->inst,
                           (requires16ByteAlignment ? 16 : 8));
      if (!mo && size) {
        terminateStateOnExecError(state, "out of memory (varargs)");
        return;
      }

      if (mo) {
        if ((WordSize == Expr::Int64) && (mo->address & 15) &&
            requires16ByteAlignment) {
          // Both 64bit Linux/Glibc and 64bit MacOSX should align to 16 bytes.
          klee_warning_once(
              0, "While allocating varargs: malloc did not align to 16 bytes.");
        }

        ObjectState *os = bindObjectInState(state, mo, true);
        unsigned offset = 0;
        for (unsigned i = funcArgs; i < callingArgs; i++) {
          // FIXME: This is really specific to the architecture, not the pointer
          // size. This happens to work for x86-32 and x86-64, however.
          if (WordSize == Expr::Int32) {
            os->write(offset, arguments[i]);
            offset += Expr::getMinBytesForWidth(arguments[i]->getWidth());
          } else {
            assert(WordSize == Expr::Int64 && "Unknown word size!");

            Expr::Width argWidth = arguments[i]->getWidth();
            if (argWidth > Expr::Int64) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
              offset = llvm::alignTo(offset, 16);
#else
              offset = llvm::RoundUpToAlignment(offset, 16);
#endif
            }
            os->write(offset, arguments[i]);
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
            offset += llvm::alignTo(argWidth, WordSize) / 8;
#else
            offset += llvm::RoundUpToAlignment(argWidth, WordSize) / 8;
#endif
          }
        }
      }
    }

    unsigned numFormals = f->arg_size();
    for (unsigned i = 0; i < numFormals; ++i)
      bindArgument(kf, i, state, arguments[i]);
  }
}

void Executor::transferToBasicBlock(BasicBlock *dst, BasicBlock *src,
                                    ExecutionState &state) {
  // Note that in general phi nodes can reuse phi values from the same
  // block but the incoming value is the eval() result *before* the
  // execution of any phi nodes. this is pathological and doesn't
  // really seem to occur, but just in case we run the PhiCleanerPass
  // which makes sure this cannot happen and so it is safe to just
  // eval things in order. The PhiCleanerPass also makes sure that all
  // incoming blocks have the same order for each PHINode so we only
  // have to compute the index once.
  //
  // With that done we simply set an index in the state so that PHI
  // instructions know which argument to eval, set the pc, and continue.

  // XXX this lookup has to go ?
  KFunction *kf = state.stack.back().kf;
  unsigned entry = kf->basicBlockEntry[dst];
  state.pc = &kf->instructions[entry];
  if (state.pc->inst->getOpcode() == Instruction::PHI) {
    PHINode *first = static_cast<PHINode *>(state.pc->inst);
    state.incomingBBIndex = first->getBasicBlockIndex(src);
  }
}

/// Compute the true target of a function call, resolving LLVM and KLEE aliases
/// and bitcasts.
Function *Executor::getTargetFunction(Value *calledVal, ExecutionState &state) {
  SmallPtrSet<const GlobalValue *, 3> Visited;

  Constant *c = dyn_cast<Constant>(calledVal);
  if (!c)
    return 0;

  while (true) {
    if (GlobalValue *gv = dyn_cast<GlobalValue>(c)) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
      if (!Visited.insert(gv).second)
        return 0;
#else
      if (!Visited.insert(gv))
        return 0;
#endif
      std::string alias = state.getFnAlias(gv->getName().str());
      if (alias != "") {
        GlobalValue *old_gv = gv;
        gv = kmodule->module->getNamedValue(alias);
        if (!gv) {
          klee_error("Function %s(), alias for %s not found!\n", alias.c_str(),
                     old_gv->getName().str().c_str());
        }
      }

      if (Function *f = dyn_cast<Function>(gv))
        return f;
      else if (GlobalAlias *ga = dyn_cast<GlobalAlias>(gv))
        c = ga->getAliasee();
      else
        return 0;
    } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(c)) {
      if (ce->getOpcode() == Instruction::BitCast)
        c = ce->getOperand(0);
      else
        return 0;
    } else
      return 0;
  }
}

static bool isDebugIntrinsic(const Function *f, KModule *KM) {
  std::string funcName = f->getName().str();
  if (funcName == "llvm.dbg.declare")
    return true;
  return false;
}

bool Executor::isKleeLibFunction(const std::string &sourceLoc) {
  bool result = false;
  if (sourceLoc.find("klee-uclibc") != std::string::npos) {
    result = true;
  }
  if (sourceLoc.find("klee/runtime") != std::string::npos) {
    result = true;
  }
  return result;
}

void Executor::setTaintMarker(bool taint) {
  if (taintMarker && taint) {
    SPDLOG_WARN("Taint marker already set to true");
  } else if (taintMarker || taint) {
    SPDLOG_DEBUG("Setting taint marker from {} to {}", taintMarker, taint);
  }
  taintMarker = taint;
}

bool Executor::checkMemRecord(ExecutionState &state) {
  return executionType == ExecutionType::MakeSnapshot &&
         !state.targetStack.empty();
}

void Executor::forkWithPatchDataManager(ExecutionState &state,
                                        KInstruction *kcaller) {
  // Snatch the return value: fork true / false
  SPDLOG_DEBUG("[state {}] Forking with patch data manager", state.getID());
  // 1. Iterate patches and get the patch result type
  solver->setTimeout(coreSolverTimeout);
  PatchDataManager &pdm = state.patchDataManager;
  // ExecutionState *baseState = stateGroup.getForkParent(&state);
  // if (baseState == nullptr) {
  //   SPDLOG_ERROR("[state {}] Cannot find base state", state.getID());
  //   return;
  // }
  // bool baseCrashed = baseState->metaData.getIsCrash();
  // PatchDataManager &bpdm = baseState->patchDataManager;

  // Set trace
  std::vector<PatchData> patches;
  std::map<int, PatchData *> &patchMap = pdm.getPatchData();
  uint64_t iter = pdm.getCount();
  if (iter == 1) {
    pdm.setPatchSet();
  }
  std::set<int> patchSet;
  pdm.getPatchSet(patchSet);
  bool hasTrue = false;
  bool hasFalse = false;
  std::stringstream ss;
  for (int patchId : patchSet) {
    ref<Expr> patchCondition = pdm.getPatchCondition(patchId);
    Solver::Validity res;
    bool tmp = solver->evaluate(state, patchCondition, res);
    if (!tmp) {
      SPDLOG_ERROR("[state {}] Solver timeout", state.getID());
      continue;
    }
    PatchData pd = PatchData(patchId, iter, patchCondition, res);
    patches.push_back(pd);
    if (res == Solver::True) {
      hasTrue = true;
      ss << patchId << ":1, ";
    } else if (res == Solver::False) {
      hasFalse = true;
      ss << patchId << ":0, ";
    } else {
      hasTrue = true;
      hasFalse = true;
      ss << patchId << ":?, ";
    }
  }
  solver->setTimeout(time::Span());
  // Both are empty: early terminate
  if (!hasTrue && !hasFalse) {
    SPDLOG_INFO("[state {}] No remaining patch", state.getID());
    terminateStateEarly(state, "No ramaining patch");
    return;
  }
  // One of them is empty: simply use the other one
  if (!hasTrue || !hasFalse) {
    ref<Expr> result;
    pdm.setLast(hasTrue);
    for (PatchData &pd : patches) {
      pdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
    }
    result = ConstantExpr::create(hasTrue ? 1 : 0, Expr::Int32);
    bindLocal(kcaller, state, result);
    for (StackFrame &ts : state.targetStack) {
      ts.lazyTrace.push_back(ConstantExpr::create(hasTrue ? 1 : 0, Expr::Bool));
    }
    klee_log_data(fmt::format(
        "[patch] [trace] [state {}] [iter {}] [res {}] [patches [{}]]",
        state.getID(), iter, hasTrue ? 1 : 0, ss.str()));
    return;
  }
  // Both are not empty: fork

  int remain = 0;
  if (stateGroup.checkMaxFork(&state, 1, remain)) {
    // Select randomly
    bool isTrue = theRNG.getBool();
    pdm.setLast(isTrue);
    for (PatchData &pd : patches) {
      pdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
    }
    ref<Expr> result = ConstantExpr::create(isTrue ? 1 : 0, Expr::Int32);
    bindLocal(kcaller, state, result);
    for (StackFrame &ts : state.targetStack) {
      ts.lazyTrace.push_back(ConstantExpr::create(isTrue ? 1 : 0, Expr::Bool));
    }
    klee_log_data(fmt::format(
        "[patch] [trace-rand] [state {}] [iter {}] [res {}] [patches [{}]]",
        state.getID(), iter, isTrue ? 1 : 0, ss.str()));
    return;
  }
  ExecutionState *falseState = cloneState(&state);
  stateGroup.addFork(&state, falseState);
  klee_log_data(fmt::format(
      "[patch] [fork] [state {}] [state {}] [iter {}] [patches [{}]]",
      state.getID(), falseState->getID(), iter, ss.str()));
  PatchDataManager &fpdm = falseState->patchDataManager;
  pdm.setLast(true);
  fpdm.setLast(false);
  for (PatchData &pd : patches) {
    pdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
    fpdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
  }
  ref<Expr> result = ConstantExpr::create(0, Expr::Int32);
  bindLocal(kcaller, *falseState, result);
  for (StackFrame &ts : falseState->targetStack) {
    ts.lazyTrace.push_back(ConstantExpr::create(0, Expr::Bool));
  }
  result = ConstantExpr::create(1, Expr::Int32);
  bindLocal(kcaller, state, result);
  for (StackFrame &ts : state.targetStack) {
    ts.lazyTrace.push_back(ConstantExpr::create(1, Expr::Bool));
  }
}

void Executor::forkForBase(ExecutionState &state, KInstruction *kcaller) {
  // Snatch the return value: fork true / false
  SPDLOG_DEBUG("[state {}] Forking for base", state.getID());
  // 1. Iterate patches and get the patch result type
  solver->setTimeout(coreSolverTimeout);
  PatchDataManager &pdm = state.patchDataManager;
  // Set trace
  std::vector<PatchData> patches;
  std::map<int, PatchData *> &patchMap = pdm.getPatchData();
  uint64_t iter = pdm.getCount();
  if (iter == 1) {
    pdm.setPatchSet();
  }
  const int bugId = 0;
  ref<Expr> patchCondition = pdm.getPatchCondition(bugId);
  std::set<int> patchSet;
  pdm.getPatchSet(patchSet);
  std::stringstream ss;
  for (int patchId : patchSet) {
    ref<Expr> patchCondition = pdm.getPatchCondition(patchId);
    Solver::Validity res;
    bool tmp = solver->evaluate(state, patchCondition, res);
    if (!tmp) {
      SPDLOG_ERROR("[state {}] Solver timeout", state.getID());
      continue;
    }
    PatchData pd = PatchData(patchId, iter, patchCondition, res);
    patches.push_back(pd);
    if (res == Solver::True) {
      ss << patchId << ":1, ";
    } else if (res == Solver::False) {
      ss << patchId << ":0, ";
    } else {
      ss << patchId << ":?, ";
    }
  }
  Solver::Validity res;
  bool tmp = solver->evaluate(state, patchCondition, res);
  if (!tmp) {
    SPDLOG_ERROR("[state {}] Solver timeout", state.getID());
    terminateStateEarly(state, "Solver failure at forkForBase");
    return;
  }
  SPDLOG_DEBUG(
      "[state {}] [patch-condition {}] of [patch {}] [res {}] [patches [{}]]",
      state.getID(), patchCondition->str(), bugId, res, ss.str());
  solver->setTimeout(time::Span());
  // One of them is empty: simply use the other one
  if (res == Solver::Validity::True || res == Solver::Validity::False) {
    ref<Expr> result;
    bool hasTrue = res == Solver::True;
    pdm.setLast(hasTrue);
    for (PatchData &pd : patches) {
      pdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
    }
    result = ConstantExpr::create(hasTrue ? 1 : 0, Expr::Int32);
    bindLocal(kcaller, state, result);
    for (StackFrame &ts : state.targetStack) {
      ts.lazyTrace.push_back(ConstantExpr::create(hasTrue ? 1 : 0, Expr::Bool));
    }
    klee_log_data(fmt::format(
        "[patch-base] [trace] [state {}] [res {}] [iter {}] [patches [{}]]",
        state.getID(), hasTrue ? 1 : 0, iter, ss.str()));
    return;
  }
  SPDLOG_DEBUG("[state {}] Fork for base: {} => {}", state.getID(), res,
               patchCondition->str());
  assert(executionType != ExecutionType::MakeSnapshot &&
         "Should not fork in snapshot mode");
  // Non constant: fork
  StatePair sp = fork(state, patchCondition, true);
  if (sp.first == nullptr && sp.second == nullptr) {
    SPDLOG_ERROR("[state {}] Fork failed", state.getID());
    return;
  } else if (sp.first == nullptr || sp.second == nullptr) {
    bool isTrue = sp.first != nullptr;
    ExecutionState *remain = isTrue ? sp.first : sp.second;
    ref<Expr> result = ConstantExpr::create(isTrue ? 1 : 0, Expr::Int32);
    bindLocal(kcaller, *remain, result);
    SPDLOG_DEBUG("[state {}] Fork {}", remain->getID(), isTrue);
    return;
  }
  ExecutionState *trueState = sp.first;
  ExecutionState *falseState = sp.second;
  klee_log_data(fmt::format(
      "[patch-base] [fork] [state {}] [state {}] [iter {}] [patches [{}]]",
      trueState->getID(), falseState->getID(), iter, ss.str()));
  PatchDataManager &tpdm = trueState->patchDataManager;
  PatchDataManager &fpdm = falseState->patchDataManager;
  tpdm.setLast(true);
  fpdm.setLast(false);

  for (PatchData &pd : patches) {
    tpdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
    fpdm.updatePatchType(pd.getPatchId(), pd.getPatchType());
  }
  ref<Expr> result = ConstantExpr::create(0, Expr::Int32);
  bindLocal(kcaller, *falseState, result);
  for (StackFrame &ts : falseState->targetStack) {
    ts.lazyTrace.push_back(ConstantExpr::create(0, Expr::Bool));
  }
  result = ConstantExpr::create(1, Expr::Int32);
  bindLocal(kcaller, *trueState, result);
  for (StackFrame &ts : trueState->targetStack) {
    ts.lazyTrace.push_back(ConstantExpr::create(1, Expr::Bool));
  }
}

void Executor::importSymbolicInput(ExecutionState &state, std::string fileName,
                                   Snapshot *snapshot, llvm::Function *f,
                                   std::vector<ref<Expr>> &arguments) {
  KFunction *kf = kmodule->functionMap[f];
  for (auto arg : snapshot->arguments) {
    ref<Expr> finalExpr = arg;
    arguments.push_back(finalExpr);
  }
  // Read json file
  Json::Value root;
  std::ifstream ifs(fileName);
  if (ifs.is_open()) {
    ifs >> root;
    ifs.close();
  } else {
    klee_error("Cannot open symbolic input file %s", fileName.c_str());
  }
  int stateId = root["state"].asInt();
  int crashId = root["crashId"].asInt();
  bool isCrash = root["isCrash"].asBool();
  std::set<uint64_t> nonSymbolic;
  // First, only treat lazy init
  for (auto &it : root["symbolic_objects"]) {
    uint64_t size = it["size"].asUInt64();
    std::string name = it["name"].asString();
    std::string value = it["value"].asString();
    uint64_t address = it["address"].asUInt64();
    std::string symType = it["symType"].asString();
    uint64_t offset = it["offset"].asUInt64();
    Json::Value &originalObject = it["original_object"];
    bool exists = originalObject["exists"].asBool();
    uint64_t originalSize = 0;
    uint64_t originalBase = 0;
    uint64_t originalOffset = 0;
    uint64_t requestedAddr = 0;
    bool existing = false;
    if (exists) { // From makeFieldSymbolic
      originalSize = originalObject["originalSize"].asUInt64();
      originalBase = originalObject["originalBase"].asUInt64();
      originalOffset = originalObject["originalOffset"].asUInt64();
      requestedAddr = originalObject["requestedAddr"].asUInt64();
      existing = originalObject["existing"].asBool();
    }
    if (requestedAddr == 0)
      continue;
    nonSymbolic.insert(requestedAddr);
    if (originalSize != 0) {
      ObjectPair oop;
      MemoryObject *mo = nullptr;
      if (state.addressSpace.resolveOne(
              ConstantExpr::create(originalBase, Expr::Int64), oop)) {
        mo = (MemoryObject *)oop.first;
        SPDLOG_DEBUG("[state {}] Found existing object at {} - existing {}",
                     state.getID(), originalBase, existing);
      } else {
        // Create new object at specific address and write it to the
        // requestedAddr
        mo = memory->inject(originalBase, originalSize, nullptr, false, false,
                            false);
      }
      mo->setName(name);
      executeMakeSymbolic(state, mo, name);
      ObjectPair op;
      ref<ConstantExpr> requestedAddrExpr =
          ConstantExpr::create(requestedAddr, Expr::Int64);
      if (!state.addressSpace.resolveOne(requestedAddrExpr, op)) {
        SPDLOG_WARN("Cannot resolve address {}", originalBase);
        continue;
      }
      uint64_t offset = requestedAddr - op.first->address;
      ObjectState *wos = state.addressSpace.getWriteable(op.first, op.second);
      wos->write(op.first->getOffsetExpr(requestedAddrExpr), mo->getBaseExpr());
    } else {
      // Find existing object
      SPDLOG_WARN("This should not happen.. {}", address);
    }
  }
  // Next, do others
  for (auto &it : root["symbolic_objects"]) {
    // continue;
    uint64_t size = it["size"].asUInt64();
    std::string name = it["name"].asString();
    std::string value = it["value"].asString();
    uint64_t address = it["address"].asUInt64();
    std::string symType = it["symType"].asString();
    uint64_t offset = it["offset"].asUInt64();
    Json::Value &originalObject = it["original_object"];
    bool exists = originalObject["exists"].asBool();
    bool existing = false;
    uint64_t originalSize = 0;
    uint64_t originalBase = 0;
    uint64_t originalOffset = 0;
    uint64_t requestedAddr = 0;
    if (exists) { // From makeFieldSymbolic
      originalSize = originalObject["originalSize"].asUInt64();
      originalBase = originalObject["originalBase"].asUInt64();
      originalOffset = originalObject["originalOffset"].asUInt64();
      requestedAddr = originalObject["requestedAddr"].asUInt64();
      existing = originalObject["existing"].asBool();
    }
    if (requestedAddr != 0)
      continue;
    if (exists && originalSize != 0) {
      if (nonSymbolic.find(originalBase + originalOffset) !=
          nonSymbolic.end()) {
        SPDLOG_DEBUG("[state {}] Skip import {}", state.getID(), originalBase);
        continue;
      }
      // Create new object and copy -> paste the symbolic value
      MemoryObject *mo =
          memory->allocate(originalSize, false, false, nullptr, 8);
      mo->setName(name);
      executeMakeSymbolic(state, mo, name);
      ObjectState *os = bindObjectInState(state, mo, false);
      ref<Expr> value = os->read(0, Expr::Width(originalSize * 8));
      ObjectPair op;
      if (!state.addressSpace.resolveOne(
              ConstantExpr::create(originalBase, Expr::Int64), op)) {
        SPDLOG_WARN("Cannot resolve address {}", originalBase);
        continue;
      }
      ObjectState *wos = state.addressSpace.getWriteable(op.first, op.second);
      wos->write(originalOffset, value);
    } else if (symType == "arg") {
      uint32_t index = it["index"].asUInt();
      MemoryObject *argvMO = memory->allocate(size, true, false, f, 8);
      argvMO->setName(name);
      executeMakeSymbolic(state, argvMO, name);
      ObjectPair op;
      bool resolved = state.addressSpace.resolveOne(argvMO->getBaseExpr(), op);
      assert(resolved && "pointer must be resolved");
      ref<Expr> symArgExpr =
          op.second->read(0, Expr::Width(arguments[index]->getWidth()));
      arguments[index] = symArgExpr;
      SPDLOG_DEBUG("[state {}] sym arg {}", state.getID(), name);
    } else {
      // Find existing object
      ObjectPair op;
      if (!state.addressSpace.resolveOne(
              ConstantExpr::create(address, Expr::Int64), op)) {
        SPDLOG_WARN("Cannot resolve address {}", address);
        continue;
      }
      MemoryObject *mo = (MemoryObject *)op.first;
      mo->setName(name);
      executeMakeSymbolic(state, mo, name);
    }
  }
  int index = 0;
  for (auto arg : arguments) {
    bindArgument(kf, index, state, arg);
    index++;
  }
}

void Executor::exportSymbolicInput(
    std::string fileName, const ExecutionState &state,
    std::vector<std::pair<std::string, std::vector<unsigned char>>> &res) {
  // if (!state.metaData.isType(StateType::ST_base_after))
  //   return;
  // Export symbolic input to the file
  Json::Value root;
  root["state"] = state.getID();
  root["crashId"] = state.metaData.getCrashID();
  root["isCrash"] = state.metaData.getIsCrash();
  // 1. Symbolic objects
  Json::Value &symbolicObjects = root["symbolic_objects"] = Json::arrayValue;
  const Array *patchId = nullptr;
  for (uint64_t i = 0; i < state.symbolics.size(); ++i) {
    Json::Value &symbolicObject = symbolicObjects.append(Json::objectValue);
    auto &obj = state.symbolics[i];
    symbolicObject["size"] = obj.getSize();
    symbolicObject["name"] = obj.getName();
    // symbolicObject["value"] = toDataString(res[i].second);
    symbolicObject["address"] = obj.getAddress();
    symbolicObject["symType"] = obj.getType();
    symbolicObject["offset"] = obj.getOffset();
    if (obj.getName() == "uni_klee_patch_id") {
      patchId = obj.getArray();
    }
    // 1.1. Parameters
    if (obj.getType() == "arg") {
      symbolicObject["index"] = obj.getIndex();
    }
    // 1.2. Heap
    // 1.3. Lazy init
    // LazyInitManager
    Json::Value &originalObject = symbolicObject["original_object"] =
        Json::objectValue;
    const Array *arr = obj.getArray();
    MemArchive ma;
  }
  ArrayChecker arrchk = ArrayChecker(patchId);
  // 2. Symbolic constraints
  bool found = false;
  Json::Value &symbolicConstraints = root["symbolic_constraints"] =
      Json::arrayValue;
  for (const ref<Expr> &constraint : state.constraints) {
    Json::Value &symbolicConstraint =
        symbolicConstraints.append(Json::objectValue);
    symbolicConstraint["constraint"] = constraint->str();
    // 2.1. Precondition: before the cpr_choice
    // 2.2. Postcondition: after the cpr_choice
    if (arrchk.containsArray(constraint)) {
      found = true;
    }
    if (!found) {
      symbolicConstraint["position"] = "pre";
    } else {
      symbolicConstraint["position"] = "post";
    }
  }
  // 3. Use json format
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "  ";
  std::ofstream ofs(fileName);
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(root, &ofs);
  ofs.close();
}

void Executor::executeInstruction(ExecutionState &state, KInstruction *ki) {
  Instruction *i = ki->inst;
  std::string sourceLoc = ki->getSourceLocation();
  const bool bbCheck = (executionType == ExecutionType::FromSnapshot ||
                        executionType == ExecutionType::InsideTargetFunction ||
                        executionType == ExecutionType::UnderConstrained) &&
                       !state.passedTarget && state.recursiveDepth == 1;
  if (!isKleeLibFunction(sourceLoc)) {
    for (auto &ts : state.targetStack) {
      ts.trace.push_back(sourceLoc);
    }
    if (PrintTrace) {
      SPDLOG_DEBUG("[trace] [state {}] {} - {}", state.getID(), sourceLoc,
                   i->getOpcodeName());
    } else {
      SPDLOG_TRACE("[trace] [state {}] {} - {}", state.getID(), sourceLoc,
                   i->getOpcodeName());
    }

    if (LogTrace) {
      std::string simpleSourceLoc = ki->getSourceLocation(true);
      if (!TraceFilter.empty()) {
        if (simpleSourceLoc.find(TraceFilter, 0) == std::string::npos) {
          std::string log_message = "[klee:trace] " + simpleSourceLoc;
          klee_log_trace(log_message.c_str());
        }
      } else if (!LocHit.empty()) {
        if (std::find(hit_list.begin(), hit_list.end(), simpleSourceLoc) !=
            hit_list.end()) {
          std::string log_message = "[klee:trace] " + simpleSourceLoc;
          klee_log_trace(log_message.c_str());
          hit_list.erase(simpleSourceLoc);
        }
      } else {
        std::string log_message =
            fmt::format("[state {}] [{}] {}", state.getID(),
                        cfg != nullptr ? 'A' : 'B', ki->getSourceLocation());
        klee_log_trace(log_message.c_str());
      }
    }
  }
  //    std::string constraints;
  //    getConstraintLog(state, constraints, Executor::SMTLIB2);
  switch (i->getOpcode()) {
  // Control flow
  case Instruction::Ret: {
    ReturnInst *ri = cast<ReturnInst>(i);
    KInstIterator kcaller = state.stack.back().caller;
    Instruction *caller = kcaller ? kcaller->inst : 0;
    bool isVoidReturn = (ri->getNumOperands() == 0);
    ref<Expr> result = ConstantExpr::alloc(0, Expr::Bool);
    bool retTaint = false;
    if (!isVoidReturn) {
      result = eval(ki, 0, state).value;
      retTaint = eval(ki, 0, state).taint;
    }

    if (state.stack.size() <= 1) {
      assert(!caller && "caller set on initial stack frame");
      terminateStateOnExit(state);
    } else {

      if (OutputLocalsOnError)
        state.addStateInfoAsReturn(ki, kmodule->targetData.get());
      state.popFrame();

      if (statsTracker)
        statsTracker->framePopped(state);

      if (InvokeInst *ii = dyn_cast<InvokeInst>(caller)) {
        transferToBasicBlock(ii->getNormalDest(), caller->getParent(), state);
      } else {
        state.pc = kcaller;
        ++state.pc;
      }

      if (!isVoidReturn) {
        Type *t = caller->getType();
        if (t != Type::getVoidTy(i->getContext())) {
          // may need to do coercion due to bitcasts
          Expr::Width from = result->getWidth();
          Expr::Width to = getWidthForLLVMType(t);

          if (from != to) {

#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
            const CallBase &cs = cast<CallBase>(*caller);
#else
            const CallSite cs =
                (isa<InvokeInst>(caller) ? CallSite(cast<InvokeInst>(caller))
                                         : CallSite(cast<CallInst>(caller)));
#endif
            // XXX need to check other param attrs ?
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
            bool isSExt = cs.hasRetAttr(llvm::Attribute::SExt);
#else
            bool isSExt = cs.paramHasAttr(0, llvm::Attribute::SExt);
#endif
            if (isSExt) {
              result = SExtExpr::create(result, to);
            } else {
              result = ZExtExpr::create(result, to);
            }
          }
          // If it's __cpr_choice, we need to track the return value
          if (ri->getFunction() == patchLocationAddr) {
            Cell &cell = getDestCell(state, kcaller);
            cell.taint = true;
            if (LazyPatch) {
              if (state.metaData.isType(StateType::ST_crash_test) ||
                  PatchFiltering) {
                forkWithPatchDataManager(state, kcaller);
              } else {
                forkForBase(state, kcaller);
              }
              break;
            }
            solver->setTimeout(coreSolverTimeout);
            Solver::Validity res;
            bool success = solver->evaluate(
                state, NeExpr::create(result, ConstantExpr::create(0, 32)),
                res);
            solver->setTimeout(time::Span());
            if (success) {
              if (res == Solver::True || res == Solver::False) {
                SPDLOG_INFO(
                    "[state {}] [patch {}] [crash {}] [res {}] __cpr_choice",
                    state.getID(), state.metaData.getPatchID(),
                    state.metaData.getCrashID(), res == Solver::True);
              } else {
                SPDLOG_INFO(
                    "[state {}] [patch {}] [crash {}] [res {}] __cpr_choice {}",
                    state.getID(), state.metaData.getPatchID(),
                    state.metaData.getCrashID(), "unknown", result->str());
              }
            }
            for (StackFrame &ts : state.targetStack) {
              ts.lazyTrace.push_back(
                  NeExpr::create(result, ConstantExpr::create(0, 32)));
            }
          } else {
            if (caller != nullptr && retTaint) {
              getDestCell(state, kcaller).taint = true;
            }
          }
          bindLocal(kcaller, state, result);
        }
      } else {
        // We check that the return value has no users instead of
        // checking the type, since C defaults to returning int for
        // undeclared functions.
        if (!caller->use_empty()) {
          terminateStateOnExecError(
              state, "return void when caller expected a result");
        }
      }
    }
    break;
  }
  case Instruction::Br: {
    BranchInst *bi = cast<BranchInst>(i);
    llvm::StringRef sourceLocRef(ki->info->file);
    if (bi->isUnconditional()) {
      bool jump = true;
      if (bbCheck) {
        const llvm::BasicBlock *dst = bi->getSuccessor(0);
        if (cfg->checkDeadNode(dst)) {
          SPDLOG_DEBUG("[state {}] DEAD NODE br uncond {} at {}", state.getID(),
                       dst->getName().str(), sourceLoc);
          terminateStateEarly(state, "Dead node");
          jump = false;
        } // else if (SymbolicJumpPolicy == SJP_FollowBuggyPath) {
          // Check if we are following the buggy path
        // }
      }
      if (jump) {
        transferToBasicBlock(bi->getSuccessor(0), bi->getParent(), state);
      }
    } else {
      // FIXME: Find a way that we don't have this hidden dependency.
      assert(bi->getCondition() == bi->getOperand(0) && "Wrong operand index!");
      ref<Expr> cond = eval(ki, 0, state).value;
      cond = optimizer.optimizeExpr(cond, false);
      if (usingSeeds && (ResolvePath)) {
        cond = concretizeExpr(state, cond);
      }
      llvm::BasicBlock *dst_a = bi->getSuccessor(0);
      llvm::BasicBlock *dst_b = bi->getSuccessor(1);
      bool trueBranchDead = false;
      bool falseBranchDead = false;
      if (bbCheck) {
        trueBranchDead = cfg->checkDeadNode(dst_a);
        falseBranchDead = cfg->checkDeadNode(dst_b);
      }
      bool earlyTerminate = false;
      if (trueBranchDead && falseBranchDead) {
        SPDLOG_DEBUG("[state {}] DEAD NODE br true {} at {} and false {} at {}",
                     state.getID(), dst_a->getName().str(), sourceLoc,
                     dst_b->getName().str(), sourceLoc);
        earlyTerminate = true;
      } else if (trueBranchDead) {
        // Check false
        bool result = false;
        solver->setTimeout(coreSolverTimeout);
        solver->mayBeFalse(state, cond, result);
        if (result) {
          addConstraint(state, Expr::createIsZero(cond));
        } else {
          earlyTerminate = true;
          SPDLOG_DEBUG("[state {}] DEAD NODE br cannot be false {}",
                       state.getID(), cond->str());
        }
      } else if (falseBranchDead) {
        // Check true
        bool result = false;
        solver->setTimeout(coreSolverTimeout);
        solver->mayBeTrue(state, cond, result);
        if (result) {
          addConstraint(state, cond);
        } else {
          earlyTerminate = true;
          SPDLOG_DEBUG("[state {}] DEAD NODE br cannot be true {}",
                       state.getID(), cond->str());
        }
      }
      if (earlyTerminate) {
        terminateStateEarly(state, "Dead node");
        break;
      }
      bool hasTaint = state.passedTarget && eval(ki, 0, state).taint;
      setTaintMarker(hasTaint);
      Executor::StatePair branches = fork(state, cond, false);
      setTaintMarker(false);
      // NOTE: There is a hidden dependency here, markBranchVisited
      // requires that we still be in the context of the branch
      // instruction (it reuses its statistic id). Should be cleaned
      // up with convenient instruction specific data.
      if (statsTracker && state.stack.back().kf->trackCoverage)
        statsTracker->markBranchVisited(branches.first, branches.second);
      if (branches.first && branches.second) {
        KFunction *kf = state.stack.back().kf;
        llvm::BasicBlock *dst_a = bi->getSuccessor(0);
        llvm::BasicBlock *dst_b = bi->getSuccessor(1);
        klee_log_data(fmt::format(
            "[fork-loc] [br] [state {}] [loc {}] -> [state {}] [loc {}] "
            "[state {}] [loc {}]",
            state.getID(), sourceLoc, branches.first->getID(),
            getBBLocation(kf, dst_a), branches.second->getID(),
            getBBLocation(kf, dst_b)));
      }

      if (branches.first) {
        if (hasTaint) {
          for (auto &ts : branches.first->targetStack) {
            klee_log_data(fmt::format(
                "[regression-trace] [state {}] [n {}] [res 1] [loc {}]",
                state.getID(), ts.regressionTrace.size(), sourceLoc));
            ts.regressionTrace.push_back(true);
          }
        }
        bool jump = true;
        if (bbCheck) {
          const llvm::BasicBlock *dst = bi->getSuccessor(0);
          if (cfg->checkDeadNode(dst)) {
            SPDLOG_DEBUG("[state {}] DEAD NODE br true {} at {}", state.getID(),
                         dst->getName().str(), sourceLoc);
            terminateStateEarly(*branches.first, "Dead node");
            jump = false;
          }
        }
        if (jump) {
          transferToBasicBlock(bi->getSuccessor(0), bi->getParent(),
                               *branches.first);
        }
      }
      if (branches.second) {
        if (hasTaint) {
          for (auto &ts : branches.second->targetStack) {
            klee_log_data(fmt::format(
                "[regression-trace] [state {}] [n {}] [res 0] [loc {}]",
                state.getID(), ts.regressionTrace.size(), sourceLoc));
            ts.regressionTrace.push_back(false);
          }
        }
        bool jump = true;
        if (bbCheck) {
          const llvm::BasicBlock *dst = bi->getSuccessor(1);
          if (cfg->checkDeadNode(dst)) {
            SPDLOG_DEBUG("[state {}] DEAD NODE br false {} at {}",
                         branches.second->getID(), dst->getName().str(),
                         sourceLoc);
            terminateStateEarly(*branches.second, "Dead node");
            jump = false;
          }
        }
        if (jump) {
          transferToBasicBlock(bi->getSuccessor(1), bi->getParent(),
                               *branches.second);
        }
      }
    }
    break;
  }
  case Instruction::IndirectBr: {
    // implements indirect branch to a label within the current function
    const auto bi = cast<IndirectBrInst>(i);
    auto address = eval(ki, 0, state).value;
    address = toUnique(state, address);

    // concrete address
    if (const auto CE = dyn_cast<ConstantExpr>(address.get())) {
      const auto bb_address =
          (BasicBlock *)CE->getZExtValue(Context::get().getPointerWidth());
      transferToBasicBlock(bb_address, bi->getParent(), state);
      break;
    }

    // symbolic address
    const auto numDestinations = bi->getNumDestinations();
    std::vector<BasicBlock *> targets;
    targets.reserve(numDestinations);
    std::vector<ref<Expr>> expressions;
    expressions.reserve(numDestinations);

    ref<Expr> errorCase = ConstantExpr::alloc(1, Expr::Bool);
    SmallPtrSet<BasicBlock *, 5> destinations;
    // collect and check destinations from label list
    for (unsigned k = 0; k < numDestinations; ++k) {
      // filter duplicates
      const auto d = bi->getDestination(k);
      if (destinations.count(d))
        continue;
      destinations.insert(d);

      // create address expression
      const auto PE = Expr::createPointer(reinterpret_cast<std::uint64_t>(d));
      ref<Expr> e = EqExpr::create(address, PE);

      // exclude address from errorCase
      errorCase = AndExpr::create(errorCase, Expr::createIsZero(e));

      // check feasibility
      bool result;
      bool success __attribute__((unused)) =
          solver->mayBeTrue(state, e, result);
      assert(success && "FIXME: Unhandled solver failure");
      if (result) {
        targets.push_back(d);
        expressions.push_back(e);
      }
    }
    // check errorCase feasibility
    bool result;
    bool success __attribute__((unused)) =
        solver->mayBeTrue(state, errorCase, result);
    assert(success && "FIXME: Unhandled solver failure");
    if (result) {
      expressions.push_back(errorCase);
    }

    // fork states
    std::vector<ExecutionState *> branches;
    branch(state, expressions, branches);

    // terminate error state
    if (result) {
      terminateStateOnExecError(*branches.back(),
                                "indirectbr: illegal label address");
      branches.pop_back();
    }

    // branch states to resp. target blocks
    assert(targets.size() == branches.size());
    for (std::vector<ExecutionState *>::size_type k = 0; k < branches.size();
         ++k) {
      if (branches[k]) {
        transferToBasicBlock(targets[k], bi->getParent(), *branches[k]);
      }
    }

    break;
  }
  case Instruction::Switch: {
    SwitchInst *si = cast<SwitchInst>(i);
    ref<Expr> cond = eval(ki, 0, state).value;
    BasicBlock *bb = si->getParent();

    cond = toUnique(state, cond);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(cond)) {
      // Somewhat gross to create these all the time, but fine till we
      // switch to an internal rep.
      llvm::IntegerType *Ty = cast<IntegerType>(si->getCondition()->getType());
      ConstantInt *ci = ConstantInt::get(Ty, CE->getZExtValue());
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
      unsigned index = si->findCaseValue(ci)->getSuccessorIndex();
#else
      unsigned index = si->findCaseValue(ci).getSuccessorIndex();
#endif
      bool jump = true;
      if (bbCheck) {
        const llvm::BasicBlock *dst = si->getSuccessor(index);
        if (cfg->checkDeadNode(dst)) {
          SPDLOG_DEBUG("[state {}] DEAD NODE switch det {} at {}",
                       state.getID(), dst->getName().str(), sourceLoc);
          terminateStateEarly(state, "Dead node");
          jump = false;
        }
      }
      if (jump) {
        transferToBasicBlock(si->getSuccessor(index), si->getParent(), state);
      }
    } else {
      // Handle possible different branch targets

      // We have the following assumptions:
      // - each case value is mutual exclusive to all other values including the
      //   default value
      // - order of case branches is based on the order of the expressions of
      //   the scase values, still default is handled last
      std::vector<BasicBlock *> bbOrder;
      std::map<BasicBlock *, ref<Expr>> branchTargets;

      std::map<ref<Expr>, BasicBlock *> expressionOrder;

      // Iterate through all non-default cases and order them by expressions
#if LLVM_VERSION_CODE > LLVM_VERSION(3, 4)
      for (auto i : si->cases()) {
#else
      for (SwitchInst::CaseIt i = si->case_begin(), e = si->case_end(); i != e;
           ++i) {
#endif
        ref<Expr> value = evalConstant(i.getCaseValue());

        BasicBlock *caseSuccessor = i.getCaseSuccessor();
        bool addCase = true;
        if (bbCheck) {
          addCase = !cfg->checkDeadNode(caseSuccessor);
        }
        if (addCase) {
          expressionOrder.insert(std::make_pair(value, caseSuccessor));
        }
      }

      // Track default branch values
      ref<Expr> defaultValue = ConstantExpr::alloc(1, Expr::Bool);

      // iterate through all non-default cases but in order of the expressions
      for (std::map<ref<Expr>, BasicBlock *>::iterator
               it = expressionOrder.begin(),
               itE = expressionOrder.end();
           it != itE; ++it) {
        ref<Expr> match = EqExpr::create(cond, it->first);

        // Make sure that the default value does not contain this target's value
        defaultValue = AndExpr::create(defaultValue, Expr::createIsZero(match));

        // Check if control flow could take this case
        bool result;
        match = optimizer.optimizeExpr(match, false);
        bool success = solver->mayBeTrue(state, match, result);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (result) {
          BasicBlock *caseSuccessor = it->second;

          // Handle the case that a basic block might be the target of multiple
          // switch cases.
          // Currently we generate an expression containing all switch-case
          // values for the same target basic block. We spare us forking too
          // many times but we generate more complex condition expressions
          // TODO Add option to allow to choose between those behaviors
          std::pair<std::map<BasicBlock *, ref<Expr>>::iterator, bool> res =
              branchTargets.insert(std::make_pair(
                  caseSuccessor, ConstantExpr::alloc(0, Expr::Bool)));

          res.first->second = OrExpr::create(match, res.first->second);

          // Only add basic blocks which have not been target of a branch yet
          if (res.second) {
            bbOrder.push_back(caseSuccessor);
          }
        }
      }

      // Check if control could take the default case
      defaultValue = optimizer.optimizeExpr(defaultValue, false);
      BasicBlock *defaultBB = si->getDefaultDest();
      if (!bbCheck || !cfg->checkDeadNode(defaultBB)) {
        bool res;
        bool success = solver->mayBeTrue(state, defaultValue, res);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        if (res) {
          std::pair<std::map<BasicBlock *, ref<Expr>>::iterator, bool> ret =
              branchTargets.insert(
                  std::make_pair(si->getDefaultDest(), defaultValue));
          if (ret.second) {
            bbOrder.push_back(si->getDefaultDest());
          }
        }
      }

      // No branch target could be taken -> terminate state
      if (bbOrder.empty()) {
        terminateStateEarly(state, "No branch target could be taken");
        break;
      }

      // Fork the current state with each state having one of the possible
      // successors of this switch
      std::vector<ref<Expr>> conditions;
      for (std::vector<BasicBlock *>::iterator it = bbOrder.begin(),
                                               ie = bbOrder.end();
           it != ie; ++it) {
        conditions.push_back(branchTargets[*it]);
      }
      std::vector<ExecutionState *> branches;
      // Is it right to exclude all?
      setTaintMarker(state.passedTarget && eval(ki, 0, state).taint);
      branch(state, conditions, branches);
      setTaintMarker(false);
      llvm::BasicBlock *base_bb = nullptr;
      std::vector<ExecutionState *>::iterator bit = branches.begin();
      for (std::vector<BasicBlock *>::iterator it = bbOrder.begin(),
                                               ie = bbOrder.end();
           it != ie; ++it) {
        ExecutionState *es = *bit;
        if (es) {
          if (es->getID() == state.getID()) {
            base_bb = *it;
          }
          if (bbOrder.size() > 1) {
            klee_log_data(fmt::format(
                "[fork-loc] [sw] [state {}] [loc {}] -> [state {}] [loc {}] "
                "[state {}] "
                "[loc {}]",
                state.getID(), sourceLoc, state.getID(),
                getBBLocation(state.stack.back().kf, base_bb), es->getID(),
                getBBLocation(state.stack.back().kf, *it)));
          }
          transferToBasicBlock(*it, bb, *es);
        }

        ++bit;
      }
    }
    break;
  }
  case Instruction::Unreachable:
    // Note that this is not necessarily an internal bug, llvm will
    // generate unreachable instructions in cases where it knows the
    // program will crash. So it is effectively a SEGV or internal
    // error.
    terminateStateOnExecError(state, "reached \"unreachable\" instruction");
    break;

  case Instruction::Invoke:
  case Instruction::Call: {
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
    CallBase &cs = cast<CallBase>(*i);
    Value *fp = cs.getCalledOperand();
#else
    CallSite cs(i);
    Value *fp = cs.getCalledValue();
#endif
    unsigned numArgs = cs.arg_size();
    Function *f = getTargetFunction(fp, state);
    const FunctionType *fpType = dyn_cast<FunctionType>(
        cast<PointerType>(fp->getType())->getElementType());

    // Skip debug intrinsics, we can't evaluate their metadata arguments.
    if (f && isDebugIntrinsic(f, kmodule.get()))
      break;

    if (isa<InlineAsm>(fp)) {
      terminateStateOnExecError(state, "inline assembly is unsupported");
      break;
    }
    // evaluate arguments
    std::vector<ref<Expr>> arguments;
    arguments.reserve(numArgs);

    for (unsigned j = 0; j < numArgs; ++j)
      arguments.push_back(eval(ki, j + 1, state).value);

    if (f) { // Call function
      const FunctionType *fType = dyn_cast<FunctionType>(
          cast<PointerType>(f->getType())->getElementType());
      // special case the call with a bitcast case
      if (fType != fpType) {
        assert(fType && fpType && "unable to get function type");

        // XXX check result coercion
        // XXX this really needs thought and validation
        unsigned i = 0;
        for (std::vector<ref<Expr>>::iterator ai = arguments.begin(),
                                              ie = arguments.end();
             ai != ie; ++ai) {
          Expr::Width to, from = (*ai)->getWidth();

          if (i < fType->getNumParams()) {
            to = getWidthForLLVMType(fType->getParamType(i));

            if (from != to) {
              // XXX need to check other param attrs ?
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
              bool isSExt = cs.paramHasAttr(i, llvm::Attribute::SExt);
#else
              bool isSExt = cs.paramHasAttr(i + 1, llvm::Attribute::SExt);
#endif
              if (isSExt) {
                arguments[i] = SExtExpr::create(arguments[i], to);
              } else {
                arguments[i] = ZExtExpr::create(arguments[i], to);
              }
            }
          }

          i++;
        }
      }
      executeCall(state, ki, f, arguments);
    } else { // Function pointer
      ref<Expr> v = eval(ki, 0, state).value;
      ExecutionState *free = &state;
      bool hasInvalid = false, first = true;
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(v)) {
        uint64_t faddr = CE->getZExtValue();
        bool isLegal = legalFunctions.count(faddr) > 0;
        bool isSnapshotted = snapshottedFunctions.count(faddr) > 0;
        if (isLegal && isSnapshotted &&
            faddr != (uint64_t)snapshottedFunctions[faddr]) {
          // Address collision -> check type
          llvm::Function *fun = (llvm::Function *)faddr;
          const FunctionType *funt = dyn_cast<FunctionType>(
              cast<PointerType>(fun->getType())->getElementType());
          llvm::Function *real_function = snapshottedFunctions[faddr];
          const FunctionType *real_funt = dyn_cast<FunctionType>(
              cast<PointerType>(real_function->getType())->getElementType());
          SPDLOG_DEBUG("[state {}] Function pointer collision: {} vs {}",
                       state.getID(), fun->getName().str(),
                       real_function->getName().str());
          if (funt == fpType) {
            f = fun;
          } else if (real_funt == fpType) {
            f = snapshottedFunctions[faddr];
          } else {
            // Type mismatch.. Check once again
            // First, argument count
            bool fargMatch = fun->arg_size() == fpType->getNumParams();
            bool real_fargMatch =
                real_function->arg_size() == fpType->getNumParams();
            if (fargMatch && real_fargMatch) {
              // Check argument types
              fargMatch = true;
              real_fargMatch = true;
              for (unsigned i = 0; i < fpType->getNumParams(); i++) {
                Type *argType = fpType->getParamType(i);
                Type *fargType = fun->getFunctionType()->getParamType(i);
                Type *real_fargType =
                    real_function->getFunctionType()->getParamType(i);
                if (argType != fargType) {
                  fargMatch = false;
                }
                if (argType != real_fargType) {
                  real_fargMatch = false;
                }
              }
            }
            if (fargMatch) {
              f = fun;
            } else if (real_fargMatch) {
              f = real_function;
            } else { // Both are invalid
              SPDLOG_DEBUG("[state {}] Both {} and {} function type mismatch",
                           state.getID(), fun->getName().str(),
                           real_function->getName().str());
              f = fun;
            }
          }
        } else {
          if (isLegal) {
            f = (llvm::Function *)faddr;
          } else if (isSnapshotted) {
            f = snapshottedFunctions[faddr];
            SPDLOG_DEBUG("[state {}] Call function pointer from snapshotted "
                         "{}({} -> {})",
                         state.getID(), f->getName().str(), (uint64_t)faddr,
                         (uint64_t)f);
          } else {
            f = (llvm::Function *)faddr;
          }
        }
        if (!hasInvalid) {
          SPDLOG_DEBUG("[state {}] Call function pointer {} at {}",
                       state.getID(), f->getName().str(), sourceLoc);
          executeCall(state, ki, f, arguments);
        } else {
          terminateStateEarly(state, "invalid function pointer");
        }
        break;
      }

      /* XXX This is wasteful, no need to do a full evaluate since we
        have already got a value. But in the end the caches should
        handle it for us, albeit with some overhead. */
      SPDLOG_DEBUG("[state {}] Call symbolic function pointer {} at {}",
                   state.getID(), v->str(), sourceLoc);
      // Symbolic value
      std::vector<uint64_t> validFnPtrs;
      for (auto &func : kmodule->module->functions()) {
        // Check if the function is a valid function pointer
        if (!func.hasAddressTaken())
          continue;
        if (dyn_cast<FunctionType>(
                cast<PointerType>(func.getType())->getElementType()) ==
            fpType) {
          validFnPtrs.push_back((uint64_t)&func);
        }
      }
      if (validFnPtrs.empty()) {
        terminateStateEarly(state, "No valid function pointer found");
        break;
      }
      if (validFnPtrs.size() == 1) {
        f = (Function *)validFnPtrs[0];
        executeCall(state, ki, f, arguments);
      } else {
        // Let's branch
        std::vector<ref<Expr>> remainingValidFnPtrsExpr;
        std::vector<llvm::Function *> remainingValidFnPtrs;
        for (auto &ptr : validFnPtrs) {
          ref<Expr> e = EqExpr::create(
              v, ConstantExpr::create(ptr, Context::get().getPointerWidth()));
          Solver::Validity res;
          bool success = solver->evaluate(state, e, res);
          if (!success) {
            SPDLOG_ERROR("[state {}] Solver error {} at {}", state.getID(),
                         e->str(), sourceLoc);
            terminateStateEarly(state, "Solver error");
            return;
          }
          if (res == Solver::True) {
            f = (Function *)ptr;
            remainingValidFnPtrsExpr.clear();
            break;
          } else if (res == Solver::Unknown) {
            remainingValidFnPtrsExpr.push_back(e);
            remainingValidFnPtrs.push_back((llvm::Function *)ptr);
          }
        }
        if (remainingValidFnPtrsExpr.size() > 1) {
          std::vector<ExecutionState *> branches;
          branch(state, remainingValidFnPtrsExpr, branches);
          for (int i = 0; i < branches.size(); i++) {
            ExecutionState *es = branches[i];
            if (es == nullptr)
              continue;
            f = remainingValidFnPtrs[i];
            SPDLOG_DEBUG("[state {}] Call symbolic function pointer {}",
                         state.getID(), f->getName().str());
            executeCall(*es, ki, f, arguments);
          }
        } else {
          if (f == nullptr) {
            terminateStateEarly(state, "No valid function pointer found");
          } else {
            executeCall(state, ki, f, arguments);
          }
        }
      }
      // do {
      //   v = optimizer.optimizeExpr(v, true);
      //   ref<ConstantExpr> value;
      //   bool success = solver->getValue(*free, v, value);
      //   assert(success && "FIXME: Unhandled solver failure");
      //   (void)success;
      //   StatePair res = fork(*free, EqExpr::create(v, value), true);
      //   if (res.first) {
      //     uint64_t addr = value->getZExtValue();
      //     if (executionType == ExecutionType::FromSnapshot) {
      //       if (snapshottedFunctions.find(addr) !=
      //       snapshottedFunctions.end()) {
      //         llvm::Function *real_function = snapshottedFunctions[addr];
      //         addr = (uint64_t)real_function;
      //       }
      //     }
      //     if (legalFunctions.count(addr)) {
      //       f = (Function *)addr;

      //       // Don't give warning on unique resolution
      //       if (res.second || !first)
      //         klee_warning_once(reinterpret_cast<void *>(addr),
      //                           "resolved symbolic function pointer to: %s",
      //                           f->getName().data());

      //       executeCall(*res.first, ki, f, arguments);
      //     } else {
      //       if (!hasInvalid) {
      //         terminateStateOnExecError(state, "invalid function pointer");
      //         hasInvalid = true;
      //       }
      //     }
      //   }

      //   first = false;
      //   free = res.second;
      // } while (free);
    }
    break;
  }
  case Instruction::PHI: {
    ref<Expr> result = eval(ki, state.incomingBBIndex, state).value;
    bindLocal(ki, state, result);
    break;
  }

    // Special instructions
  case Instruction::Select: {
    // NOTE: It is not required that operands 1 and 2 be of scalar type.
    ref<Expr> cond = eval(ki, 0, state).value;
    ref<Expr> tExpr = eval(ki, 1, state).value;
    ref<Expr> fExpr = eval(ki, 2, state).value;
    ref<Expr> result = SelectExpr::create(cond, tExpr, fExpr);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::VAArg:
    terminateStateOnExecError(state, "unexpected VAArg instruction");
    break;

    // Arithmetic / logical

  case Instruction::Add: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    bindLocal(ki, state, AddExpr::create(left, right));
    break;
  }

  case Instruction::Sub: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    bindLocal(ki, state, SubExpr::create(left, right));
    break;
  }

  case Instruction::Mul: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    bindLocal(ki, state, MulExpr::create(left, right));
    break;
  }

  case Instruction::UDiv: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = UDivExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::SDiv: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = SDivExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::URem: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = URemExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::SRem: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = SRemExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::And: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = AndExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::Or: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = OrExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::Xor: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = XorExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::Shl: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = ShlExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::LShr: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = LShrExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::AShr: {
    ref<Expr> left = eval(ki, 0, state).value;
    ref<Expr> right = eval(ki, 1, state).value;
    ref<Expr> result = AShrExpr::create(left, right);
    bindLocal(ki, state, result);
    break;
  }

    // Compare

  case Instruction::ICmp: {
    CmpInst *ci = cast<CmpInst>(i);
    ICmpInst *ii = cast<ICmpInst>(ci);

    switch (ii->getPredicate()) {
    case ICmpInst::ICMP_EQ: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = EqExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_NE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = NeExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_UGT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UgtExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_UGE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UgeExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_ULT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UltExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_ULE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = UleExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SGT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SgtExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SGE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SgeExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SLT: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SltExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    case ICmpInst::ICMP_SLE: {
      ref<Expr> left = eval(ki, 0, state).value;
      ref<Expr> right = eval(ki, 1, state).value;
      ref<Expr> result = SleExpr::create(left, right);
      bindLocal(ki, state, result);
      break;
    }

    default:
      terminateStateOnExecError(state, "invalid ICmp predicate");
    }
    break;
  }

    // Memory instructions...
  case Instruction::Alloca: {
    AllocaInst *ai = cast<AllocaInst>(i);
    unsigned elementSize =
        kmodule->targetData->getTypeStoreSize(ai->getAllocatedType());
    ref<Expr> size = Expr::createPointer(elementSize);
    if (ai->isArrayAllocation()) {
      ref<Expr> count = eval(ki, 0, state).value;
      count = Expr::createZExtToPointerWidth(count);
      size = MulExpr::create(size, count);
    }
    executeAlloc(state, size, true, ki);
    break;
  }

  case Instruction::Load: {
    ref<Expr> base = eval(ki, 0, state).value;
    executeMemoryOperation(state, false, base, 0, ki);
    break;
  }
  case Instruction::Store: {
    ref<Expr> base = eval(ki, 1, state).value;
    ref<Expr> value = eval(ki, 0, state).value;
    // specialFunctionHandler->trackTaint(state, ki, value);
    executeMemoryOperation(state, true, base, value, ki);
    break;
  }

  case Instruction::GetElementPtr: {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(ki);
    bool tck = state.passedTarget;
    bool taint = tck ? eval(ki, 0, state).taint : false;
    ref<Expr> base = eval(ki, 0, state).value;
    for (std::vector<std::pair<unsigned, uint64_t>>::iterator
             it = kgepi->indices.begin(),
             ie = kgepi->indices.end();
         it != ie; ++it) {
      uint64_t elementSize = it->second;
      taint = tck ? taint || eval(ki, it->first, state).taint : false;
      ref<Expr> index = eval(ki, it->first, state).value;
      base = AddExpr::create(
          base, MulExpr::create(Expr::createSExtToPointerWidth(index),
                                Expr::createPointer(elementSize)));
    }
    if (kgepi->offset)
      base = AddExpr::create(base, Expr::createPointer(kgepi->offset));

    if (taint) {
      getDestCell(state, ki).taint = true;
    }
    // SPDLOG_DEBUG("[state {}] GEP {} -> {} at {}", state.getID(),
    //              eval(ki, 0, state).value->str(), base->str(), sourceLoc);
    bindLocal(ki, state, base);
    break;
  }

    // Conversion
  case Instruction::Trunc: {
    CastInst *ci = cast<CastInst>(i);
    ref<Expr> result = ExtractExpr::create(eval(ki, 0, state).value, 0,
                                           getWidthForLLVMType(ci->getType()));
    bindLocal(ki, state, result);
    break;
  }
  case Instruction::ZExt: {
    CastInst *ci = cast<CastInst>(i);
    ref<Expr> result = ZExtExpr::create(eval(ki, 0, state).value,
                                        getWidthForLLVMType(ci->getType()));
    bindLocal(ki, state, result);
    break;
  }
  case Instruction::SExt: {
    CastInst *ci = cast<CastInst>(i);
    ref<Expr> result = SExtExpr::create(eval(ki, 0, state).value,
                                        getWidthForLLVMType(ci->getType()));
    bindLocal(ki, state, result);
    break;
  }

  case Instruction::IntToPtr: {
    CastInst *ci = cast<CastInst>(i);
    Expr::Width pType = getWidthForLLVMType(ci->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    bindLocal(ki, state, ZExtExpr::create(arg, pType));
    break;
  }
  case Instruction::PtrToInt: {
    CastInst *ci = cast<CastInst>(i);
    Expr::Width iType = getWidthForLLVMType(ci->getType());
    ref<Expr> arg = eval(ki, 0, state).value;
    bindLocal(ki, state, ZExtExpr::create(arg, iType));
    break;
  }

  case Instruction::BitCast: {
    ref<Expr> result = eval(ki, 0, state).value;
    bindLocal(ki, state, result);
    break;
  }

    // Floating point instructions

  case Instruction::FAdd: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FAdd operation");

    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.add(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FSub: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FSub operation");
    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.subtract(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FMul: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FMul operation");

    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.multiply(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FDiv: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FDiv operation");

    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
    Res.divide(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

  case Instruction::FRem: {
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FRem operation");
    llvm::APFloat Res(*fpWidthToSemantics(left->getWidth()),
                      left->getAPValue());
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 8)
    Res.mod(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()));
#else
    Res.mod(
        APFloat(*fpWidthToSemantics(right->getWidth()), right->getAPValue()),
        APFloat::rmNearestTiesToEven);
#endif
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }

#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
  case Instruction::FNeg: {
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FNeg operation");

    llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    Res = llvm::neg(Res);
    bindLocal(ki, state, ConstantExpr::alloc(Res.bitcastToAPInt()));
    break;
  }
#endif

  case Instruction::FPTrunc: {
    FPTruncInst *fi = cast<FPTruncInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || resultType > arg->getWidth())
      return terminateStateOnExecError(state, "Unsupported FPTrunc operation");

    llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    bool losesInfo = false;
    Res.convert(*fpWidthToSemantics(resultType),
                llvm::APFloat::rmNearestTiesToEven, &losesInfo);
    bindLocal(ki, state, ConstantExpr::alloc(Res));
    break;
  }

  case Instruction::FPExt: {
    FPExtInst *fi = cast<FPExtInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || arg->getWidth() > resultType)
      return terminateStateOnExecError(state, "Unsupported FPExt operation");
    llvm::APFloat Res(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    bool losesInfo = false;
    Res.convert(*fpWidthToSemantics(resultType),
                llvm::APFloat::rmNearestTiesToEven, &losesInfo);
    bindLocal(ki, state, ConstantExpr::alloc(Res));
    break;
  }

  case Instruction::FPToUI: {
    FPToUIInst *fi = cast<FPToUIInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || resultType > 64)
      return terminateStateOnExecError(state, "Unsupported FPToUI operation");

    llvm::APFloat Arg(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());
    uint64_t value = 0;
    bool isExact = true;
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
    auto valueRef = makeMutableArrayRef(value);
#else
    uint64_t *valueRef = &value;
#endif
    Arg.convertToInteger(valueRef, resultType, false,
                         llvm::APFloat::rmTowardZero, &isExact);
    bindLocal(ki, state, ConstantExpr::alloc(value, resultType));
    break;
  }

  case Instruction::FPToSI: {
    FPToSIInst *fi = cast<FPToSIInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    if (!fpWidthToSemantics(arg->getWidth()) || resultType > 64)
      return terminateStateOnExecError(state, "Unsupported FPToSI operation");
    llvm::APFloat Arg(*fpWidthToSemantics(arg->getWidth()), arg->getAPValue());

    uint64_t value = 0;
    bool isExact = true;
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
    auto valueRef = makeMutableArrayRef(value);
#else
    uint64_t *valueRef = &value;
#endif
    Arg.convertToInteger(valueRef, resultType, true,
                         llvm::APFloat::rmTowardZero, &isExact);
    bindLocal(ki, state, ConstantExpr::alloc(value, resultType));
    break;
  }

  case Instruction::UIToFP: {
    UIToFPInst *fi = cast<UIToFPInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    const llvm::fltSemantics *semantics = fpWidthToSemantics(resultType);
    if (!semantics)
      return terminateStateOnExecError(state, "Unsupported UIToFP operation");
    llvm::APFloat f(*semantics, 0);
    f.convertFromAPInt(arg->getAPValue(), false,
                       llvm::APFloat::rmNearestTiesToEven);

    bindLocal(ki, state, ConstantExpr::alloc(f));
    break;
  }

  case Instruction::SIToFP: {
    SIToFPInst *fi = cast<SIToFPInst>(i);
    Expr::Width resultType = getWidthForLLVMType(fi->getType());
    ref<ConstantExpr> arg =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    const llvm::fltSemantics *semantics = fpWidthToSemantics(resultType);
    if (!semantics)
      return terminateStateOnExecError(state, "Unsupported SIToFP operation");
    llvm::APFloat f(*semantics, 0);
    f.convertFromAPInt(arg->getAPValue(), true,
                       llvm::APFloat::rmNearestTiesToEven);

    bindLocal(ki, state, ConstantExpr::alloc(f));
    break;
  }

  case Instruction::FCmp: {
    FCmpInst *fi = cast<FCmpInst>(i);
    ref<ConstantExpr> left =
        toConstant(state, eval(ki, 0, state).value, "floating point");
    ref<ConstantExpr> right =
        toConstant(state, eval(ki, 1, state).value, "floating point");
    if (!fpWidthToSemantics(left->getWidth()) ||
        !fpWidthToSemantics(right->getWidth()))
      return terminateStateOnExecError(state, "Unsupported FCmp operation");

    APFloat LHS(*fpWidthToSemantics(left->getWidth()), left->getAPValue());
    APFloat RHS(*fpWidthToSemantics(right->getWidth()), right->getAPValue());
    APFloat::cmpResult CmpRes = LHS.compare(RHS);

    bool Result = false;
    switch (fi->getPredicate()) {
    // Predicates which only care about whether or not the operands are NaNs.
    case FCmpInst::FCMP_ORD:
      Result = (CmpRes != APFloat::cmpUnordered);
      break;

    case FCmpInst::FCMP_UNO:
      Result = (CmpRes == APFloat::cmpUnordered);
      break;

      // Ordered comparisons return false if either operand is NaN.  Unordered
      // comparisons return true if either operand is NaN.
    case FCmpInst::FCMP_UEQ:
      Result = (CmpRes == APFloat::cmpUnordered || CmpRes == APFloat::cmpEqual);
      break;
    case FCmpInst::FCMP_OEQ:
      Result = (CmpRes != APFloat::cmpUnordered && CmpRes == APFloat::cmpEqual);
      break;

    case FCmpInst::FCMP_UGT:
      Result = (CmpRes == APFloat::cmpUnordered ||
                CmpRes == APFloat::cmpGreaterThan);
      break;
    case FCmpInst::FCMP_OGT:
      Result = (CmpRes != APFloat::cmpUnordered &&
                CmpRes == APFloat::cmpGreaterThan);
      break;

    case FCmpInst::FCMP_UGE:
      Result =
          (CmpRes == APFloat::cmpUnordered ||
           (CmpRes == APFloat::cmpGreaterThan || CmpRes == APFloat::cmpEqual));
      break;
    case FCmpInst::FCMP_OGE:
      Result =
          (CmpRes != APFloat::cmpUnordered &&
           (CmpRes == APFloat::cmpGreaterThan || CmpRes == APFloat::cmpEqual));
      break;

    case FCmpInst::FCMP_ULT:
      Result =
          (CmpRes == APFloat::cmpUnordered || CmpRes == APFloat::cmpLessThan);
      break;
    case FCmpInst::FCMP_OLT:
      Result =
          (CmpRes != APFloat::cmpUnordered && CmpRes == APFloat::cmpLessThan);
      break;

    case FCmpInst::FCMP_ULE:
      Result =
          (CmpRes == APFloat::cmpUnordered ||
           (CmpRes == APFloat::cmpLessThan || CmpRes == APFloat::cmpEqual));
      break;
    case FCmpInst::FCMP_OLE:
      Result =
          (CmpRes != APFloat::cmpUnordered &&
           (CmpRes == APFloat::cmpLessThan || CmpRes == APFloat::cmpEqual));
      break;

    case FCmpInst::FCMP_UNE:
      Result = (CmpRes == APFloat::cmpUnordered || CmpRes != APFloat::cmpEqual);
      break;
    case FCmpInst::FCMP_ONE:
      Result = (CmpRes != APFloat::cmpUnordered && CmpRes != APFloat::cmpEqual);
      break;

    default:
      assert(0 && "Invalid FCMP predicate!");
      break;
    case FCmpInst::FCMP_FALSE:
      Result = false;
      break;
    case FCmpInst::FCMP_TRUE:
      Result = true;
      break;
    }

    bindLocal(ki, state, ConstantExpr::alloc(Result, Expr::Bool));
    break;
  }
  case Instruction::InsertValue: {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(ki);

    ref<Expr> agg = eval(ki, 0, state).value;
    ref<Expr> val = eval(ki, 1, state).value;

    ref<Expr> l = NULL, r = NULL;
    unsigned lOffset = kgepi->offset * 8,
             rOffset = kgepi->offset * 8 + val->getWidth();

    if (lOffset > 0)
      l = ExtractExpr::create(agg, 0, lOffset);
    if (rOffset < agg->getWidth())
      r = ExtractExpr::create(agg, rOffset, agg->getWidth() - rOffset);

    ref<Expr> result;
    if (!l.isNull() && !r.isNull())
      result = ConcatExpr::create(r, ConcatExpr::create(val, l));
    else if (!l.isNull())
      result = ConcatExpr::create(val, l);
    else if (!r.isNull())
      result = ConcatExpr::create(r, val);
    else
      result = val;

    bindLocal(ki, state, result);
    break;
  }
  case Instruction::ExtractValue: {
    KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(ki);

    ref<Expr> agg = eval(ki, 0, state).value;

    ref<Expr> result = ExtractExpr::create(agg, kgepi->offset * 8,
                                           getWidthForLLVMType(i->getType()));

    bindLocal(ki, state, result);
    break;
  }
  case Instruction::Fence: {
    // Ignore for now
    break;
  }
  case Instruction::InsertElement: {
    InsertElementInst *iei = cast<InsertElementInst>(i);
    ref<Expr> vec = eval(ki, 0, state).value;
    ref<Expr> newElt = eval(ki, 1, state).value;
    ref<Expr> idx = eval(ki, 2, state).value;

    ConstantExpr *cIdx = dyn_cast<ConstantExpr>(idx);
    if (cIdx == NULL) {
      terminateStateOnError(
          state, "InsertElement, support for symbolic index not implemented",
          Unhandled);
      return;
    }
    uint64_t iIdx = cIdx->getZExtValue();
#if LLVM_VERSION_CODE >= LLVM_VERSION(11, 0)
    const auto *vt = cast<llvm::FixedVectorType>(iei->getType());
#else
    const llvm::VectorType *vt = iei->getType();
#endif
    unsigned EltBits = getWidthForLLVMType(vt->getElementType());

    if (iIdx >= vt->getNumElements()) {
      // Out of bounds write
      terminateStateOnError(state, "Out of bounds write when inserting element",
                            BadVectorAccess);
      return;
    }

    const unsigned elementCount = vt->getNumElements();
    llvm::SmallVector<ref<Expr>, 8> elems;
    elems.reserve(elementCount);
    for (unsigned i = elementCount; i != 0; --i) {
      auto of = i - 1;
      unsigned bitOffset = EltBits * of;
      elems.push_back(
          of == iIdx ? newElt : ExtractExpr::create(vec, bitOffset, EltBits));
    }

    assert(Context::get().isLittleEndian() && "FIXME:Broken for big endian");
    ref<Expr> Result = ConcatExpr::createN(elementCount, elems.data());
    bindLocal(ki, state, Result);
    break;
  }
  case Instruction::ExtractElement: {
    ExtractElementInst *eei = cast<ExtractElementInst>(i);
    ref<Expr> vec = eval(ki, 0, state).value;
    ref<Expr> idx = eval(ki, 1, state).value;

    ConstantExpr *cIdx = dyn_cast<ConstantExpr>(idx);
    if (cIdx == NULL) {
      terminateStateOnError(
          state, "ExtractElement, support for symbolic index not implemented",
          Unhandled);
      return;
    }
    uint64_t iIdx = cIdx->getZExtValue();
#if LLVM_VERSION_CODE >= LLVM_VERSION(11, 0)
    const auto *vt = cast<llvm::FixedVectorType>(eei->getVectorOperandType());
#else
    const llvm::VectorType *vt = eei->getVectorOperandType();
#endif
    unsigned EltBits = getWidthForLLVMType(vt->getElementType());

    if (iIdx >= vt->getNumElements()) {
      // Out of bounds read
      terminateStateOnError(state, "Out of bounds read when extracting element",
                            BadVectorAccess);
      return;
    }

    unsigned bitOffset = EltBits * iIdx;
    ref<Expr> Result = ExtractExpr::create(vec, bitOffset, EltBits);
    bindLocal(ki, state, Result);
    break;
  }
  case Instruction::ShuffleVector:
    // Should never happen due to Scalarizer pass removing ShuffleVector
    // instructions.
    terminateStateOnExecError(state, "Unexpected ShuffleVector instruction");
    break;
  case Instruction::AtomicRMW:
    terminateStateOnExecError(state, "Unexpected Atomic instruction, should be "
                                     "lowered by LowerAtomicInstructionPass");
    break;
  case Instruction::AtomicCmpXchg:
    terminateStateOnExecError(state,
                              "Unexpected AtomicCmpXchg instruction, should be "
                              "lowered by LowerAtomicInstructionPass");
    break;
    // Other instructions...
    // Unhandled
  default:
    terminateStateOnExecError(state, "illegal instruction");
    break;
  }
}

void Executor::updateStates(ExecutionState *current) {

  // if (UseExtractFix) {
  //   if (current) {
  //     if (current->passedCrashLoc && current->passedTarget) {
  //       // Terminate
  //       bool terminate = true;
  //       for (ExecutionState *s : removedStates) {
  //         if (s == current) {
  //           terminate = false;
  //           break;
  //         }
  //       }
  //       if (terminate) {
  //         for (ExecutionState *s : pausedStates) {
  //           if (s == current) {
  //             terminate = false;
  //             break;
  //           }
  //         }
  //       }
  //       if (terminate)
  //         terminateStateOnExtractfixCrashLoc(*current);
  //     }
  //   }
  // }

  if (addedStates.size() > 0) {
    std::stringstream ss;
    for (ExecutionState *es : addedStates) {
      ss << es->getID() << " ";
    }
    SPDLOG_DEBUG("[states {}] Added states: {}", states.size(), ss.str());
  }
  if (removedStates.size() > 0) {
    std::stringstream ss;
    for (ExecutionState *es : removedStates) {
      ss << es->getID() << " ";
    }
    SPDLOG_DEBUG("[states {}] Removed states: {}", states.size(), ss.str());
  }
  if (pausedStates.size() > 0) {
    std::stringstream ss;
    for (ExecutionState *es : pausedStates) {
      ss << es->getID() << " ";
    }
    SPDLOG_DEBUG("[states {}] Paused states: {}", states.size(), ss.str());
  }
  if (continuedStates.size() > 0) {
    std::stringstream ss;
    for (ExecutionState *es : continuedStates) {
      ss << es->getID() << " ";
    }
    SPDLOG_DEBUG("[states {}] Continued states: {}", states.size(), ss.str());
  }
  if (stateGroup.getPatchedStatesSize() > 0) {
    if (stateGroup.shouldPopInput()) {
      stateGroup.popInput();
      if (addedStates.size() > 0) {
        std::stringstream ss;
        for (ExecutionState *es : addedStates) {
          ss << es->getID() << " ";
        }
        SPDLOG_DEBUG("[pop] [states {}] Added states: {}", states.size(),
                     ss.str());
      }
      if (removedStates.size() > 0) {
        std::stringstream ss;
        for (ExecutionState *es : removedStates) {
          auto pit = std::find(pausedStates.begin(), pausedStates.end(), es);
          if (pit != pausedStates.end()) {
            pausedStates.erase(pit);
          }
          ss << es->getID() << " ";
        }
        SPDLOG_DEBUG("[pop] [states {}] Removed states: {}", states.size(),
                     ss.str());
      }
      if (pausedStates.size() > 0) {
        std::stringstream ss;
        for (ExecutionState *es : pausedStates) {
          ss << es->getID() << " ";
        }
        SPDLOG_DEBUG("[pop] [states {}] Paused states: {}", states.size(),
                     ss.str());
      }
      if (continuedStates.size() > 0) {
        std::stringstream ss;
        for (ExecutionState *es : continuedStates) {
          ss << es->getID() << " ";
        }
        SPDLOG_DEBUG("[pop] [states {}] Continued states: {}", states.size(),
                     ss.str());
      }
    }
  }

  if (current != nullptr &&
      current->metaData.isType(StateType::ST_crash_test) &&
      current->cost > time::minutes(20)) {
    auto it = std::find(pausedStates.begin(), pausedStates.end(), current);
    if (it != pausedStates.end()) {
      pausedStates.erase(it);
      SPDLOG_DEBUG("Remove paused state({})", current->getID());
    }
    auto it2 = std::find(removedStates.begin(), removedStates.end(), current);
    if (it2 == removedStates.end()) {
      SPDLOG_DEBUG("[state {}] Removed because of timeout", current->getID());
      terminateStateEarly(*current, "timeout");
    }
  }

  if (searcher) {
    searcher->update(current, addedStates, removedStates);
  }

  states.insert(addedStates.begin(), addedStates.end());
  addedStates.clear();

  for (std::vector<ExecutionState *>::iterator it = removedStates.begin(),
                                               ie = removedStates.end();
       it != ie; ++it) {
    ExecutionState *es = *it;
    std::set<ExecutionState *>::iterator it2 = states.find(es);
    assert(it2 != states.end());
    states.erase(it2);
    std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it3 =
        seedMap.find(es);
    if (it3 != seedMap.end())
      seedMap.erase(it3);
    processTree->remove(es->ptreeNode);
    if (stateGroup.checkDeleteState(es))
      delete es;
  }
  removedStates.clear();

  // If all patched states are consumed, continue all paused states
  if (isAllStatePaused()) {
    SPDLOG_DEBUG("All patched states are consumed, continue all paused "
                 "states({})",
                 states.size());
    stateGroup.continueAll();
  }

  if (searcher) {
    searcher->update(nullptr, continuedStates, pausedStates);
    pausedStates.clear();
    continuedStates.clear();
  }
}

template <typename TypeIt>
void Executor::computeOffsets(KGEPInstruction *kgepi, TypeIt ib, TypeIt ie) {
  ref<ConstantExpr> constantOffset =
      ConstantExpr::alloc(0, Context::get().getPointerWidth());
  uint64_t index = 1;
  for (TypeIt ii = ib; ii != ie; ++ii) {
    if (StructType *st = dyn_cast<StructType>(*ii)) {
      const StructLayout *sl = kmodule->targetData->getStructLayout(st);
      const ConstantInt *ci = cast<ConstantInt>(ii.getOperand());
      uint64_t addend = sl->getElementOffset((unsigned)ci->getZExtValue());
      constantOffset = constantOffset->Add(
          ConstantExpr::alloc(addend, Context::get().getPointerWidth()));
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
    } else if (isa<ArrayType>(*ii)) {
      computeOffsetsSeqTy<ArrayType>(kgepi, constantOffset, index, ii);
    } else if (isa<VectorType>(*ii)) {
      computeOffsetsSeqTy<VectorType>(kgepi, constantOffset, index, ii);
    } else if (isa<PointerType>(*ii)) {
      computeOffsetsSeqTy<PointerType>(kgepi, constantOffset, index, ii);
#else
    } else if (isa<SequentialType>(*ii)) {
      computeOffsetsSeqTy<SequentialType>(kgepi, constantOffset, index, ii);
#endif
    } else
      assert("invalid type" && 0);
    index++;
  }
  kgepi->offset = constantOffset->getZExtValue();
}

void Executor::bindInstructionConstants(KInstruction *KI) {
  KGEPInstruction *kgepi = static_cast<KGEPInstruction *>(KI);

  if (GetElementPtrInst *gepi = dyn_cast<GetElementPtrInst>(KI->inst)) {
    computeOffsets(kgepi, gep_type_begin(gepi), gep_type_end(gepi));
  } else if (InsertValueInst *ivi = dyn_cast<InsertValueInst>(KI->inst)) {
    computeOffsets(kgepi, iv_type_begin(ivi), iv_type_end(ivi));
    assert(kgepi->indices.empty() && "InsertValue constant offset expected");
  } else if (ExtractValueInst *evi = dyn_cast<ExtractValueInst>(KI->inst)) {
    computeOffsets(kgepi, ev_type_begin(evi), ev_type_end(evi));
    assert(kgepi->indices.empty() && "ExtractValue constant offset expected");
  }
}

void Executor::bindModuleConstants() {
  for (auto &kfp : kmodule->functions) {
    KFunction *kf = kfp.get();
    for (unsigned i = 0; i < kf->numInstructions; ++i)
      bindInstructionConstants(kf->instructions[i]);
  }

  kmodule->constantTable =
      std::unique_ptr<Cell[]>(new Cell[kmodule->constants.size()]);
  for (unsigned i = 0; i < kmodule->constants.size(); ++i) {
    Cell &c = kmodule->constantTable[i];
    c.value = evalConstant(kmodule->constants[i]);
  }
}

void Executor::checkMemoryUsage() {
  if (!MaxMemory)
    return;
  if ((stats::instructions & 0xFFFF) == 0) {
    // We need to avoid calling GetTotalMallocUsage() often because it
    // is O(elts on freelist). This is really bad since we start
    // to pummel the freelist once we hit the memory cap.
    unsigned mbs = (util::GetTotalMallocUsage() >> 20) +
                   (memory->getUsedDeterministicSize() >> 20);

    if (mbs > MaxMemory) {
      if (mbs > MaxMemory + 100) {
        // just guess at how many to kill
        unsigned numStates = states.size();
        unsigned toKill = std::max(1U, numStates - numStates * MaxMemory / mbs);
        klee_warning("killing %d states (over memory cap)", toKill);
        std::vector<ExecutionState *> arr(states.begin(), states.end());
        for (unsigned i = 0, N = arr.size(); N && i < toKill; ++i, --N) {
          unsigned idx = rand() % N;
          // Make two pulls to try and not hit a state that
          // covered new code.
          if (arr[idx]->coveredNew)
            idx = rand() % N;

          std::swap(arr[idx], arr[N - 1]);
          terminateStateEarly(*arr[N - 1], "Memory limit exceeded.");
        }
      }
      atMemoryLimit = true;
    } else {
      atMemoryLimit = false;
    }
  }
}

void Executor::doDumpStates() {
  if (!DumpStatesOnHalt || states.empty())
    return;

  klee_message("halting execution, dumping remaining states");
  for (const auto &state : states)
    terminateStateEarly(*state, "Execution halting.");
  updateStates(nullptr);
}

void Executor::toGraphviz(std::string filename,
                          std::map<uint64_t, std::vector<uint64_t>> &graph) {
  // TODO: change to use memNodeMap
  std::ofstream of(filename.c_str());
  of << "digraph G {\n";
  // Iterate over all nodes in memoryNodeMap
  for (auto &mnode : graph) {
    of << "  \"" << pointerToBigEndianString(mnode.first)
       << "\" [shape=record\tlabel=\"{" << pointerToBigEndianString(mnode.first)
       << "|(" << pointerToDataString(mnode.first) << ")}\"]\n";
  }
  for (auto &mnode : graph) {
    std::stringstream ss;
    ss << "  \"" << pointerToBigEndianString(mnode.first) << "\" -> \"";
    std::string from = ss.str();
    // Find all outgoing edges
    for (auto edge : mnode.second) {
      of << from << pointerToBigEndianString(edge) << "\"\n";
    }
  }
  of << "}\n";
  of.close();
}

Snapshot *Executor::getSnapshot(ExecutionState &state, llvm::Function *f,
                                KInstruction *ki,
                                std::vector<ref<Expr>> &arguments,
                                int snapshotNo, std::string snapshotName) {
  SPDLOG_INFO("[state {}] Creating snapshot for function {} - {}",
              state.getID(), f->getName().str(), snapshotNo);
  std::vector<std::string> args;
  interpreterHandler->getArgs(args);
  std::vector<ref<ConstantExpr>> constantArgs;
  for (auto &arg : arguments) {
    if (isa<ConstantExpr>(arg)) {
      constantArgs.push_back(dyn_cast<ConstantExpr>(arg));
    } else {
      // constantArgs.push_back(ConstantExpr::create(0, arg->getWidth()));
      ref<Expr> ce = concretizeExpr(state, arg);
      if (isa<ConstantExpr>(ce)) {
        constantArgs.push_back(dyn_cast<ConstantExpr>(ce));
      } else {
        SPDLOG_WARN("Cannot concretize argument {}", ce->str());
        constantArgs.push_back(ConstantExpr::create(0, arg->getWidth()));
      }
    }
  }
  MemGraph *pruned = memGraph.prune();
  Snapshot *snapshot =
      new Snapshot(f->getName().str(), &state, constantArgs, ki->info->file,
                   args, snapshotNo, legalFunctions, externalObjects, *pruned);
  delete pruned;
  if (baseSnapshot != nullptr) {
    for (auto &it : baseSnapshot->fn_ptrs_map) {
      if (snapshot->fn_ptrs_map.find(it.first) != snapshot->fn_ptrs_map.end()) {
        SPDLOG_DEBUG("Conflicting function pointer {} for base {} and new {}",
                     pointerToDataString(it.first), it.second,
                     snapshot->fn_ptrs_map[it.first]);
      }
      snapshot->fn_ptrs_map[it.first] = it.second;
    }
  }
  if (!snapshotName.empty()) {
    snapshot->stateId = state.getID();
    snapshot->patchId = state.metaData.getPatchID();
    snapshot->dump(snapshotName);
  }
  return snapshot;
}

Snapshot *Executor::applyDynamicAnalysisToSnapshot(ExecutionState &state,
                                                   Snapshot *snapshot,
                                                   StackFrame &sf,
                                                   std::string msg,
                                                   std::string suffix) {
  snapshot->afterExit(&state, msg, suffix, sf.regressionTrace,
                      accessedByExternalCall, sf.readAccessMap,
                      sf.writeAccessMap);
  // Update snapshot->memGraph using readAccessMap and writeAccessMap
  MemGraph &smg = snapshot->memGraph;
  for (auto &it : sf.readAccessMap) {
    uint64_t address = it.first;
    MemRecord &mr = it.second.first;
    PtrEdge *ptrEdge = it.second.second;
    if (ptrEdge == nullptr ||
        sf.writeAccessMap.find(address) != sf.writeAccessMap.end()) {
      continue; // Skip if non-pointer access or write access exists
    }
    // Newly found ptrEdge by readAccess: add to this graph
    MemNode *srcMemNode = smg.getMemNode(mr.base);
    if (srcMemNode == nullptr) {
      continue; // Skip for non-existing MemNode
    }
    PtrNode *srcPtrNode = ptrEdge->getFrom();
    PtrNode *dstPtrNode = ptrEdge->getTo();
    MemNode *dstMemNode = smg.getMemNode(dstPtrNode->getBaseAddr());
    if (dstMemNode == nullptr) {
      continue; // Skip for non-existing MemNode
    }
    // Add newly found PtrEdge & MemEdge
    smg.addPtrNode(srcPtrNode);
    smg.addPtrNode(dstPtrNode);
    smg.addPtrEdge(srcPtrNode, dstPtrNode);
    smg.addMemEdge(srcMemNode, dstMemNode, ptrEdge);
  }
  return snapshot;
}

// Handle the case for target function
void Executor::executeInstructionWrapper(ExecutionState &state,
                                         KInstruction *ki) {
  Instruction *i = ki->inst;
#ifdef ENABLE_DG
  ki->isSliced = slicingInfo->isSliced(ki);
#endif
  std::string sourceLoc = ki->getSourceLocation();
  if (!isKleeLibFunction(sourceLoc) && !state.targetStack.empty()) {
    if (ki->inst->isTerminator()) {
      klee_record_bb_trace(state.getID(), ki->info->assemblyLine,
                           i->getOpcode(),
                           state.stack.back().kf->function->getName().str());
    }
  }
  bool runInstruction = true;
  if (CrashLoc != 0 && ki->info->assemblyLine == CrashLoc) {
    state.passedCrashLoc = true;
  }
  switch (i->getOpcode()) {
  case Instruction::Ret: {
    if (targetFunctionAddr == i->getFunction()) {
      state.recursiveDepth -= 1;
      SPDLOG_DEBUG("[state {}] Return from target function {} at depth {}",
                   state.getID(), i->getFunction()->getName().str(),
                   state.recursiveDepth);
    }
    if (targetFunctionAddr != 0 && targetFunctionAddr == i->getFunction() &&
        !state.targetStack.empty()) {
      if (DumpSnapshot || executionType == ExecutionType::MakeSnapshot) {
        Snapshot *funcSnapshot = state.targetStack.back().snapshot;
        int snapshotId = funcSnapshot->snapshotId;
        if (DumpSnapshot) {
          std::vector<ref<Expr>> arguments;
          for (auto &it : funcSnapshot->arguments) {
            arguments.push_back(it);
          }
          Snapshot *newSnapshot = getSnapshot(state, i->getFunction(), ki,
                                              arguments, snapshotId, "");
          newSnapshot->regressionTrace = funcSnapshot->regressionTrace;
          funcSnapshot = newSnapshot;
        }
        funcSnapshot = applyDynamicAnalysisToSnapshot(
            state, funcSnapshot, state.targetStack.back(), "return", "ret");
        std::string filename = funcSnapshot->getSnapshotName(&state, "ret");
        funcSnapshot->dump(interpreterHandler->getOutputFilename(filename));
      }
      // FIXME: is it correct to terminate at return?
      SPDLOG_DEBUG("[state {}] Return from target function {}", state.getID(),
                   sourceLoc);
      if (executionType == ExecutionType::InsideTargetFunction ||
          executionType == ExecutionType::FromSnapshotFork) {
        if (state.targetStack.size() > 1) {
          state.targetStack.pop_back();
        } else {
          if (state.stack.empty()) {
            terminateStateOnReturn(state, "Reached target function return");
            runInstruction = false;
          }
        }
      }
      if (executionType == ExecutionType::FromSnapshot ||
          executionType == ExecutionType::UnderConstrained) {
        if (state.recursiveDepth == 0) {
          terminateStateOnReturn(state, "Reached target function return");
          runInstruction = false;
        }
      }
    }

    break;
  }
  case Instruction::Invoke: // fallthrough
  case Instruction::Call: {
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
    CallBase &cs = cast<CallBase>(*i);
    Value *fp = cs.getCalledOperand();
    FunctionType *ft = cast<FunctionType>(cs.getFunctionType());
#else
    CallSite cs(i);
    Value *fp = cs.getCalledValue();
    FunctionType *ft = cast<FunctionType>(cast<CallInst>(i)->getFunctionType());
#endif
    unsigned numArgs = cs.arg_size();
    if (isa<InlineAsm>(fp))
      break;
    Function *f = getTargetFunction(fp, state);
    // if (f == nullptr) {
    //   ref<Expr> addr = eval(ki, 0, state).value;
    //   if (ConstantExpr *CE = dyn_cast<ConstantExpr>(addr)) {
    //     Function *ptr = (Function *)CE->getZExtValue();
    //     if (kmodule->functionMap.find(ptr) != kmodule->functionMap.end()) {
    //       f = ptr;
    //     }
    //   }
    // }
    // Function pointer and debug intrinsic is not handled in here
    if (f == nullptr || isDebugIntrinsic(f, kmodule.get()))
      break;
    std::string funcName = f->getName().str();
    if (!isKleeLibFunction(sourceLoc) && !state.targetStack.empty()) {
      klee_record_bb_trace(state.getID(), ki->info->assemblyLine,
                           i->getOpcode(),
                           state.stack.back().kf->function->getName().str());
    }
    if (targetFunctionAddr == f) {
      state.recursiveDepth += 1;
      SPDLOG_DEBUG("[state {}] Call target function {} at depth {}",
                   state.getID(), funcName.c_str(), state.recursiveDepth);
    }
    KFunction *kf = kmodule->functionMap[f];
    if ((executionType == ExecutionType::FromSnapshotFork) &&
        targetFunctionAddr != 0) {
      if (targetFunctionAddr == f) { // reached target function
        // TODO: split this into a separate function
        int currentSnapshotId = interpreterHandler->getSnapshotNumber();
        if (!DumpSnapshot)
          interpreterHandler->getSnapshotName();
        if (baseSnapshot->snapshotId == currentSnapshotId) {
          SPDLOG_INFO("[state {}] Reached snapshot location {}:{}",
                      state.getID(), funcName.c_str(), currentSnapshotId);
          initUniKleeAtEntry(state, kf, baseSnapshot);
          std::vector<ref<Expr>> arguments;
          makeSymbolicFromSnapshot(state, baseSnapshot, f, arguments);
          // Run the target function from snapshot
          state.targetStack.push_back(state.stack.back());
          state.targetStack.back().snapshot = baseSnapshot;
          executionType = ExecutionType::InsideTargetFunction;
          executeCall(state, ki, f, arguments);
          runInstruction = false;
        }
      }
    }
    if (patchLocationAddr == f) {
      state.patchDataManager.countPatchLocation();
      SPDLOG_DEBUG("[state {}] countPatchLocation {}", state.getID(),
                   state.patchDataManager.getCount());
    }
    SPDLOG_TRACE("Call FN {} at {}", f->getName().str(), sourceLoc);
    break;
  }
  case Instruction::BitCast: {
    if (executionType != ExecutionType::MakeSnapshot && !DumpSnapshot) {
      break;
    }
    CastInst *ci = cast<CastInst>(i);
    const llvm::Type *bitcastType = ci->getType();
    ref<Expr> result = eval(ki, 0, state).value;
    // llvm uses i8* to represent void*
    const llvm::Type *voidType = llvm::Type::getInt8PtrTy(i->getContext());
    if (ci->getSrcTy() == voidType) {
      SPDLOG_TRACE("Bitcast from void pointer type: {}",
                   get_type_string(bitcastType));
      if (bitcastType->isPointerTy()) {
        if (isa<ConstantExpr>(result)) {
          ref<ConstantExpr> ce = dyn_cast<ConstantExpr>(result);
          uint64_t addr = ce->getZExtValue();
          const llvm::PointerType *ptrType =
              cast<llvm::PointerType>(bitcastType);
          const llvm::Type *elemType = ptrType->getElementType();
          SPDLOG_TRACE("Bitcast to pointer type: {}",
                       get_type_string(elemType));
          MemNode *mn = memGraph.getMemNode(addr);
          if (mn) {
            memGraph.addPtrNode(addr, mn);
          }
        }
      }
    } else {
      SPDLOG_TRACE("Bitcast from: {} to: {}", get_type_string(ci->getSrcTy()),
                   get_type_string(bitcastType));
    }
    break;
  }
  default:
    break;
  }
  // Handle taint tracking
  if (state.passedTarget) {
    unsigned opcode = i->getOpcode();
    if (i->isBinaryOp() || opcode == Instruction::ICmp) {
      const Cell &left = eval(ki, 0, state);
      const Cell &right = eval(ki, 1, state);
      Cell &result = getDestCell(state, ki);
      if (left.taint || right.taint) {
        result.taint = true;
      }
      for (const MemRecord &mr : left.mr) {
        result.mr.insert(mr);
      }
      for (const MemRecord &mr : right.mr) {
        result.mr.insert(mr);
      }
    } else if (i->isCast() || i->isUnaryOp() || opcode == Instruction::PHI) {
      const Cell &source = eval(ki, 0, state);
      Cell &target = getDestCell(state, ki);
      if (source.taint) {
        target.taint = true;
      }
      for (const MemRecord &mr : source.mr) {
        target.mr.insert(mr);
      }
    } else if (opcode == Instruction::Select) {
      const Cell &cond = eval(ki, 0, state);
      const Cell &tCell = eval(ki, 1, state);
      const Cell &fCell = eval(ki, 2, state);
      Cell &target = getDestCell(state, ki);
      if (cond.taint || tCell.taint || fCell.taint) {
        target.taint = true;
      }
    }
    // else { // Other instructions are handled in executeInstruction
    // switch (i->getOpcode()) {
    // BB terminator
    // Other terminator instructions are handled
    // case Instruction::Ret:
    // case Instruction::Br:
    // case Instruction::Switch:
    // case Instruction::IndirectBr:
    // case Instruction::Unreachable:
    // memory op
    // case Instruction::Fence:
    // case Instruction::GetElementPtr: handled
    // case Instruction::Alloca:
    // case Instruction::Load:
    // case Instruction::Store: {
    //   // We don't currently track taint for memory operations
    //   break;
    // }
    // // other ops
    // case Instruction::Invoke: // fall-through
    // case Instruction::Call: {
    //   break; // For now, ignore
    // }
    // case Instruction::Select: {
    //   const Cell &cond = eval(ki, 0, state);
    //   const Cell &tCell = eval(ki, 1, state);
    //   const Cell &fCell = eval(ki, 2, state);
    //   if (cond.taint || tCell.taint || fCell.taint) {
    //     getDestCell(state, ki).taint = true;
    //   }
    //   break;
    // }
    // default:
    //   break;
    // }
    //}
  }
  if (runInstruction) {
    executeInstruction(state, ki);
  }
}

int Executor::selectPatchID(ExecutionState &state, KInstruction *target,
                            std::vector<ref<Expr>> &arguments,
                            ref<Expr> targetExpr) {
  SPDLOG_DEBUG("[state {}] Select patch id", state.getID());
  state.metaData.setStateType(StateType::ST_base_after);
  int prevStateId = state.getID();
  state.resetID();
  int nextStateId = state.getID();
  klee_log_data(fmt::format("[fork-map] [sel-patch] [state {}] [state {}]",
                            prevStateId, nextStateId));
  // Pause other states (base state before select patch id)
  stateGroup.pauseAll(&state);
  state.passedTarget = true;
  int bugId = 0;
  if (LazyPatch) {
    // In lazy patch mode, select original program
    if (executionType != ExecutionType::MakeSnapshot) {
      ExecutionState *patchedState = cloneState(&state);
      patchedState->metaData.setStateType(StateType::ST_patch);
      stateGroup.addPatchState(patchedState, state.getID());
      SPDLOG_INFO("[lazy-patch] [state {}] -> [state {}] Select patch {}",
                  state.getID(), patchedState->getID(), bugId);
    }
    state.metaData.setPatchID(bugId);
    stateGroup.addForkParent(&state);
    state.addConstraint(EqExpr::create(
        targetExpr, ConstantExpr::create((uint64_t)bugId, Expr::Int32)));
    return 0;
  }
  // Now, select patch id with fork
  ExecutionState *targetState = &state;
  int patchSize = PatchID.size();
  for (int i = 0; i < patchSize; i++) {
    int patch = PatchID[i];
    ref<klee::ConstantExpr> pid =
        ConstantExpr::create((uint64_t)patch, klee::Expr::Int32);
    ref<Expr> eqExpr = EqExpr::create(targetExpr, pid);
    if (i == patchSize - 1) {
      addConstraint(*targetState, eqExpr);
      targetState->explain = "Select patch " + std::to_string(patch);
      targetState->metaData.setPatchID(patch);
      SPDLOG_INFO("[state {}] Select final patch {} at {}",
                  targetState->getID(), patch, target->getSourceLocation());
      if (patchSize > 1) {
        // Pause and save to patchStates
        // Later, used in crashStateHandler
        targetState->metaData.setStateType(StateType::ST_patch);
        stateGroup.addPatchState(targetState, state.getID());
      }
      break;
    }
    StatePair sp = fork(*targetState, eqExpr, true, false);
    if (i == 0) { // Use first patch
      ExecutionState *es = sp.first;
      es->metaData.setPatchID(patch);
      ExecutionState *ns = cloneState(es);
      // State with shadow memory
      es->metaData.setStateType(StateType::ST_base_after);
      es->explain += ": Base case";
      stateGroup.addForkParent(es);
      // normal
      ns->metaData.setStateType(StateType::ST_patch);
      ns->explain += ": Rerun with crashing memory state";
      stateGroup.addPatchState(ns, state.getID());
    } else {
      sp.first->metaData.setStateType(StateType::ST_patch);
      stateGroup.addPatchState(sp.first, state.getID());
    }
    sp.first->explain = "Select patch " + std::to_string(patch);
    targetState = sp.second;
    sp.first->metaData.setPatchID(patch);
    SPDLOG_INFO(
        "[state {}] Select patch {} at {} / target state {} patch id {}",
        sp.first->getID(), patch, target->getSourceLocation(),
        targetState->getID(), targetState->metaData.getPatchID());
  }
  return 0;
}

ref<Expr> Executor::getExprFromData(ExecutionState &state,
                                    std::vector<uint8_t> &data, uint64_t offset,
                                    uint64_t size) {
  ref<Expr> originalExpr(0);
  for (uint32_t i = 0; i < size; i++) {
    uint32_t idx = Context::get().isLittleEndian() ? i : (size - i - 1);
    ref<Expr> byteExpr = ConstantExpr::create(data[offset + idx], Expr::Int8);
    if (originalExpr.isNull()) {
      originalExpr = byteExpr;
    } else {
      originalExpr = ConcatExpr::create(byteExpr, originalExpr);
    }
  }
  APInt apInt = APInt(
      size * 8, dyn_cast<ConstantExpr>(originalExpr)->getZExtValue(), true);
  ref<Expr> finalExpr = ConstantExpr::alloc(apInt);
  ref<Expr> test0 = ConstantExpr::create(2147483652, 32);
  ref<Expr> stest0 = SExtExpr::create(test0, 32);
  ref<Expr> val1 = ConstantExpr::create(3, 32);
  ref<Expr> val2 = AddExpr::create(val1, test0);
  ref<Expr> val3 = AddExpr::create(val1, stest0);
  ref<Expr> test = SubExpr::create(finalExpr, val1);
  ref<Expr> test2 = SubExpr::create(finalExpr, val2);
  return finalExpr;
}

MemoryObject *Executor::lazyAllocation(ExecutionState &state, ref<Expr> addr,
                                       const llvm::Type *type,
                                       KInstruction *target, const Array *array,
                                       uint64_t size, int depth) {
  SPDLOG_DEBUG("[state {}] lazyAllocation on {} - type {}, array {}",
               state.getID(), addr->str(), get_type_string(type), array->name);
  if (type->isPointerTy()) {
    if (array->getSize() > 8) {
      SPDLOG_INFO("[state {}] Large array {} is array with size {}",
                  state.getID(), array->name, array->getSize());
    }
    llvm::Type *base_type = type->getPointerElementType();
    uint64_t typeSize = 8;
    if (base_type->isSized())
      typeSize =
          kmodule->targetData->getTypeStoreSize(base_type).getKnownMinSize();
    if (size > 0) {
      typeSize = size;
    }
    auto res = lazyInitManager.getLazyAlloc(array);
    MemoryObject *mo = nullptr;
    const Array *newArray = nullptr;
    if (res.first == nullptr) {
      // allocate memory
      mo = memory->allocate(typeSize, /*isLocal=*/false,
                            /*isGlobal=*/false, nullptr, 8);
      executeMakeSymbolic(state, mo, fmt::format("{}.lazy", array->name));
      newArray = state.symbolics.back().getArray();
      lazyInitManager.addLazyAlloc(array, mo, newArray);
    } else {
      // Check previous lazy allocation
      mo = res.first;
      newArray = res.second;
      bindObjectInState(state, mo, false, newArray);
      state.addSymbolic(mo, newArray);
    }
    ref<Expr> eqExpr = EqExpr::create(addr, mo->getBaseExpr());
    solver->setTimeout(coreSolverTimeout);
    bool result = false;
    if (!solver->mayBeTrue(state, eqExpr, result)) {
      SPDLOG_DEBUG("[state {}] solver error in lazy allocation of {}",
                   state.getID(), mo->getBaseExpr()->str());
      return nullptr;
    }
    if (result) {
      addConstraint(state, eqExpr);
      return mo;
    }
    return nullptr;
  }
  return nullptr;
}

ref<Expr>
Executor::expandLazyAllocation(ExecutionState &state, ref<Expr> addr,
                               const llvm::Type *type, ObjectState *os,
                               KInstruction *target, const std::string name,
                               bool allow_null, uint64_t requestedAddr,
                               const MemoryObject *requestedMO, int depth) {
  SymbolicAddressVisitor saVisitor = SymbolicAddressVisitor();
  std::set<uint64_t> baseAddresses;
  bool found = analyzeSymbolicAddr(state, addr, saVisitor, baseAddresses);
  SPDLOG_DEBUG("[state {}] Expand lazy allocation: {} with depth {} with "
               "{} symbolic vars at {}, {}",
               state.getID(), name, depth, saVisitor.getArrays().size(),
               target->getSourceLocation(), addr->str());
  if (found) {
    // concrete base + symbolic offset
    // handle this case when actually access to the address
    SPDLOG_DEBUG(
        "[state {}] Skip lazy allocation: {} is concrete base {} + symbolic "
        "offset at {}",
        state.getID(), name, *baseAddresses.begin(),
        target->getSourceLocation());
    bindLocal(target, state, addr);
    return addr;
  }
  assert(type->isPointerTy());
  llvm::Type *base_type = type->getPointerElementType();
  uint64_t typeSize = 8;
  if (base_type->isSized())
    typeSize =
        kmodule->targetData->getTypeStoreSize(base_type).getKnownMinSize();

  // symbolic base -> allocate new object
  const Array *arr = *saVisitor.getArrays().begin();
  MemArchive memArc;
  // Only works for first expansion
  if (lazyInitManager.getOriginalObject(arr, memArc)) {
    SPDLOG_INFO(
        "[state {}] Lazy allocation: use original object size {} instead of {}",
        state.getID(), memArc.size, typeSize);
    typeSize = memArc.size;
  } else {
    auto res = lazyInitManager.getLazyAlloc(arr);
    if (res.first == nullptr) {

    } else {
    }
  }
  time::Span timeout = coreSolverTimeout;
  solver->setTimeout(timeout);
  if (base_type->isFirstClassType()) {
    ref<klee::ConstantExpr> null = Expr::createPointer(0);
    ref<Expr> eqNull = EqExpr::create(addr, null);
    if (depth >= MaxLazyDepth) {
      if (allow_null) {
        // too deep. no more forking for this pointer.
        bool result = false;
        solver->mayBeTrue(state, eqNull, result);
        if (result) {
          addConstraint(state, eqNull);
        } else {
          terminateStateEarly(state, "lazy allocation too deep");
        }
      } else {
        terminateStateEarly(state, "lazy allocation too deep");
      }
      // must not touch state again in case of failure
    } else {
      state.explain = "Trash";
      ExecutionState *next_fork;
      if (allow_null && state.passedTarget) {
        StatePair sp = fork(state, eqNull, true);
        // true case -> addr == nullptr
        if (sp.first && sp.second) {
          sp.first->explain = "Lazy allocation: " + name + " is null";
          sp.second->explain = "Lazy allocation: " + name + " is not null";
          next_fork = sp.second;
          SPDLOG_DEBUG(
              "Lazy allocation: [state {}] is null, [state {}] is not null",
              sp.first->getID(), sp.second->getID());
          klee_log_data(fmt::format(
              "[fork-loc] [lazy] [state {}] -> [state {}] [loc {}] [name {}]",
              sp.first->getID(), sp.second->getID(),
              target->getSourceLocation(), name));
          sp.first->memTraces.push_back(
              MemTrace(requestedAddr, requestedMO->address, requestedMO->size,
                       0, 0, null));
        } else if (sp.second != nullptr) {
          next_fork = sp.second; // false case -> addr != nullptr
          next_fork->explain = "Lazy allocation: " + name + " is not null";
          SPDLOG_DEBUG("Lazy allocation: [state {}] is not null",
                       sp.second->getID());
        } else if (sp.first != nullptr) {
          sp.first->explain = "Lazy allocation: " + name + " is null";
          next_fork = nullptr;
          SPDLOG_DEBUG("Lazy allocation: [state {}] is null",
                       sp.first->getID());
          sp.first->memTraces.push_back(
              MemTrace(requestedAddr, requestedMO->address, requestedMO->size,
                       0, 0, null));
        }
      } else {
        next_fork = &state;
      }
      if (next_fork) {
        // allocate memory
        const Array *arr = nullptr;
        MemoryObject *mo = lazyInitManager.getLazyInitAssignment(requestedAddr);
        if (mo == nullptr) {
          // auto result = lazyInitManager.getSymbolicPointer(arr);
          // // If symbolic pointer from existing object, use that
          // if (std::get<0>(result) != 0) {
          //   ref<Expr> ptr = ConstantExpr::createPointer(std::get<0>(result));
          //   ref<Expr> eq = EqExpr::create(addr, ptr);
          //   addConstraint(*next_fork, eq);
          //   bindLocal(target, *next_fork, ptr);
          //   return ptr;
          // }
          // Else, create new object
          mo = memory->allocate(typeSize,
                                /*isLocal=*/false,
                                /*isGlobal=*/false, nullptr, 8);
          lazyInitManager.addLazyInitAssignment(requestedAddr, mo);
          std::string lname = getUniqueName(*next_fork, name,
                                            fmt::format("{}th", depth + 1), 0);
          SPDLOG_DEBUG("[state {}] lname: {}, mo name: {}, addr: {}, size: {}, "
                       "local: {}, global: {}",
                       next_fork->getID(), lname, mo->name, mo->address,
                       mo->size, mo->isLocal, mo->isGlobal);
          mo->setName(lname);
          mo->depth = depth;
          executeMakeSymbolic(*next_fork, mo, mo->name);
          arr = next_fork->symbolics.back().getArray();
          memArc.resetOriginalForLazy(false, requestedAddr, mo->address,
                                      typeSize);
          lazyInitManager.addOriginalObject(
              next_fork->symbolics.back().getArray(), memArc);
          SPDLOG_DEBUG("[state {}] Lazy allocation: {} add original object {}",
                       next_fork->getID(), mo->name, arr->getName());
          next_fork->symbolics.back().setLazy(
              state.prevPC->getSourceLocation());
          if (arr->getSize() > 8) {
            // Too large array size
            int min = -1;
            auto arrmap = saVisitor.getArrayIndices();
            for (int i : arrmap[arr]) {
              if (min == -1 || i < min) {
                min = i;
              }
            }
            SPDLOG_INFO("[state {}] Too large array size {}, {} at {}",
                        state.getID(), arr->getSize(), min, addr->str());
            std::vector<uint8_t> data = lazyInitManager.getSymbolicData(arr);
            if (!data.empty() && min >= 0 && min < arr->getSize()) {
              // Read actual data from min index
              SPDLOG_INFO("[state {}] Read actual data from min index {}",
                          state.getID(), min);
            } else {
              SPDLOG_INFO("[state {}] No actual data for {}", state.getID(),
                          arr->getName());
            }
          } else if (arr->getSize() < 8) {
            // Too small array size
            SPDLOG_INFO("[state {}] Too small array size {} at {}",
                        state.getID(), arr->getSize(), addr->str());
          } else {
            // lazyInitManager.addSymbolicPointer(
            //     arr, mo->address, mo, next_fork->symbolics.back().getArray(),
            //     false);
            lazyInitManager.addLazyAlloc(
                arr, mo, next_fork->symbolics.back().getArray());
          }
        } else {
          SPDLOG_DEBUG("[state {}] pre-allocated lname: mo name: {}, addr: {}, "
                       "size: {}, "
                       "local: {}, global: {}",
                       next_fork->getID(), mo->name, mo->address, mo->size,
                       mo->isLocal, mo->isGlobal);
          bool found = false;
          for (SymbolicObjectRecord &sor : next_fork->symbolics) {
            if (sor.getMemoryObject() == mo) {
              found = true;
              arr = sor.getArray();
              break;
            }
          }
          if (!found) {
            memArc.resetOriginalForLazy(true, requestedAddr, mo->address,
                                        mo->size);
            executeMakeSymbolic(*next_fork, mo, mo->name, false);
            next_fork->symbolics.back().setLazy(
                state.prevPC->getSourceLocation());
            lazyInitManager.addOriginalObject(
                next_fork->symbolics.back().getArray(), memArc);
            arr = next_fork->symbolics.back().getArray();
          }
        }

        if (next_fork->metaData.isTypeBase()) {
          const MemoryObject *cmo = (const MemoryObject *)mo;
          next_fork->shadowMemory.push_back(std::make_pair(cmo, arr));
          SPDLOG_INFO("[state {}] Save to shadow address space: {}({}) with {}",
                      next_fork->getID(), mo->name,
                      mo->getBaseExpr()->getZExtValue(),
                      get_type_string(base_type));
        }
        // add constraint
        ref<ConstantExpr> ptr = mo->getBaseExpr();
        ref<Expr> eq = EqExpr::create(addr, ptr);
        SPDLOG_INFO(
            "[state {}] Lazy allocation {} at: {}({}) with depth {} with {}",
            next_fork->getID(), mo->name, ptr->getZExtValue(),
            get_type_string(base_type), depth, eq->str());
        bool result = false;
        solver->mayBeTrue(*next_fork, eq, result);
        if (result) {
          SPDLOG_INFO("[state {}] Success to add constraint: {}",
                      next_fork->getID(), eq->str());
          addConstraint(*next_fork, eq);
          next_fork->memTraces.push_back(
              MemTrace(requestedAddr, requestedMO->address, requestedMO->size,
                       mo->address, mo->address, mo->getBaseExpr()));
          if (allow_null) {
            bindLocal(target, *next_fork, ptr);
          }
          return ptr;
        } else {
          // failed to add constraint. must not touch state again
          SPDLOG_INFO("[state {}] Failed to add constraint: {}", state.getID(),
                      eq->str());
          bindLocal(target, *next_fork, addr);
          // terminateStateEarly(*next_fork, "failed to add constraint");
          return addr;
        }
        // bindLocal(target, *next_fork, ptr);
      }
    }
  }
  return Expr::createPointer(0);
}

ExecutionState *Executor::cloneState(ExecutionState *state) {
  ExecutionState *ns = state->branch();
  addedStates.push_back(ns);
  state->ptreeNode->data = 0;
  std::pair<PTree::Node *, PTree::Node *> res =
      processTree->split(state->ptreeNode, ns, state);
  ns->ptreeNode = res.first;
  state->ptreeNode = res.second;
  SPDLOG_DEBUG("Cloned [state {}] to [state {}] (with patch {})",
               state->getID(), ns->getID(), ns->metaData.getPatchID());
  return ns;
}

template <typename SqType, typename TypeIt>
void Executor::computeOffsetsSeqTy(KGEPInstruction *kgepi,
                                   ref<ConstantExpr> &constantOffset,
                                   uint64_t index, const TypeIt it) {
  const auto *sq = cast<SqType>(*it);
  uint64_t elementSize =
      kmodule->targetData->getTypeStoreSize(sq->getElementType());
  const Value *operand = it.getOperand();
  if (const Constant *c = dyn_cast<Constant>(operand)) {
    ref<ConstantExpr> index =
        evalConstant(c)->SExt(Context::get().getPointerWidth());
    ref<ConstantExpr> addend = index->Mul(
        ConstantExpr::alloc(elementSize, Context::get().getPointerWidth()));
    constantOffset = constantOffset->Add(addend);
  } else {
    kgepi->indices.emplace_back(index, elementSize);
  }
}

bool Executor::isAlive() {
  return !states.empty() && !haltExecution &&
         (states.size() - stateGroup.getPatchedStatesSize() > 0);
}

bool Executor::isAllStatePaused() {
  for (ExecutionState *state : states) {
    if (!state->metaData.getIsPaused()) {
      return false;
    }
  }
  return true;
}

void Executor::run(ExecutionState &initialState) {
  bindModuleConstants();

  // Delay init till now so that ticks don't accrue during
  // optimization and such.
  initTimers();

  states.insert(&initialState);

  // initialize hit locations
  if (!LocHit.empty()) {
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    while ((pos = LocHit.find(delimiter)) != std::string::npos) {
      token = LocHit.substr(0, pos);
      hit_list.insert(token);
      LocHit.erase(0, pos + delimiter.length());
    }
    hit_list.insert(LocHit);
    LocHit = "ACTIVE";
  }

  if (usingSeeds) {
    std::vector<SeedInfo> &v = seedMap[&initialState];

    for (std::vector<KTest *>::const_iterator it = usingSeeds->begin(),
                                              ie = usingSeeds->end();
         it != ie; ++it) {
      v.push_back(SeedInfo(*it));
      int num_obj = SeedInfo(*it).input->numObjects;
      for (int k = 0; k < num_obj; k++) {
        KTestObject obj = SeedInfo(*it).input->objects[k];
        int num_bytes = obj.numBytes;
        //                printf("seed-obj[%d].name = %s\n", k, obj.name);

        if (strcmp(obj.name, "A-data-stat") == 0) {
          A_data_stat = (int *)malloc(num_bytes * sizeof(int));
          for (int i = 0; i < num_bytes; i++) {
            A_data_stat[i] = obj.bytes[i];
            //                        printf("%d ", obj.bytes[i]);
          }
        } else if (strcmp(obj.name, "A-data") == 0) {
          A_data = (int *)malloc(num_bytes * sizeof(int));
          for (int i = 0; i < num_bytes; i++) {
            A_data[i] = obj.bytes[i];
            //                        printf("%d ", A_data[i]);
          }
        } else if (strstr(obj.name, "arg0")) {
          int value_final = 0;
          int *value = (int *)malloc(num_bytes * sizeof(int));
          for (int i = 0; i < num_bytes; i++) {
            value[i] = obj.bytes[i];
            value_final += (obj.bytes[i] << 8 * (i));
          }
          arg_map.insert(std::pair<std::string, int *>(obj.name, value));
          klee_warning("Reading Argument, name:%s, size:%d and value:%d",
                       obj.name, num_bytes, value_final);

        } else if (strcmp(obj.name, "model_version") == 0) {

          // Do nothing

        } else {
          int value_final = 0;
          int *value = (int *)malloc(num_bytes * sizeof(int));
          for (int i = 0; i < num_bytes; i++) {
            value[i] = obj.bytes[i];
            value_final += (obj.bytes[i] << 8 * (i));
          }
          var_map.insert(std::pair<std::string, int *>(obj.name, value));
          klee_warning(
              "Reading Second Order Variable, name:%s, size:%d and value:%d",
              obj.name, num_bytes, value_final);
        }
      }
    }

    int lastNumSeeds = usingSeeds->size() + 10;
    time::Point lastTime, startTime = lastTime = time::getWallTime();
    ExecutionState *lastState = 0;
    bool reachedTarget = false;
    while (!seedMap.empty()) {
      if (haltExecution) {
        doDumpStates();
        return;
      }

      std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
          seedMap.upper_bound(lastState);
      if (it == seedMap.end())
        it = seedMap.begin();
      lastState = it->first;
      unsigned numSeeds = it->second.size();
      ExecutionState &state = *lastState;
      KInstruction *ki = state.pc;
      stepInstruction(state);

      executeInstructionWrapper(state, ki);
      processTimers(&state, maxInstructionTime * numSeeds);
      updateStates(&state);

      if ((stats::instructions % 1000) == 0) {
        int numSeeds = 0, numStates = 0;
        for (std::map<ExecutionState *, std::vector<SeedInfo>>::iterator
                 it = seedMap.begin(),
                 ie = seedMap.end();
             it != ie; ++it) {
          numSeeds += it->second.size();
          numStates++;
        }
        const auto time = time::getWallTime();
        const time::Span seedTime(SeedTime);
        if (seedTime && time > startTime + seedTime) {
          klee_warning("seed time expired, %d seeds remain over %d states",
                       numSeeds, numStates);
          break;
        } else if (numSeeds <= lastNumSeeds - 10 ||
                   time - lastTime >= time::seconds(10)) {
          lastTime = time;
          lastNumSeeds = numSeeds;
          klee_message("%d seeds remaining over: %d states", numSeeds,
                       numStates);
        }
      }
    }

    klee_message("seeding done (%d states remain)", (int)states.size());

    // XXX total hack, just because I like non uniform better but want
    // seed results to be equally weighted.
    for (std::set<ExecutionState *>::iterator it = states.begin(),
                                              ie = states.end();
         it != ie; ++it) {
      (*it)->weight = 1.;
    }

    if (OnlySeed) {
      doDumpStates();
      return;
    }
  }

  searcher = constructUserSearcher(*this);

  std::vector<ExecutionState *> newStates(states.begin(), states.end());
  searcher->update(0, newStates, std::vector<ExecutionState *>());
  KInstruction *ki;
  bool reachedTarget = false;
  while (isAlive()) {
    ExecutionState &state = searcher->selectState();
    WallTimer timer = WallTimer();
    ki = state.pc;
    if (state.metaData.getIsPaused()) {
      SPDLOG_DEBUG("[state {}] [paused] {}", state.getID(),
                   ki->getSourceLocation());
    }
    stepInstruction(state);

    executeInstructionWrapper(state, ki);

    processTimers(&state, maxInstructionTime);

    checkMemoryUsage();

    state.cost += timer.check();

    updateStates(&state);
  }
  SPDLOG_WARN("Executor is not alive: states.empty() = {}, haltExecution = "
              "{}, states.size() = {}, stateGroup.getSize(patch) = {}",
              states.empty(), haltExecution, states.size(),
              stateGroup.getPatchedStatesSize());
  // Exit: write stats to data.log
  klee_log_data(fmt::format("[stats] [instructions {}] [symbolics {}]",
                            stats::instructions, countSymbolic));
  memGraph.free();
  delete searcher;
  searcher = 0;

  doDumpStates();
}

std::string Executor::getAddressInfo(ExecutionState &state,
                                     ref<Expr> address) const {
  std::string Str;
  llvm::raw_string_ostream info(Str);
  SPDLOG_INFO("[state {}] getAddressInfo {}", state.getID(), address->str());
  info << "\taddress: " << address << "\n";
  uint64_t example;
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(address)) {
    example = CE->getZExtValue();
  } else {
    ref<ConstantExpr> value;
    bool success = solver->getValue(state, address, value);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    example = value->getZExtValue();
    info << "\texample: " << example << "\n";
    std::pair<ref<Expr>, ref<Expr>> res = solver->getRange(state, address);
    info << "\trange: [" << res.first << ", " << res.second << "]\n";
  }

  MemoryObject hack((unsigned)example);
  MemoryMap::iterator lower = state.addressSpace.objects.upper_bound(&hack);
  info << "\tnext: ";
  if (lower == state.addressSpace.objects.end()) {
    info << "none\n";
  } else {
    const MemoryObject *mo = lower->first;
    std::string alloc_info;
    mo->getAllocInfo(alloc_info);
    info << "object at " << mo->address << " of size " << mo->size << "\n"
         << "\t\t" << alloc_info << "\n";
  }
  if (lower != state.addressSpace.objects.begin()) {
    --lower;
    info << "\tprev: ";
    if (lower == state.addressSpace.objects.end()) {
      info << "none\n";
    } else {
      const MemoryObject *mo = lower->first;
      std::string alloc_info;
      mo->getAllocInfo(alloc_info);
      info << "object at " << mo->address << " of size " << mo->size << "\n"
           << "\t\t" << alloc_info << "\n";
    }
  }

  return info.str();
}

void Executor::pauseState(ExecutionState &state) {
  auto it = std::find(continuedStates.begin(), continuedStates.end(), &state);
  // If the state was to be continued, but now gets paused again
  SPDLOG_INFO("[state {}] pause state", state.getID());
  if (state.metaData.getIsPaused() || state.ptreeNode->validChildren <= 0) {
    assert(state.metaData.getIsPaused() && "Invalid state not paused");
    if (state.ptreeNode->validChildren <= 0) {
      SPDLOG_DEBUG("[state {}] Already paused", state.getID());
    } else {
      SPDLOG_DEBUG("[state {}] Will be removed", state.getID());
    }
    return;
  }
  state.metaData.setIsPaused(true);
  if (it != continuedStates.end()) {
    // ...just don't continue it
    std::swap(*it, continuedStates.back());
    continuedStates.pop_back();
  } else {
    processTree->updateValidNodeInfo(state.ptreeNode, -1);
    pausedStates.push_back(&state);
  }
}

void Executor::continueState(ExecutionState &state) {
  auto it = std::find(pausedStates.begin(), pausedStates.end(), &state);
  // If the state was to be paused, but now gets continued again
  SPDLOG_INFO("[state {}] continue state", state.getID());
  state.metaData.setIsPaused(false);
  if (it != pausedStates.end()) {
    // ...don't pause it
    std::swap(*it, pausedStates.back());
    pausedStates.pop_back();
  } else {
    processTree->updateValidNodeInfo(state.ptreeNode, 1);
    continuedStates.push_back(&state);
  }
}

void Executor::dumpLazyMemoryGraph(ExecutionState &state,
                                   std::string filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    SPDLOG_ERROR("Failed to open file {}", filename);
    return;
  }
  for (auto it : state.memTraces) {
    // Check if target is null
    uint64_t value = 0;
    Solver::Validity res;
    time::Span timeout = coreSolverTimeout;
    solver->setTimeout(timeout);
    bool success =
        solver->evaluate(state, EqExpr::createIsZero(it.targetExpr), res);
    solver->setTimeout(time::Span());
    if (!success) {
      SPDLOG_WARN("[state {}] Solver timeout", state.getID());
      continue;
    }
    if (res == Solver::True) {
      value = 0;
    } else if (res == Solver::False) {
      value = it.targetAddr;
    } else {
      value = 1;
    }
    file << fmt::format("[node] [addr {}] [base {}] [size {}] [value {}]\n",
                        it.address, it.base, it.size, value);
  }
  // Dump symbolic objects: target values
  for (SymbolicObjectRecord &sor : state.symbolics) {
    std::string name = sor.getName();
    uint64_t size = sor.getSize();
    uint64_t address = sor.getAddress();
    uint64_t base = sor.getAddress() - sor.getOffset();
    std::string symType = sor.getType();
    if (symType == "arg") {
      file << fmt::format("[sym] [arg] [index {}] [size {}] [name {}]\n",
                          sor.getIndex(), size, name);
    } else {
      file << fmt::format(
          "[sym] [heap] [type {}] [addr {}] [base {}] [size {}] [name {}]\n",
          symType, address, base, size, name);
    }
  }
  file.close();
}

void Executor::terminateState(ExecutionState &state, std::string msg,
                              std::string suffix) {
  state.metaData.setIsPaused(true);
  if (replayKTest && replayPosition != replayKTest->numObjects) {
    klee_warning_once(replayKTest,
                      "replay did not consume all objects in test input.");
  }
  llvm::StringRef suffixRef(suffix);
  std::set<std::string> suffixSet = {"ret",
                                     "exit",
                                     "assert.err",
                                     "abort.err",
                                     "bad_vector_access.err",
                                     "free.err",
                                     "overflow.err",
                                     "overshift.err",
                                     "ptr.err",
                                     "div.err",
                                     "readonly.err",
                                     "extractfix"};
  bool check = !state.targetStack.empty() && state.passedTarget &&
               suffixSet.count(suffix) > 0;
  SPDLOG_INFO(
      "[state {}] [patch {}, crash {}, type {}] Terminating with {} - {} at {}",
      state.getID(), state.metaData.getPatchID(), state.getCrashID(),
      state.metaData.getStateType(), suffix, msg,
      state.prevPC->getSourceLocation());

  if (!suffixRef.endswith("err")) {
    std::string tmp;
    llvm::raw_string_ostream msgs(tmp);
    msgs << "\nStack: \n";
    state.dumpStack(msgs, kmodule->targetData.get());
    if (PrintStack) {
      errs() << "Stack: \n";
      state.dumpStack(errs(), kmodule->targetData.get());
    }
    msgs << "Info: \n" << msg;
    interpreterHandler->processTestCase(state, msgs.str().c_str(),
                                        suffix.c_str());
  }

  if (check) {
    if (state.metaData.isType(StateType::ST_base_after)) {
      state.metaData.setIsCrash(suffixRef.endswith("err"));
      // If not crash, it does not need to be replayed
      // other behavior will only cause regression error
      if (executionType != ExecutionType::MakeSnapshot &&
          state.metaData.getIsCrash()) {
        stateGroup.pushInput(&state);
      }
    } else {
      state.metaData.setActuallyCrashed(suffixRef.endswith("err"));
    }
  }

  klee_log_data(fmt::format(
      "[meta-data] [state {}] [crashId {}] [patchId {}] [stateType {}] "
      "[isCrash {}] [actuallyCrashed {}] [use {}] [exitLoc {}] [exit {}]",
      state.getID(), state.getCrashID(), state.metaData.getPatchID(),
      state.metaData.getStateType(), state.metaData.getIsCrash(),
      state.metaData.getActuallyCrashed(), check,
      state.prevPC->getSourceLocation(), suffix));
  if (!state.targetStack.empty()) {
    StackFrame *sf = &state.targetStack.back();
    std::stringstream rss;
    for (bool reg : sf->regressionTrace) {
      rss << reg << ", ";
    }
    klee_log_data(fmt::format("[regression] [state {}] [reg {}]", state.getID(),
                              rss.str()));
    std::stringstream lss;
    for (ref<Expr> trace : sf->lazyTrace) {
      trace = optimizer.optimizeExpr(trace, false);
      if (isa<NonConstantExpr>(trace)) {
        Solver::Validity res;
        time::Span timeout = coreSolverTimeout;
        solver->setTimeout(timeout);
        bool success = solver->evaluate(state, trace, res);
        solver->setTimeout(time::Span());
        if (!success) {
          SPDLOG_WARN("[state {}] Solver timeout", state.getID());
          continue;
        }
        if (res == Solver::True) {
          lss << "1, ";
        } else if (res == Solver::False) {
          lss << "0, ";
        } else {
          lss << "?, ";
          SPDLOG_WARN("[state {}] Solver unknown for {}", state.getID(),
                      trace->str());
        }
      } else {
        lss << dyn_cast<ConstantExpr>(trace)->getZExtValue() << ", ";
      }
    }
    std::string patchEval =
        state.patchDataManager.evalPatchSet(solver, coreSolverTimeout, state);
    std::stringstream pss;
    state.patchDataManager.printPatchSet(pss);
    klee_log_data(fmt::format(
        "[lazy-trace] [state {}] [reg {}] [patches [{}]] [patch-eval [{}]]",
        state.getID(), lss.str(), pss.str(), patchEval));
    // Dump lazy heap
    dumpLazyMemoryGraph(state,
                        interpreterHandler->getOutputFilename(
                            fmt::format("test{:06d}.mem", state.getID())));
  } else {
    SPDLOG_DEBUG("[state {}] No stack trace", state.getID());
  }
  std::stringstream sss;
  std::vector<std::string> stackTrace;
  state.dumpStackTrace(stackTrace, nullptr);
  for (auto it : stackTrace) {
    sss << it << ", ";
  }
  klee_log_data(
      fmt::format("[stack-trace] [state {}] [reg {}] [passed-crash-loc {}]",
                  state.getID(), sss.str(), state.passedCrashLoc));
  if ((this->executionType == ExecutionType::MakeSnapshot &&
       PatchID.size() == 1) ||
      DumpSnapshot) {
    Snapshot *snapshot = nullptr;
    StackFrame *sf = nullptr;
    if (!state.targetStack.empty()) {
      sf = &state.targetStack.back();
      snapshot = sf->snapshot;
    }
    if (snapshot != nullptr && !suffixRef.endswith("early")) {
      if (DumpSnapshot) {
        std::vector<ref<Expr>> arguments;
        for (auto &it : snapshot->arguments) {
          arguments.push_back(it);
        }
        Snapshot *newSnapshot =
            getSnapshot(state, sf->kf->function, state.prevPC, arguments,
                        snapshot->snapshotId, "");
        newSnapshot->regressionTrace = snapshot->regressionTrace;
        snapshot = newSnapshot;
      }
      SPDLOG_WARN("[state {}] Terminating with snapshot {} - {}: {}",
                  state.getID(), snapshot->snapshotId, suffix, msg);
      snapshot =
          applyDynamicAnalysisToSnapshot(state, snapshot, *sf, msg, suffix);
      std::string filename = snapshot->getSnapshotName(&state, suffix);
      snapshot->dump(interpreterHandler->getOutputFilename(filename));
      std::string snapshotLast =
          interpreterHandler->getOutputFilename("snapshot-last.json");
      unlink(snapshotLast.c_str());
      symlink(filename.c_str(), snapshotLast.c_str());
    } else {
      SPDLOG_WARN("[state {}] No snapshot found", state.getID());
    }
  }

  interpreterHandler->incPathsExplored();

  std::vector<ExecutionState *>::iterator it =
      std::find(addedStates.begin(), addedStates.end(), &state);
  if (it == addedStates.end()) {
    state.pc = state.prevPC;
    removedStates.push_back(&state);
  } else {
    // never reached searcher, just delete immediately
    std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it3 =
        seedMap.find(&state);
    if (it3 != seedMap.end())
      seedMap.erase(it3);
    addedStates.erase(it);
    processTree->remove(state.ptreeNode);
    if (stateGroup.checkDeleteState(&state)) {
      delete &state;
    }
  }
}

void Executor::terminateStateOnReturn(ExecutionState &state,
                                      const Twine &message) {
  if (!OnlyOutputStatesCoveringNew || state.coveredNew ||
      (AlwaysOutputSeeds && seedMap.count(&state)))
    interpreterHandler->processTestCase(state, (message + "\n").str().c_str(),
                                        "ret");
  terminateState(state, message.str(), "ret");
}

void Executor::terminateStateEarly(ExecutionState &state,
                                   const Twine &message) {
  if (!OnlyOutputStatesCoveringNew || state.coveredNew ||
      (AlwaysOutputSeeds && seedMap.count(&state)))
    interpreterHandler->processTestCase(state, (message + "\n").str().c_str(),
                                        "early");
  terminateState(state, message.str(), "early");
}

void Executor::terminateStateOnExit(ExecutionState &state) {
  if (!OnlyOutputStatesCoveringNew || state.coveredNew ||
      (AlwaysOutputSeeds && seedMap.count(&state)))
    interpreterHandler->processTestCase(state, 0, 0);
  terminateState(state, "Program exited normally.", "exit");
}

void Executor::terminateStateOnExtractfixCrashLoc(ExecutionState &state) {
  terminateState(state, fmt::format("ExtractFix crash loc {}", CrashLoc),
                 "extractfix");
}

const InstructionInfo &
Executor::getLastNonKleeInternalInstruction(const ExecutionState &state,
                                            Instruction **lastInstruction) {
  // unroll the stack of the applications state and find
  // the last instruction which is not inside a KLEE internal function
  ExecutionState::stack_ty::const_reverse_iterator it = state.stack.rbegin(),
                                                   itE = state.stack.rend();

  // don't check beyond the outermost function (i.e. main())
  itE--;

  const InstructionInfo *ii = 0;
  if (kmodule->internalFunctions.count(it->kf->function) == 0) {
    ii = state.prevPC->info;
    *lastInstruction = state.prevPC->inst;
    //  Cannot return yet because even though
    //  it->function is not an internal function it might of
    //  been called from an internal function.
  }

  // Wind up the stack and check if we are in a KLEE internal function.
  // We visit the entire stack because we want to return a CallInstruction
  // that was not reached via any KLEE internal functions.
  for (; it != itE; ++it) {
    // check calling instruction and if it is contained in a KLEE internal
    // function
    const Function *f = (*it->caller).inst->getParent()->getParent();
    if (kmodule->internalFunctions.count(f)) {
      ii = 0;
      continue;
    }
    if (!ii) {
      ii = (*it->caller).info;
      *lastInstruction = (*it->caller).inst;
    }
  }

  if (!ii) {
    // something went wrong, play safe and return the current instruction info
    *lastInstruction = state.prevPC->inst;
    return *state.prevPC->info;
  }
  return *ii;
}

bool Executor::shouldExitOn(enum TerminateReason termReason) {
  std::vector<TerminateReason>::iterator s = ExitOnErrorType.begin();
  std::vector<TerminateReason>::iterator e = ExitOnErrorType.end();

  for (; s != e; ++s)
    if (termReason == *s)
      return true;

  return false;
}

void Executor::terminateStateOnError(ExecutionState &state,
                                     const llvm::Twine &messaget,
                                     enum TerminateReason termReason,
                                     const char *suffix,
                                     const llvm::Twine &info) {
  std::string message = messaget.str();
  static std::set<std::pair<Instruction *, std::string>> emittedErrors;
  Instruction *lastInst;
  const InstructionInfo &ii =
      getLastNonKleeInternalInstruction(state, &lastInst);

  SPDLOG_WARN("[state {}] Found Error!! {}", state.getID(), message);

  if (EmitAllErrors ||
      emittedErrors.insert(std::make_pair(lastInst, message)).second) {
    if (ii.file != "") {
      klee_message("ERROR: %s:%d:%d:%d: %s", ii.file.c_str(), ii.line,
                   ii.column, ii.assemblyLine, message.c_str());
    } else {
      klee_message("ERROR: (location information missing) %s", message.c_str());
    }
    if (!EmitAllErrors)
      klee_message("NOTE: now ignoring this error at this location");

    std::string MsgString;
    llvm::raw_string_ostream msg(MsgString);
    msg << "Error: " << message << "\n";
    if (ii.file != "") {
      msg << "File: " << ii.file << "\n";
      msg << "Line: " << ii.line << "\n";
      msg << "Column: " << ii.column << "\n";
      msg << "assembly.ll line: " << ii.assemblyLine << "\n";
    }

    if (!NoExitOnError) {
      //            interpreterHandler->processTestCase(state,
      //            msg.str().c_str(), suffix);
      haltExecution = true;
    }

    msg << "Stack: \n";
    state.dumpStack(msg, kmodule->targetData.get());

    if (PrintStack) {

      errs() << "Stack: \n";
      state.dumpStack(errs(), kmodule->targetData.get());
    }

    std::string info_str = info.str();
    if (info_str != "")
      msg << "Info: \n" << info_str;

    std::string suffix_buf;
    if (!suffix) {
      suffix_buf = TerminateReasonNames[termReason];
      suffix_buf += ".err";
      suffix = suffix_buf.c_str();
    }

    interpreterHandler->processTestCase(state, msg.str().c_str(), suffix);
    terminateState(state, msg.str(), std::string(suffix));
  } else {
    terminateState(state, "err", "err");
  }

  if (shouldExitOn(termReason)) {
    SPDLOG_WARN("[state {}] Exit on error {}", state.getID(), termReason);
    haltExecution = true;
  }
}

// XXX shoot me
static const char *okExternalsList[] = {"printf", "fprintf", "puts", "getpid"};
static std::set<std::string> okExternals(
    okExternalsList,
    okExternalsList + (sizeof(okExternalsList) / sizeof(okExternalsList[0])));

ref<Expr> Executor::concretizeReadExpr(const ExecutionState &state,
                                       ref<Expr> &expr) {
  const ReadExpr *base = dyn_cast<ReadExpr>(expr);
  SPDLOG_DEBUG("[state {}] Concretizing read expr: {}", state.getID(),
               expr->str());
  ref<Expr> child = expr->getKid(0);
  ref<Expr> index_expr;
  std::string str_index;
  bool modified = false;

  /* concretizing index */
  if (dyn_cast<ConstantExpr>(child)) {
    //        ConstantExpr *index_expr = dyn_cast<ConstantExpr>(child);
    //        index_expr->toString(str_index);
    index_expr = child;
    ref<ConstantExpr> index_expr_const = dyn_cast<ConstantExpr>(child);
    index_expr_const->toString(str_index);

  } else {
    index_expr = concretizeExpr(state, child);
    ref<ConstantExpr> index_expr_const = dyn_cast<ConstantExpr>(index_expr);
    index_expr_const->toString(str_index);
  }

  int index = stoi(str_index);
  int width = expr->getWidth();

  std::string name_src = base->updates.root->name;
  int updateSize = base->updates.getSize();
  ref<ConstantExpr> resolve;

  if (usingSeeds) {
    if (name_src == "A-data") {
      int value = A_data[index];
      std::string log_message = "\n[concretizing] A-data[" + str_index + "] \n";
      klee_log_concrete(log_message.c_str());
      resolve = ConstantExpr::create(value, width);
      modified = true;
    } else if (name_src == "A-data-stat") {
      int value = A_data_stat[index];
      resolve = ConstantExpr::create(value, width);
      modified = true;
    } else if (strstr(name_src.c_str(), "arg0")) {
      if (arg_map.find(name_src) != var_map.end()) {
        int value = arg_map.find(name_src)->second[index];
        resolve = ConstantExpr::create(value, width);
        modified = true;
      }
    } else {
      if (var_map.find(name_src) != var_map.end()) {
        int value = var_map.find(name_src)->second[index];
        resolve = ConstantExpr::create(value, width);
        modified = true;
      } else {
        ref<Expr> ce;
        ce = ReadExpr::create(base->updates, index_expr);

        bool success = solver->getValue(state, ce, resolve);
        assert(success && "FIXME: Unhandled solver failure");
        (void)success;
        modified = true;
      }
    }

  } else {
    ref<Expr> ce;
    ce = ReadExpr::create(base->updates, index_expr);

    bool success = solver->getValue(state, ce, resolve);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;
    modified = true;
  }

  if (modified) {
    SPDLOG_DEBUG("[state {}] Concretized read expr: {}", state.getID(),
                 resolve->str());
    return resolve;
  }

  SPDLOG_DEBUG("[state {}] Not concretized read expr: {}", state.getID(),
               expr->str());
  return expr;
}

ref<Expr> Executor::concretizeExpr(const klee::ExecutionState &state,
                                   klee::ref<klee::Expr> &expr) {
  int numKids = expr.get()->getNumKids();
  ref<ConstantExpr> resolve;

  if (isa<ConstantExpr>(expr)) {
    return expr;
  }

  if (numKids > 0) {
    ref<Expr> list_child[numKids];
    for (int l = 0; l < numKids; l++) {
      ref<Expr> child = expr->getKid(l);
      ref<Expr> concretized_child;
      if (child->getKind() == Expr::Read) {
        concretized_child = concretizeReadExpr(state, child);
        if (concretized_child->getKind() == Expr::Constant) {
          list_child[l] = concretized_child;
        } else {
          list_child[l] = concretizeExpr(state, child);
        }

      } else {
        list_child[l] = concretizeExpr(state, child);
      }
    }

    expr = expr->rebuild(list_child);
  }

  bool success = solver->getValue(state, expr, resolve);
  assert(success && "FIXME: Unhandled solver failure");
  (void)success;

  if (!isa<ConstantExpr>(resolve))
    SPDLOG_DEBUG("[state {}] Non concretized expr: {}", state.getID(),
                 resolve->str());

  SPDLOG_DEBUG("[state {}] Concretizing expr: {} -> {}", state.getID(),
               expr->str(), resolve->str());
  return resolve;
}

ref<Expr> Executor::cloneTree(ref<Expr> &tree) {
  ref<Expr> clone;
  int numKids = tree->getNumKids();
  ref<Expr> clone_kids[numKids];

  if (dyn_cast<ConstantExpr>(tree)) {
    return tree;
  }

  if (dyn_cast<ReadExpr>(tree)) {

    if (dyn_cast<ConstantExpr>(tree->getKid(0))) {
      ref<ConstantExpr> child = dyn_cast<ConstantExpr>(tree->getKid(0));
      ref<Expr> clone_child =
          ConstantExpr::create(child->getZExtValue(), child->getWidth());
      clone_kids[0] = clone_child;
      clone = tree->rebuild(clone_kids);

    } else {
      ref<Expr> child = tree->getKid(0);
      ref<Expr> clone_child = cloneTree(child);
      clone_kids[0] = clone_child;
      clone = tree->rebuild(clone_kids);
    }

  } else {

    if (numKids == 0) {
      clone = tree->rebuild(clone_kids);
    } else {
      for (int l = 0; l < numKids; l++) {
        ref<Expr> child = tree->getKid(l);
        ref<Expr> clone_child = cloneTree(child);
        clone_kids[l] = clone_child;
      }
      clone = tree->rebuild(clone_kids);
    }
  }
  return clone;
}

void Executor::iterateUpdateList(ref<Expr> &expr) {
  if (dyn_cast<ReadExpr>(expr)) {
    ref<ReadExpr> readExpr = dyn_cast<ReadExpr>(expr);
    UpdateList updates = readExpr->updates;
    const UpdateNode *head = updates.head;

    // Special case empty list.
    if (head) {
      for (const UpdateNode *un = head; un; un = un->next) {
        ref<Expr> e(un->value);
      }
    }
  }
}

bool Executor::isReadExprAtOffset(ref<Expr> e, const ReadExpr *base,
                                  ref<Expr> offset) {
  const ReadExpr *re = dyn_cast<ReadExpr>(e.get());

  // right now, all Reads are byte reads but some
  // transformations might change this
  if (!re || (re->getWidth() != Expr::Int8))
    return false;

  // Check if the index follows the stride.
  // FIXME: How aggressive should this be simplified. The
  // canonicalizing builder is probably the right choice, but this
  // is yet another area where we would really prefer it to be
  // global or else use static methods.
  return SubExpr::create(re->index, base->index) == offset;
}

/// hasOrderedReads: \arg e must be a ConcatExpr, \arg stride must
/// be 1 or -1.
///
/// If all children of this Concat are reads or concats of reads
/// with consecutive offsets according to the given \arg stride, it
/// returns the base ReadExpr according to \arg stride: first Read
/// for 1 (MSB), last Read for -1 (LSB).  Otherwise, it returns
/// null.
const ReadExpr *Executor::hasOrderedReads(ref<Expr> e, int stride) {
  assert(e->getKind() == Expr::Concat);
  assert(stride == 1 || stride == -1);

  const ReadExpr *base = dyn_cast<ReadExpr>(e->getKid(0));

  // right now, all Reads are byte reads but some
  // transformations might change this
  if (!base || base->getWidth() != Expr::Int8)
    return NULL;

  // Get stride expr in proper index width.
  Expr::Width idxWidth = base->index->getWidth();
  ref<Expr> strideExpr = ConstantExpr::alloc(stride, idxWidth);
  ref<Expr> offset = ConstantExpr::create(0, idxWidth);

  e = e->getKid(1);

  // concat chains are unbalavoid Executor::nced to the right
  while (e->getKind() == Expr::Concat) {
    offset = AddExpr::create(offset, strideExpr);
    if (!isReadExprAtOffset(e->getKid(0), base, offset))
      return NULL;

    e = e->getKid(1);
  }

  offset = AddExpr::create(offset, strideExpr);
  if (!isReadExprAtOffset(e, base, offset))
    return NULL;

  if (stride == -1)
    return cast<ReadExpr>(e.get());
  else
    return base;
}

void Executor::traverseTree(ExecutionState &state, ref<Expr> &parent,
                            ref<Expr> &current) {

  errs() << "\ntraversing: " << current << "\n";
  errs() << "\ntraversing.kind: " << current->getKind() << "\n";
  errs() << "num-kids: " << current.get()->getNumKids() << "\n";
  int numKids = current.get()->getNumKids();
  if (numKids > 0) {
    for (int l = 0; l < numKids; l++) {
      ref<Expr> child = current->getKid(l);
      //            traverseTree(state, current, child);
    }
  }

  //    transformExpr(state,current);
  errs() << "\ntraversed[final]: " << current << "\n";
  return;
}

void Executor::callExternalFunction(ExecutionState &state, KInstruction *target,
                                    Function *function,
                                    std::vector<ref<Expr>> &arguments) {
  // check if specialFunctionHandler wants it
  if (specialFunctionHandler->handle(state, function, target, arguments)) {
    SPDLOG_TRACE("specialFunctionHandler handled {}",
                 function->getName().str());
    return;
  }

  if (usingSeeds) {
    for (std::vector<ref<Expr>>::iterator ai = arguments.begin(),
                                          ae = arguments.end();
         ai != ae; ++ai) {
      *ai = concretizeExpr(state, *ai);
    }
  }
  if (ExternalCalls == ExternalCallPolicy::None &&
      !okExternals.count(function->getName().str())) {
    klee_warning("Disallowed call to external function: %s\n",
                 function->getName().str().c_str());
    terminateStateOnError(state, "external calls disallowed", User);
    return;
  }

  // external call: accessed memory
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
  CallBase &cs = cast<CallBase>(*target->inst);
#else
  CallSite cs(target->inst);
#endif

  if (executionType == ExecutionType::UnderConstrained) {
    // Pretend it was done: return symbolic value
    SPDLOG_DEBUG("[state {}] External call in UC mode: {}", state.getID(),
                 function->getName().str());
    Type *resultType = target->inst->getType();
    if (resultType != Type::getVoidTy(function->getContext())) {
      ref<Expr> e = getSymbolicValue(
                        state, getWidthForLLVMType(resultType) / 8,
                        fmt::format("{}.ret", function->getName().str()), 0, 0)
                        .second;
      state.symbolics.back().setSpecial("lazy.ret");
      bindLocal(target, state, e);
    }
    return;
  }

  if (checkMemRecord(state)) {
    const llvm::FunctionType *fType = function->getFunctionType();
    unsigned numArgs = cs.arg_size();
    for (unsigned j = 0; j < numArgs; ++j) {
      const Cell &cell = eval(target, j + 1, state);
      for (const MemRecord &mr : cell.mr) {
        MemRecord mr2 = mr;
        mr2.type = MemRecordType::ExternalCall;
        if (accessedByExternalCall.count(mr2) == 0) {
          SPDLOG_DEBUG(
              "[state {}] Blacklist memrecord: addr {}, base {}, size {}",
              state.getID(), mr2.addr, mr2.base, mr2.size);
        }
        accessedByExternalCall.insert(mr2);
      }
      ref<Expr> val = cell.value;
      bool isPtr = val->getWidth() == Context::get().getPointerWidth();
      if (numArgs == fType->getNumParams()) {
        llvm::Type *type = fType->getParamType(j);
        isPtr = type != nullptr && type->isPointerTy();
      }
      if (isPtr) {
        if (isa<ConstantExpr>(val)) {
          ref<ConstantExpr> ce = dyn_cast<ConstantExpr>(val);
          if (ce->isZero()) {
            continue;
          }
          // If argument is a pointer, consider it reads whole memory object
          ObjectPair op;
          bool success = state.addressSpace.resolveOne(ce, op);
          if (!success) {
            continue;
          }
          const MemoryObject *mo = op.first;
          uint64_t addr = ce->getZExtValue();
          uint64_t size = mo->size;
          if (addr > mo->address) {
            size -= addr - mo->address;
          }
          MemRecord mr = MemRecord(addr, mo->address, true, size,
                                   MemRecordType::ExternalCall);
          if (accessedByExternalCall.count(mr) == 0) {
            SPDLOG_DEBUG(
                "[state {}] Blacklist memrecord ptr: addr {}, base {}, size {}",
                state.getID(), mr.addr, mr.base, mr.size);
          }
          accessedByExternalCall.insert(mr);
        }
      }
    }
  }

  // normal external function handling path
  // allocate 128 bits for each argument (+return value) to support fp80's;
  // we could iterate through all the arguments first and determine the exact
  // size we need, but this is faster, and the memory usage isn't significant.
  uint64_t *args =
      (uint64_t *)alloca(2 * sizeof(*args) * (arguments.size() + 1));
  memset(args, 0, 2 * sizeof(*args) * (arguments.size() + 1));
  unsigned wordIndex = 2;
  for (std::vector<ref<Expr>>::iterator ai = arguments.begin(),
                                        ae = arguments.end();
       ai != ae; ++ai) {
    if (ExternalCalls ==
        ExternalCallPolicy::All) { // don't bother checking uniqueness
      *ai = optimizer.optimizeExpr(*ai, true);
      ref<ConstantExpr> ce;
      bool success = solver->getValue(state, *ai, ce);
      if (!isa<ConstantExpr>(*ai)) {
        SPDLOG_DEBUG("[state {}] Concretized argument: {} => {}", state.getID(),
                     (*ai)->str(), ce->str());
      }
      assert(success && "FIXME: Unhandled solver failure");

      ce->toMemory(&args[wordIndex]);
      ObjectPair op;
      // Checking to see if the argument is a pointer to something
      if (ce->getWidth() == Context::get().getPointerWidth() &&
          state.addressSpace.resolveOne(ce, op)) {
        //                op.second->flushToConcreteStore(solver, state);
      }
      wordIndex += (ce->getWidth() + 63) / 64;
    } else {
      ref<Expr> arg = toUnique(state, *ai);
      if (ConstantExpr *ce = dyn_cast<ConstantExpr>(arg)) {
        // XXX kick toMemory functions from here
        ce->toMemory(&args[wordIndex]);
        wordIndex += (ce->getWidth() + 63) / 64;
      } else {
        terminateStateOnExecError(state,
                                  "external call with symbolic argument: " +
                                      function->getName());
        return;
      }
    }
  }
  // Prepare external memory for invoking the function
  state.addressSpace.copyOutConcretes();
#ifndef WINDOWS
  // Update external errno state with local state value
  int *errno_addr = getErrnoLocation(state);
  ObjectPair result;
  bool resolved = state.addressSpace.resolveOne(
      ConstantExpr::create((uint64_t)errno_addr, Expr::Int64), result);
  if (!resolved)
    klee_error("Could not resolve memory object for errno");
  ref<Expr> errValueExpr = result.second->read(0, sizeof(*errno_addr) * 8);
  ConstantExpr *errnoValue = dyn_cast<ConstantExpr>(errValueExpr);
  if (!errnoValue) {
    terminateStateOnExecError(state,
                              "external call with errno value symbolic: " +
                                  function->getName());
    return;
  }

  externalDispatcher->setLastErrno(
      errnoValue->getZExtValue(sizeof(*errno_addr) * 8));
#endif
  if (!SuppressExternalWarnings) {

    std::string TmpStr;
    llvm::raw_string_ostream os(TmpStr);
    os << "calling external: " << function->getName().str() << "(";
    for (unsigned i = 0; i < arguments.size(); i++) {
      os << arguments[i];
      if (i != arguments.size() - 1)
        os << ", ";
    }
    os << ") at " << state.pc->getSourceLocation();
    SPDLOG_DEBUG("[state {}] {}", state.getID(), os.str());

    if (AllExternalWarnings)
      klee_warning("%s", os.str().c_str());
    else
      klee_warning_once(function, "%s", os.str().c_str());
  }
  if (function->getName() == "syscall") {
    ref<Expr> syscallNum = arguments[0];
    ref<Expr> fd = arguments[1];
    if (isa<ConstantExpr>(syscallNum) && isa<ConstantExpr>(fd)) {
      ref<ConstantExpr> syscallNumCE = dyn_cast<ConstantExpr>(syscallNum);
      ref<ConstantExpr> fdCE = dyn_cast<ConstantExpr>(fd);
      if (syscallNumCE->getZExtValue() == 0 && fdCE->getZExtValue() == 0) {
        // Read stdin
        terminateStateOnExecError(state, "syscall with stdin argument: " +
                                             function->getName());
        return;
      }
    }
  }

  bool success = externalDispatcher->executeCall(function, target->inst, args);
  if (!success) {
    terminateStateOnError(state, "failed external call: " + function->getName(),
                          External);
    return;
  }
  if (!state.addressSpace.copyInConcretes()) {
    terminateStateOnError(state, "external modified read-only object",
                          External);
    return;
  }

#ifndef WINDOWS
  // Update errno memory object with the errno value from the call
  int error = externalDispatcher->getLastErrno();
  state.addressSpace.copyInConcrete(result.first, result.second,
                                    (uint64_t)&error);
#endif

  Type *resultType = target->inst->getType();
  if (resultType != Type::getVoidTy(function->getContext())) {
    ref<Expr> e =
        ConstantExpr::fromMemory((void *)args, getWidthForLLVMType(resultType));
    bindLocal(target, state, e);
  }
}

/***/

ref<Expr> Executor::replaceReadWithSymbolic(ExecutionState &state,
                                            ref<Expr> e) {
  unsigned n = interpreterOpts.MakeConcreteSymbolic;
  if (!n || replayKTest || replayPath)
    return e;

  // right now, we don't replace symbolics (is there any reason to?)
  if (!isa<ConstantExpr>(e))
    return e;

  if (n != 1 && random() % n)
    return e;

  // create a new fresh location, assert it is equal to concrete value in e
  // and return it.

  static unsigned id;
  const Array *array =
      arrayCache.CreateArray("rrws_arr" + llvm::utostr(++id),
                             Expr::getMinBytesForWidth(e->getWidth()));
  ref<Expr> res = Expr::createTempRead(array, e->getWidth());
  ref<Expr> eq = NotOptimizedExpr::create(EqExpr::create(e, res));
  SPDLOG_INFO("Making symbolic: {}", eq->str());
  state.addConstraint(eq);
  return res;
}

ObjectState *Executor::bindObjectInState(ExecutionState &state,
                                         const MemoryObject *mo, bool isLocal,
                                         const Array *array) {
  ObjectState *os = array ? new ObjectState(mo, array) : new ObjectState(mo);
  state.addressSpace.bindObject(mo, os);

  // Its possible that multiple bindings of the same mo in the state
  // will put multiple copies on this list, but it doesn't really
  // matter because all we use this list for is to unbind the object
  // on function return.
  if (isLocal)
    state.stack.back().allocas.push_back(mo);

  return os;
}

void Executor::onPtrAccess(ExecutionState &state, uint64_t source,
                           const MemoryObject *srcMO, bool isWrite,
                           ref<Expr> value) {
  if (source == 0 || srcMO == nullptr)
    return;
  // Convert value to concrete value
  uint64_t concreteValue = 0;
  ref<Expr> se = state.constraints.simplifyExpr(value);
  if (isa<ConstantExpr>(se)) {
    ref<ConstantExpr> ce = dyn_cast<ConstantExpr>(se);
    concreteValue = ce->getZExtValue();
  } else {
    // TODO: Handle symbolic value
  }
  if (concreteValue == 0) {
    onMemAccess(state, srcMO, source, isWrite, value);
    return;
  }
  // Get target memory object
  // Check if target memory object is in address space to reduce waste
  ObjectPair op;
  bool success = false;
  if ((concreteValue >= 0x7ff30000000) &&
      (concreteValue < 0x7ff30000000 + 100 * 1048576)) {
    solver->setTimeout(coreSolverTimeout);
    success = state.addressSpace.resolveOne(
        ConstantExpr::create(concreteValue, 64), op);
    solver->setTimeout(time::Span());
  }
  if (!success) {
    onMemAccess(state, srcMO, source, isWrite, value);
    return;
  }
  const MemoryObject *tgtMO = op.first;
  // Create MemNodes
  MemNode *srcMemNode = memGraph.addMemNode(srcMO->address, srcMO->size, srcMO);
  MemNode *tgtMemNode = memGraph.addMemNode(tgtMO->address, tgtMO->size, tgtMO);
  // Create PtrNodes
  PtrNode *srcPtrNode = memGraph.addPtrNode(source, srcMemNode);
  PtrNode *tgtPtrNode = memGraph.addPtrNode(concreteValue, tgtMemNode);
  // Create PtrEdge
  PtrEdge *ptrEdge = memGraph.addPtrEdge(srcPtrNode, tgtPtrNode);
  MemEdge *memEdge = memGraph.addMemEdge(srcMemNode, tgtMemNode, ptrEdge);
  // Record access
  MemRecord mr =
      MemRecord(source, srcMO->address, true, value->getWidth() / 8,
                isWrite ? MemRecordType::Write : MemRecordType::Read);
  for (auto &ts : state.targetStack) {
    if (isWrite) {
      ts.writeAccessMap[source] = std::make_pair(mr, ptrEdge);
    } else {
      ts.readAccessMap[source] = std::make_pair(mr, ptrEdge);
    }
  }
}

void Executor::onMemAccess(ExecutionState &state, const MemoryObject *mo,
                           uint64_t addr, bool isWrite, ref<Expr> value) {
  if (addr == 0 || mo == nullptr) {
    return;
  }
  MemNode *memNode = memGraph.addMemNode(mo->address, mo->size, mo);
  // Record access
  MemRecord mr =
      MemRecord(addr, mo->address, false, value->getWidth() / 8,
                isWrite ? MemRecordType::Write : MemRecordType::Read);
  for (auto &ts : state.targetStack) {
    if (isWrite) {
      ts.writeAccessMap[addr] = std::make_pair(mr, nullptr);
    } else {
      ts.readAccessMap[addr] = std::make_pair(mr, nullptr);
    }
  }
}

void Executor::executeAlloc(ExecutionState &state, ref<Expr> size, bool isLocal,
                            KInstruction *target, bool zeroMemory,
                            const ObjectState *reallocFrom) {
  size = toUnique(state, size);
  llvm::AllocaInst *allocaInst = dyn_cast<llvm::AllocaInst>(target->inst);

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(size)) {
    const llvm::Value *allocSite = state.prevPC->inst;
    size_t allocationAlignment = getAllocationAlignment(allocSite);
    MemoryObject *mo =
        memory->allocate(CE->getZExtValue(), isLocal, /*isGlobal=*/false,
                         allocSite, allocationAlignment);
    if (!mo) {
      bindLocal(target, state,
                ConstantExpr::alloc(0, Context::get().getPointerWidth()));

    } else {
      std::string name = "";
      if (allocaInst != nullptr) {
        if (allocaInst->hasName()) {
          name = allocaInst->getName().str();
          if (BasicBlock *bb = allocaInst->getParent()) {
            Function *fn = bb->getParent();
            if (fn && fn->hasName())
              name = fn->getName().str() + "." + name;
          }
        }
        name = getUniqueName(state, name, "_alloc", 0);
        if (!state.targetStack.empty()) {
          SPDLOG_TRACE("[state {}] alloc name: {} size: {}, addr: {}, local: "
                       "{}, global: {}",
                       state.getID(), name, mo->size, mo->address, mo->isLocal,
                       mo->isGlobal);
        }
        mo->setName(name);
        mo->type = allocaInst->getAllocatedType();
        if (memory->isValidAddress(mo->address)) {
          memGraph.addMemNode(mo->address, mo->size, mo);
        }

      } else {
        SPDLOG_TRACE("Not alloca inst...");
      }

      ObjectState *os = bindObjectInState(state, mo, isLocal);
      if (zeroMemory) {
        os->initializeToZero();
      } else {
        os->initializeToRandom();
      }
      bindLocal(target, state, mo->getBaseExpr());
      specialFunctionHandler->trackMemory(state, target->inst->getType(),
                                          mo->getBaseExpr(), size);
      if (reallocFrom) {
        unsigned count = std::min(reallocFrom->size, os->size);
        for (unsigned i = 0; i < count; i++)
          os->write(i, reallocFrom->read8(i));
        state.addressSpace.unbindObject(reallocFrom->getObject());
      }
    }
  } else {
    // XXX For now we just pick a size. Ideally we would support
    // symbolic sizes fully but even if we don't it would be better to
    // "smartly" pick a value, for example we could fork and pick the
    // min and max values and perhaps some intermediate (reasonable
    // value).
    //
    // It would also be nice to recognize the case when size has
    // exactly two values and just fork (but we need to get rid of
    // return argument first). This shows up in pcre when llvm
    // collapses the size expression with a select.

    size = optimizer.optimizeExpr(size, true);

    ref<ConstantExpr> example;
    bool success = solver->getValue(state, size, example);
    assert(success && "FIXME: Unhandled solver failure");
    (void)success;

    // Try and start with a small example.
    Expr::Width W = example->getWidth();
    while (example->Ugt(ConstantExpr::alloc(128, W))->isTrue()) {
      ref<ConstantExpr> tmp = example->LShr(ConstantExpr::alloc(1, W));
      bool res;
      bool success = solver->mayBeTrue(state, EqExpr::create(tmp, size), res);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      if (!res)
        break;
      example = tmp;
    }

    StatePair fixedSize = fork(state, EqExpr::create(example, size), true);

    if (fixedSize.second) {
      // Check for exactly two values
      ref<ConstantExpr> tmp;
      bool success = solver->getValue(*fixedSize.second, size, tmp);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      bool res;
      success =
          solver->mustBeTrue(*fixedSize.second, EqExpr::create(tmp, size), res);
      assert(success && "FIXME: Unhandled solver failure");
      (void)success;
      if (res) {
        executeAlloc(*fixedSize.second, tmp, isLocal, target, zeroMemory,
                     reallocFrom);
      } else {
        // See if a *really* big value is possible. If so assume
        // malloc will fail for it, so lets fork and return 0.
        StatePair hugeSize =
            fork(*fixedSize.second,
                 UltExpr::create(ConstantExpr::alloc(1U << 31, W), size), true);
        if (hugeSize.first) {
          klee_message("NOTE: found huge malloc, returning 0");
          bindLocal(target, *hugeSize.first,
                    ConstantExpr::alloc(0, Context::get().getPointerWidth()));
        }

        if (hugeSize.second) {

          std::string Str;
          llvm::raw_string_ostream info(Str);
          ExprPPrinter::printOne(info, "  size expr", size);
          info << "  concretization : " << example << "\n";
          info << "  unbound example: " << tmp << "\n";

          errs() << "example = " << example << " |  tmp = " << tmp << "\n";
          errs() << "first = " << fixedSize.first
                 << " | second = " << fixedSize.second << "\n";
          errs() << "first = " << hugeSize.first
                 << " | second = " << hugeSize.second << "\n";
          //          terminateStateOnError(*hugeSize.second, "concretized
          //          symbolic size",
          //                                Model, NULL, info.str());
          klee_message("NOTE: found huge malloc");
          //            bindLocal(target, *hugeSize.second,
          //                      ConstantExpr::alloc(0,
          //                      Context::get().getPointerWidth()));
          executeAlloc(*hugeSize.second, example, isLocal, target, zeroMemory,
                       reallocFrom);
        }
      }
    }

    if (fixedSize.first) // can be zero when fork fails
      executeAlloc(*fixedSize.first, example, isLocal, target, zeroMemory,
                   reallocFrom);
  }
}

void Executor::executeFree(ExecutionState &state, ref<Expr> address,
                           KInstruction *target) {
  address = concretizeExpr(state, address);
  address = optimizer.optimizeExpr(address, true);
  StatePair zeroPointer = fork(state, Expr::createIsZero(address), true);
  if (zeroPointer.first) {
    if (target) {
      bindLocal(target, *zeroPointer.first, Expr::createPointer(0));
      specialFunctionHandler->trackMemory(state, target->inst->getType(),
                                          address, Expr::createPointer(0));
    }
  }
  if (zeroPointer.second) { // address != 0
    ExactResolutionList rl;
    resolveExact(*zeroPointer.second, address, rl, "free");

    for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
         it != ie; ++it) {
      const MemoryObject *mo = it->first.first;
      if (mo->isLocal) {
        terminateStateOnError(*it->second, "free of alloca", Free, NULL,
                              getAddressInfo(*it->second, address));
      } else if (mo->isGlobal) {
        terminateStateOnError(*it->second, "free of global", Free, NULL,
                              getAddressInfo(*it->second, address));
      } else {
        it->second->addressSpace.unbindObject(mo);
        if (target) {
          bindLocal(target, *it->second, Expr::createPointer(0));
          specialFunctionHandler->trackMemory(state, target->inst->getType(),
                                              address, Expr::createPointer(0));
        }
      }
    }
  }
}

void Executor::resolveExact(ExecutionState &state, ref<Expr> p,
                            ExactResolutionList &results,
                            const std::string &name) {
  p = optimizer.optimizeExpr(p, true);
  // XXX we may want to be capping this?
  ResolutionList rl;
  state.addressSpace.resolve(state, solver, p, rl);

  ExecutionState *unbound = &state;
  for (ResolutionList::iterator it = rl.begin(), ie = rl.end(); it != ie;
       ++it) {
    ref<Expr> inBounds = EqExpr::create(p, it->first->getBaseExpr());

    StatePair branches = fork(*unbound, inBounds, true);

    if (branches.first)
      results.push_back(std::make_pair(*it, branches.first));

    unbound = branches.second;
    if (!unbound) // Fork failure
      break;
  }

  if (unbound) {
    SPDLOG_DEBUG("[state {}] {} resolve exact unbound", state.getID(), name);
    terminateStateOnError(*unbound, "memory error: invalid pointer: " + name,
                          Ptr, NULL, getAddressInfo(*unbound, p));
  }
}

bool Executor::analyzeSymbolicAddr(ExecutionState &state, ref<Expr> address,
                                   SymbolicAddressVisitor &visitor,
                                   std::set<uint64_t> &baseAddresses) {
  visitor.visitAddr(address);
  if (visitor.getArrays().empty()) {
    SPDLOG_DEBUG("[state {}] Not a symbolic address {}", state.getID(),
                 address->str());
    return false;
  }
  bool found = false;
  for (const ConstantExpr *ce : visitor.getConstants()) {
    if (memory->isValidAddress(ce->getZExtValue())) {
      uint64_t baseAddr = ce->getZExtValue();
      baseAddresses.insert(baseAddr);
      found = true;
      SPDLOG_DEBUG("[state {}] Found baseAddr: {}", state.getID(), baseAddr);
    }
  }
  for (const Array *arr : visitor.getArrays()) {
    auto sym = lazyInitManager.getSymbolicPointer(arr);
    if (std::get<0>(sym) != 0) {
      uint64_t baseAddr = std::get<0>(sym);
      baseAddresses.insert(baseAddr);
      found = true;
      SPDLOG_DEBUG("[state {}] Found baseAddr(symPtr): {}", state.getID(),
                   baseAddr);
    }
  }
  return found;
}

void Executor::selectCandidateConcreteAddr(ExecutionState &state, uint64_t base,
                                           uint64_t size,
                                           std::vector<uint64_t> *baseAddrs,
                                           std::vector<uint64_t> &candAddrs,
                                           std::vector<uint64_t> &candidates) {
  std::size_t k = 3;
  if (candAddrs.size() <= k) {
    for (uint64_t addr : candAddrs) {
      candidates.push_back(addr);
    }
  }
  std::vector<uint64_t> offsets;
  uint64_t minOffset = size;
  uint64_t maxOffset = 0;
  std::set<uint64_t> baseOffsets;
  for (uint64_t addr : *baseAddrs) {
    if (addr >= base && addr < base + size) {
      uint64_t offset = addr - base;
      baseOffsets.insert(offset);
      minOffset = std::min(minOffset, offset);
      maxOffset = std::max(maxOffset, offset);
    }
  }
  // Let's assume we have only one offset: minOffset
  for (uint64_t addr : candAddrs) {
    if (addr >= base && addr < base + size) {
      uint64_t offset = addr - base;
      if (offset < minOffset) {
        // Too small
        continue;
      }
      offsets.push_back(addr - base);
    }
  }
  // Select k candidates including min, max, mid
  if (offsets.empty()) {
    // No candidate
    return;
  }
  candidates.push_back(base + offsets[0]);
  if (offsets.size() > 1) {
    candidates.push_back(base + offsets[offsets.size() - 1]);
  }
  if (offsets.size() > 2) {
    candidates.push_back(base + offsets[offsets.size() / 2]);
  }
}

void Executor::sortCandidates(uint64_t base, std::vector<uint64_t> &allCand) {
  std::vector<uint64_t> offsets;
  std::vector<uint64_t> remainings;
  for (uint64_t ofs : allCand) {
    if (ofs >= base) {
      offsets.push_back(ofs);
    } else {
      remainings.push_back(ofs);
    }
  }
  if (offsets.empty()) {
    allCand.clear();
    for (uint64_t ofs : remainings) {
      allCand.push_back(ofs);
    }
    return;
  }
  allCand.clear();
  allCand.push_back(offsets[0]);
  uint64_t mid = offsets.size() / 2;
  std::queue<uint64_t> upper;
  std::queue<uint64_t> lower;
  for (uint64_t i = 1; i < mid; i++) {
    lower.push(offsets[mid - i]);
  }
  for (uint64_t i = mid + 1; i < offsets.size(); i++) {
    upper.push(offsets[i]);
  }
  bool up = true;
  while (!lower.empty() || !upper.empty()) {
    if (up) {
      up = !upper.empty();
    } else {
      up = lower.empty();
    }
    if (up) {
      allCand.push_back(upper.front());
      upper.pop();
    } else {
      allCand.push_back(lower.front());
      lower.pop();
    }
    up = !up;
  }
  for (uint64_t ofs : remainings) {
    allCand.push_back(ofs);
  }
}

void Executor::collectCandidateConcreteAddr(ExecutionState &state,
                                            ref<Expr> address,
                                            std::set<uint64_t> &baseAddresses,
                                            llvm::Type *type, uint64_t typeSize,
                                            std::vector<uint64_t> &candidates,
                                            uint32_t k) {
  // Null pointer
  solver->setTimeout(coreSolverTimeout);
  // ref<Expr> isNull = EqExpr::create(address, Expr::createPointer(0));
  // bool result;
  // if (!solver->mayBeTrue(state, isNull, result)) {
  //   SPDLOG_DEBUG("[state {}] Solver failure (null pointer).",
  //   state.getID()); return;
  // }
  // if (result) {
  //   candidates.push_back(0);
  // }

  // Find memory object from baseAddr
  ObjectPair op;
  std::map<const MemoryObject *, std::vector<uint64_t> *> mos;
  for (uint64_t baseAddr : baseAddresses) {
    if (!state.addressSpace.resolveOne(Expr::createPointer(baseAddr), op)) {
      SPDLOG_DEBUG(
          "[state {}] Cannot resolve memory object from baseAddr: {}, size: {}",
          state.getID(), baseAddr, typeSize);
      return;
    }
    const MemoryObject *mo = op.first;
    if (mo) {
      if (mos.count(mo) == 0) {
        mos[mo] = new std::vector<uint64_t>();
      }
      mos[mo]->push_back(baseAddr);
    }
  }

  // Alignment
  uint64_t alignment = typeSize;
  if (type->isSized()) {
    alignment = kmodule->targetData->getPrefTypeAlignment(type);
  }
  if (!bits64::isPowerOfTwo(alignment)) {
    alignment = 8;
  }

  for (auto elem : mos) {
    const MemoryObject *mo = elem.first;
    ref<Expr> inBound = mo->getBoundsCheckPointer(address);
    bool result;
    if (!solver->mayBeTrue(state, inBound, result)) {
      SPDLOG_DEBUG("[state {}] Solver failure (bounds check).", state.getID());
      continue;
    }
    if (!result) {
      SPDLOG_DEBUG("[state {}] Address is out of bound: {}", state.getID(),
                   address->str());
      continue;
    }
    SPDLOG_DEBUG(
        "[state {}] Symbolic address is in bound {}(size {}) (al {}) {}",
        state.getID(), mo->address, mo->size, alignment, mo->name);
    // Collect all possible candidate concrete addresses
    std::vector<uint64_t> allCand;
    uint64_t maxAddr = mo->address + mo->size;
    for (uint64_t offset = 0; offset < mo->size; offset += alignment) {
      allCand.push_back(offset);
    }
    uint64_t baseAddr = mo->address + mo->size;
    for (uint64_t bs : *elem.second) {
      if (bs < baseAddr) {
        baseAddr = bs;
      }
    }
    // Sort allCand
    sortCandidates(baseAddr - mo->address, allCand);
    int count = 0;
    for (uint64_t offset : allCand) {
      count++;
      if (count > 100) {
        SPDLOG_DEBUG("[state {}] Too many candidates ({}).", state.getID(),
                     allCand.size());
        break;
      }
      uint64_t i = mo->address + offset;
      ref<Expr> candidateAddr = Expr::createPointer(i);
      ref<Expr> eqExpr = EqExpr::create(address, candidateAddr);
      if (!solver->mayBeTrue(state, eqExpr, result)) {
        SPDLOG_DEBUG("[state {}] Solver failure (concrete address).",
                     state.getID());
        continue;
      }
      if (result) {
        SPDLOG_DEBUG("[state {}] Concrete address: {} ({} + {})", state.getID(),
                     candidateAddr->str(), mo->address, offset);
        candidates.push_back(i);
        // select max k candidates
        if (candidates.size() >= k)
          break;
      } else {
        SPDLOG_DEBUG("[state {}] Concrete address failed: {} ({} + {})",
                     state.getID(), candidateAddr->str(), mo->address, offset);
      }
    }
  }
  solver->setTimeout(time::Span());
  for (auto elem : mos) {
    delete elem.second;
  }
}

void Executor::handleSymbolicPointer(ExecutionState &state, bool isWrite,
                                     ref<Expr> address,
                                     ref<Expr> value /* undef if read */,
                                     KInstruction *target) {
  int remainingFork = stateGroup.getRemainFork(&state);
  SPDLOG_INFO("[state {}] Handle symbolic pointer [fork {}]: {}", state.getID(),
              remainingFork, address->str());
  Expr::Width type = (isWrite ? value->getWidth()
                              : getWidthForLLVMType(target->inst->getType()));
  uint64_t bytes = Expr::getMinBytesForWidth(type);
  // Look before you leap:
  // check remaining fork count and minimize the SMT solver usage
  bool doNull = state.passedTarget;
  bool doFork = remainingFork > 1;
  if (!doFork) {
    doNull = doNull && theRNG.getBool();
  }
  ref<Expr> check;
  solver->setTimeout(coreSolverTimeout);
  bool solverResult;
  ExecutionState *nonNullState = &state;
  if (doNull) {
    check = EqExpr::create(address, Expr::createPointer(0));
    if (!solver->mayBeTrue(state, check, solverResult)) {
      SPDLOG_DEBUG("[state {}] Solver failure (null pointer).", state.getID());
      terminateStateEarly(state, "Failed to concretize symbolic address");
      return;
    }
    if (solverResult) {
      if (doFork) {
        StatePair sp = fork(state, check, true);
        if (sp.first) {
          terminateStateOnError(
              *sp.first, "memory error: null pointer dereference (sym ptr)",
              Ptr, NULL, getAddressInfo(*sp.first, address));
        }
        nonNullState = sp.second;
      } else {
        addConstraint(state, check);
        terminateStateOnError(
            state, "memory error: null pointer dereference (sym ptr)", Ptr,
            NULL, getAddressInfo(state, address));
        return;
      }
    }
  } else {
    check = NeExpr::create(address, Expr::createPointer(0));
    if (!solver->mayBeTrue(state, check, solverResult)) {
      SPDLOG_DEBUG("[state {}] Solver failure (non-null pointer).",
                   state.getID());
      terminateStateEarly(state, "Failed to concretize symbolic address");
      return;
    }
    if (!solverResult) {
      addConstraint(state, EqExpr::createIsZero(address));
      terminateStateOnError(state, "memory error: null pointer dereference",
                            Ptr, NULL, getAddressInfo(state, address));
      return;
    }
  }
  if (nonNullState == nullptr)
    return;

  SymbolicAddressVisitor saVisitor;
  std::set<uint64_t> baseAddresses;
  uint32_t k = 3;
  if (k >= 0 && remainingFork < k) {
    k = remainingFork - 1;
  }
  bool tryConcretize = false;
  if (analyzeSymbolicAddr(*nonNullState, address, saVisitor, baseAddresses)) {
    // It has base address
    // Find all possible candidate concrete addresses
    std::vector<uint64_t> candidates;
    collectCandidateConcreteAddr(*nonNullState, address, baseAddresses,
                                 target->inst->getType(), bytes, candidates, k);
    std::vector<ref<Expr>> conditions;
    for (uint64_t candidate : candidates) {
      ref<Expr> candidateExpr = Expr::createPointer(candidate);
      ref<Expr> eqExpr = EqExpr::create(address, candidateExpr);
      conditions.push_back(eqExpr);
    }
    if (conditions.empty()) {
      tryConcretize = true;
    } else {
      std::vector<ExecutionState *> branches;
      branch(*nonNullState, conditions, branches);
      for (ExecutionState *es : branches) {
        if (es == nullptr) {
          continue;
        }
        executeMemoryOperation(*es, isWrite, address, value, target);
      }
    }
  } else {
    // 1. It is a symbolic pointer read from new symbolic object
    const Array *arr = *saVisitor.getArrays().begin();
    MemoryObject *mo = lazyAllocation(
        *nonNullState, address, target->inst->getType(), target, arr, 0, 0);
    ref<Expr> result = mo ? mo->getBaseExpr() : Expr::createPointer(0);
    if (!solver->mayBeTrue(*nonNullState, EqExpr::create(address, result),
                           solverResult)) {
      SPDLOG_DEBUG("[state {}] Solver failure (concrete address).",
                   state.getID());
      terminateStateEarly(*nonNullState,
                          "Failed to concretize symbolic address");
      return;
    }
    if (solverResult) {
      addConstraint(*nonNullState, EqExpr::create(address, result));
      executeMemoryOperation(*nonNullState, isWrite, result, value, target);
    } else {
      tryConcretize = true;
    }
  }
  // Last chance: concretize the address
  if (tryConcretize) {
    ObjectPair op;
    ref<ConstantExpr> candidate;
    bool result;
    if (!nonNullState->addressSpace.resolveOne(*nonNullState, solver, address,
                                               op, result)) {
      SPDLOG_DEBUG("[state {}] Cannot resolve memory object from baseAddr: {}",
                   state.getID(), address->str());
      candidate = toConstant(*nonNullState, address, "resolveOne failure");
      bool success = nonNullState->addressSpace.resolveOne(candidate, op);
      if (!success) {
        SPDLOG_DEBUG(
            "[state {}] Cannot resolve memory object from baseAddr: {}",
            state.getID(), candidate->str());
        terminateStateEarly(*nonNullState,
                            "Failed to concretize symbolic address");
        return;
      }
    }
    if (result) {
      const MemoryObject *mo = op.first;
      ref<Expr> offset = mo->getOffsetExpr(address);
      ref<Expr> check = mo->getBoundsCheckOffset(offset, bytes);
      SPDLOG_DEBUG("[state {}] Force concretieze to address {}",
                   nonNullState->getID(), mo->address);
      check = optimizer.optimizeExpr(check, true);
      bool inBounds;
      solver->setTimeout(coreSolverTimeout);
      bool success = solver->mayBeTrue(*nonNullState, check, inBounds);
      solver->setTimeout(time::Span());
      if (!success) {
        terminateStateEarly(*nonNullState, "Query timed out (bounds check).");
        return;
      }
      if (inBounds) {
        addConstraint(*nonNullState, check);
      }
    }
    candidate = toConstant(*nonNullState, address, "resolveOne failure");
    executeMemoryOperation(*nonNullState, isWrite, candidate, value, target);
  }
}

void Executor::executeMemoryOperation(
    ExecutionState &state, bool isWrite, ref<Expr> address,
    ref<Expr> value /* undef if read */,
    KInstruction *target /* undef if write */) {
  Expr::Width type = (isWrite ? value->getWidth()
                              : getWidthForLLVMType(target->inst->getType()));
  uint64_t bytes = Expr::getMinBytesForWidth(type);
  uint64_t ptrWidth = Context::get().getPointerWidth() / 8;
  if (SimplifySymIndices) {
    if (!isa<ConstantExpr>(address)) {
      address = state.constraints.simplifyExpr(address);
      SPDLOG_TRACE("simplified sym index: {}", address->str());
    } else if (executionType == ExecutionType::FromSnapshot) {
      uint64_t addr = cast<ConstantExpr>(address)->getZExtValue();
      if (!memory->isValidAddress(addr)) {
        if (snapshottedExternalObjects.find(addr) !=
            snapshottedExternalObjects.end()) {
          addr = snapshottedExternalObjects[addr];
          address = ConstantExpr::create(addr, ptrWidth * 8);
        }
      }
    }
    if (isWrite && !isa<ConstantExpr>(value))
      value = state.constraints.simplifyExpr(value);
  }
  // Still symbolic address: concretize it
  if (!isa<ConstantExpr>(address)) {
    queryCounter++;
    if (usingSeeds) {
      address = concretizeExpr(state, address);
    } else {
      handleSymbolicPointer(state, isWrite, address, value, target);
      return;
    }
  }

  address = optimizer.optimizeExpr(address, true);

  // fast path: single in-bounds resolution
  ObjectPair op;
  bool success;
  solver->setTimeout(coreSolverTimeout);
  if (!state.addressSpace.resolveOne(state, solver, address, op, success)) {
    SPDLOG_DEBUG("[state {}] resolveOne failure: {}", state.getID(),
                 address->str());
    address = toConstant(state, address, "resolveOne failure");
    success = state.addressSpace.resolveOne(cast<ConstantExpr>(address), op);
  }
  solver->setTimeout(time::Span());
  if (success) {
    const MemoryObject *mo = op.first;

    if (MaxSymArraySize && mo->size >= MaxSymArraySize) {
      address = toConstant(state, address, "max-sym-array-size");
    }

    ref<Expr> offset = mo->getOffsetExpr(address);
    ref<Expr> check = mo->getBoundsCheckOffset(offset, bytes);
    check = optimizer.optimizeExpr(check, true);

    bool inBounds;
    solver->setTimeout(coreSolverTimeout);
    bool success = solver->mustBeTrue(state, check, inBounds);
    solver->setTimeout(time::Span());
    if (!success) {
      state.pc = state.prevPC;
      terminateStateEarly(state, "Query timed out (bounds check).");
      return;
    }
    // if (inBounds && state.lazyObjectSizeMap.count(mo) > 0) {
    //   // Object from lazy init (size is symbolic)
    //   uint64_t size = state.lazyObjectSizeMap[mo];
    //   ref<Expr> sizeExpr = ConstantExpr::create(size,
    //   Context::get().getPointerWidth()); ref<Expr> check =
    //   mo->getBoundsCheckOffset(offset, size);
    //   solver->setTimeout(coreSolverTimeout);
    //   bool success = solver->mustBeTrue(state, check, inBounds);
    //   solver->setTimeout(time::Span());
    //   if (!success) {
    //     state.pc = state.prevPC;
    //     terminateStateEarly(state, "Query timed out (bounds check).");
    //     return;
    //   }
    //   if (!inBounds) {
    //     // Out of bounds -> increase the size only when base mode
    //   }
    // }

    if (inBounds) {
      const ObjectState *os = op.second;
      // First, update the memory graph (only in MakeSnapshot mode)
      if (executionType == ExecutionType::MakeSnapshot || DumpSnapshot) {
        uint64_t address_value = 0;
        if (isa<ConstantExpr>(address)) {
          address_value = dyn_cast<ConstantExpr>(address)->getZExtValue();
        }
        // Filter out the case where type is definitely not a pointer
        if (target && bytes == ptrWidth) {
          // If it's 8 byte, it can be a pointer, regardless of type
          if (isa<StoreInst>(target->inst)) {
            onPtrAccess(state, address_value, mo, true, value);
          } else if (isa<LoadInst>(target->inst)) {
            ref<Expr> result = os->read(offset, type);
            onPtrAccess(state, address_value, mo, false, result);
          }
        } else {
          if (isWrite) {
            onMemAccess(state, mo, address_value, true, value);
          } else {
            ref<Expr> result = os->read(offset, type);
            onMemAccess(state, mo, address_value, false, result);
          }
        }
      }
      // Second, do the actual read/write
      if (isWrite) {
        if (os->readOnly) {
          terminateStateOnError(state, "memory error: object read only",
                                ReadOnly);
        } else {
          ObjectState *wos = state.addressSpace.getWriteable(mo, os);
          // TODO: Check if the address is from lazy init
          bool isPointerType = (bytes == ptrWidth);
          wos->write(offset, value);
          if (!isa<ConstantExpr>(value)) {
            // Trace the symbolic write
            state.trackedSymbolicObjects[mo->address];
            value = state.constraints.simplifyExpr(value);
            SPDLOG_TRACE("[state {}] symbolic write at {}: {} ({})",
                         state.getID(), address->str(), value->str(), mo->name);
          } else {
            SPDLOG_TRACE(
                "[state {}] os->write {} from = {}({}+{}) size {} ({})",
                state.getID(), value->str(), address->str(), mo->address,
                offset->str(), bytes, mo->name);
          }
          // Record the write
          if (state.stack.back().nonLocalsWritten.find(mo) !=
                  state.stack.back().nonLocalsWritten.end() ||
              !llvm::isa<ConstantExpr>(value))
            state.stack.back().nonLocalsWritten[mo] =
                std::make_pair(offset, value);
          // Give name to object
          if (target && isa<llvm::StoreInst>(target->inst)) {
            llvm::StoreInst *si = dyn_cast<llvm::StoreInst>(target->inst);
            const Value *v = si->getValueOperand();
            if (v->hasName()) {
              std::string name =
                  si->getFunction()->getName().str() + "." + v->getName().str();
              if (mo->name.empty()) {
                name = getUniqueName(state, name, "_write", mo->address);
                mo->setName(name);
              }
            }
          }
        }
      } else {
        ref<Expr> result = os->read(offset, type);
        // Trace the symbolic read
        if (isReadExpr(result)) {
          state.trackedSymbolicObjects[mo->address];
          result = state.constraints.simplifyExpr(result);
        }
        if (!isa<ConstantExpr>(result)) {
          SPDLOG_TRACE("[state {}] symbolic read at {}: {} ({})", state.getID(),
                       address->str(), result->str(), mo->name);
          if (result->str().find("w15") != std::string::npos) {
            SPDLOG_DEBUG("[state {}] error symbolic read at {}: {} ({})",
                         state.getID(), address->str(), result->str(),
                         mo->name);
          }
        }
        // Give name to object
        if (target && isa<llvm::LoadInst>(target->inst)) {
          llvm::LoadInst *li = dyn_cast<llvm::LoadInst>(target->inst);
          if (li->hasName()) {
            std::string name =
                li->getFunction()->getName().str() + "." + li->getName().str();
            if (mo->name.empty()) {
              name = getUniqueName(state, name, "_read", mo->address);
              mo->setName(name);
            }
          }
          bool isPointerType = li->getType()->isPointerTy();
          if (isPointerType) {
            llvm::Type *pointedType = li->getType()->getPointerElementType();
            if (isa<NonConstantExpr>(result) && mo->depth < MaxLazyDepth) {
              if (llvm::FunctionType *funcType =
                      dyn_cast<llvm::FunctionType>(pointedType)) {
                SPDLOG_DEBUG("[state {}] Symbolic function pointer: {}",
                             state.getID(), result->str());
              }
              // lazy init
              LoadInst *loadInst = dyn_cast<LoadInst>(target->inst);
              Type *targetType = loadInst->getType();
              SPDLOG_DEBUG(
                  "[state {}] from mo {} ({}) lazy init at {}(d{}) type {}",
                  state.getID(), mo->name,
                  (target->isSliced ? " sliced" : " not sliced"),
                  pointerToDataString(mo->address), mo->address,
                  get_type_string(targetType));
              if (isa<ConstantExpr>(address)) {
                uint64_t cAddr =
                    dyn_cast<ConstantExpr>(address)->getZExtValue();
                expandLazyAllocation(state, result, targetType,
                                     (ObjectState *)os, target, mo->name, true,
                                     cAddr, mo, mo->depth);
              }
            }
          }
          // Add this to the Cell (register)
          if (checkMemRecord(state)) {
            if (isa<ConstantExpr>(address)) {
              uint64_t cAddr = dyn_cast<ConstantExpr>(address)->getZExtValue();
              MemRecord memRecord = MemRecord(cAddr, mo->address, isPointerType,
                                              bytes, MemRecordType::Read);
              getDestCell(state, target).mr.insert(memRecord);
            }
          }
        }
        if (interpreterOpts.MakeConcreteSymbolic)
          result = replaceReadWithSymbolic(state, result);

        SPDLOG_TRACE(
            "[state {}] read result = {} from = {}({}+{}) size {} ({})",
            state.getID(), result->str(), address->str(), mo->address,
            offset->str(), bytes, mo->name);
        bindLocal(target, state, result);
        // Record the read
        if (state.stack.back().nonLocalsRead.find(mo) !=
                state.stack.back().nonLocalsRead.end() ||
            !llvm::isa<ConstantExpr>(result))
          state.stack.back().nonLocalsRead[mo] = std::make_pair(offset, result);
      }
      return;
    } else {
      SPDLOG_DEBUG("[state {}] out of bound {}", state.getID(), check->str());
    }
  }

  SPDLOG_INFO("[state {}] ERROR address = {}", state.getID(), address->str());
  // we are on an error path (no resolution, multiple resolution, one
  // resolution with out of bounds)

  address = optimizer.optimizeExpr(address, true);
  ResolutionList rl;
  solver->setTimeout(coreSolverTimeout);
  bool incomplete = state.addressSpace.resolve(state, solver, address, rl, 0,
                                               coreSolverTimeout);
  solver->setTimeout(time::Span());

  if (rl.size() > 8) {
    terminateStateOnError(state, "memory error: too many candidates", Ptr, NULL,
                          getAddressInfo(state, address));
    return;
  }

  // XXX there is some query wasteage here. who cares?
  ExecutionState *unbound = &state;

  for (ResolutionList::iterator i = rl.begin(), ie = rl.end(); i != ie; ++i) {
    const MemoryObject *mo = i->first;
    const ObjectState *os = i->second;
    ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);

    StatePair branches = fork(*unbound, inBounds, true);
    ExecutionState *bound = branches.first;

    // bound can be 0 on failure or overlapped
    if (bound) {
      if (isWrite) {
        if (os->readOnly) {
          terminateStateOnError(*bound, "memory error: object read only",
                                ReadOnly);
        } else {
          ObjectState *wos = bound->addressSpace.getWriteable(mo, os);
          wos->write(mo->getOffsetExpr(address), value);
        }
      } else {
        ref<Expr> result = os->read(mo->getOffsetExpr(address), type);
        bindLocal(target, *bound, result);
      }
    }

    unbound = branches.second;
    if (!unbound)
      break;
  }

  // XXX should we distinguish out of bounds and overlapped cases?
  if (unbound) {
    if (incomplete) {
      terminateStateEarly(*unbound, "Query timed out (resolve).");
    } else {
      if (!DisableMemoryCheck) {
        terminateStateOnError(*unbound, "memory error: out of bound pointer",
                              Ptr, NULL, getAddressInfo(*unbound, address));
      }
    }
  }
}

std::string Executor::getUniqueName(ExecutionState &state, std::string name,
                                    std::string prefix, uint64_t address) {
  name = llvm::StringRef(name.c_str()).trim().str();
  std::string namePrefix = "";
  // SPDLOG_DEBUG("[state {}] getUniqueName: {} {}", state.getID(), name,
  // prefix);
  if (name.empty()) {
    namePrefix = prefix;
  } else {
    if (state.arrayNames.count(name) == 0) {
      state.arrayNames.insert(name);
      return name;
    }
    namePrefix = fmt::format("{}_{}", prefix, name);
  }

  if (address)
    namePrefix = fmt::format("{}_{}", namePrefix, address);
  if (state.arrayNames.count(namePrefix) == 0) {
    state.arrayNames.insert(name);
    return namePrefix;
  }
  std::string uniqueName = namePrefix;
  uint32_t id = 0;
  while (!state.arrayNames.insert(uniqueName).second) {
    uniqueName = fmt::format("{}_{}", namePrefix, id++);
  }
  return uniqueName;
}

void Executor::executeMakeSymbolic(ExecutionState &state,
                                   const MemoryObject *mo,
                                   const std::string name, bool updateDepth) {
  // Create a new object state for the memory object (instead of a copy).
  if (!replayKTest) {
    countSymbolic++;
    // Find a unique name for this array.  First try the original name,
    // or if that fails try adding a unique identifier.
    if (updateDepth)
      mo->depth += 1;
    const ObjectState *os = state.addressSpace.findObject(mo);
    std::string uniqueName = getUniqueName(state, name, "sym", mo->address);
    const Array *array = arrayCache.CreateArray(uniqueName, mo->size);
    bindObjectInState(state, mo, false, array);
    state.addSymbolic(mo, array);
    // Add to the shadow memory
    // if (state.shadowAddressSpace.initialized) {
    //   const ObjectState *os = state.addressSpace.findObject(mo);
    //   ObjectState *sos = state.shadowAddressSpace.copyObjectState(mo, os);
    //   state.shadowAddressSpace.bindObject(mo, sos);
    // }
    SPDLOG_INFO("[state {}] Make symbolic name: {}(array {}) at {}(d{}) with "
                "size {} at {}",
                state.getID(), uniqueName, array->getName(),
                pointerToDataString(mo->address), mo->address, mo->size,
                state.pc->getSourceLocation());

    std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
        seedMap.find(&state);
    if (it != seedMap.end()) { // In seed mode we need to add this as a
      // binding.
      for (std::vector<SeedInfo>::iterator siit = it->second.begin(),
                                           siie = it->second.end();
           siit != siie; ++siit) {
        SeedInfo &si = *siit;
        KTestObject *obj = si.getNextInput(mo, NamedSeedMatching);

        if (!obj) {
          if (ZeroSeedExtension) {
            std::vector<unsigned char> &values = si.assignment.bindings[array];
            values = std::vector<unsigned char>(mo->size, '\0');
          } else if (!AllowSeedExtension) {
            terminateStateOnError(state, "ran out of inputs during seeding",
                                  User);
            break;
          }
        } else {
          if (obj->numBytes != mo->size &&
              ((!(AllowSeedExtension || ZeroSeedExtension) &&
                obj->numBytes < mo->size) ||
               (!AllowSeedTruncation && obj->numBytes > mo->size))) {
            std::stringstream msg;
            msg << "replace size mismatch: " << mo->name << "[" << mo->size
                << "]"
                << " vs " << obj->name << "[" << obj->numBytes << "]"
                << " in test\n";

            terminateStateOnError(state, msg.str(), User);
            break;
          } else {
            std::vector<unsigned char> &values = si.assignment.bindings[array];
            values.insert(values.begin(), obj->bytes,
                          obj->bytes + std::min(obj->numBytes, mo->size));
            if (ZeroSeedExtension) {
              for (unsigned i = obj->numBytes; i < mo->size; ++i)
                values.push_back('\0');
            }
          }
        }
      }
    }
  } else {
    ObjectState *os = bindObjectInState(state, mo, false);
    if (replayPosition >= replayKTest->numObjects) {
      terminateStateOnError(state, "replay count mismatch", User);
    } else {
      KTestObject *obj = &replayKTest->objects[replayPosition++];
      if (obj->numBytes != mo->size) {
        terminateStateOnError(state, "replay size mismatch", User);
      } else {
        for (unsigned i = 0; i < mo->size; i++)
          os->write8(i, obj->bytes[i]);
      }
    }
  }
}

std::pair<const Array *, ref<Expr>>
Executor::getSymbolicValue(ExecutionState &state, uint64_t size,
                           std::string name, uint64_t writeToAddr,
                           uint64_t base) {
  std::string uniqueName = getUniqueName(state, name, "_symval", 0);
  SPDLOG_INFO("[state {}] Make symbolic value {} size {}", state.getID(),
              uniqueName, size);
  const Array *array = arrayCache.CreateArray(uniqueName, size);
  state.addSymbolic(array, size, writeToAddr, base);
  ref<Expr> value(0); // symbolic address of mo
  UpdateList *ul = new UpdateList(array, 0);
  for (uint32_t i = 0; i < size; i++) {
    uint32_t idx = Context::get().isLittleEndian() ? i : (size - i - 1);
    ref<Expr> tmpByte =
        ReadExpr::create(*ul, ConstantExpr::create(idx, Expr::Int32));
    value = i ? ConcatExpr::create(tmpByte, value) : tmpByte;
  }
  return std::make_pair(array, value);
}

std::pair<const Array *, ref<Expr>>
Executor::getSymbolicPointer(ExecutionState &state, const MemoryObject *mo) {
  // Make mo symbolic
  std::vector<uint8_t> data;
  const ObjectState *os = state.addressSpace.findObject(mo);
  os->readConcreteStore(data);
  executeMakeSymbolic(state, mo, mo->name);
  state.symbolics.back().setLazy("");
  const Array *arr = state.symbolics.back().getArray();
  MemArchive ma = MemArchive(false, true, mo->address, mo->address, mo->size,
                             data, mo->address, mo->address, mo->size);
  lazyInitManager.addOriginalObject(arr, ma);

  llvm::StringRef sr(mo->name);
  std::string name = "";
  if (sr.trim().empty())
    name = "_sym_ptr";
  else
    name = fmt::format("_sym_addr_{}", sr.trim().str());
  std::pair<const Array *, ref<Expr>> addr =
      getSymbolicValue(state, 8, name, 0, 0);
  state.symbolics.back().setSymPtr(mo->address);
  lazyInitManager.addSymbolicPointer(addr.first, mo->address, mo, arr, true);
  lazyInitManager.addSymbolicData(arr, data);
  state.addConstraint(OrExpr::create(
      EqExpr::create(mo->getBaseExpr(), addr.second),
      EqExpr::create(ConstantExpr::create(0, Expr::Int64), addr.second)));
  return addr;
}

void Executor::executeMakePointerSymbolic(ExecutionState &state,
                                          const MemoryObject *mo,
                                          uint64_t offset, ref<Expr> addr,
                                          uint64_t destOffset,
                                          const MemoryObject *symbolized) {
  const ObjectState *os = state.addressSpace.findObject(mo);
  ref<Expr> zero = ConstantExpr::create(0, Expr::Int64);
  ref<Expr> addrPlusOffset =
      AddExpr::create(addr, ConstantExpr::create(destOffset, Expr::Int64));
  SPDLOG_DEBUG("[state {}] pointer symbolic {} or null at {} + {}",
               state.getID(), addrPlusOffset->str(), mo->address, offset);
  addr = addrPlusOffset;
  ObjectState *wos = state.addressSpace.getWriteable(mo, os);
  wos->write(offset, addr);
  lazyInitManager.addLazyInitAssignment(mo->address + offset,
                                        (MemoryObject *)symbolized);
  state.memTraces.push_back(MemTrace(mo->address + offset, mo->address,
                                     mo->size, symbolized->address,
                                     symbolized->address + destOffset, addr));
}

const Array *Executor::executeMakeFieldSymbolic(
    ExecutionState &state, const MemoryObject *mo, uint32_t offset,
    llvm::Type *type, uint64_t size, const std::string name, bool isSpecial) {
  // Create a new object state for the memory object (instead of a copy).
  // Find a unique name for this array.  First try the original name,
  // or if that fails try adding a unique identifier.
  std::string uniqueName = name;
  if (state.arrayNames.count(uniqueName) > 0) {
    uniqueName = getUniqueName(
        state, name, fmt::format("_field_o{}_s{}", offset, size), mo->address);
    SPDLOG_INFO(
        "[state {}] Make field symbolic {} at {} (d{}) + {} with size {}",
        state.getID(), uniqueName, pointerToDataString(mo->address),
        mo->address, offset, size);
  }
  std::vector<uint8_t> originalData;
  const ObjectState *originalOS = state.addressSpace.findObject(mo);
  ref<Expr> readValue = originalOS->read(offset, size * 8);
  if (!isa<ConstantExpr>(readValue)) {
    SPDLOG_DEBUG("[state {}] Ignore already symbolic field {}", state.getID(),
                 readValue->str());
    return nullptr;
  }
  ref<ConstantExpr> ce = dyn_cast<ConstantExpr>(readValue);
  originalOS->readConcreteStore(originalData);
  auto res = getSymbolicValue(state, size, uniqueName, mo->address + offset,
                              mo->address);
  const Array *array = res.first;
  ref<Expr> value = res.second;
  // lazyInitManager.addSymbolicValue(array, ce);
  state.symbolics.back().setHeap();
  const ObjectState *os = state.addressSpace.findObject(mo);
  // If not a pointer, save the original value to the array
  std::vector<uint8_t> data;
  for (uint32_t i = offset; i < offset + size; i++) {
    data.push_back(originalData[i]);
  }
  MemArchive ma = MemArchive(false, false, mo->address + offset, mo->address,
                             size, data, mo->address, offset, size);
  lazyInitManager.addOriginalObject(array, ma);
  SPDLOG_DEBUG(
      "[state {}] not ptr preserve original value {}: {} size {} = {}, {}",
      state.getID(), array->getName(), mo->address, mo->size, data.size(),
      toDataString(data));
  ObjectState *wos = state.addressSpace.getWriteable(mo, os);
  wos->write(offset, value);
  if (state.metaData.isTypeBase()) {
    // state.shadowMemory.push_back(std::make_pair(symbolicData,
    // symbolicDataOS));
    state.trackedSymbolicObjects[mo->address].push_back(TrackedSymbolicObject(
        mo->address, offset, size, value, true, !state.targetStack.empty(),
        true, array->getName()));
  }
  if (state.symbolicFields.find(mo) == state.symbolicFields.end()) {
    std::vector<std::pair<int, int>> field{
        std::make_pair((int)offset, (int)size)};
    state.symbolicFields[mo] = field;
  } else {
    state.symbolicFields[mo].push_back(std::make_pair((int)offset, (int)size));
  }
  return array;
}

void getTerminalNodes(
    MemGraph &graph, uint64_t start,
    std::unordered_set<uint64_t> &terminalNodes,
    std::unordered_set<uint64_t> &visited,
    std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> &nodes) {
  std::set<uint64_t> filter;
  // map: address -> base, size, value, is_ptr
  // std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t, bool>> nodes;
  std::queue<uint64_t> q;
  q.push(start);

  while (!q.empty()) {
    uint64_t curr = q.front();
    q.pop();

    visited.insert(curr);

    bool is_terminal = true;
    // First, check all internal fields if it's a pointer to the start of the
    // object
    for (auto neighbor : graph.getOutgoingMemEdges(curr)) {
      uint64_t src = neighbor->getPtrEdge()->getFrom()->getAddress();
      uint64_t tgt = neighbor->getPtrEdge()->getTo()->getAddress();
      nodes[src] = std::make_tuple(neighbor->getFrom()->getAddress(),
                                   neighbor->getFrom()->getSize(), tgt);
      if (visited.count(tgt) == 0) {
        if (filter.count(tgt) > 0) {
          continue;
        }
        filter.insert(tgt);
        q.push(tgt);
        is_terminal = false;
      }
    }
    // For the rest, check pointer access to that address
    auto &outPtrs = graph.getOutgoingPtrEdges(curr);
    if (!outPtrs.empty()) {
      // Use latest pointer
      auto ptrEdge = outPtrs.back();
      uint64_t tgt = ptrEdge->getTo()->getAddress();
      nodes[curr] =
          std::make_tuple(ptrEdge->getFrom()->getBaseAddr(),
                          ptrEdge->getFrom()->getMemNode()->getSize(), tgt);
      if (visited.count(tgt) == 0) {
        if (filter.count(tgt) == 0) {
          filter.insert(tgt);
          q.push(tgt);
          is_terminal = false;
        }
      }
    }

    if (is_terminal) {
      SPDLOG_DEBUG("Terminal node: {}(d{}) : {}", pointerToDataString(curr),
                   curr, pointerToBigEndianString(curr));
      terminalNodes.insert(curr);
    }
  }
}

void dumpBaseMemoryGraph(
    MemGraph &graph,
    std::vector<std::tuple<uint64_t, std::string, uint32_t>> &startPoints,
    std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> &nodes,
    std::string filename) {
  std::ofstream ofs(filename);
  // Dump start points
  for (auto &start : startPoints) {
    ofs << fmt::format("[start] [ptr {}] [name {}] [index {}]\n",
                       std::get<0>(start), std::get<1>(start),
                       std::get<2>(start));
  }
  // Dump nodes
  for (auto &node : nodes) {
    ofs << fmt::format("[node] [addr {}] [base {}] [size {}] [value {}]\n",
                       node.first, std::get<0>(node.second),
                       std::get<1>(node.second), std::get<2>(node.second));
  }
  ofs.close();
}

void dumpSymbolicGlobals(
    ExecutionState &state,
    std::vector<std::tuple<uint64_t, uint64_t, uint64_t, const Array *,
                           const MemoryObject *>> &symbolize,
    MemGraph &graph, std::unordered_set<uint64_t> &visited,
    std::string filename) {
  if (symbolize.empty())
    return;
  std::ofstream ofs(filename);
  for (auto &sym : symbolize) {
    uint64_t addr = std::get<0>(sym);
    uint64_t base = std::get<1>(sym);
    uint64_t size = std::get<2>(sym);
    const Array *array = std::get<3>(sym);
    const MemoryObject *mo = std::get<4>(sym);
    if (mo->name == "uni_klee_patch_id" || mo->name == "read.n_calls" ||
        mo->name == "")
      continue;
    if (visited.count(addr) > 0 || visited.count(base) > 0)
      continue; // This can be covered by memory graph
    if (mo->isGlobal) {
      ofs << fmt::format(
          "[global] [addr {}] [base {}] [size {}] [array {}] [name {}]\n", addr,
          base, size, array->getName(), mo->name);
    }
  }
  ofs.close();
}

std::vector<uint8_t> getDataFromConstantExpr(ref<klee::ConstantExpr> ce) {
  uint8_t *buffer = (uint8_t *)malloc(ce->getWidth() / 8);
  ce->toMemory(buffer);
  std::vector<uint8_t> data;
  for (unsigned i = 0; i < ce->getWidth() / 8; i++) {
    data.push_back(buffer[i]);
  }
  free(buffer);
  return data;
}

/**
 * 1. Analyze the memory graph
 * 2. Find terminal node
 * 3. Make symbolic
 */
int Executor::makeSymbolicFromSnapshot(ExecutionState &state,
                                       Snapshot *snapshot, llvm::Function *f,
                                       std::vector<ref<Expr>> &arguments) {
  if (UniKleeReplay != "") {
    SPDLOG_INFO("[state {}] Importing symbolic input from {}", state.getID(),
                UniKleeReplay);
    importSymbolicInput(state, UniKleeReplay, snapshot, f, arguments);
    return 0;
  }
  // 0. Build a blacklist for memory objects that should not be symbolized
  AddressSpace addrSpace(state.addressSpace);
  std::map<uint64_t, MemRecord> blacklist;
  std::set<uint64_t> MOblacklist;
  std::set<uint64_t> blacklist_uni;
  for (const MemRecord &mr : snapshot->blacklistedRecords) {
    // if (mr.type == MemRecordType::UniKleeMakeSymbolic) {
    //   blacklist_uni.insert(mr.addr);
    // } else {
    //   blacklist[mr.addr] = mr;
    //   MOblacklist.insert(mr.base);
    // }
    blacklist[mr.addr] = mr;
    MOblacklist.insert(mr.base);
  }
  // 1. Analyze the memory graph: start from arguments
  KFunction *kf = kmodule->functionMap[f];
  std::vector<std::tuple<uint64_t, std::string, uint32_t>> startPoints;
  int index = 0;
  SPDLOG_WARN("Analyzing memory graph for function {}", f->getName().str());
  for (llvm::Function::const_arg_iterator ai = f->arg_begin(),
                                          ae = f->arg_end();
       ai != ae; ++ai, ++index) {
    llvm::Type *argType = ai->getType();
    std::string argName = ai->getName().str();
    if (argName.empty()) {
      argName = fmt::format("arg_{}", index);
    }
    ref<ConstantExpr> argExpr(snapshot->arguments[index]);
    ref<Expr> finalExpr;
    if (TargetFunction == "main" ||
        SymbolizeLevel == SymbolizeLevelEnum::NoSymbolization) {
      finalExpr = argExpr;
    } else if (argType->isPointerTy()) {
      uint64_t addr = argExpr->getZExtValue();
      if (addr == 0 && UseNullArg) {
        SPDLOG_INFO("[state {}] arg {} is null: make symbolic pointer",
                    state.getID(), argName);
        std::pair<const Array *, ref<Expr>> saddr =
            getSymbolicValue(state, 8, argName, 0, 0);
        state.symbolics.back().setSymPtr(addr);
        finalExpr = saddr.second;
      } else {
        SPDLOG_DEBUG("[state {}] arg {} is not a valid address", state.getID(),
                     argName);
        finalExpr = argExpr;
      }
      startPoints.push_back(std::make_tuple(addr, argName, index));
    } else if (MakeAllParameterSymbolic ||
               SymbolizeLevel >= MediumSymbolization) {
      uint64_t typeSize = kmodule->targetData->getTypeStoreSize(argType);
      // TODO: check if the memory object is in the blacklist
      SPDLOG_DEBUG("[state {}] arg {}", state.getID(), argName);
      auto symVal = getSymbolicValue(state, typeSize, argName, 0, 0);
      finalExpr = symVal.second;
      state.symbolics.back().setArg(index);
      // lazyInitManager.addSymbolicValue(symVal.first, argExpr);
    } else {
      finalExpr = argExpr;
    }
    // TODO?: maybe we should make non-pointer value symbolic
    arguments.push_back(finalExpr);
    bindArgument(kf, index, state, finalExpr);
  }
  // 2. Find terminal node
  // Terminal node: no outgoing pointer or all outgoing pointers are known
  // TODO: use BFS to find terminal nodes
  // For now, only select pointer
  std::unordered_set<uint64_t> visited;
  std::unordered_set<uint64_t> terminalNodes;
  std::vector<std::tuple<uint64_t, uint64_t>> candidates;
  std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> nodes;
  std::vector<std::tuple<uint64_t, std::string, uint32_t>> usedStartPoints;
  for (auto &start : startPoints) {
    uint64_t addr = std::get<0>(start);
    std::string name = std::get<1>(start);
    // address to memory object
    PtrNode *ptrNode = snapshot->memGraph.searchPtrNode(addr);
    if (!ptrNode) {
      continue;
    }
    uint64_t offset = ptrNode->getOffset();
    ObjectPair op;
    bool resolved = state.addressSpace.resolveOne(
        ConstantExpr::create(ptrNode->getBaseAddr(), Expr::Int64), op);
    if (!resolved) {
      continue;
    }
    const MemoryObject *mo = op.first;
    if (mo->size < offset) {
      continue;
    }
    usedStartPoints.push_back(start);
    SPDLOG_INFO(fmt::format("[terminal] [root {}] [addr {}] [offset {}]", name,
                            addr, offset));
    // Find outgoing pointers
    getTerminalNodes(snapshot->memGraph, addr, terminalNodes, visited, nodes);
  }
  // Dump memory graph
  dumpBaseMemoryGraph(snapshot->memGraph, usedStartPoints, nodes,
                      interpreterHandler->getOutputFilename("base-mem.graph"));

  if (SymbolizeLevel == NoSymbolization) {
    return 0;
  }
  // 3. Make symbolic
  // Find the target by searching the back edge of terminal nodes
  int symbolicId = 0;
  std::set<uint64_t> done;

  std::queue<std::set<uint64_t> *> terminalQueue;
  std::set<uint64_t> symbolizeTargets;
  std::map<uint64_t, MemRecord> mrMap;
  // TODO: apply k-bounded symbolicalization
  if (SymbolizeLevel >= LowSymbolization) {
    std::set<uint64_t> currents, nexts;
    std::set<uint64_t> alreadyTreated;
    for (uint64_t term : terminalNodes) {
      currents.insert(term);
    }
    for (MemRecord mr : snapshot->readAccess) {
      mrMap[mr.addr] = mr;
    }
    for (int index = 0; index < SymbolizeBound; index++) {
      std::set<uint64_t> *level = new std::set<uint64_t>();
      for (auto cur : currents) {
        ObjectPair terminalOp;
        bool resolved = state.addressSpace.resolveOne(
            ConstantExpr::create(cur, Expr::Int64), terminalOp);
        MemNode *mn = snapshot->memGraph.getMemNode(cur);
        if (mn == nullptr || !resolved)
          continue;
        // if (terminalOp.first->size > 1024)
        //   continue;
        bool isAdded = false;
        for (PtrEdge *ptr :
             snapshot->memGraph.getIncomingPtrEdges(mn->getAddress())) {
          PtrNode *pn = ptr->getFrom();
          PtrNode *dpn = ptr->getTo();
          uint64_t addr = pn->getAddress();
          uint64_t baseAddr = pn->getBaseAddr();
          MemRecord mr = snapshot->getMemRecord(mrMap, addr);
          if (mr.base == 0 || mr.base != baseAddr || done.count(addr) > 0) {
            continue;
          }
          if (blacklist.count(addr) > 0 || MOblacklist.count(baseAddr) > 0) {
            SPDLOG_DEBUG(
                "Skip blacklisted pointer: {} : {} ({}) offset {} size {}",
                addr, pointerToDataString(addr), pointerToBigEndianString(addr),
                addr - baseAddr, mr.size);
            continue;
          }
          if (!mr.isPtr)
            continue;
          if (blacklist_uni.count(addr) > 0) {
            SPDLOG_INFO("Skip uni_klee blacklisted pointer: {} : {} ({}) "
                        "offset {} size {}",
                        addr, pointerToDataString(addr),
                        pointerToBigEndianString(addr), addr - baseAddr,
                        mr.size);
            continue;
          }
          ObjectPair op;
          bool resolved = addrSpace.resolveOne(
              ConstantExpr::create(baseAddr, Expr::Int64), op);
          if (!resolved) {
            SPDLOG_WARN("Cannot resolve base address: {}", baseAddr);
            continue;
          }
          if (alreadyTreated.count(baseAddr) > 0)
            continue;
          alreadyTreated.insert(baseAddr);
          isAdded = true;
          nexts.insert(baseAddr);
        }
        if (isAdded) {
          symbolizeTargets.insert(terminalOp.first->address);
          level->insert(terminalOp.first->address);
        }
      }
      terminalQueue.push(level);
      currents.clear();
      for (auto next : nexts) {
        currents.insert(next);
      }
      nexts.clear();
    }
  }

  // symbolic pointer
  std::map<uint64_t, std::pair<const Array *, ref<Expr>>> symbolicMap;
  for (uint64_t target : symbolizeTargets) {
    ObjectPair op;
    bool resolved = state.addressSpace.resolveOne(
        ConstantExpr::create(target, Expr::Int64), op);
    std::pair<const Array *, ref<Expr>> symAddr =
        std::make_pair(nullptr, op.first->getBaseExpr());
    if (op.first->isGlobal) {
      // Check if global constant
      if (globalNames.count(op.first->name) > 0) {
        const llvm::GlobalVariable *gv = globalNames[op.first->name];
        // Skip if it's a constant
        if (gv->isConstant()) {
          SPDLOG_DEBUG("[state {}] Skip constant global {}", state.getID(),
                       op.first->name);
          continue;
        }
        if (llvm::MDNode *md = gv->getMetadata("dbg")) {
          if (auto *digv = llvm::dyn_cast<llvm::DIGlobalVariable>(md)) {
            llvm::DIFile *file = digv->getFile();
            std::string directory = file->getDirectory().str();
            if (directory.find("uni-klee/runtime") != std::string::npos) {
              SPDLOG_DEBUG("[state {}] Skip uni-klee global {}", state.getID(),
                           op.first->name);
              continue;
            }
          }
        }
      }
    }
    if (SymbolizeLevel >= MediumSymbolization) {
      symAddr = getSymbolicPointer(state, op.first);
    }
    symbolicMap[target] = symAddr;
  }

  std::set<std::string> notSymbolicList = {"__exe_fs",
                                           "__exe_env",
                                           "errno_loc",
                                           "ext_errno_loc",
                                           "ext__ctype_b_loc",
                                           "ext__ctype_b_loc_ptr",
                                           "ext__ctype_tolower_loc",
                                           "ext__ctype_tolower_loc_ptr",
                                           "ext__ctype_toupper_loc",
                                           "ext__ctype_toupper_loc_ptr",
                                           "__dso_handle"};
  while (!terminalQueue.empty()) {
    std::set<uint64_t> *level = terminalQueue.front();
    for (uint64_t term : *level) {
      ObjectPair op;
      bool resolved = state.addressSpace.resolveOne(
          ConstantExpr::create(term, Expr::Int64), op);
      MemNode *mn = snapshot->memGraph.getMemNode(term);
      if (symbolicMap.count(term) == 0)
        continue;
      ref<Expr> destAddr = symbolicMap[term].second;
      for (PtrEdge *pe :
           snapshot->memGraph.getIncomingPtrEdges(mn->getAddress())) {
        ObjectPair readFrom;
        bool resolvedCheck = state.addressSpace.resolveOne(
            ConstantExpr::create(pe->getFrom()->getBaseAddr(), Expr::Int64),
            readFrom);
        if (!resolvedCheck) {
          SPDLOG_WARN("Cannot resolve base address {}",
                      pe->getFrom()->getBaseAddr());
          continue;
        }
        if (notSymbolicList.count(readFrom.first->name) > 0) {
          SPDLOG_DEBUG("Skip not symbolic {}", readFrom.first->name);
          continue;
        }
        if (readFrom.first->isGlobal) {
          if (globalNames.count(readFrom.first->name) > 0) {
            const llvm::GlobalVariable *gv = globalNames[readFrom.first->name];
            // Skip if it's a constant
            if (gv->isConstant()) {
              SPDLOG_DEBUG("[state {}] Skip constant global {}", state.getID(),
                           readFrom.first->name);
              continue;
            }
            if (llvm::MDNode *md = gv->getMetadata("dbg")) {
              if (auto *digv = llvm::dyn_cast<llvm::DIGlobalVariable>(md)) {
                llvm::DIFile *file = digv->getFile();
                std::string directory = file->getDirectory().str();
                if (directory.find("uni-klee/runtime") != std::string::npos) {
                  SPDLOG_DEBUG("[state {}] Skip uni-klee global {}",
                               state.getID(), readFrom.first->name);
                  continue;
                }
              }
            }
          }
        }
        if (SymbolizeLevel > LowSymbolization) {
          executeMakePointerSymbolic(state, readFrom.first,
                                     pe->getFrom()->getOffset(), destAddr,
                                     pe->getTo()->getOffset(), op.first);
        }
      }
    }
    terminalQueue.pop();
    level->clear();
    delete level;
  }

  // 4. Make symbolic for the rest of the memory
  std::vector<std::tuple<uint64_t, uint64_t, uint64_t, const Array *,
                         const MemoryObject *>>
      symbolize;
  if (SymbolizeLevel >= HighSymbolization ||
      SymbolizeLevel == LowSymbolization) {
    SPDLOG_INFO("Make symbolic for the rest of the memory");
    for (auto &mr : snapshot->readAccess) {
      if (mr.isPtr)
        continue;
      uint64_t pointer = mr.addr;
      if (blacklist.count(pointer) > 0 || MOblacklist.count(mr.base) > 0) {
        continue;
      }
      ObjectPair op;
      bool resolved =
          addrSpace.resolveOne(ConstantExpr::create(mr.base, Expr::Int64), op);
      if (!resolved)
        continue;
      state.addressSpace.resolveOne(ConstantExpr::create(mr.base, Expr::Int64),
                                    op);
      if (notSymbolicList.count(op.first->name) > 0) {
        SPDLOG_DEBUG("Skip not symbolic {}", op.first->name);
        continue;
      }
      if (mr.size == 8) {
        // Check if it's a pointer
        ref<Expr> value = op.second->read(mr.addr - mr.base, 64);
        if (isa<ConstantExpr>(value)) {
          uint64_t target = cast<ConstantExpr>(value)->getZExtValue();
          if (snapshottedExternalObjects.count(target) > 0 ||
              snapshottedFunctions.count(target) > 0) {
            SPDLOG_DEBUG(
                "Skip pointer to external object or function pointer: {} -> {}",
                pointer, target);
            continue;
          }
        }
      }
      if (op.first->isGlobal) {
        // Check if global constant
        if (globalNames.count(op.first->name) > 0) {
          const llvm::GlobalVariable *gv = globalNames[op.first->name];
          // Skip if it's a constant
          if (gv->isConstant()) {
            SPDLOG_DEBUG("[state {}] Skip constant global {}", state.getID(),
                         op.first->name);
            continue;
          }
          if (llvm::MDNode *md = gv->getMetadata("dbg")) {
            if (auto *digv = llvm::dyn_cast<llvm::DIGlobalVariable>(md)) {
              llvm::DIFile *file = digv->getFile();
              std::string directory = file->getDirectory().str();
              if (directory.find("uni-klee/runtime") != std::string::npos) {
                SPDLOG_DEBUG("[state {}] Skip uni-klee global {}",
                             state.getID(), op.first->name);
                continue;
              }
            }
          }
        }
      }
      // if (SymbolizeLevel == LowSymbolization &&
      //     symbolicMap.count(op.first->address) == 0) {
      //   continue;
      // }
      const Array *arr =
          executeMakeFieldSymbolic(state, op.first, pointer - mr.base, nullptr,
                                   mr.size, op.first->name, false);
      if (arr != nullptr) {
        symbolize.push_back(
            std::make_tuple(mr.addr, mr.base, mr.size, arr, op.first));
      }
    }
  }
  // Dump symbolic globals
  dumpSymbolicGlobals(
      state, symbolize, snapshot->memGraph, visited,
      interpreterHandler->getOutputFilename("base-mem.symbolic-globals"));
  return 0;
}

/**
 * Run from the main function, using the given arguments and environment
 * If it reach the target function, dump the memory state to a snapshot file
 */
void Executor::runFunctionAsMain(Function *f, int argc, char **argv,
                                 char **envp) {
  std::vector<ref<Expr>> arguments;
  int i;
  executionType = ExecutionType::MakeSnapshot;
  // for (i = 0; i < argc; i++) {
  //   std::cout << argv[i] << std::endl;
  // }
  // return;

  // force deterministic initialization of memory objects
  srand(1);
  srandom(1);

  MemoryObject *argvMO = 0;

  // In order to make uclibc happy and be closer to what the system is
  // doing we lay out the environments at the end of the argv array
  // (both are terminated by a null). There is also a final terminating
  // null that uclibc seems to expect, possibly the ELF header?

  int envc;
  for (envc = 0; envp[envc]; ++envc)
    ;

  unsigned NumPtrBytes = Context::get().getPointerWidth() / 8;
  KFunction *kf = kmodule->functionMap[f];
  assert(kf);
  Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
  if (ai != ae) {
    arguments.push_back(ConstantExpr::alloc(argc, Expr::Int32));
    if (++ai != ae) {
      Instruction *first = &*(f->begin()->begin());
      argvMO = memory->allocate((argc + 1 + envc + 1 + 1) * NumPtrBytes,
                                /*isLocal=*/false, /*isGlobal=*/true,
                                /*allocSite=*/first, /*alignment=*/8);

      if (!argvMO)
        klee_error("Could not allocate memory for function arguments");

      arguments.push_back(argvMO->getBaseExpr());
      argvMO->setName(
          fmt::format("{}_arg_{}", f->getName().str(), ai->getName().str()));

      if (++ai != ae) {
        uint64_t envp_start = argvMO->address + (argc + 1) * NumPtrBytes;
        arguments.push_back(Expr::createPointer(envp_start));

        if (++ai != ae)
          klee_error("invalid main function (expect 0-3 arguments)");
      }
    }
  }

  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);

  if (pathWriter)
    state->pathOS = pathWriter->open();
  if (symPathWriter)
    state->symPathOS = symPathWriter->open();

  if (statsTracker)
    statsTracker->framePushed(*state, 0);

  assert(arguments.size() == f->arg_size() && "wrong number of arguments");
  for (unsigned i = 0, e = f->arg_size(); i != e; ++i)
    bindArgument(kf, i, *state, arguments[i]);

  if (argvMO) {
    ObjectState *argvOS = bindObjectInState(*state, argvMO, false);

    for (int i = 0; i < argc + 1 + envc + 1 + 1; i++) {
      if (i == argc || i >= argc + 1 + envc) {
        // Write NULL pointer
        argvOS->write(i * NumPtrBytes, Expr::createPointer(0));
      } else {
        char *s = i < argc ? argv[i] : envp[i - (argc + 1)];
        int j, len = strlen(s);

        MemoryObject *arg =
            memory->allocate(len + 1, /*isLocal=*/false, /*isGlobal=*/true,
                             /*allocSite=*/state->pc->inst, /*alignment=*/8);
        if (!arg)
          klee_error("Could not allocate memory for function arguments");
        ObjectState *os = bindObjectInState(*state, arg, false);
        for (j = 0; j < len + 1; j++)
          os->write8(j, s[j]);

        // Write pointer to newly allocated and initialised argv/envp c-string
        argvOS->write(i * NumPtrBytes, arg->getBaseExpr());
      }
    }
  }

  initializeGlobals(*state);

  processTree = new PTree(state);
  state->ptreeNode = processTree->root;
  run(*state);
  delete processTree;
  processTree = 0;

  // hack to clear memory objects
  delete memory;
  memory = new MemoryManager(NULL);

  globalObjects.clear();
  globalAddresses.clear();

  if (statsTracker)
    statsTracker->done();
}

/**
 * Run from the main function, and fork from the target function.
 */
void Executor::runFunctionFromSnapshotFork(llvm::Function *f, int argc,
                                           char **argv, char **envp,
                                           Snapshot *snapshot) {

  std::vector<ref<Expr>> arguments;
  fromSnapshot = true;
  executionType = ExecutionType::FromSnapshotFork;
  baseSnapshot = snapshot;
  int i;
  // for (i = 0; i < argc; i++) {
  //   std::cout << argv[i] << std::endl;
  // }
  // return;

  // force deterministic initialization of memory objects
  srand(1);
  srandom(1);

  MemoryObject *argvMO = 0;

  // In order to make uclibc happy and be closer to what the system is
  // doing we lay out the environments at the end of the argv array
  // (both are terminated by a null). There is also a final terminating
  // null that uclibc seems to expect, possibly the ELF header?

  int envc;
  for (envc = 0; envp[envc]; ++envc)
    ;

  unsigned NumPtrBytes = Context::get().getPointerWidth() / 8;
  KFunction *kf = kmodule->functionMap[f];
  assert(kf);
  Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
  if (ai != ae) {
    arguments.push_back(ConstantExpr::alloc(argc, Expr::Int32));
    if (++ai != ae) {
      Instruction *first = &*(f->begin()->begin());
      argvMO = memory->allocate((argc + 1 + envc + 1 + 1) * NumPtrBytes,
                                /*isLocal=*/false, /*isGlobal=*/true,
                                /*allocSite=*/first, /*alignment=*/8);

      if (!argvMO)
        klee_error("Could not allocate memory for function arguments");
      argvMO->setName(
          fmt::format("{}_arg_{}", f->getName().str(), ai->getName().str()));
      arguments.push_back(argvMO->getBaseExpr());

      if (++ai != ae) {
        uint64_t envp_start = argvMO->address + (argc + 1) * NumPtrBytes;
        arguments.push_back(Expr::createPointer(envp_start));

        if (++ai != ae)
          klee_error("invalid main function (expect 0-3 arguments)");
      }
    }
  }

  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);

  if (pathWriter)
    state->pathOS = pathWriter->open();
  if (symPathWriter)
    state->symPathOS = symPathWriter->open();

  if (statsTracker)
    statsTracker->framePushed(*state, 0);

  assert(arguments.size() == f->arg_size() && "wrong number of arguments");
  for (unsigned i = 0, e = f->arg_size(); i != e; ++i)
    bindArgument(kf, i, *state, arguments[i]);

  if (argvMO) {
    ObjectState *argvOS = bindObjectInState(*state, argvMO, false);

    for (int i = 0; i < argc + 1 + envc + 1 + 1; i++) {
      if (i == argc || i >= argc + 1 + envc) {
        // Write NULL pointer
        argvOS->write(i * NumPtrBytes, Expr::createPointer(0));
      } else {
        char *s = i < argc ? argv[i] : envp[i - (argc + 1)];
        int j, len = strlen(s);

        MemoryObject *arg =
            memory->allocate(len + 1, /*isLocal=*/false, /*isGlobal=*/true,
                             /*allocSite=*/state->pc->inst, /*alignment=*/8);
        if (!arg)
          klee_error("Could not allocate memory for function arguments");
        ObjectState *os = bindObjectInState(*state, arg, false);
        for (j = 0; j < len + 1; j++)
          os->write8(j, s[j]);

        // Write pointer to newly allocated and initialised argv/envp c-string
        argvOS->write(i * NumPtrBytes, arg->getBaseExpr());
      }
    }
  }

  initializeGlobals(*state);

  processTree = new PTree(state);
  state->ptreeNode = processTree->root;
  run(*state);

  baseSnapshot->memGraph.free();

  delete processTree;
  processTree = 0;

  // hack to clear memory objects
  delete memory;
  memory = new MemoryManager(NULL);

  globalObjects.clear();
  globalAddresses.clear();

  if (statsTracker)
    statsTracker->done();
}

/**
 * Run the target function directly from the snapshot.
 */
void Executor::runFunctionFromSnapshot(llvm::Function *f, int argc, char **argv,
                                       char **envp, Snapshot *snapshot) {

  std::vector<ref<ConstantExpr>> arguments = snapshot->arguments;
  fromSnapshot = true;
  executionType = ExecutionType::FromSnapshot;
  baseSnapshot = snapshot;
  // force deterministic initialization of memory objects
  srand(1);
  srandom(1);

  // In order to make uclibc happy and be closer to what the system is
  // doing we lay out the environments at the end of the argv array
  // (both are terminated by a null). There is also a final terminating
  // null that uclibc seems to expect, possibly the ELF header?

  int envc;
  for (envc = 0; envp[envc]; ++envc)
    ;

  // unsigned NumPtrBytes = Context::get().getPointerWidth() / 8;
  KFunction *kf = kmodule->functionMap[f];
  assert(kf);
  ExecutionState *state = new ExecutionState(kf);
  // Load functions and external objects
  initializeFunctions(*state);
  state->recursiveDepth = 1;
  // Load function pointers and create map to actual
  for (auto &it : snapshot->fn_ptrs_map) {
    uint64_t snapshot_addr = it.first;
    std::string fn_name = it.second;
    llvm::Function *fn = kmodule->module->getFunction(fn_name);
    snapshottedFunctions.insert(std::make_pair(snapshot_addr, fn));
  }
  for (auto &it : snapshot->externalObjects) {
    std::string obj_name = it.first;
    uint64_t obj_addr = it.second;
    if (externalObjects.find(obj_name) == externalObjects.end()) {
      SPDLOG_INFO("External object {} not found", obj_name);
      continue;
    }
    uint64_t current_addr = externalObjects[obj_name];
    snapshottedExternalObjects.insert(std::make_pair(obj_addr, current_addr));
  }
  if (pathWriter)
    state->pathOS = pathWriter->open();
  if (symPathWriter)
    state->symPathOS = symPathWriter->open();

  if (statsTracker)
    statsTracker->framePushed(*state, 0);
  // Load all memory objects from snapshot
  std::map<std::string, const GlobalVariable *> globalVariableMap;
  Module *m = kmodule->module.get();
  int globalVariableCount = 0;
  // Find global objects to get type information
  for (Module::const_global_iterator i = m->global_begin(), e = m->global_end();
       i != e; ++i) {
    const GlobalVariable *v = &*i;
    globalVariableMap[v->getName().str()] = v;
    globalNames[v->getName().str()] = v;
    globalVariableCount++;
  }
  // Load all memory objects from snapshot except for external objects
  for (SnapshotObject &so : snapshot->objects) {
    uint64_t so_addr = so.addr;
    if (!memory->isValidAddress(so_addr)) {
      std::cout << "Skipping external object at " << std::hex << so_addr
                << " named " << so.name << "\n";
      continue;
    }
    if (so.isGlobal) {
      llvm::StringRef name(so.name);
      // if (name.startswith(".str."))
      //   continue;
      if (globalVariableMap.find(so.name) != globalVariableMap.end()) {
        const GlobalVariable *v = globalVariableMap[so.name];
        globalVariableMap.erase(so.name);
        if (so.name == "__exe_env") {
          baseSnapshot->initFiles(so);
        }
        MemoryObject *mo = so.toMemoryObject(memory, v);
        ObjectState *os = bindObjectInState(*state, mo, false);
        mo->type = v->getType()->getElementType();
        for (unsigned i = 0; i < so.size; i++) {
          os->write8(i, so.data[i]);
        }
        globalObjects.insert(std::make_pair(v, mo));
        globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));
        specialFunctionHandler->trackMemory(
            *state, v->getType(), mo->getBaseExpr(), mo->getSizeExpr());
        continue;
      } else {
        SPDLOG_DEBUG("Warning: global variable {} not found in module",
                     so.name);
      }
    }
    MemoryObject *mo = so.toMemoryObject(memory, nullptr);
    ObjectState *os = bindObjectInState(*state, mo, false);
    for (unsigned i = 0; i < so.size; i++)
      os->write8(i, so.data[i]);
  }

  for (Module::alias_iterator i = m->alias_begin(), ie = m->alias_end();
       i != ie; ++i) {
    globalAddresses.insert(std::make_pair(&*i, evalConstant(i->getAliasee())));
  }

  processTree = new PTree(state);
  state->ptreeNode = processTree->root;

  // Load arguments from snapshot
  initUniKleeAtEntry(*state, kf, baseSnapshot);
  std::vector<ref<Expr>> argumentsExpr;
  state->targetStack.push_back(state->stack.back());
  state->targetStack.back().snapshot = baseSnapshot;
  makeSymbolicFromSnapshot(*state, baseSnapshot, f, argumentsExpr);

  // Handle global variables that are not in the snapshot
  for (auto it : globalVariableMap) {
    SPDLOG_DEBUG("Handle remaining global variable {}", it.first);
    const GlobalVariable *v = it.second;
    size_t globalObjectAlignment = getAllocationAlignment(v);
    if (v->isDeclaration()) {
      // FIXME: We have no general way of handling unknown external
      // symbols. If we really cared about making external stuff work
      // better we could support user definition, or use the EXE style
      // hack where we check the object file information.

      Type *ty = v->getType()->getElementType();
      uint64_t size = 0;
      if (ty->isSized()) {
        size = kmodule->targetData->getTypeStoreSize(ty);
      } else {
        klee_warning("Type for %.*s is not sized", (int)v->getName().size(),
                     v->getName().data());
      }

      // XXX - DWD - hardcode some things until we decide how to fix.
#ifndef WINDOWS
      if (v->getName() == "_ZTVN10__cxxabiv117__class_type_infoE") {
        size = 0x2C;
      } else if (v->getName() == "_ZTVN10__cxxabiv120__si_class_type_infoE") {
        size = 0x2C;
      } else if (v->getName() == "_ZTVN10__cxxabiv121__vmi_class_type_infoE") {
        size = 0x2C;
      }
#endif

      if (size == 0) {
        klee_warning("Unable to find size for global variable: %.*s (use will "
                     "result in out of bounds access)",
                     (int)v->getName().size(), v->getName().data());
      }

      MemoryObject *mo = memory->allocate(size, /*isLocal=*/false,
                                          /*isGlobal=*/true, /*allocSite=*/v,
                                          /*alignment=*/globalObjectAlignment);
      ObjectState *os = bindObjectInState(*state, mo, false);
      mo->setName(v->getName().str()); // global variable name is unique
      mo->type = ty;
      globalObjects.insert(std::make_pair(v, mo));
      globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));

      // Program already running = object already initialized.  Read
      // concrete value and write it to our copy.
      if (size) {
        void *addr;
        if (v->getName() == "__dso_handle") {
          addr = &__dso_handle; // wtf ?
        } else {
          addr = externalDispatcher->resolveSymbol(v->getName().str());
        }
        if (!addr)
          klee_error("unable to load symbol(%s) while initializing globals.",
                     v->getName().data());

        for (unsigned offset = 0; offset < mo->size; offset++)
          os->write8(offset, ((unsigned char *)addr)[offset]);
      }
    } else {
      Type *ty = v->getType()->getElementType();
      uint64_t size = kmodule->targetData->getTypeStoreSize(ty);
      MemoryObject *mo = memory->allocate(size, /*isLocal=*/false,
                                          /*isGlobal=*/true, /*allocSite=*/v,
                                          /*alignment=*/globalObjectAlignment);
      if (!mo)
        llvm::report_fatal_error("out of memory");
      mo->setName(v->getName().str()); // global variable name is unique
      mo->type = ty;
      ObjectState *os = bindObjectInState(*state, mo, false);
      globalObjects.insert(std::make_pair(v, mo));
      globalAddresses.insert(std::make_pair(v, mo->getBaseExpr()));

      if (!v->hasInitializer())
        os->initializeToRandom();
    }
    if (v->hasInitializer()) {
      MemoryObject *mo = globalObjects[v];
      const ObjectState *os = state->addressSpace.findObject(mo);
      assert(os);
      ObjectState *wos = state->addressSpace.getWriteable(mo, os);
      initializeGlobalObject(*state, wos, v->getInitializer(), 0);
    }
  }

  // Run the target function
  run(*state);
  delete processTree;
  processTree = 0;

  // hack to clear memory objects
  baseSnapshot->memGraph.free();
  delete memory;
  memory = new MemoryManager(NULL);

  globalObjects.clear();
  globalAddresses.clear();

  if (statsTracker)
    statsTracker->done();
}

/**
 * Run the target function directly without snapshot
 */
void Executor::runFunctionUnderConstrained(llvm::Function *f) {

  executionType = ExecutionType::UnderConstrained;
  // force deterministic initialization of memory objects
  srand(1);
  srandom(1);

  // In order to make uclibc happy and be closer to what the system is
  // doing we lay out the environments at the end of the argv array
  // (both are terminated by a null). There is also a final terminating
  // null that uclibc seems to expect, possibly the ELF header?

  // unsigned NumPtrBytes = Context::get().getPointerWidth() / 8;
  KFunction *kf = kmodule->functionMap[f];
  assert(kf);
  ExecutionState *state = new ExecutionState(kf);
  state->recursiveDepth = 1;
  // Load functions and external objects
  initUniKleeAtEntry(*state, kf, baseSnapshot);
  initializeUnderConstrained(*state);

  if (pathWriter)
    state->pathOS = pathWriter->open();
  if (symPathWriter)
    state->symPathOS = symPathWriter->open();

  if (statsTracker)
    statsTracker->framePushed(*state, 0);
  // Load all memory objects from snapshot
  std::map<std::string, const GlobalVariable *> globalVariableMap;
  Module *m = kmodule->module.get();
  int globalVariableCount = 0;
  // Find global objects to get type information
  for (Module::const_global_iterator i = m->global_begin(), e = m->global_end();
       i != e; ++i) {
    const GlobalVariable *v = &*i;
    globalVariableMap[v->getName().str()] = v;
    globalNames[v->getName().str()] = v;
    globalVariableCount++;
  }

  for (Module::alias_iterator i = m->alias_begin(), ie = m->alias_end();
       i != ie; ++i) {
    globalAddresses.insert(std::make_pair(&*i, evalConstant(i->getAliasee())));
  }

  processTree = new PTree(state);
  state->ptreeNode = processTree->root;

  // Load arguments from snapshot
  int index = 0;
  std::vector<ref<Expr>> arguments;
  SPDLOG_WARN("Init function argument {}", f->getName().str());
  for (llvm::Function::const_arg_iterator ai = f->arg_begin(),
                                          ae = f->arg_end();
       ai != ae; ++ai, ++index) {
    llvm::Type *argType = ai->getType();
    std::string argName = ai->getName().str();
    if (argName.empty()) {
      argName = fmt::format("arg_{}", index);
    }
    uint64_t typeSize = kmodule->targetData->getTypeStoreSize(argType);
    ref<Expr> argExpr =
        getSymbolicValue(*state, typeSize, argName, 0, 0).second;
    state->symbolics.back().setArg(index);
    arguments.push_back(argExpr);
    bindArgument(kf, index, *state, argExpr);
  }
  std::vector<ref<Expr>> argumentsExpr;
  state->targetStack.push_back(state->stack.back());
  state->targetStack.back().snapshot = baseSnapshot;

  // Run the target function
  run(*state);
  delete processTree;
  processTree = 0;

  // hack to clear memory objects
  delete memory;
  memory = new MemoryManager(NULL);

  globalObjects.clear();
  globalAddresses.clear();

  if (statsTracker)
    statsTracker->done();
}

bool Executor::isReadExpr(ref<Expr> e) const {
  Expr::Kind k = e->getKind();
  if (k == Expr::Kind::Read) {
    return true;
  } else if (k == Expr::Kind::Concat) {
    ref<ConcatExpr> ce = dyn_cast<ConcatExpr>(e);
    return (ce->getLeft()->getKind() == Expr::Kind::Read) &&
           (isReadExpr(ce->getRight()));
  } else {
    return false;
  }
}

bool Executor::isSymbolicPtr(const ExecutionState &state, ref<Expr> e) const {
  if (isa<ConstantExpr>(e)) {
    return false;
  }
  Expr::Width width = Context::get().getPointerWidth();
  if (e->getWidth() == width) {
    e = state.constraints.simplifyExpr(e);
    ref<ConstantExpr> max =
        Expr::createPointer(width == Expr::Int32 ? UINT32_MAX : UINT64_MAX);
    ref<Expr> eqMax = EqExpr::create(e, max);
    bool result = false;
    solver->mayBeTrue(state, eqMax, result);
    return result;
  }
  return false;
}

bool Executor::resolveMO(ExecutionState &state, ref<Expr> address,
                         ObjectPair &op) {
  address = state.constraints.simplifyExpr(address);
  if (isa<ConstantExpr>(address)) {
    return state.addressSpace.resolveOne(cast<ConstantExpr>(address), op);
  }
  bool result = false;
  if (!state.addressSpace.resolveOne(state, solver, address, op, result)) {
    ref<ConstantExpr> caddr;
    if (solver->getValue(state, address, caddr)) {
      return resolveMO(state, caddr, op);
    }
  }
  return result;
}

unsigned Executor::getPathStreamID(const ExecutionState &state) {
  assert(pathWriter);
  return state.pathOS.getID();
}

unsigned Executor::getSymbolicPathStreamID(const ExecutionState &state) {
  assert(symPathWriter);
  return state.symPathOS.getID();
}

void Executor::getConstraintLog(const ExecutionState &state, std::string &res,
                                Interpreter::LogType logFormat) {

  switch (logFormat) {
  case STP: {
    Query query(state.constraints, ConstantExpr::alloc(0, Expr::Bool));
    char *log = solver->getConstraintLog(query);
    res = std::string(log);
    free(log);
  } break;

  case KQUERY: {
    std::string Str;
    llvm::raw_string_ostream info(Str);
    ExprPPrinter::printConstraints(info, state.constraints);
    res = info.str();
  } break;

  case SMTLIB2: {
    std::string Str;
    llvm::raw_string_ostream info(Str);
    ExprSMTLIBPrinter printer;
    printer.setOutput(info);
    Query query(state.constraints, ConstantExpr::alloc(0, Expr::Bool));
    printer.setQuery(query);
    printer.generateOutput();
    res = info.str();
  } break;

  default:
    klee_warning("Executor::getConstraintLog() : Log format not supported!");
  }
}

bool Executor::getSymbolicSolution(
    const ExecutionState &state,
    std::vector<std::pair<std::string, std::vector<unsigned char>>> &res) {
  solver->setTimeout(coreSolverTimeout);

  ExecutionState tmp(state);

  // Go through each byte in every test case and attempt to restrict
  // it to the constraints contained in cexPreferences.  (Note:
  // usually this means trying to make it an ASCII character (0-127)
  // and therefore human readable. It is also possible to customize
  // the preferred constraints.  See test/Features/PreferCex.c for
  // an example) While this process can be very expensive, it can
  // also make understanding individual test cases much easier.
  for (auto &pi : state.cexPreferences) {
    bool mustBeTrue;
    // Attempt to bound byte to constraints held in cexPreferences
    bool success = solver->mustBeTrue(tmp, Expr::createIsZero(pi), mustBeTrue);
    // If it isn't possible to constrain this particular byte in the desired
    // way (normally this would mean that the byte can't be constrained to
    // be between 0 and 127 without making the entire constraint list UNSAT)
    // then just continue on to the next byte.
    if (!success)
      break;
    // If the particular constraint operated on in this iteration through
    // the loop isn't implied then add it to the list of constraints.
    if (!mustBeTrue)
      tmp.addConstraint(pi);
  }

  // std::vector<std::vector<unsigned char>> values;
  // std::vector<const Array *> objects;
  // for (unsigned i = 0; i != state.symbolics.size(); ++i)
  //   objects.push_back(state.symbolics[i].getArray());
  // bool success = solver->getInitialValues(tmp, objects, values);
  // solver->setTimeout(time::Span());

  // if (!success) {
  //   klee_warning("unable to compute initial values (invalid constraints?)!");

  //   ExprPPrinter::printQuery(llvm::errs(), state.constraints,
  //                            ConstantExpr::alloc(0, Expr::Bool));
  //   return false;
  // }

  // for (unsigned i = 0; i != state.symbolics.size(); ++i) {
  //   res.push_back(
  //       std::make_pair(state.symbolics[i].getArray()->getName(), values[i]));
  // }

  return true;
}

void Executor::getCoveredLines(
    const ExecutionState &state,
    std::map<const std::string *, std::set<unsigned>> &res) {
  res = state.coveredLines;
}

void Executor::doImpliedValueConcretization(ExecutionState &state, ref<Expr> e,
                                            ref<ConstantExpr> value) {
  abort(); // FIXME: Broken until we sort out how to do the write back.

  if (DebugCheckForImpliedValues)
    ImpliedValue::checkForImpliedValues(solver->solver, e, value);

  ImpliedValueList results;
  ImpliedValue::getImpliedValues(e, value, results);
  for (ImpliedValueList::iterator it = results.begin(), ie = results.end();
       it != ie; ++it) {
    ReadExpr *re = it->first.get();

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
      // FIXME: This is the sole remaining usage of the Array object
      // variable. Kill me.
      const MemoryObject *mo = 0; // re->updates.root->object;
      const ObjectState *os = state.addressSpace.findObject(mo);

      if (!os) {
        // object has been free'd, no need to concretize (although as
        // in other cases we would like to concretize the outstanding
        // reads, but we have no facility for that yet)
      } else {
        assert(!os->readOnly &&
               "not possible? read only object with static read?");
        ObjectState *wos = state.addressSpace.getWriteable(mo, os);
        wos->write(CE, it->second);
      }
    }
  }
}

Expr::Width Executor::getWidthForLLVMType(llvm::Type *type) const {
  return kmodule->targetData->getTypeSizeInBits(type);
}

size_t Executor::getAllocationAlignment(const llvm::Value *allocSite) const {
  // FIXME: 8 was the previous default. We shouldn't hard code this
  // and should fetch the default from elsewhere.
  const size_t forcedAlignment = 8;
  size_t alignment = 0;
  llvm::Type *type = NULL;
  std::string allocationSiteName(allocSite->getName().str());
  if (const GlobalObject *GV = dyn_cast<GlobalObject>(allocSite)) {
    alignment = GV->getAlignment();
    if (const GlobalObject *globalVar = dyn_cast<GlobalObject>(GV)) {
      // All GlobalVariables's have pointer type
      llvm::PointerType *ptrType =
          dyn_cast<llvm::PointerType>(globalVar->getType());
      assert(ptrType && "globalVar's type is not a pointer");
      type = ptrType->getElementType();
    } else {
      type = GV->getType();
    }
  } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(allocSite)) {
    alignment = AI->getAlignment();
    type = AI->getAllocatedType();
  } else if (isa<InvokeInst>(allocSite) || isa<CallInst>(allocSite)) {
    // FIXME: Model the semantics of the call to use the right alignment
    llvm::Value *allocSiteNonConst = const_cast<llvm::Value *>(allocSite);
#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
    const CallBase &cs = cast<CallBase>(*allocSite);
#else
    const CallSite cs = (isa<InvokeInst>(allocSiteNonConst)
                             ? CallSite(cast<InvokeInst>(allocSiteNonConst))
                             : CallSite(cast<CallInst>(allocSiteNonConst)));
#endif
    llvm::Function *fn =
        klee::getDirectCallTarget(cs, /*moduleIsFullyLinked=*/true);
    if (fn)
      allocationSiteName = fn->getName().str();

    klee_warning_once(fn != NULL ? fn : allocSite,
                      "Alignment of memory from call \"%s\" is not "
                      "modelled. Using alignment of %zu.",
                      allocationSiteName.c_str(), forcedAlignment);
    alignment = forcedAlignment;
  } else {
    llvm_unreachable("Unhandled allocation site");
  }

  if (alignment == 0) {
    assert(type != NULL);
    // No specified alignment. Get the alignment for the type.
    if (type->isSized()) {
      alignment = kmodule->targetData->getPrefTypeAlignment(type);
    } else {
      klee_warning_once(allocSite,
                        "Cannot determine memory alignment for "
                        "\"%s\". Using alignment of %zu.",
                        allocationSiteName.c_str(), forcedAlignment);
      alignment = forcedAlignment;
    }
  }

  // Currently we require alignment be a power of 2
  if (!bits64::isPowerOfTwo(alignment)) {
    klee_warning_once(allocSite,
                      "Alignment of %zu requested for %s but this "
                      "not supported. Using alignment of %zu",
                      alignment, allocSite->getName().str().c_str(),
                      forcedAlignment);
    alignment = forcedAlignment;
  }
  assert(bits64::isPowerOfTwo(alignment) &&
         "Returned alignment must be a power of two");
  return alignment;
}

std::string Executor::getBBLocation(KFunction *kf, llvm::BasicBlock *bb) {
  if (bb == nullptr) {
    return "no-loc";
  }
  unsigned entry = kf->basicBlockEntry[bb];
  KInstruction *ki = kf->instructions[entry];
  return ki->getSourceLocation();
}

void Executor::prepareForEarlyExit() {
  if (statsTracker) {
    // Make sure stats get flushed out
    statsTracker->done();
  }
}

/// Returns the errno location in memory
int *Executor::getErrnoLocation(const ExecutionState &state) const {
#ifndef __APPLE__
  /* From /usr/include/errno.h: it [errno] is a per-thread variable. */
  return __errno_location();
#else
  return __error();
#endif
}

///

Interpreter *Interpreter::create(LLVMContext &ctx,
                                 const InterpreterOptions &opts,
                                 InterpreterHandler *ih) {
  return new Executor(ctx, opts, ih);
}
