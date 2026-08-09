/* Read value of a symbolic link.  Linux version.
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

#include <unistd.h>
#include <fcntl.h>
#include <sysdep.h>

/* android: the wrapper (fc-readlink.c) takes over both `readlink' and
   `__readlink'; this implementation becomes the raw "next" it calls through to.  */
#if !IS_IN (rtld)
# define __readlink __android_next_readlink
#endif

/* Read the contents of the symbolic link PATH into no more than
   LEN bytes of BUF.  The contents are not null-terminated.
   Returns the number of characters read, or -1 for errors.  */
ssize_t
__readlink (const char *path, char *buf, size_t len)
{
#ifdef __NR_readlink
  return INLINE_SYSCALL_CALL (readlink, path, buf, len);
#else
  return INLINE_SYSCALL_CALL (readlinkat, AT_FDCWD, path, buf, len);
#endif
}
#if IS_IN (rtld)
weak_alias (__readlink, readlink)
#endif
