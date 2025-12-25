/* Get file status.  Linux version.
   Copyright (C) 2020-2024 Free Software Foundation, Inc.
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

#define __fstatat __redirect___fstatat
#define fstatat   __redirect_fstatat
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sysdep.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <internal-stat.h>

#if __TIMESIZE == 64 \
     && (__WORDSIZE == 32 \
     && (!defined __SYSCALL_WORDSIZE || __SYSCALL_WORDSIZE == 32))
/* Sanity check to avoid newer 32-bit ABI to support non-LFS calls.  */
_Static_assert (sizeof (__off_t) == sizeof (__off64_t),
                "__blkcnt_t and __blkcnt64_t must match");
_Static_assert (sizeof (__ino_t) == sizeof (__ino64_t),
                "__blkcnt_t and __blkcnt64_t must match");
_Static_assert (sizeof (__blkcnt_t) == sizeof (__blkcnt64_t),
                "__blkcnt_t and __blkcnt64_t must match");
#endif

/* Only statx supports 64-bit timestamps for 32-bit architectures with
   __ASSUME_STATX, so there is no point in building the fallback.  */
static inline int
fstatat64_time64_stat (int fd, const char *file, struct __stat64_t64 *buf,
		       int flag)
{
  int r;

#if XSTAT_IS_XSTAT64
# ifdef __NR_newfstatat
  /* 64-bit kABI, e.g. aarch64, powerpc64*, s390x, riscv64, and
     x86_64.  */
  r = INTERNAL_SYSCALL_CALL (newfstatat, fd, file, buf, flag);
# elif defined __NR_fstatat64
#  if STAT64_IS_KERNEL_STAT64
  /* 64-bit kABI outlier, e.g. alpha  */
  r = INTERNAL_SYSCALL_CALL (fstatat64, fd, file, buf, flag);
#  else
  /* 64-bit kABI outlier, e.g. sparc64.  */
  struct kernel_stat64 kst64;
  r = INTERNAL_SYSCALL_CALL (fstatat64, fd, file, &kst64, flag);
  if (r == 0)
    __cp_stat64_kstat64 (buf, &kst64);
#  endif
# endif
#else
# ifdef __NR_fstatat64
  /* All kABIs with non-LFS support and with old 32-bit time_t support
     e.g. arm, csky, i386, hppa, m68k, microblaze, nios2, sh, powerpc32,
     and sparc32.  */
  struct stat64 st64;
  r = INTERNAL_SYSCALL_CALL (fstatat64, fd, file, &st64, flag);
  if (r == 0)
    {
      /* Clear both pad and reserved fields.  */
      memset (buf, 0, sizeof (*buf));

      buf->st_dev = st64.st_dev,
      buf->st_ino = st64.st_ino;
      buf->st_mode = st64.st_mode;
      buf->st_nlink = st64.st_nlink;
      buf->st_uid = st64.st_uid;
      buf->st_gid = st64.st_gid;
      buf->st_rdev = st64.st_rdev;
      buf->st_size = st64.st_size;
      buf->st_blksize = st64.st_blksize;
      buf->st_blocks  = st64.st_blocks;
      buf->st_atim = valid_timespec_to_timespec64 (st64.st_atim);
      buf->st_mtim = valid_timespec_to_timespec64 (st64.st_mtim);
      buf->st_ctim = valid_timespec_to_timespec64 (st64.st_ctim);
    }
# else
  /* 64-bit kabi outlier, e.g. mips64 and mips64-n32.  */
  struct kernel_stat kst;
  r = INTERNAL_SYSCALL_CALL (newfstatat, fd, file, &kst, flag);
  if (r == 0)
    __cp_kstat_stat64_t64 (&kst, buf);
# endif
#endif

  return r;
}

int
__fstatat64_time64 (int fd, const char *file, struct __stat64_t64 *buf,
		    int flag)
{
  int r = fstatat64_time64_stat (fd, file, buf, flag);
  return INTERNAL_SYSCALL_ERROR_P (r)
	 ? INLINE_SYSCALL_ERROR_RETURN_VALUE (-r)
	 : 0;
}
#if __TIMESIZE != 64
hidden_def (__fstatat64_time64)

int
__fstatat64 (int fd, const char *file, struct stat64 *buf, int flags)
{
  struct __stat64_t64 st_t64;
  return __fstatat64_time64 (fd, file, &st_t64, flags)
	 ?: __cp_stat64_t64_stat64 (&st_t64, buf);
}
#endif

#undef __fstatat
#undef fstatat

hidden_def (__fstatat64)
weak_alias (__fstatat64, fstatat64)

#if XSTAT_IS_XSTAT64
strong_alias (__fstatat64, __fstatat)
weak_alias (__fstatat64, fstatat)
strong_alias (__fstatat64, __GI___fstatat);
#endif
