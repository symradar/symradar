//===-- ErrorHandling.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __KLEE_ERROR_HANDLING_H__
#define __KLEE_ERROR_HANDLING_H__

#ifdef __CYGWIN__
#ifndef WINDOWS
#define WINDOWS
#endif
#endif

#include <stdio.h>
#include <string>

// #include "sqlite3.h"

namespace klee {

extern FILE *klee_warning_file;
extern FILE *klee_message_file;
extern FILE *klee_ppc_file;
extern FILE *klee_expr_file;
extern FILE *klee_trace_file;
extern FILE *klee_concrete_file;
extern FILE *klee_taint_file;
extern FILE *klee_memory_file;
extern FILE *klee_data_log_file;
extern FILE *klee_bb_trace_file;
// extern sqlite3 *klee_bb_trace_db;

void start_time();

/// Print "KLEE: ERROR: " followed by the msg in printf format and a
/// newline on stderr and to warnings.txt, then exit with an error.
void klee_error(const char *msg, ...)
    __attribute__((format(printf, 1, 2), noreturn));

/// Print "KLEE: " followed by the msg in printf format and a
/// newline on stderr and to messages.txt.
void klee_message(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Print "KLEE: " followed by the msg in printf format and a
/// newline to messages.txt.
void klee_message_to_file(const char *msg, ...)
    __attribute__((format(printf, 1, 2)));

/// Log PPC
void klee_log_ppc(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Log Trace
void klee_log_trace(const char *msg, ...) __attribute__((format(printf, 1, 2)));
/// Log Concretization
void klee_log_concrete(const char *msg, ...)
    __attribute__((format(printf, 1, 2)));

/// Log Expr
void klee_log_expr(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Log Taint
void klee_log_taint(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Log Memory
void klee_log_memory(const char *msg, ...)
    __attribute__((format(printf, 1, 2)));

void klee_log_data(std::string msg);

void klee_log_bb_trace(std::string msg);
void klee_record_bb_trace(int stateId, int assemblyLine, int opcode,
                          std::string functionName);

/// Print "KLEE: WARNING: " followed by the msg in printf format and a
/// newline on stderr and to warnings.txt.
void klee_warning(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Print "KLEE: WARNING: " followed by the msg in printf format and a
/// newline on stderr and to warnings.txt. However, the warning is only
/// printed once for each unique (id, msg) pair (as pointers).
void klee_warning_once(const void *id, const char *msg, ...)
    __attribute__((format(printf, 2, 3)));
} // namespace klee

#endif /* __KLEE_ERROR_HANDLING_H__ */
