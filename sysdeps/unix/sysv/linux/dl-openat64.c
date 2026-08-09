/* Copyright (C) 2011-2025 Free Software Foundation, Inc.
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sysdep.h>


int
openat64 (int dfd, const char *file, int oflag, ...)
{
  assert (!__OPEN_NEEDS_MODE (oflag));

  return INLINE_SYSCALL (openat, 3, dfd, file, oflag | O_LARGEFILE);
}

/* android: rtld references __openat64, not just openat64.  Upstream
   that reference is satisfied by pulling openat64.os out of libc_pic.a during
   the librtld.map discovery link -- but once the wrappers claim __openat64,
   that alias is suppressed there and the only remaining definition in the
   archive is fc-openat64.os.  The link then drags libfakechroot into ld.so and
   fails with duplicate __getcwd, malloc, _dl_start and __libc_fatal, none of
   which mention openat64 (nix-on-droid/docs/ANDROID-GLIBC.md precondition 4).

   Defining the alias here makes rtld self-sufficient.  libc_hidden_proto is a
   no-op under IS_IN (rtld), so this is a plain definition with no __GI_
   redirect.  */
strong_alias (openat64, __openat64)
