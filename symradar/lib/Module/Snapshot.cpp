//===-----------------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iomanip>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "llvm/Support/raw_ostream.h"

#include "klee/Internal/Module/Snapshot.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/System/Memory.h"
#include "klee/Internal/System/RuntimeFD.h"
#include "klee/Config/CompileTimeInfo.h"

#include "../Core/MemoryManager.h"
#include "../Core/Context.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"

using namespace klee;

namespace klee {

bool MemRecord::toJson(Json::Value &root) const {
  root["base"] = pointerToDataString(base);
  root["addr"] = pointerToDataString(addr);
  root["isPtr"] = isPtr;
  root["type"] = (int)type;
  root["size"] = size;
  return true;
}

MemRecord MemRecord::loadFromJson(Json::Value &root) {
  std::string base = root["base"].asString();
  std::string addr = root["addr"].asString();
  bool isPtr = root["isPtr"].asBool();
  int type = root["type"].asInt();
  uint64_t size = root["size"].asUInt64();
  return MemRecord(dataStringToPointer(addr), dataStringToPointer(base), isPtr,
                   size, (MemRecordType)type);
}

std::string MemRecord::toString() const {
  return fmt::format("MemRecord[base={}, addr={}, isPtr={}, size={}, type={}]",
                     base, addr, isPtr, size, (int)type);
}

SnapshotObject::SnapshotObject(std::string _addr, unsigned _size,
                               unsigned _count, std::string _data,
                               MemKind _kind, std::string _name,
                               std::string _type, bool _isLocal, bool _isGlobal,
                               bool _isFixed)
    : size(_size), count(_count), kind(_kind), name(_name), type(_type),
      isLocal(_isLocal), isGlobal(_isGlobal), isFixed(_isFixed) {
  std::stringstream ss(_addr);
  ss >> std::hex >> addr;
  addr = swap_endian(addr);
  fromDataString(data, _data);
}

SnapshotObject::SnapshotObject(const MemoryObject *mo, const ObjectState *os) {
  addr = mo->address;
  size = mo->size;
  count = 0;
  kind = mo->kind;
  name = mo->name;
  type = get_type_string(mo->type);
  os->readConcreteStore(data);
  isLocal = mo->isLocal;
  isGlobal = mo->isGlobal;
  isFixed = mo->isFixed;
}

/**
 * @brief copy constructor
 */
SnapshotObject::SnapshotObject(const SnapshotObject &other)
    : addr(other.addr), size(other.size), count(other.count), data(other.data),
      kind(other.kind), name(other.name), type(other.type),
      isLocal(other.isLocal), isGlobal(other.isGlobal), isFixed(other.isFixed) {
}

MemoryObject *
SnapshotObject::toMemoryObject(MemoryManager *mm,
                               const llvm::Value *allocSite) const {
  MemoryObject *mo =
      mm->inject(addr, size, allocSite, isLocal, isGlobal, isFixed);
  mo->setName(name);
  return mo;
}

/**
 * @brief load from json
 */
SnapshotObject::SnapshotObject(Json::Value &root) {
  std::string address = root["addr"].asString();
  size = root["size"].asInt();
  count = root["count"].asInt();
  std::string data_str = root["data"].asString();
  kind = (MemKind)root["kind"].asInt();
  name = root["name"].asString();
  type = root["type"].asString();
  isLocal = root["isLocal"].asBool();
  isGlobal = root["isGlobal"].asBool();
  isFixed = root["isFixed"].asBool();
  std::stringstream ss(address);
  ss >> std::hex >> addr;
  addr = swap_endian(addr);
  fromDataString(data, data_str);
}

bool SnapshotObject::toJson(Json::Value &root) const {
  root["addr"] = pointerToDataString(addr);
  root["size"] = size;
  root["count"] = count;
  root["data"] = toDataString(data);
  root["kind"] = (int)kind;
  root["name"] = name;
  root["type"] = type;
  root["isLocal"] = isLocal;
  root["isGlobal"] = isGlobal;
  root["isFixed"] = isFixed;
  return true;
}

Snapshot::Snapshot(std::string func, ExecutionState *state,
                   std::vector<ref<ConstantExpr>> &_arguments, std::string file,
                   std::vector<std::string> &args, int id,
                   std::set<uint64_t> &legalFunctions,
                   std::map<std::string, uint64_t> externalObjects,
                   const MemGraph &memGraph)
    : snapshotId(id), stateId(0), patchId(-1), func(func), file(file),
      state(state), memGraph(memGraph) {
  arguments.assign(_arguments.begin(), _arguments.end());
  externalObjects = externalObjects;
  for (auto &it : state->addressSpace.objects) {
    const MemoryObject *mo = it.first;
    if (true) { // FIXME: fix this using MemoryManager::isValidAddress
      const ObjectState *os = it.second;
      objects.push_back(SnapshotObject(mo, os));
    }
  }
  std::stringstream ss;
  for (uint64_t index = 0; index < args.size(); index++) {
    if (index > 0)
      ss << ' ';
    ss << '\'' << args[index] << '\'';
  }
  argV = ss.str();
  argC = args.size();
  for (auto &it : legalFunctions) {
    llvm::Function *f = (llvm::Function *)it;
    fn_ptrs_map.insert(std::make_pair(it, f->getName().str()));
  }
}

/**
 * @brief Load Snapshot from json file
 */
Snapshot::Snapshot(Json::Value &root) {
  if (root.isNull())
    return;
  state = nullptr;
  snapshotId = root["snapshotId"].asInt();
  func = root["targetFn"].asString();
  file = root["file"].asString();
  argC = root["argC"].asInt();
  argV = root["argV"].asString();
  // load arguments of target function
  Json::Value &args = root["arguments"];
  arguments.reserve(args.size());
  for (auto &it : args) {
    std::string arg = it.asString();
    std::vector<unsigned char> data;
    fromDataString(data, arg);
    uint64_t v = 0;
    switch (data.size()) {
    case 1:
      v = *((uint8_t *)data.data());
      arguments.push_back(ConstantExpr::create(v, Expr::Int8));
      break;
    case 2:
      v = *((uint16_t *)data.data());
      arguments.push_back(ConstantExpr::create(v, Expr::Int16));
      break;
    case 4:
      v = *((uint32_t *)data.data());
      arguments.push_back(ConstantExpr::create(v, Expr::Int32));
      break;
    case 8:
      v = *((uint64_t *)data.data());
      arguments.push_back(ConstantExpr::create(v, Expr::Int64));
      break;
    default:
      assert(false && "Invalid argument size");
      break;
    }
  }
  // load objects
  Json::Value &objs = root["objects"];
  objects.reserve(objs.size());
  for (auto &it : objs) {
    objects.push_back(SnapshotObject(it));
  }
  Json::Value &rt = root["regressionTrace"];
  regressionTrace.reserve(rt.size());
  for (auto &it : rt) {
    regressionTrace.push_back(it.asBool());
  }
  // load fn_ptrs_map
  Json::Value &fn_ptrs = root["fnPtrs"];
  for (auto itr = fn_ptrs.begin(), end = fn_ptrs.end(); itr != end; ++itr) {
    uint64_t addr = dataStringToPointer(itr.key().asString());
    std::string name = itr->asString();
    fn_ptrs_map.insert(std::make_pair(addr, name));
  }
  Json::Value &ext_objs = root["externalObjects"];
  for (auto itr = ext_objs.begin(), end = ext_objs.end(); itr != end; ++itr) {
    uint64_t addr = itr->asUInt64();
    std::string name = itr.key().asString();
    externalObjects.insert(std::make_pair(name, addr));
  }
  // load memGraph, memPtrGraph
  Json::Value &memoryGraph = root["memoryGraph"];
  memGraph.loadFromJSON(memoryGraph);

  // load fds
  Json::Value &fds = root["fds"];
  for (auto &it : fds) {
    exe_file_t exe_file;
    int exe_fd = it["exe_fd"].asInt();
    exe_file.fd = it["fd"].asInt();
    exe_file.flags = it["flags"].asUInt();
    exe_file.off = it["off"].asUInt64();
    exe_file.real_flags = it["real_flags"].asUInt();
    exe_file.mode = it["mode"].asUInt();
    std::string path = it["path"].asString();
    runtime_fds[exe_fd] = std::make_pair(exe_file, path);
  }
  // load blacklistedRecords
  Json::Value &accessedByExternalCalls = root["blacklistedRecords"];
  loadSet(accessedByExternalCalls, this->blacklistedRecords);
  Json::Value &readAccess = root["readAccess"];
  loadSet(readAccess, this->readAccess);
  Json::Value &writeAccess = root["writeAccess"];
  loadSet(writeAccess, this->writeAccess);
}

void Snapshot::initFiles(SnapshotObject &exe_env_so) {
  // Open files
  exe_sym_env_t *exe_env = (exe_sym_env_t *)exe_env_so.data.data();
  for (auto &fd : runtime_fds) {
    int exe_fd = fd.first;
    exe_file_t &exe_file = fd.second.first;
    std::string &path = fd.second.second;
    exe_env->fds[exe_fd].fd =
        open(path.c_str(), exe_file.real_flags, exe_file.mode);
  }
}

void Snapshot::dumpSet(Json::Value &root, std::set<MemRecord> &records) {
  for (const MemRecord &mr : records) {
    Json::Value &elem = root.append(Json::objectValue);
    mr.toJson(elem);
  }
}

void Snapshot::loadSet(Json::Value &root, std::set<MemRecord> &records) {
  for (auto &it : root) {
    records.insert(MemRecord::loadFromJson(it));
  }
}

MemRecord Snapshot::getMemRecord(std::map<uint64_t, MemRecord> &mrSet,
                                 uint64_t address) {
  MemRecord tmp = MemRecord(0, 0, false, 0, MemRecordType::Unknown);
  if (mrSet.count(address) == 0)
    return tmp;
  return mrSet[address];
}

void Snapshot::dump(const std::string &snapshotName) {
  std::ofstream fout(snapshotName);
  if (!fout) {
    klee_error("Cannot open file %s", snapshotName.c_str());
  }
  SPDLOG_INFO("Dumping snapshot to {}", snapshotName);
  Json::Value root = Json::objectValue;
  root["kleeRevision"] = KLEE_BUILD_REVISION;
  root["snapshotId"] = snapshotId;
  root["stateId"] = stateId;
  root["patchId"] = patchId;
  root["file"] = file;
  root["targetFn"] = func;
  root["argC"] = argC;
  root["argV"] = argV;
  root["suffix"] = suffix;
  root["msg"] = msg;
  Json::Value &args = root["arguments"] = Json::arrayValue;
  for (auto &arg : arguments) {
    klee::ref<klee::ConstantExpr> ce = cast<klee::ConstantExpr>(arg);
    if (ce.isNull()) {
      args.append("");
    } else {
      uint64_t value = ce->getZExtValue();
      unsigned width = ce->getWidth() / 8;
      if (width == 0)
        width = 1;
      auto *byte = (unsigned char *)(&value);
      std::vector<unsigned char> v;
      for (unsigned idx = 0; idx < width; ++idx) {
        v.push_back(*byte++);
      }
      args.append(toDataString(v));
    }
  }
  // dump objects
  Json::Value &objs = root["objects"] = Json::arrayValue;
  Json::Value &fds = root["fds"] = Json::arrayValue;
  char buffer[1024];
  for (auto &so : objects) {
    Json::Value &mem = objs.append(Json::objectValue);
    so.toJson(mem);
    if (so.name == "__exe_env") { // dump fds
      exe_sym_env_t *exe_env = (exe_sym_env_t *)so.data.data();
      for (int i = 3; i < MAX_FDS; i++) { // Skip stdin, stdout, stderr
        exe_file_t *exe_file = &exe_env->fds[i];
        if (exe_file->fd <= 0 || (exe_file->flags & eOpen) == 0) {
          continue;
        }
        Json::Value &fd = fds.append(Json::objectValue);
        fd["fd"] = exe_file->fd;
        fd["exe_fd"] = i;
        fd["flags"] = exe_file->flags;
        fd["real_flags"] = exe_file->real_flags;
        fd["mode"] = exe_file->mode;
        fd["offset"] = exe_file->off;
        ssize_t len =
            readlink(("/proc/self/fd/" + std::to_string(exe_file->fd)).c_str(),
                     buffer, sizeof(buffer));
        if (len < 0) {
          SPDLOG_INFO("Cannot read link for fd({} -> {})", i, exe_file->fd);
        } else {
          buffer[len] = '\0';
          fd["path"] = std::string(buffer, len);
          SPDLOG_INFO("Found runtime_fd({} -> {}) -> {}", i, exe_file->fd,
                      buffer);
        }
      }
    }
  }
  Json::Value &rtArr = root["regressionTrace"] = Json::arrayValue;
  for (bool rt : regressionTrace) {
    rtArr.append(rt);
  }
  Json::Value &fnPtrs = root["fnPtrs"] = Json::objectValue;
  for (auto &it : fn_ptrs_map) {
    fnPtrs[pointerToDataString(it.first)] = it.second;
  }
  Json::Value &extObjs = root["externalObjects"] = Json::objectValue;
  for (auto &it : externalObjects) {
    extObjs[it.first] = it.second;
  }
  Json::Value &graph = root["memoryGraph"] = Json::objectValue;
  memGraph.dumpToJSON(graph);

  Json::Value &accessedByExternalCalls = root["blacklistedRecords"] =
      Json::arrayValue;
  dumpSet(accessedByExternalCalls, blacklistedRecords);
  Json::Value &readAccess = root["readAccess"] = Json::arrayValue;
  dumpSet(readAccess, this->readAccess);
  Json::Value &writeAccess = root["writeAccess"] = Json::arrayValue;
  dumpSet(writeAccess, this->writeAccess);

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "  ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(root, &fout);
  fout << "\n";
  fout.close();
  SPDLOG_INFO("Dumped snapshot to {}", snapshotName);
}

void Snapshot::afterExit(
    const ExecutionState *state, std::string msg, std::string suffix,
    std::vector<bool> &regressionTrace, std::set<MemRecord> &blacklistedRecords,
    std::map<uint64_t, std::pair<MemRecord, PtrEdge *>> &readAccessMap,
    std::map<uint64_t, std::pair<MemRecord, PtrEdge *>> &writeAccessMap) {
  this->msg = msg;
  this->suffix = suffix;
  this->regressionTrace = regressionTrace;
  for (auto &it : readAccessMap) {
    this->readAccess.insert(it.second.first);
  }
  for (auto &it : writeAccessMap) {
    this->writeAccess.insert(it.second.first);
  }
  this->blacklistedRecords = blacklistedRecords;
  this->stateId = state->getID();
  this->patchId = state->metaData.getPatchID();
}

std::string Snapshot::getSnapshotName(const ExecutionState *state,
                                      std::string suffix) {
  return fmt::format("snapshot-{}-r{}-{}{}{}-p{}.{}.json", state->getID(),
                     snapshotId, state->metaData.getIsCrash() ? 'c' : 'e',
                     state->getCrashID(),
                     state->metaData.getActuallyCrashed() ? 'c' : 'e',
                     state->metaData.getPatchID(), suffix);
}
} // namespace klee