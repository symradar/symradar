//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <map>
#include <set>
#include <string>
#include <vector>

#include "klee/ExecutionState.h"
#include "klee/Expr.h"
#include "klee/Interpreter.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/System/Memory.h"
#include "klee/Internal/System/RuntimeFD.h"
#include "klee/Internal/Module/KInstruction.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/Pass.h"

#ifndef KLEE_CONTROL_FLOW_GRAPH_H
#define KLEE_CONTROL_FLOW_GRAPH_H

namespace klee {
extern llvm::cl::opt<std::string> UniKleeReplay;
enum class CFGNodeType {
  Target,      // target instruction
  Required,    // required to be executed
  AtLeastOnce, // should be executed at least once
  Optional,    // optional to be executed
  NotAllowed,  // immediate access is not permitted
  Unknown,     // unknown
};

// llvm hierarchy: module -> function -> basic block -> instruction
// We only need CFG of single target function
class CFGNode {
public:
  const llvm::Function *parent;
  const llvm::BasicBlock *v;
  std::vector<const llvm::Instruction *> children;
  std::vector<CFGNode *> pred;
  std::vector<CFGNode *> succ;
  CFGNodeType type;
  bool dead;
  std::string name;
  int id;

public:
  CFGNode(const llvm::Function *parent, const llvm::BasicBlock *v, int id);
  ~CFGNode();
  void addChild(const llvm::Instruction *v);
  void addPred(CFGNode *v);
  void addSucc(CFGNode *v);
  void markDead(bool isDead) { dead = isDead; }
};

// class CFGEdge {
// public:
//   CFGNode *src;
//   CFGNode *dst;

// public:
//   CFGEdge(CFGNode *src, CFGNode *dst) : src(src), dst(dst) {}
//   ~CFGEdge();
// };

void dump_cfg(const KModule *km, std::string filename);

class ControlFlowGraph {
public:
  const KFunction *kf;
  const llvm::Function *f;
  KInstruction *targetKInst;
  const llvm::Instruction *targetInst;
  const llvm::Instruction *patchBranchInst;
  std::map<const llvm::BasicBlock *, CFGNode *> cfg;
  std::vector<CFGNode *> buggyTrace;
  std::string targetLineStr;

public:
  ControlFlowGraph(const KFunction *kf);
  ~ControlFlowGraph();

  void build();
  void markTargetKInst(KInstruction *kinst);
  void markBasicBlocks();
  void addBuggyTrace(const std::vector<std::string> &trace);
  bool checkExitNode(const llvm::BasicBlock *bb);
  bool checkDeadNode(const llvm::BasicBlock *bb) const;
  KInstruction *findTargetInst(std::string func) const;
};

} // namespace klee
#endif