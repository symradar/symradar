#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iomanip>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

namespace {
struct GlobalVariablePass : public ModulePass {
  static char ID;
  GlobalVariablePass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    LLVMContext &Context = M.getContext();
    IRBuilder<> Builder(Context);

    Function *Fprintf = M.getFunction("fprintf");
    if (!Fprintf) {
      errs() << "Could not find existing declaration of fprintf.\n";
      return false;
    }

    Function *GetEnv = M.getFunction("getenv");
    if (!GetEnv) {
      errs() << "Could not find existing declaration of getenv"
             << "\n";
      return false;
    }
    Function *Fopen = M.getFunction("fopen");
    if (!Fopen) {
      errs() << "Could not find existing declaration of fopen"
             << "\n";
      return false;
    }
    Function *Fclose = M.getFunction("fclose");

    if (!Fclose) {
      errs() << "Could not find existing declaration of fclose"
             << "\n";
      return false;
    }

    // Declare the uni_klee_hex_string function
    FunctionType *funcType =
        FunctionType::get(PointerType::getUnqual(Type::getInt8Ty(Context)),
                          {PointerType::getUnqual(Type::getInt8Ty(Context)),
                           Type::getInt64Ty(Context)},
                          false);
    Function *uniKleeHexStringFunc = M.getFunction("uni_klee_hex_string");

    if (!uniKleeHexStringFunc) {
      errs() << "Function uni_klee_hex_string not found.\n";
      return false;
    }

    // Read environment variable UNI_KLEE_SYMBOLIC_GLOBALS_FILE
    const char *EnvVarName = "UNI_KLEE_SYMBOLIC_GLOBALS_FILE";
    const char *GlobalsFilePath = std::getenv(EnvVarName);
    if (!GlobalsFilePath) {
      errs() << "Environment variable " << EnvVarName << " is not set.\n";
      return false;
    }

    // Read target variables from the specified file
    std::ifstream GlobalsFile(GlobalsFilePath);
    if (!GlobalsFile.is_open()) {
      errs() << "Failed to open globals file: " << GlobalsFilePath << "\n";
      return false;
    }

    std::map<std::string, std::pair<std::string, int>> TargetVariables;
    std::string Line;
    std::regex lineRegex(
        R"(\[size\s+(\d+)\]\s+\[array\s+([\w\.]+)\]\s+\[name\s+([\w\.]+)\])");
    while (std::getline(GlobalsFile, Line)) {
      if (!Line.empty()) {
        std::smatch match;
        if (std::regex_search(Line, match, lineRegex) && match.size() > 3) {
          TargetVariables[match.str(3)] = {match.str(2),
                                           std::stoi(match.str(1))};
          errs() << "Target variable: " << match.str(2) << "="
                 << match.str(3) << ", size = " << match.str(1) << "\n";
        }
      }
    }
    GlobalsFile.close();

    bool Modified = false;

    // Find the "uni_klee_heap_check" function
    Function *HeapCheckFunc = M.getFunction("uni_klee_heap_check");
    if (!HeapCheckFunc) {
      errs() << "Function uni_klee_heap_check not found.\n";
      return false;
    }

    // Insert instrumentation at the beginning of "uni_klee_heap_check"
    BasicBlock &EntryBlock = HeapCheckFunc->getEntryBlock();
    Builder.SetInsertPoint(&EntryBlock, EntryBlock.begin());

    DILocation *FuncLoc = nullptr;
    if (auto *SP = HeapCheckFunc->getSubprogram()) {
       FuncLoc = DILocation::get(Context, SP->getLine(), 0, SP);
    }

    for (GlobalVariable &GV : M.globals()) {
      if (!GV.hasName())
        continue;

      std::string VarName = GV.getName().str();

      if (TargetVariables.count(VarName) > 0) {
        errs() << "Found target variable: " << VarName << "\n";
        std::string symName = TargetVariables[VarName].first;
        int size = TargetVariables[VarName].second;

        // Set debug location for inserted instructions
        if (FuncLoc) {
          errs() << "Setting debug location for " << VarName << "\n";
          Builder.SetCurrentDebugLocation(FuncLoc);
        }

        // getenv("UNI_KLEE_MEM_RESULT")
        Value *EnvVar = Builder.CreateGlobalStringPtr("UNI_KLEE_MEM_RESULT");
        Value *FilePath = Builder.CreateCall(GetEnv, {EnvVar});

        // fopen(result_file, "a")
        Value *AppendMode = Builder.CreateGlobalStringPtr("a");
        Value *FileHandle = Builder.CreateCall(Fopen, {FilePath, AppendMode});

        // Prepare format string and variable name string
        Value *FormatStr =
            Builder.CreateGlobalStringPtr("[global] [sym %s] [value %s]\n");
        Value *VarNameStr = Builder.CreateGlobalStringPtr(symName);

        // Load the variable value
        Value *VarPtr = Builder.CreatePointerCast(
            &GV, PointerType::getUnqual(Type::getInt8Ty(Context)));

        // Get size as an LLVM Value
        Value *SizeValue = ConstantInt::get(Type::getInt64Ty(Context), size);

        // Call uni_klee_hex_string to get hex string
        Value *HexString =
            Builder.CreateCall(uniKleeHexStringFunc, {VarPtr, SizeValue});

        // Output the hex string
        Builder.CreateCall(Fprintf,
                           {FileHandle, FormatStr, VarNameStr, HexString});

        // fclose(f)
        Builder.CreateCall(Fclose, {FileHandle});
        Builder.SetCurrentDebugLocation(nullptr);

        Modified = true;
      }
    }
    return Modified;
  }
};
} // namespace

// opt -load lib/UniKleeGlobalVariablePass.so -global-var-pass < test.bc > out.bc
char GlobalVariablePass::ID = 0;
static RegisterPass<GlobalVariablePass> X("global-var-pass",
                                          "Track Global Variables Pass");