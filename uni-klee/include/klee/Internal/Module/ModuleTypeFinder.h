//===-- ModuleTypeFinder.h - Class to find used struct types --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the ModuleTypeFinder class.
// This class only exists because llvm-3.4 ModuleTypeFinder is throwing an
// exception
//
//===----------------------------------------------------------------------===//

#ifndef MODULE_TYPES_H
#define MODULE_TYPES_H

#include "llvm/ADT/DenseSet.h"
#include <map>
#include <set>
#include <vector>

namespace llvm {

class MDNode;
class MetadataAsValue;
class Module;
class StructType;
class Type;
class Value;

} // namespace llvm

namespace klee {

/// ModuleTypeFinder - Walk over a module, identifying all of the types that are
/// used by the module.
class ModuleTypeFinder {
  // To avoid walking constant expressions multiple times and other IR
  // objects, we keep several helper maps.
  std::set<const llvm::Value *> VisitedConstants;
  std::set<llvm::Type *> VisitedTypes;
  const llvm::Module *module;
  std::map<std::string, llvm::Type *> mapDescToType;

  void insert_type(llvm::Type *type);

  /// incorporateType - This method adds the type to the list of used
  /// structures if it's not in there already.
  void incorporateType(llvm::Type *Ty, std::set<llvm::StructType *> &types);

  /// incorporateValue - This method is used to walk operand lists finding types
  /// hiding in constant expressions and other operands that won't be walked in
  /// other ways.  GlobalValues, basic blocks, instructions, and inst operands
  /// are all explicitly enumerated.
  void incorporateValue(const llvm::Value *V,
                        std::set<llvm::StructType *> &types);

  /// incorporateMDNode - This method is used to walk the operands of an MDNode
  /// to find types hiding within.
  void incorporateMDNode(const llvm::MetadataAsValue *V,
                         std::set<llvm::StructType *> &types);

  bool find(std::set<llvm::StructType *> &types);

  static bool isEquivalentType(
      const llvm::Type *type1, const llvm::Type *type2,
      std::set<std::pair<const llvm::Type *, const llvm::Type *>> &visited);

public:
  explicit ModuleTypeFinder(const llvm::Module *m);
  llvm::Type *getEquivalentType(const std::string &desc);
  void addMatchingStructTypes(const std::string &desc,
                              std::set<llvm::Type *> &types) const;

  static bool isEquivalentType(const llvm::Type *type1,
                               const llvm::Type *type2);

private:
};

} // namespace klee

#endif
