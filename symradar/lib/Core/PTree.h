//===-- PTree.h -------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_PTREE_H__
#define __UTIL_PTREE_H__

#include <klee/Expr.h>
#include "klee/ExecutionState.h"

namespace klee {
class ExecutionState;

class PTree {
  typedef ExecutionState *data_type;

public:
  typedef class PTreeNode Node;
  Node *root;

  PTree(const data_type &_root);
  ~PTree();

  std::pair<Node *, Node *> split(Node *n, const data_type &leftData,
                                  const data_type &rightData);
  void remove(Node *n);

  void dump(llvm::raw_ostream &os);

  void updateValidNodeInfo(Node *n, int delta);
};

class PTreeNode {
  friend class PTree;

public:
  PTreeNode *parent, *left, *right;
  ExecutionState *data;
  ref<Expr> condition;
  int validChildren;

private:
  PTreeNode(PTreeNode *_parent, ExecutionState *_data);
  ~PTreeNode();
};
} // namespace klee

#endif
