#ifndef KLEE_SLICINGHANDLER_H
#define KLEE_SLICINGHANDLER_H

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Error.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/CommandLine.h"

#include <string>
#include <memory>

namespace llvm {
class Error;
class LLVMContext;
class Module;
} // namespace llvm

namespace klee {
class Executor;
class ExecutionState;

extern llvm::cl::opt<std::string> SlicingCriteria;

extern void initializeSlicing(Executor *);
#ifdef ENABLE_DG
extern std::unique_ptr<llvm::Module> slice(llvm::Module *);
extern std::unique_ptr<llvm::Module> slice(std::unique_ptr<llvm::Module>);
#endif
} // namespace klee
#endif
