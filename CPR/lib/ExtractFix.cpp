/* #########################################################################
This file is part of crash-free-fix.
Copyright (C) 2016

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################*/


#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>

#include <cxxabi.h>

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SpecialCaseList.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MD5.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/Local.h"
#include "/root/projects/uni-klee/include/external/json/json.h"

#include "MetadataExtractor.h"

using namespace llvm;
using namespace std;


static cl::opt<string> functionCallList("lf",
                                        cl::desc("Specify the list of executed function by failing test"),
                                        cl::value_desc("F1 F2 ... Fn"));

static cl::opt<string> targetFunction("fun",
                                      cl::desc("Specify the function where the crash is triggered"),
                                      cl::value_desc("FUNCTION_NAME"));

static cl::opt<int> targetNO("no",
                             cl::desc("Specify the crash line number"),
                             cl::value_desc("LINE_NUMBER"));

static cl::opt<string> crashRelatedVars("cv",
                                        cl::desc("Specify the variables that are related to the crash"),
                                        cl::value_desc("V1 V2 ... Vn"));

static cl::opt<bool> verbose("v",
                             cl::desc("debug mode"));

struct SeenEntry{
    bool forward;
    Value *X;

    bool operator<(const SeenEntry &Entry) const
    {
        if (Entry.forward < forward)
            return true;
        if (Entry.forward > forward)
            return false;
        return (Entry.X < X);
    }
};

struct FixEntry{
    string description;
    string filename;
    string funcName;
    int lineNo;
    mutable vector<struct variable> varsToSymbolize;

    bool operator==(const FixEntry &fixEntry){
        return funcName == fixEntry.funcName && lineNo == fixEntry.lineNo;
    }

    bool operator<(const FixEntry &fixEntry) const
    {
        if (funcName == fixEntry.funcName)
            return lineNo > fixEntry.lineNo;
        return funcName < fixEntry.funcName;
    }
};

// all the potential fix locations
static set<FixEntry> allPotentialFixLocs;

// the argument list that that should be propagated back to caller
static vector<int> argumentsForBackwardAnalysis;

/* check whether val is an argument of func */
bool isFuncArgument(Function *func, Value * val){
    int index = 0;
    for(auto arg = func->arg_begin(); arg != func->arg_end(); ++arg, index++) {
        if (&(*arg) == &(*val)){
            if (find(argumentsForBackwardAnalysis, index) == argumentsForBackwardAnalysis.end())
                argumentsForBackwardAnalysis.push_back(index);
            return true;
        }
    }
    return false;
}

static void findFixLocsDataFlow(const DominatorTree &DT, std::set<SeenEntry> &Seen, set<FixEntry> &pFixLocs,
                                Value *X, Instruction *Dst, Function* funcName);

static void recordFixLoc(const char* message, const string funcName,
                         const Instruction* inst, set<FixEntry> &pFixLocs){
    const DebugLoc debugLoc = inst->getDebugLoc();
    // the instruction does not contain debug location
    if (!debugLoc){
        fprintf(stderr, "debug information is not found.");;
    }
    int lineNo = debugLoc.getLine();

    const DILocation *test =debugLoc.get();
    string filename = test->getFilename();

    FixEntry Entry = {message, filename, funcName, lineNo};
    auto i = pFixLocs.find(Entry);
    if (i != pFixLocs.end())
        return;
    pFixLocs.insert(Entry);

    // fprintf(stderr, "%s", message);
    // inst->print(errs());
    // fprintf(stderr, "line no: %d\n", lineNo);
}

/*
 * Suggest fix locations for X.
 *
 * Does a forward data-flow analysis.
 */
static void findFixLocsForward(const DominatorTree &DT, std::set<SeenEntry> &Seen, set<FixEntry> &pFixLocs,
                               Value *X, Instruction *Dst, Function * func)
{
    if (isFuncArgument(func, X)){
        if (verbose)
            fprintf(stderr, "PROPAGATE ARGUMENT\n");
    }

    SeenEntry Entry = {/*forward=*/true, X};
    auto i = Seen.find(Entry);
    if (i != Seen.end())
        return;
    Seen.insert(Entry);

    if (verbose) {
        fprintf(stderr, "\t\tFORWARD ");
        X->print(errs()); fprintf(stderr, "\n");
    }

    if (auto *Cmp = dyn_cast<ICmpInst>(X))
    {
        // We have found a comparison that may effect control flow.
        if (!DT.dominates(Cmp, Dst))
        {
            return;         // Does not dominate
        }
        for (auto *User: Cmp->users())
        {
            auto *Br = dyn_cast<BranchInst>(User);
            if (Br == nullptr)
                continue;   // Not a branch
            if (!Br->isConditional() || Br->getCondition() != Cmp)
                continue;   // Not conditional.

            recordFixLoc("control flow", func->getName(), Cmp, pFixLocs);

            findFixLocsDataFlow(DT, Seen, pFixLocs, Cmp->getOperand(0), Dst, func);
            findFixLocsDataFlow(DT, Seen, pFixLocs, Cmp->getOperand(1), Dst, func);
            break;
        }
        return;
    }
    else if(auto *store = dyn_cast<StoreInst>(X)){ //store instruction may affect the value of memory
        if (!DT.dominates(store, Dst))
        {
            return;         // Does not dominate
        }
        findFixLocsDataFlow(DT, Seen, pFixLocs, store->getOperand(0), Dst, func);
        findFixLocsDataFlow(DT, Seen, pFixLocs, store->getOperand(1), Dst, func);
    }

    // This is a forward analysis, so we look at all "users" of X.
    for (auto *User: X->users())
    {
        if (isa<GetElementPtrInst>(User) ||
            isa<CastInst>(User) ||
            isa<PHINode>(User) ||
            isa<BinaryOperator>(User) ||
            isa<ICmpInst>(User) ||
            isa<LoadInst>(User) ||
            isa<StoreInst>(User))
        {
            findFixLocsForward(DT, Seen, pFixLocs, User, Dst, func);
        }
    }
}

/*
 * Suggest fix locations for X.
 *
 * Does a backwards data-flow analysis.
 */
static void findFixLocsDataFlow(const DominatorTree &DT, std::set<SeenEntry> &Seen, set<FixEntry> &pFixLocs,
                                Value *X, Instruction *Dst, Function * func)
{
    if (isFuncArgument(func, X)){
        if (verbose)
            fprintf(stderr, "PROPAGATE ARGUMENT\n");
        return;
    }

    SeenEntry Entry = {/*forward=*/false, X};
    auto i = Seen.find(Entry);
    if (i != Seen.end())
        return;
    Seen.insert(Entry);

    findFixLocsForward(DT, Seen, pFixLocs, X, Dst, func);

    // Only care about instructions:
    if (!isa<Instruction>(X))
        return;

    if (verbose) {
        fprintf(stderr, "\t\tBACKWARD ");
        X->print(errs());
        fprintf(stderr, "\n");
    }

    // Find control-flow locations:

    // Data-flow patching:
    if (auto *GEP = dyn_cast<GetElementPtrInst>(X))
    {
        // Pointer arithmetic: ptr = ptr + k;
        if (DT.dominates(GEP, Dst))
        {
            recordFixLoc("data flow", func->getName(), GEP, pFixLocs);
        }

        int numIdxs = GEP->getNumIndices();
        for (unsigned int j = 0; j < numIdxs; j++)
        {
            Value *Idx = GEP->getOperand(j+1);
            findFixLocsDataFlow(DT, Seen, pFixLocs, Idx, Dst, func);
        }
        findFixLocsDataFlow(DT, Seen, pFixLocs, GEP->getPointerOperand(), Dst, func);
    }
    else if (auto *BinOp = dyn_cast<BinaryOperator>(X))
    {
        if (DT.dominates(BinOp, Dst))
        {
            recordFixLoc("data flow", func->getName(), BinOp, pFixLocs);
        }
        findFixLocsDataFlow(DT, Seen, pFixLocs, BinOp->getOperand(0), Dst, func);
        findFixLocsDataFlow(DT, Seen, pFixLocs, BinOp->getOperand(1), Dst, func);
    }
    else if (auto *Cast = dyn_cast<CastInst>(X))
    {
        findFixLocsDataFlow(DT, Seen, pFixLocs, Cast->getOperand(0), Dst, func);
    }
    else if (auto *PHI = dyn_cast<PHINode>(X))
    {
        // PHI-nodes: attempt to find fix locations along all paths.
        //
        // Note: Perhaps a better approach is to restrict the analysis to the
        // path of the failing test case.
        size_t numValues = PHI->getNumIncomingValues();
        for (size_t j = 0; j < numValues; j++)
            findFixLocsDataFlow(DT, Seen, pFixLocs, PHI->getIncomingValue(j), Dst, func);
    }
    else if(auto *load = dyn_cast<LoadInst>(X)){
        findFixLocsDataFlow(DT, Seen, pFixLocs, load->getOperand(0), Dst, func);
    }
    else if(auto *store = dyn_cast<StoreInst>(X)){
        // X->print(errs());
        findFixLocsDataFlow(DT, Seen, pFixLocs, store->getOperand(0), Dst, func);
        findFixLocsDataFlow(DT, Seen, pFixLocs, store->getOperand(1), Dst, func);
    }
    else
    {
        // Not yet implemented!
        if (verbose){
            fprintf(stderr, "\t\t\33[33mSTOP\33[0m [not yet implemented] \n");
            X->print(errs()); fprintf(stderr, "\n");
        }
    }
}

//static void suggestFixLocs(const DominatorTree &DT, Instruction *inst, Function *func)
//{
//    fprintf(stderr, "\n-------------------------------------------------------\n");
//    fprintf(stderr, "\t\33[31mSTORE\33[0m ");
//    inst->print(errs()); fprintf(stderr, "\n");
//    std::set<SeenEntry> Seen;
//    findFixLocsDataFlow(DT, Seen, inst, inst, func);
//}

set<FixEntry> determineVarsToSymbolize(std::set<SeenEntry> seen, map<Value*, struct variable> value2Meta,
        set<FixEntry> &pFixLocs){
    for (map<Value*, struct variable>::iterator it=value2Meta.begin(); it!=value2Meta.end(); ++it){
        SeenEntry entry = {/*forward=*/false, it->first};
        if (seen.find(entry)==seen.end()){
            value2Meta.erase(it->first);
        }
    }

    // TODO: check crash function
    vector<string> vars;
    size_t pos = 0;
    std::string token;
    while ((pos = crashRelatedVars.find(" ")) != std::string::npos) {
        token = crashRelatedVars.substr(0, pos);
        vars.push_back(token);
        crashRelatedVars.erase(0, pos + 1);
    }
    vars.push_back(crashRelatedVars);

    set<FixEntry> newPFixLocs;
    for (FixEntry fixEntry: pFixLocs){
        for(map<Value*, struct variable>::iterator it=value2Meta.begin(); it!=value2Meta.end(); ++it){
            if (it->second.declareLineNo < fixEntry.lineNo){

                // skip unsupported type
                Type* T = it->first->getType();
                if (PointerType* PT = dyn_cast<PointerType>(T)) {
                    if (!PT->getPointerElementType()->isIntegerTy()) {
                        continue;
                    }
                }

                if(std::find(vars.begin(), vars.end(), it->second.varName) != vars.end())
                    fixEntry.varsToSymbolize.push_back(it->second);
                else{
                    // the variable is used in the fix line
                    for (auto *User: it->first->users())
                    {
                        if (auto* u = dyn_cast<Instruction>(User)) {
                            const DebugLoc debugLoc = u->getDebugLoc();
                            if (!debugLoc) {
                                continue;
                            }
                            int lineNo = debugLoc.getLine();
                            if (lineNo == fixEntry.lineNo){
                                fixEntry.varsToSymbolize.push_back(it->second);
                                break;
                            }
                        }
                    }
                }
            }
        }
        for (int i=0; i < vars.size(); i++) {
            string var = vars[i];
            if (var.find("LOWFAT_GLOBAL_")!=string::npos){
                struct variable global_var = {var, 0, nullptr};
                fixEntry.varsToSymbolize.push_back(global_var);
            }
        }
        newPFixLocs.insert(fixEntry);
    }
    return newPFixLocs;
}

static void suggestFixLocs(Module &M)
{
    // parse function list
    vector<string> funcCalls;
    size_t pos = 0;
    std::string token;
    // TODO: check crash function
    while ((pos = functionCallList.find(" ")) != std::string::npos) {
        token = functionCallList.substr(0, pos);
        funcCalls.push_back(token);
        functionCallList.erase(0, pos + 1);
    }
    funcCalls.push_back(functionCallList);

    string callee, current_function;
    static bool isCrashFunction = true; // the first iteration will explore crash function
    for(long i=funcCalls.size()-1; i>=0; i--){
        current_function = funcCalls[i];

        Function *F = M.getFunction(current_function);

        std::map<Value*, struct variable> value2Meta = collect_local_variable_full_metadata(*F);
        //for (std::map<Value*, struct variable>::iterator it=aEvalue2Meta.begin(); it!=value2Meta.end(); ++it){
        //    errs() << *(it->first) << " => " << it->second.varName << " => " << it->second.declareLineNo << '\n';
        //}

        if (verbose)
            fprintf(stderr, "\nFound \33[32mFUNCTION\33[0m %s\n", F->getName().str().c_str());

        vector<int> argumentsForBackwardAnalysisOfCallee;

        DominatorTree DT(*F);
        std::set<SeenEntry> Seen;
        set<FixEntry> pFixLocs;
        for (auto &BB: *F)
        {
            for (auto &I: BB) {
                if (isCrashFunction){

                    if (auto *inst = dyn_cast<Instruction>(&I)) {

                        const DebugLoc debugLoc = inst->getDebugLoc();
                        // the instruction does not contain debug location
                        if (!debugLoc){
                            continue;
                        }
                        int lineNo = debugLoc.getLine();
                        if (lineNo != targetNO)
                            continue;

                        // inst->print(errs());
                        findFixLocsDataFlow(DT, Seen, pFixLocs, inst, inst, F);
                    }
                }
                else if(!isCrashFunction && isa<CallInst>(&I)){
                    auto *inst = dyn_cast<CallInst>(&I);
                    if (inst->getCalledFunction() && inst->getCalledFunction()->getName() == callee){

                        for(int argumentIndex: argumentsForBackwardAnalysisOfCallee){
                            if (verbose)
                                errs() << "the argument includes " << argumentIndex << "\n";
                            findFixLocsDataFlow(DT, Seen, pFixLocs, inst->getArgOperand(argumentIndex), inst, F);
                            // suggestFixLocs(DT, inst->getArgOperand(argumentIndex), F);
                        }
                        //for(SeenEntry seen: Seen)
                        //    errs() << "seen entry:" << *(seen.X) << "\n";
                        goto done;
                    }
                }
            }
        }
    done:
        isCrashFunction = false;
        pFixLocs = determineVarsToSymbolize(Seen, value2Meta, pFixLocs);
        allPotentialFixLocs.insert(pFixLocs.begin(), pFixLocs.end());

        callee = current_function;
        argumentsForBackwardAnalysisOfCallee = argumentsForBackwardAnalysis;
    }
}

static void writeToJsonFile(string &fileName, set<FixEntry> &fixLocs){
    Json::Value fixLocations;
    int id = 0;
    for (FixEntry fixEntry: fixLocs){
        Json::Value fixLocation;
        fixLocation[0] = fixEntry.description;
        fixLocation[1] = fixEntry.filename;
        fixLocation[2] = fixEntry.funcName;
        fixLocation[3] = fixEntry.lineNo;
        // inst->print(errs());

        Json::Value sysVars;
        if (fixEntry.varsToSymbolize.size() <= 0)
            continue;
        for (int i = 0; i<fixEntry.varsToSymbolize.size(); i++){
            sysVars.append(fixEntry.varsToSymbolize[i].varName);
        }
        fixLocation[4] = sysVars;
        fixLocations[id++] = fixLocation;
    }

    Json::StyledWriter styledWriter;
    string content = styledWriter.write(fixLocations);

    std::ofstream os;
    os.open (fileName);
    os << content << "\n";
    if (verbose)
        errs() << content << "\n";
    os.close();
}

/*
 * LLVM Module pass biolerplate.
 */

namespace
{
    struct FixLoc : public ModulePass
    {
        static char ID;

        FixLoc() : ModulePass(ID) {
        }

        virtual bool runOnModule(Module &M)
        {
            suggestFixLocs(M);
            string fileName = "/tmp/fixlocations.json";
            writeToJsonFile(fileName, allPotentialFixLocs);
            if (verbose)
                errs() << "Fix locations have been written to file \33[32m" << fileName << "\n";
            return true;
        }
    };

    char FixLoc::ID = 0;
}

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/CommandLine.h"


static RegisterPass<FixLoc> X("fixloc", "FixLoc pass");

static void register_pass(const PassManagerBuilder &PMB,
                          legacy::PassManagerBase &PM)
{
    PM.add(new FixLoc());
}

static RegisterStandardPasses RegisterPass(
        PassManagerBuilder::EP_LoopOptimizerEnd, register_pass);

