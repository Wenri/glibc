/* Create a directory.  Linux version.
   Copyright (C) 2011-2025 Free Software Foundation, Inc.
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sysdep.h>

/* android: the wrapper in fc-mkdir.c takes over BOTH the public
   `mkdir' and the internal `__mkdir' so that glibc's own callers are
   path-translated too.  This implementation is renamed out of the way and
   becomes the raw "next" function the wrapper calls through to.  */
#if !IS_IN (rtld)
# define __mkdir __android_next_mkdir
#endif

/* Create a directory named PATH with protections MODE.  */
int
__mkdir (const char *path, mode_t mode)
{
#ifdef __NR_mkdir
  return INLINE_SYSCALL_CALL (mkdir,  path, mode);
#else
  return INLINE_SYSCALL_CALL (mkdirat, AT_FDCWD, path, mode);
#endif
}

#if IS_IN (rtld)
libc_hidden_def (__mkdir)
weak_alias (__mkdir, mkdir)
#endif
