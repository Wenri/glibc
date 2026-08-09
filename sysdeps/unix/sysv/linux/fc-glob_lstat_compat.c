/* the android port wrapper for the COMPAT glob arm (glob@GLIBC_2.17).

   This is the one wrapper written by hand rather than imported: fakechroot has
   no __glob_lstat_compat.c, because under LD_PRELOAD there is only ever one
   glob to interpose.  Inside glibc there are two, at two addresses:

     glob/glob64@@GLIBC_2.27   linux/glob.c,              GLOB_LSTAT = gl_lstat
     glob/glob64@GLIBC_2.17    linux/glob-lstat-compat.c, GLOB_LSTAT = gl_stat

   Wiring only the modern one would leave binaries bound to the compat version
   reaching untranslated code -- a silent behaviour split, invisible to the ABI
   diff because both symbols would still be present.  So each arm gets its own
   wrapper and keeps its own semantics: this file translates paths for the
   compat implementation without changing its stat-vs-lstat behaviour, which
   lives entirely in the renamed implementation it calls through to.

   This body once claimed to mirror android/glob.c "exactly"; it did not, and
   the claim is why nobody looked.  android/glob.c was fixed and this copy was
   not, so the compat arm kept an unbounded strcpy of a filesystem-supplied
   path into a stack buffer, an `rc < 0' test that is never true, a NULL deref
   under GLOB_DOOFFS, and a prefix strip that ignored ANDROID_DISABLE_NARROW.

   The shared half now lives in android/glob-narrow.h and there is nothing left
   to keep in step: only the nextcall target and the version tags differ.  */

#define glob64   __no_glob64_decl
#define __glob64 __no___glob64_decl
#include <glob.h>
#undef glob64
#undef __glob64
#include <string.h>
#include <shlib-compat.h>
#include <sys/stat.h>
#include <kernel_stat.h>

#include "android/wrapper.h"
#include "android/glob-narrow.h"

#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_27)

/* glibc's compat implementation, renamed by linux/glob-lstat-compat.c.  */
extern int __android_next_glob_lstat_compat (const char *, int,
                                                int (*) (const char *, int),
                                                glob_t *);

/* NOT static: compat_symbol emits a .symver, and binutils will not export a
   LOCAL symbol -- glob@GLIBC_2.17 silently vanished from the dynamic table
   while glob64@GLIBC_2.17 survived, because strong_alias re-declares its
   target extern.  glibc's own __glob_lstat_compat is global too.  */
int
__fc_glob_lstat_compat (const char *pattern, int flags,
                        int (*errfunc) (const char *, int), glob_t *pglob)
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    int rc;

    debug("glob@compat(\"%s\", %d, &errfunc, &pglob)", pattern, flags);
    pattern = expand_chroot_rel_path(pattern, fakechroot_buf);
    /* Too long to translate: no file can bear that name.  Checked because
       glibc's glob dereferences the pattern immediately.  */
    if (pattern == NULL)
        return GLOB_NOMATCH;

    rc = __android_next_glob_lstat_compat(pattern, flags, errfunc, pglob);
    /* Errors are POSITIVE (GLOB_NOSPACE 1, GLOB_ABORTED 2, GLOB_NOMATCH 3).
       The old `rc < 0' never fired, so a failed glob fell through and walked a
       gl_pathv that was never filled.  */
    if (rc != 0)
        return rc;

    fc_glob_narrow_results(pglob, flags);
    return rc;
}

/* One function serves both compat names, as glibc's own arm does.  */
strong_alias (__fc_glob_lstat_compat, __fc_glob64_lstat_compat)

compat_symbol (libc, __fc_glob_lstat_compat, glob, GLIBC_2_0);
# if XSTAT_IS_XSTAT64
compat_symbol (libc, __fc_glob64_lstat_compat, glob64, GLIBC_2_0);
# endif

#endif /* SHLIB_COMPAT */
