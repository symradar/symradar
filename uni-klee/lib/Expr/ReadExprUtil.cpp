//===-- ExprUtil.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/util/ReadExprUtil.h"
#include "klee/util/ExprHashMap.h"

#include "klee/Expr.h"

#include "klee/util/ExprVisitor.h"

#include <set>

using namespace klee;

namespace klee {

class SymbolicObjectFinder : public ExprVisitor {
protected:
  Action visitRead(const ReadExpr &re) {
    const UpdateList &ul = re.updates;

    // XXX should we memo better than what ExprVisitor is doing for us?
    for (const UpdateNode *un = ul.head; un; un = un->next) {
      visit(un->index);
      visit(un->value);
    }

    if (ul.root->isSymbolicArray())
      if (results.insert(ul.root).second)
        objects.push_back(ul.root);

    return Action::doChildren();
  }

public:
  std::set<const Array *> results;
  std::vector<const Array *> &objects;

  SymbolicObjectFinder(std::vector<const Array *> &_objects)
      : objects(_objects) {}
};

ExprVisitor::Action
SymbolicAddressVisitor::visitConstant(const ConstantExpr &ce) {
  constants.push_back(&ce);
  return Action::skipChildren();
}

ExprVisitor::Action SymbolicAddressVisitor::visitRead(const ReadExpr &re) {
  reads.push_back(&re);
  arrays.insert(re.updates.root);
  ref<Expr> index = re.index;
  if (isa<ConstantExpr>(index)) {
    const Array *array = re.updates.root;
    if (arrayIndices.count(array) == 0)
      arrayIndices[array] = std::vector<int>();
    arrayIndices[array].push_back(cast<ConstantExpr>(index)->getZExtValue());
  }
  return Action::doChildren();
}

void SymbolicAddressVisitor::visitAddr(const ref<Expr> &e) {
  constants.clear();
  reads.clear();
  arrays.clear();
  arrayIndices.clear();
  visit(e);
}

} // namespace klee
