//===-- ExprUtil.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_READEXPRUTIL_H
#define KLEE_READEXPRUTIL_H

#include "klee/util/ExprVisitor.h"
#include "klee/Internal/System/Memory.h"
#include <vector>

#include "spdlog/spdlog.h"

namespace klee {
class Array;
class Expr;
class ReadExpr;
template <typename T> class ref;

// /// Find all ReadExprs used in the expression DAG. If visitUpdates
// /// is true then this will including those reachable by traversing
// /// update lists. Note that this may be slow and return a large
// /// number of results.
// void findReads(ref<Expr> e, bool visitUpdates,
//                std::vector<ref<ReadExpr>> &result);

// /// Return a list of all unique symbolic objects referenced by the given
// /// expression.
// void findSymbolicObjects(ref<Expr> e, std::vector<const Array *> &results);

// /// Return a list of all unique symbolic objects referenced by the
// /// given expression range.
// template <typename InputIterator>
// void findSymbolicObjects(InputIterator begin, InputIterator end,
//                          std::vector<const Array *> &results);

class ArrayChecker : public ExprVisitor {
private:
  const Array *array;
  bool found;

public:
  ArrayChecker(const Array *array) : array(array), found(false) {}
  bool containsArray(const ref<Expr> &e) {
    found = false;
    visit(e);
    return found;
  }
  Action visitRead(const ReadExpr &re) {
    if (re.updates.root == array) {
      found = true;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }
};

class ReadExprChecker : public ExprVisitor {
private:
  const ObjectState *os;
  bool found;

public:
  ReadExprChecker(const ObjectState *os) : os(os), found(false) {}
  bool containsReadFromObject(const ref<Expr> &e) {
    found = false;
    visit(e);
    return found;
  }
  Action visitRead(const ReadExpr &re) {
    if (re.updates.root == os->getUpdates().root) {
      found = true;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }
};

class ReadExprCollector : public ExprVisitor {
private:
  const ObjectState *os;
  std::vector<ref<ReadExpr>> &reads;

public:
  ReadExprCollector(const ObjectState *os, std::vector<ref<ReadExpr>> &reads)
      : os(os), reads(reads) {}
  ref<Expr> getReads(const ref<Expr> &e) {
    reads.clear();
    return visit(e);
  }
  Action visitRead(const ReadExpr &re) override {
    if (re.updates.root == os->getUpdates().root) {
      ref<ReadExpr> read =
          dyn_cast<ReadExpr>(ReadExpr::create(re.updates, re.index));
      reads.push_back(read);
      return Action::skipChildren();
    }
    return Action::doChildren();
  }
};

class ReadExprReplacer : public ExprVisitor {
private:
  const ObjectState *src_os;
  const ObjectState *dst_os;

public:
  ReadExprReplacer(const ObjectState *src_os, const ObjectState *dst_os)
      : src_os(src_os), dst_os(dst_os) {}
  ref<Expr> replcaeReadExpr(const ref<Expr> &expr) { return visit(expr); }
  Action visitRead(const ReadExpr &re) override {
    if (re.updates.root == src_os->getUpdates().root) {
      ref<Expr> read = ReadExpr::create(dst_os->getUpdates(), re.index);
      SPDLOG_DEBUG("Replace ReadExpr {} (array: {}) with {} (array: {})",
                   re.str(), (void *)re.updates.root, read->str(),
                   (void *)dst_os->getUpdates().root);
      return Action::changeTo(read);
    }
    return Action::doChildren();
  }
};

class ExprDeepCopyVisitor : public ExprVisitor {
private:
  std::map<const Expr *, ref<Expr>> exprMap;

public:
  ref<Expr> deepCopy(const ref<Expr> &expr) {
    visit(expr);
    return exprMap[expr.get()];
  }

  Action visitExpr(const Expr &e) override {
    auto it = exprMap.find(&e);
    if (it != exprMap.end()) {
      // Expression already visited, no need to visit again
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitExprPost(const Expr &e) override {
    auto it = exprMap.find(&e);
    if (it == exprMap.end()) {
      // Create a deep copy of the expression
      ref<Expr> copy = e.rebuild(nullptr);
      exprMap[&e] = copy;
    }
    return Action::doChildren();
  }
};

class SymbolicAddressVisitor : public ExprVisitor {
private:
  std::vector<const ConstantExpr *> constants;
  std::vector<const ReadExpr *> reads;
  std::set<const Array *> arrays;
  std::map<const Array *, std::vector<int>> arrayIndices;

  ref<Expr> visitActual(const ref<Expr> &e);
  Action visitConstant(const ConstantExpr &ce) override;
  Action visitRead(const ReadExpr &re) override;

public:
  SymbolicAddressVisitor() {}
  void visitAddr(const ref<Expr> &e);
  const std::vector<const ConstantExpr *> &getConstants() const {
    return constants;
  }
  const std::vector<const ReadExpr *> &getReads() const { return reads; }
  const std::set<const Array *> &getArrays() const { return arrays; }
  const std::map<const Array *, std::vector<int>> &getArrayIndices() const {
    return arrayIndices;
  }
};

} // namespace klee

#endif
