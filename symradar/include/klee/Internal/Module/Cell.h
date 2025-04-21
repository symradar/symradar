//===-- Cell.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CELL_H
#define KLEE_CELL_H

#include <klee/Expr.h>
#include "klee/Internal/Module/Snapshot.h"

#include "llvm/ADT/SmallSet.h"

namespace klee {
class MemoryObject;

struct Cell {
  ref<Expr> value;
  bool taint;
  llvm::SmallSet<MemRecord, 2> mr;
  Cell() : taint(false) {}
};

} // namespace klee

#endif
