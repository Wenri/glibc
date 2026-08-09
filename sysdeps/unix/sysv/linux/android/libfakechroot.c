/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2003-2015 Piotr Roszatycki <dexter@debian.org>
    Copyright (c) 2007 Mark Eichin <eichin@metacarta.com>
    Copyright (c) 2006, 2007 Alexander Shishkin <virtuoso@slind.org>

    klik2 support -- give direct access to a list of directories
    Copyright (c) 2006, 2007 Lionel Tricon <lionel.tricon@free.fr>

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

#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <dlfcn.h>
#include "setenv.h"
#include "wrapper.h"
#include "getcwd_real.h"

/* Compile-time exclude list from configure --with-android-exclude-path */
#if defined(ANDROID_PATH_TABLES)
/* Compiled into glibc: the tables are pre-expanded into path-tables.h, which is
   CHECKED IN beside this file rather than generated at build time.  Its source
   of truth is common/pkgs/android-fakechroot.nix (excludePath / includePath),
   and verify-fc.sh check 6 compares the two -- drift there silently changes
   which paths get translated, moves no symbol and fails no build, so it is the
   most consequential thing in the port and the least visible.

   The Boost.PP path below is left intact and stays: it is header-only and
   clean, and it is what lets this file still build in fakechroot's own tree.  */
#include "path-tables.h"
#elif defined(EXCLUDE_PATH_SEQ)
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/size.hpp>

#define EXCLUDE_PATH_ELEM(r, data, elem) elem,
#define EXCLUDE_LENGTH_ELEM(r, data, elem) sizeof(elem) - 1,

static const char * const exclude_list[] = {
    BOOST_PP_SEQ_FOR_EACH(EXCLUDE_PATH_ELEM, _, EXCLUDE_PATH_SEQ)
};
static const size_t exclude_length[] = {
    BOOST_PP_SEQ_FOR_EACH(EXCLUDE_LENGTH_ELEM, _, EXCLUDE_PATH_SEQ)
};
static const size_t exclude_max = BOOST_PP_SEQ_SIZE(EXCLUDE_PATH_SEQ);
#else
#error "ANDROID_EXCLUDE_PATH must be set at configure time"
#endif

/* Compile-time include list (overrides excludes) from configure */
#if defined(ANDROID_PATH_TABLES)
/* provided by path-tables.h above */
#elif defined(INCLUDE_PATH_SEQ)
static const char * const include_list[] = {
    BOOST_PP_SEQ_FOR_EACH(EXCLUDE_PATH_ELEM, _, INCLUDE_PATH_SEQ)
};
static const size_t include_length[] = {
    BOOST_PP_SEQ_FOR_EACH(EXCLUDE_LENGTH_ELEM, _, INCLUDE_PATH_SEQ)
};
static const size_t include_max = BOOST_PP_SEQ_SIZE(INCLUDE_PATH_SEQ);
#else
#error "ANDROID_INCLUDE_PATH must be set at configure time"
#endif


/* List of environment variables to preserve on clearenv() */
const char * const preserve_env_list[] = {
    "FAKECHROOT_DEBUG",
    "FAKEROOTKEY",
    "FAKED_MODE",
    "LD_LIBRARY_PATH",
    "LD_PRELOAD"
};
const size_t preserve_env_list_count = sizeof preserve_env_list / sizeof preserve_env_list[0];


LOCAL int fakechroot_debug (const char *fmt, ...)
{
    int ret;
    char newfmt[2048];
    va_list ap;

    /* Check FAKECHROOT_DEBUG BEFORE va_start to avoid undefined behavior.
     * Calling va_start without va_end is undefined behavior that can
     * corrupt the stack/heap on some architectures. */
    if (!getenv("FAKECHROOT_DEBUG"))
        return 0;

    va_start(ap, fmt);

    snprintf(newfmt, sizeof(newfmt), PACKAGE ": %s\n", fmt);

    ret = vfprintf(stderr, newfmt, ap);
    va_end(ap);

    return ret;
}


#include "getcwd.h"


/* Check if path matches any prefix in the given list */
static inline bool match_prefix_list(const char *v_path, size_t len,
                                     const char * const *list, const size_t *lengths, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        const size_t prefix_len = lengths[i];

        /* Path must be at least as long as the prefix */
        if (len < prefix_len)
            continue;

        /* Check prefix match */
        if (strncmp(list[i], v_path, prefix_len) != 0)
            continue;

        /* Match if exact or followed by '/' */
        if (len == prefix_len || v_path[prefix_len] == '/')
            return true;
    }
    return false;
}


/* Decide from an already-absolute path.  Split out so the two callers below can
   share it across the frame boundary the next comment explains.  */
static inline bool localdir_decide(const char *v_path)
{
    const size_t len = strlen(v_path);

    /* Include list overrides exclude list */
    if (match_prefix_list(v_path, len, include_list, include_length, include_max))
        return false;  /* NOT local, should translate */

    /* Check exclude list (tail call) */
    return match_prefix_list(v_path, len, exclude_list, exclude_length, exclude_max);
}


/* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream keeps CWD_PATH in
   fakechroot_localdir's own frame, where only the relative-path case uses it.
   GCC sizes a frame at entry, so that charged all 4096 bytes to every call --
   and this function is on EVERY translation, ahead of the wrapper's own buffer
   and rel2abs.  Measured in the built libc.so.6: fakechroot_localdir 4112,
   which put the open() chain at 8272 bytes and lstat() at 16480 -- past this
   architecture's SIGSTKSZ of 16384, on a signal stack, for functions POSIX
   declares async-signal-safe.  noinline keeps the buffer out of the caller.  */
static bool __attribute__ ((noinline))
localdir_from_cwd(void)
{
    char cwd_path[FAKECHROOT_PATH_MAX];

    getcwd_real(cwd_path, FAKECHROOT_PATH_MAX);
    narrow_chroot_path(cwd_path);
    return localdir_decide(cwd_path);
}


/* Check if path is on exclude list (but not on include list) */
LOCAL bool fakechroot_localdir(const char *p_path)
{
    if (!p_path)
        return false;

    /* Expand relative paths */
    if (p_path[0] != '/')
        return localdir_from_cwd();

    return localdir_decide(p_path);
}


/* ADAPTED FOR GLIBC -- not upstream fakechroot.  fakechroot_try_cmd_subst and
   its FAKECHROOT_CMD_SUBST parser lived here and had ZERO callers -- in this
   tree and in the vendor tree alike.  Upstream fork commit 2e7dffc, "Remove
   environment variable support, use compile-time constants only", deleted every
   call site and left the function behind; man/fakechroot.pod still documents the
   variable, which is that commit's stale documentation, not a live feature.
   Removed here rather than re-imported, along with the now-unused
   #include "strchrnul.h" it was the only user of.  Do not restore either on a
   re-import without first restoring a caller.  */
