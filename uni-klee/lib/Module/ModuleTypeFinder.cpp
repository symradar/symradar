//===-- ModuleTypeFinder.cpp - Implement the ModuleTypeFinder class
//-------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ModuleTypeFinder class for the IR library.
// This class only exists because llvm-3.4 ModuleTypeFinder is throwing an
// exception
//
//===----------------------------------------------------------------------===//

#include <regex>

#include "klee/Internal/Module/ModuleTypeFinder.h"
#include "klee/Internal/Module/Snapshot.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace klee {

ModuleTypeFinder::ModuleTypeFinder(const llvm::Module *m) : module(m) {

  std::set<StructType *> types;
  find(types);

  for (Type *type : types) {
    insert_type(type);
  }

  // add primative and integer types to map
  for (unsigned id = 0; id <= Type::TokenTyID; ++id) {
    insert_type(Type::getPrimitiveType(module->getContext(), (Type::TypeID)id));
  }
  std::vector<unsigned> integers = {1, 8, 16, 32, 64};
  for (unsigned width : integers) {
    insert_type(Type::getIntNTy(module->getContext(), width));
  }
}

llvm::Type *ModuleTypeFinder::getEquivalentType(const std::string &desc) {

  // if this is type is cached, then return it
  auto itr = mapDescToType.find(desc);
  if (itr != mapDescToType.end()) {
    return itr->second;
  } else if (desc.front() == '[' && desc.back() == ']') {

    // not cached, but this is an array
    size_t pos = desc.find('x');
    if (pos != std::string::npos) {
      unsigned count = stoul(desc.substr(1, pos - 2));
      std::string sub_desc = desc.substr(pos + 2, desc.size() - pos - 3);
      if (Type *sub_type = getEquivalentType(sub_desc)) {
        Type *type = ArrayType::get(sub_type, count);

        // cache for future lookups
        mapDescToType.insert(std::make_pair(desc, type));
        return type;
      } else {
        return nullptr;
      }
    }
  } else if (desc.back() == '*') {

    // not cached, but this is a pointer
    std::string sub_desc = desc.substr(0, desc.size() - 1);
    if (Type *sub_type = getEquivalentType(sub_desc)) {
      Type *type = PointerType::get(sub_type, 0);

      // cache for future lookups
      mapDescToType.insert(std::make_pair(desc, type));
      return type;
    } else {
      return nullptr;
    }
  } else {

    static std::regex re(R"((struct\..*)\.(\d+))");
    std::smatch matches;

    if (regex_match(desc, matches, re)) {

      // not cached, but this is a duplicate type
      Type *type = getEquivalentType(matches[1]);
      mapDescToType.insert(std::make_pair(desc, type));
      return type;
    }
  }
  return nullptr;
}

void ModuleTypeFinder::addMatchingStructTypes(const std::string &desc,
                                              std::set<Type *> &types) const {

  std::string name = "%struct." + desc;
  std::string prefix = name + ".";

  for (const auto &itr : mapDescToType) {
    if (itr.first == name || itr.first.rfind(prefix, 0) == 0) {
      types.insert(itr.second);
    }
  }
}

void ModuleTypeFinder::insert_type(Type *type) {

  static std::regex re(R"((struct\..*)\.(\d+))");

  auto *st = dyn_cast<StructType>(type);
  if (st != nullptr) {
    if (st->hasName()) {

      std::string name("%");
      name += st->getName().str();
      std::smatch matches;
      if (regex_match(name, matches, re)) {

        std::string base = matches[1];
        // if the structure base name is not already in the map, insert it
        // if we later find the actual base, then entry will be overritten
        if (mapDescToType.find(base) == mapDescToType.end()) {
          mapDescToType.insert(std::make_pair(base, type));
          mapDescToType.insert(
              std::make_pair(base + '*', type->getPointerTo(0)));
        }

        mapDescToType.insert(std::make_pair(name, type));
        type = type->getPointerTo(0);
        mapDescToType.insert(std::make_pair(get_type_string(type), type));

      } else {
        // if keys already exist, then update to this type
        auto pr = mapDescToType.insert(std::make_pair(name, type));
        if (!pr.second)
          pr.first->second = type;
        type = type->getPointerTo(0);
        mapDescToType.insert(std::make_pair(get_type_string(type), type));
        if (!pr.second)
          pr.first->second = type;
      }
    }
  } else {
    mapDescToType.insert(std::make_pair(get_type_string(type), type));
    type = type->getPointerTo(0);
    mapDescToType.insert(std::make_pair(get_type_string(type), type));
  }
}

bool ModuleTypeFinder::find(std::set<StructType *> &types) {

  types.clear();

  // Get types from global variables.
  for (auto itr = module->global_begin(), end = module->global_end();
       itr != end; ++itr) {
    incorporateType(itr->getType(), types);
    if (itr->hasInitializer())
      incorporateValue(itr->getInitializer(), types);
  }

  // Get types from aliases.
  for (auto itr = module->alias_begin(), end = module->alias_end(); itr != end;
       ++itr) {
    incorporateType(itr->getType(), types);
    if (const Value *Aliasee = itr->getAliasee())
      incorporateValue(Aliasee, types);
  }

  // Get types from functions.
  SmallVector<std::pair<unsigned, MDNode *>, 4> MDForInst;
  for (auto fn_itr = module->begin(), fn_end = module->end(); fn_itr != fn_end;
       ++fn_itr) {
    incorporateType(fn_itr->getType(), types);

    if (fn_itr->hasPrefixData())
      incorporateValue(fn_itr->getPrefixData(), types);

    // First incorporate the arguments.
    for (auto arg_itr = fn_itr->arg_begin(), arg_end = fn_itr->arg_end();
         arg_itr != arg_end; ++arg_itr) {
      incorporateValue(arg_itr, types);
    }

    for (auto bb_itr = fn_itr->begin(), bb_end = fn_itr->end();
         bb_itr != bb_end; ++bb_itr) {
      for (auto inst_itr = bb_itr->begin(), inst_end = bb_itr->end();
           inst_itr != inst_end; ++inst_end) {
        const Instruction &I = *inst_itr;

        // Incorporate the type of the instruction.
        incorporateType(I.getType(), types);

        // Incorporate non-instruction operand types. (We are incorporating all
        // instructions with this loop.)
        for (auto op_itr = I.op_begin(), op_end = I.op_end(); op_itr != op_end;
             ++op_itr) {
          if (!isa<Instruction>(op_itr)) {
            incorporateValue(*op_itr, types);
          }
        }

        // Incorporate types hiding in metadata.
        I.getAllMetadataOtherThanDebugLoc(MDForInst);
        for (unsigned idx = 0, end = MDForInst.size(); idx != end; ++idx) {
          incorporateMDNode(MDForInst[idx].second, types);
        }
        MDForInst.clear();
      }
    }
  }

  for (auto itr = module->named_metadata_begin(),
            end = module->named_metadata_end();
       itr != end; ++itr) {
    auto NMD = itr; // named metadata node
    for (unsigned i = 0, e = NMD->getNumOperands(); i != e; ++i) {
      incorporateMDNode(NMD->getOperand(i), types);
    }
  }
  return !types.empty();
}

/// incorporateType - This method adds the type to the list of used structures
/// if it's not in there already.

void ModuleTypeFinder::incorporateType(Type *Ty,
                                       std::set<llvm::StructType *> &types) {
  // Check to see if we've already visited this type.
  if (!VisitedTypes.insert(Ty).second)
    return;

  std::vector<Type *> worklist;
  worklist.push_back(Ty);
  do {
    Type *type = worklist.back();
    worklist.pop_back();

    // If this is a structure or opaque type, add a name for the type.
    if (StructType *STy = dyn_cast<StructType>(type)) {
      types.insert(STy);
    }

    // Add all unvisited subtypes to worklist for processing
    for (auto itr = type->subtype_rbegin(), end = type->subtype_rend();
         itr != end; ++itr) {
      Type *subtype = *itr;
      if (VisitedTypes.insert(subtype).second)
        worklist.push_back(subtype);
    }
  } while (!worklist.empty());
}

/// incorporateValue - This method is used to walk operand lists finding types
/// hiding in constant expressions and other operands that won't be walked in
/// other ways.  GlobalValues, basic blocks, instructions, and inst operands are
/// all explicitly enumerated.
void ModuleTypeFinder::incorporateValue(const Value *V,
                                        std::set<llvm::StructType *> &types) {

  if (const MetadataAsValue *M = dyn_cast<MetadataAsValue>(V)) {
    return incorporateMDNode(M, types);
  }

  if (!isa<Constant>(V) || isa<GlobalValue>(V))
    return;

  // Already visited?
  if (!VisitedConstants.insert(V).second)
    return;

  // Check this type.
  incorporateType(V->getType(), types);

  // If this is an instruction, we incorporate it separately.
  if (isa<Instruction>(V))
    return;

  // Look in operands for types.
  const User *U = cast<User>(V);
  for (auto itr = U->op_begin(), end = U->op_end(); itr != end; ++itr) {
    incorporateValue(*itr, types);
  }
}

/// incorporateMDNode - This method is used to walk the operands of an MDNode to
/// find types hiding within.
void ModuleTypeFinder::incorporateMDNode(const MetadataAsValue *V,
                                         std::set<llvm::StructType *> &types) {
  // Already visited?
  if (!VisitedConstants.insert(dyn_cast<const Value>(V)).second)
    return;

  // Look in operands for types.
  for (unsigned idx = 0, end = V->getNumOperands(); idx != end; ++idx) {
    if (Value *Op = dyn_cast<Value>(V->getOperand(idx))) {
      incorporateValue(Op, types);
    }
  }
}

bool ModuleTypeFinder::isEquivalentType(
    const Type *type1, const Type *type2,
    std::set<std::pair<const Type *, const Type *>> &visited) {

  // get the easy cases first
  if (type1 == type2)
    return true;

  Type::TypeID tid = type1->getTypeID();
  if (tid != type2->getTypeID())
    return false;

  // if this is a circular type reference, default to true
  if (visited.find(std::make_pair(type1, type2)) != visited.end())
    return true;
  visited.insert(std::make_pair(type1, type2));

  // how we have to check by type id
  switch (tid) {

  case Type::VoidTyID:
  case Type::FloatTyID:
  case Type::DoubleTyID:
  case Type::X86_FP80TyID:
  case Type::FP128TyID:
  case Type::PPC_FP128TyID:
  case Type::LabelTyID:
  case Type::MetadataTyID:
    return true;

  case Type::IntegerTyID: {
    const auto *int1 = cast<IntegerType>(type1);
    const auto *int2 = cast<IntegerType>(type2);
    return int1->getBitWidth() == int2->getBitWidth();
  }

  case Type::PointerTyID: {
    const auto *ptr1 = cast<PointerType>(type1);
    const auto *ptr2 = cast<PointerType>(type2);
    return isEquivalentType(ptr1->getPointerElementType(),
                            ptr2->getPointerElementType(), visited);
  }

  case Type::ArrayTyID: {
    const auto *array1 = cast<ArrayType>(type1);
    const auto *array2 = cast<ArrayType>(type2);
    if (array1->getNumElements() != array2->getNumElements())
      return false;
    return isEquivalentType(array1->getElementType(), array2->getElementType(),
                            visited);
  }

  case Type::VectorTyID: {
    const auto *vec1 = cast<VectorType>(type1);
    const auto *vec2 = cast<VectorType>(type2);
    if (vec1->getNumElements() != vec2->getNumElements())
      return false;
    return isEquivalentType(vec1->getElementType(), vec2->getElementType(),
                            visited);
  }

  case Type::StructTyID: {
    const auto *struct1 = cast<StructType>(type1);
    const auto *struct2 = cast<StructType>(type2);

    if (struct1->isPacked() != struct2->isPacked())
      return false;
    if (struct1->getNumElements() != struct2->getNumElements())
      return false;

    for (unsigned idx = 0, end = struct1->getNumElements(); idx != end; ++idx) {
      if (!isEquivalentType(struct1->getElementType(idx),
                            struct2->getElementType(idx), visited)) {
        return false;
      }
    }
    return true;
  }

  case Type::FunctionTyID: {
    const auto *fn1 = cast<FunctionType>(type1);
    const auto *fn2 = cast<FunctionType>(type2);
    if (fn1->isVarArg() != fn2->isVarArg())
      return false;
    if (fn1->getNumParams() != fn2->getNumParams())
      return false;
    if (!isEquivalentType(fn1->getReturnType(), fn2->getReturnType(), visited))
      return false;

    for (unsigned idx = 0, end = fn1->getNumParams(); idx != end; ++idx) {
      if (!isEquivalentType(fn1->getParamType(idx), fn2->getParamType(idx),
                            visited)) {
        return false;
      }
    }
    return true;
  }

  default:
    assert(false && ("unknown type in comparison"));
  }
  // should not arrive here...
  return false;
}

bool ModuleTypeFinder::isEquivalentType(const Type *type1, const Type *type2) {

  std::set<std::pair<const Type *, const Type *>> visited;
  return isEquivalentType(type1, type2, visited);
}

} // namespace klee
