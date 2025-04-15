//===-- FunctionStateInfo.h -------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_FUNCTIONSTATEINFO_H
#define KLEE_FUNCTIONSTATEINFO_H

#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <set>

namespace klee {

class FunctionStateInfo {
public:
  unsigned refCount;

private:
  std::map<llvm::Function *, std::string> stateInfoMap;

public:
  FunctionStateInfo() : refCount(0) {}

  void addStateInfo(llvm::Function *callee, std::string info);

  FunctionStateInfo *copy() const {
    FunctionStateInfo *ret = new FunctionStateInfo();
    ret->stateInfoMap = stateInfoMap;
    return ret;
  }

  void dump() const { print(llvm::errs()); }

  void print(llvm::raw_ostream &out) const;

  virtual ~FunctionStateInfo();
};

} /* namespace klee */

#endif /* KLEE_FUNCTIONSTATEINFO_H */