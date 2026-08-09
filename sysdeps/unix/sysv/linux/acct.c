/* acct -- Linux implementation, written so the android port can rename it.

   glibc normally GENERATES this from sysdeps/unix/syscalls.list; a .c in
   $sysdirs suppresses the entry (sysdeps/unix/make-syscalls.sh:61-69,:223),
   which is how glibc itself overrides statfs, truncate and friends.

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

#include <unistd.h>
#include <sysdep.h>

#if !IS_IN (rtld)
# define acct __android_next_acct
/* Precondition 9: INLINE_SYSCALL_CALL pastes the FUNCTION name into __NR_##name,
   and for a bare-name function that is the token just renamed.  Point it back.  */
# define __NR___android_next_acct __NR_acct
#endif

int
acct (const char *name)
{
  return INLINE_SYSCALL_CALL (acct, name);
}
