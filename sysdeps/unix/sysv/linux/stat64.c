/* Get file status.  Linux version.
   Copyright (C) 2020-2025 Free Software Foundation, Inc.
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

#define __stat __redirect___stat
#define stat   __redirect_stat
#include <sys/stat.h>
#include <fcntl.h>
#include <kernel_stat.h>
#include <stat_t64_cp.h>

/* Renamed AFTER the includes: on __TIMESIZE == 64 the defining token is
   __stat64_time64, which include/sys/stat.h collapses to __stat64, so the rename
   only reaches it by macro rescanning -- and placing it before the headers
   would rewrite their declarations too.  */
#if !IS_IN (rtld)
# define __stat64 __android_next_stat64
#endif

int
__stat64_time64 (const char *file, struct __stat64_t64 *buf)
{
  return __fstatat64_time64 (AT_FDCWD, file, buf, 0);
}
#if __TIMESIZE != 64
hidden_def (__stat64_time64)

int
__stat64 (const char *file, struct stat64 *buf)
{
  struct __stat64_t64 st_t64;
  return __stat64_time64 (file, &st_t64)
	 ?: __cp_stat64_t64_stat64 (&st_t64, buf);
}
#endif

#undef __stat
#undef stat

#if IS_IN (rtld)
hidden_def (__stat64)
#endif
#if IS_IN (rtld)
weak_alias (__stat64, stat64)
#endif

#if XSTAT_IS_XSTAT64
#if IS_IN (rtld)
strong_alias (__stat64, __stat)
#endif
#if IS_IN (rtld)
weak_alias (__stat64, stat)
#endif
#endif
