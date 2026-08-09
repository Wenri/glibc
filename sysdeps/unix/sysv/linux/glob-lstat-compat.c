/* Compat glob which does not use gl_lstat for GLOB_ALTDIRFUNC.
   Linux version which handles LFS when required.
   Copyright (C) 2017-2025 Free Software Foundation, Inc.
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

#include <sys/stat.h>
#include <kernel_stat.h>
#include <shlib-compat.h>

#define glob64 __no_glob64_decl
#include <glob.h>
#undef glob64

/* android: the compat arm keeps its OWN semantics -- GLOB_LSTAT is
   gl_stat below, not gl_lstat -- and gets its own wrapper, so that binaries
   bound to glob@GLIBC_2.17 are translated too rather than silently reaching
   untranslated code.  Rename the implementation; the shim supplies the
   wrapper and the compat_symbols.

   The #else arm is glibc's own name for this body, and is what an rtld build
   would take.  Nothing compiles this file into ld.so -- all-rtld-routines is
   dl-* only -- so it is unreachable today.  It is kept because it is the right
   answer if that ever changes, and because dropping it would leave a bare
   #define that reads as unconditional.  Note the condition used to mean "is the
   wrapper wired" and now means "am I rtld"; the two agree here only because
   this file is always compiled for libc.  */
#if !IS_IN (rtld)
# define __glob __android_next_glob_lstat_compat
#else
# define __glob __glob_lstat_compat
#endif

#define GLOB_ATTRIBUTE attribute_compat_text_section

/* Avoid calling gl_lstat with GLOB_ALTDIRFUNC.  */
#define struct_stat    struct stat
#define struct_stat64  struct stat64
#define GLOB_LSTAT     gl_stat
#define GLOB_STAT64    __stat64
#define GLOB_LSTAT64   __stat64

#include <posix/glob.c>

#ifndef GLOB_LSTAT_VERSION
# define GLOB_LSTAT_VERSION GLIBC_2_0
#endif

#if SHLIB_COMPAT(libc, GLOB_LSTAT_VERSION, GLIBC_2_27)
# if IS_IN (rtld)
compat_symbol (libc, __glob_lstat_compat, glob, GLOB_LSTAT_VERSION);
# endif
# if XSTAT_IS_XSTAT64
# if IS_IN (rtld)
strong_alias (__glob_lstat_compat, __glob64_lstat_compat)
# endif
# if IS_IN (rtld)
compat_symbol (libc, __glob64_lstat_compat, glob64, GLOB_LSTAT_VERSION);
# endif
# endif
#endif
