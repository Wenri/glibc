/* Copyright (C) 2015-2025 Free Software Foundation, Inc.
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

#include <sys/socket.h>
#include <socketcall.h>

/* android: the wrapper (fc-getsockname.c) takes over both `getsockname' and
   `__getsockname'; this implementation becomes the raw "next" it calls through to.  */
#if !IS_IN (rtld)
# define __getsockname __android_next_getsockname
#endif

int
__getsockname (int fd, __SOCKADDR_ARG addr, socklen_t *len)
{
#ifdef __ASSUME_GETSOCKNAME_SYSCALL
  return INLINE_SYSCALL_CALL (getsockname, fd, addr.__sockaddr__, len);
#else
  return SOCKETCALL (getsockname, fd, addr.__sockaddr__, len);
#endif
}
#if IS_IN (rtld)
weak_alias (__getsockname, getsockname)
#endif
