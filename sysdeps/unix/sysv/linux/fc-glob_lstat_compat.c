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

   The body mirrors android/glob.c exactly; only the nextcall target
   differs.  Keep the two in step when re-importing fakechroot.  */

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
    int rc, i;

    debug("glob@compat(\"%s\", %d, &errfunc, &pglob)", pattern, flags);
    pattern = expand_chroot_rel_path(pattern, fakechroot_buf);

    rc = __android_next_glob_lstat_compat(pattern, flags, errfunc, pglob);
    if (rc < 0)
        return rc;

    /* Strip ANDROID_BASE prefix from results */
    for (i = 0; i < pglob->gl_pathc; i++) {
        char tmp[FAKECHROOT_PATH_MAX], *tmpptr;

        strcpy(tmp, pglob->gl_pathv[i]);

        const char *ptr = strstr(tmp, ANDROID_BASE);
        if (ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + ANDROID_BASE_LEN;
        }
        strcpy(pglob->gl_pathv[i], tmpptr);
    }
    return rc;
}

/* One function serves both compat names, as glibc's own arm does.  */
strong_alias (__fc_glob_lstat_compat, __fc_glob64_lstat_compat)

compat_symbol (libc, __fc_glob_lstat_compat, glob, GLIBC_2_0);
# if XSTAT_IS_XSTAT64
compat_symbol (libc, __fc_glob64_lstat_compat, glob64, GLIBC_2_0);
# endif

#endif /* SHLIB_COMPAT */
