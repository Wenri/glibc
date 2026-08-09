/* chdir -- Linux implementation, written so the android port can rename it.

   glibc normally GENERATES this from sysdeps/unix/syscalls.list, which leaves no
   .c to rename and makes CFLAGS-chdir.c inert.  A .c anywhere in $sysdirs
   suppresses the list entry (sysdeps/unix/make-syscalls.sh:61-69 sets srcfile,
   and the stub is emitted only in case x-* at :223), which is how glibc itself
   overrides statfs, truncate, utimes, connect and rename.  See nix-on-droid/docs/ANDROID-GLIBC.md.

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
# define __chdir __android_next_chdir
#endif

int
__chdir (const char *path)
{
  return INLINE_SYSCALL_CALL (chdir, path);
}
#if IS_IN (rtld)
libc_hidden_def (__chdir)
weak_alias (__chdir, chdir)
#endif
