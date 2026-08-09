/* Linux implementation for rename function.
   Copyright (C) 2016-2025 Free Software Foundation, Inc.
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
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <fcntl.h>
#include <sysdep.h>
#include <errno.h>

/* Rename the file OLD to NEW.  */
#if !IS_IN (rtld)
# define rename __android_next_rename
/* Precondition 9.  The #ifdef __NR_rename arm below issues
   INLINE_SYSCALL_CALL (rename, ...), which pastes the renamed token into
   __NR_##name.  aarch64 has no __NR_rename so that arm is dead here and the
   build passes by luck; keep this so the rename stays correct on an arch
   that does have it.  */
# define __NR___android_next_rename __NR_rename
#endif

int
rename (const char *old, const char *new)
{
#if defined (__NR_rename)
  return INLINE_SYSCALL_CALL (rename, old, new);
#elif defined (__NR_renameat)
  return INLINE_SYSCALL_CALL (renameat, AT_FDCWD, old, AT_FDCWD, new);
#else
  return INLINE_SYSCALL_CALL (renameat2, AT_FDCWD, old, AT_FDCWD, new, 0);
#endif
}
