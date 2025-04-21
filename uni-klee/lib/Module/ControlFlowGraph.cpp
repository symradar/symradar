//===-----------------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iomanip>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(8, 0)
#include "llvm/IR/CFG.h"
#else
#include "llvm/Analysis/CFG.h"
#endif
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/User.h"
#include "llvm/Pass.h"
#include "llvm/ADT/StringRef.h"

#include "klee/Internal/Module/ControlFlowGraph.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"

// #include "sqlite3.h"

using namespace klee;

namespace klee {

void dump_cfg(const KModule *km, std::string filename) {
  // Dump CFG to bb-trace.db
  SPDLOG_DEBUG("Dumping CFG to {}", filename);
  std::ofstream ofs(filename);
  // Dump assembly line -> file, line mapping
  ofs << fmt::format("[meta] [module {}] [num-fn {}]\n",
                     km->module->getName().str(), km->functions.size());
  // Create table for file -> id
  // const char *createFileTable = "CREATE TABLE IF NOT EXISTS file ("
  //                               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
  //                               "name TEXT NOT NULL UNIQUE"
  //                               ");";
  // if (sqlite3_exec(klee_bb_trace_db, createFileTable, NULL, NULL, NULL) !=
  //     SQLITE_OK) {
  //   SPDLOG_ERROR("Failed to create file table: {}",
  //                sqlite3_errmsg(klee::klee_bb_trace_db));
  //   return;
  // }
  // // Create table for function -> id
  // const char *createFnTable =
  //     "CREATE TABLE IF NOT EXISTS function ("
  //     "id INTEGER PRIMARY KEY AUTOINCREMENT,"
  //     "name TEXT NOT NULL UNIQUE,"
  //     "file INTEGER NOT NULL,"
  //     "bb_size INTEGER NOT NULL,"
  //     "CONSTRAINT fk_file FOREIGN KEY (file) REFERENCES file(id)"
  //     ");";
  // if (sqlite3_exec(klee_bb_trace_db, createFnTable, NULL, NULL, NULL) !=
  //     SQLITE_OK) {
  //   SPDLOG_ERROR("Failed to create function table: {}",
  //                sqlite3_errmsg(klee::klee_bb_trace_db));
  //   return;
  // }
  // // Create table for basic block -> id
  // const char *createBbTable =
  //     "CREATE TABLE IF NOT EXISTS basic_block ("
  //     "id INTEGER PRIMARY KEY,"
  //     "function INTEGER NOT NULL,"
  //     "start INTEGER NOT NULL,"
  //     "end INTEGER NOT NULL,"
  //     "size INTEGER NOT NULL,"
  //     "CONSTRAINT fk_function FOREIGN KEY (function) REFERENCES function(id)"
  //     ");";
  // if (sqlite3_exec(klee_bb_trace_db, createBbTable, NULL, NULL, NULL) !=
  //     SQLITE_OK) {
  //   SPDLOG_ERROR("Failed to create basic block table: {}",
  //                sqlite3_errmsg(klee::klee_bb_trace_db));
  //   return;
  // }
  // // Create table for assembly line -> file (id), line, function (id)
  // const char *createAssmLineTable =
  //     "CREATE TABLE IF NOT EXISTS assembly_line ("
  //     "assembly INTEGER PRIMARY KEY,"
  //     "bb INTEGER NOT NULL,"
  //     "line INTEGER NOT NULL,"
  //     "CONSTRAINT fk_bb FOREIGN KEY (bb) REFERENCES basic_block(id)"
  //     ");";
  // if (sqlite3_exec(klee_bb_trace_db, createAssmLineTable, NULL, NULL, NULL)
  // !=
  //     SQLITE_OK) {
  //   SPDLOG_ERROR("Failed to create assembly line table: {}",
  //                sqlite3_errmsg(klee::klee_bb_trace_db));
  //   return;
  // }
  // sqlite3_exec(klee_bb_trace_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  // Start module analysis
  uint32_t global_bbId = 0;
  for (auto &kfp : km->functions) {
    std::string file_name = kfp->instructions[0]->info->file;
    file_name = file_name.empty() ? "no-debug-info" : file_name;
    ofs << fmt::format("[fn] [name {}] [file {}] [bb-size {}]\n",
                       kfp->function->getName().str(), file_name,
                       kfp->basicBlockEntry.size());
    ofs << fmt::format("[fn-start] [name {}]\n",
                       kfp->function->getName().str());
    // Insert file, function into db: if file is not in file table, insert
    // std::string insertFile = fmt::format(
    //     "INSERT OR IGNORE INTO file (name) VALUES ('{}');", file_name);
    // if (sqlite3_exec(klee_bb_trace_db, insertFile.c_str(), NULL, NULL, NULL)
    // !=
    //     SQLITE_OK) {
    //   SPDLOG_WARN("Failed to insert file: {}",
    //               sqlite3_errmsg(klee::klee_bb_trace_db));
    //   continue;
    // }
    // Insert function; Get file id by querying file table
    // std::string insertFn = fmt::format(
    //     "INSERT OR IGNORE INTO function (name, file, bb_size) "
    //     "VALUES ('{}', (SELECT id FROM file WHERE name = '{}'), {});",
    //     kfp->function->getName().str(), kfp->instructions[0]->info->file,
    //     kfp->basicBlockEntry.size());
    // if (sqlite3_exec(klee_bb_trace_db, insertFn.c_str(), NULL, NULL, NULL) !=
    //     SQLITE_OK) {
    //   SPDLOG_WARN("Failed to insert function: {}",
    //               sqlite3_errmsg(klee::klee_bb_trace_db));
    //   continue;
    // }
    for (auto &bbp : kfp->basicBlockEntry) {
      global_bbId++;
      const llvm::BasicBlock *bb = bbp.first;
      uint32_t bbId = bbp.second;
      uint32_t start = kfp->instructions[bbId]->info->assemblyLine;
      uint32_t size = bb->size();
      uint32_t line = kfp->instructions[bbId]->info->line;
      ofs << fmt::format("  [bb] [id {}] [start {}] [size {}] [line {}]\n",
                         bbId, start, size, line);
      // Insert basic block
      // std::string insertBb = fmt::format(
      //     "INSERT OR IGNORE INTO basic_block (id, function, start, end, size)
      //     " "VALUES ({}, (SELECT id FROM function WHERE name = '{}'), {}, {},
      //     "
      //     "{});",
      //     global_bbId, kfp->function->getName().str(), start, start + size -
      //     1, size);
      // if (sqlite3_exec(klee_bb_trace_db, insertBb.c_str(), NULL, NULL, NULL)
      // !=
      //     SQLITE_OK) {
      //   SPDLOG_WARN("Failed to insert basic block: {}",
      //               sqlite3_errmsg(klee::klee_bb_trace_db));
      //   continue;
      // }
      // Insert assembly line
      for (int i = 0; i < size; i++) {
        KInstruction *ki = kfp->instructions[bbId + i];
        uint32_t assemblyLine = ki->info->assemblyLine;
        uint32_t line = ki->info->line;
        uint32_t opcode = ki->inst->getOpcode();
        // std::string assemblyLineStr = fmt::format(
        //     "INSERT OR IGNORE INTO assembly_line (assembly, bb, line) "
        //     "VALUES ({}, {}, {});",
        //     assemblyLine, global_bbId, line);
        // if (sqlite3_exec(klee_bb_trace_db, assemblyLineStr.c_str(), NULL,
        // NULL,
        //                  NULL) != SQLITE_OK) {
        //   SPDLOG_WARN("Failed to insert assembly line: {}",
        //               sqlite3_errmsg(klee::klee_bb_trace_db));
        // }
      }
    }
    ofs << fmt::format("[fn-end] [name {}]\n", kfp->function->getName().str());
  }
  ofs.close();
  // Index by function name
  // const char *createFnIndex =
  //     "CREATE UNIQUE INDEX IF NOT EXISTS fn_index ON function (name);";
  // if (sqlite3_exec(klee_bb_trace_db, createFnIndex, NULL, NULL, NULL) !=
  //     SQLITE_OK) {
  //   SPDLOG_ERROR("Failed to create function index: {}",
  //                sqlite3_errmsg(klee::klee_bb_trace_db));
  //   return;
  // }
  // sqlite3_exec(klee_bb_trace_db, "END TRANSACTION;", NULL, NULL, NULL);
  SPDLOG_DEBUG("Dumped CFG to bb-trace.db and {}", filename);
}

CFGNode::CFGNode(const llvm::Function *parent, const llvm::BasicBlock *v,
                 int id)
    : parent(parent), v(v), type(CFGNodeType::Unknown), id(id), dead(false) {
  name = v->getName().str();
}

void CFGNode::addChild(const llvm::Instruction *v) { children.push_back(v); }
void CFGNode::addPred(CFGNode *v) { pred.push_back(v); }
void CFGNode::addSucc(CFGNode *v) { succ.push_back(v); }
CFGNode::~CFGNode() {}

ControlFlowGraph::ControlFlowGraph(const KFunction *kf) : kf(kf) {
  f = kf->function;
  build();
}

ControlFlowGraph::~ControlFlowGraph() {
  for (auto &it : cfg) {
    delete it.second;
  }
}

void ControlFlowGraph::build() {
  // Initialize CFG: add nodes
  for (const llvm::BasicBlock &basicBlock : f->getBasicBlockList()) {
    CFGNode *node =
        new CFGNode(f, &basicBlock,
                    kf->basicBlockEntry.at((llvm::BasicBlock *)&basicBlock));
    for (const llvm::Instruction &instruction : basicBlock.getInstList()) {
      node->addChild(&instruction);
    }
    cfg[&basicBlock] = node;
  }
  // Add edges
  for (const llvm::BasicBlock &basicBlock : f->getBasicBlockList()) {
    CFGNode *node = cfg[&basicBlock];
    if (checkExitNode(&basicBlock)) {
      node->type = CFGNodeType::Optional;
      SPDLOG_DEBUG("{}({}) is exit node", node->name, node->id);
      continue;
    }
    for (const llvm::BasicBlock *s : llvm::successors(&basicBlock)) {
      CFGNode *succ = cfg[s];
      node->addSucc(succ);
      succ->addPred(node);
      SPDLOG_DEBUG("{}({}) -> {}({})", node->name, node->id, succ->name,
                   succ->id);
    }
  }
}

void ControlFlowGraph::markTargetKInst(KInstruction *kinst) {
  CFGNode *node = cfg[kinst->inst->getParent()];
  node->type = CFGNodeType::Target;
  targetKInst = kinst;
  targetInst = kinst->inst;
  for (const llvm::Instruction &inst : node->v->getInstList()) {
    if (inst.isTerminator()) {
      patchBranchInst = &inst;
    }
  }
}

/**
 * Mark basic blocks that are target instruction is reachable.
 * Currently, only check if basic block is predecessor of target instruction.
 */
void ControlFlowGraph::markBasicBlocks() {
  // Initialize the worklist with the target instruction
  std::queue<CFGNode *> worklist;
  worklist.push(cfg[targetInst->getParent()]);

  while (!worklist.empty()) {
    CFGNode *currentNode = worklist.front();
    worklist.pop();
    SPDLOG_DEBUG("Marking {}({}) as required: pred {} succ {}",
                 currentNode->name, currentNode->id, currentNode->pred.size(),
                 currentNode->succ.size());

    // If the basic block of the current instruction is not marked yet,
    // mark it as "Required" since it's reachable from the target instruction
    if (currentNode->type != CFGNodeType::Required) {
      currentNode->type = CFGNodeType::Required;
      currentNode->markDead(false);
      // Add predecessors of the current instruction to the worklist
      for (CFGNode *predNode : currentNode->pred) {
        const llvm::BasicBlock *predBB = predNode->v;
        CFGNode *predCFGNode = cfg[predBB];
        worklist.push(predCFGNode);
      }
    }
  }

  // Now, mark all other basic blocks that are not marked as "Required" as
  // "Optional"
  for (auto &it : cfg) {
    CFGNode *node = it.second;
    if (node->type == CFGNodeType::Unknown ||
        node->type == CFGNodeType::Optional) {
      node->type = CFGNodeType::Optional;
      node->markDead(true);
      SPDLOG_DEBUG("Marking {}({}) as dead", node->name, node->id);
    } else {
      SPDLOG_DEBUG("Skip {}({}) - type {}/{}", node->name, node->id,
                   static_cast<int>(node->type), node->dead);
    }
  }
  // for (const llvm::BasicBlock &basicBlock : f->getBasicBlockList()) {
  //   CFGNode *node = cfg[&basicBlock];
  //   const llvm::Instruction *firstInst =
  //       basicBlock.getFirstNonPHIOrDbgOrLifetime();
  //   if (node->type == CFGNodeType::Unknown) {
  //     node->type = CFGNodeType::Optional;
  //     node->markDead(true);
  //     SPDLOG_DEBUG("Marking {}({}) as dead", node->name, node->id);
  //   } else {
  //     SPDLOG_DEBUG("Skip {}({}) - type {}/{}", node->name, node->id,
  //                  static_cast<int>(node->type), node->dead);
  //   }
  // }
}

void ControlFlowGraph::addBuggyTrace(const std::vector<std::string> &trace) {
  SPDLOG_INFO("Adding buggy trace len {}", trace.size());
  std::map<uint32_t, CFGNode *> locStrToNode;
  // std::string filename = kf->instructions[0]->info->file;
  // uint32_t startLine = kf->instructions[0]->info->line;
  // uint32_t endLine = kf->instructions[0]->info->line;
  for (int i = 0; i < kf->numInstructions; i++) {
    KInstruction *ki = kf->instructions[i];
    uint32_t assmLine = ki->info->assemblyLine;
    locStrToNode[assmLine] = cfg[ki->inst->getParent()];
    // uint32_t line = ki->info->line;
    // if (line > endLine) {
    //   endLine = line;
    // } else if (line < startLine) {
    //   startLine = line;
    // }
  }
  // Iterate over trace and add nodes to buggyTrace
  CFGNode *prev = nullptr;
  for (auto &loc : trace) {
    // Get assembly line from string
    llvm::StringRef locRef(loc);
    auto locParts = locRef.rsplit(':');
    uint32_t assmLine = std::stoi(locParts.second.str());
    if (locStrToNode.find(assmLine) != locStrToNode.end()) {
      CFGNode *node = locStrToNode[assmLine];
      if (node != prev) {
        buggyTrace.push_back(node);
        prev = node;
      }
    }
  }
}

bool ControlFlowGraph::checkExitNode(const llvm::BasicBlock *bb) {
  return llvm::succ_begin(bb) == llvm::succ_end(bb);
}

bool ControlFlowGraph::checkDeadNode(const llvm::BasicBlock *bb) const {
  if (!UniKleeReplay.empty())
    return false;
  if (cfg.find(bb) == cfg.end()) {
    // This happens if there is a function call inside of target function
    // SPDLOG_DEBUG("Basic block {} not found in CFG", bb->getName().str());
    return false;
  }
  return cfg.at(bb)->dead;
}

KInstruction *ControlFlowGraph::findTargetInst(std::string func) const {
  for (unsigned i = 0; i < kf->numInstructions; i++) {
    KInstruction *ki = kf->instructions[i];
    if (ki->inst->getOpcode() == llvm::Instruction::Call) {
      const llvm::CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(ki->inst);
      const llvm::Function *calledFunc = callInst->getCalledFunction();
      if (calledFunc && calledFunc->getName().str() == func) {
        SPDLOG_INFO("Found target instruction at {}/{}",
                    ki->inst->getParent()->getName().str(), func);
        return ki;
      }
    }
  }
  return nullptr;
}

} // namespace klee