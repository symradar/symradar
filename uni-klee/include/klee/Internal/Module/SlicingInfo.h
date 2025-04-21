#ifndef KLEE_SLICINGINFO_H
#define KLEE_SLICINGINFO_H

#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include <map>
#include <string>
#include <vector>

namespace llvm {
class Module;
class Instruction;
} // namespace llvm

namespace klee {
class SlicedInst {
public:
  SlicedInst(std::string fileName, std::string funcName, unsigned line,
             unsigned columnNumber)
      : m_fileName(fileName), m_funcName(funcName), m_line(line),
        m_column(columnNumber) {}

  bool isSame(const SlicedInst i) const {
    return (m_fileName == i.m_fileName && m_funcName == i.m_funcName &&
            m_line == i.m_line && m_column == i.m_column);
  }
  bool isSame(const KInstruction *inst) const {
    return isInFile(inst->info->file) && m_line == inst->info->line &&
           m_column == inst->info->column;
  }
  bool isInFile(std::string fileName) const { return fileName == m_fileName; }
  bool isInFunction(std::string fileName, std::string funcName) const {
    return isInFile(fileName) && funcName == m_funcName;
  }
  void print() const {
    klee_message("SlicedInstruction '%s:%s': (%d, %d)", m_fileName.c_str(),
                 m_funcName.c_str(), (int)m_line, (int)m_column);
  }

  bool operator==(const SlicedInst inst) const { return this->isSame(inst); }

private:
  friend class SlicingInfo;
  std::string m_fileName;
  std::string m_funcName;
  unsigned m_line;
  unsigned m_column;
};

class SlicingInfo {
public:
  virtual ~SlicingInfo();
  void load(llvm::Module *);
  void load(std::string);

  bool isLoaded() { return m_loaded; }
  bool isSliced(SlicedInst *);
  bool isSliced(KInstruction *);

private:
  bool m_loaded = false;
  std::map<std::string, std::vector<SlicedInst *>> insts;
  void appendInst(SlicedInst *inst);
};
} // namespace klee

#endif