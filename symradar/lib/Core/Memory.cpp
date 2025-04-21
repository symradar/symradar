//===-- Memory.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/System/Memory.h"

#include "Context.h"
#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/BitArray.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/util/ArrayCache.h"
#include "klee/Internal/Module/Snapshot.h"

#include "ObjectHolder.h"
#include "MemoryManager.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "spdlog/spdlog.h"

#include <cassert>
#include <sstream>

using namespace llvm;

namespace {
cl::opt<bool> UseConstantArrays("use-constant-arrays", cl::init(true));
}

/***/

namespace klee {
void fromDataString(std::vector<unsigned char> &data, const std::string &str) {

  assert(str.size() % 2 == 0);
  data.clear();
  data.reserve(str.size() / 2);

  unsigned char val = 0;
  unsigned counter = 0;
  for (const auto &ch : str) {
    unsigned char nibble = 0;
    if (isdigit(ch))
      nibble = ch - '0';
    else if (ch >= 'A' && ch <= 'F')
      nibble = ch - 'A' + 10;
    if (counter++ % 2 == 0) {
      val = nibble;
    } else {
      val = (val << 4) | nibble;
      data.push_back(val);
    }
  }
}

std::string toDataString(const std::vector<unsigned char> &data) {

  unsigned counter = 0;
  std::ostringstream bytes;
  for (auto itrData = data.begin(), endData = data.end(); (itrData != endData);
       ++itrData, ++counter) {

    unsigned char hi = (unsigned char)(*itrData >> 4);
    unsigned char low = (unsigned char)(*itrData & 0x0F);
    hi = (unsigned char)((hi > 9) ? ('A' + (hi - 10)) : ('0' + hi));
    low = (unsigned char)((low > 9) ? ('A' + (low - 10)) : ('0' + low));
    bytes << hi << low;
  }
  return bytes.str();
}

std::string pointerToDataString(uint64_t ptr) {
  std::vector<unsigned char> addr_arr;
  unsigned ptr_width = Context::get().getPointerWidth() / 8;
  unsigned char *addrBytes = ((unsigned char *)&(ptr));
  for (unsigned index = 0; index < ptr_width; ++index, ++addrBytes) {
    addr_arr.push_back(*addrBytes);
  }
  return toDataString(addr_arr);
}

std::string pointerToBigEndianString(uint64_t ptr) {
  std::stringstream ss;
  ss << std::hex << ptr;
  return ss.str();
}

uint64_t dataStringToPointer(const std::string &str) {
  uintptr_t addr;
  std::stringstream ss(str);
  ss >> std::hex >> addr;
  return swap_endian(addr);
}

std::string get_type_string(const llvm::Type *type) {
  if (const_cast<llvm::Type *>(type) == nullptr)
    return "nil";
  std::string str;
  llvm::raw_string_ostream rss(str);
  type->print(rss);
  return rss.str();
}

uintptr_t swap_endian(uintptr_t u) {
  static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
  union {
    uintptr_t u;
    unsigned char u8[sizeof(uintptr_t)];
  } source, dest;
  source.u = u;
  for (size_t k = 0; k < sizeof(uintptr_t); k++)
    dest.u8[k] = source.u8[sizeof(uintptr_t) - k - 1];
  return dest.u;
};

void MemNode::dumpToJSON(Json::Value &root) {
  root["address"] = pointerToDataString(address);
  root["size"] = size;
  Json::Value &ptrs = root["pointers"] = Json::arrayValue;
  for (auto ptr : pointers) {
    ptrs.append(pointerToDataString(ptr));
  }
}

void PtrNode::dumpToJSON(Json::Value &root) {
  root["address"] = pointerToDataString(address);
  root["baseAddr"] = pointerToDataString(baseAddr);
  root["offset"] = offset;
}

void PtrEdge::dumpToJSON(Json::Value &root) {
  root["from"] = pointerToDataString(from->getAddress());
  root["to"] = pointerToDataString(to->getAddress());
}

void MemEdge::dumpToJSON(Json::Value &root) {
  root["from"] = pointerToDataString(from->getAddress());
  root["to"] = pointerToDataString(to->getAddress());
  Json::Value &pe = root["ptrEdge"] = Json::objectValue;
  ptrEdge->dumpToJSON(pe);
}

void MemGraph::dumpToJSON(Json::Value &root) {
  SPDLOG_INFO("Dumping MemGraph to JSON: {} memNodes, {} ptrNodes, {} memEdges",
              memNodes.size(), ptrNodes.size(), memEdges.size());
  // Dump memNodes
  Json::Value &memoryNodes = root["memNodes"] = Json::arrayValue;
  for (auto &mn : memNodes) {
    Json::Value &memoryNode = memoryNodes.append(Json::objectValue);
    mn.second->dumpToJSON(memoryNode);
  }
  // Dump ptrNodes
  Json::Value &pointerNodes = root["ptrNodes"] = Json::arrayValue;
  for (auto &pn : ptrNodes) {
    Json::Value &pointerNode = pointerNodes.append(Json::objectValue);
    pn.second->dumpToJSON(pointerNode);
  }
  // Dump memEdges + ptrEdges
  Json::Value &memoryEdges = root["memEdges"] = Json::arrayValue;
  for (auto me : memEdges) {
    Json::Value &memoryEdge = memoryEdges.append(Json::objectValue);
    me->dumpToJSON(memoryEdge);
  }
}

void MemGraph::loadFromJSON(Json::Value &root) {
  // Load memNodes
  for (auto &mn : root["memNodes"]) {
    uint64_t address = dataStringToPointer(mn["address"].asString());
    uint64_t size = mn["size"].asUInt64();
    addMemNode(address, size);
  }
  // Load ptrNodes
  for (auto &pn : root["ptrNodes"]) {
    uint64_t address = dataStringToPointer(pn["address"].asString());
    uint64_t baseAddr = dataStringToPointer(pn["baseAddr"].asString());
    uint64_t offset = pn["offset"].asUInt64();
    MemNode *baseNode = getMemNode(baseAddr);
    addPtrNode(address, baseAddr, offset, baseNode);
  }
  // Load memEdges
  for (auto &me : root["memEdges"]) {
    uint64_t from = dataStringToPointer(me["from"].asString());
    uint64_t to = dataStringToPointer(me["to"].asString());
    Json::Value &pe = me["ptrEdge"];
    uint64_t fromPtr = dataStringToPointer(pe["from"].asString());
    uint64_t toPtr = dataStringToPointer(pe["to"].asString());
    PtrNode *fromPtrNode = getPtrNode(fromPtr);
    PtrNode *toPtrNode = getPtrNode(toPtr);
    PtrEdge *pointerEdge = addPtrEdge(fromPtrNode, toPtrNode);
    MemNode *fromNode = getMemNode(from);
    MemNode *toNode = getMemNode(to);
    addMemEdge(fromNode, toNode, pointerEdge);
  }
}

void MemGraph::free() {
  for (auto it = memNodes.begin(); it != memNodes.end(); ++it) {
    delete it->second;
  }
  for (auto it = ptrNodes.begin(); it != ptrNodes.end(); ++it) {
    delete it->second;
  }
  for (auto it = memEdges.begin(); it != memEdges.end(); ++it) {
    delete *it;
  }
  for (auto it = ptrEdges.begin(); it != ptrEdges.end(); ++it) {
    delete *it;
  }
  memNodes.clear();
  ptrNodes.clear();
  memEdges.clear();
  ptrEdges.clear();
  outgoingMemEdges.clear();
  incomingMemEdges.clear();
  outgoingPtrEdges.clear();
  incomingPtrEdges.clear();
}

MemGraph *MemGraph::prune() {
  // Remove invalid PtrEdge & MemEdge
  // Return new pruned MemGraph (should be freed after use)
  MemGraph *pruned = new MemGraph(memNodes, ptrNodes);
  for (auto &it : memNodes) {
    std::set<PtrEdge *> ptrEdges;
    auto &ptrs = it.second->getPointers();
    for (auto ptr : ptrs) {
      auto &outgoingEdges = getOutgoingPtrEdges(ptr);
      if (outgoingEdges.empty())
        continue;
      PtrEdge *validEdge = outgoingEdges.back();
      ptrEdges.insert(validEdge);
      pruned->addPtrEdge(validEdge);
      // for (auto edge : outgoingEdges) {
      //   if (edge != validEdge) {
      //     delete edge;
      //   }
      // }
    }
    auto &outEdges = getOutgoingMemEdges(it.first);
    for (auto edge : outEdges) {
      if (ptrEdges.count(edge->getPtrEdge()) > 0) {
        pruned->addMemEdge(edge);
      }
      // else {
      //   delete edge;
      // }
    }
  }
  return pruned;
}

MemNode *MemGraph::addMemNode(uint64_t address, uint64_t size,
                              const MemoryObject *mo) {
  if (memNodes.find(address) != memNodes.end()) {
    MemNode *mn = memNodes[address];
    if (mn->getMemoryObject() == nullptr && mo != nullptr) {
      mn->setMemoryObject(mo);
    }
    return mn;
  }
  MemNode *node = new MemNode(address, size, mo);
  return addMemNode(node);
}

MemNode *MemGraph::addMemNode(MemNode *memNode) {
  memNodes[memNode->getAddress()] = memNode;
  return memNode;
}

MemNode *MemGraph::getMemNode(uint64_t address) {
  if (memNodes.find(address) == memNodes.end()) {
    return nullptr;
  }
  return memNodes[address];
}

PtrNode *MemGraph::addPtrNode(uint64_t address, uint64_t baseAddr,
                              uint64_t offset, MemNode *memNode) {
  if (ptrNodes.find(address) != ptrNodes.end()) {
    PtrNode *pn = ptrNodes[address];
    if (pn->getMemNode() == nullptr && memNode != nullptr) {
      pn->setMemNode(memNode);
    }
    return pn;
  }
  PtrNode *node = new PtrNode(address, baseAddr, offset, memNode);
  return addPtrNode(node);
}

PtrNode *MemGraph::addPtrNode(uint64_t address, MemNode *memNode) {
  if (memNode == nullptr) {
    return nullptr;
  }
  uint64_t baseAddr = memNode->getAddress();
  uint64_t offset = address - baseAddr;
  return addPtrNode(address, baseAddr, offset, memNode);
}

PtrNode *MemGraph::addPtrNode(PtrNode *ptrNode) {
  ptrNodes[ptrNode->getAddress()] = ptrNode;
  return ptrNode;
}

PtrNode *MemGraph::getPtrNode(uint64_t address) {
  if (ptrNodes.find(address) == ptrNodes.end()) {
    return nullptr;
  }
  return ptrNodes[address];
}

PtrNode *MemGraph::searchPtrNode(uint64_t address) {
  const auto it = ptrNodes.lower_bound(address);
  if (it == ptrNodes.end()) {
    return nullptr;
  }
  return it->second;
}

MemEdge *MemGraph::addMemEdge(MemNode *from, MemNode *to, PtrEdge *ptrEdge) {
  if (from == nullptr || to == nullptr || ptrEdge == nullptr) {
    return nullptr;
  }
  from->addPointer(ptrEdge->getFrom()->getAddress());
  MemEdge *edge = new MemEdge(from, to, ptrEdge);
  return addMemEdge(edge);
}

MemEdge *MemGraph::addMemEdge(MemEdge *edge) {
  memEdges.push_back(edge);
  outgoingMemEdges[edge->getFrom()->getAddress()].push_back(edge);
  incomingMemEdges[edge->getTo()->getAddress()].push_back(edge);
  return edge;
}

std::vector<MemEdge *> &MemGraph::getOutgoingMemEdges(uint64_t address) {
  if (outgoingMemEdges.find(address) == outgoingMemEdges.end()) {
    return emptyMemEdgeVector;
  }
  return outgoingMemEdges[address];
}

std::vector<MemEdge *> &MemGraph::getIncomingMemEdges(uint64_t address) {
  if (incomingMemEdges.find(address) == incomingMemEdges.end()) {
    return emptyMemEdgeVector;
  }
  return incomingMemEdges[address];
}

PtrEdge *MemGraph::addPtrEdge(PtrNode *from, PtrNode *to) {
  PtrEdge *edge = new PtrEdge(from, to);
  return addPtrEdge(edge);
}

PtrEdge *MemGraph::addPtrEdge(PtrEdge *edge) {
  ptrEdges.push_back(edge);
  outgoingPtrEdges[edge->getFrom()->getAddress()].push_back(edge);
  incomingPtrEdges[edge->getTo()->getAddress()].push_back(edge);
  return edge;
}

std::vector<PtrEdge *> &MemGraph::getOutgoingPtrEdges(uint64_t address) {
  if (outgoingPtrEdges.find(address) == outgoingPtrEdges.end()) {
    return emptyPtrEdgeVector;
  }
  return outgoingPtrEdges[address];
}

std::vector<PtrEdge *> &MemGraph::getIncomingPtrEdges(uint64_t address) {
  if (incomingPtrEdges.find(address) == incomingPtrEdges.end()) {
    return emptyPtrEdgeVector;
  }
  return incomingPtrEdges[address];
}

ObjectHolder::ObjectHolder(const ObjectHolder &b) : os(b.os) {
  if (os)
    ++os->refCount;
}

ObjectHolder::ObjectHolder(ObjectState *_os) : os(_os) {
  if (os)
    ++os->refCount;
}

ObjectHolder::~ObjectHolder() {
  if (os && --os->refCount == 0)
    delete os;
}

ObjectHolder &ObjectHolder::operator=(const ObjectHolder &b) {
  if (b.os)
    ++b.os->refCount;
  if (os && --os->refCount == 0)
    delete os;
  os = b.os;
  return *this;
}

/***/

int MemoryObject::counter = 0;

MemoryObject::~MemoryObject() {
  if (parent)
    parent->markFreed(this);
}

void MemoryObject::getAllocInfo(std::string &result) const {
  llvm::raw_string_ostream info(result);

  info << "MO" << id << "[" << size << "]";

  if (allocSite) {
    info << " allocated at ";
    if (const Instruction *i = dyn_cast<Instruction>(allocSite)) {
      info << i->getParent()->getParent()->getName() << "():";
      info << *i;
    } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(allocSite)) {
      info << "global:" << gv->getName();
    } else {
      info << "value:" << *allocSite;
    }
  } else {
    info << " (no allocation info)";
  }

  info.flush();
}

/***/

ObjectState::ObjectState(const MemoryObject *mo)
    : copyOnWriteOwner(0), refCount(0), object(mo),
      concreteStore(new uint8_t[mo->size]), concreteMask(0), flushMask(0),
      knownSymbolics(0), updates(0, 0), size(mo->size), readOnly(false) {
  mo->refCount++;
  if (!UseConstantArrays) {
    static unsigned id = 0;
    const Array *array =
        getArrayCache()->CreateArray("tmp_arr" + llvm::utostr(++id), size);
    updates = UpdateList(array, 0);
  }
  memset(concreteStore, 0, size);
}

ObjectState::ObjectState(const MemoryObject *mo, const Array *array)
    : copyOnWriteOwner(0), refCount(0), object(mo),
      concreteStore(new uint8_t[mo->size]), concreteMask(0), flushMask(0),
      knownSymbolics(0), updates(array, 0), size(mo->size), readOnly(false) {
  mo->refCount++;
  makeSymbolic();
  memset(concreteStore, 0, size);
}

ObjectState::ObjectState(const ObjectState &os)
    : copyOnWriteOwner(0), refCount(0), object(os.object),
      concreteStore(new uint8_t[os.size]),
      concreteMask(os.concreteMask ? new BitArray(*os.concreteMask, os.size)
                                   : 0),
      flushMask(os.flushMask ? new BitArray(*os.flushMask, os.size) : 0),
      knownSymbolics(0), updates(os.updates), size(os.size), readOnly(false) {
  assert(!os.readOnly && "no need to copy read only object?");
  if (object)
    object->refCount++;

  if (os.knownSymbolics) {
    knownSymbolics = new ref<Expr>[size];
    for (unsigned i = 0; i < size; i++)
      knownSymbolics[i] = os.knownSymbolics[i];
  }

  memcpy(concreteStore, os.concreteStore, size * sizeof(*concreteStore));
}

ObjectState::~ObjectState() {
  delete concreteMask;
  delete flushMask;
  delete[] knownSymbolics;
  delete[] concreteStore;
  concreteMask = nullptr;
  flushMask = nullptr;
  knownSymbolics = nullptr;
  concreteStore = nullptr;

  if (object) {
    assert(object->refCount > 0);
    object->refCount--;
    if (object->refCount == 0) {
      delete object;
      object = nullptr;
    }
  }
}

ArrayCache *ObjectState::getArrayCache() const {
  assert(object && "object was NULL");
  return object->parent->getArrayCache();
}

/***/

const UpdateList &ObjectState::getUpdates() const {
  // Constant arrays are created lazily.
  if (!updates.root) {
    // Collect the list of writes, with the oldest writes first.

    // FIXME: We should be able to do this more efficiently, we just need to be
    // careful to get the interaction with the cache right. In particular we
    // should avoid creating UpdateNode instances we never use.
    unsigned NumWrites = updates.head ? updates.head->getSize() : 0;
    std::vector<std::pair<ref<Expr>, ref<Expr>>> Writes(NumWrites);
    const UpdateNode *un = updates.head;
    for (unsigned i = NumWrites; i != 0; un = un->next) {
      --i;
      Writes[i] = std::make_pair(un->index, un->value);
    }

    std::vector<ref<ConstantExpr>> Contents(size);

    // Initialize to zeros.
    for (unsigned i = 0, e = size; i != e; ++i)
      Contents[i] = ConstantExpr::create(0, Expr::Int8);

    // Pull off as many concrete writes as we can.
    unsigned Begin = 0, End = Writes.size();
    for (; Begin != End; ++Begin) {
      // Push concrete writes into the constant array.
      ConstantExpr *Index = dyn_cast<ConstantExpr>(Writes[Begin].first);
      if (!Index)
        break;

      ConstantExpr *Value = dyn_cast<ConstantExpr>(Writes[Begin].second);
      if (!Value)
        break;

      Contents[Index->getZExtValue()] = Value;
    }

    static unsigned id = 0;
    const Array *array = getArrayCache()->CreateArray(
        "const_arr" + llvm::utostr(++id), size, &Contents[0],
        &Contents[0] + Contents.size());
    updates = UpdateList(array, 0);

    // Apply the remaining (non-constant) writes.
    for (; Begin != End; ++Begin)
      updates.extend(Writes[Begin].first, Writes[Begin].second);
  }

  return updates;
}

void ObjectState::flushToConcreteStore(TimingSolver *solver,
                                       const ExecutionState &state) const {
  for (unsigned i = 0; i < size; i++) {
    if (isByteKnownSymbolic(i)) {
      ref<ConstantExpr> ce;
      bool success = solver->getValue(state, read8(i), ce);
      if (!success)
        klee_warning("Solver timed out when getting a value for external call, "
                     "byte %p+%u will have random value",
                     (void *)object->address, i);
      else
        ce->toMemory(concreteStore + i);
    }
  }
}

bool ObjectState::readConcreteStore(std::vector<unsigned char> &data,
                                    uint64_t offset, uint64_t length) const {

  data.clear();
  if (concreteStore != nullptr && offset < size) {
    uint64_t count = std::min(size - offset, length);
    if (count > size || count > 64 * 1024 * 1024) {
      SPDLOG_DEBUG(
          "readConcreteStore: count {} is too large offset: {}, lenght: {}",
          count, offset, length);
      return false;
    }
    data.reserve(count);
    for (unsigned idx = offset, end = offset + count; idx < end; ++idx) {
      data.push_back(concreteStore[idx]);
    }
    return true;
  }
  return false;
}

void ObjectState::makeConcrete() {
  delete concreteMask;
  delete flushMask;
  delete[] knownSymbolics;
  concreteMask = 0;
  flushMask = 0;
  knownSymbolics = 0;
}

void ObjectState::makeSymbolic() {
  assert(!updates.head &&
         "XXX makeSymbolic of objects with symbolic values is unsupported");

  // XXX simplify this, can just delete various arrays I guess
  for (unsigned i = 0; i < size; i++) {
    markByteSymbolic(i);
    setKnownSymbolic(i, 0);
    markByteFlushed(i);
  }
}

void ObjectState::initializeToZero() {
  makeConcrete();
  memset(concreteStore, 0, size);
}

void ObjectState::initializeToRandom() {
  makeConcrete();
  for (unsigned i = 0; i < size; i++) {
    // randomly selected by 256 sided die
    concreteStore[i] = 0xAB;
  }
}

/*
Cache Invariants
--
isByteKnownSymbolic(i) => !isByteConcrete(i)
isByteConcrete(i) => !isByteKnownSymbolic(i)
!isByteFlushed(i) => (isByteConcrete(i) || isByteKnownSymbolic(i))
 */

void ObjectState::fastRangeCheckOffset(ref<Expr> offset, unsigned *base_r,
                                       unsigned *size_r) const {
  *base_r = 0;
  *size_r = size;
}

void ObjectState::flushRangeForRead(unsigned rangeBase,
                                    unsigned rangeSize) const {
  if (!flushMask)
    flushMask = new BitArray(size, true);

  for (unsigned offset = rangeBase; offset < rangeBase + rangeSize; offset++) {
    if (!isByteFlushed(offset)) {
      if (isByteConcrete(offset)) {
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       ConstantExpr::create(concreteStore[offset], Expr::Int8));
      } else {
        assert(isByteKnownSymbolic(offset) && "invalid bit set in flushMask");
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       knownSymbolics[offset]);
      }

      flushMask->unset(offset);
    }
  }
}

void ObjectState::flushRangeForWrite(unsigned rangeBase, unsigned rangeSize) {
  if (!flushMask)
    flushMask = new BitArray(size, true);

  for (unsigned offset = rangeBase; offset < rangeBase + rangeSize; offset++) {
    if (!isByteFlushed(offset)) {
      if (isByteConcrete(offset)) {
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       ConstantExpr::create(concreteStore[offset], Expr::Int8));
        markByteSymbolic(offset);
      } else {
        assert(isByteKnownSymbolic(offset) && "invalid bit set in flushMask");
        updates.extend(ConstantExpr::create(offset, Expr::Int32),
                       knownSymbolics[offset]);
        setKnownSymbolic(offset, 0);
      }

      flushMask->unset(offset);
    } else {
      // flushed bytes that are written over still need
      // to be marked out
      if (isByteConcrete(offset)) {
        markByteSymbolic(offset);
      } else if (isByteKnownSymbolic(offset)) {
        setKnownSymbolic(offset, 0);
      }
    }
  }
}

bool ObjectState::isByteConcrete(unsigned offset) const {
  return !concreteMask || concreteMask->get(offset);
}

bool ObjectState::isByteFlushed(unsigned offset) const {
  return flushMask && !flushMask->get(offset);
}

bool ObjectState::isByteKnownSymbolic(unsigned offset) const {
  return knownSymbolics && knownSymbolics[offset].get();
}

void ObjectState::markByteConcrete(unsigned offset) {
  if (concreteMask)
    concreteMask->set(offset);
}

void ObjectState::markByteSymbolic(unsigned offset) {
  if (!concreteMask)
    concreteMask = new BitArray(size, true);
  concreteMask->unset(offset);
}

void ObjectState::markByteUnflushed(unsigned offset) {
  if (flushMask)
    flushMask->set(offset);
}

void ObjectState::markByteFlushed(unsigned offset) {
  if (!flushMask) {
    flushMask = new BitArray(size, false);
  } else {
    flushMask->unset(offset);
  }
}

void ObjectState::setKnownSymbolic(unsigned offset,
                                   Expr *value /* can be null */) {
  if (knownSymbolics) {
    knownSymbolics[offset] = value;
  } else {
    if (value) {
      knownSymbolics = new ref<Expr>[size];
      knownSymbolics[offset] = value;
    }
  }
}

/***/

ref<Expr> ObjectState::read8(unsigned offset) const {
  if (isByteConcrete(offset)) {
    return ConstantExpr::create(concreteStore[offset], Expr::Int8);
  } else if (isByteKnownSymbolic(offset)) {
    return knownSymbolics[offset];
  } else {
    assert(isByteFlushed(offset) && "unflushed byte without cache value");

    return ReadExpr::create(getUpdates(),
                            ConstantExpr::create(offset, Expr::Int32));
  }
}

ref<Expr> ObjectState::read8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");
  unsigned base, size;
  fastRangeCheckOffset(offset, &base, &size);
  flushRangeForRead(base, size);

  if (size > 4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(0,
                      "flushing %d bytes on read, may be slow and/or crash: %s",
                      size, allocInfo.c_str());
  }

  return ReadExpr::create(getUpdates(), ZExtExpr::create(offset, Expr::Int32));
}

void ObjectState::write8(unsigned offset, uint8_t value) {
  // assert(read_only == false && "writing to read-only object!");
  concreteStore[offset] = value;
  setKnownSymbolic(offset, 0);

  markByteConcrete(offset);
  markByteUnflushed(offset);
}

void ObjectState::write8(unsigned offset, ref<Expr> value) {
  // can happen when ExtractExpr special cases
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    write8(offset, (uint8_t)CE->getZExtValue(8));
  } else {
    setKnownSymbolic(offset, value.get());

    markByteSymbolic(offset);
    markByteUnflushed(offset);
  }
}

void ObjectState::write8(ref<Expr> offset, ref<Expr> value) {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic write8");
  unsigned base, size;
  fastRangeCheckOffset(offset, &base, &size);
  flushRangeForWrite(base, size);

  if (size > 4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(
        0, "flushing %d bytes on write, may be slow and/or crash: %s", size,
        allocInfo.c_str());
  }

  updates.extend(ZExtExpr::create(offset, Expr::Int32), value);
}

/***/

ref<Expr> ObjectState::read(ref<Expr> offset, Expr::Width width) const {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for reads at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset))
    return read(CE->getZExtValue(32), width);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte =
        read8(AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)));
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

ref<Expr> ObjectState::read(unsigned offset, Expr::Width width) const {
  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid width for read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = read8(offset + idx);
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

void ObjectState::write(ref<Expr> offset, ref<Expr> value) {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for writes at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset)) {
    write(CE->getZExtValue(32), value);
    return;
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  unsigned NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)),
           ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
}

void ObjectState::write(unsigned offset, ref<Expr> value) {
  // Check for writes of constant values.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    Expr::Width w = CE->getWidth();
    if (w <= 64 && klee::bits64::isPowerOfTwo(w)) {
      uint64_t val = CE->getZExtValue();
      switch (w) {
      default:
        assert(0 && "Invalid write size!");
      case Expr::Bool:
      case Expr::Int8:
        write8(offset, val);
        return;
      case Expr::Int16:
        write16(offset, val);
        return;
      case Expr::Int32:
        write32(offset, val);
        return;
      case Expr::Int64:
        write64(offset, val);
        return;
      }
    }
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  unsigned NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
}

void ObjectState::write16(unsigned offset, uint16_t value) {
  unsigned NumBytes = 2;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::write32(unsigned offset, uint32_t value) {
  unsigned NumBytes = 4;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::write64(unsigned offset, uint64_t value) {
  unsigned NumBytes = 8;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::print() const {
  llvm::errs() << "-- ObjectState --\n";
  llvm::errs() << "\tMemoryObject ID: " << object->id << "\n";
  llvm::errs() << "\tRoot Object: " << updates.root << "\n";
  llvm::errs() << "\tSize: " << size << "\n";

  llvm::errs() << "\tBytes:\n";
  for (unsigned i = 0; i < size; i++) {
    llvm::errs() << "\t\t[" << i << "]"
                 << " concrete? " << isByteConcrete(i) << " known-sym? "
                 << isByteKnownSymbolic(i) << " flushed? " << isByteFlushed(i)
                 << " = ";
    ref<Expr> e = read8(i);
    llvm::errs() << e << "\n";
  }

  llvm::errs() << "\tUpdates:\n";
  for (const UpdateNode *un = updates.head; un; un = un->next) {
    llvm::errs() << "\t\t[" << un->index << "] = " << un->value << "\n";
  }
}

} // namespace klee