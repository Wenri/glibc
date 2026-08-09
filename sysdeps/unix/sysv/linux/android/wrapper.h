/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010-2015, 2019 Piotr Roszatycki <dexter@debian.org>

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


#ifndef __ANDROID_WRAPPER_H
#define __ANDROID_WRAPPER_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/* No <dlfcn.h>: it was here for the dlsym(RTLD_NEXT) machinery this header
   replaces (see the GLIBC GLUE block below).  Nothing in the port calls dlsym
   or dlopen -- every remaining mention is in a comment -- and this header is
   included by every one of the ~112 wrapper TUs, so it was pure build cost.  */
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rel2abs.h"
#include "rel2absat.h"


#define debug fakechroot_debug


#ifdef HAVE___ATTRIBUTE__VISIBILITY
# define LOCAL __attribute__((visibility("hidden")))
# define PUBLIC __attribute__((visibility("default")))
#else
# define LOCAL
# define PUBLIC
#endif

#ifdef HAVE___ATTRIBUTE__CONSTRUCTOR
# define CONSTRUCTOR __attribute__((constructor))
#else
# define CONSTRUCTOR
#endif

#if defined(PATH_MAX)
# define FAKECHROOT_PATH_MAX PATH_MAX
#elif defined(_POSIX_PATH_MAX)
# define FAKECHROOT_PATH_MAX _POSIX_PATH_MAX
#elif defined(MAXPATHLEN)
# define FAKECHROOT_PATH_MAX MAXPATHLEN
#else
# define FAKECHROOT_PATH_MAX 2048
#endif

#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX 108
#endif


#ifdef AF_UNIX
# ifndef SUN_LEN
#  define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
# endif
#endif

#ifndef __set_errno
# define __set_errno(e) (errno = (e))
#endif

#ifndef HAVE_VFORK
# define vfork fork
#endif

/* Forward declaration needed by inline functions below */
bool fakechroot_localdir (const char *);

/* ANDROID_BASE is guaranteed non-empty by configure */
/* Compile-time constant for ANDROID_BASE length */
#define ANDROID_BASE_LEN (sizeof(ANDROID_BASE) - 1)

static inline void narrow_chroot_path(char *path)
{
    if (path == NULL || *path == '\0') {
        return;
    }

    /* ANDROID_DISABLE_NARROW: suppress prefix stripping in libc-level
     * readback paths (getcwd, readlink, etc.).  Callers that mix libc
     * calls with direct syscalls (e.g. Bun's Zig-based module resolver)
     * see the stripped bare form come back from libc, then feed it into
     * a raw openat that never reaches this code at all — the kernel returns
     * ENOENT because the bare path does not exist on the real filesystem.
     * Setting this env var keeps paths in their real-fs form throughout,
     * so raw syscalls succeed.  The expand direction (add prefix) still
     * runs unchanged.
     *
     * ADAPTED FOR GLIBC: was FAKECHROOT_DISABLE_NARROW.  Renamed because there
     * is no preloaded libfakechroot any more -- this is a knob on libc itself.
     * The name is part of the interface: it is set by
     * common/overlays/{claude-code-launcher.c,go-proot.nix} and
     * common/pkgs/proot-android.nix, and deliberately unset in
     * hosts/nix-on-droid/home.nix, so the two repos must move together with the
     * android-glibc.nix pin.  go-proot calls it REQUIRED.  */
    if (getenv("ANDROID_DISABLE_NARROW")) {
        return;
    }

    char *fakechroot_ptr = strstr(path, ANDROID_BASE);
    if (fakechroot_ptr != path) {
        return;
    }

    const size_t path_len = strlen(path);

    if (path_len == ANDROID_BASE_LEN) {
        path[0] = '/';
        path[1] = '\0';
    }
    else if (path[ANDROID_BASE_LEN] == '/') {
        memmove(path, path + ANDROID_BASE_LEN, 1 + path_len - ANDROID_BASE_LEN);
    }
}

/* ADAPTED FOR GLIBC -- not upstream fakechroot.  These three return NULL on
   failure; upstream's cannot fail.  Two reasons, both memory safety:

   1. The translated form is ANDROID_BASE_LEN bytes longer than PATH, and every
      expansion buffer in this tree is exactly FAKECHROOT_PATH_MAX -- verified
      for fakechroot_buf, old/newpath_buf, abs_filename, resolved, execve.h's
      ctx fields and both alloca() sites in syscall.c.  Upstream memmove()s
      without a bound, so an absolutised path of 4061+ bytes writes 35 bytes
      past the end; in syscall.c the destination is alloca'd, so there is no
      canary between it and the return address.  Rejecting beats truncating: a
      truncated path can name a DIFFERENT EXISTING FILE.  Nothing is lost, since
      the expanded path exceeds the kernel's own PATH_MAX and would be refused
      anyway -- this just returns that errno from here.

   2. rel2abs()/rel2absat() signal failure by returning NULL WITHOUT writing
      RESOLVED.  Upstream ignores that and expands the buffer regardless, so a
      failed getcwd or fchdir leaves the wrapper translating uninitialised
      stack -- and if the garbage happens to start with '/', it gets the
      ANDROID_BASE prefix and is opened.

   The ~115 wrappers that pass the result straight to nextcall() need no change:
   nextcall(f)(NULL, ...) fails, which is the correct outcome.  Callers that
   instead COPY or DEREFERENCE the result must check -- see chroot.c, glob.c,
   realpath.c, execve.c.  */

static inline const char *expand_chroot_rel_path(const char *path, char *buf)
{
    if (path == NULL || *path != '/' || fakechroot_localdir(path)) {
        return path;
    }
    const size_t path_len = strlen(path);
    if (path_len + ANDROID_BASE_LEN >= FAKECHROOT_PATH_MAX) {
        __set_errno(ENAMETOOLONG);
        return NULL;
    }
    /* memmove handles both path==buf (overlap) and path!=buf cases */
    memmove(buf + ANDROID_BASE_LEN, path, path_len + 1);
    memcpy(buf, ANDROID_BASE, ANDROID_BASE_LEN);
    return buf;
}

static inline const char *expand_chroot_path(const char *path, char *buf)
{
    if (path == NULL || fakechroot_localdir(path)) {
        return path;
    }
    if (rel2abs(path, buf) == NULL) {
        return NULL;
    }
    return expand_chroot_rel_path(buf, buf);
}

static inline const char *expand_chroot_path_at(int dirfd, const char *path, char *buf)
{
    if (path == NULL || fakechroot_localdir(path)) {
        return path;
    }
    if (rel2absat(dirfd, path, buf) == NULL) {
        return NULL;
    }
    return expand_chroot_rel_path(buf, buf);
}


/* ------------------------------------------------------------------
   GLIBC GLUE -- replaces the LD_PRELOAD dlsym machinery.

   Under LD_PRELOAD, wrapper()/nextcall() resolved the "real" function
   with dlsym(RTLD_NEXT, ...).  Compiled INTO glibc there is no "next"
   object to search: the real implementation is glibc's own, which by
   convention is the __-prefixed internal name (__mkdir, __access, ...).
   So nextcall(f) is simply __f, and wrapper() defines the public symbol
   f -- glibc's own `weak_alias (__f, f)` is removed at each wired
   function so the public name is free.

   Consequence, and it matches today's behaviour exactly: glibc-INTERNAL
   callers keep calling __f directly and are NOT translated, just as they
   are not translated under LD_PRELOAD today.  Routing internal callers
   through the wrapper is a deliberate, separate change -- do not do it
   as a side effect of wiring a function.

   Where glibc's internal name is not __f, define NEXTCALL_<f> before
   including this header (see nextcall-overrides.h).
   ------------------------------------------------------------------ */

#define wrapper_decl_proto(function)                    /* nothing */
#define wrapper_stub(function, return_type, arguments)  /* nothing */
#define wrapper_decl(function, return_type, arguments)  /* nothing */

#define wrapper_fn_t(function, return_type, arguments) \
    typedef return_type (*fakechroot_##function##_fn_t) arguments

#define wrapper_proto(function, return_type, arguments) \
    extern return_type function arguments; \
    wrapper_fn_t(function, return_type, arguments)

#include "nextcall-overrides.h"

/* Every wrapper defines its body as __fc_<name> and takes over BOTH of glibc's
   entry points:

     f          the public symbol (glibc's weak_alias is guarded out)
     __f        the internal symbol glibc's own code calls
     __GI_f     hidden alias for intra-libc callers of the public name
     __GI___f   hidden alias for intra-libc callers of the internal name
                REQUIRED wherever glibc has libc_hidden_proto (__f) -- e.g.
                include/sys/stat.h:73 for __mkdir -- or the link fails with
                "__EI___f aliased to undefined __GI___f".

   Taking over __f is what makes glibc-INTERNAL callers translated too.  This is
   deliberately a change from the LD_PRELOAD library, which could never reach
   them: dlsym(RTLD_NEXT) only redirects calls that go through the caller's PLT,
   so libc's own use of __open/__lstat64/__getcwd was always untranslated.
   Closing that gap is a principal reason for merging into libc.

   It is safe because translation is idempotent: glibc's internal paths are
   mostly already real (the set-dirs layer rewrote the compile-time _PATH_*
   constants), and an already-real path matches the exclude list and passes
   through unchanged.  Recursion is handled by fakechroot's own design -- rel2abs
   resolves the cwd via getcwd_real, which issues a raw SYS_getcwd precisely so
   it cannot re-enter a wrapper.

   strong_alias and libc_hidden_ver are glibc's own macros; both are no-ops in a
   static build, where hidden_proto marks visibility instead.

   ld.so must never reach these objects, and being IS_IN (libc) does NOT achieve
   that on its own -- that inference was wrong and cost a debugging cycle.
   elf/Makefile's librtld.map step link-probes dl-allobjs.os against libc_pic.a,
   so an IS_IN (libc) object is a first-class candidate to satisfy an ld.so
   undefined symbol.  Exclusion is enforced two ways: the !IS_IN (rtld) guard on
   each renamed glibc source, and -- the part that actually matters -- giving
   ld.so its own untranslated copy via sysdep-rtld-routines (dl-access.c,
   dl-readlink.c).  See nix-on-droid/docs/ANDROID-GLIBC.md "Precondition 4".  */
/* The PUBLIC name is a WEAK alias, matching what glibc's own sources do
   (weak_alias (__mkdir, mkdir), weak_alias (__access, access), ...).  Binding is
   part of the ABI: a weak definition lets a program supply its own mkdir and
   have it win at static link, and a strong one turns that into a duplicate
   symbol error.  This was strong_alias at first; every wrapped symbol went
   WEAK -> GLOBAL and a name-only readelf diff reported zero drift across five
   builds.  verify-fc.sh now compares binding as well.

   The INTERNAL __f stays a strong alias: glibc defines it strongly too, and
   nothing may override it.  */
/* Which binding the PUBLIC alias gets is per-function, because glibc is not
   uniform: where it writes weak_alias (__f, f) the symbol is WEAK (mkdir,
   access, opendir, ...), but where the .c defines the public name directly
   there is no alias at all and the symbol is GLOBAL (mkfifo, rename, statx,
   ...).  Getting this wrong is ABI drift in whichever direction you guessed.
   Default weak; a shim opts in to strong by defining FC_PUBLIC_STRONG before
   including its vendor file, which is why this is tested here and not in the
   vendor source.  Confirm against the reference with
   `nm -D libc.so.6 | grep '\bf@'` -- W or T -- and let verify-fc.sh check it.  */
#if defined FC_NO_PUBLIC_ALIAS
/* The shim emits the public name itself.  Needed when the name carries a
   VERSION: glob lives in two version blocks of posix/Versions (GLIBC_2.0 and
   GLIBC_2.27), so an unversioned alias cannot select a node -- which is exactly
   why glibc uses versioned_symbol/compat_symbol there.  The shim emits those,
   and wrapper() must keep out of the way.  */
# define _FC_PUBLIC_ALIAS(from, to) /* the shim does it */
#elif defined FC_PUBLIC_STRONG
# define _FC_PUBLIC_ALIAS(from, to) strong_alias (from, to)
#else
# define _FC_PUBLIC_ALIAS(from, to) weak_alias (from, to)
#endif

#define wrapper(function, return_type, arguments) \
    wrapper_proto(function, return_type, arguments); \
    extern return_type NEXTCALL_NAME(function) arguments; \
    extern return_type __fc_##function arguments; \
    _FC_PUBLIC_ALIAS (__fc_##function, function) \
    strong_alias (__fc_##function, __##function) \
    libc_hidden_ver (__fc_##function, function) \
    libc_hidden_ver (__fc_##function, __##function) \
    return_type __fc_##function arguments

#define wrapper_alias(function, return_type, arguments) \
    wrapper(function, return_type, arguments)

#define nextcall(function) (NEXTCALL_NAME(function))


#ifdef __GNUC__
# if __GNUC__ >= 6
#  pragma GCC diagnostic ignored "-Wnonnull-compare"
# endif
#endif

#ifdef __clang__
# if __clang_major__ >= 4 || __clang_major__ == 3 && __clang_minor__ >= 6
#  pragma clang diagnostic ignored "-Wpointer-bool-conversion"
# endif
#endif

#ifndef _STAT_VER
 #if defined (__aarch64__)
  #define _STAT_VER 0
 #elif defined (__powerpc__) && __WORDSIZE == 64
  #define _STAT_VER 1
 #elif defined (__riscv) && __riscv_xlen==64
  #define _STAT_VER 0
 #elif defined (__s390x__)
  #define _STAT_VER 1
 #elif defined (__x86_64__)
  #define _STAT_VER 1
 #else
  #define _STAT_VER 3
 #endif
#endif

extern const char * const preserve_env_list[];
extern const size_t preserve_env_list_count;

int fakechroot_debug (const char *, ...);

#endif
