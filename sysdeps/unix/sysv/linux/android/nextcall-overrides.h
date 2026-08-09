/* Where nextcall(f) lands: the raw, untranslated implementation.

   The wrapper takes over BOTH public `f' and internal `__f' (see
   wrapper.h), so glibc's own implementation is renamed out of the way to
   __android_next_<f> and that is what the wrapper calls through to.

   Add an entry here only when glibc's implementation cannot simply be renamed
   -- e.g. it lives behind a funnel under another name.  Keep this list short:
   a long one means the wiring is fighting glibc's conventions.  */

#ifndef _FAKECHROOT_NEXTCALL_OVERRIDES_H
#define _FAKECHROOT_NEXTCALL_OVERRIDES_H

#define _NEXTCALL_DEFAULT(function) __android_next_##function
#define NEXTCALL_NAME(function)     _NEXTCALL_DEFAULT(function)

/* open: glibc has no plain __open to rename.  sysdeps/unix/sysv/linux/open.c
   defines __libc_open (:31) and makes BOTH __open and open weak aliases of it
   (:49-51), with open64 a further alias of the same code, so the rename trick
   would have to move four names at once; ld.so also calls __open through
   sysdeps/generic/dl-fcntl.h.

   None of that is needed to satisfy nextcall(open).  nextcall means "the raw,
   untranslated implementation", and that is __libc_open -- declared with a
   hidden proto at include/fcntl.h:9 and variadic, matching the wrapper
   prototype in open.h.  This lets rel2absat.c, and therefore every *at
   wrapper, build without wiring the open wrapper at all.

   IT MUST BE __libc_open, NOT __open.  wrapper() unconditionally emits
   strong_alias (__fc_f, __f) (wrapper.h:230), so the day `open' is wired
   __open BECOMES __fc_open -- and an override pointing at __open would make
   nextcall(open) inside __fc_open call itself.  __libc_open is not one of the
   names wrapper(open, ...) takes over, so it stays the untranslated entry
   point for good.  */
#define __android_next_open      __libc_open

/* The rest of the open family, same reasoning.  open64.c defines __libc_open64
   and builds every other name from it, so that one definition is the
   untranslated implementation for all four wrappers; openat64.c likewise
   (there is no __libc_openat).  These names are never taken over by
   wrapper(), so they stay raw for good.  */
#define __android_next_open64    __libc_open64
#define __android_next_openat    __libc_openat64
#define __android_next_openat64  __libc_openat64

/* Collapsed LFS pairs where the family is RENAMED rather than suppressed.
   stat.c/lstat.c/fstatat.c are #if'd to nothing on this arch (precondition 7),
   so only the 64-bit definition exists and only it was renamed.  The non-64
   wrapper's nextcall therefore has no __android_next_<f> of its own --
   point it at the one real implementation, which is what both names meant all
   along.  Without these the build fails with `undefined reference to
   __android_next_stat' from the vendor wrapper itself.  */
#define __android_next_stat      __android_next_stat64
#define __android_next_lstat     __android_next_lstat64
#define __android_next_fstatat   __android_next_fstatat64
#define __android_next_creat     __android_next_creat64
#define __android_next_truncate  __android_next_truncate64
#define __android_next_statfs    __android_next_statfs64
#define __android_next_statvfs   __android_next_statvfs64
#define __android_next_scandir   __android_next_scandir64

/* fopen is a funnel like open: _IO_new_fopen is the definition and every
   public name is an alias of it, so that is what nextcall must reach.  */
#define __android_next_fopen     _IO_new_fopen
#define __android_next_fopen64   _IO_new_fopen

/* Plain funnels: the definition has a different name and every public name
   is an alias of it, so nextcall must reach the definition.  */
#define __android_next_connect    __libc_connect
#define __android_next_system     __libc_system
/* euidaccess is RENAMED (its definition is __euidaccess, which is the __f
   wrapper(euidaccess) claims), so euidaccess needs NO override -- the
   default __android_next_euidaccess is exactly the renamed definition.
   Only eaccess needs one, since both wrappers share that one glibc object.
   Pointing either at __euidaccess is infinite recursion: the glue aliases
   __euidaccess to the wrapper.  */
#define __android_next_eaccess    __android_next_euidaccess

/* mk*stemp*: here the 64 name is the ALIAS and the bare name is the
   definition, so the override runs the other way round.  */
#define __android_next_mkstemp64 __android_next_mkstemp
#define __android_next_mkostemp64 __android_next_mkostemp
#define __android_next_mkstemps64 __android_next_mkstemps
#define __android_next_mkostemps64 __android_next_mkostemps

#endif
