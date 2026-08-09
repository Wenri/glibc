/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2013 Piotr Roszatycki <dexter@debian.org>

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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "wrapper.h"
#include "strlcpy.h"
#include "dedotdot.h"
#include "getcwd_real.h"


/* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream calls getcwd_real()
   unconditionally with CWD in rel2abs's own frame, but only the RELATIVE arm
   consumes it: the absolute arm is a plain strlcpy.

   Splitting it out rather than merely scoping it to that arm is deliberate, and
   measured.  GCC fixes the frame at function entry, so a block-scoped array
   still costs every caller: with the array simply moved inside the `else',
   rel2abs still measured 4128 bytes in the built libc.so.6.  Only moving it
   into a callee that the absolute path never enters actually removes the cost.
   noinline because the whole point is that this frame stay out of rel2abs.

   It matters because a wrapper may run on a sigaltstack -- SIGSTKSZ is 16384
   here and MINSIGSTKSZ 5120 -- and most wrapped functions are POSIX
   async-signal-safe, so calling them from a handler is ordinary.  Measured
   before this split: open() 8272 bytes, lstat() 16480, the latter already over
   SIGSTKSZ.

   Checking getcwd_real is the other half: on failure CWD is uninitialised, and
   upstream would snprintf stack garbage into RESOLVED and translate that.  */
static char *__attribute__ ((noinline))
rel2abs_from_cwd (const char * name, char * resolved)
{
    char cwd[FAKECHROOT_PATH_MAX - 1];

    if (getcwd_real(cwd, FAKECHROOT_PATH_MAX - 1) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd);
    snprintf(resolved, FAKECHROOT_PATH_MAX, "%s/%s", cwd, name);
    return resolved;
}

LOCAL char * rel2abs(const char * name, char * resolved)
{
    debug("rel2abs(\"%s\", &resolved)", name);

    if (name == NULL) {
        resolved = NULL;
        goto end;
    }

    if (*name == '\0') {
        *resolved = '\0';
        goto end;
    }

    if (*name == '/') {
        strlcpy(resolved, name, FAKECHROOT_PATH_MAX);
    }
    else if ((resolved = rel2abs_from_cwd(name, resolved)) == NULL) {
        goto end;
    }

    dedotdot(resolved);

end:
    debug("rel2abs(\"%s\", \"%s\")", name, resolved);
    return resolved;
}
