//===-- MemoryManager.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MemoryManager.h"
#include "CoreStats.h"
#include "klee/Internal/System/Memory.h"

#include "klee/Expr.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#include "spdlog/spdlog.h"

#include <inttypes.h>
#include <sys/mman.h>

using namespace klee;

namespace {
llvm::cl::opt<bool> DeterministicAllocation(
    "allocate-determ",
    llvm::cl::desc("Allocate memory deterministically(default=on)"),
    llvm::cl::init(true));

llvm::cl::opt<unsigned> DeterministicAllocationSize(
    "allocate-determ-size",
    llvm::cl::desc(
        "Preallocated memory for deterministic allocation in MB (default=100)"),
    llvm::cl::init(100));

llvm::cl::opt<bool>
    NullOnZeroMalloc("return-null-on-zero-malloc",
                     llvm::cl::desc("Returns NULL in case malloc(size) was "
                                    "called with size 0 (default=off)."),
                     llvm::cl::init(false));

llvm::cl::opt<unsigned> RedZoneSpace(
    "red-zone-space",
    llvm::cl::desc("Set the amount of free space between allocations. This is "
                   "important to detect out-of-bound accesses (default=10)."),
    llvm::cl::init(10));

llvm::cl::opt<unsigned long long> DeterministicStartAddress(
    "allocate-determ-start-address",
    llvm::cl::desc("Start address for deterministic allocation. Has to be page "
                   "aligned (default=0x7ff30000000)."),
    llvm::cl::init(0x7ff30000000));
} // namespace

/***/
MemoryManager::MemoryManager(ArrayCache *_arrayCache)
    : arrayCache(_arrayCache), deterministicSpace(0), nextFreeSlot(0),
      spaceSize(DeterministicAllocationSize.getValue() * 1024 * 1024) {
  if (DeterministicAllocation) {
    // Page boundary
    void *expectedAddress = (void *)DeterministicStartAddress.getValue();

    char *newSpace =
        (char *)mmap(expectedAddress, spaceSize, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (newSpace == MAP_FAILED) {
      klee_error("Couldn't mmap() memory for deterministic allocations");
    }
    if (expectedAddress != newSpace && expectedAddress != 0) {
      klee_error("Could not allocate memory deterministically");
    }

    klee_message("Deterministic memory allocation starting from %p", newSpace);
    deterministicSpace = newSpace;
    nextFreeSlot = newSpace;
  }
}

MemoryManager::~MemoryManager() {
  while (!objects.empty()) {
    MemoryObject *mo = *objects.begin();
    if (!mo->isFixed && !DeterministicAllocation)
      free((void *)mo->address);
    objects.erase(mo);
    delete mo;
  }

  if (DeterministicAllocation)
    munmap(deterministicSpace, spaceSize);
}

MemoryObject *MemoryManager::allocate(uint64_t size, bool isLocal,
                                      bool isGlobal,
                                      const llvm::Value *allocSite,
                                      size_t alignment) {
  if (size > 10 * 1024 * 1024)
    klee_warning_once(
        0, "Large alloc: %" PRIu64 " bytes.  KLEE may run out of memory.",
        size);

  // Return NULL if size is zero, this is equal to error during allocation
  if (NullOnZeroMalloc && size == 0)
    return 0;

  if (!llvm::isPowerOf2_64(alignment)) {
    klee_warning("Only alignment of power of two is supported");
    return 0;
  }

  uint64_t address = 0;
  if (DeterministicAllocation) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
    address = llvm::alignTo((uint64_t)nextFreeSlot + alignment - 1, alignment);
#else
    address = llvm::RoundUpToAlignment((uint64_t)nextFreeSlot + alignment - 1,
                                       alignment);
#endif

    // Handle the case of 0-sized allocations as 1-byte allocations.
    // This way, we make sure we have this allocation between its own red zones
    size_t alloc_size = std::max(size, (uint64_t)1);
    if ((char *)address + alloc_size < deterministicSpace + spaceSize) {
      nextFreeSlot = (char *)address + alloc_size + RedZoneSpace;
    } else {
      klee_warning_once(0,
                        "Couldn't allocate %" PRIu64
                        " bytes. Not enough deterministic space left.",
                        size);
      address = 0;
    }
  } else {
    // Use malloc for the standard case
    if (alignment <= 8)
      address = (uint64_t)malloc(size);
    else {
      int res = posix_memalign((void **)&address, alignment, size);
      if (res < 0) {
        klee_warning("Allocating aligned memory failed.");
        address = 0;
      }
    }
  }

  if (size == 0 || size > 10 * 1024 * 1024) {
    SPDLOG_DEBUG("MemoryManager::allocate: size = {}, addr = {}", size,
                 address);
  }

  if (!address)
    return 0;

  ++stats::allocations;
  MemoryObject *res = new MemoryObject(address, size, isLocal, isGlobal, false,
                                       allocSite, this);
  objects.insert(res);
  return res;
}

MemoryObject *MemoryManager::allocateFixed(uint64_t address, uint64_t size,
                                           const llvm::Value *allocSite) {
#ifndef NDEBUG
  for (objects_ty::iterator it = objects.begin(), ie = objects.end(); it != ie;
       ++it) {
    MemoryObject *mo = *it;
    if (address + size > mo->address && address < mo->address + mo->size)
      klee_error("Trying to allocate an overlapping object");
  }
#endif

  ++stats::allocations;
  MemoryObject *res =
      new MemoryObject(address, size, false, true, true, allocSite, this);
  objects.insert(res);
  if (size == 0 || size > 10 * 1024 * 1024) {
    SPDLOG_DEBUG("MemoryManager::allocateFixed: size = {}, addr = {}", size,
                 address);
  }
  return res;
}

MemoryObject *MemoryManager::inject(uint64_t address, uint64_t size,
                                    const llvm::Value *allocSite, bool isLocal,
                                    bool isGlobal, bool isFixed) {
  MemoryObject *mo = nullptr;
  if (DeterministicAllocation) {
    if (size == 0 || size > 10 * 1024 * 1024) {
      SPDLOG_DEBUG("MemoryManager::allocate: size = {}, addr = {}", size,
                   address);
    }
    mo = new MemoryObject(address, size, isLocal, isGlobal, isFixed, allocSite,
                          this);
    if ((uint64_t)nextFreeSlot < address + size &&
        address > DeterministicStartAddress &&
        address < DeterministicStartAddress + spaceSize) {
      nextFreeSlot = (char *)address + size + RedZoneSpace;
      // llvm::outs() << "nextFreeSlot: " << (uint64_t)nextFreeSlot << "\n";
    }
  }
  ++stats::allocations;
  objects.insert(mo);
  return mo;
}

void MemoryManager::deallocate(const MemoryObject *mo) { assert(0); }

void MemoryManager::markFreed(MemoryObject *mo) {
  if (objects.find(mo) != objects.end()) {
    if (!mo->isFixed && !DeterministicAllocation)
      free((void *)mo->address);
    objects.erase(mo);
  }
}

size_t MemoryManager::getUsedDeterministicSize() {
  return nextFreeSlot - deterministicSpace;
}

bool MemoryManager::isValidAddress(uint64_t address) {
  if (DeterministicAllocation) {
    return address >= DeterministicStartAddress &&
           address < DeterministicStartAddress + spaceSize;
  }
  return address != 0;
}

ref<Expr> MemoryManager::getValidAddressExpr(ref<Expr> address) {
  if (DeterministicAllocation) {
    ref<Expr> start = ConstantExpr::create(DeterministicStartAddress, 64);
    ref<Expr> end = ConstantExpr::create((uint64_t)nextFreeSlot, 64);
    return AndExpr::create(UltExpr::create(start, address),
                           UleExpr::create(address, end));
  }
  return ConstantExpr::create(1, Expr::Bool);
}