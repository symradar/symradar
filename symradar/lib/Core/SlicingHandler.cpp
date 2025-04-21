#include "klee/SlicingHandler.h"
#include "Executor.h"
#include "klee/Internal/Module/InstructionInfoTable.h"

#ifdef ENABLE_DG
#include "dg/tools/llvm-slicer.h"
#include "dg/tools/llvm-slicer-opts.h"
#include "dg/tools/llvm-slicer-preprocess.h"
#include "dg/tools/llvm-slicer-utils.h"
#include "dg/tools/llvm-slicer.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/PointerAnalysis/LLVMPointerAnalysisOptions.h"
#endif

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Type.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include <iostream>
#include <memory>
#include <string>

using namespace llvm;
using namespace std;

namespace klee {

cl::opt<std::string> SlicingCriteria("slicing-crit",
                                     cl::desc("Slicing criteria."));

void initializeSlicing(Executor *_executor) {
  if (SlicingCriteria.empty()) {
    return;
  }

  _executor->slicingInfo = new SlicingInfo();
}

#ifdef ENABLE_DG
std::unique_ptr<llvm::Module> slice(std::unique_ptr<llvm::Module> _M) {
  std::string entryFunctionName("main");
  // std::unique_ptr<llvm::Module> M = llvm::CloneModule(_M.get());

  llvm::Module *m = _M.get();
  if (!m->getFunction(entryFunctionName)) {
    klee_message("Fail to find entry function '%s'. ignore slicing",
                 entryFunctionName.c_str());
    return NULL;
  }
  SlicerOptions options = {};
  if (SlicingCriteria.empty()) {
    klee_message("Slicing Criteria is not set. slicer ignored.");
    return NULL;
  }
  options.dgOptions.entryFunction = string(entryFunctionName);
  options.dgOptions.PTAOptions.analysisType =
      dg::LLVMPointerAnalysisOptions::AnalysisType::fi;
  options.legacySlicingCriteria = string(SlicingCriteria);
  // options.inputFile = "get_sign.g.bc";// string(InputFile);

  if (!m->getFunction(options.dgOptions.entryFunction)) {
    llvm::errs() << "The entry function not found: "
                 << options.dgOptions.entryFunction << "\n";
    return NULL;
  }

  auto csvalues = getSlicingCriteriaValues(
      *m, options.slicingCriteria, options.legacySlicingCriteria,
      options.legacySecondarySlicingCriteria, false);
  if (csvalues.empty()) {
    llvm::errs() << "No reachable slicing criteria: '"
                 << options.slicingCriteria << "' '"
                 << options.legacySlicingCriteria << "'\n";
    ::Slicer slicer(m, options);
    if (!slicer.createEmptyMain()) {
      llvm::errs() << "ERROR: failed creating an empty main\n";
      return NULL;
    }
  }

  if (!llvmdg::cutoffDivergingBranches(*m, options.dgOptions.entryFunction,
                                       csvalues)) {
    errs() << "[llvm-slicer]: Failed cutting off diverging branches\n";
    return NULL;
  }

  ::Slicer slicer(m, options);
  if (!slicer.buildDG()) {
    errs() << "ERROR: Failed building DG\n";
    return NULL;
  }

  dg::debug::LLVMDGAssemblyAnnotationWriter::AnnotationOptsT annotationOpts;
  // ::ModuleAnnotator annotator(options, &slicer.getDG(), annotationOpts);

  std::set<LLVMNode *> criteria_nodes;
  klee_message("slicing criteria: %s", options.legacySlicingCriteria.c_str());
  if (!getSlicingCriteriaNodes(slicer.getDG(), options.slicingCriteria,
                               options.legacySlicingCriteria,
                               options.legacySecondarySlicingCriteria,
                               criteria_nodes, false)) {
    llvm::errs() << "ERROR: Failed finding slicing criteria: '"
                 << options.slicingCriteria << "'\n";

    return NULL;
  }
  if (criteria_nodes.empty()) {
    llvm::errs() << "No reachable slicing criteria: '"
                 << options.slicingCriteria << "' '"
                 << options.legacySlicingCriteria << "'\n";

    if (!slicer.createEmptyMain()) {
      llvm::errs() << "ERROR: failed creating an empty main\n";
      return NULL;
    }
  }

  if (!slicer.mark(criteria_nodes)) {
    klee_message("Finding dependent nodes failed.");
    return NULL;
  }
  return _M;
}

std::unique_ptr<llvm::Module> slice(llvm::Module *module) {
  std::unique_ptr<llvm::Module> M = llvm::CloneModule(module);
  std::string entryFunctionName("main");

  llvm::Module *m = M.get();
  if (!m->getFunction(entryFunctionName)) {
    klee_message("Fail to find entry function '%s'. ignore slicing",
                 entryFunctionName.c_str());
    return NULL;
  }
  SlicerOptions options = {};
  if (SlicingCriteria.empty()) {
    klee_message("Slicing Criteria is not set. slicer ignored.");
    return NULL;
  }
  options.dgOptions.entryFunction = string(entryFunctionName);
  options.dgOptions.PTAOptions.analysisType =
      dg::LLVMPointerAnalysisOptions::AnalysisType::fi;
  options.legacySlicingCriteria = string(SlicingCriteria);
  // options.inputFile = "get_sign.g.bc";// string(InputFile);

  if (!m->getFunction(options.dgOptions.entryFunction)) {
    llvm::errs() << "The entry function not found: "
                 << options.dgOptions.entryFunction << "\n";
    return NULL;
  }

  auto csvalues = getSlicingCriteriaValues(
      *m, options.slicingCriteria, options.legacySlicingCriteria,
      options.legacySecondarySlicingCriteria, false);
  if (csvalues.empty()) {
    llvm::errs() << "No reachable slicing criteria: '"
                 << options.slicingCriteria << "' '"
                 << options.legacySlicingCriteria << "'\n";
    ::Slicer slicer(m, options);
    if (!slicer.createEmptyMain()) {
      llvm::errs() << "ERROR: failed creating an empty main\n";
      return NULL;
    }
  }

  if (!llvmdg::cutoffDivergingBranches(*m, options.dgOptions.entryFunction,
                                       csvalues)) {
    errs() << "[llvm-slicer]: Failed cutting off diverging branches\n";
    return NULL;
  }

  ::Slicer slicer(m, options);
  if (!slicer.buildDG()) {
    errs() << "ERROR: Failed building DG\n";
    return NULL;
  }

  dg::debug::LLVMDGAssemblyAnnotationWriter::AnnotationOptsT annotationOpts;
  // ::ModuleAnnotator annotator(options, &slicer.getDG(), annotationOpts);

  std::set<LLVMNode *> criteria_nodes;
  klee_message("slicing criteria: %s", options.legacySlicingCriteria.c_str());
  if (!getSlicingCriteriaNodes(slicer.getDG(), options.slicingCriteria,
                               options.legacySlicingCriteria,
                               options.legacySecondarySlicingCriteria,
                               criteria_nodes, false)) {
    llvm::errs() << "ERROR: Failed finding slicing criteria: '"
                 << options.slicingCriteria << "'\n";

    return NULL;
  }
  if (criteria_nodes.empty()) {
    llvm::errs() << "No reachable slicing criteria: '"
                 << options.slicingCriteria << "' '"
                 << options.legacySlicingCriteria << "'\n";

    if (!slicer.createEmptyMain()) {
      llvm::errs() << "ERROR: failed creating an empty main\n";
      return NULL;
    }
  }

  if (!slicer.mark(criteria_nodes)) {
    klee_message("Finding dependent nodes failed.");
    return NULL;
  }
  return M;
}
#endif

} // namespace klee
