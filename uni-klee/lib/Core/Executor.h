//===-- Executor.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to perform actual execution, hides implementation details from external
// interpreter.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTOR_H
#define KLEE_EXECUTOR_H

#include "klee/ExecutionState.h"
#include "klee/Internal/Module/Cell.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Module/SlicingInfo.h"
#include "klee/Internal/System/Time.h"
#include "klee/Interpreter.h"
#include "klee/util/ArrayCache.h"
#include "klee/util/ReadExprUtil.h"

#include "klee/Internal/Module/Snapshot.h"
#include "klee/Internal/Module/ControlFlowGraph.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include "../Expr/ArrayExprOptimizer.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <queue>

struct KTest;

namespace llvm {
class BasicBlock;
class BranchInst;
class CallInst;
class Constant;
class ConstantExpr;
class Function;
class GlobalValue;
class Instruction;
class LLVMContext;
class DataLayout;
class Twine;
class Value;
} // namespace llvm

namespace klee {
class Array;
struct Cell;
class ExecutionState;
class ExternalDispatcher;
class Expr;
class InstructionInfoTable;
struct KFunction;
struct KInstruction;
class KInstIterator;
class KModule;
class MemoryManager;
class MemoryObject;
class ObjectState;
class PTree;
class Searcher;
class SeedInfo;
class SpecialFunctionHandler;
struct StackFrame;
class StatsTracker;
class TimingSolver;
class TreeStreamWriter;
class MergeHandler;
class SlicingInfo;
class StateGroup;
template <class T> class ref;

extern llvm::cl::opt<bool> NoSnapshot;
extern llvm::cl::opt<std::string> TargetFunction;

/// \todo Add a context object to keep track of data only live
/// during an instruction step. Should contain addedStates,
/// removedStates, and haltExecution, among others.

class Executor : public Interpreter {
  friend class RandomPathSearcher;
  friend class OwningSearcher;
  friend class WeightedRandomSearcher;
  friend class SpecialFunctionHandler;
  friend class StatsTracker;
  friend class MergeHandler;
  friend class MergingSearcher;

public:
  class Timer {
  public:
    Timer();
    virtual ~Timer();

    /// The event callback.
    virtual void run() = 0;
  };

  // FIXME: Better name would be PausedStateGroup
  // Stores and manage paused states
  class StateGroup {
  private:
    Executor *executor;
    bool continuePopInput;
    int maxBaseForks;
    int maxBaseAfterForks;
    int maxCrashTestForks;
    time::Point inputGenerationTime;
    std::map<int, int> forkMap;
    std::map<int, int> forkCountMap;
    std::map<int, StateMetaData> stateMetaDataMap;
    std::map<int, std::vector<ExecutionState *> *> patchStates; // always paused
    std::queue<ExecutionState *> inputQueue;
    std::set<ExecutionState *> doNotDeleteStates;

  public:
    StateGroup(Executor *executor);
    ~StateGroup();
    void addAndPauseState(ExecutionState *state);
    void addPatchState(ExecutionState *state, int baseStateID);
    void pauseAll(ExecutionState *state);
    void continueAll();
    bool shouldPopInput();
    void pushInput(ExecutionState *state);
    void popInput();
    void insertCrashTest(ExecutionState *state);
    void removeCrashTest(ExecutionState *state);
    void handleCrashState(ExecutionState *state);
    std::vector<ExecutionState *> *getPatchStates(int stateID);
    std::size_t getPatchedStatesSize();
    std::size_t getInputQueueSize() { return inputQueue.size(); }
    int getMaxForkValue(StateType type);
    void addForkParent(ExecutionState *state);
    ExecutionState *getForkParent(ExecutionState *state);
    bool addFork(ExecutionState *state, ExecutionState *forkedState);
    bool checkMaxFork(ExecutionState *state, int add, int &remain);
    int getRemainFork(ExecutionState *state);
    void stopInputGeneration() { continuePopInput = true; }
    bool checkInputGenerationTime();
    bool checkDeleteState(ExecutionState *state);
  };

  /// Slicing Information
  SlicingInfo *slicingInfo;

  typedef std::pair<ExecutionState *, ExecutionState *> StatePair;

  enum TerminateReason {
    Abort,
    Assert,
    BadVectorAccess,
    Exec,
    External,
    Free,
    Model,
    Overflow,
    Ptr,
    ReadOnly,
    ReportError,
    User,
    Unhandled
  };

  enum class ExecutionType {
    Normal,
    MakeSnapshot,
    FromSnapshot,
    FromSnapshotFork,
    InsideTargetFunction,
    UnderConstrained,
  };

private:
  bool fromSnapshot;
  enum ExecutionType executionType;
  std::map<uint64_t, llvm::Function *> snapshottedFunctions;
  MemGraph memGraph;
  Snapshot *baseSnapshot;
  static const char *TerminateReasonNames[];
  std::map<std::string, uint64_t> externalObjects;
  std::map<uint64_t, uint64_t> snapshottedExternalObjects;
  llvm::SmallPtrSet<llvm::Function *, 16> functionBlackList;

  class TimerInfo;
  uint64_t queryCounter = 0;

  std::unique_ptr<KModule> kmodule;
  llvm::DebugInfoFinder finder;
  InterpreterHandler *interpreterHandler;
  Searcher *searcher;
  ControlFlowGraph *cfg;

  ExternalDispatcher *externalDispatcher;
  TimingSolver *solver;
  MemoryManager *memory;
  LazyInitManager lazyInitManager;
  std::set<ExecutionState *> states;
  StatsTracker *statsTracker;
  TreeStreamWriter *pathWriter, *symPathWriter;
  SpecialFunctionHandler *specialFunctionHandler;
  std::vector<TimerInfo *> timers;
  PTree *processTree;
  std::set<MemRecord> accessedByExternalCall;

  /// Keeps track of all currently ongoing merges.
  /// An ongoing merge is a set of states which branched from a single state
  /// which ran into a klee_open_merge(), and not all states in the set have
  /// reached the corresponding klee_close_merge() yet.
  std::vector<MergeHandler *> mergeGroups;

  /// ExecutionStates currently paused from scheduling because they are
  /// waiting to be merged in a klee_close_merge instruction
  std::set<ExecutionState *> inCloseMerge;

  /// Used to track states that have been added during the current
  /// instructions step.
  /// \invariant \ref addedStates is a subset of \ref states.
  /// \invariant \ref addedStates and \ref removedStates are disjoint.
  std::vector<ExecutionState *> addedStates;
  /// Used to track states that have been removed during the current
  /// instructions step.
  /// \invariant \ref removedStates is a subset of \ref states.
  /// \invariant \ref addedStates and \ref removedStates are disjoint.
  std::vector<ExecutionState *> removedStates;

  /// Used to track states that are not terminated, but should not
  /// be scheduled by the searcher.
  std::vector<ExecutionState *> pausedStates;
  /// States that were 'paused' from scheduling, that now may be
  /// scheduled again
  std::vector<ExecutionState *> continuedStates;

  // States that are forked at selectPatchID()
  // First paused, and will be continued at crash
  StateGroup stateGroup;

  /// When non-empty the Executor is running in "seed" mode. The
  /// states in this map will be executed in an arbitrary order
  /// (outside the normal search interface) until they terminate. When
  /// the states reach a symbolic branch then either direction that
  /// satisfies one or more seeds will be added to this map. What
  /// happens with other states (that don't satisfy the seeds) depends
  /// on as-yet-to-be-determined flags.
  std::map<ExecutionState *, std::vector<SeedInfo>> seedMap;

  /// Map of globals to their representative memory object.
  std::map<const llvm::GlobalValue *, MemoryObject *> globalObjects;

  /// Map of globals to their bound address. This also includes
  /// globals that have no representative object (i.e. functions).
  std::map<const llvm::GlobalValue *, ref<ConstantExpr>> globalAddresses;

  std::map<std::string, const llvm::GlobalVariable *> globalNames;

  /// The set of legal function addresses, used to validate function
  /// pointers. We use the actual Function* address as the function address.
  std::set<uint64_t> legalFunctions;

  /// When non-null the bindings that will be used for calls to
  /// klee_make_symbolic in order replay.
  const struct KTest *replayKTest;

  /// When non-null a list of branch decisions to be used for replay.
  const std::vector<bool> *replayPath;

  /// The index into the current \ref replayKTest or \ref replayPath
  /// object.
  unsigned replayPosition;

  /// When non-null a list of "seed" inputs which will be used to
  /// drive execution.
  const std::vector<struct KTest *> *usingSeeds;

  /// Disables forking, instead a random path is chosen. Enabled as
  /// needed to control memory usage. \see fork()
  bool atMemoryLimit;

  /// Disables forking, set by client. \see setInhibitForking()
  bool inhibitForking;

  /// Signals the executor to halt execution at the next instruction
  /// step.
  bool haltExecution;

  /// Whether implied-value concretization is enabled. Currently
  /// false, it is buggy (it needs to validate its writes).
  bool ivcEnabled;

  /// The maximum time to allow for a single core solver query.
  /// (e.g. for a single STP query)
  time::Span coreSolverTimeout;

  /// Maximum time to allow for a single instruction.
  time::Span maxInstructionTime;

  /// Assumes ownership of the created array objects
  ArrayCache arrayCache;

  /// File to print executed instructions to
  std::unique_ptr<llvm::raw_ostream> debugInstFile;

  // @brief Buffer used by logBuffer
  std::string debugBufferString;

  // @brief buffer to store logs before flushing to file
  llvm::raw_string_ostream debugLogBuffer;

  /// Optimizes expressions
  ExprOptimizer optimizer;

  // std::map<unsigned, MemoryObject *> pointerToMemoryObject;
  bool taintMarker;

  llvm::Function *getTargetFunction(llvm::Value *calledVal,
                                    ExecutionState &state);
  bool isKleeLibFunction(const std::string &sourceLoc);
  void executeInstruction(ExecutionState &state, KInstruction *ki);
  void executeInstructionWrapper(ExecutionState &state, KInstruction *ki);

  void printFileLine(ExecutionState &state, KInstruction *ki,
                     llvm::raw_ostream &file);

  void toGraphviz(std::string filename,
                  std::map<uint64_t, std::vector<uint64_t>> &graph);
  // Called at entry of the target function
  Snapshot *getSnapshot(ExecutionState &state, llvm::Function *f,
                        KInstruction *ki, std::vector<ref<Expr>> &arguments,
                        int snapshotNo, std::string snapshotName);
  // Called at exit of the target function
  // Record which memory objects are accessed by the target function
  Snapshot *applyDynamicAnalysisToSnapshot(ExecutionState &state,
                                           Snapshot *snapshot, StackFrame &sf,
                                           std::string msg, std::string suffix);

  int makeSymbolicFromSnapshot(ExecutionState &state, Snapshot *snapshot,
                               llvm::Function *f,
                               std::vector<ref<Expr>> &arguments);

  void run(ExecutionState &initialState);

  bool isAlive();

  // Given a concrete object in our [klee's] address space, add it to
  // objects checked code can reference.
  MemoryObject *addExternalObject(ExecutionState &state, void *addr,
                                  unsigned size, bool isReadOnly,
                                  std::string name);

  void initializeGlobalObject(ExecutionState &state, ObjectState *os,
                              const llvm::Constant *c, unsigned offset);
  void initializeGlobals(ExecutionState &state);
  void initializeFunctions(ExecutionState &state);
  void initializeUnderConstrained(ExecutionState &state);

  void stepInstruction(ExecutionState &state);
  void updateStates(ExecutionState *current);
  void transferToBasicBlock(llvm::BasicBlock *dst, llvm::BasicBlock *src,
                            ExecutionState &state);

  void callExternalFunction(ExecutionState &state, KInstruction *target,
                            llvm::Function *function,
                            std::vector<ref<Expr>> &arguments);

  ObjectState *bindObjectInState(ExecutionState &state, const MemoryObject *mo,
                                 bool isLocal, const Array *array = 0);

  /// Resolve a pointer to the memory objects it could point to the
  /// start of, forking execution when necessary and generating errors
  /// for pointers to invalid locations (either out of bounds or
  /// address inside the middle of objects).
  ///
  /// \param results[out] A list of ((MemoryObject,ObjectState),
  /// state) pairs for each object the given address can point to the
  /// beginning of.
  typedef std::vector<std::pair<
      std::pair<const MemoryObject *, const ObjectState *>, ExecutionState *>>
      ExactResolutionList;
  void resolveExact(ExecutionState &state, ref<Expr> p,
                    ExactResolutionList &results, const std::string &name);

  /// Allocate and bind a new object in a particular state. NOTE: This
  /// function may fork.
  ///
  /// \param isLocal Flag to indicate if the object should be
  /// automatically deallocated on function return (this also makes it
  /// illegal to free directly).
  ///
  /// \param target Value at which to bind the base address of the new
  /// object.
  ///
  /// \param reallocFrom If non-zero and the allocation succeeds,
  /// initialize the new object from the given one and unbind it when
  /// done (realloc semantics). The initialized bytes will be the
  /// minimum of the size of the old and new objects, with remaining
  /// bytes initialized as specified by zeroMemory.
  void executeAlloc(ExecutionState &state, ref<Expr> size, bool isLocal,
                    KInstruction *target, bool zeroMemory = false,
                    const ObjectState *reallocFrom = 0);

  /// Free the given address with checking for errors. If target is
  /// given it will be bound to 0 in the resulting states (this is a
  /// convenience for realloc). Note that this function can cause the
  /// state to fork and that \ref state cannot be safely accessed
  /// afterwards.
  void executeFree(ExecutionState &state, ref<Expr> address,
                   KInstruction *target = 0);

  void executeCall(ExecutionState &state, KInstruction *ki, llvm::Function *f,
                   std::vector<ref<Expr>> &arguments);

  bool analyzeSymbolicAddr(ExecutionState &state, ref<Expr> address,
                           SymbolicAddressVisitor &visitor,
                           std::set<uint64_t> &baseAddresses);
  void collectCandidateConcreteAddr(ExecutionState &state, ref<Expr> address,
                                    std::set<uint64_t> &baseAddresses,
                                    llvm::Type *type, uint64_t typeSize,
                                    std::vector<uint64_t> &candidates,
                                    uint32_t k);
  void selectCandidateConcreteAddr(ExecutionState &state, uint64_t base,
                                   uint64_t size,
                                   std::vector<uint64_t> *baseAddrs,
                                   std::vector<uint64_t> &candAddrs,
                                   std::vector<uint64_t> &candidates);
  void sortCandidates(uint64_t base, std::vector<uint64_t> &allCand);
  void handleSymbolicPointer(ExecutionState &state, bool isWrite,
                             ref<Expr> address,
                             ref<Expr> value /* undef if read */,
                             KInstruction *target);

  // do address resolution / object binding / out of bounds checking
  // and perform the operation
  void executeMemoryOperation(ExecutionState &state, bool isWrite,
                              ref<Expr> address,
                              ref<Expr> value /* undef if read */,
                              KInstruction *target /* undef if write */);

  void executeMakeSymbolic(ExecutionState &state, const MemoryObject *mo,
                           const std::string name, bool updateDepth = true);
  const Array *executeMakeFieldSymbolic(ExecutionState &state,
                                        const MemoryObject *mo, uint32_t offset,
                                        llvm::Type *type, uint64_t size,
                                        const std::string name, bool isSpecial);
  void executeMakePointerSymbolic(ExecutionState &state, const MemoryObject *mo,
                                  uint64_t offset, ref<Expr> addr,
                                  uint64_t destOffset,
                                  const MemoryObject *symbolized);
  std::pair<const Array *, ref<Expr>>
  getSymbolicPointer(ExecutionState &state, const MemoryObject *mo);
  std::pair<const Array *, ref<Expr>>
  getSymbolicValue(ExecutionState &state, uint64_t size, std::string name,
                   uint64_t writeToAddr, uint64_t base);

  /// Create a new state where each input condition has been added as
  /// a constraint and return the results. The input state is included
  /// as one of the results. Note that the output vector may included
  /// NULL pointers for states which were unable to be created.
  void branch(ExecutionState &state, const std::vector<ref<Expr>> &conditions,
              std::vector<ExecutionState *> &result);

  // Fork current and return states in which condition holds / does
  // not hold, respectively. One of the states is necessarily the
  // current state, and one of the states may be null.
  StatePair fork(ExecutionState &current, ref<Expr> condition, bool isInternal,
                 bool countFork = true);

  /// Add the given (boolean) condition as a constraint on state. This
  /// function is a wrapper around the state's addConstraint function
  /// which also manages propagation of implied values,
  /// validity checks, and seed patching.
  void addConstraint(ExecutionState &state, ref<Expr> condition);

  // Called on [for now] concrete reads, replaces constant with a symbolic
  // Used for testing.
  ref<Expr> replaceReadWithSymbolic(ExecutionState &state, ref<Expr> e);
  void onPtrAccess(ExecutionState &state, uint64_t source,
                   const MemoryObject *srcMO, bool isWrite,
                   ref<Expr> value); // Pointer memory access
  void onMemAccess(ExecutionState &state, const MemoryObject *mo, uint64_t addr,
                   bool isWrite, ref<Expr> value); // Non-pointer memory access
  const Cell &eval(KInstruction *ki, unsigned index,
                   ExecutionState &state) const;

  Cell &getArgumentCell(ExecutionState &state, KFunction *kf, unsigned index) {
    return state.stack.back().locals[kf->getArgRegister(index)];
  }

  Cell &getDestCell(ExecutionState &state, KInstruction *target) {
    return state.stack.back().locals[target->dest];
  }

  void bindLocal(KInstruction *target, ExecutionState &state, ref<Expr> value);
  void bindLocal(KInstruction *target, ExecutionState &state, ref<Expr> value,
                 MemRecord *mr);
  void bindArgument(KFunction *kf, unsigned index, ExecutionState &state,
                    ref<Expr> value);
  template <typename SqType, typename TypeIt>
  void computeOffsetsSeqTy(KGEPInstruction *kgepi,
                           ref<ConstantExpr> &constantOffset, uint64_t index,
                           const TypeIt it);
  /// Evaluates an LLVM constant expression.  The optional argument ki
  /// is the instruction where this constant was encountered, or NULL
  /// if not applicable/unavailable.
  ref<klee::ConstantExpr> evalConstantExpr(const llvm::ConstantExpr *c,
                                           const KInstruction *ki = NULL);

  /// Evaluates an LLVM constant.  The optional argument ki is the
  /// instruction where this constant was encountered, or NULL if
  /// not applicable/unavailable.
  ref<klee::ConstantExpr> evalConstant(const llvm::Constant *c,
                                       const KInstruction *ki = NULL);

  /// Return a unique constant value for the given expression in the
  /// given state, if it has one (i.e. it provably only has a single
  /// value). Otherwise return the original expression.
  ref<Expr> toUnique(const ExecutionState &state, ref<Expr> &e);

  /// Return a constant value for the given expression, forcing it to
  /// be constant in the given state by adding a constraint if
  /// necessary. Note that this function breaks completeness and
  /// should generally be avoided.
  ///
  /// \param purpose An identify string to printed in case of concretization.
  ref<klee::ConstantExpr> toConstant(ExecutionState &state, ref<Expr> e,
                                     const char *purpose);

  /// Bind a constant value for e to the given target. NOTE: This
  /// function may fork state if the state has multiple seeds.
  void executeGetValue(ExecutionState &state, ref<Expr> e,
                       KInstruction *target);
  ref<Expr> concretizeReadExpr(const ExecutionState &state, ref<Expr> &expr);
  ref<Expr> concretizeExpr(const ExecutionState &state, ref<Expr> &expr);
  ref<Expr> cloneTree(ref<Expr> &tree);
  void traverseTree(ExecutionState &state, ref<Expr> &parent, ref<Expr> &expr);
  void iterateUpdateList(ref<Expr> &expr);
  const ReadExpr *hasOrderedReads(ref<Expr> e, int stride);
  bool isReadExprAtOffset(ref<Expr> e, const ReadExpr *base, ref<Expr> offset);

  /// Get textual information regarding a memory address.
  std::string getAddressInfo(ExecutionState &state, ref<Expr> address) const;

  // Determines the \param lastInstruction of the \param state which is not KLEE
  // internal and returns its InstructionInfo
  const InstructionInfo &
  getLastNonKleeInternalInstruction(const ExecutionState &state,
                                    llvm::Instruction **lastInstruction);

  bool shouldExitOn(enum TerminateReason termReason);

  // remove state from searcher only
  void pauseState(ExecutionState &state);
  // add state to searcher only
  void continueState(ExecutionState &state);
  // remove state from queue and delete
  void terminateState(ExecutionState &state, std::string msg,
                      std::string suffix);
  void terminateStateOnReturn(ExecutionState &state,
                              const llvm::Twine &message);
  // call exit handler and terminate state
  void terminateStateEarly(ExecutionState &state, const llvm::Twine &message);
  // call exit handler and terminate state
  void terminateStateOnExit(ExecutionState &state);
  void terminateStateOnExtractfixCrashLoc(ExecutionState &state);
  // call error handler and terminate state
  void terminateStateOnError(ExecutionState &state, const llvm::Twine &message,
                             enum TerminateReason termReason,
                             const char *suffix = NULL,
                             const llvm::Twine &longMessage = "");

  // call error handler and terminate state, for execution errors
  // (things that should not be possible, like illegal instruction or
  // unlowered instrinsic, or are unsupported, like inline assembly)
  void terminateStateOnExecError(ExecutionState &state,
                                 const llvm::Twine &message,
                                 const llvm::Twine &info = "") {
    terminateStateOnError(state, message, Exec, NULL, info);
  }

  /// bindModuleConstants - Initialize the module constant table.
  void bindModuleConstants();

  template <typename TypeIt>
  void computeOffsets(KGEPInstruction *kgepi, TypeIt ib, TypeIt ie);

  /// bindInstructionConstants - Initialize any necessary per instruction
  /// constant values.
  void bindInstructionConstants(KInstruction *KI);

  void handlePointsToObj(ExecutionState &state, KInstruction *target,
                         const std::vector<ref<Expr>> &arguments);

  void doImpliedValueConcretization(ExecutionState &state, ref<Expr> e,
                                    ref<ConstantExpr> value);

  /// Add a timer to be executed periodically.
  ///
  /// \param timer The timer object to run on firings.
  /// \param rate The approximate delay (in seconds) between firings.
  void addTimer(Timer *timer, time::Span rate);

  void initTimers();
  void processTimers(ExecutionState *current, time::Span maxInstTime);
  void checkMemoryUsage();
  void printDebugInstructions(ExecutionState &state);
  void doDumpStates();
  void setTaintMarker(bool taint);
  bool checkMemRecord(ExecutionState &state);
  void forkWithPatchDataManager(ExecutionState &state, KInstruction *kcaller);
  void forkForBase(ExecutionState &state, KInstruction *kcaller);

public:
  Executor(llvm::LLVMContext &ctx, const InterpreterOptions &opts,
           InterpreterHandler *ie);
  virtual ~Executor();

  const InterpreterHandler &getHandler() { return *interpreterHandler; }

  void setPathWriter(TreeStreamWriter *tsw) override { pathWriter = tsw; }

  void setSymbolicPathWriter(TreeStreamWriter *tsw) override {
    symPathWriter = tsw;
  }

  void setReplayKTest(const struct KTest *out) override {
    assert(!replayPath && "cannot replay both buffer and path");
    replayKTest = out;
    replayPosition = 0;
  }

  void setReplayPath(const std::vector<bool> *path) override {
    assert(!replayKTest && "cannot replay both buffer and path");
    replayPath = path;
    replayPosition = 0;
  }

  void initUniKleeAtEntry(ExecutionState &state, const KFunction *kf,
                          Snapshot *snapshot);

  llvm::Module *setModule(std::vector<std::unique_ptr<llvm::Module>> &modules,
                          const ModuleOptions &opts) override;

  void useSeeds(const std::vector<struct KTest *> *seeds) override {
    usingSeeds = seeds;
  }

  void runFunctionAsMain(llvm::Function *f, int argc, char **argv,
                         char **envp) override;

  void runFunctionFromSnapshot(llvm::Function *f, int argc, char **argv,
                               char **envp, Snapshot *snapshot) override;

  void runFunctionFromSnapshotFork(llvm::Function *f, int argc, char **argv,
                                   char **envp, Snapshot *snapshot) override;

  void runFunctionUnderConstrained(llvm::Function *f) override;
  /*** Runtime options ***/

  void setHaltExecution(bool value) override { haltExecution = value; }

  void setInhibitForking(bool value) override { inhibitForking = value; }

  void prepareForEarlyExit() override;

  /*** State accessor methods ***/

  unsigned getPathStreamID(const ExecutionState &state) override;

  unsigned getSymbolicPathStreamID(const ExecutionState &state) override;

  void
  getConstraintLog(const ExecutionState &state, std::string &res,
                   Interpreter::LogType logFormat = Interpreter::STP) override;

  bool getSymbolicSolution(
      const ExecutionState &state,
      std::vector<std::pair<std::string, std::vector<unsigned char>>> &res)
      override;

  void importSymbolicInput(ExecutionState &state, std::string fileName,
                           Snapshot *snapshot, llvm::Function *f,
                           std::vector<ref<Expr>> &arguments);

  void exportSymbolicInput(
      std::string fileName, const ExecutionState &state,
      std::vector<std::pair<std::string, std::vector<unsigned char>>> &res)
      override;

  void getCoveredLines(
      const ExecutionState &state,
      std::map<const std::string *, std::set<unsigned>> &res) override;

  Expr::Width getWidthForLLVMType(llvm::Type *type) const;
  size_t getAllocationAlignment(const llvm::Value *allocSite) const;

  /// Returns the errno location in memory of the state
  int *getErrnoLocation(const ExecutionState &state) const;

  ref<Expr> getExprFromData(ExecutionState &state, std::vector<uint8_t> &data,
                            uint64_t offset, uint64_t size);

  MemoryObject *lazyAllocation(ExecutionState &state, ref<Expr> addr,
                               const llvm::Type *type, KInstruction *target,
                               const Array *array, uint64_t size, int depth);
  ref<Expr> expandLazyAllocation(ExecutionState &state, ref<Expr> addr,
                                 const llvm::Type *type, ObjectState *os,
                                 KInstruction *target, const std::string name,
                                 bool allow_null, uint64_t requestedAddr,
                                 const MemoryObject *requestedMO, int depth);
  int selectPatchID(ExecutionState &state, KInstruction *target,
                    std::vector<ref<Expr>> &arguments, ref<Expr> targetExpr);
  bool isReadExpr(ref<Expr> e) const;
  bool isSymbolicPtr(const ExecutionState &state, ref<Expr> e) const;
  bool resolveMO(ExecutionState &state, ref<Expr> address, ObjectPair &op);
  ExecutionState *cloneState(ExecutionState *state);
  std::string getUniqueName(ExecutionState &state, std::string name,
                            std::string prefix, uint64_t address);
  std::string getBBLocation(KFunction *kf, llvm::BasicBlock *bb);
  bool isAllStatePaused();
  void dumpLazyMemoryGraph(ExecutionState &state, std::string filename);
};

} // namespace klee

#endif
