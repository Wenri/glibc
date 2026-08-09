/* Set the kernel's process name (comm).  Linux version.
   Copyright (C) 2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _DL_PROGNAME_H
#define _DL_PROGNAME_H 1

#include <sysdep.h>
#include <sys/prctl.h>

/* When ld.so is run as a command -- `ld.so PROGRAM ARGS...' -- the kernel took
   comm from the loader's own name, so ps, top and /proc/PID/comm all report
   ld-linux-aarch64.so.1 for the process.  Point it at PATH's basename instead.

   This replaces android/syscall.c's fakechroot_set_process_name
   CONSTRUCTOR, which reached the same conclusion the hard way: readlink
   /proc/self/exe, strstr for "ld-linux", then open and read /proc/self/cmdline.
   As an ELF constructor in libc.so that ran in EVERY process, paying a readlink
   to discover what rtld already knows for certain -- and _dl_argv[0] is by this
   point the program, --argv0 applied, so there is nothing left to parse.

   Hand-rolled basename scan rather than strrchr, to avoid giving ld.so a string
   routine it does not otherwise need.  INTERNAL_SYSCALL_CALL because under
   IS_IN (rtld) errno is the single non-TLS rtld_errno; the result is ignored
   because a cosmetic name is not worth failing a program start over.  */
static inline void
_dl_set_process_name (const char *path)
{
  if (path == NULL)
    return;

  const char *base = path;
  for (const char *p = path; *p != '\0'; ++p)
    if (*p == '/')
      base = p + 1;

  if (*base != '\0')
    INTERNAL_SYSCALL_CALL (prctl, PR_SET_NAME, base, 0, 0, 0);
}

#endif /* dl-progname.h */
