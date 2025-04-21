//===-- RuntimeFD.h ----------------------------------------------*- C++-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef RUNTIME_FD_H
#define RUNTIME_FD_H

#include <dirent.h>
#include <sys/types.h>

#ifdef HAVE_SYSSTATFS_H
#include <sys/statfs.h>
#endif

#if defined(__APPLE__)
#include <sys/dtrace.h>
#include <sys/mount.h>
#include <sys/param.h>
#if !defined(dirent64)
#define dirent64 dirent
#endif
#endif

namespace klee {
typedef struct {
  unsigned size; /* in bytes */
  char *contents;
  struct stat64 *stat;
} exe_disk_file_t;

typedef enum {
  eOpen = (1 << 0),
  eCloseOnExec = (1 << 1),
  eReadable = (1 << 2),
  eWriteable = (1 << 3)
} exe_file_flag_t;

typedef struct {
  int fd;                 /* actual fd if not symbolic */
  unsigned flags;         /* set of exe_file_flag_t values. fields
                             are only defined when flags at least
                             has eOpen. */
  unsigned real_flags;    /* actual flags used to open the file */
  unsigned mode;          /* mode used to open the file */
  off64_t off;            /* offset */
  exe_disk_file_t *dfile; /* ptr to file on disk, if symbolic */
} exe_file_t;

typedef struct {
  unsigned n_sym_files; /* number of symbolic input files, excluding stdin */
  exe_disk_file_t *sym_stdin, *sym_stdout;
  unsigned stdout_writes; /* how many chars were written to stdout */
  exe_disk_file_t *sym_files;
  /* --- */
  /* the maximum number of failures on one path; gets decremented after each
   * failure */
  unsigned max_failures;

  /* Which read, write etc. call should fail */
  int *read_fail, *write_fail, *close_fail, *ftruncate_fail, *getcwd_fail;
  int *chmod_fail, *fchmod_fail;
} exe_file_system_t;

#define MAX_FDS 32

/* Note, if you change this structure be sure to update the
   initialization code if necessary. New fields should almost
   certainly be at the end. */
typedef struct {
  exe_file_t fds[MAX_FDS];
  mode_t umask; /* process umask */
  unsigned version;
  /* If set, writes execute as expected.  Otherwise, writes extending
     the file size only change the contents up to the initial
     size. The file offset is always incremented correctly. */
  int save_all_writes;
} exe_sym_env_t;
} // namespace klee

#endif