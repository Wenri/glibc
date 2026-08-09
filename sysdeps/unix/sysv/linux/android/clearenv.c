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

#ifdef HAVE_CLEARENV

#ifndef __GLIBC__
extern char **environ;
#endif

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#include "wrapper.h"

/* ADAPTED FOR THE IN-GLIBC BUILD (see nix-on-droid/docs/ANDROID-GLIBC.md "The adapted files").
   Upstream declares and calls glibc's __clearenv directly, bypassing
   nextcall().  That cannot work here: wrapper() aliases __clearenv to
   __fc_clearenv, so the direct call is the wrapper calling itself --
   SIGSEGV, and invisible to every symbol-level check.

   The call below is UNCONDITIONAL, and deliberately carries no build-time
   guard at all: any such guard belongs on the GLIBC source being renamed,
   never on the fc-*.c shim that compiles this file, so one here would always
   take the wrong branch.  nextcall() is correct in both worlds anyway --
   under LD_PRELOAD it is the dlsym(RTLD_NEXT) stub, which is exactly
   what the direct call was reaching for.  */


wrapper(clearenv, int, (void))
{
    size_t j;
    int n;
    const char *key;
    char *env;
    char **tmpkey, **tmpenv;

    debug("clearenv()");

    /* Preserve old environment variables */
    tmpkey = alloca( (preserve_env_list_count + 1) * sizeof (char *) );
    tmpenv = alloca( (preserve_env_list_count + 1) * sizeof (char *) );

    for (j = 0, n = 0; j < preserve_env_list_count; j++) {
        key = preserve_env_list[j];
        env = getenv(key);
        if (env != NULL) {
            tmpkey[n] = alloca(strlen(key) + 1);
            tmpenv[n] = alloca(strlen(env) + 1);
            strcpy(tmpkey[n], key);
            strcpy(tmpenv[n], env);
            n++;
        }
    }

    tmpkey[n] = NULL;
    tmpenv[n] = NULL;

    /* Clear */
    nextcall(clearenv)();

    /* Set one variable explicitly so environ won't be NULL */
    setenv("FAKECHROOT", "true", 0);

    /* Recover preserved variables */
    for (n = 0; tmpkey[n]; n++) {
        if (setenv(tmpkey[n], tmpenv[n], 1) != 0) {
            return -1;
        }
    }

    return 0;
}

#else
typedef int empty_translation_unit;
#endif
