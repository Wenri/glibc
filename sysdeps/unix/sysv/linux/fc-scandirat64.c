/* android wrapper shim for scandirat64 / scandirat.  Why shims exist and every
   wiring precondition: nix-on-droid/docs/ANDROID-GLIBC.md.

   There is no vendor android/scandirat.c -- upstream fakechroot never wrapped
   it.  scandir and scandir64 ARE wrapped and were fine, because glibc's
   dirent/scandir.c:26 and scandir64.c:27 reach __opendir, which is a wrapper.
   scandirat instead reaches __opendirat, which is not, so a direct call went to
   the kernel with an untranslated path.

   TWO public names, one address.  This is the LFS collapse (precondition 7)
   with the bare-name shape on top: dirent/scandirat.c is #if'd to nothing under
   _DIRENT_MATCHES_DIRENT64, so scandirat64.c is the live file, it defines the
   public name directly, and glibc adds `weak_alias (scandirat64, scandirat)'.
   The reference agrees, and the BINDINGS DIFFER, which is why this cannot be a
   table entry -- wrapper() emits one public alias, not two:

     readelf --dyn-syms libc.so.6 | grep scandirat
       ... GLOBAL scandirat64@@GLIBC_2.17      \  same address
       ... WEAK   scandirat@@GLIBC_2.17        /

   So: FC_PUBLIC_STRONG gives scandirat64 its GLOBAL binding through the glue,
   and the weak_alias at the tail re-creates scandirat exactly as glibc's own
   arm does.  Getting either binding wrong is ABI drift -- a weak definition
   lets a program supply its own scandirat and win at static link, a strong one
   turns that into a duplicate-symbol error.  */

/* config.h FIRST -- it is where ANDROID_BASE comes from.  Then the strong
   opt-in, which wrapper.h reads when it decides the public alias.  */
#include <config.h>
#define FC_PUBLIC_STRONG 1

/* Mask scandirat's declaration across <dirent.h> only, the same way
   android/glob.c:33-42 masks glob64's and for the same reason.  dirent.h:295
   declares scandirat taking `struct dirent ***', but the weak alias at the tail
   redeclares it as __typeof of a dirent64 function -- and those are two
   DISTINCT struct definitions (bits/dirent.h:22 and :37), identical member for
   member, which _DIRENT_MATCHES_DIRENT64 asserts but the compiler does not
   care about.  Without the mask: "conflicting types for 'scandirat'".

   glibc's own dirent/scandirat64.c writes the alias with no mask and builds,
   which is misleading -- reproduced standalone against the installed headers it
   fails exactly as this did.  Do not copy that file's shape here.  */
#define scandirat __no_scandirat_decl
#include <dirent.h>
#undef scandirat

#include "android/wrapper.h"

wrapper(scandirat64, int, (int dfd, const char * dir,
                           struct dirent64 *** namelist,
                           int (* select) (const struct dirent64 *),
                           int (* cmp) (const struct dirent64 **,
                                        const struct dirent64 **)))
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("scandirat64(%d, \"%s\", &namelist, &select, &cmp)", dfd, dir);

    dir = expand_chroot_path_at(dfd, dir, fakechroot_buf);
    if (dir == NULL) {
        return -1;
    }

    return nextcall(scandirat64)(dfd, dir, namelist, select, cmp);
}

/* One function serves both names, as glibc's own arm does.  WEAK deliberately:
   see the binding table above.  */
weak_alias (__fc_scandirat64, scandirat)
