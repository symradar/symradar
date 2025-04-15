//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Module/KInstruction.h"
#include <string>

using namespace llvm;
using namespace klee;

/***/

KInstruction::~KInstruction() { delete[] operands; }

std::string KInstruction::getSourceLocation(bool simple) const {
  if (!info->file.empty()) {
    if (simple)
      return info->file + ":" + std::to_string(info->line);
    else
      return info->file + ":" + std::to_string(info->line) + ":" +
             std::to_string(info->column) + ":" +
             std::to_string(info->assemblyLine);
  } else {
    return "no_debug_info";
  }
}
