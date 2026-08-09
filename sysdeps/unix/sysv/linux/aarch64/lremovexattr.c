/* lremovexattr -- Linux implementation, written so the android port can rename it.

   glibc normally GENERATES this from sysdeps/unix/sysv/linux/syscalls.list,
   which leaves no .c to rename.  IT LIVES HERE, in the arch dir, and not beside
   that list, because make-syscalls.sh:44-48 truncates $sysdirs to directories
   STRICTLY ABOVE the one holding the list before looking for a .c -- so a file
   next to the list does not suppress its entry and the generated stub still
   defines the symbol, colliding with the wrapper.  A file here does outrank it.
   (chdir, acct and chroot need no such move: their entries are in the
   lower-priority sysdeps/unix/syscalls.list.)  See nix-on-droid/docs/ANDROID-GLIBC.md.

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

#include <sys/xattr.h>
#include <sysdep.h>

#if !IS_IN (rtld)
# define lremovexattr __android_next_lremovexattr
/* Precondition 9: INLINE_SYSCALL_CALL pastes the FUNCTION name into
   __NR_##name, and for a bare-name function that is the token just renamed.
   Point it back at the real syscall number.  */
# define __NR___android_next_lremovexattr __NR_lremovexattr
#endif

int
lremovexattr (const char *path, const char *name)
{
  return INLINE_SYSCALL_CALL (lremovexattr, path, name);
}
