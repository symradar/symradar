//===-- SpecialFunctionHandler.cpp ----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SpecialFunctionHandler.h"
#include "TimingSolver.h"
#include "klee/ExecutionState.h"
#include "klee/Internal/System/Memory.h"
#include "klee/MergeHandler.h"
#include "klee/SlicingHandler.h"
#include "klee/util/ExprSMTLIBPrinter.h"

#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/Debug.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include "Executor.h"
#include "MemoryManager.h"

#include "klee/SolverCmdLine.h"

#include "llvm/IR/Module.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DataLayout.h"

#include "spdlog/spdlog.h"

#include <errno.h>
#include <iostream>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace {
cl::opt<bool>
    ReadablePosix("readable-posix-inputs", cl::init(false),
                  cl::desc("Prefer creation of POSIX inputs (command-line "
                           "arguments, files, etc.) with human readable bytes. "
                           "Note: option is expensive when creating lots of "
                           "tests (default=false)"));

cl::opt<bool>
    SilentKleeAssume("silent-klee-assume", cl::init(false),
                     cl::desc("Silently terminate paths with an infeasible "
                              "condition given to klee_assume() rather than "
                              "emitting an error (default=false)"));
} // namespace

/// \todo Almost all of the demands in this file should be replaced
/// with terminateState calls.

///

// FIXME: We are more or less committed to requiring an intrinsic
// library these days. We can move some of this stuff there,
// especially things like realloc which have complicated semantics
// w.r.t. forking. Among other things this makes delayed query
// dispatch easier to implement.
static SpecialFunctionHandler::HandlerInfo handlerInfo[] = {
#define add(name, handler, ret)                                                \
  { name, &SpecialFunctionHandler::handler, false, ret, false }
#define addDNR(name, handler)                                                  \
  { name, &SpecialFunctionHandler::handler, true, false, false }
    addDNR("__assert_rtn", handleAssertFail),
    addDNR("__assert_fail", handleAssertFail),
    addDNR("_assert", handleAssert),
    addDNR("abort", handleAbort),
    addDNR("_exit", handleExit),
    {"exit", &SpecialFunctionHandler::handleExit, true, false, true},
    addDNR("klee_abort", handleAbort),
    addDNR("klee_silent_exit", handleSilentExit),
    addDNR("klee_report_error", handleReportError),
    add("calloc", handleCalloc, true),
    add("free", handleFree, false),
    add("klee_assume", handleAssume, false),
    add("klee_check_memory_access", handleCheckMemoryAccess, false),
    add("klee_get_valuef", handleGetValue, true),
    add("klee_get_valued", handleGetValue, true),
    add("klee_get_valuel", handleGetValue, true),
    add("klee_get_valuell", handleGetValue, true),
    add("klee_get_value_i32", handleGetValue, true),
    add("klee_get_value_i64", handleGetValue, true),
    add("klee_define_fixed_object", handleDefineFixedObject, false),
    add("klee_get_obj_size", handleGetObjSize, true),
    add("klee_get_errno", handleGetErrno, true),
#ifndef __APPLE__
    add("__errno_location", handleErrnoLocation, true),
#else
    add("__error", handleErrnoLocation, true),
#endif
    add("klee_is_symbolic", handleIsSymbolic, true),
    add("klee_make_symbolic", handleMakeSymbolic, false),
    add("uni_klee_make_symbolic", handleUniMakeSymbolic, false),
    add("extractfix_make_symbolic", handleExtractFixMakeSymbolic, false),
    add("klee_sliced", handleSlicing, false),
    add("klee_mark_global", handleMarkGlobal, false),
    add("klee_open_merge", handleOpenMerge, false),
    add("klee_close_merge", handleCloseMerge, false),
    add("klee_prefer_cex", handlePreferCex, false),
    add("klee_posix_prefer_cex", handlePosixPreferCex, false),
    add("klee_print_expr", handlePrintExpr, false),
    add("klee_print_stmt", handlePrintStatement, false),
    add("klee_print_range", handlePrintRange, false),
    add("klee_set_forking", handleSetForking, false),
    add("klee_stack_trace", handleStackTrace, false),
    add("klee_warning", handleWarning, false),
    add("klee_warning_once", handleWarningOnce, false),
    add("klee_alias_function", handleAliasFunction, false),
    add("malloc", handleMalloc, true),
    add("realloc", handleRealloc, true),
    add("klee_select_patch", handleSelectPatch, false),
    add("uni_klee_add_patch", handleUniKleeAddPatch, false),
    // operator delete[](void*)
    add("_ZdaPv", handleDeleteArray, false),
    // operator delete(void*)
    add("_ZdlPv", handleDelete, false),

    // operator new[](unsigned int)
    add("_Znaj", handleNewArray, true),
    // operator new(unsigned int)
    add("_Znwj", handleNew, true),

    // FIXME-64: This is wrong for 64-bit long...

    // operator new[](unsigned long)
    add("_Znam", handleNewArray, true),
    // operator new(unsigned long)
    add("_Znwm", handleNew, true),

    // Run clang with -fsanitize=signed-integer-overflow and/or
    // -fsanitize=unsigned-integer-overflow
    add("__ubsan_handle_add_overflow", handleAddOverflow, false),
    add("__ubsan_handle_sub_overflow", handleSubOverflow, false),
    add("__ubsan_handle_mul_overflow", handleMulOverflow, false),
    add("__ubsan_handle_divrem_overflow", handleDivRemOverflow, false),
    add("__ubsan_handle_shift_out_of_bounds", handleShiftOverflow, false),

#undef addDNR
#undef add
};

SpecialFunctionHandler::const_iterator SpecialFunctionHandler::begin() {
  return SpecialFunctionHandler::const_iterator(handlerInfo);
}

SpecialFunctionHandler::const_iterator SpecialFunctionHandler::end() {
  // NULL pointer is sentinel
  return SpecialFunctionHandler::const_iterator(0);
}

SpecialFunctionHandler::const_iterator &
SpecialFunctionHandler::const_iterator::operator++() {
  ++index;
  if (index >= SpecialFunctionHandler::size()) {
    // Out of range, return .end()
    base = 0; // Sentinel
    index = 0;
  }

  return *this;
}

int SpecialFunctionHandler::size() {
  return sizeof(handlerInfo) / sizeof(handlerInfo[0]);
}

SpecialFunctionHandler::SpecialFunctionHandler(Executor &_executor)
    : executor(_executor) {}

void SpecialFunctionHandler::prepare(
    std::vector<const char *> &preservedFunctions) {
  unsigned N = size();

  for (unsigned i = 0; i < N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);
    // No need to create if the function doesn't exist, since it cannot
    // be called in that case.
    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      preservedFunctions.push_back(hi.name);
      // Make sure NoReturn attribute is set, for optimization and
      // coverage counting.
      if (hi.doesNotReturn)
        f->addFnAttr(Attribute::NoReturn);

      // Change to a declaration since we handle internally (simplifies
      // module and allows deleting dead code).
      if (!f->isDeclaration())
        f->deleteBody();
    }
  }
}

void SpecialFunctionHandler::bind() {
  unsigned N = sizeof(handlerInfo) / sizeof(handlerInfo[0]);

  for (unsigned i = 0; i < N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);

    if (f && (!hi.doNotOverride || f->isDeclaration()))
      handlers[f] = std::make_pair(hi.handler, hi.hasReturnValue);
  }
}

bool SpecialFunctionHandler::handle(ExecutionState &state, Function *f,
                                    KInstruction *target,
                                    std::vector<ref<Expr>> &arguments) {
  //  llvm::errs() << "\n[SpecialFunctionHandler::handle]\n";
  handlers_ty::iterator it = handlers.find(f);
  if (it != handlers.end()) {
    Handler h = it->second.first;
    bool hasReturnValue = it->second.second;
    // FIXME: Check this... add test?
    if (!hasReturnValue && !target->inst->use_empty()) {
      executor.terminateStateOnExecError(
          state, "expected return value from void special function");
    } else {
      (this->*h)(state, target, arguments);
    }
    return true;
  } else {
    return false;
  }
}

/****/

// reads a concrete string from memory
std::string SpecialFunctionHandler::readStringAtAddress(ExecutionState &state,
                                                        ref<Expr> addressExpr) {
  ObjectPair op;
  addressExpr = executor.toUnique(state, addressExpr);
  if (!isa<ConstantExpr>(addressExpr)) {
    executor.terminateStateOnError(
        state, "Symbolic string pointer passed to one of the klee_ functions",
        Executor::TerminateReason::User);
    return "";
  }
  ref<ConstantExpr> address = cast<ConstantExpr>(addressExpr);
  if (!state.addressSpace.resolveOne(address, op)) {
    executor.terminateStateOnError(
        state, "Invalid string pointer passed to one of the klee_ functions",
        Executor::TerminateReason::User);
    return "";
  }
  bool res __attribute__((unused));
  assert(executor.solver->mustBeTrue(
             state, EqExpr::create(address, op.first->getBaseExpr()), res) &&
         res && "XXX interior pointer unhandled");
  const MemoryObject *mo = op.first;
  const ObjectState *os = op.second;

  char *buf = new char[mo->size];

  unsigned i;
  for (i = 0; i < mo->size - 1; i++) {
    ref<Expr> cur = os->read8(i);
    cur = executor.toUnique(state, cur);
    assert(isa<ConstantExpr>(cur) &&
           "hit symbolic char while reading concrete string");
    buf[i] = cast<ConstantExpr>(cur)->getZExtValue(8);
  }
  buf[i] = 0;

  std::string result(buf);
  delete[] buf;
  return result;
}

/****/

void SpecialFunctionHandler::handleAbort(ExecutionState &state,
                                         KInstruction *target,
                                         std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 0 && "invalid number of arguments to abort");
  executor.terminateStateOnError(state, "abort failure", Executor::Abort);
}

void SpecialFunctionHandler::handleExit(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to exit");
  executor.terminateStateOnExit(state);
}

void SpecialFunctionHandler::handleSilentExit(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to exit");
  executor.terminateState(state, "silent exit", "exit");
}

void SpecialFunctionHandler::handleAliasFunction(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_alias_function");
  std::string old_fn = readStringAtAddress(state, arguments[0]);
  std::string new_fn = readStringAtAddress(state, arguments[1]);
  KLEE_DEBUG_WITH_TYPE("alias_handling", llvm::errs()
                                             << "Replacing " << old_fn
                                             << "() with " << new_fn << "()\n");
  if (old_fn == new_fn)
    state.removeFnAlias(old_fn);
  else
    state.addFnAlias(old_fn, new_fn);
}

void SpecialFunctionHandler::handleAssert(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 3 && "invalid number of arguments to _assert");
  executor.terminateStateOnError(
      state, "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
      Executor::Assert);
}

void SpecialFunctionHandler::handleAssertFail(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 4 &&
         "invalid number of arguments to __assert_fail");
  executor.terminateStateOnError(
      state, "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
      Executor::Assert);
}

void SpecialFunctionHandler::handleReportError(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 4 &&
         "invalid number of arguments to klee_report_error");

  // arguments[0], arguments[1] are file, line
  executor.terminateStateOnError(
      state, readStringAtAddress(state, arguments[2]), Executor::ReportError,
      readStringAtAddress(state, arguments[3]).c_str());
}

void SpecialFunctionHandler::handleSelectPatch(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_select_patch");
  if (state.metaData.getPatchID() >= 0) {
    SPDLOG_DEBUG("[state {}] Patch {} is already selected", state.getID(),
                 state.metaData.getPatchID());
    return;
  }
  if (!state.metaData.isType(StateType::ST_base)) {
    SPDLOG_DEBUG("[state {}] State is not base state", state.getID());
    return;
  }
  uint64_t patch_id_addr;
  std::string patch_id_str = "uni_klee_patch_id";
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "klee_select_patch");
  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    mo->setName(patch_id_str);

    const ObjectState *old = it->first.second;
    ExecutionState *s = it->second;

    executor.executeMakeSymbolic(*s, mo, patch_id_str);
    s->symbolics.back().setSpecial("patch");
    ObjectPair op;
    bool success __attribute__((unused)) =
        s->addressSpace.resolveOne(mo->getBaseExpr(), op);
    ref<Expr> patchIdExpr = op.second->read(0, Expr::Int32);
    executor.selectPatchID(*s, target, arguments, patchIdExpr);
  }
}

void SpecialFunctionHandler::handleUniKleeAddPatch(
    klee::ExecutionState &state, klee::KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 3 &&
         "invalid number of arguments to uni_klee_add_patch");
  // save patch
  ref<Expr> patch_results_expr = arguments[0];
  ref<ConstantExpr> patch_results = dyn_cast<ConstantExpr>(patch_results_expr);
  ObjectPair op;
  if (!state.addressSpace.resolveOne(patch_results, op)) {
    executor.terminateStateOnError(state, "Invalid access to patch_results",
                                   Executor::TerminateReason::User);
  }
  const MemoryObject *mo = op.first;
  const ObjectState *os = op.second;
  ref<Expr> patchIdExpr = arguments[1];
  int patchId = dyn_cast<ConstantExpr>(patchIdExpr)->getZExtValue();
  ref<Expr> patchResultExpr = arguments[2];
  ref<Expr> patchResult = NeExpr::create(
      patchResultExpr, ConstantExpr::create(0, patchResultExpr->getWidth()));
  state.patchDataManager.addPatchData(patchId, patchResult);
  ObjectState *wos = state.addressSpace.getWriteable(mo, os);
  wos->write(patchId * sizeof(int), patchResultExpr);
}

void SpecialFunctionHandler::handleSlicing(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr>> &arguments) {
  if (SlicingCriteria.empty()) {
    klee_message("Sliced file is not given. ignored.");
  }
}

void SpecialFunctionHandler::handleOpenMerge(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (!UseMerge) {
    klee_warning_once(0, "klee_open_merge ignored, use '-use-merge'");
    return;
  }

  state.openMergeStack.push_back(
      ref<MergeHandler>(new MergeHandler(&executor, &state)));

  if (DebugLogMerge)
    llvm::errs() << "open merge: " << &state << "\n";
}

void SpecialFunctionHandler::handleCloseMerge(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (!UseMerge) {
    klee_warning_once(0, "klee_close_merge ignored, use '-use-merge'");
    return;
  }
  Instruction *i = target->inst;

  if (DebugLogMerge)
    llvm::errs() << "close merge: " << &state << " at " << i << '\n';

  if (state.openMergeStack.empty()) {
    std::ostringstream warning;
    warning << &state << " ran into a close at " << i
            << " without a preceding open";
    klee_warning("%s", warning.str().c_str());
  } else {
    assert(executor.inCloseMerge.find(&state) == executor.inCloseMerge.end() &&
           "State cannot run into close_merge while being closed");
    executor.inCloseMerge.insert(&state);
    state.openMergeStack.back()->addClosedState(&state, i);
    state.openMergeStack.pop_back();
  }
}

void SpecialFunctionHandler::handleFileOpen(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_file_open");
  std::string filename = readStringAtAddress(state, arguments[0]);
  int flags = cast<ConstantExpr>(arguments[1])->getZExtValue(32);
  // executor.fdList.push_back()
}

void SpecialFunctionHandler::handleNew(ExecutionState &state,
                                       KInstruction *target,
                                       std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to new");

  executor.executeAlloc(state, arguments[0], false, target);
}

void SpecialFunctionHandler::handleDelete(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // FIXME: Should check proper pairing with allocation type (malloc/free,
  // new/delete, new[]/delete[]).

  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to delete");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleNewArray(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to new[]");
  executor.executeAlloc(state, arguments[0], false, target);
}

void SpecialFunctionHandler::handleDeleteArray(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to delete[]");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleMalloc(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to malloc");
  executor.executeAlloc(state, arguments[0], false, target);
}

void SpecialFunctionHandler::handleAssume(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to klee_assume");

  ref<Expr> e = arguments[0];

  if (e->getWidth() != Expr::Bool)
    e = NeExpr::create(e, ConstantExpr::create(0, e->getWidth()));

  bool res;
  bool success __attribute__((unused)) =
      executor.solver->mustBeFalse(state, e, res);
  assert(success && "FIXME: Unhandled solver failure");
  if (res) {
    if (SilentKleeAssume) {
      executor.terminateState(
          state, "invalid klee_assume call (provably false)", "assume");
    } else {
      executor.terminateStateOnError(
          state, "invalid klee_assume call (provably false)", Executor::User);
    }
  } else {
    executor.addConstraint(state, e);
  }
}

void SpecialFunctionHandler::handleIsSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_is_symbolic");

  executor.bindLocal(
      target, state,
      ConstantExpr::create(!isa<ConstantExpr>(arguments[0]), Expr::Int32));
}

void SpecialFunctionHandler::handlePreferCex(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_prefex_cex");

  ref<Expr> cond = arguments[1];
  if (cond->getWidth() != Expr::Bool)
    cond = NeExpr::create(cond, ConstantExpr::alloc(0, cond->getWidth()));

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "prefex_cex");

  assert(rl.size() == 1 &&
         "prefer_cex target must resolve to precisely one object");

  state.addCexPreference(cond);
}

void SpecialFunctionHandler::handlePosixPreferCex(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (ReadablePosix)
    return handlePreferCex(state, target, arguments);
}

void SpecialFunctionHandler::trackTaint(ExecutionState &state,
                                        KInstruction *target, ref<Expr> value) {

  std::string Str;
  llvm::raw_string_ostream info(Str);
  ExprSMTLIBPrinter printer;
  printer.setOutput(info);
  const ref<Expr> expr = value;
  ExprSMTLIBPrinter::SMTLIB_SORT sort = printer.getSort(expr);
  printer.printExpression(expr, sort);
  //  printer.generateOutput();
  std::string res = info.str();
  std::string source_loc = target->getSourceLocation();
  std::string type;

  if (target->inst->getType()->isFloatTy() ||
      target->inst->getType()->isDoubleTy()) {
    type = "float";
  } else if (target->inst->getType()->isPointerTy()) {
    type = "pointer";
  } else {
    type = "integer";
  }

  if (source_loc.find("/klee", 0) == std::string::npos) {
    std::string log_message = source_loc + " : " + type + " : " + res + "\n";
    klee_log_taint(log_message.c_str());
  }
}

void SpecialFunctionHandler::trackMemory(ExecutionState &state,
                                         llvm::Type *ptr_type,
                                         ref<Expr> address, ref<Expr> size) {

  std::string Str;
  llvm::raw_string_ostream info(Str);
  ExprSMTLIBPrinter printer;
  printer.setOutput(info);

  ExprSMTLIBPrinter::SMTLIB_SORT sort_address = printer.getSort(address);
  printer.printExpression(address, sort_address);

  ExprSMTLIBPrinter::SMTLIB_SORT sort_size = printer.getSort(size);
  printer.printExpression(size, sort_size);

  //  llvm::Type *ptr_type = target->inst->getType();
  unsigned ptr_width = 0;
  if (ptr_type->isPointerTy()) {
    llvm::Type *return_type =
        llvm::dyn_cast<PointerType>(ptr_type)->getPointerElementType();
    ptr_width = return_type->getPrimitiveSizeInBits();
  }

  std::string width_str = std::to_string(ptr_width);

  std::string log_message = info.str() + "(" + width_str + ")" + "\n";
  klee_log_memory(log_message.c_str());
}

void SpecialFunctionHandler::handlePrintExpr(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_print_expr");

  std::string msg_str = readStringAtAddress(state, arguments[0]);

  std::string Str;
  llvm::raw_string_ostream info(Str);
  ExprSMTLIBPrinter printer;
  printer.setOutput(info);
  const ref<Expr> expr = arguments[1];
  ExprSMTLIBPrinter::SMTLIB_SORT sort = printer.getSort(expr);
  printer.printExpression(expr, sort);
  //  printer.generateOutput();
  std::string res = info.str();
  //  llvm::errs() << msg_str << ":" << res << "\n";
  std::string type;

  if (target->inst->getType()->isFloatTy() ||
      target->inst->getType()->isDoubleTy()) {
    type = "float";
  } else if (target->inst->getType()->isPointerTy()) {
    type = "pointer";
  } else {
    type = "integer";
  }

  std::string log_message = "\n[klee:expr] " + msg_str + " : " + res + "\n";
  log_message += "[klee:expr] [var-type]: " + type + "\n";
  klee_log_expr(log_message.c_str());
}

void SpecialFunctionHandler::handlePrintStatement(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_print_stmt");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  //  printer.generateOutput();
  klee_log_expr(msg_str.c_str());
}

void SpecialFunctionHandler::handleSetForking(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_set_forking");
  ref<Expr> value = executor.toUnique(state, arguments[0]);

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    state.forkDisabled = CE->isZero();
  } else {
    executor.terminateStateOnError(
        state, "klee_set_forking requires a constant arg", Executor::User);
  }
}

void SpecialFunctionHandler::handleStackTrace(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  state.dumpStack(outs());
}

void SpecialFunctionHandler::handleWarning(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_warning");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning("%s: %s", state.stack.back().kf->function->getName().data(),
               msg_str.c_str());
}

void SpecialFunctionHandler::handleWarningOnce(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_warning_once");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning_once(0, "%s: %s",
                    state.stack.back().kf->function->getName().data(),
                    msg_str.c_str());
}

void SpecialFunctionHandler::handlePrintRange(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_print_range");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  llvm::errs() << msg_str << ":" << arguments[1];
  if (!isa<ConstantExpr>(arguments[1])) {
    // FIXME: Pull into a unique value method?
    ref<ConstantExpr> value;
    bool success __attribute__((unused)) =
        executor.solver->getValue(state, arguments[1], value);
    assert(success && "FIXME: Unhandled solver failure");
    bool res;
    success = executor.solver->mustBeTrue(
        state, EqExpr::create(arguments[1], value), res);
    assert(success && "FIXME: Unhandled solver failure");
    if (res) {
      llvm::errs() << " == " << value;
    } else {
      llvm::errs() << " ~= " << value;
      std::pair<ref<Expr>, ref<Expr>> res =
          executor.solver->getRange(state, arguments[1]);
      llvm::errs() << " (in [" << res.first << ", " << res.second << "])";
    }
  }
  llvm::errs() << "\n";
}

void SpecialFunctionHandler::handleGetObjSize(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_get_obj_size");
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "klee_get_obj_size");
  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    executor.bindLocal(
        target, *it->second,
        ConstantExpr::create(it->first.first->size,
                             executor.kmodule->targetData->getTypeSizeInBits(
                                 target->inst->getType())));
  }
}

void SpecialFunctionHandler::handleGetErrno(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 0 &&
         "invalid number of arguments to klee_get_errno");
#ifndef WINDOWS
  int *errno_addr = executor.getErrnoLocation(state);
#else
  int *errno_addr = nullptr;
#endif

  // Retrieve the memory object of the errno variable
  ObjectPair result;
  bool resolved = state.addressSpace.resolveOne(
      ConstantExpr::create((uint64_t)errno_addr, Expr::Int64), result);
  if (!resolved)
    executor.terminateStateOnError(state, "Could not resolve address for errno",
                                   Executor::User);
  executor.bindLocal(target, state, result.second->read(0, Expr::Int32));
}

void SpecialFunctionHandler::handleErrnoLocation(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // Returns the address of the errno variable
  assert(arguments.size() == 0 &&
         "invalid number of arguments to __errno_location/__error");

#ifndef WINDOWS
  int *errno_addr = executor.getErrnoLocation(state);
#else
  int *errno_addr = nullptr;
#endif

  executor.bindLocal(
      target, state,
      ConstantExpr::create((uint64_t)errno_addr,
                           executor.kmodule->targetData->getTypeSizeInBits(
                               target->inst->getType())));
}
void SpecialFunctionHandler::handleCalloc(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 && "invalid number of arguments to calloc");

  ref<Expr> size = MulExpr::create(arguments[0], arguments[1]);
  executor.executeAlloc(state, size, false, target, true);
}

void SpecialFunctionHandler::handleRealloc(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 && "invalid number of arguments to realloc");
  ref<Expr> address = arguments[0];
  ref<Expr> size = arguments[1];

  Executor::StatePair zeroSize =
      executor.fork(state, Expr::createIsZero(size), true);

  if (zeroSize.first) { // size == 0
    executor.executeFree(*zeroSize.first, address, target);
  }
  if (zeroSize.second) { // size != 0
    Executor::StatePair zeroPointer =
        executor.fork(*zeroSize.second, Expr::createIsZero(address), true);

    if (zeroPointer.first) { // address == 0
      executor.executeAlloc(*zeroPointer.first, size, false, target);
    }
    if (zeroPointer.second) { // address != 0
      Executor::ExactResolutionList rl;
      executor.resolveExact(*zeroPointer.second, address, rl, "realloc");

      for (Executor::ExactResolutionList::iterator it = rl.begin(),
                                                   ie = rl.end();
           it != ie; ++it) {
        executor.executeAlloc(*it->second, size, false, target, false,
                              it->first.second);
      }
    }
  }
}

void SpecialFunctionHandler::handleFree(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to free");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleCheckMemoryAccess(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_check_memory_access");

  ref<Expr> address = executor.toUnique(state, arguments[0]);
  ref<Expr> size = executor.toUnique(state, arguments[1]);
  if (!isa<ConstantExpr>(address) || !isa<ConstantExpr>(size)) {
    executor.terminateStateOnError(
        state, "check_memory_access requires constant args", Executor::User);
  } else {
    ObjectPair op;

    if (!state.addressSpace.resolveOne(cast<ConstantExpr>(address), op)) {
      executor.terminateStateOnError(state, "check_memory_access: memory error",
                                     Executor::Ptr, NULL,
                                     executor.getAddressInfo(state, address));
    } else {
      ref<Expr> chk = op.first->getBoundsCheckPointer(
          address, cast<ConstantExpr>(size)->getZExtValue());
      if (!chk->isTrue()) {
        executor.terminateStateOnError(
            state, "check_memory_access: memory error", Executor::Ptr, NULL,
            executor.getAddressInfo(state, address));
      }
    }
  }
}

void SpecialFunctionHandler::handleGetValue(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_get_value");

  executor.executeGetValue(state, arguments[0], target);
}

void SpecialFunctionHandler::handleDefineFixedObject(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[0]) &&
         "expect constant address argument to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[1]) &&
         "expect constant size argument to klee_define_fixed_object");

  uint64_t address = cast<ConstantExpr>(arguments[0])->getZExtValue();
  uint64_t size = cast<ConstantExpr>(arguments[1])->getZExtValue();
  MemoryObject *mo =
      executor.memory->allocateFixed(address, size, state.prevPC->inst);
  executor.bindObjectInState(state, mo, false);
  mo->isUserSpecified = true; // XXX hack;
}

void SpecialFunctionHandler::handleMakeSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  std::string name;

  if (arguments.size() != 3) {
    executor.terminateStateOnError(state,
                                   "Incorrect number of arguments to "
                                   "klee_make_symbolic(void*, size_t, char*)",
                                   Executor::User);
    return;
  }

  name = arguments[2]->isZero() ? "" : readStringAtAddress(state, arguments[2]);

  if (name.length() == 0) {
    name = "unnamed";
    klee_warning("klee_make_symbolic: renamed empty name to \"unnamed\"");
  }

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "make_symbolic");

  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    mo->setName(name);

    const ObjectState *old = it->first.second;
    ExecutionState *s = it->second;

    if (old->readOnly) {
      executor.terminateStateOnError(*s, "cannot make readonly object symbolic",
                                     Executor::User);
      return;
    }

    // FIXME: Type coercion should be done consistently somewhere.
    bool res;
    bool success __attribute__((unused)) = executor.solver->mustBeTrue(
        *s,
        EqExpr::create(
            ZExtExpr::create(arguments[1], Context::get().getPointerWidth()),
            mo->getSizeExpr()),
        res);
    assert(success && "FIXME: Unhandled solver failure");

    if (res) {
      executor.executeMakeSymbolic(*s, mo, name);
      s->symbolics.back().setSpecial("heap");
    } else {
      executor.terminateStateOnError(
          *s, "wrong size given to klee_make_symbolic[_name]", Executor::User);
    }
  }
}

void SpecialFunctionHandler::handleUniMakeSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (arguments.size() != 3) {
    executor.terminateStateOnError(state,
                                   "Incorrect number of arguments to "
                                   "klee_make_symbolic(void*, size_t, char*)",
                                   Executor::User);
    return;
  } else {
    std::string name = readStringAtAddress(state, arguments[2]);
    ref<Expr> address = arguments[0];
    ObjectPair op;
    SPDLOG_DEBUG("[state {}] uni_klee_make_symbolic {} at {} size {}",
                 state.getID(), name, address->str(), arguments[1]->str());
    executor.resolveMO(state, address, op);
    if (op.first == nullptr) {
      // Just ignore it
      return;
    }
    ref<Expr> size = arguments[1];
    ConstantExpr *CE = dyn_cast<ConstantExpr>(size);
    if (!isa<ConstantExpr>(address)) {
      return;
    }
    ConstantExpr *addressCE = dyn_cast<ConstantExpr>(address);
    if (CE->getZExtValue() == op.first->address) {
      executor.executeMakeSymbolic(state, op.first, name, true);
      state.symbolics.back().setHeap();
    } else {
      executor.executeMakeFieldSymbolic(
          state, op.first, addressCE->getZExtValue() - op.first->address,
          nullptr, CE->getZExtValue(), name, true);
    }
  }
}

void SpecialFunctionHandler::handleExtractFixMakeSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (arguments.size() != 3) {
    executor.terminateStateOnError(
        state,
        "Incorrect number of arguments to "
        "extractfix_make_symbolic(void*, size_t, char*)",
        Executor::User);
    return;
  } else {
    std::string name = readStringAtAddress(state, arguments[2]);
    ref<Expr> address = arguments[0];
    ObjectPair op;
    SPDLOG_DEBUG("[state {}] extractfix_make_symbolic {} at {} size {}",
                 state.getID(), name, address->str(), arguments[1]->str());
    executor.resolveMO(state, address, op);
    if (op.first == nullptr) {
      // Just ignore it
      return;
    }
    ref<Expr> size = arguments[1];
    ConstantExpr *CE = dyn_cast<ConstantExpr>(size);
    if (!isa<ConstantExpr>(address)) {
      return;
    }
    ConstantExpr *addressCE = dyn_cast<ConstantExpr>(address);
    if (CE->getZExtValue() == op.first->address) {
      executor.executeMakeSymbolic(state, op.first, name, true);
      state.symbolics.back().setHeap();
    } else {
      executor.executeMakeFieldSymbolic(
          state, op.first, addressCE->getZExtValue() - op.first->address,
          nullptr, CE->getZExtValue(), name, true);
    }
  }
}

void SpecialFunctionHandler::handleMarkGlobal(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_mark_global");

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "mark_global");

  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    assert(!mo->isLocal);
    mo->isGlobal = true;
  }
}

void SpecialFunctionHandler::handleAddOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on addition",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleSubOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on subtraction",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleMulOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on multiplication",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleShiftOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on shift operation",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleDivRemOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on division or remainder",
                                 Executor::Overflow);
}
