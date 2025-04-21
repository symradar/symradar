//===-- ErrorHandling.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <set>

#include "spdlog/spdlog.h"

#include <chrono>

using namespace klee;
using namespace llvm;

FILE *klee::klee_warning_file = NULL;
FILE *klee::klee_message_file = NULL;
FILE *klee::klee_ppc_file = NULL;
FILE *klee::klee_expr_file = NULL;
FILE *klee::klee_trace_file = NULL;
FILE *klee::klee_concrete_file = NULL;
FILE *klee::klee_taint_file = NULL;
FILE *klee::klee_memory_file = NULL;
FILE *klee::klee_data_log_file = NULL;
FILE *klee::klee_bb_trace_file = NULL;

// sqlite3 *klee::klee_bb_trace_db = NULL;

static int bb_line_count = 0;

static const char *warningPrefix = "WARNING";
static const char *warningOncePrefix = "WARNING ONCE";
static const char *errorPrefix = "ERROR";
static const char *notePrefix = "NOTE";
static const char *ppcPrefix = "PartialPathCondition";
static const char *exprPrefix = "VariableExpression";
static const char *tracePrefix = nullptr;
static const char *concretePrefix = "CONCRETE";
static const char *taintPrefix = "TaintTrack";
static const char *memoryPrefix = "MemoryTrack";

std::chrono::high_resolution_clock::time_point startTime;

static char LoggerBuffer[4096];

namespace {
cl::opt<bool> WarningsOnlyToFile(
    "warnings-only-to-file", cl::init(false),
    cl::desc("All warnings will be written to warnings.txt only.  If disabled, "
             "they are also written on screen."));
}

void klee::start_time() {
  startTime = std::chrono::high_resolution_clock::now();
}

static bool shouldSetColor(const char *pfx, const char *msg,
                           const char *prefixToSearchFor) {
  if (pfx && strcmp(pfx, prefixToSearchFor) == 0)
    return true;

  if (llvm::StringRef(msg).startswith(prefixToSearchFor))
    return true;

  return false;
}

static void klee_vfmessage(FILE *fp, const char *pfx, const char *msg,
                           va_list ap) {
  if (!fp)
    return;

  llvm::raw_fd_ostream fdos(fileno(fp), /*shouldClose=*/false,
                            /*unbuffered=*/true);
  bool modifyConsoleColor = fdos.is_displayed() && (fp == stderr);

  if (modifyConsoleColor) {

    // Warnings
    if (shouldSetColor(pfx, msg, warningPrefix))
      fdos.changeColor(llvm::raw_ostream::MAGENTA,
                       /*bold=*/false,
                       /*bg=*/false);

    // Once warning
    if (shouldSetColor(pfx, msg, warningOncePrefix))
      fdos.changeColor(llvm::raw_ostream::MAGENTA,
                       /*bold=*/true,
                       /*bg=*/false);

    // Errors
    if (shouldSetColor(pfx, msg, errorPrefix))
      fdos.changeColor(llvm::raw_ostream::RED,
                       /*bold=*/true,
                       /*bg=*/false);

    // Notes
    if (shouldSetColor(pfx, msg, notePrefix))
      fdos.changeColor(llvm::raw_ostream::WHITE,
                       /*bold=*/true,
                       /*bg=*/false);
  }

  fdos << "KLEE: ";
  if (pfx)
    fdos << pfx << ": ";

  // FIXME: Can't use fdos here because we need to print
  // a variable number of arguments and do substitution
  vfprintf(fp, msg, ap);
  fflush(fp);

  fdos << "\n";

  if (modifyConsoleColor)
    fdos.resetColor();

  fdos.flush();
}

/* Prints a message/warning.

   If pfx is NULL, this is a regular message, and it's sent to
   klee_message_file (messages.txt).  Otherwise, it is sent to
   klee_warning_file (warnings.txt).

   Iff onlyToFile is false, the message is also printed on stderr.
*/
static void klee_vmessage(const char *pfx, bool onlyToFile, int log_mode,
                          const char *msg, va_list ap) {
  // if (!onlyToFile) {
  //   va_list ap2;
  //   va_copy(ap2, ap);
  //   klee_vfmessage(stderr, pfx, msg, ap2);
  //   va_end(ap2);
  // }

  if (log_mode == 1)
    klee_vfmessage(klee_ppc_file, pfx, msg, ap);
  else if (log_mode == 2)
    klee_vfmessage(klee_expr_file, pfx, msg, ap);
  else if (log_mode == 3)
    klee_vfmessage(klee_trace_file, pfx, msg, ap);
  else if (log_mode == 4)
    klee_vfmessage(klee_concrete_file, pfx, msg, ap);
  else if (log_mode == 5)
    klee_vfmessage(klee_taint_file, pfx, msg, ap);
  else if (log_mode == 6)
    klee_vfmessage(klee_memory_file, pfx, msg, ap);
  else {
    va_list ap2;
    va_copy(ap2, ap);
    vsnprintf(LoggerBuffer, 4096, msg, ap2);
    va_end(ap2);
    if (pfx == nullptr)
      SPDLOG_INFO("KLEE: {}", LoggerBuffer);
    else
      SPDLOG_WARN("KLEE: {}: {}", pfx, LoggerBuffer);
    klee_vfmessage(pfx ? klee_warning_file : klee_message_file, pfx, msg, ap);
  }
}

void klee::klee_message(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(NULL, false, 0, msg, ap);
  va_end(ap);
}

/* Message to be written only to file */
void klee::klee_message_to_file(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(NULL, true, 0, msg, ap);
  va_end(ap);
}

/* Log PPC to file */
void klee::klee_log_ppc(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(ppcPrefix, true, 1, msg, ap);
  va_end(ap);
}

/* Log Concrete values to file */
void klee::klee_log_concrete(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(concretePrefix, true, 4, msg, ap);
  va_end(ap);
}

/* Log Trace to file */
void klee::klee_log_trace(const char *msg, ...) {
  if (klee_trace_file) {
    fprintf(klee_trace_file, "%s\n", msg);
    fflush(klee_trace_file);
  }
}

/* Log Taint to file */
void klee::klee_log_taint(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(taintPrefix, true, 5, msg, ap);
  va_end(ap);
}

/* Log Memory Changes to file */
void klee::klee_log_memory(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(memoryPrefix, true, 6, msg, ap);
  va_end(ap);
}

void klee::klee_log_data(std::string msg) {
  auto endTime = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime -
                                                            startTime);
  SPDLOG_DEBUG("{} [time {}]", msg, ms.count());
  if (klee_data_log_file) {
    fprintf(klee_data_log_file, "%s [time %lld]\n", msg.c_str(), ms.count());
    fflush(klee_data_log_file);
  }
}

void klee::klee_log_bb_trace(std::string msg) {
  SPDLOG_DEBUG("{}", msg);
  // limit the size of bb trace (Currently 10M line)
  if (klee_bb_trace_file && bb_line_count < 10000000) {
    fprintf(klee_bb_trace_file, "%s\n", msg.c_str());
    fflush(klee_bb_trace_file);
    bb_line_count++;
  }
}

void klee::klee_record_bb_trace(int stateId, int assemblyLine, int opcode,
                                std::string functionName) {
  // limit the size of bb trace (Currently 1B line, 40GB max)
  // if (klee_bb_trace_db && bb_line_count < 1000000000) {
  //   bb_line_count++;

  //   int rc = sqlite3_exec(
  //       klee_bb_trace_db,
  //       fmt::format(
  //           "INSERT INTO bb_trace (stateId, assemblyLine, opcode, function)"
  //           "VALUES ({}, {}, {}, (SELECT id FROM function WHERE name =
  //           '{}'));", stateId, assemblyLine, opcode, functionName) .c_str(),
  //       NULL, NULL, NULL);
  //   if (rc != SQLITE_OK) {
  //     SPDLOG_WARN("Error preparing bb trace: {}",
  //                 sqlite3_errmsg(klee_bb_trace_db));
  //   }
  // }
}

/* Log Expr to file */
void klee::klee_log_expr(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(exprPrefix, true, 2, msg, ap);
  va_end(ap);
}

void klee::klee_error(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(errorPrefix, false, 0, msg, ap);
  va_end(ap);
  exit(1);
}

void klee::klee_warning(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  klee_vmessage(warningPrefix, WarningsOnlyToFile, 0, msg, ap);
  va_end(ap);
}

/* Prints a warning once per message. */
void klee::klee_warning_once(const void *id, const char *msg, ...) {
  static std::set<std::pair<const void *, const char *>> keys;
  std::pair<const void *, const char *> key;

  /* "calling external" messages contain the actual arguments with
     which we called the external function, so we need to ignore them
     when computing the key. */
  if (strncmp(msg, "calling external", strlen("calling external")) != 0)
    key = std::make_pair(id, msg);
  else
    key = std::make_pair(id, "calling external");

  if (!keys.count(key)) {
    keys.insert(key);
    va_list ap;
    va_start(ap, msg);
    klee_vmessage(warningOncePrefix, WarningsOnlyToFile, 0, msg, ap);
    va_end(ap);
  }
}
