/* Copyright (C) 2009-2025 Free Software Foundation, Inc.
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __GT_FILE
# define __GT_FILE 0
#endif

/* Generate a unique temporary file name from TEMPLATE.  The last six
   characters before a suffix of length SUFFIXLEN of TEMPLATE must be
   "XXXXXX"; they are replaced with a string that makes the filename
   unique.  Then open the file and return a fd. */
/* The LIVE file of this pair is the NON-64 one: mkostemps64.c is #if'd to
   nothing because O_LARGEFILE is 0 here, and this file defines the bare
   public name with mkostemps64 as a weak alias -- the reverse of stat/open
   (precondition 7).  */
#if !IS_IN (rtld)
# define mkostemps __android_next_mkostemps
#endif

int
mkostemps (char *template, int suffixlen, int flags)
{
  if (suffixlen < 0)
    {
      __set_errno (EINVAL);
      return -1;
    }

  return __gen_tempname (template, suffixlen, flags, __GT_FILE);
}

#if !defined O_LARGEFILE || O_LARGEFILE == 0
#if IS_IN (rtld)
weak_alias (mkostemps, mkostemps64)
#endif
#endif
