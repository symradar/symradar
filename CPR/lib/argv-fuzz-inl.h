/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


/*
   american fuzzy lop - sample argv fuzzing wrapper
   ------------------------------------------------

   Written by Michal Zalewski <lcamtuf@google.com>

   This file shows a simple way to fuzz command-line parameters with stock
   afl-fuzz. To use, add:

   #include "/path/to/argv-fuzz-inl.h"

   ...to the file containing main(), ideally placing it after all the 
   standard includes. Next, put AFL_INIT_ARGV(); near the very beginning of
   main().

   This will cause the program to read NUL-delimited input from stdin and
   put it in argv[]. Two subsequent NULs terminate the array.

   If you would like to always preserve argv[0], use this instead:
   AFL_INIT_SET0("prog_name");
*/

#ifndef _HAVE_ARGV_FUZZ_INL
#define _HAVE_ARGV_FUZZ_INL

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#define AFL_INIT_ARGV() do { argv = afl_init_argv(&argc); } while (0)

#define AFL_INIT_SET0(_p) do { \
    argv = afl_init_argv(&argc); \
    argv[0] = (_p); \
    if (!argc) argc = 1; \
  } while (0)

#define AFL_INIT_SET02(_p, _two) do { \
    char **new_argv = malloc(sizeof(char*) * 4); \
    new_argv[0] = (_p); \
    new_argv[1] = afl_init_single_argv_before_space(); \
    new_argv[2] = (_two); \
    argc = 3; \
    new_argv[argc] = NULL; \
    argv = new_argv; \
  } while (0)

#define AFL_INIT_SET0234(_p, _two, _three, _four) do { \
    char **new_argv = malloc(sizeof(char*) * 6); \
    new_argv[0] = (_p); \
    new_argv[1] = afl_init_single_argv(); \
    new_argv[2] = (_two); \
    new_argv[3] = (_three); \
    new_argv[4] = (_four); \
    argc = 5; \
    new_argv[argc] = NULL; \
    argv = new_argv; \
  } while (0)

#define AFL_INIT_SET03(_p, _three) do { \
    char **new_argv = malloc(sizeof(char*) * 5); \
    new_argv[0] = (_p); \
    char **ret = afl_init_two_argv(); \
    new_argv[1] = ret[0]; \
    new_argv[2] = ret[1]; \
    new_argv[3] = (_three); \
    argc = 4; \
    new_argv[argc] = NULL; \
    argv = new_argv; \
  } while (0)

#define MAX_CMDLINE_LEN 100000
#define MAX_CMDLINE_PAR 1000

static char** afl_init_argv(int* argc) {

  static char  in_buf[MAX_CMDLINE_LEN];
  static char* ret[MAX_CMDLINE_PAR];

  char* ptr = in_buf;
  int   rc  = 1; /* start after argv[0] */

  if (read(0, in_buf, MAX_CMDLINE_LEN - 2) < 0);

  while (*ptr) {

    ret[rc] = ptr;

    /* insert '\0' at the end of ret[rc] on first space-sym */
    while (*ptr && !isspace(*ptr)) ptr++;
    *ptr = '\0';
    ptr++;

    /* skip more space-syms */
    while (*ptr && isspace(*ptr)) ptr++;

    rc++;
  }

  *argc = rc;

  return ret;

}

/**
 * Init a single argv, all chars including spaces are considered as one arg.
 **/
static char* afl_init_single_argv(void) {
  static char in_buf[MAX_CMDLINE_LEN];
  static char* ret;
  if (read(0, in_buf, MAX_CMDLINE_LEN - 2) < 0);

  char *ptr = in_buf;
  while (*ptr && isspace(*ptr)) ptr++;
  ret = ptr;
  while(*ptr) ptr++;
  *ptr = '\0';

  return ret;
}

/**
 * Takes one single argv before space from stdin.
 **/
static char* afl_init_single_argv_before_space(void) {
  static char in_buf[MAX_CMDLINE_LEN];
  static char* ret;
  if (read(0, in_buf, MAX_CMDLINE_LEN - 2) < 0);

  char *ptr = in_buf;
  ret = ptr;
  while (*ptr && !isspace(*ptr)) ptr++;
  *ptr = '\0';

  return ret;
}

/**
 * Takes two space-seprated argv from stdin.
 **/
static char** afl_init_two_argv(void) {
  static char in_buf[MAX_CMDLINE_LEN];
  static char* ret[2];
  if (read(0, in_buf, MAX_CMDLINE_LEN - 2) < 0);

  char *ptr = in_buf;
  ret[0] = ptr;
  while (*ptr && !isspace(*ptr)) ptr++;
  *ptr = '\0';
  ptr++;
  // skip space between two args
  while (*ptr && isspace(*ptr)) ptr++;
  ret[1] = ptr;
  while (*ptr && !isspace(*ptr)) ptr++;
  *ptr = '\0';

  return ret;
}

#undef MAX_CMDLINE_LEN
#undef MAX_CMDLINE_PAR

#endif /* !_HAVE_ARGV_FUZZ_INL */
