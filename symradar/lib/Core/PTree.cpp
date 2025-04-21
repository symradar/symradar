//===-- PTree.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PTree.h"

#include "spdlog/spdlog.h"

#include <klee/Expr.h>
#include <klee/util/ExprPPrinter.h>

#include <vector>

using namespace klee;

/* *** */

PTree::PTree(const data_type &_root) : root(new Node(0, _root)) {}

PTree::~PTree() {}

std::pair<PTreeNode *, PTreeNode *>
PTree::split(Node *n, const data_type &leftData, const data_type &rightData) {
  assert(n && !n->left && !n->right);
  n->left = new Node(n, leftData);
  n->right = new Node(n, rightData);
  n->left->validChildren = 1;
  n->right->validChildren = n->validChildren;
  updateValidNodeInfo(n, 1);
  return std::make_pair(n->left, n->right);
}

void PTree::remove(Node *n) {
  assert(!n->left && !n->right);
  updateValidNodeInfo(n, -1 * n->validChildren);
  do {
    Node *p = n->parent;
    if (p) {
      if (n == p->left) {
        p->left = 0;
      } else {
        assert(n == p->right);
        p->right = 0;
      }
    }
    delete n;
    n = p;
  } while (n && !n->left && !n->right);
}

void PTree::dump(llvm::raw_ostream &os) {
  ExprPPrinter *pp = ExprPPrinter::create(os);
  pp->setNewline("\\l");
  os << "digraph G {\n";
  os << "\tsize=\"10,7.5\";\n";
  os << "\tratio=fill;\n";
  os << "\trotate=90;\n";
  os << "\tcenter = \"true\";\n";
  os << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n";
  os << "\tedge [arrowsize=.3]\n";
  std::vector<PTree::Node *> stack;
  stack.push_back(root);
  while (!stack.empty()) {
    PTree::Node *n = stack.back();
    stack.pop_back();
    if (n->condition.isNull()) {
      os << "\tn" << n << " [label=\"\"";
    } else {
      os << "\tn" << n << " [label=\"";
      pp->print(n->condition);
      os << "\",shape=diamond";
    }
    if (n->data)
      os << ",fillcolor=green";
    os << "];\n";
    if (n->left) {
      os << "\tn" << n << " -> n" << n->left << ";\n";
      stack.push_back(n->left);
    }
    if (n->right) {
      os << "\tn" << n << " -> n" << n->right << ";\n";
      stack.push_back(n->right);
    }
  }
  os << "}\n";
  delete pp;
}

void PTree::updateValidNodeInfo(Node *n, int delta) {
  if (delta < 0) {
    SPDLOG_DEBUG("[state {}] removed or paused", n->data->getID());
  } else if (delta > 0 && n->data) {
    SPDLOG_DEBUG("[state {}] added or continued", n->data->getID());
  }
  Node *p = n;
  while (p) {
    p->validChildren += delta;
    p = p->parent;
  }
}

PTreeNode::PTreeNode(PTreeNode *_parent, ExecutionState *_data)
    : parent(_parent), left(0), right(0), data(_data), condition(0),
      validChildren(1) {}

PTreeNode::~PTreeNode() {}
