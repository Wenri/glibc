/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013 Piotr Roszatycki <dexter@debian.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/


#include <config.h>

/* ADAPTED FOR GLIBC -- not upstream fakechroot.  `glob' appears in TWO blocks
   of posix/Versions (:38 under GLIBC_2.0, :146 under GLIBC_2.27), and glob64
   likewise.  An unversioned weak_alias cannot select a node, which is why glibc
   binds these with versioned_symbol -- so wrapper() must not emit the public
   name and the tail of this file emits it, at the right version.  The compat
   arm (glob@GLIBC_2.17) is a different implementation and lives in
   fc-glob_lstat_compat.c.  */
#define FC_NO_PUBLIC_ALIAS 1
#include <shlib-compat.h>

/* Mask glob64's declaration across <glob.h> only, exactly as glibc's own
   sysdeps/unix/sysv/linux/glob.c:28-29 does.  <glob.h> declares glob64 taking
   glob64_t *, but on this arch it is the SAME function as glob, so the alias
   below has glob_t * and the two declarations conflict.  glibc hides the
   declaration rather than fight it.  */
#define glob64   __no_glob64_decl
#define __glob64 __no___glob64_decl
#include <glob.h>
#undef glob64
#undef __glob64

#include "wrapper.h"
#include "glob-narrow.h"


wrapper(glob, int, (const char * pattern, int flags, int (* errfunc) (const char *, int), glob_t * pglob))
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    int rc;

    debug("glob(\"%s\", %d, &errfunc, &pglob)", pattern, flags);
    pattern = expand_chroot_rel_path(pattern, fakechroot_buf);
    /* Too long to translate: no file can bear that name.  Checked because
       glibc's glob dereferences the pattern immediately.  */
    if (pattern == NULL)
        return GLOB_NOMATCH;

    rc = nextcall(glob)(pattern, flags, errfunc, pglob);
    /* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream tests `rc < 0',
       which is never true: glob reports errors as POSITIVE codes (GLOB_NOSPACE
       1, GLOB_ABORTED 2, GLOB_NOMATCH 3).  So on GLOB_NOMATCH it fell through
       and walked a gl_pathv that was never filled.  */
    if (rc != 0)
        return rc;

    /* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream copied every
       match through a stack temp with strcpy, indexed from 0 under
       GLOB_DOOFFS, and open-coded the prefix strip.  All three defects, and
       the fix, are now in glob-narrow.h -- shared with the compat arm, which
       is where they survived after this file was fixed.  */
    fc_glob_narrow_results(pglob, flags);
    return rc;
}

/* One function serves both names, exactly as glibc's own arm does (its
   linux/glob.c aliases __glob64 to __glob before versioning it), so the
   reference has glob@@GLIBC_2.27 and glob64@@GLIBC_2.27 at one address.  */
strong_alias (__fc_glob, __fc_glob64)

versioned_symbol (libc, __fc_glob, glob, GLIBC_2_27);
versioned_symbol (libc, __fc_glob64, glob64, GLIBC_2_27);
