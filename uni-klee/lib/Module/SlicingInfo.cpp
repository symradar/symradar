#include "klee/Internal/Module/SlicingInfo.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/Support/FileSystem.h>

#include "external/json/json.h"

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

using namespace klee;

void SlicingInfo::load(std::string path) {
  Json::Value root;
  Json::String errs;
  std::ifstream f(path);
  klee_message("try to load sliced file '%s'.", path.c_str());
  if (f.fail()) {
    klee_message("Fail to load sliced file '%s'.", path.c_str());
    return;
  }

  f >> root;
  for (auto iter : root) {
    SlicedInst *inst =
        new SlicedInst(std::string(iter["SourceFile"].asString()),
                       std::string(iter["FunctionName"].asString()),
                       unsigned(iter["LineNumber"].asUInt()),
                       unsigned(iter["Column"].asUInt()));
    appendInst(inst);
  }
  m_loaded = true;
}

void SlicingInfo::load(llvm::Module *m) {
  for (llvm::Module::iterator mIter = m->begin(); mIter != m->end(); ++mIter) {
    llvm::Function *f = llvm::cast<llvm::Function>(mIter);
    llvm::SmallString<128> filenameVec =
        llvm::StringRef(f->getParent()->getSourceFileName());
    llvm::sys::fs::make_absolute(filenameVec);

    for (llvm::Function::iterator fIter = f->begin(); fIter != f->end();
         ++fIter) {
      llvm::BasicBlock *b = llvm::cast<llvm::BasicBlock>(fIter);

      for (llvm::BasicBlock::iterator bIter = b->begin(); bIter != b->end();
           ++bIter) {
        llvm::Instruction *i = llvm::cast<llvm::Instruction>(bIter);
        const llvm::DebugLoc &debugInfo = i->getDebugLoc();

        SlicedInst *inst =
            new SlicedInst(filenameVec.c_str(), std::string(f->getName()),
                           debugInfo.getLine(), debugInfo.getCol());
        inst->print();
        appendInst(inst);
      }
    }
  }
  this->m_loaded = true;

  return;
}

bool SlicingInfo::isSliced(KInstruction *inst) {
  std::string key = inst->info->file;
  if (!insts.count(key))
    return false;
  for (auto slicedI : insts[key]) {
    if (slicedI->isSame(inst))
      return true;
  }
  return false;
}

bool SlicingInfo::isSliced(SlicedInst *inst) {
  std::string key = inst->m_fileName;
  if (!insts.count(key))
    return false;

  for (auto slicedI : insts[key]) {
    if (*slicedI == *inst) {
      return true;
    }
  }
  return false;
}

void SlicingInfo::appendInst(SlicedInst *inst) {
  if (isSliced(inst)) {
    return;
  }

  std::string key = inst->m_fileName;
  if (!insts.count(key)) {
    std::vector<SlicedInst *> v = {};

    insts[key] = v;
  }
  insts[key].push_back(inst);
}

SlicingInfo::~SlicingInfo() {}
