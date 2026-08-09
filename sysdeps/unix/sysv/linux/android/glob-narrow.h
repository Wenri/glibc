/* Shared result-narrowing for the two glob arms.

   PORT-OWNED -- no upstream fakechroot counterpart, because under LD_PRELOAD
   there is only ever one glob to interpose.  Inside glibc there are two:

     glob/glob64@@GLIBC_2.27   android/glob.c
     glob/glob64@GLIBC_2.17    fc-glob_lstat_compat.c

   They were separate copies of the same loop, and they drifted.  The compat
   arm kept upstream's version long after android/glob.c had fixed it, while
   its own header claimed the bodies "mirror exactly" -- so the copy that was
   asserted to be identical was the one carrying:

     * an unbounded strcpy of a filesystem-supplied path into a
       FAKECHROOT_PATH_MAX stack buffer (glob returns dirname + '/' + d_name,
       which can exceed PATH_MAX, so this was a stack overflow),
     * `rc < 0', which is never true -- glob reports errors as POSITIVE codes,
       so on GLOB_NOMATCH it walked a gl_pathv that was never filled,
     * indexing from 0 under GLOB_DOOFFS, where glibc fills the leading gl_offs
       slots with NULL and stores matches above them -- strcpy from NULL,
     * an open-coded prefix strip that ignored both ANDROID_DISABLE_NARROW and
       the '/'-boundary test, so <BASE-parent>/usrlocal narrowed to "local".

   One inline function, two callers, no room to drift.  Keep it that way: if a
   third glob arm ever appears, it calls this too.

   Not in wrapper.h on purpose -- that header is included by every one of the
   ~114 wrapper TUs, and this needs <glob.h>.  */

#ifndef ANDROID_GLOB_NARROW_H
#define ANDROID_GLOB_NARROW_H 1

/* Strip ANDROID_BASE from each match, in place.

   Under GLOB_DOOFFS the caller reserves gl_offs leading slots, which glibc
   fills with NULL (posix/glob.c:350) and stores the matches ABOVE (:486,
   oldcount = gl_pathc + gl_offs).  When the flag is absent glibc forces
   gl_offs to 0 (:331), so the offset is a no-op there.

   narrow_chroot_path rewrites in place -- stripping a prefix only shortens the
   string -- so there is no temp buffer and no copy of a filesystem-supplied
   path.  It also owns the ANDROID_DISABLE_NARROW check and the boundary rule,
   which is the point of routing through it rather than open-coding a
   strncmp/memmove here.  Call only when glob returned 0; on any error
   gl_pathv may be untouched.  */
static inline void
fc_glob_narrow_results (glob_t *pglob, int flags)
{
    const size_t base = (flags & GLOB_DOOFFS) ? pglob->gl_offs : 0;
    size_t i;

    for (i = 0; i < pglob->gl_pathc; i++)
        narrow_chroot_path (pglob->gl_pathv[base + i]);
}

#endif /* ANDROID_GLOB_NARROW_H */
