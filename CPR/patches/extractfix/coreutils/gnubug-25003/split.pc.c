/* split.c -- split a file into pieces.
   Copyright (C) 1988-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* By tege@sics.se, with rms.

   TODO:
   * support -p REGEX as in BSD's split.
   * support --suppress-matched as in csplit.  */
#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "system.h"
#include "die.h"
#include "error.h"
#include "fd-reopen.h"
#include "fcntl--.h"
#include "full-write.h"
#include "ioblksize.h"
#include "quote.h"
#include "safe-read.h"
#include "sig2str.h"
#include "xfreopen.h"
#include "xdectoint.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "split"

#define AUTHORS \
  proper_name ("Torbjorn Granlund"), \
  proper_name ("Richard M. Stallman")

/* Shell command to filter through, instead of creating files.  */
static char const *filter_command;

/* Process ID of the filter.  */
static int filter_pid;

/* Array of open pipes.  */
static int *open_pipes;
static size_t open_pipes_alloc;
static size_t n_open_pipes;

/* Blocked signals.  */
static sigset_t oldblocked;
static sigset_t newblocked;

/* Base name of output files.  */
static char const *outbase;

/* Name of output files.  */
static char *outfile;

/* Pointer to the end of the prefix in OUTFILE.
   Suffixes are inserted here.  */
static char *outfile_mid;

/* Generate new suffix when suffixes are exhausted.  */
static bool suffix_auto = true;

/* Length of OUTFILE's suffix.  */
static size_t suffix_length;

/* Alphabet of characters to use in suffix.  */
static char const *suffix_alphabet = "abcdefghijklmnopqrstuvwxyz";

/* Numerical suffix start value.  */
static const char *numeric_suffix_start;

/* Additional suffix to append to output file names.  */
static char const *additional_suffix;

/* Name of input file.  May be "-".  */
static char *infile;

/* stat buf for input file.  */
static struct stat in_stat_buf;
#include <klee/klee.h>
#ifndef CPR_OUTPUT
#define CPR_OUTPUT(id, typestr, value) value
#endif

/* Descriptor on which output file is open.  */
static int output_desc = -1;

/* If true, print a diagnostic on standard error just before each
   output file is opened. */
static bool verbose;

/* If true, don't generate zero length output files. */
static bool elide_empty_files;

/* If true, in round robin mode, immediately copy
   input to output, which is much slower, so disabled by default.  */
static bool unbuffered;

/* The character marking end of line.  Defaults to \n below.  */
static int eolchar = -1;

/* The split mode to use.  */
enum Split_type
{
  type_undef, type_bytes, type_byteslines, type_lines, type_digits,
  type_chunk_bytes, type_chunk_lines, type_rr
};

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  VERBOSE_OPTION = CHAR_MAX + 1,
  FILTER_OPTION,
  IO_BLKSIZE_OPTION,
  ADDITIONAL_SUFFIX_OPTION
};

static struct option const longopts[] =
{
  {"bytes", required_argument, NULL, 'b'},
  {"lines", required_argument, NULL, 'l'},
  {"line-bytes", required_argument, NULL, 'C'},
  {"number", required_argument, NULL, 'n'},
  {"elide-empty-files", no_argument, NULL, 'e'},
  {"unbuffered", no_argument, NULL, 'u'},
  {"suffix-length", required_argument, NULL, 'a'},
  {"additional-suffix", required_argument, NULL,
   ADDITIONAL_SUFFIX_OPTION},
  {"numeric-suffixes", optional_argument, NULL, 'd'},
  {"filter", required_argument, NULL, FILTER_OPTION},
  {"verbose", no_argument, NULL, VERBOSE_OPTION},
  {"separator", required_argument, NULL, 't'},
  {"-io-blksize", required_argument, NULL,
   IO_BLKSIZE_OPTION}, /* do not document */
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Return true if the errno value, ERR, is ignorable.  */
static inline bool
ignorable (int err)
{
  return filter_command && err == EPIPE;
}

static void
set_suffix_length (uintmax_t n_units, enum Split_type split_type)
{
#define DEFAULT_SUFFIX_LENGTH 2

  uintmax_t suffix_needed = 0;

  /* The suffix auto length feature is incompatible with
     a user specified start value as the generated suffixes
     are not all consecutive.  */
  if (numeric_suffix_start)
    suffix_auto = false;

  /* Auto-calculate the suffix length if the number of files is given.  */
  if (split_type == type_chunk_bytes || split_type == type_chunk_lines
      || split_type == type_rr)
    {
      uintmax_t n_units_end = n_units;
      if (numeric_suffix_start)
        {
          uintmax_t n_start;
          strtol_error e = xstrtoumax (numeric_suffix_start, NULL, 10,
                                       &n_start, "");
          if (e == LONGINT_OK && n_start <= UINTMAX_MAX - n_units)
            {
              /* Restrict auto adjustment so we don't keep
                 incrementing a suffix size arbitrarily,
                 as that would break sort order for files
                 generated from multiple split runs.  */
              if (n_start < n_units)
                n_units_end += n_start;
            }

        }
      size_t alphabet_len = strlen (suffix_alphabet);
      bool alphabet_slop = (n_units_end % alphabet_len) != 0;
      while (n_units_end /= alphabet_len)
        suffix_needed++;
      suffix_needed += alphabet_slop;
      suffix_auto = false;
    }

  if (suffix_length)            /* set by user */
    {
      if (suffix_length < suffix_needed)
        {
          die (EXIT_FAILURE, 0,
               _("the suffix length needs to be at least %"PRIuMAX),
               suffix_needed);
        }
      suffix_auto = false;
      return;
    }
  else
    suffix_length = MAX (DEFAULT_SUFFIX_LENGTH, suffix_needed);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE [PREFIX]]\n\
"),
              program_name);
      fputs (_("\
Output pieces of FILE to PREFIXaa, PREFIXab, ...;\n\
default size is 1000 lines, and default PREFIX is 'x'.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fprintf (stdout, _("\
  -a, --suffix-length=N   generate suffixes of length N (default %d)\n\
      --additional-suffix=SUFFIX  append an additional SUFFIX to file names\n\
  -b, --bytes=SIZE        put SIZE bytes per output file\n\
  -C, --line-bytes=SIZE   put at most SIZE bytes of records per output file\n\
  -d                      use numeric suffixes starting at 0, not alphabetic\n\
      --numeric-suffixes[=FROM]  same as -d, but allow setting the start value\
\n\
  -e, --elide-empty-files  do not generate empty output files with '-n'\n\
      --filter=COMMAND    write to shell COMMAND; file name is $FILE\n\
  -l, --lines=NUMBER      put NUMBER lines/records per output file\n\
  -n, --number=CHUNKS     generate CHUNKS output files; see explanation below\n\
  -t, --separator=SEP     use SEP instead of newline as the record separator;\n\
                            '\\0' (zero) specifies the NUL character\n\
  -u, --unbuffered        immediately copy input to output with '-n r/...'\n\
"), DEFAULT_SUFFIX_LENGTH);
      fputs (_("\
      --verbose           print a diagnostic just before each\n\
                            output file is opened\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_size_note ();
      fputs (_("\n\
CHUNKS may be:\n\
  N       split into N files based on size of input\n\
  K/N     output Kth of N to stdout\n\
  l/N     split into N files without splitting lines/records\n\
  l/K/N   output Kth of N to stdout without splitting lines/records\n\
  r/N     like 'l' but use round robin distribution\n\
  r/K/N   likewise but only output Kth of N to stdout\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Return the number of bytes that can be read from FD with status ST.
   Store up to the first BUFSIZE bytes of the file's data into BUF,
   and advance the file position by the number of bytes read.  On
   input error, set errno and return -1.  */

static off_t
input_file_size (int fd, struct stat const *st, char *buf, size_t bufsize)
{
  off_t cur = lseek (fd, 0, SEEK_CUR);
  if (cur < 0)
    {
      if (errno == ESPIPE)
        errno = 0; /* Suppress confusing seek error.  */
      return -1;
    }

  off_t size = 0;
  do
    {
      size_t n_read = safe_read (fd, buf + size, bufsize - size);
      if (n_read == 0)
        return size;
      if (n_read == SAFE_READ_ERROR)
        return -1;
      size += n_read;
    }
  while (size < bufsize);

  /* Note we check st_size _after_ the read() above
     because /proc files on GNU/Linux are seekable
     but have st_size == 0.  */
  if (st->st_size == 0)
    {
      /* We've filled the buffer, from a seekable file,
         which has an st_size==0, E.g., /dev/zero on GNU/Linux.
         Assume there is no limit to file size.  */
      errno = EOVERFLOW;
      return -1;
    }

  cur += size;
  off_t end;
  if (usable_st_size (st) && cur <= st->st_size)
    end = st->st_size;
  else
    {
      end = lseek (fd, 0, SEEK_END);
      if (end < 0)
        return -1;
      if (end != cur)
        {
          if (lseek (fd, cur, SEEK_SET) < 0)
            return -1;
          if (end < cur)
            end = cur;
        }
    }

  size += end - cur;
  if (size == OFF_T_MAX)
    {
      /* E.g., /dev/zero on GNU/Hurd.  */
      errno = EOVERFLOW;
      return -1;
    }

  return size;
}

/* Compute the next sequential output file name and store it into the
   string 'outfile'.  */

static void
next_file_name (void)
{
  /* Index in suffix_alphabet of each character in the suffix.  */
  static size_t *sufindex;
  static size_t outbase_length;
  static size_t outfile_length;
  static size_t addsuf_length;

  if (! outfile)
    {
      bool widen;

new_name:
      widen = !! outfile_length;

      if (! widen)
        {
          /* Allocate and initialize the first file name.  */

          outbase_length = strlen (outbase);
          addsuf_length = additional_suffix ? strlen (additional_suffix) : 0;
          outfile_length = outbase_length + suffix_length + addsuf_length;
        }
      else
        {
          /* Reallocate and initialize a new wider file name.
             We do this by subsuming the unchanging part of
             the generated suffix into the prefix (base), and
             reinitializing the now one longer suffix.  */

          outfile_length += 2;
          suffix_length++;
        }

      if (outfile_length + 1 < outbase_length)
        xalloc_die ();
      outfile = xrealloc (outfile, outfile_length + 1);

      if (! widen)
        memcpy (outfile, outbase, outbase_length);
      else
        {
          /* Append the last alphabet character to the file name prefix.  */
          outfile[outbase_length] = suffix_alphabet[sufindex[0]];
          outbase_length++;
        }

      outfile_mid = outfile + outbase_length;
      memset (outfile_mid, suffix_alphabet[0], suffix_length);
      if (additional_suffix)
        memcpy (outfile_mid + suffix_length, additional_suffix, addsuf_length);
      outfile[outfile_length] = 0;

      free (sufindex);
      sufindex = xcalloc (suffix_length, sizeof *sufindex);

      if (numeric_suffix_start)
        {
          assert (! widen);

          /* Update the output file name.  */
          size_t i = strlen (numeric_suffix_start);
          memcpy (outfile_mid + suffix_length - i, numeric_suffix_start, i);

          /* Update the suffix index.  */
          size_t *sufindex_end = sufindex + suffix_length;
          while (i-- != 0)
            *--sufindex_end = numeric_suffix_start[i] - '0';
        }

#if ! _POSIX_NO_TRUNC && HAVE_PATHCONF && defined _PC_NAME_MAX
      /* POSIX requires that if the output file name is too long for
         its directory, 'split' must fail without creating any files.
         This must be checked for explicitly on operating systems that
         silently truncate file names.  */
      {
        char *dir = dir_name (outfile);
        long name_max = pathconf (dir, _PC_NAME_MAX);
        if (0 <= name_max && name_max < base_len (last_component (outfile)))
          die (EXIT_FAILURE, ENAMETOOLONG, "%s", quotef (outfile));
        free (dir);
      }
#endif
    }
  else
    {
      /* Increment the suffix in place, if possible.  */

      size_t i = suffix_length;
      while (i-- != 0)
        {
          sufindex[i]++;
          if (suffix_auto && i == 0 && ! suffix_alphabet[sufindex[0] + 1])
            goto new_name;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
          if (outfile_mid[i])
            return;
          sufindex[i] = 0;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
        }
      die (EXIT_FAILURE, 0, _("output file suffixes exhausted"));
    }
}

/* Create or truncate a file.  */

static int
create (const char *name)
{
  if (!filter_command)
    {
      if (verbose)
        fprintf (stdout, _("creating file %s\n"), quoteaf (name));

      int fd = open (name, O_WRONLY | O_CREAT | O_BINARY, MODE_RW_UGO);
      if (fd < 0)
        return fd;
      struct stat out_stat_buf;
      if (fstat (fd, &out_stat_buf) != 0)
        die (EXIT_FAILURE, errno, _("failed to stat %s"), quoteaf (name));
      if (SAME_INODE (in_stat_buf, out_stat_buf))
        die (EXIT_FAILURE, 0, _("%s would overwrite input; aborting"),
             quoteaf (name));
      if (ftruncate (fd, 0) != 0)
        die (EXIT_FAILURE, errno, _("%s: error truncating"), quotef (name));

      return fd;
    }
  else
    {
      int fd_pair[2];
      pid_t child_pid;
      char const *shell_prog = getenv ("SHELL");
      if (shell_prog == NULL)
        shell_prog = "/bin/sh";
      if (setenv ("FILE", name, 1) != 0)
        die (EXIT_FAILURE, errno,
             _("failed to set FILE environment variable"));
      if (verbose)
        fprintf (stdout, _("executing with FILE=%s\n"), quotef (name));
      if (pipe (fd_pair) != 0)
        die (EXIT_FAILURE, errno, _("failed to create pipe"));
      child_pid = fork ();
      if (child_pid == 0)
        {
          /* This is the child process.  If an error occurs here, the
             parent will eventually learn about it after doing a wait,
             at which time it will emit its own error message.  */
          int j;
          /* We have to close any pipes that were opened during an
             earlier call, otherwise this process will be holding a
             write-pipe that will prevent the earlier process from
             reading an EOF on the corresponding read-pipe.  */
          for (j = 0; j < n_open_pipes; ++j)
            if (close (open_pipes[j]) != 0)
              die (EXIT_FAILURE, errno, _("closing prior pipe"));
          if (close (fd_pair[1]))
            die (EXIT_FAILURE, errno, _("closing output pipe"));
          if (fd_pair[0] != STDIN_FILENO)
            {
              if (dup2 (fd_pair[0], STDIN_FILENO) != STDIN_FILENO)
                die (EXIT_FAILURE, errno, _("moving input pipe"));
              if (close (fd_pair[0]) != 0)
                die (EXIT_FAILURE, errno, _("closing input pipe"));
            }
          sigprocmask (SIG_SETMASK, &oldblocked, NULL);
          execl (shell_prog, last_component (shell_prog), "-c",
                 filter_command, (char *) NULL);
          die (EXIT_FAILURE, errno, _("failed to run command: \"%s -c %s\""),
               shell_prog, filter_command);
        }
      if (child_pid == -1)
        die (EXIT_FAILURE, errno, _("fork system call failed"));
      if (close (fd_pair[0]) != 0)
        die (EXIT_FAILURE, errno, _("failed to close input pipe"));
      filter_pid = child_pid;
      if (n_open_pipes == open_pipes_alloc)
        open_pipes = x2nrealloc (open_pipes, &open_pipes_alloc,
                                 sizeof *open_pipes);
      open_pipes[n_open_pipes++] = fd_pair[1];
      return fd_pair[1];
    }
}

/* Close the output file, and do any associated cleanup.
   If FP and FD are both specified, they refer to the same open file;
   in this case FP is closed, but FD is still used in cleanup.  */
static void
closeout (FILE *fp, int fd, pid_t pid, char const *name)
{
  if (fp != NULL && fclose (fp) != 0 && ! ignorable (errno))
    die (EXIT_FAILURE, errno, "%s", quotef (name));
  if (fd >= 0)
    {
      if (fp == NULL && close (fd) < 0)
        die (EXIT_FAILURE, errno, "%s", quotef (name));
      int j;
      for (j = 0; j < n_open_pipes; ++j)
        {
          if (open_pipes[j] == fd)
            {
              open_pipes[j] = open_pipes[--n_open_pipes];
              break;
            }
        }
    }
  if (pid > 0)
    {
      int wstatus = 0;
      if (waitpid (pid, &wstatus, 0) == -1 && errno != ECHILD)
        die (EXIT_FAILURE, errno, _("waiting for child process"));
      if (WIFSIGNALED (wstatus))
        {
          int sig = WTERMSIG (wstatus);
          if (sig != SIGPIPE)
            {
              char signame[MAX (SIG2STR_MAX, INT_BUFSIZE_BOUND (int))];
              if (sig2str (sig, signame) != 0)
                sprintf (signame, "%d", sig);
              error (sig + 128, 0,
                     _("with FILE=%s, signal %s from command: %s"),
                     quotef (name), signame, filter_command);
            }
        }
      else if (WIFEXITED (wstatus))
        {
          int ex = WEXITSTATUS (wstatus);
          if (ex != 0)
            error (ex, 0, _("with FILE=%s, exit %d from command: %s"),
                   quotef (name), ex, filter_command);
        }
      else
        {
          /* shouldn't happen.  */
          die (EXIT_FAILURE, 0,
               _("unknown status from command (0x%X)"), wstatus + 0u);
        }
    }
}

/* Write BYTES bytes at BP to an output file.
   If NEW_FILE_FLAG is true, open the next output file.
   Otherwise add to the same output file already in use.
   Return true if successful.  */

static bool
cwrite (bool new_file_flag, const char *bp, size_t bytes)
{
  if (new_file_flag)
    {
      if (!bp && bytes == 0 && elide_empty_files)
        return true;
      closeout (NULL, output_desc, filter_pid, outfile);
      next_file_name ();
      output_desc = create (outfile);
      if (output_desc < 0)
        die (EXIT_FAILURE, errno, "%s", quotef (outfile));
    }

  if (full_write (output_desc, bp, bytes) == bytes)
    return true;
  else
    {
      if (! ignorable (errno))
        die (EXIT_FAILURE, errno, "%s", quotef (outfile));
      return false;
    }
}

/* Split into pieces of exactly N_BYTES bytes.
   Use buffer BUF, whose size is BUFSIZE.
   BUF contains the first INITIAL_READ input bytes.  */

static void
bytes_split (uintmax_t n_bytes, char *buf, size_t bufsize, size_t initial_read,
             uintmax_t max_files)
{
  size_t n_read;
  bool new_file_flag = true;
  uintmax_t to_write = n_bytes;
  uintmax_t opened = 0;
  bool eof;

  do
    {
      if (initial_read != SIZE_MAX)
        {
          n_read = initial_read;
          initial_read = SIZE_MAX;
          eof = n_read < bufsize;
        }
      else
        {
          n_read = safe_read (STDIN_FILENO, buf, bufsize);
          if (n_read == SAFE_READ_ERROR)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
          eof = n_read == 0;
        }
      char *bp_out = buf;
      size_t to_read = n_read;
      while (to_write <= to_read)
        {
          size_t w = to_write;
          bool cwrite_ok = cwrite (new_file_flag, bp_out, w);
          opened += new_file_flag;
          new_file_flag = !max_files || (opened < max_files);
          if (!new_file_flag && !cwrite_ok)
            {
              /* If filter no longer accepting input, stop reading.  */
              n_read = to_read = 0;
              break;
            }
          bp_out += w;
          to_read -= w;
          to_write = n_bytes;
        }
      if (to_read != 0)
        {
          bool cwrite_ok = cwrite (new_file_flag, bp_out, to_read);
          opened += new_file_flag;
          to_write -= to_read;
          new_file_flag = false;
          if (!cwrite_ok)
            {
              /* If filter no longer accepting input, stop reading.  */
              n_read = 0;
              break;
            }
        }
    }
  while (! eof);

  /* Ensure NUMBER files are created, which truncates
     any existing files or notifies any consumers on fifos.
     FIXME: Should we do this before EXIT_FAILURE?  */
  while (opened++ < max_files)
    cwrite (true, NULL, 0);
}

/* Split into pieces of exactly N_LINES lines.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
lines_split (uintmax_t n_lines, char *buf, size_t bufsize)
{
  size_t n_read;
  char *bp, *bp_out, *eob;
  bool new_file_flag = true;
  uintmax_t n = 0;

  do
    {
      n_read = safe_read (STDIN_FILENO, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      bp = bp_out = buf;
      eob = bp + n_read;
      *eob = eolchar;
      while (true)
        {
          bp = memchr (bp, eolchar, eob - bp + 1);
          if (bp == eob)
            {
              if (eob != bp_out) /* do not write 0 bytes! */
                {
                  size_t len = eob - bp_out;
                  cwrite (new_file_flag, bp_out, len);
                  new_file_flag = false;
                }
              break;
            }

          ++bp;
          if (++n >= n_lines)
            {
              cwrite (new_file_flag, bp_out, bp - bp_out);
              bp_out = bp;
              new_file_flag = true;
              n = 0;
            }
        }
    }
  while (n_read);
}

/* Split into pieces that are as large as possible while still not more
   than N_BYTES bytes, and are split on line boundaries except
   where lines longer than N_BYTES bytes occur. */

static void
line_bytes_split (uintmax_t n_bytes, char *buf, size_t bufsize)
{
  size_t n_read;
  uintmax_t n_out = 0;      /* for each split.  */
  size_t n_hold = 0;
  char *hold = NULL;        /* for lines > bufsize.  */
  size_t hold_size = 0;
  bool split_line = false;  /* Whether a \n was output in a split.  */

  do
    {
      n_read = safe_read (STDIN_FILENO, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      size_t n_left = n_read;
      char *sob = buf;
      while (n_left)
        {
          size_t split_rest = 0;
          char *eoc = NULL;
          char *eol;

          /* Determine End Of Chunk and/or End of Line,
             which are used below to select what to write or buffer.  */
          if (n_bytes - n_out - n_hold <= n_left)
            {
              /* Have enough for split.  */
              split_rest = n_bytes - n_out - n_hold;
              eoc = sob + split_rest - 1;
              eol = memrchr (sob, eolchar, split_rest);
            }
          else
            eol = memrchr (sob, eolchar, n_left);

          /* Output hold space if possible.  */
          if (n_hold && !(!eol && n_out))
            {
              cwrite (n_out == 0, hold, n_hold);
              n_out += n_hold;
              if (n_hold > bufsize)
                hold = xrealloc (hold, bufsize);
              n_hold = 0;
              hold_size = bufsize;
            }

          /* Output to eol if present.  */
          if (eol)
            {
              split_line = true;
              size_t n_write = eol - sob + 1;
              cwrite (n_out == 0, sob, n_write);
              n_out += n_write;
              n_left -= n_write;
              sob += n_write;
              if (eoc)
                split_rest -= n_write;
            }

          /* Output to eoc or eob if possible.  */
          if (n_left && !split_line)
            {
              size_t n_write = eoc ? split_rest : n_left;
              cwrite (n_out == 0, sob, n_write);
              n_out += n_write;
              n_left -= n_write;
              sob += n_write;
              if (eoc)
                split_rest -= n_write;
            }

          /* Update hold if needed.  */
          if ((eoc && split_rest) || (!eoc && n_left))
            {
              size_t n_buf = eoc ? split_rest : n_left;
              if (hold_size - n_hold < n_buf)
                {
                  if (hold_size <= SIZE_MAX - bufsize)
                    hold_size += bufsize;
                  else
                    xalloc_die ();
                  hold = xrealloc (hold, hold_size);
                }
              memcpy (hold + n_hold, sob, n_buf);
              n_hold += n_buf;
              n_left -= n_buf;
              sob += n_buf;
            }

          /* Reset for new split.  */
          if (eoc)
            {
              n_out = 0;
              split_line = false;
            }
        }
    }
  while (n_read);

  /* Handle no eol at end of file.  */
  if (n_hold)
    cwrite (n_out == 0, hold, n_hold);

  free (hold);
}

/* -n l/[K/]N: Write lines to files of approximately file size / N.
   The file is partitioned into file size / N sized portions, with the
   last assigned any excess.  If a line _starts_ within a partition
   it is written completely to the corresponding file.  Since lines
   are not split even if they overlap a partition, the files written
   can be larger or smaller than the partition size, and even empty
   if a line is so long as to completely overlap the partition.  */

static void
lines_chunk_split (uintmax_t k, uintmax_t n, char *buf, size_t bufsize,
                   size_t initial_read, off_t file_size)
{
  assert (n && k <= n && n <= file_size);

  const off_t chunk_size = file_size / n;
  uintmax_t chunk_no = 1;
  off_t chunk_end = chunk_size - 1;
  off_t n_written = 0;
  bool new_file_flag = true;
  bool chunk_truncated = false;

  if (k > 1)
    {
      /* Start reading 1 byte before kth chunk of file.  */
      off_t start = (k - 1) * chunk_size - 1;
      if (start < initial_read)
        {
          memmove (buf, buf + start, initial_read - start);
          initial_read -= start;
        }
      else
        {
          if (lseek (STDIN_FILENO, start - initial_read, SEEK_CUR) < 0)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
          initial_read = SIZE_MAX;
        }
      n_written = start;
      chunk_no = k - 1;
      chunk_end = chunk_no * chunk_size - 1;
    }

  while (n_written < file_size)
    {
      char *bp = buf, *eob;
      size_t n_read;
      if (initial_read != SIZE_MAX)
        {
          n_read = initial_read;
          initial_read = SIZE_MAX;
        }
      else
        {
          n_read = safe_read (STDIN_FILENO, buf, bufsize);
          if (n_read == SAFE_READ_ERROR)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
        }
      if (n_read == 0)
        break; /* eof.  */
      n_read = MIN (n_read, file_size - n_written);
      chunk_truncated = false;
      eob = buf + n_read;

      while (bp != eob)
        {
          size_t to_write;
          bool next = false;

          /* Begin looking for '\n' at last byte of chunk.  */
          off_t skip = MIN (n_read, MAX (0, chunk_end - n_written));
          char *bp_out = memchr (bp + skip, eolchar, n_read - skip);
          if (bp_out++)
            next = true;
          else
            bp_out = eob;
          to_write = bp_out - bp;

          if (k == chunk_no)
            {
              /* We don't use the stdout buffer here since we're writing
                 large chunks from an existing file, so it's more efficient
                 to write out directly.  */
              if (full_write (STDOUT_FILENO, bp, to_write) != to_write)
                die (EXIT_FAILURE, errno, "%s", _("write error"));
            }
          else if (! k)
            cwrite (new_file_flag, bp, to_write);
          n_written += to_write;
          bp += to_write;
          n_read -= to_write;
          new_file_flag = next;

          /* A line could have been so long that it skipped
             entire chunks. So create empty files in that case.  */
          while (next || chunk_end <= n_written - 1)
            {
              if (!next && bp == eob)
                {
                  /* replenish buf, before going to next chunk.  */
                  chunk_truncated = true;
                  break;
                }
              chunk_no++;
              if (k && chunk_no > k)
                return;
              if (chunk_no == n)
                chunk_end = file_size - 1; /* >= chunk_size.  */
              else
                chunk_end += chunk_size;
              if (chunk_end <= n_written - 1)
                {
                  if (! k)
                    cwrite (true, NULL, 0);
                }
              else
                next = false;
            }
        }
    }

  if (chunk_truncated)
    chunk_no++;

  /* Ensure NUMBER files are created, which truncates
     any existing files or notifies any consumers on fifos.
     FIXME: Should we do this before EXIT_FAILURE?  */
  while (!k && chunk_no++ <= n)
    cwrite (true, NULL, 0);
}

/* -n K/N: Extract Kth of N chunks.  */

static void
bytes_chunk_extract (uintmax_t k, uintmax_t n, char *buf, size_t bufsize,
                     size_t initial_read, off_t file_size)
{
  off_t start;
  off_t end;

  assert (k && n && k <= n && n <= file_size);

  start = (k - 1) * (file_size / n);
  end = (k == n) ? file_size : k * (file_size / n);

	int uni_klee_pc;
	klee_make_symbolic(&uni_klee_pc, sizeof(uni_klee_pc), "uni_klee_pc");
	if (uni_klee_pc == 0) {
		klee_assume((end == 0));
		exit(0);
	}
	if (uni_klee_pc == 1) {
		klee_assume((start == 0));
		exit(0);
	}
	if (uni_klee_pc == 2) {
		klee_assume((bufsize != 0));
		exit(0);
	}
	if (uni_klee_pc == 3) {
		klee_assume((file_size != 0));
		exit(0);
	}
	if (uni_klee_pc == 4) {
		klee_assume((k != 0));
		exit(0);
	}
	if (uni_klee_pc == 5) {
		klee_assume((n != 0));
		exit(0);
	}
	if (uni_klee_pc == 6) {
		klee_assume((bufsize != -10));
		exit(0);
	}
	if (uni_klee_pc == 7) {
		klee_assume((bufsize != -9));
		exit(0);
	}
	if (uni_klee_pc == 8) {
		klee_assume((bufsize != -8));
		exit(0);
	}
	if (uni_klee_pc == 9) {
		klee_assume((bufsize != -7));
		exit(0);
	}
	if (uni_klee_pc == 10) {
		klee_assume((bufsize != -6));
		exit(0);
	}
	if (uni_klee_pc == 11) {
		klee_assume((bufsize != -5));
		exit(0);
	}
	if (uni_klee_pc == 12) {
		klee_assume((bufsize != -4));
		exit(0);
	}
	if (uni_klee_pc == 13) {
		klee_assume((bufsize != -3));
		exit(0);
	}
	if (uni_klee_pc == 14) {
		klee_assume((bufsize != -2));
		exit(0);
	}
	if (uni_klee_pc == 15) {
		klee_assume((bufsize != -1));
		exit(0);
	}
	if (uni_klee_pc == 16) {
		klee_assume((bufsize != 1));
		exit(0);
	}
	if (uni_klee_pc == 17) {
		klee_assume((bufsize != 2));
		exit(0);
	}
	if (uni_klee_pc == 18) {
		klee_assume((bufsize != 3));
		exit(0);
	}
	if (uni_klee_pc == 19) {
		klee_assume((bufsize != 4));
		exit(0);
	}
	if (uni_klee_pc == 20) {
		klee_assume((bufsize != 5));
		exit(0);
	}
	if (uni_klee_pc == 21) {
		klee_assume((bufsize != 6));
		exit(0);
	}
	if (uni_klee_pc == 22) {
		klee_assume((bufsize != 7));
		exit(0);
	}
	if (uni_klee_pc == 23) {
		klee_assume((bufsize != 8));
		exit(0);
	}
	if (uni_klee_pc == 24) {
		klee_assume((bufsize != 9));
		exit(0);
	}
	if (uni_klee_pc == 25) {
		klee_assume((bufsize != 10));
		exit(0);
	}
	if (uni_klee_pc == 26) {
		klee_assume((bufsize != 16));
		exit(0);
	}
	if (uni_klee_pc == 27) {
		klee_assume((bufsize != 32));
		exit(0);
	}
	if (uni_klee_pc == 28) {
		klee_assume((bufsize != 64));
		exit(0);
	}
	if (uni_klee_pc == 29) {
		klee_assume((bufsize != 128));
		exit(0);
	}
	if (uni_klee_pc == 30) {
		klee_assume((bufsize != 256));
		exit(0);
	}
	if (uni_klee_pc == 31) {
		klee_assume((bufsize != 512));
		exit(0);
	}
	if (uni_klee_pc == 32) {
		klee_assume((bufsize != 1024));
		exit(0);
	}
	if (uni_klee_pc == 33) {
		klee_assume((bufsize != 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 34) {
		klee_assume((bufsize != 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 35) {
		klee_assume((end != -10));
		exit(0);
	}
	if (uni_klee_pc == 36) {
		klee_assume((end != -9));
		exit(0);
	}
	if (uni_klee_pc == 37) {
		klee_assume((end != -8));
		exit(0);
	}
	if (uni_klee_pc == 38) {
		klee_assume((end != -7));
		exit(0);
	}
	if (uni_klee_pc == 39) {
		klee_assume((end != -6));
		exit(0);
	}
	if (uni_klee_pc == 40) {
		klee_assume((end != -5));
		exit(0);
	}
	if (uni_klee_pc == 41) {
		klee_assume((end != -4));
		exit(0);
	}
	if (uni_klee_pc == 42) {
		klee_assume((end != -3));
		exit(0);
	}
	if (uni_klee_pc == 43) {
		klee_assume((end != -2));
		exit(0);
	}
	if (uni_klee_pc == 44) {
		klee_assume((end != -1));
		exit(0);
	}
	if (uni_klee_pc == 45) {
		klee_assume((end != 1));
		exit(0);
	}
	if (uni_klee_pc == 46) {
		klee_assume((end != 2));
		exit(0);
	}
	if (uni_klee_pc == 47) {
		klee_assume((end != 3));
		exit(0);
	}
	if (uni_klee_pc == 48) {
		klee_assume((end != 4));
		exit(0);
	}
	if (uni_klee_pc == 49) {
		klee_assume((end != 5));
		exit(0);
	}
	if (uni_klee_pc == 50) {
		klee_assume((end != 6));
		exit(0);
	}
	if (uni_klee_pc == 51) {
		klee_assume((end != 7));
		exit(0);
	}
	if (uni_klee_pc == 52) {
		klee_assume((end != 8));
		exit(0);
	}
	if (uni_klee_pc == 53) {
		klee_assume((end != 9));
		exit(0);
	}
	if (uni_klee_pc == 54) {
		klee_assume((end != 10));
		exit(0);
	}
	if (uni_klee_pc == 55) {
		klee_assume((end != 16));
		exit(0);
	}
	if (uni_klee_pc == 56) {
		klee_assume((end != 32));
		exit(0);
	}
	if (uni_klee_pc == 57) {
		klee_assume((end != 64));
		exit(0);
	}
	if (uni_klee_pc == 58) {
		klee_assume((end != 128));
		exit(0);
	}
	if (uni_klee_pc == 59) {
		klee_assume((end != 256));
		exit(0);
	}
	if (uni_klee_pc == 60) {
		klee_assume((end != 512));
		exit(0);
	}
	if (uni_klee_pc == 61) {
		klee_assume((end != 1024));
		exit(0);
	}
	if (uni_klee_pc == 62) {
		klee_assume((end != 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 63) {
		klee_assume((end != 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 64) {
		klee_assume((file_size != -10));
		exit(0);
	}
	if (uni_klee_pc == 65) {
		klee_assume((file_size != -9));
		exit(0);
	}
	if (uni_klee_pc == 66) {
		klee_assume((file_size != -8));
		exit(0);
	}
	if (uni_klee_pc == 67) {
		klee_assume((file_size != -7));
		exit(0);
	}
	if (uni_klee_pc == 68) {
		klee_assume((file_size != -6));
		exit(0);
	}
	if (uni_klee_pc == 69) {
		klee_assume((file_size != -5));
		exit(0);
	}
	if (uni_klee_pc == 70) {
		klee_assume((file_size != -4));
		exit(0);
	}
	if (uni_klee_pc == 71) {
		klee_assume((file_size != -3));
		exit(0);
	}
	if (uni_klee_pc == 72) {
		klee_assume((file_size != -2));
		exit(0);
	}
	if (uni_klee_pc == 73) {
		klee_assume((file_size != -1));
		exit(0);
	}
	if (uni_klee_pc == 74) {
		klee_assume((file_size != 1));
		exit(0);
	}
	if (uni_klee_pc == 75) {
		klee_assume((file_size != 2));
		exit(0);
	}
	if (uni_klee_pc == 76) {
		klee_assume((file_size != 3));
		exit(0);
	}
	if (uni_klee_pc == 77) {
		klee_assume((file_size != 4));
		exit(0);
	}
	if (uni_klee_pc == 78) {
		klee_assume((file_size != 5));
		exit(0);
	}
	if (uni_klee_pc == 79) {
		klee_assume((file_size != 6));
		exit(0);
	}
	if (uni_klee_pc == 80) {
		klee_assume((file_size != 8));
		exit(0);
	}
	if (uni_klee_pc == 81) {
		klee_assume((file_size != 9));
		exit(0);
	}
	if (uni_klee_pc == 82) {
		klee_assume((file_size != 10));
		exit(0);
	}
	if (uni_klee_pc == 83) {
		klee_assume((file_size != 16));
		exit(0);
	}
	if (uni_klee_pc == 84) {
		klee_assume((file_size != 32));
		exit(0);
	}
	if (uni_klee_pc == 85) {
		klee_assume((file_size != 64));
		exit(0);
	}
	if (uni_klee_pc == 86) {
		klee_assume((file_size != 128));
		exit(0);
	}
	if (uni_klee_pc == 87) {
		klee_assume((file_size != 256));
		exit(0);
	}
	if (uni_klee_pc == 88) {
		klee_assume((file_size != 512));
		exit(0);
	}
	if (uni_klee_pc == 89) {
		klee_assume((file_size != 1024));
		exit(0);
	}
	if (uni_klee_pc == 90) {
		klee_assume((file_size != 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 91) {
		klee_assume((file_size != 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 92) {
		klee_assume((k != -10));
		exit(0);
	}
	if (uni_klee_pc == 93) {
		klee_assume((k != -9));
		exit(0);
	}
	if (uni_klee_pc == 94) {
		klee_assume((k != -8));
		exit(0);
	}
	if (uni_klee_pc == 95) {
		klee_assume((k != -7));
		exit(0);
	}
	if (uni_klee_pc == 96) {
		klee_assume((k != -6));
		exit(0);
	}
	if (uni_klee_pc == 97) {
		klee_assume((k != -5));
		exit(0);
	}
	if (uni_klee_pc == 98) {
		klee_assume((k != -4));
		exit(0);
	}
	if (uni_klee_pc == 99) {
		klee_assume((k != -3));
		exit(0);
	}
	if (uni_klee_pc == 100) {
		klee_assume((k != -2));
		exit(0);
	}
	if (uni_klee_pc == 101) {
		klee_assume((k != -1));
		exit(0);
	}
	if (uni_klee_pc == 102) {
		klee_assume((k != 10));
		exit(0);
	}
	if (uni_klee_pc == 103) {
		klee_assume((k != 16));
		exit(0);
	}
	if (uni_klee_pc == 104) {
		klee_assume((k != 32));
		exit(0);
	}
	if (uni_klee_pc == 105) {
		klee_assume((k != 64));
		exit(0);
	}
	if (uni_klee_pc == 106) {
		klee_assume((k != 128));
		exit(0);
	}
	if (uni_klee_pc == 107) {
		klee_assume((k != 256));
		exit(0);
	}
	if (uni_klee_pc == 108) {
		klee_assume((k != 512));
		exit(0);
	}
	if (uni_klee_pc == 109) {
		klee_assume((k != 1024));
		exit(0);
	}
	if (uni_klee_pc == 110) {
		klee_assume((k != 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 111) {
		klee_assume((k != 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 112) {
		klee_assume((n != -10));
		exit(0);
	}
	if (uni_klee_pc == 113) {
		klee_assume((n != -9));
		exit(0);
	}
	if (uni_klee_pc == 114) {
		klee_assume((n != -8));
		exit(0);
	}
	if (uni_klee_pc == 115) {
		klee_assume((n != -7));
		exit(0);
	}
	if (uni_klee_pc == 116) {
		klee_assume((n != -6));
		exit(0);
	}
	if (uni_klee_pc == 117) {
		klee_assume((n != -5));
		exit(0);
	}
	if (uni_klee_pc == 118) {
		klee_assume((n != -4));
		exit(0);
	}
	if (uni_klee_pc == 119) {
		klee_assume((n != -3));
		exit(0);
	}
	if (uni_klee_pc == 120) {
		klee_assume((n != -2));
		exit(0);
	}
	if (uni_klee_pc == 121) {
		klee_assume((n != -1));
		exit(0);
	}
	if (uni_klee_pc == 122) {
		klee_assume((n != 1));
		exit(0);
	}
	if (uni_klee_pc == 123) {
		klee_assume((n != 2));
		exit(0);
	}
	if (uni_klee_pc == 124) {
		klee_assume((n != 3));
		exit(0);
	}
	if (uni_klee_pc == 125) {
		klee_assume((n != 4));
		exit(0);
	}
	if (uni_klee_pc == 126) {
		klee_assume((n != 5));
		exit(0);
	}
	if (uni_klee_pc == 127) {
		klee_assume((n != 6));
		exit(0);
	}
	if (uni_klee_pc == 128) {
		klee_assume((n != 8));
		exit(0);
	}
	if (uni_klee_pc == 129) {
		klee_assume((n != 9));
		exit(0);
	}
	if (uni_klee_pc == 130) {
		klee_assume((n != 10));
		exit(0);
	}
	if (uni_klee_pc == 131) {
		klee_assume((n != 16));
		exit(0);
	}
	if (uni_klee_pc == 132) {
		klee_assume((n != 32));
		exit(0);
	}
	if (uni_klee_pc == 133) {
		klee_assume((n != 64));
		exit(0);
	}
	if (uni_klee_pc == 134) {
		klee_assume((n != 128));
		exit(0);
	}
	if (uni_klee_pc == 135) {
		klee_assume((n != 256));
		exit(0);
	}
	if (uni_klee_pc == 136) {
		klee_assume((n != 512));
		exit(0);
	}
	if (uni_klee_pc == 137) {
		klee_assume((n != 1024));
		exit(0);
	}
	if (uni_klee_pc == 138) {
		klee_assume((n != 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 139) {
		klee_assume((n != 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 140) {
		klee_assume((start != -10));
		exit(0);
	}
	if (uni_klee_pc == 141) {
		klee_assume((start != -9));
		exit(0);
	}
	if (uni_klee_pc == 142) {
		klee_assume((start != -8));
		exit(0);
	}
	if (uni_klee_pc == 143) {
		klee_assume((start != -7));
		exit(0);
	}
	if (uni_klee_pc == 144) {
		klee_assume((start != -6));
		exit(0);
	}
	if (uni_klee_pc == 145) {
		klee_assume((start != -5));
		exit(0);
	}
	if (uni_klee_pc == 146) {
		klee_assume((start != -4));
		exit(0);
	}
	if (uni_klee_pc == 147) {
		klee_assume((start != -3));
		exit(0);
	}
	if (uni_klee_pc == 148) {
		klee_assume((start != -2));
		exit(0);
	}
	if (uni_klee_pc == 149) {
		klee_assume((start != -1));
		exit(0);
	}
	if (uni_klee_pc == 150) {
		klee_assume((start != 1));
		exit(0);
	}
	if (uni_klee_pc == 151) {
		klee_assume((start != 2));
		exit(0);
	}
	if (uni_klee_pc == 152) {
		klee_assume((start != 3));
		exit(0);
	}
	if (uni_klee_pc == 153) {
		klee_assume((start != 4));
		exit(0);
	}
	if (uni_klee_pc == 154) {
		klee_assume((start != 5));
		exit(0);
	}
	if (uni_klee_pc == 155) {
		klee_assume((start != 6));
		exit(0);
	}
	if (uni_klee_pc == 156) {
		klee_assume((start != 7));
		exit(0);
	}
	if (uni_klee_pc == 157) {
		klee_assume((start != 8));
		exit(0);
	}
	if (uni_klee_pc == 158) {
		klee_assume((start != 9));
		exit(0);
	}
	if (uni_klee_pc == 159) {
		klee_assume((start != 10));
		exit(0);
	}
	if (uni_klee_pc == 160) {
		klee_assume((start != 16));
		exit(0);
	}
	if (uni_klee_pc == 161) {
		klee_assume((start != 32));
		exit(0);
	}
	if (uni_klee_pc == 162) {
		klee_assume((start != 64));
		exit(0);
	}
	if (uni_klee_pc == 163) {
		klee_assume((start != 128));
		exit(0);
	}
	if (uni_klee_pc == 164) {
		klee_assume((start != 256));
		exit(0);
	}
	if (uni_klee_pc == 165) {
		klee_assume((start != 512));
		exit(0);
	}
	if (uni_klee_pc == 166) {
		klee_assume((start != 1024));
		exit(0);
	}
	if (uni_klee_pc == 167) {
		klee_assume((start != 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 168) {
		klee_assume((start != 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 169) {
		klee_assume((bufsize >= -10));
		exit(0);
	}
	if (uni_klee_pc == 170) {
		klee_assume((bufsize >= -9));
		exit(0);
	}
	if (uni_klee_pc == 171) {
		klee_assume((bufsize >= -8));
		exit(0);
	}
	if (uni_klee_pc == 172) {
		klee_assume((bufsize >= -7));
		exit(0);
	}
	if (uni_klee_pc == 173) {
		klee_assume((bufsize >= -6));
		exit(0);
	}
	if (uni_klee_pc == 174) {
		klee_assume((bufsize >= -5));
		exit(0);
	}
	if (uni_klee_pc == 175) {
		klee_assume((bufsize >= -4));
		exit(0);
	}
	if (uni_klee_pc == 176) {
		klee_assume((bufsize >= -3));
		exit(0);
	}
	if (uni_klee_pc == 177) {
		klee_assume((bufsize >= -2));
		exit(0);
	}
	if (uni_klee_pc == 178) {
		klee_assume((bufsize >= -1));
		exit(0);
	}
	if (uni_klee_pc == 179) {
		klee_assume((bufsize >= 0));
		exit(0);
	}
	if (uni_klee_pc == 180) {
		klee_assume((bufsize >= 1));
		exit(0);
	}
	if (uni_klee_pc == 181) {
		klee_assume((bufsize >= 2));
		exit(0);
	}
	if (uni_klee_pc == 182) {
		klee_assume((bufsize >= 3));
		exit(0);
	}
	if (uni_klee_pc == 183) {
		klee_assume((bufsize >= 4));
		exit(0);
	}
	if (uni_klee_pc == 184) {
		klee_assume((bufsize >= 5));
		exit(0);
	}
	if (uni_klee_pc == 185) {
		klee_assume((bufsize >= 6));
		exit(0);
	}
	if (uni_klee_pc == 186) {
		klee_assume((bufsize >= 7));
		exit(0);
	}
	if (uni_klee_pc == 187) {
		klee_assume((bufsize >= 8));
		exit(0);
	}
	if (uni_klee_pc == 188) {
		klee_assume((bufsize >= 9));
		exit(0);
	}
	if (uni_klee_pc == 189) {
		klee_assume((bufsize >= 10));
		exit(0);
	}
	if (uni_klee_pc == 190) {
		klee_assume((bufsize >= 16));
		exit(0);
	}
	if (uni_klee_pc == 191) {
		klee_assume((bufsize >= 32));
		exit(0);
	}
	if (uni_klee_pc == 192) {
		klee_assume((bufsize >= 64));
		exit(0);
	}
	if (uni_klee_pc == 193) {
		klee_assume((bufsize >= 128));
		exit(0);
	}
	if (uni_klee_pc == 194) {
		klee_assume((bufsize >= 256));
		exit(0);
	}
	if (uni_klee_pc == 195) {
		klee_assume((bufsize >= 512));
		exit(0);
	}
	if (uni_klee_pc == 196) {
		klee_assume((bufsize >= 1024));
		exit(0);
	}
	if (uni_klee_pc == 197) {
		klee_assume((end >= -10));
		exit(0);
	}
	if (uni_klee_pc == 198) {
		klee_assume((end >= -9));
		exit(0);
	}
	if (uni_klee_pc == 199) {
		klee_assume((end >= -8));
		exit(0);
	}
	if (uni_klee_pc == 200) {
		klee_assume((end >= -7));
		exit(0);
	}
	if (uni_klee_pc == 201) {
		klee_assume((end >= -6));
		exit(0);
	}
	if (uni_klee_pc == 202) {
		klee_assume((end >= -5));
		exit(0);
	}
	if (uni_klee_pc == 203) {
		klee_assume((end >= -4));
		exit(0);
	}
	if (uni_klee_pc == 204) {
		klee_assume((end >= -3));
		exit(0);
	}
	if (uni_klee_pc == 205) {
		klee_assume((end >= -2));
		exit(0);
	}
	if (uni_klee_pc == 206) {
		klee_assume((end >= -1));
		exit(0);
	}
	if (uni_klee_pc == 207) {
		klee_assume((end >= 0));
		exit(0);
	}
	if (uni_klee_pc == 208) {
		klee_assume((file_size >= -10));
		exit(0);
	}
	if (uni_klee_pc == 209) {
		klee_assume((file_size >= -9));
		exit(0);
	}
	if (uni_klee_pc == 210) {
		klee_assume((file_size >= -8));
		exit(0);
	}
	if (uni_klee_pc == 211) {
		klee_assume((file_size >= -7));
		exit(0);
	}
	if (uni_klee_pc == 212) {
		klee_assume((file_size >= -6));
		exit(0);
	}
	if (uni_klee_pc == 213) {
		klee_assume((file_size >= -5));
		exit(0);
	}
	if (uni_klee_pc == 214) {
		klee_assume((file_size >= -4));
		exit(0);
	}
	if (uni_klee_pc == 215) {
		klee_assume((file_size >= -3));
		exit(0);
	}
	if (uni_klee_pc == 216) {
		klee_assume((file_size >= -2));
		exit(0);
	}
	if (uni_klee_pc == 217) {
		klee_assume((file_size >= -1));
		exit(0);
	}
	if (uni_klee_pc == 218) {
		klee_assume((file_size >= 0));
		exit(0);
	}
	if (uni_klee_pc == 219) {
		klee_assume((file_size >= 1));
		exit(0);
	}
	if (uni_klee_pc == 220) {
		klee_assume((file_size >= 2));
		exit(0);
	}
	if (uni_klee_pc == 221) {
		klee_assume((file_size >= 3));
		exit(0);
	}
	if (uni_klee_pc == 222) {
		klee_assume((file_size >= 4));
		exit(0);
	}
	if (uni_klee_pc == 223) {
		klee_assume((file_size >= 5));
		exit(0);
	}
	if (uni_klee_pc == 224) {
		klee_assume((file_size >= 6));
		exit(0);
	}
	if (uni_klee_pc == 225) {
		klee_assume((file_size >= 7));
		exit(0);
	}
	if (uni_klee_pc == 226) {
		klee_assume((k >= -10));
		exit(0);
	}
	if (uni_klee_pc == 227) {
		klee_assume((k >= -9));
		exit(0);
	}
	if (uni_klee_pc == 228) {
		klee_assume((k >= -8));
		exit(0);
	}
	if (uni_klee_pc == 229) {
		klee_assume((k >= -7));
		exit(0);
	}
	if (uni_klee_pc == 230) {
		klee_assume((k >= -6));
		exit(0);
	}
	if (uni_klee_pc == 231) {
		klee_assume((k >= -5));
		exit(0);
	}
	if (uni_klee_pc == 232) {
		klee_assume((k >= -4));
		exit(0);
	}
	if (uni_klee_pc == 233) {
		klee_assume((k >= -3));
		exit(0);
	}
	if (uni_klee_pc == 234) {
		klee_assume((k >= -2));
		exit(0);
	}
	if (uni_klee_pc == 235) {
		klee_assume((k >= -1));
		exit(0);
	}
	if (uni_klee_pc == 236) {
		klee_assume((k >= 0));
		exit(0);
	}
	if (uni_klee_pc == 237) {
		klee_assume((k >= 1));
		exit(0);
	}
	if (uni_klee_pc == 238) {
		klee_assume((n >= -10));
		exit(0);
	}
	if (uni_klee_pc == 239) {
		klee_assume((n >= -9));
		exit(0);
	}
	if (uni_klee_pc == 240) {
		klee_assume((n >= -8));
		exit(0);
	}
	if (uni_klee_pc == 241) {
		klee_assume((n >= -7));
		exit(0);
	}
	if (uni_klee_pc == 242) {
		klee_assume((n >= -6));
		exit(0);
	}
	if (uni_klee_pc == 243) {
		klee_assume((n >= -5));
		exit(0);
	}
	if (uni_klee_pc == 244) {
		klee_assume((n >= -4));
		exit(0);
	}
	if (uni_klee_pc == 245) {
		klee_assume((n >= -3));
		exit(0);
	}
	if (uni_klee_pc == 246) {
		klee_assume((n >= -2));
		exit(0);
	}
	if (uni_klee_pc == 247) {
		klee_assume((n >= -1));
		exit(0);
	}
	if (uni_klee_pc == 248) {
		klee_assume((n >= 0));
		exit(0);
	}
	if (uni_klee_pc == 249) {
		klee_assume((n >= 1));
		exit(0);
	}
	if (uni_klee_pc == 250) {
		klee_assume((n >= 2));
		exit(0);
	}
	if (uni_klee_pc == 251) {
		klee_assume((n >= 3));
		exit(0);
	}
	if (uni_klee_pc == 252) {
		klee_assume((n >= 4));
		exit(0);
	}
	if (uni_klee_pc == 253) {
		klee_assume((n >= 5));
		exit(0);
	}
	if (uni_klee_pc == 254) {
		klee_assume((n >= 6));
		exit(0);
	}
	if (uni_klee_pc == 255) {
		klee_assume((n >= 7));
		exit(0);
	}
	if (uni_klee_pc == 256) {
		klee_assume((start >= -10));
		exit(0);
	}
	if (uni_klee_pc == 257) {
		klee_assume((start >= -9));
		exit(0);
	}
	if (uni_klee_pc == 258) {
		klee_assume((start >= -8));
		exit(0);
	}
	if (uni_klee_pc == 259) {
		klee_assume((start >= -7));
		exit(0);
	}
	if (uni_klee_pc == 260) {
		klee_assume((start >= -6));
		exit(0);
	}
	if (uni_klee_pc == 261) {
		klee_assume((start >= -5));
		exit(0);
	}
	if (uni_klee_pc == 262) {
		klee_assume((start >= -4));
		exit(0);
	}
	if (uni_klee_pc == 263) {
		klee_assume((start >= -3));
		exit(0);
	}
	if (uni_klee_pc == 264) {
		klee_assume((start >= -2));
		exit(0);
	}
	if (uni_klee_pc == 265) {
		klee_assume((start >= -1));
		exit(0);
	}
	if (uni_klee_pc == 266) {
		klee_assume((start >= 0));
		exit(0);
	}
	if (uni_klee_pc == 267) {
		klee_assume((bufsize <= 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 268) {
		klee_assume((bufsize <= 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 269) {
		klee_assume((end <= 0));
		exit(0);
	}
	if (uni_klee_pc == 270) {
		klee_assume((end <= 1));
		exit(0);
	}
	if (uni_klee_pc == 271) {
		klee_assume((end <= 2));
		exit(0);
	}
	if (uni_klee_pc == 272) {
		klee_assume((end <= 3));
		exit(0);
	}
	if (uni_klee_pc == 273) {
		klee_assume((end <= 4));
		exit(0);
	}
	if (uni_klee_pc == 274) {
		klee_assume((end <= 5));
		exit(0);
	}
	if (uni_klee_pc == 275) {
		klee_assume((end <= 6));
		exit(0);
	}
	if (uni_klee_pc == 276) {
		klee_assume((end <= 7));
		exit(0);
	}
	if (uni_klee_pc == 277) {
		klee_assume((end <= 8));
		exit(0);
	}
	if (uni_klee_pc == 278) {
		klee_assume((end <= 9));
		exit(0);
	}
	if (uni_klee_pc == 279) {
		klee_assume((end <= 10));
		exit(0);
	}
	if (uni_klee_pc == 280) {
		klee_assume((end <= 16));
		exit(0);
	}
	if (uni_klee_pc == 281) {
		klee_assume((end <= 32));
		exit(0);
	}
	if (uni_klee_pc == 282) {
		klee_assume((end <= 64));
		exit(0);
	}
	if (uni_klee_pc == 283) {
		klee_assume((end <= 128));
		exit(0);
	}
	if (uni_klee_pc == 284) {
		klee_assume((end <= 256));
		exit(0);
	}
	if (uni_klee_pc == 285) {
		klee_assume((end <= 512));
		exit(0);
	}
	if (uni_klee_pc == 286) {
		klee_assume((end <= 1024));
		exit(0);
	}
	if (uni_klee_pc == 287) {
		klee_assume((end <= 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 288) {
		klee_assume((end <= 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 289) {
		klee_assume((file_size <= 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 290) {
		klee_assume((file_size <= 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 291) {
		klee_assume((k <= 9));
		exit(0);
	}
	if (uni_klee_pc == 292) {
		klee_assume((k <= 10));
		exit(0);
	}
	if (uni_klee_pc == 293) {
		klee_assume((k <= 16));
		exit(0);
	}
	if (uni_klee_pc == 294) {
		klee_assume((k <= 32));
		exit(0);
	}
	if (uni_klee_pc == 295) {
		klee_assume((k <= 64));
		exit(0);
	}
	if (uni_klee_pc == 296) {
		klee_assume((k <= 128));
		exit(0);
	}
	if (uni_klee_pc == 297) {
		klee_assume((k <= 256));
		exit(0);
	}
	if (uni_klee_pc == 298) {
		klee_assume((k <= 512));
		exit(0);
	}
	if (uni_klee_pc == 299) {
		klee_assume((k <= 1024));
		exit(0);
	}
	if (uni_klee_pc == 300) {
		klee_assume((k <= 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 301) {
		klee_assume((k <= 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 302) {
		klee_assume((n <= 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 303) {
		klee_assume((n <= 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 304) {
		klee_assume((start <= 0));
		exit(0);
	}
	if (uni_klee_pc == 305) {
		klee_assume((start <= 1));
		exit(0);
	}
	if (uni_klee_pc == 306) {
		klee_assume((start <= 2));
		exit(0);
	}
	if (uni_klee_pc == 307) {
		klee_assume((start <= 3));
		exit(0);
	}
	if (uni_klee_pc == 308) {
		klee_assume((start <= 4));
		exit(0);
	}
	if (uni_klee_pc == 309) {
		klee_assume((start <= 5));
		exit(0);
	}
	if (uni_klee_pc == 310) {
		klee_assume((start <= 6));
		exit(0);
	}
	if (uni_klee_pc == 311) {
		klee_assume((start <= 7));
		exit(0);
	}
	if (uni_klee_pc == 312) {
		klee_assume((start <= 8));
		exit(0);
	}
	if (uni_klee_pc == 313) {
		klee_assume((start <= 9));
		exit(0);
	}
	if (uni_klee_pc == 314) {
		klee_assume((start <= 10));
		exit(0);
	}
	if (uni_klee_pc == 315) {
		klee_assume((start <= 16));
		exit(0);
	}
	if (uni_klee_pc == 316) {
		klee_assume((start <= 32));
		exit(0);
	}
	if (uni_klee_pc == 317) {
		klee_assume((start <= 64));
		exit(0);
	}
	if (uni_klee_pc == 318) {
		klee_assume((start <= 128));
		exit(0);
	}
	if (uni_klee_pc == 319) {
		klee_assume((start <= 256));
		exit(0);
	}
	if (uni_klee_pc == 320) {
		klee_assume((start <= 512));
		exit(0);
	}
	if (uni_klee_pc == 321) {
		klee_assume((start <= 1024));
		exit(0);
	}
	if (uni_klee_pc == 322) {
		klee_assume((start <= 2147483647));
		exit(0);
	}
	if (uni_klee_pc == 323) {
		klee_assume((start <= 4294967295));
		exit(0);
	}
	if (uni_klee_pc == 324) {
		klee_assume((bufsize >= end));
		exit(0);
	}
	if (uni_klee_pc == 325) {
		klee_assume((bufsize >= k));
		exit(0);
	}
	if (uni_klee_pc == 326) {
		klee_assume((bufsize >= start));
		exit(0);
	}
	if (uni_klee_pc == 327) {
		klee_assume((end >= start));
		exit(0);
	}
	if (uni_klee_pc == 328) {
		klee_assume((file_size >= end));
		exit(0);
	}
	if (uni_klee_pc == 329) {
		klee_assume((file_size >= k));
		exit(0);
	}
	if (uni_klee_pc == 330) {
		klee_assume((file_size >= n));
		exit(0);
	}
	if (uni_klee_pc == 331) {
		klee_assume((file_size >= start));
		exit(0);
	}
	if (uni_klee_pc == 332) {
		klee_assume((k >= end));
		exit(0);
	}
	if (uni_klee_pc == 333) {
		klee_assume((k >= start));
		exit(0);
	}
	if (uni_klee_pc == 334) {
		klee_assume((n >= end));
		exit(0);
	}
	if (uni_klee_pc == 335) {
		klee_assume((n >= file_size));
		exit(0);
	}
	if (uni_klee_pc == 336) {
		klee_assume((n >= k));
		exit(0);
	}
	if (uni_klee_pc == 337) {
		klee_assume((n >= start));
		exit(0);
	}
	if (uni_klee_pc == 338) {
		klee_assume((start >= end));
		exit(0);
	}
	if (uni_klee_pc == 339) {
		klee_assume(((bufsize - end) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 340) {
		klee_assume(((bufsize - end) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 341) {
		klee_assume(((bufsize - end) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 342) {
		klee_assume(((bufsize - end) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 343) {
		klee_assume(((bufsize - end) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 344) {
		klee_assume(((bufsize - end) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 345) {
		klee_assume(((bufsize - end) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 346) {
		klee_assume(((bufsize - end) >= 8));
		exit(0);
	}
	if (uni_klee_pc == 347) {
		klee_assume(((bufsize - end) >= 9));
		exit(0);
	}
	if (uni_klee_pc == 348) {
		klee_assume(((bufsize - end) >= 10));
		exit(0);
	}
	if (uni_klee_pc == 349) {
		klee_assume(((bufsize - end) >= 16));
		exit(0);
	}
	if (uni_klee_pc == 350) {
		klee_assume(((bufsize - end) >= 32));
		exit(0);
	}
	if (uni_klee_pc == 351) {
		klee_assume(((bufsize - end) >= 64));
		exit(0);
	}
	if (uni_klee_pc == 352) {
		klee_assume(((bufsize - end) >= 128));
		exit(0);
	}
	if (uni_klee_pc == 353) {
		klee_assume(((bufsize - end) >= 256));
		exit(0);
	}
	if (uni_klee_pc == 354) {
		klee_assume(((bufsize - end) >= 512));
		exit(0);
	}
	if (uni_klee_pc == 355) {
		klee_assume(((bufsize - end) >= 1024));
		exit(0);
	}
	if (uni_klee_pc == 356) {
		klee_assume(((bufsize - k) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 357) {
		klee_assume(((bufsize - k) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 358) {
		klee_assume(((bufsize - k) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 359) {
		klee_assume(((bufsize - k) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 360) {
		klee_assume(((bufsize - k) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 361) {
		klee_assume(((bufsize - k) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 362) {
		klee_assume(((bufsize - k) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 363) {
		klee_assume(((bufsize - k) >= 8));
		exit(0);
	}
	if (uni_klee_pc == 364) {
		klee_assume(((bufsize - k) >= 9));
		exit(0);
	}
	if (uni_klee_pc == 365) {
		klee_assume(((bufsize - k) >= 10));
		exit(0);
	}
	if (uni_klee_pc == 366) {
		klee_assume(((bufsize - k) >= 16));
		exit(0);
	}
	if (uni_klee_pc == 367) {
		klee_assume(((bufsize - k) >= 32));
		exit(0);
	}
	if (uni_klee_pc == 368) {
		klee_assume(((bufsize - k) >= 64));
		exit(0);
	}
	if (uni_klee_pc == 369) {
		klee_assume(((bufsize - k) >= 128));
		exit(0);
	}
	if (uni_klee_pc == 370) {
		klee_assume(((bufsize - k) >= 256));
		exit(0);
	}
	if (uni_klee_pc == 371) {
		klee_assume(((bufsize - k) >= 512));
		exit(0);
	}
	if (uni_klee_pc == 372) {
		klee_assume(((bufsize - k) >= 1024));
		exit(0);
	}
	if (uni_klee_pc == 373) {
		klee_assume(((bufsize - start) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 374) {
		klee_assume(((bufsize - start) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 375) {
		klee_assume(((bufsize - start) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 376) {
		klee_assume(((bufsize - start) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 377) {
		klee_assume(((bufsize - start) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 378) {
		klee_assume(((bufsize - start) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 379) {
		klee_assume(((bufsize - start) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 380) {
		klee_assume(((bufsize - start) >= 8));
		exit(0);
	}
	if (uni_klee_pc == 381) {
		klee_assume(((bufsize - start) >= 9));
		exit(0);
	}
	if (uni_klee_pc == 382) {
		klee_assume(((bufsize - start) >= 10));
		exit(0);
	}
	if (uni_klee_pc == 383) {
		klee_assume(((bufsize - start) >= 16));
		exit(0);
	}
	if (uni_klee_pc == 384) {
		klee_assume(((bufsize - start) >= 32));
		exit(0);
	}
	if (uni_klee_pc == 385) {
		klee_assume(((bufsize - start) >= 64));
		exit(0);
	}
	if (uni_klee_pc == 386) {
		klee_assume(((bufsize - start) >= 128));
		exit(0);
	}
	if (uni_klee_pc == 387) {
		klee_assume(((bufsize - start) >= 256));
		exit(0);
	}
	if (uni_klee_pc == 388) {
		klee_assume(((bufsize - start) >= 512));
		exit(0);
	}
	if (uni_klee_pc == 389) {
		klee_assume(((bufsize - start) >= 1024));
		exit(0);
	}
	if (uni_klee_pc == 390) {
		klee_assume(((file_size - end) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 391) {
		klee_assume(((file_size - end) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 392) {
		klee_assume(((file_size - end) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 393) {
		klee_assume(((file_size - end) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 394) {
		klee_assume(((file_size - end) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 395) {
		klee_assume(((file_size - end) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 396) {
		klee_assume(((file_size - end) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 397) {
		klee_assume(((file_size - start) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 398) {
		klee_assume(((file_size - start) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 399) {
		klee_assume(((file_size - start) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 400) {
		klee_assume(((file_size - start) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 401) {
		klee_assume(((file_size - start) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 402) {
		klee_assume(((file_size - start) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 403) {
		klee_assume(((file_size - start) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 404) {
		klee_assume(((k - end) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 405) {
		klee_assume(((k - start) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 406) {
		klee_assume(((n - end) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 407) {
		klee_assume(((n - end) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 408) {
		klee_assume(((n - end) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 409) {
		klee_assume(((n - end) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 410) {
		klee_assume(((n - end) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 411) {
		klee_assume(((n - end) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 412) {
		klee_assume(((n - end) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 413) {
		klee_assume(((n - start) >= 1));
		exit(0);
	}
	if (uni_klee_pc == 414) {
		klee_assume(((n - start) >= 2));
		exit(0);
	}
	if (uni_klee_pc == 415) {
		klee_assume(((n - start) >= 3));
		exit(0);
	}
	if (uni_klee_pc == 416) {
		klee_assume(((n - start) >= 4));
		exit(0);
	}
	if (uni_klee_pc == 417) {
		klee_assume(((n - start) >= 5));
		exit(0);
	}
	if (uni_klee_pc == 418) {
		klee_assume(((n - start) >= 6));
		exit(0);
	}
	if (uni_klee_pc == 419) {
		klee_assume(((n - start) >= 7));
		exit(0);
	}
	if (uni_klee_pc == 420) {
		klee_assume(((end * 2) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 421) {
		klee_assume(((end * 3) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 422) {
		klee_assume(((end * 4) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 423) {
		klee_assume(((end * 5) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 424) {
		klee_assume(((end * 6) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 425) {
		klee_assume(((end * 7) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 426) {
		klee_assume(((end * 8) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 427) {
		klee_assume(((end * 9) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 428) {
		klee_assume(((end * 2) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 429) {
		klee_assume(((end * 3) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 430) {
		klee_assume(((end * 4) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 431) {
		klee_assume(((end * 5) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 432) {
		klee_assume(((end * 6) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 433) {
		klee_assume(((end * 7) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 434) {
		klee_assume(((end * 8) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 435) {
		klee_assume(((end * 9) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 436) {
		klee_assume(((end * 2) <= k));
		exit(0);
	}
	if (uni_klee_pc == 437) {
		klee_assume(((end * 3) <= k));
		exit(0);
	}
	if (uni_klee_pc == 438) {
		klee_assume(((end * 4) <= k));
		exit(0);
	}
	if (uni_klee_pc == 439) {
		klee_assume(((end * 5) <= k));
		exit(0);
	}
	if (uni_klee_pc == 440) {
		klee_assume(((end * 6) <= k));
		exit(0);
	}
	if (uni_klee_pc == 441) {
		klee_assume(((end * 7) <= k));
		exit(0);
	}
	if (uni_klee_pc == 442) {
		klee_assume(((end * 8) <= k));
		exit(0);
	}
	if (uni_klee_pc == 443) {
		klee_assume(((end * 9) <= k));
		exit(0);
	}
	if (uni_klee_pc == 444) {
		klee_assume(((end * 2) <= n));
		exit(0);
	}
	if (uni_klee_pc == 445) {
		klee_assume(((end * 3) <= n));
		exit(0);
	}
	if (uni_klee_pc == 446) {
		klee_assume(((end * 4) <= n));
		exit(0);
	}
	if (uni_klee_pc == 447) {
		klee_assume(((end * 5) <= n));
		exit(0);
	}
	if (uni_klee_pc == 448) {
		klee_assume(((end * 6) <= n));
		exit(0);
	}
	if (uni_klee_pc == 449) {
		klee_assume(((end * 7) <= n));
		exit(0);
	}
	if (uni_klee_pc == 450) {
		klee_assume(((end * 8) <= n));
		exit(0);
	}
	if (uni_klee_pc == 451) {
		klee_assume(((end * 9) <= n));
		exit(0);
	}
	if (uni_klee_pc == 452) {
		klee_assume(((end * 2) <= start));
		exit(0);
	}
	if (uni_klee_pc == 453) {
		klee_assume(((end * 3) <= start));
		exit(0);
	}
	if (uni_klee_pc == 454) {
		klee_assume(((end * 4) <= start));
		exit(0);
	}
	if (uni_klee_pc == 455) {
		klee_assume(((end * 5) <= start));
		exit(0);
	}
	if (uni_klee_pc == 456) {
		klee_assume(((end * 6) <= start));
		exit(0);
	}
	if (uni_klee_pc == 457) {
		klee_assume(((end * 7) <= start));
		exit(0);
	}
	if (uni_klee_pc == 458) {
		klee_assume(((end * 8) <= start));
		exit(0);
	}
	if (uni_klee_pc == 459) {
		klee_assume(((end * 9) <= start));
		exit(0);
	}
	if (uni_klee_pc == 460) {
		klee_assume(((k * 2) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 461) {
		klee_assume(((k * 3) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 462) {
		klee_assume(((k * 4) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 463) {
		klee_assume(((k * 5) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 464) {
		klee_assume(((k * 6) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 465) {
		klee_assume(((k * 7) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 466) {
		klee_assume(((k * 8) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 467) {
		klee_assume(((k * 9) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 468) {
		klee_assume(((start * 2) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 469) {
		klee_assume(((start * 3) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 470) {
		klee_assume(((start * 4) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 471) {
		klee_assume(((start * 5) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 472) {
		klee_assume(((start * 6) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 473) {
		klee_assume(((start * 7) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 474) {
		klee_assume(((start * 8) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 475) {
		klee_assume(((start * 9) <= bufsize));
		exit(0);
	}
	if (uni_klee_pc == 476) {
		klee_assume(((start * 2) <= end));
		exit(0);
	}
	if (uni_klee_pc == 477) {
		klee_assume(((start * 3) <= end));
		exit(0);
	}
	if (uni_klee_pc == 478) {
		klee_assume(((start * 4) <= end));
		exit(0);
	}
	if (uni_klee_pc == 479) {
		klee_assume(((start * 5) <= end));
		exit(0);
	}
	if (uni_klee_pc == 480) {
		klee_assume(((start * 6) <= end));
		exit(0);
	}
	if (uni_klee_pc == 481) {
		klee_assume(((start * 7) <= end));
		exit(0);
	}
	if (uni_klee_pc == 482) {
		klee_assume(((start * 8) <= end));
		exit(0);
	}
	if (uni_klee_pc == 483) {
		klee_assume(((start * 9) <= end));
		exit(0);
	}
	if (uni_klee_pc == 484) {
		klee_assume(((start * 2) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 485) {
		klee_assume(((start * 3) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 486) {
		klee_assume(((start * 4) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 487) {
		klee_assume(((start * 5) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 488) {
		klee_assume(((start * 6) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 489) {
		klee_assume(((start * 7) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 490) {
		klee_assume(((start * 8) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 491) {
		klee_assume(((start * 9) <= file_size));
		exit(0);
	}
	if (uni_klee_pc == 492) {
		klee_assume(((start * 2) <= k));
		exit(0);
	}
	if (uni_klee_pc == 493) {
		klee_assume(((start * 3) <= k));
		exit(0);
	}
	if (uni_klee_pc == 494) {
		klee_assume(((start * 4) <= k));
		exit(0);
	}
	if (uni_klee_pc == 495) {
		klee_assume(((start * 5) <= k));
		exit(0);
	}
	if (uni_klee_pc == 496) {
		klee_assume(((start * 6) <= k));
		exit(0);
	}
	if (uni_klee_pc == 497) {
		klee_assume(((start * 7) <= k));
		exit(0);
	}
	if (uni_klee_pc == 498) {
		klee_assume(((start * 8) <= k));
		exit(0);
	}
	if (uni_klee_pc == 499) {
		klee_assume(((start * 9) <= k));
		exit(0);
	}
	if (uni_klee_pc == 500) {
		klee_assume(((start * 2) <= n));
		exit(0);
	}
	if (uni_klee_pc == 501) {
		klee_assume(((start * 3) <= n));
		exit(0);
	}
	if (uni_klee_pc == 502) {
		klee_assume(((start * 4) <= n));
		exit(0);
	}
	if (uni_klee_pc == 503) {
		klee_assume(((start * 5) <= n));
		exit(0);
	}
	if (uni_klee_pc == 504) {
		klee_assume(((start * 6) <= n));
		exit(0);
	}
	if (uni_klee_pc == 505) {
		klee_assume(((start * 7) <= n));
		exit(0);
	}
	if (uni_klee_pc == 506) {
		klee_assume(((start * 8) <= n));
		exit(0);
	}
	if (uni_klee_pc == 507) {
		klee_assume(((start * 9) <= n));
		exit(0);
	}
if(__cpr_choice("L290", "bool", (long long[]){start, initial_read, bufsize}, (char*[]){"start","initial_read", "bufsize"}, 3, (int*[]){}, (char*[]){}, 0))
    {
CPR_OUTPUT("obs", "i32", initial_read - start);

klee_assert(initial_read > start);
      memmove (buf, buf + start, initial_read - start);
      initial_read -= start;
    }
  else
    {
      if (lseek (STDIN_FILENO, start, SEEK_CUR) < 0)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      initial_read = SIZE_MAX;
    }

  while (start < end)
    {
      size_t n_read;
      if (initial_read != SIZE_MAX)
        {
          n_read = initial_read;
          initial_read = SIZE_MAX;
        }
      else
        {
          n_read = safe_read (STDIN_FILENO, buf, bufsize);
          if (n_read == SAFE_READ_ERROR)
            die (EXIT_FAILURE, errno, "%s", quotef (infile));
        }
      if (n_read == 0)
        break; /* eof.  */
      n_read = MIN (n_read, end - start);
      if (full_write (STDOUT_FILENO, buf, n_read) != n_read
          && ! ignorable (errno))
        die (EXIT_FAILURE, errno, "%s", quotef ("-"));
      start += n_read;
    }
}

typedef struct of_info
{
  char *of_name;
  int ofd;
  FILE *ofile;
  int opid;
} of_t;

enum
{
  OFD_NEW = -1,
  OFD_APPEND = -2
};

/* Rotate file descriptors when we're writing to more output files than we
   have available file descriptors.
   Return whether we came under file resource pressure.
   If so, it's probably best to close each file when finished with it.  */

static bool
ofile_open (of_t *files, size_t i_check, size_t nfiles)
{
  bool file_limit = false;

  if (files[i_check].ofd <= OFD_NEW)
    {
      int fd;
      size_t i_reopen = i_check ? i_check - 1 : nfiles - 1;

      /* Another process could have opened a file in between the calls to
         close and open, so we should keep trying until open succeeds or
         we've closed all of our files.  */
      while (true)
        {
          if (files[i_check].ofd == OFD_NEW)
            fd = create (files[i_check].of_name);
          else /* OFD_APPEND  */
            {
              /* Attempt to append to previously opened file.
                 We use O_NONBLOCK to support writing to fifos,
                 where the other end has closed because of our
                 previous close.  In that case we'll immediately
                 get an error, rather than waiting indefinitely.
                 In specialised cases the consumer can keep reading
                 from the fifo, terminating on conditions in the data
                 itself, or perhaps never in the case of 'tail -f'.
                 I.e., for fifos it is valid to attempt this reopen.

                 We don't handle the filter_command case here, as create()
                 will exit if there are not enough files in that case.
                 I.e., we don't support restarting filters, as that would
                 put too much burden on users specifying --filter commands.  */
              fd = open (files[i_check].of_name,
                         O_WRONLY | O_BINARY | O_APPEND | O_NONBLOCK);
            }

          if (-1 < fd)
            break;

          if (!(errno == EMFILE || errno == ENFILE))
            die (EXIT_FAILURE, errno, "%s", quotef (files[i_check].of_name));

          file_limit = true;

          /* Search backwards for an open file to close.  */
          while (files[i_reopen].ofd < 0)
            {
              i_reopen = i_reopen ? i_reopen - 1 : nfiles - 1;
              /* No more open files to close, exit with E[NM]FILE.  */
              if (i_reopen == i_check)
                die (EXIT_FAILURE, errno, "%s",
                     quotef (files[i_check].of_name));
            }

          if (fclose (files[i_reopen].ofile) != 0)
            die (EXIT_FAILURE, errno, "%s", quotef (files[i_reopen].of_name));
          files[i_reopen].ofile = NULL;
          files[i_reopen].ofd = OFD_APPEND;
        }

      files[i_check].ofd = fd;
      if (!(files[i_check].ofile = fdopen (fd, "a")))
        die (EXIT_FAILURE, errno, "%s", quotef (files[i_check].of_name));
      files[i_check].opid = filter_pid;
      filter_pid = 0;
    }

  return file_limit;
}

/* -n r/[K/]N: Divide file into N chunks in round robin fashion.
   When K == 0, we try to keep the files open in parallel.
   If we run out of file resources, then we revert
   to opening and closing each file for each line.  */

static void
lines_rr (uintmax_t k, uintmax_t n, char *buf, size_t bufsize)
{
  bool wrapped = false;
  bool wrote = false;
  bool file_limit;
  size_t i_file;
  of_t *files IF_LINT (= NULL);
  uintmax_t line_no;

  if (k)
    line_no = 1;
  else
    {
      if (SIZE_MAX < n)
        xalloc_die ();
      files = xnmalloc (n, sizeof *files);

      /* Generate output file names. */
      for (i_file = 0; i_file < n; i_file++)
        {
          next_file_name ();
          files[i_file].of_name = xstrdup (outfile);
          files[i_file].ofd = OFD_NEW;
          files[i_file].ofile = NULL;
          files[i_file].opid = 0;
        }
      i_file = 0;
      file_limit = false;
    }

  while (true)
    {
      char *bp = buf, *eob;
      size_t n_read = safe_read (STDIN_FILENO, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        die (EXIT_FAILURE, errno, "%s", quotef (infile));
      else if (n_read == 0)
        break; /* eof.  */
      eob = buf + n_read;

      while (bp != eob)
        {
          size_t to_write;
          bool next = false;

          /* Find end of line. */
          char *bp_out = memchr (bp, eolchar, eob - bp);
          if (bp_out)
            {
              bp_out++;
              next = true;
            }
          else
            bp_out = eob;
          to_write = bp_out - bp;

          if (k)
            {
              if (line_no == k && unbuffered)
                {
                  if (full_write (STDOUT_FILENO, bp, to_write) != to_write)
                    die (EXIT_FAILURE, errno, "%s", _("write error"));
                }
              else if (line_no == k && fwrite (bp, to_write, 1, stdout) != 1)
                {
                  clearerr (stdout); /* To silence close_stdout().  */
                  die (EXIT_FAILURE, errno, "%s", _("write error"));
                }
              if (next)
                line_no = (line_no == n) ? 1 : line_no + 1;
            }
          else
            {
              /* Secure file descriptor. */
              file_limit |= ofile_open (files, i_file, n);
              if (unbuffered)
                {
                  /* Note writing to fd, rather than flushing the FILE gives
                     an 8% performance benefit, due to reduced data copying.  */
                  if (full_write (files[i_file].ofd, bp, to_write) != to_write
                      && ! ignorable (errno))
                    {
                      die (EXIT_FAILURE, errno, "%s",
                           quotef (files[i_file].of_name));
                    }
                }
              else if (fwrite (bp, to_write, 1, files[i_file].ofile) != 1
                       && ! ignorable (errno))
                {
                  die (EXIT_FAILURE, errno, "%s",
                       quotef (files[i_file].of_name));
                }

              if (! ignorable (errno))
                wrote = true;

              if (file_limit)
                {
                  if (fclose (files[i_file].ofile) != 0)
                    {
                      die (EXIT_FAILURE, errno, "%s",
                           quotef (files[i_file].of_name));
                    }
                  files[i_file].ofile = NULL;
                  files[i_file].ofd = OFD_APPEND;
                }
              if (next && ++i_file == n)
                {
                  wrapped = true;
                  /* If no filters are accepting input, stop reading.  */
                  if (! wrote)
                    goto no_filters;
                  wrote = false;
                  i_file = 0;
                }
            }

          bp = bp_out;
        }
    }

no_filters:
  /* Ensure all files created, so that any existing files are truncated,
     and to signal any waiting fifo consumers.
     Also, close any open file descriptors.
     FIXME: Should we do this before EXIT_FAILURE?  */
  if (!k)
    {
      int ceiling = (wrapped ? n : i_file);
      for (i_file = 0; i_file < n; i_file++)
        {
          if (i_file >= ceiling && !elide_empty_files)
            file_limit |= ofile_open (files, i_file, n);
          if (files[i_file].ofd >= 0)
            closeout (files[i_file].ofile, files[i_file].ofd,
                      files[i_file].opid, files[i_file].of_name);
          files[i_file].ofd = OFD_APPEND;
        }
    }
  IF_LINT (free (files));
}

#define FAIL_ONLY_ONE_WAY()					\
  do								\
    {								\
      error (0, 0, _("cannot split in more than one way"));	\
      usage (EXIT_FAILURE);					\
    }								\
  while (0)


/* Parse K/N syntax of chunk options.  */

static void
parse_chunk (uintmax_t *k_units, uintmax_t *n_units, char *slash)
{
  *n_units = xdectoumax (slash + 1, 1, UINTMAX_MAX, "",
                         _("invalid number of chunks"), 0);
  if (slash != optarg)           /* a leading number is specified.  */
    {
      *slash = '\0';
      *k_units = xdectoumax (optarg, 1, *n_units, "",
                             _("invalid chunk number"), 0);
    }
}


int
main (int argc, char **argv)
{
  enum Split_type split_type = type_undef;
  size_t in_blk_size = 0;	/* optimal block size of input file device */
  size_t page_size = getpagesize ();
  uintmax_t k_units = 0;
  uintmax_t n_units = 0;

  static char const multipliers[] = "bEGKkMmPTYZ0";
  int c;
  int digits_optind = 0;
  off_t file_size = OFF_T_MAX;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Parse command line options.  */

  infile = bad_cast ("-");
  outbase = bad_cast ("x");

  while (true)
    {
      /* This is the argv-index of the option we will read next.  */
      int this_optind = optind ? optind : 1;
      char *slash;

      c = getopt_long (argc, argv, "0123456789C:a:b:del:n:t:u",
                       longopts, NULL);
      if (c == -1)
        break;

      switch (c)
        {
        case 'a':
          suffix_length = xdectoumax (optarg, 0, SIZE_MAX / sizeof (size_t),
                                      "", _("invalid suffix length"), 0);
          break;

        case ADDITIONAL_SUFFIX_OPTION:
          if (last_component (optarg) != optarg)
            {
              error (0, 0,
                     _("invalid suffix %s, contains directory separator"),
                     quote (optarg));
              usage (EXIT_FAILURE);
            }
          additional_suffix = optarg;
          break;

        case 'b':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_bytes;
          /* Limit to OFF_T_MAX, because if input is a pipe, we could get more
             data than is possible to write to a single file, so indicate that
             immediately rather than having possibly future invocations fail. */
          n_units = xdectoumax (optarg, 1, OFF_T_MAX, multipliers,
                                _("invalid number of bytes"), 0);
          break;

        case 'l':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_lines;
          n_units = xdectoumax (optarg, 1, UINTMAX_MAX, "",
                                _("invalid number of lines"), 0);
          break;

        case 'C':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_byteslines;
          n_units = xdectoumax (optarg, 1, MIN (SIZE_MAX, OFF_T_MAX),
                                multipliers, _("invalid number of bytes"), 0);
          break;

        case 'n':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          /* skip any whitespace */
          while (isspace (to_uchar (*optarg)))
            optarg++;
          if (STRNCMP_LIT (optarg, "r/") == 0)
            {
              split_type = type_rr;
              optarg += 2;
            }
          else if (STRNCMP_LIT (optarg, "l/") == 0)
            {
              split_type = type_chunk_lines;
              optarg += 2;
            }
          else
            split_type = type_chunk_bytes;
          if ((slash = strchr (optarg, '/')))
            parse_chunk (&k_units, &n_units, slash);
          else
            n_units = xdectoumax (optarg, 1, UINTMAX_MAX, "",
                                  _("invalid number of chunks"), 0);
          break;

        case 'u':
          unbuffered = true;
          break;

        case 't':
          {
            char neweol = optarg[0];
            if (! neweol)
              die (EXIT_FAILURE, 0, _("empty record separator"));
            if (optarg[1])
              {
                if (STREQ (optarg, "\\0"))
                  neweol = '\0';
                else
                  {
                    /* Provoke with 'split -txx'.  Complain about
                       "multi-character tab" instead of "multibyte tab", so
                       that the diagnostic's wording does not need to be
                       changed once multibyte characters are supported.  */
                    die (EXIT_FAILURE, 0, _("multi-character separator %s"),
                         quote (optarg));
                  }
              }
            /* Make it explicit we don't support multiple separators.  */
            if (0 <= eolchar && neweol != eolchar)
              {
                die (EXIT_FAILURE, 0,
                     _("multiple separator characters specified"));
              }

            eolchar = neweol;
          }
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (split_type == type_undef)
            {
              split_type = type_digits;
              n_units = 0;
            }
          if (split_type != type_undef && split_type != type_digits)
            FAIL_ONLY_ONE_WAY ();
          if (digits_optind != 0 && digits_optind != this_optind)
            n_units = 0;	/* More than one number given; ignore other. */
          digits_optind = this_optind;
          if (!DECIMAL_DIGIT_ACCUMULATE (n_units, c - '0', uintmax_t))
            {
              char buffer[INT_BUFSIZE_BOUND (uintmax_t)];
              die (EXIT_FAILURE, 0,
                   _("line count option -%s%c... is too large"),
                   umaxtostr (n_units, buffer), c);
            }
          break;

        case 'd':
          suffix_alphabet = "0123456789";
          if (optarg)
            {
              if (strlen (optarg) != strspn (optarg, suffix_alphabet))
                {
                  error (0, 0,
                         _("%s: invalid start value for numerical suffix"),
                         quote (optarg));
                  usage (EXIT_FAILURE);
                }
              else
                {
                  /* Skip any leading zero.  */
                  while (*optarg == '0' && *(optarg + 1) != '\0')
                    optarg++;
                  numeric_suffix_start = optarg;
                }
            }
          break;

        case 'e':
          elide_empty_files = true;
          break;

        case FILTER_OPTION:
          filter_command = optarg;
          break;

        case IO_BLKSIZE_OPTION:
          in_blk_size = xdectoumax (optarg, 1, SIZE_MAX - page_size,
                                    multipliers, _("invalid IO block size"), 0);
          break;

        case VERBOSE_OPTION:
          verbose = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (k_units != 0 && filter_command)
    {
      error (0, 0, _("--filter does not process a chunk extracted to stdout"));
      usage (EXIT_FAILURE);
    }

  /* Handle default case.  */
  if (split_type == type_undef)
    {
      split_type = type_lines;
      n_units = 1000;
    }

  if (n_units == 0)
    {
      error (0, 0, "%s: %s", _("invalid number of lines"), quote ("0"));
      usage (EXIT_FAILURE);
    }

  if (eolchar < 0)
    eolchar = '\n';

  set_suffix_length (n_units, split_type);

  /* Get out the filename arguments.  */

  if (optind < argc)
    infile = argv[optind++];

  if (optind < argc)
    outbase = argv[optind++];

  if (optind < argc)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind]));
      usage (EXIT_FAILURE);
    }

  /* Check that the suffix length is large enough for the numerical
     suffix start value.  */
  if (numeric_suffix_start && strlen (numeric_suffix_start) > suffix_length)
    {
      error (0, 0, _("numerical suffix start value is too large "
                     "for the suffix length"));
      usage (EXIT_FAILURE);
    }

  /* Open the input file.  */
  if (! STREQ (infile, "-")
      && fd_reopen (STDIN_FILENO, infile, O_RDONLY, 0) < 0)
    die (EXIT_FAILURE, errno, _("cannot open %s for reading"),
         quoteaf (infile));

  /* Binary I/O is safer when byte counts are used.  */
  if (O_BINARY && ! isatty (STDIN_FILENO))
    xfreopen (NULL, "rb", stdin);

  /* Get the optimal block size of input device and make a buffer.  */

  if (fstat (STDIN_FILENO, &in_stat_buf) != 0)
    die (EXIT_FAILURE, errno, "%s", quotef (infile));

  bool specified_buf_size = !! in_blk_size;
  if (! specified_buf_size)
    in_blk_size = io_blksize (in_stat_buf);

  void *b = xmalloc (in_blk_size + 1 + page_size - 1);
  char *buf = ptr_align (b, page_size);
  size_t initial_read = SIZE_MAX;

  if (split_type == type_chunk_bytes || split_type == type_chunk_lines)
    {
      file_size = input_file_size (STDIN_FILENO, &in_stat_buf,
                                   buf, in_blk_size);
      if (file_size < 0)
        die (EXIT_FAILURE, errno, _("%s: cannot determine file size"),
             quotef (infile));
      initial_read = MIN (file_size, in_blk_size);
      /* Overflow, and sanity checking.  */
      if (OFF_T_MAX < n_units)
        {
          char buffer[INT_BUFSIZE_BOUND (uintmax_t)];
          die (EXIT_FAILURE, EOVERFLOW, "%s: %s",
               _("invalid number of chunks"),
               quote (umaxtostr (n_units, buffer)));
        }
      /* increase file_size to n_units here, so that we still process
         any input data, and create empty files for the rest.  */
      file_size = MAX (file_size, n_units);
    }

  /* When filtering, closure of one pipe must not terminate the process,
     as there may still be other streams expecting input from us.  */
  if (filter_command)
    {
      struct sigaction act;
      sigemptyset (&newblocked);
      sigaction (SIGPIPE, NULL, &act);
      if (act.sa_handler != SIG_IGN)
        sigaddset (&newblocked, SIGPIPE);
      sigprocmask (SIG_BLOCK, &newblocked, &oldblocked);
    }

  switch (split_type)
    {
    case type_digits:
    case type_lines:
      lines_split (n_units, buf, in_blk_size);
      break;

    case type_bytes:
      bytes_split (n_units, buf, in_blk_size, SIZE_MAX, 0);
      break;

    case type_byteslines:
      line_bytes_split (n_units, buf, in_blk_size);
      break;

    case type_chunk_bytes:
      if (k_units == 0)
        bytes_split (file_size / n_units, buf, in_blk_size, initial_read,
                     n_units);
      else
        bytes_chunk_extract (k_units, n_units, buf, in_blk_size, initial_read,
                             file_size);
      break;

    case type_chunk_lines:
      lines_chunk_split (k_units, n_units, buf, in_blk_size, initial_read,
                         file_size);
      break;

    case type_rr:
      /* Note, this is like 'sed -n ${k}~${n}p' when k > 0,
         but the functionality is provided for symmetry.  */
      lines_rr (k_units, n_units, buf, in_blk_size);
      break;

    default:
      abort ();
    }

  IF_LINT (free (b));

  if (close (STDIN_FILENO) != 0)
    die (EXIT_FAILURE, errno, "%s", quotef (infile));
  closeout (NULL, output_desc, filter_pid, outfile);

  return EXIT_SUCCESS;
}
