/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013, 2015 Piotr Roszatycki <dexter@debian.org>

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

#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "setenv.h"
#include "wrapper.h"

#include "strlcpy.h"
#include "dedotdot.h"

#ifdef HAVE___XSTAT64
# include "__xstat64.h"
# define STAT_T stat64
# define STAT(path, sb) nextcall(__xstat64)(_STAT_VER, path, sb)
#else
# include "stat.h"
# define STAT_T stat
# define STAT(path, sb) nextcall(stat)(path, sb)
#endif

#include "getcwd_real.h"

/* ADAPTED FOR GLIBC -- not upstream fakechroot.  Under LD_PRELOAD, nextcall(stat)
   is a dlsym(RTLD_NEXT) lookup and needs no declaration.  Compiled into glibc it
   is a real symbol, and nextcall-overrides.h points __android_next_stat at
   __android_next_stat64, because stat.c is #if'd to nothing on this arch
   (precondition 7 -- the LFS collapse) so only the 64-bit wrapper exists to be
   renamed.  That name is declared only inside fc-stat64.c, by its own wrapper(),
   so this cross-caller has to declare it.

   chroot is the only live case: the other cross-nextcall callers (execve,
   syscall, rel2absat) reach __libc_open, a glibc name its own headers declare,
   and getcwd_real's nextcall(lstat) sits in the dead #else of #ifdef SYS_getcwd.

   __typeof (stat) so the prototype matches what the call site passes -- a
   struct stat *, layout-identical here to the struct stat64 * the
   implementation takes.  glibc makes the same claim with
   strong_alias (__stat64, __stat).  */
extern __typeof (stat) __android_next_stat64;

wrapper(chroot, int, (const char * path))
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    char *ld_library_path, *separator, *new_ld_library_path;
    int status;
    size_t len;
    char cwd[FAKECHROOT_PATH_MAX - 1];
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr = tmp;
    struct STAT_T sb;

    debug("chroot(\"%s\")", path);

    if (!path) {
        __set_errno(EFAULT);
        return -1;
    }

    if (!*path) {
        __set_errno(ENOENT);
        return -1;
    }

    if (getcwd_real(cwd, FAKECHROOT_PATH_MAX - 1) == NULL) {
        __set_errno(EIO);
        return -1;
    }

    /* ADAPTED FOR GLIBC: the expand helpers can now fail, and strlcpy() below
       would dereference NULL.  errno is set by the helper.  */
    if (strstr(cwd, ANDROID_BASE) == cwd) {
        path = expand_chroot_path(path, fakechroot_buf);
        if (path == NULL) {
            return -1;
        }
        strlcpy(tmp, path, FAKECHROOT_PATH_MAX);
        dedotdot(tmpptr);
        path = tmpptr;
    }
    else {
        size_t tmplen;
        if (*path == '/') {
            path = expand_chroot_rel_path(path, fakechroot_buf);
            if (path == NULL) {
                return -1;
            }
            strlcpy(tmp, path, FAKECHROOT_PATH_MAX);
            dedotdot(tmpptr);
            path = tmpptr;
        }
        else {
            snprintf(tmp, FAKECHROOT_PATH_MAX, "%s/%s", cwd, path);
            dedotdot(tmpptr);
            path = tmpptr;
        }
        tmplen = strlen(tmpptr);
        while(tmplen > 1 && tmpptr[tmplen - 1] == '/') {
            tmpptr[--tmplen] = '\0';
        }
    }

    /* Suppress a trailing slash */
    {
        size_t tmplen = strlen(tmpptr);
        if ((tmplen > 1) && tmpptr[tmplen - 1] == '/') {
            tmpptr[tmplen - 1] = '\0';
        }
    }

    if ((status = STAT(path, &sb)) != 0) {
        return status;
    }

    if ((sb.st_mode & S_IFMT) != S_IFDIR) {
        __set_errno(ENOTDIR);
        return -1;
    }

    /* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream sets
       FAKECHROOT_BASE here and reads it back everywhere to find the root.  This
       port makes the root a COMPILE-TIME constant (ANDROID_BASE), so nothing
       reads the variable any more -- no code mentions it outside this note.
       Writing it was pure ceremony, and the ceremony hid the real situation:

       chroot() CANNOT be emulated here, and this function returns 0 anyway, so
       the caller believes it is confined when it is not.  What survives below
       is the LD_LIBRARY_PATH adjustment, which is the part that still does
       something.  Left returning success deliberately -- switching to -1/EPERM
       is more truthful but would break callers that succeed today, and that is
       a behaviour change to make on purpose, not in a memory-safety pass.  See
       nix-on-droid/docs/ANDROID-GLIBC.md "Known limitations".  */

    ld_library_path = getenv("LD_LIBRARY_PATH");

    if (ld_library_path != NULL && strlen(ld_library_path) > 0) {
        separator = ":";
    }
    else {
        ld_library_path = "";
        separator = "";
    }

    len = strlen(ld_library_path)+strlen(separator)+strlen(path)*2+sizeof("/usr/lib:/lib");

    if ((new_ld_library_path = malloc(len)) == NULL) {
        __set_errno(ENOMEM);
        return -1;
    }

    snprintf(new_ld_library_path, len, "%s%s%s/usr/lib:%s/lib", ld_library_path, separator, path, path);
    setenv("LD_LIBRARY_PATH", new_ld_library_path, 1);
    free(new_ld_library_path);

    return 0;
}
