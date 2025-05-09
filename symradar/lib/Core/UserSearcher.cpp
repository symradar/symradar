//===-- UserSearcher.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UserSearcher.h"

#include "Searcher.h"
#include "Executor.h"

#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/SolverCmdLine.h"
#include "klee/MergeHandler.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace klee;

namespace {
cl::list<Searcher::CoreSearchType> CoreSearch(
    "search",
    cl::desc("Specify the search heuristic (default=random-path interleaved "
             "with nurs:covnew)"),
    cl::values(
        clEnumValN(Searcher::DFS, "dfs", "use Depth First Search (DFS)"),
        clEnumValN(Searcher::BFS, "bfs",
                   "use Breadth First Search (BFS), where scheduling decisions "
                   "are taken at the level of (2-way) forks"),
        clEnumValN(Searcher::RandomState, "random-state",
                   "randomly select a state to explore"),
        clEnumValN(Searcher::RandomPath, "random-path",
                   "use Random Path Selection (see OSDI'08 paper)"),
        clEnumValN(Searcher::NURS_CovNew, "nurs:covnew",
                   "use Non Uniform Random Search (NURS) with Coverage-New"),
        clEnumValN(Searcher::NURS_MD2U, "nurs:md2u",
                   "use NURS with Min-Dist-to-Uncovered"),
        clEnumValN(Searcher::NURS_Depth, "nurs:depth", "use NURS with 2^depth"),
        clEnumValN(Searcher::NURS_ICnt, "nurs:icnt",
                   "use NURS with Instr-Count"),
        clEnumValN(Searcher::NURS_CPICnt, "nurs:cpicnt",
                   "use NURS with CallPath-Instr-Count"),
        clEnumValN(Searcher::NURS_QC, "nurs:qc", "use NURS with Query-Cost"),
        clEnumValN(Searcher::UniKLEE, "uni-klee", "use UniKLEE search")
            KLEE_LLVM_CL_VAL_END));

cl::opt<bool>
    UseIterativeDeepeningTimeSearch("use-iterative-deepening-time-search",
                                    cl::desc("(experimental)"));

cl::opt<bool> UseBatchingSearch(
    "use-batching-search",
    cl::desc("Use batching searcher (keep running selected state for N "
             "instructions/time, see --batch-instructions and --batch-time)"),
    cl::init(false));

cl::opt<unsigned> BatchInstructions(
    "batch-instructions",
    cl::desc(
        "Number of instructions to batch when using --use-batching-search"),
    cl::init(10000));

cl::opt<std::string> BatchTime("batch-time",
                               cl::desc("Amount of time to batch when using "
                                        "--use-batching-search (default=5s)"),
                               cl::init("5s"));

} // namespace

void klee::initializeSearchOptions() {
  // default values
  if (CoreSearch.empty()) {
    if (UseMerge) {
      CoreSearch.push_back(Searcher::NURS_CovNew);
      klee_warning(
          "--use-merge enabled. Using NURS_CovNew as default searcher.");
    } else {
      CoreSearch.push_back(Searcher::RandomPath);
      CoreSearch.push_back(Searcher::NURS_CovNew);
      // CoreSearch.push_back(Searcher::UniKLEE);
    }
  }
}

bool klee::userSearcherRequiresMD2U() {
  return (std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_MD2U) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_CovNew) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_ICnt) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_CPICnt) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(), Searcher::NURS_QC) !=
              CoreSearch.end());
}

Searcher *getNewSearcher(Searcher::CoreSearchType type, Executor &executor) {
  Searcher *searcher = NULL;
  switch (type) {
  case Searcher::DFS:
    searcher = new DFSSearcher();
    break;
  case Searcher::BFS:
    searcher = new BFSSearcher();
    break;
  case Searcher::RandomState:
    searcher = new RandomSearcher();
    break;
  case Searcher::RandomPath:
    searcher = new RandomPathSearcher(executor);
    break;
  case Searcher::NURS_CovNew:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::CoveringNew);
    break;
  case Searcher::NURS_MD2U:
    searcher =
        new WeightedRandomSearcher(WeightedRandomSearcher::MinDistToUncovered);
    break;
  case Searcher::NURS_Depth:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::Depth);
    break;
  case Searcher::NURS_ICnt:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::InstCount);
    break;
  case Searcher::NURS_CPICnt:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::CPInstCount);
    break;
  case Searcher::NURS_QC:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::QueryCost);
    break;
  case Searcher::UniKLEE:
    searcher = new UniKLEESearcher(executor);
    break;
  }

  return searcher;
}

Searcher *klee::constructUserSearcher(Executor &executor) {

  Searcher *searcher = getNewSearcher(CoreSearch[0], executor);

  if (CoreSearch.size() > 1) {
    std::vector<Searcher *> s;
    s.push_back(searcher);

    for (unsigned i = 1; i < CoreSearch.size(); i++)
      s.push_back(getNewSearcher(CoreSearch[i], executor));

    searcher = new InterleavedSearcher(s);
  }

  if (UseMerge) {
    if (std::find(CoreSearch.begin(), CoreSearch.end(), Searcher::RandomPath) !=
        CoreSearch.end()) {
      klee_error("use-merge currently does not support random-path, please use "
                 "another search strategy");
    }
  }

  if (UseBatchingSearch) {
    searcher = new BatchingSearcher(searcher, time::Span(BatchTime),
                                    BatchInstructions);
  }

  if (UseMerge && UseIncompleteMerge) {
    searcher = new MergingSearcher(executor, searcher);
  }

  if (UseIterativeDeepeningTimeSearch) {
    searcher = new IterativeDeepeningTimeSearcher(searcher);
  }

  llvm::raw_ostream &os = executor.getHandler().getInfoStream();

  os << "BEGIN searcher description\n";
  searcher->printName(os);
  os << "END searcher description\n";

  return searcher;
}
