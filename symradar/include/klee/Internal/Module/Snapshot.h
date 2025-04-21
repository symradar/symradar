//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "klee/ExecutionState.h"
#include "klee/Expr.h"
#include "klee/Interpreter.h"
#include "klee/Internal/System/Memory.h"
#include "klee/Internal/System/RuntimeFD.h"
#include "klee/Internal/Module/KInstruction.h"

#include "external/json/json.h"

#ifndef KLEE_SNAPSHOT_H
#define KLEE_SNAPSHOT_H

namespace klee {
/**
 * Record the memory access from external calls
 * 1. If the value is read from memory, then the address is recorded
 * 2. If the value read from memory is a pointer, then the pointer
 *    value should be also recorded
 */
enum class MemRecordType {
  Unknown = 0,
  Read = 1,
  Write = 2,
  ExternalCall = 3,
  UniKleeMakeSymbolic = 4,
};

struct MemRecord {
  bool isPtr;
  MemRecordType type;
  uintptr_t addr;
  uintptr_t base;
  uint64_t size;

public:
  MemRecord()
      : isPtr(false), type(MemRecordType::Unknown), addr(0), base(0), size(0) {}
  MemRecord(uintptr_t addr, uintptr_t base, bool isPtr, uint64_t size,
            MemRecordType type)
      : isPtr(isPtr), type(type), addr(addr), base(base), size(size) {}
  MemRecord(const MemRecord &other)
      : isPtr(other.isPtr), type(other.type), addr(other.addr),
        base(other.base), size(other.size) {}
  bool toJson(Json::Value &root) const;
  bool operator<(const MemRecord &other) const {
    if (addr == other.addr)
      return size < other.size;
    else
      return addr < other.addr;
  }
  bool operator==(const MemRecord &other) const {
    return addr == other.addr && size == other.size;
  }
  std::string toString() const;
  static MemRecord loadFromJson(Json::Value &root);
};

class SnapshotObject {
public:
  uintptr_t addr;
  unsigned size;
  unsigned count;
  std::vector<uint8_t> data;
  MemKind kind;
  std::string name;
  std::string type;
  bool isLocal;
  bool isGlobal;
  bool isFixed;

public:
  SnapshotObject(std::string _addr, unsigned _size, unsigned _count,
                 std::string _data, MemKind _kind, std::string _name,
                 std::string _type, bool _isLocal, bool _isGlobal,
                 bool _isFixed);
  SnapshotObject(const MemoryObject *mo, const ObjectState *os);
  SnapshotObject(const SnapshotObject &other);
  MemoryObject *toMemoryObject(MemoryManager *mm,
                               const llvm::Value *allocSite) const;
  bool toJson(Json::Value &root) const;
  SnapshotObject(Json::Value &root); // load from json
};

class Snapshot {
public:
  // Serialized
  // Snapshot meta-data, should not be copied
  int snapshotId;
  int stateId;
  int patchId;
  // Cannot be serialized
  ExecutionState *state;
  // Common
  std::string func; // function name
  std::string file; // file name
  // Arguments of main function
  int argC;
  std::string argV;
  // Exit status
  std::string msg;
  std::string suffix;
  // Arguments of the target function
  std::vector<ref<ConstantExpr>> arguments;
  // Regression trace: trace of the result of __cpr_choice
  std::vector<bool> regressionTrace;
  // SnapshotObjects: serializable memory objects
  std::vector<SnapshotObject> objects;
  std::map<uintptr_t, SnapshotObject *> address_to_object;
  // Function pointers to names
  std::map<uint64_t, std::string> fn_ptrs_map;
  uint64_t max_object_size;
  std::set<MemRecord> blacklistedRecords;
  // Runtime file descriptors
  std::map<int, std::pair<exe_file_t, std::string>> runtime_fds;
  // External objects: Memory::allocatedFixed
  std::map<std::string, uint64_t> externalObjects;
  // MemGraph at the entry of the target function
  MemGraph memGraph;
  // MemRecords of the target function
  std::set<MemRecord> readAccess;
  std::set<MemRecord> writeAccess;
  // Not serialized
public:
  Snapshot() {}
  Snapshot(std::string func, ExecutionState *state,
           std::vector<ref<ConstantExpr>> &_arguments, std::string file,
           std::vector<std::string> &args, int id,
           std::set<uint64_t> &legalFunctions,
           std::map<std::string, uint64_t> externalObjects,
           const MemGraph &memGraph);
  Snapshot(const Snapshot &s)
      : func(s.func), state(s.state), arguments(s.arguments),
        regressionTrace(s.regressionTrace), fn_ptrs_map(s.fn_ptrs_map),
        memGraph(s.memGraph) {}
  Snapshot(Json::Value &root); // load from json
  // void dump(InterpreterHandler *ih);
  void dump(const std::string &filename);
  void initFiles(SnapshotObject &exe_env_so);
  void dumpSet(Json::Value &root, std::set<MemRecord> &records);
  void loadSet(Json::Value &root, std::set<MemRecord> &records);
  MemRecord getMemRecord(std::map<uint64_t, MemRecord> &mrSet,
                         uint64_t address);
  std::string getSnapshotName(const ExecutionState *state, std::string suffix);
  void afterExit(
      const ExecutionState *state, std::string msg, std::string suffix,
      std::vector<bool> &regressionTrace,
      std::set<MemRecord> &blacklistedRecords,
      std::map<uint64_t, std::pair<MemRecord, PtrEdge *>> &readAccessMap,
      std::map<uint64_t, std::pair<MemRecord, PtrEdge *>> &writeAccessMap);
};

} // namespace klee
#endif