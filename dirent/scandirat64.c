/* Copyright (C) 2000-2025 Free Software Foundation, Inc.
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

#define scandirat __no_scandirat_decl
#include <dirent.h>
#undef scandirat

/* ANDROID: rename out of the way so fc-scandirat64.c can take both public names
   over.  This is the LIVE file of the pair -- dirent/scandirat.c is #if'd to
   nothing under _DIRENT_MATCHES_DIRENT64 (precondition 7) -- and it defines the
   public name bare, which is why the reference exports scandirat64 GLOBAL with
   scandirat a WEAK alias at the same address.  The shim re-emits both.

   Precondition 9 does not apply: the body issues no syscall named after itself,
   it calls __scandir64_tail (__opendirat (...)).  __opendirat is NOT wrapped,
   which is exactly why this needed wiring -- scandir/scandir64 reach __opendir
   and were already translated, but a direct scandirat was not.  */
#if !IS_IN (rtld)
# define scandirat64 __android_next_scandirat64
#endif

int
scandirat64 (int dfd, const char *dir, struct dirent64 ***namelist,
	     int (*select) (const struct dirent64 *),
	     int (*cmp) (const struct dirent64 **, const struct dirent64 **))
{
  return __scandir64_tail (__opendirat (dfd, dir), namelist, select, cmp);
}

#if _DIRENT_MATCHES_DIRENT64
/* Suppressed when the wrapper takes the name: aliasing here would point
   scandirat at the RENAMED implementation, leaving it untranslated.  */
# if IS_IN (rtld)
weak_alias (scandirat64, scandirat)
# endif
#endif
