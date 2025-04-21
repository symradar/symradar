//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTIONSTATE_H
#define KLEE_EXECUTIONSTATE_H

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Internal/ADT/TreeStream.h"
#include "klee/Internal/System/Time.h"
#include "klee/MergeHandler.h"
#include "klee/Internal/Module/Snapshot.h"

// FIXME: We do not want to be exposing these? :(
#include "../../lib/Core/AddressSpace.h"
#include "../../lib/Core/FunctionStateInfo.h"
#include "klee/Internal/Module/KInstIterator.h"
#include "llvm/IR/DataLayout.h"
#include <map>
#include <set>
#include <vector>

namespace klee {
class Array;

class CallPathNode;

struct Cell;
struct KFunction;
struct KInstruction;

class MemoryObject;

class PTreeNode;

class Snapshot;

struct InstructionInfo;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<const MemoryObject *> allocas;
  std::map<const MemoryObject *, std::pair<ref<Expr>, ref<Expr>>> nonLocalsRead;
  std::map<const MemoryObject *, std::pair<ref<Expr>, ref<Expr>>>
      nonLocalsWritten;
  Cell *locals;
  Snapshot *snapshot;
  std::map<uint64_t, std::pair<MemRecord, PtrEdge *>> readAccessMap;
  std::map<uint64_t, std::pair<MemRecord, PtrEdge *>> writeAccessMap;
  std::vector<std::string> trace;
  std::vector<bool> regressionTrace;
  std::vector<ref<Expr>> lazyTrace;
  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is set up to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);

  StackFrame(const StackFrame &s);

  ~StackFrame();
};

struct TrackedSymbolicObject {
  uint64_t address;
  uint64_t offset;
  uint64_t size;
  ref<Expr> value;
  bool isLazy;
  bool isInTargetFunction;
  bool isField;
  std::string name;
  TrackedSymbolicObject(uint64_t address, uint64_t offset, uint64_t size,
                        ref<Expr> value, bool isLazy, bool isInTargetFunction,
                        bool isField, std::string name)
      : address(address), offset(offset), size(size), value(value),
        isLazy(isLazy), isInTargetFunction(isInTargetFunction),
        isField(isField), name(name) {}
  TrackedSymbolicObject(const TrackedSymbolicObject &tso)
      : address(tso.address), offset(tso.offset), size(tso.size),
        value(tso.value), isLazy(tso.isLazy),
        isInTargetFunction(tso.isInTargetFunction), isField(tso.isField),
        name(tso.name) {}
};

class PatchData {
  int patchId;
  int iteration;
  Solver::Validity patchType;
  ref<Expr> patchCondition;

public:
  PatchData(int patchId, int iteration, ref<Expr> patchCondition,
            Solver::Validity patchType = Solver::Validity::Unknown)
      : patchId(patchId), iteration(iteration), patchType(patchType),
        patchCondition(patchCondition) {}
  PatchData(const PatchData &pd)
      : patchId(pd.patchId), iteration(pd.iteration), patchType(pd.patchType),
        patchCondition(pd.patchCondition) {}
  void setPatchType(Solver::Validity p) { this->patchType = p; }
  Solver::Validity getPatchType() const { return patchType; }
  ref<Expr> getPatchCondition() const { return patchCondition; }
  int getPatchId() const { return patchId; }
};

class PatchDataManager {
  uint64_t count;
  std::vector<bool> trace;
  PatchDataManager *prev;
  std::vector<std::map<int, PatchData *> *> patchData;
  std::vector<PatchData *> buggyTrace;
  std::set<int> patchSet;
  bool last;

public:
  PatchDataManager() : count(0), prev(0), last(false) {}
  PatchDataManager(const PatchDataManager &pdm)
      : count(pdm.count), prev(pdm.prev), patchSet(pdm.patchSet),
        last(pdm.last), trace(pdm.trace) {
    for (auto &patchDataMap : pdm.patchData) {
      std::map<int, PatchData *> *newPatchDataMap =
          new std::map<int, PatchData *>();
      for (auto &patchDataIter : *patchDataMap) {
        newPatchDataMap->insert(std::make_pair(
            patchDataIter.first, new PatchData(*patchDataIter.second)));
      }
      patchData.push_back(newPatchDataMap);
    }
  }
  ~PatchDataManager() {
    for (std::map<int, PatchData *> *patchDataMap : patchData) {
      for (auto &patch : *patchDataMap) {
        delete patch.second;
      }
      delete patchDataMap;
    }
  }
  void setLast(bool patchResult) {
    last = patchResult;
    trace.push_back(last);
  }
  void setSymbolicInput(const PatchDataManager &pdm) {
    prev = new PatchDataManager(pdm);
  }
  void countPatchLocation() {
    count++;
    patchData.push_back(new std::map<int, PatchData *>());
  }
  void addPatchData(int patchId, ref<Expr> patchCondition) {
    std::map<int, PatchData *> &patchDataMap = getPatchData();
    PatchData *pd = new PatchData(patchId, count, patchCondition);
    patchDataMap[patchId] = pd;
    if (patchId == 0) {
      buggyTrace.push_back(pd);
    }
  }
  void updatePatchType(int patchId, Solver::Validity patchType) {
    std::map<int, PatchData *> &patchDataMap = getPatchData();
    patchDataMap[patchId]->setPatchType(patchType);
    if (last && patchType == Solver::False) {
      patchSet.erase(patchId);
    }
    if (!last && patchType == Solver::True) {
      patchSet.erase(patchId);
    }
  }
  std::map<int, PatchData *> &getPatchData() const {
    return *patchData[count - 1];
  }
  void getPatchSet(std::set<int> &possiblePatchSet) {
    for (int patchId : patchSet) {
      possiblePatchSet.insert(patchId);
    }
  }
  void setPatchSet() {
    for (auto patchId : patchData) {
      for (auto &patch : *patchId) {
        patchSet.insert(patch.first);
      }
    }
  }

  std::string evalPatchSet(TimingSolver *solver, time::Span timeout,
                           ExecutionState &state) {
    std::set<int> removedPatchSet;
    solver->setTimeout(timeout);
    std::stringstream ss;
    for (int patchId : patchSet) {
      ref<Expr> patchResult = ConstantExpr::create(1, Expr::Bool);
      for (int i = 0; i < count; i++) {
        std::map<int, PatchData *> &patchDataMap = *patchData[i];
        PatchData *pd = patchDataMap[patchId];
        ref<Expr> patchCondition = pd->getPatchCondition();
        bool isTrue = trace[i];
        ref<Expr> eval = Expr::createIsZero(patchCondition);
        if (isTrue)
          eval = Expr::createIsZero(eval);
        patchResult = AndExpr::create(patchResult, eval);
      }
      // Check if the patch conditions evaluated to traced value
      Solver::Validity res;
      bool success = solver->evaluate(state, patchResult, res);
      if (!success) {
        continue;
      }
      if (res == Solver::False) {
        removedPatchSet.insert(patchId);
        ss << patchId << ":fail, ";
      } else if (res == Solver::True) {
        // Good
        ss << patchId << ":pass, ";
      } else {
        // Unknown
        ss << patchId << ":unknown, ";
      }
    }
    for (int patchId : removedPatchSet) {
      patchSet.erase(patchId);
    }
    solver->setTimeout(time::Span());
    return ss.str();
  }

  void printPatchSet(std::stringstream &ss) {
    for (int patchId : patchSet) {
      ss << patchId << ", ";
    }
  }
  ref<Expr> getPatchCondition(int patchId) const {
    PatchData *pd = getPatchData()[patchId];
    ref<Expr> patchCondition = pd->getPatchCondition();
    //    for (int i = 0; i < count; i++) {
    //      std::map<int, PatchData *> &patchDataMap = *patchData[i];
    //      pd = patchDataMap[patchId];
    //      if (pd->getPatchType() == Solver::Unknown) {
    //
    //        patchCondition = AndExpr::create(patchCondition,
    //        pd->getPatchCondition());
    //      }
    //    }
    return patchCondition;
  }
  bool empty() { return patchSet.empty(); }
  uint64_t getCount() const { return count; }
};

// basic: basic, base: base, base_after: base after patch selection
// patch: after patch selection, always paused
// crash_test: after base_after exits, forked from patch, merge with base_after
enum StateType { ST_basic, ST_base, ST_base_after, ST_patch, ST_crash_test };
struct StateMetaData {
  bool isCrash;         // For ST_base
  bool actuallyCrashed; // For ST_crash_test
  int id;
  int oldId;
  int crashId;
  int patchId;
  enum StateType stateType;
  bool isPaused;

  StateMetaData()
      : id(-1), oldId(-1), crashId(0), patchId(-1), isCrash(false),
        actuallyCrashed(false), stateType(StateType::ST_basic),
        isPaused(false) {}
  StateMetaData(const StateMetaData &md)
      : id(-1), oldId(-1), crashId(md.crashId), patchId(md.patchId),
        isCrash(md.isCrash), actuallyCrashed(md.actuallyCrashed),
        stateType(md.stateType), isPaused(md.isPaused) {}
  void handleCrashState(struct StateMetaData *md) {
    this->crashId = md->crashId;
    this->isCrash = md->isCrash;
    this->stateType = StateType::ST_crash_test;
    this->setIsPaused(false);
  }
  int setID(int id) {
    this->id = id;
    return id;
  }
  int resetID(int id) {
    this->oldId = this->id;
    this->id = id;
    return id;
  }
  int getID() const { return id; }
  int getOldID() const { return oldId; }
  int setCrashID(int crashID) {
    this->crashId = crashID;
    return crashId;
  }
  int getCrashID() const { return crashId; }
  void setPatchID(int patchId) { this->patchId = patchId; }
  int getPatchID() const { return patchId; }
  void setIsCrash(bool isCrash) {
    this->isCrash = isCrash;
    this->actuallyCrashed = isCrash;
  }
  bool getIsCrash() const { return isCrash; }
  void setActuallyCrashed(bool actuallyCrashed) {
    this->actuallyCrashed = actuallyCrashed;
  }
  bool getActuallyCrashed() const { return actuallyCrashed; }
  void setStateType(enum StateType stateType) { this->stateType = stateType; }
  enum StateType getStateType() const { return stateType; }
  bool isType(enum StateType stateType) const {
    return this->stateType == stateType;
  }
  bool isTypeBase() const { return isType(StateType::ST_base_after); }
  bool getIsPaused() const { return isPaused; }
  void setIsPaused(bool isPaused) { this->isPaused = isPaused; }
};

class SymbolicObjectRecord {
private:
  const MemoryObject *mo;
  const Array *array;
  std::string type; // Type: arg, heap, lazy, klee, patch
  std::string name;
  std::string loc;
  std::string value;
  uint64_t size;
  uint64_t offset;
  uint64_t address;
  uint32_t index;

public:
  SymbolicObjectRecord(const MemoryObject *mo, const Array *array);
  SymbolicObjectRecord(const Array *array, uint64_t size, uint64_t address,
                       uint64_t base);
  SymbolicObjectRecord(const SymbolicObjectRecord &sor)
      : mo(sor.mo), array(sor.array), type(sor.type), name(sor.name),
        loc(sor.loc), value(sor.value), size(sor.size), offset(sor.offset),
        address(sor.address), index(sor.index) {}
  const MemoryObject *getMemoryObject() const { return mo; }
  void updateRefCount() {
    if (mo)
      ((MemoryObject *)mo)->updateRefCount();
  }
  const Array *getArray() const { return array; }
  std::string getType() const { return type; }
  std::string getName() const { return name; }
  std::string getLoc() const { return loc; }
  std::string getValue() const { return value; }
  uint64_t getSize() const { return size; }
  uint64_t getOffset() const { return offset; }
  uint64_t getAddress() const { return address; }
  uint32_t getIndex() const { return index; }
  bool operator==(const SymbolicObjectRecord &other) const {
    return mo == other.mo && array == other.array;
  }
  void setHeap();
  void setArg(uint32_t index);
  void setLazy(std::string loc);
  void setSymPtr(uint64_t concAddr);
  void setSpecial(std::string type) { this->type = type; }
};

struct MemTrace {
  uint64_t address;
  uint64_t base;
  uint64_t size;
  uint64_t targetAddr;
  uint64_t targetBase;
  ref<Expr> targetExpr;

  MemTrace(uint64_t address, uint64_t base, uint64_t size, uint64_t targetAddr,
           uint64_t targetBase, ref<Expr> targetExpr)
      : address(address), base(base), size(size), targetAddr(targetAddr),
        targetBase(targetBase), targetExpr(targetExpr) {}
};

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

public:
  static int nextID;
  static int nextCrashID;
  StateMetaData metaData;
  bool passedTarget;
  bool passedCrashLoc;
  bool taintMarker;
  int recursiveDepth;
  std::vector<std::pair<const MemoryObject *, const Array *>> shadowMemory;
  ConstraintManager shadowConstraints;
  ConstraintManager shadowConstraintsForNewSymbolics;
  PatchDataManager patchDataManager;
  std::map<const MemoryObject *, uint64_t> lazyObjectSizeMap;

  // Execution - Control Flow specific

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Stack representing the current instruction stream
  stack_ty stack;

  /// @brief Stack representing the only target function
  stack_ty targetStack;

  /// @brief Remember from which Basic Block control flow arrived
  /// (i.e. to select the right phi values)
  unsigned incomingBBIndex;

  // Overall state of the state - Data specific

  /// @brief Address space used by this state (e.g. Global and Heap)
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  ConstraintManager constraints;

  std::map<const MemoryObject *, std::vector<std::pair<int, int>>>
      symbolicFields;

  std::map<uint64_t, std::vector<TrackedSymbolicObject>> trackedSymbolicObjects;

  /// Statistics and information

  /// @brief Costs for all queries issued for this state, in seconds
  mutable time::Span queryCost;
  mutable time::Span cost;

  /// @brief Weight assigned for importance of this state.  Can be
  /// used for searchers to decide what paths to explore
  double weight;

  /// @brief Exploration depth, i.e., number of times KLEE branched for this
  /// state
  unsigned depth;

  /// @brief History of complete path: represents branches taken to
  /// reach/create this state (both concrete and symbolic)
  TreeOStream pathOS;

  /// @brief History of symbolic path: represents symbolic branches
  /// taken to reach/create this state
  TreeOStream symPathOS;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  unsigned instsSinceCovNew;

  /// @brief Whether a new instruction was covered in this state
  bool coveredNew;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<unsigned>> coveredLines;

  /// @brief Pointer to the process tree of the current state
  PTreeNode *ptreeNode;

  std::string explain;

  /// @brief Ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<SymbolicObjectRecord> symbolics;

  /// A list of boolean expressions the user has requested be true of
  /// a counterexample. Mutable since we play a little fast and loose
  /// with allowing it to be added to during execution (although
  /// should sensibly be only at creation time).
  std::vector<ref<Expr>> cexPreferences;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  // A list of updates to the memory state
  std::vector<MemTrace> memTraces;

  int getID() const { return metaData.getID(); }
  int setID() { return metaData.setID(nextID++); }
  int resetID() { return metaData.resetID(nextID++); }
  int getCrashID() const { return metaData.getCrashID(); }
  int setCrashID() { return metaData.setCrashID(nextCrashID++); }
  int setCrashID(int id) { return metaData.setCrashID(id); }

  std::string getFnAlias(std::string fn);

  void addFnAlias(std::string old_fn, std::string new_fn);

  void removeFnAlias(std::string fn);

  // The objects handling the klee_open_merge calls this state ran through
  std::vector<ref<MergeHandler>> openMergeStack;

  // The numbers of times this state has run through Executor::stepInstruction
  std::uint64_t steppedInstructions;

  // Function state info
  ref<FunctionStateInfo> functionStateInfo;

  std::vector<std::tuple<ref<Expr>, std::string>> lazyMemory;

private:
  ExecutionState() : ptreeNode(0), functionStateInfo(new FunctionStateInfo()) {}

public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr>> &assumptions);

  ExecutionState(const ExecutionState &state);

  ~ExecutionState();

  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);

  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addSymbolic(const Array *array, uint64_t size, uint64_t address,
                   uint64_t base);

  void addConstraint(ref<Expr> e);

  void addCexPreference(ref<Expr> cond);

  void addIntBoundConstraint(ref<Expr> e);

  bool merge(const ExecutionState &b);

  void dumpStack(llvm::raw_ostream &out,
                 llvm::DataLayout *dataLayout = 0) const;
  void dumpStackTrace(std::vector<std::string> &out,
                      llvm::DataLayout *dl) const;

  void dumpFrame(llvm::raw_ostream &out, const StackFrame &sf,
                 const KInstruction *target, llvm::DataLayout *dataLayout,
                 bool onStack = false) const;

  void dumpHandleType(llvm::raw_ostream &out, const std::string &prefix,
                      const ObjectState *valueObjectState, llvm::Type *type,
                      llvm::DataLayout *dataLayout) const;

  void dumpHandleStructType(llvm::raw_ostream &out, const std::string &prefix,
                            const ObjectState *valueObjectState,
                            uint64_t initOffset, llvm::StructType *type,
                            llvm::DataLayout *dataLayout) const;
  void addStateInfoAsReturn(const KInstruction *target,
                            llvm::DataLayout *dataLayout);
  void setTaintMarker(bool taint);
};
} // namespace klee

#endif
