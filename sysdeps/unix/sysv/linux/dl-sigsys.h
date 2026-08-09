/* SIGSYS handler for Android's seccomp filter.  Linux version.
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

#ifndef _DL_SIGSYS_H
#define _DL_SIGSYS_H 1

/* Install ld.so's SIGSYS handler.  Called once, from the end of dl_main.
   Internal to ld.so.  */
extern void _dl_sigsys_install (void) attribute_hidden;

/* Exchange the application's SIGSYS disposition with the one ld.so has
   recorded for chaining.  *HANDLER and *FLAGS are in/out: the recorded pair is
   read out into them and, if SET is nonzero, the incoming pair is recorded
   first.  ld.so's own handler is never uninstalled.

   Returns 1 if the request was absorbed, in which case the caller must NOT
   issue rt_sigaction; 0 if ld.so has no handler installed, in which case
   nothing was touched and the caller should proceed normally.

   Scalars rather than a struct sigaction deliberately: ld.so speaks
   struct kernel_sigaction, whose field order, size and membership all differ
   from the userspace struct, so no aggregate layout crosses the module
   boundary.  One pointer covers both sa_handler and sa_sigaction because they
   are a union; SA_SIGINFO in FLAGS selects between them.  */
extern int _dl_sigsys_exchange (int __set, void **__handler, int *__flags);

#endif /* dl-sigsys.h */
