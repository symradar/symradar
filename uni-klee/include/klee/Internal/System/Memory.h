//===-- Memory.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MEMORY_H
#define KLEE_MEMORY_H

#include "../../lib/Core/Context.h"
#include "../../lib/Core/TimingSolver.h"
#include "../../lib/Core/AddressSpace.h"
#include "klee/Expr.h"

#include "llvm/ADT/StringExtras.h"

#include "external/json/json.h"

#include <vector>
#include <string>

namespace llvm {
class Value;
}

namespace klee {

class BitArray;
class MemoryManager;
class Solver;
class ArrayCache;
class AddressSpace;

enum class MemKind {
  invalid,
  external,
  global,
  param,
  alloca_l,
  heap,
  output,
  lazy,
  va_arg
};

void fromDataString(std::vector<unsigned char> &data, const std::string &str);
std::string toDataString(const std::vector<unsigned char> &data);
std::string pointerToDataString(uint64_t ptr);
uint64_t dataStringToPointer(const std::string &str);
std::string pointerToBigEndianString(uint64_t ptr);
std::string get_type_string(const llvm::Type *type);
uintptr_t swap_endian(uintptr_t u);
std::vector<ref<ConstantExpr>> *loadArgs(Json::Value &root);
std::vector<MemoryObject *> *loadGlbs(Json::Value &root);
std::vector<MemoryObject *> *loadObjs(ExecutionState &state, Json::Value &root,
                                      MemoryManager &mm);

class MemNode {
private:
  // Essentials, serialized
  uint64_t address;
  uint64_t size;
  std::set<uint64_t> pointers; // outgoing pointers
  // Extra, not serialized, for debug
  const MemoryObject *mo;

public:
  MemNode(uint64_t address, uint64_t size, const MemoryObject *mo = nullptr)
      : address(address), size(size), mo(mo) {}
  uint64_t getAddress() { return address; }
  uint64_t getSize() { return size; }
  void setMemoryObject(const MemoryObject *mo) { this->mo = mo; }
  const MemoryObject *getMemoryObject() { return mo; }
  void addPointer(uint64_t pointer) { pointers.insert(pointer); }
  std::set<uint64_t> &getPointers() { return pointers; }
  void dumpToJSON(Json::Value &root);
};

class PtrNode {
private:
  // Essentials, serialized
  uint64_t address;
  uint64_t baseAddr;
  uint64_t offset;
  // Extra, not serialized, for debug
  MemNode *memNode;

public:
  PtrNode(uint64_t address, uint64_t baseAddr, uint64_t offset,
          MemNode *memNode = nullptr)
      : address(address), baseAddr(baseAddr), offset(offset), memNode(memNode) {
  }
  uint64_t getAddress() { return address; }
  uint64_t getBaseAddr() { return baseAddr; }
  uint64_t getOffset() { return offset; }
  void setMemNode(MemNode *memNode) { this->memNode = memNode; }
  MemNode *getMemNode() { return memNode; }
  void dumpToJSON(Json::Value &root);
};

class PtrEdge {
private:
  PtrNode *from;
  PtrNode *to;

public:
  PtrEdge(PtrNode *from, PtrNode *to) : from(from), to(to) {}
  PtrNode *getFrom() { return from; }
  PtrNode *getTo() { return to; }
  void dumpToJSON(Json::Value &root);
};

class MemEdge {
private:
  MemNode *from;
  MemNode *to;
  PtrEdge *ptrEdge;

public:
  MemEdge(MemNode *from, MemNode *to, PtrEdge *ptrEdge)
      : from(from), to(to), ptrEdge(ptrEdge) {}
  MemNode *getFrom() { return from; }
  MemNode *getTo() { return to; }
  PtrEdge *getPtrEdge() { return ptrEdge; }
  void dumpToJSON(Json::Value &root);
};

class MemGraph {
private:
  // Nodes: 1-to-1 mapping from address to node (serialized)
  std::map<uint64_t, MemNode *> memNodes;
  std::map<uint64_t, PtrNode *> ptrNodes;
  // Edges: record all accessed pointers (serialized)
  std::vector<MemEdge *> memEdges;
  std::vector<PtrEdge *> ptrEdges;
  // Edges for in memory use (not serialized, can be rebuilt from memEdges and
  // ptrEdges)
  std::map<uint64_t, std::vector<MemEdge *>> outgoingMemEdges;
  std::map<uint64_t, std::vector<MemEdge *>> incomingMemEdges;
  std::map<uint64_t, std::vector<PtrEdge *>> outgoingPtrEdges;
  std::map<uint64_t, std::vector<PtrEdge *>> incomingPtrEdges;
  // Just empty vectors for invalid addresses
  std::vector<MemEdge *> emptyMemEdgeVector;
  std::vector<PtrEdge *> emptyPtrEdgeVector;

public:
  MemGraph() {}
  MemGraph(std::map<uint64_t, MemNode *> &memNodes,
           std::map<uint64_t, PtrNode *> &ptrNodes)
      : memNodes(memNodes), ptrNodes(ptrNodes) {}
  MemGraph(const MemGraph &other)
      : memNodes(other.memNodes), ptrNodes(other.ptrNodes),
        memEdges(other.memEdges), ptrEdges(other.ptrEdges),
        outgoingMemEdges(other.outgoingMemEdges),
        incomingMemEdges(other.incomingMemEdges),
        outgoingPtrEdges(other.outgoingPtrEdges),
        incomingPtrEdges(other.incomingPtrEdges) {}
  void free();
  MemGraph *prune();
  MemNode *addMemNode(uint64_t address, uint64_t size,
                      const MemoryObject *mo = nullptr);
  MemNode *addMemNode(MemNode *memNode);
  MemNode *getMemNode(uint64_t address);
  PtrNode *addPtrNode(uint64_t address, uint64_t baseAddr, uint64_t offset,
                      MemNode *memNode = nullptr);
  PtrNode *addPtrNode(uint64_t address, MemNode *memNode);
  PtrNode *addPtrNode(PtrNode *ptrNode);
  PtrNode *getPtrNode(uint64_t address);
  PtrNode *searchPtrNode(uint64_t address);
  MemEdge *addMemEdge(MemNode *from, MemNode *to, PtrEdge *ptrEdge);
  MemEdge *addMemEdge(MemEdge *memEdge);
  std::vector<MemEdge *> &getOutgoingMemEdges(uint64_t address);
  std::vector<MemEdge *> &getIncomingMemEdges(uint64_t address);
  PtrEdge *addPtrEdge(PtrNode *from, PtrNode *to);
  PtrEdge *addPtrEdge(PtrEdge *ptrEdge);
  std::vector<PtrEdge *> &getOutgoingPtrEdges(uint64_t address);
  std::vector<PtrEdge *> &getIncomingPtrEdges(uint64_t address);
  void dumpToJSON(Json::Value &root);
  void loadFromJSON(Json::Value &root);
};

struct MemArchive {
  bool isPtr;
  bool existing;
  uint64_t originalBase;
  uint64_t originalOffset;
  uint64_t originalSize;
  uint64_t address;
  uint64_t base;
  uint64_t size;
  uint64_t requestedAddr;
  std::vector<uint8_t> data;
  MemArchive()
      : isPtr(false), existing(false), originalBase(0), originalOffset(0),
        originalSize(0), address(0), base(0), size(0), requestedAddr(0) {}
  MemArchive(bool isPtr, bool existing, uint64_t address, uint64_t base,
             uint64_t size, std::vector<uint8_t> &data, uint64_t originalBase,
             uint64_t originalOffset, uint64_t originalSize)
      : isPtr(isPtr), existing(existing), address(address), base(base),
        size(size), data(data), originalBase(originalBase),
        originalOffset(originalOffset), originalSize(originalSize),
        requestedAddr(0) {}
  void resetOriginalForLazy(bool existing, uint64_t requestedAddr,
                            uint64_t base, uint64_t size) {
    isPtr = false;
    existing = existing;
    originalBase = base;
    originalOffset = 0;
    originalSize = size;
    this->requestedAddr = requestedAddr;
    this->base = base;
    this->size = size;
    this->address = base;
  }
};

class MemoryObject {
  friend class STPBuilder;
  friend class ObjectState;
  friend class ExecutionState;
  friend class LazyInitManager;

private:
  static int counter;
  mutable unsigned refCount;

public:
  unsigned id;
  uint64_t address;

  /// size in bytes
  unsigned size;
  mutable std::string name;

  bool isLocal;
  mutable bool isGlobal;
  bool isFixed;

  bool isUserSpecified;

  MemKind kind;
  const llvm::Type *type;

  MemoryManager *parent;

  /// "Location" for which this memory object was allocated. This
  /// should be either the allocating instruction or the global object
  /// it was allocated for (or whatever else makes sense).
  const llvm::Value *allocSite;

  mutable int depth;

  // DO NOT IMPLEMENT
  MemoryObject(const MemoryObject &b);
  MemoryObject &operator=(const MemoryObject &b);

public:
  // XXX this is just a temp hack, should be removed
  explicit MemoryObject(uint64_t _address)
      : refCount(0), id(counter++), address(_address), size(0), name(""),
        isFixed(true), kind(MemKind::invalid), type(nullptr), parent(NULL),
        allocSite(0), depth(0) {}

  MemoryObject(uint64_t _address, unsigned _size, bool _isLocal, bool _isGlobal,
               bool _isFixed, const llvm::Value *_allocSite,
               MemoryManager *_parent)
      : refCount(0), id(counter++), address(_address), size(_size), name(""),
        isLocal(_isLocal), isGlobal(_isGlobal), isFixed(_isFixed),
        isUserSpecified(false), kind(MemKind::invalid), type(nullptr),
        parent(_parent), allocSite(_allocSite), depth(0) {}

  ~MemoryObject();

  /// Get an identifying string for this allocation.
  void getAllocInfo(std::string &result) const;

  void setName(std::string name) const { this->name = name; }

  ref<ConstantExpr> getBaseExpr() const {
    return ConstantExpr::create(address, Context::get().getPointerWidth());
  }
  ref<ConstantExpr> getSizeExpr() const {
    return ConstantExpr::create(size, Context::get().getPointerWidth());
  }
  ref<Expr> getOffsetExpr(ref<Expr> pointer) const {
    return SubExpr::create(pointer, getBaseExpr());
  }
  ref<Expr> getBoundsCheckPointer(ref<Expr> pointer) const {
    return getBoundsCheckOffset(getOffsetExpr(pointer));
  }
  ref<Expr> getBoundsCheckPointer(ref<Expr> pointer, unsigned bytes) const {
    return getBoundsCheckOffset(getOffsetExpr(pointer), bytes);
  }

  ref<Expr> getBoundsCheckOffset(ref<Expr> offset) const {
    if (size == 0) {
      return EqExpr::create(
          offset, ConstantExpr::alloc(0, Context::get().getPointerWidth()));
    } else {
      return UltExpr::create(offset, getSizeExpr());
    }
  }
  ref<Expr> getBoundsCheckOffset(ref<Expr> offset, unsigned bytes) const {
    if (bytes <= size) {
      return UltExpr::create(
          offset, ConstantExpr::alloc(size - bytes + 1,
                                      Context::get().getPointerWidth()));
    } else {
      return ConstantExpr::alloc(0, Expr::Bool);
    }
  }
  void updateRefCount() { refCount++; }
};

class ObjectState {
private:
  friend class AddressSpace;
  unsigned copyOnWriteOwner; // exclusively for AddressSpace

  friend class ObjectHolder;
  unsigned refCount;

  const MemoryObject *object;

  uint8_t *concreteStore;

  // XXX cleanup name of flushMask (its backwards or something)
  BitArray *concreteMask;

  // mutable because may need flushed during read of const
  mutable BitArray *flushMask;

  ref<Expr> *knownSymbolics;

  // mutable because we may need flush during read of const
  mutable UpdateList updates;

public:
  unsigned size;

  bool readOnly;

public:
  /// Create a new object state for the given memory object with concrete
  /// contents. The initial contents are undefined, it is the callers
  /// responsibility to initialize the object contents appropriately.
  ObjectState(const MemoryObject *mo);

  /// Create a new object state for the given memory object with symbolic
  /// contents.
  ObjectState(const MemoryObject *mo, const Array *array);

  ObjectState(const ObjectState &os);
  ~ObjectState();

  const MemoryObject *getObject() const { return object; }

  void setReadOnly(bool ro) { readOnly = ro; }

  // make contents all concrete and zero
  void initializeToZero();
  // make contents all concrete and random
  void initializeToRandom();

  ref<Expr> read(ref<Expr> offset, Expr::Width width) const;
  ref<Expr> read(unsigned offset, Expr::Width width) const;
  ref<Expr> read8(unsigned offset) const;

  // return bytes written.
  void write(unsigned offset, ref<Expr> value);
  void write(ref<Expr> offset, ref<Expr> value);

  void write8(unsigned offset, uint8_t value);
  void write16(unsigned offset, uint16_t value);
  void write32(unsigned offset, uint32_t value);
  void write64(unsigned offset, uint64_t value);
  void print() const;

  /*
    Looks at all the symbolic bytes of this object, gets a value for them
    from the solver and puts them in the concreteStore.
  */
  void flushToConcreteStore(TimingSolver *solver,
                            const ExecutionState &state) const;

  bool readConcreteStore(std::vector<unsigned char> &data, uint64_t offset = 0,
                         uint64_t length = UINT64_MAX) const;

  // private:
  const UpdateList &getUpdates() const;

  void makeConcrete();

  void makeSymbolic();

  ref<Expr> read8(ref<Expr> offset) const;
  void write8(unsigned offset, ref<Expr> value);
  void write8(ref<Expr> offset, ref<Expr> value);

  void fastRangeCheckOffset(ref<Expr> offset, unsigned *base_r,
                            unsigned *size_r) const;
  void flushRangeForRead(unsigned rangeBase, unsigned rangeSize) const;
  void flushRangeForWrite(unsigned rangeBase, unsigned rangeSize);

  bool isByteConcrete(unsigned offset) const;
  bool isByteFlushed(unsigned offset) const;
  bool isByteKnownSymbolic(unsigned offset) const;

  void markByteConcrete(unsigned offset);
  void markByteSymbolic(unsigned offset);
  void markByteFlushed(unsigned offset);
  void markByteUnflushed(unsigned offset);
  void setKnownSymbolic(unsigned offset, Expr *value);

  ArrayCache *getArrayCache() const;
};

class LazyInitManager {
  std::map<uint64_t, MemoryObject *> lazyInitAssignments;
  std::map<const Array *, MemArchive> originalObjects;
  std::set<MemoryObject *> lazyInitObjectsSet;
  std::map<const Array *, std::pair<MemoryObject *, const Array *>> lazyAlloc;
  // Save original value
  std::map<const Array *,
           std::tuple<uint64_t, const MemoryObject *, const Array *, bool>>
      symbolicPointerMap; // bool: existing - if not, use symbolic size
  // std::map<const Array *, ref<ConstantExpr>>
  //     symbolicValueMap; // Symbolized value
  std::map<const Array *, std::vector<uint8_t>>
      symbolicDataMap; // Symbolized object

public:
  LazyInitManager() {}

  void addSymbolicPointer(const Array *array, uint64_t originalAddr,
                          const MemoryObject *mo, const Array *moArray,
                          bool existing) {
    symbolicPointerMap[array] =
        std::make_tuple(originalAddr, mo, moArray, existing);
  }

  std::tuple<uint64_t, const MemoryObject *, const Array *, bool>
  getSymbolicPointer(const Array *array) {
    if (symbolicPointerMap.find(array) == symbolicPointerMap.end()) {
      return std::make_tuple(0, nullptr, nullptr, false);
    }
    return symbolicPointerMap[array];
  }

  // void addSymbolicValue(const Array *array, ref<ConstantExpr> value) {
  //   symbolicValueMap[array] = value;
  // }

  // ref<ConstantExpr> getSymbolicValue(const Array *array) {
  //   if (symbolicValueMap.find(array) == symbolicValueMap.end()) {
  //     return nullptr;
  //   }
  //   return symbolicValueMap[array];
  // }

  void addSymbolicData(const Array *array, std::vector<uint8_t> data) {
    symbolicDataMap[array] = data;
  }

  std::vector<uint8_t> getSymbolicData(const Array *array) {
    if (symbolicDataMap.find(array) == symbolicDataMap.end()) {
      return std::vector<uint8_t>();
    }
    return symbolicDataMap[array];
  }

  void addLazyInitAssignment(uint64_t address, MemoryObject *mo) {
    if (lazyInitObjectsSet.find(mo) != lazyInitObjectsSet.end()) {
      return;
    }
    mo->refCount++;
    lazyInitAssignments[address] = mo;
    lazyInitObjectsSet.insert(mo);
  }
  MemoryObject *getLazyInitAssignment(uint64_t address) {
    if (lazyInitAssignments.find(address) == lazyInitAssignments.end()) {
      return nullptr;
    }
    return lazyInitAssignments[address];
  }
  bool isLazyInitObject(MemoryObject *mo) {
    return lazyInitObjectsSet.find(mo) != lazyInitObjectsSet.end();
  }
  void addOriginalObject(const Array *array, MemArchive memArc) {
    originalObjects[array] = memArc;
  }
  bool getOriginalObject(const Array *array, MemArchive &memArc) {
    if (originalObjects.find(array) == originalObjects.end()) {
      return false;
    }
    memArc = originalObjects[array];
    return true;
  }
  int getLazyInitNum() { return lazyInitAssignments.size(); }
  void addLazyAlloc(const Array *array, MemoryObject *mo,
                    const Array *lazyArray) {
    lazyAlloc[array] = std::make_pair(mo, lazyArray);
  }
  std::pair<MemoryObject *, const Array *> getLazyAlloc(const Array *array) {
    if (lazyAlloc.find(array) == lazyAlloc.end()) {
      return std::make_pair(nullptr, nullptr);
    }
    return lazyAlloc[array];
  }
};

} // namespace klee

#endif
