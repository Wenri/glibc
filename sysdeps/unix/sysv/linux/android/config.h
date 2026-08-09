/* config.h substitute for the in-glibc build of the android port.

   fakechroot's autotools configure produced this; compiled into glibc the
   answers are all known statically (we are glibc, on Linux, with GCC).  Only
   the symbols the imported sources actually test are listed -- see
   nix-on-droid/docs/ANDROID-GLIBC.md.  */

#ifndef _FAKECHROOT_CONFIG_H
#define _FAKECHROOT_CONFIG_H

/* Selects the pre-expanded tables in libfakechroot.c instead of Boost.PP.  */
#define ANDROID_PATH_TABLES 1

/* autoconf's AC_INIT package name; used only as the debug()/FAKECHROOT_DEBUG
   message prefix (libfakechroot.c fakechroot_debug).  */
#define PACKAGE "libfakechroot"

/* The chroot base: nix-on-droid's installation prefix.
   SOURCE OF TRUTH: common/modules/android/paths.nix installationDir.  */
#define ANDROID_BASE "/data/data/com.termux.nix/files/usr"

/* execve.c needs both of these; fakechroot's configure supplied them and
   nothing in this tree did, so execve.c did not compile at all until now.

   ANDROID_ELFLOADER is the loader a binary gets re-execed through when its
   PT_INTERP is not already an Android one (execve.c:189, execve.h:118).
   DERIVED, not pasted: common/pkgs/android-glibc.nix already puts
   -DANDROID_GLIBC_LIB="<prefix><out>/lib" on NIX_CFLAGS_COMPILE for every TU,
   so this resolves to the loader shipping in THIS libc and the two can never
   skew -- unlike the LD_PRELOAD build, whose value pointed at a separately
   built android-glibc.  The #error is not decoration: elf/dl-android-paths.h
   defaults ANDROID_GLIBC_LIB to NULL when unset, and `NULL "/ld-..."' would
   silently route every wrapped exec at a garbage path.  */
#ifndef ANDROID_GLIBC_LIB
# error "android: execve needs -DANDROID_GLIBC_LIB (see common/pkgs/android-glibc.nix)"
#endif
#define ANDROID_ELFLOADER ANDROID_GLIBC_LIB "/ld-linux-aarch64.so.1"

/* The loader option that sets the program's argv[0] -- elf/rtld.c:1462.
   Cast because execve.c assigns it into a char ** (execve.c:378, :494) and
   this build runs with -Wwrite-strings.  */
#define ANDROID_ARGV0_OPT ((char *) "--argv0")

/* Syscall numbers.  Order matters: <sys/syscall.h> is glibc's internal wrapper
   (only __NR_*), <disabled-syscall.h> restores the __NR_ that
   process-fakesyscalls.sh moved out of <arch-syscall.h>, and the compat header
   then supplies the SYS_* spellings the imported sources use.  */
#include <sys/syscall.h>
#include <disabled-syscall.h>
#include "sys-syscall-compat.h"

/* Toolchain/libc capabilities -- constant for this build.  */
#define HAVE___ATTRIBUTE__VISIBILITY  1
#define HAVE___ATTRIBUTE__CONSTRUCTOR 1
#define HAVE_SYS_SYSCALL_H            1
#define HAVE_STRCHRNUL                1
#define HAVE_FCHDIR                   1
#define HAVE_VFORK                    1

/* Function-signature macros.

   fakechroot's configure PROBES these with AC_CHECK_FUNC_ARGTYPES
   (m4/check_func_argtypes.m4) because readlink/scandir/bind/... are declared
   differently across libcs.  Compiled into glibc there is nothing to probe:
   the declarations we build against are glibc's own, so the answers are fixed
   and taken straight from its headers (cited per line).  The ARGn forms are
   function-like -- they take the parameter name and expand to a full
   declarator, exactly as AC_DEFINE_UNQUOTED emits them.  */

/* io/unistd.h:838  ssize_t readlink (const char *, char *, size_t)  */
#define READLINK_TYPE_RETURN        ssize_t
#define READLINK_TYPE_ARG3(_)       size_t _

/* dirent/dirent.h:257  int scandir (..., int (*)(const struct dirent *),
                                          int (*)(const struct dirent **,
                                                  const struct dirent **))  */
#define SCANDIR_TYPE_ARG3(_)        int (*_) (const struct dirent *)
#define SCANDIR_TYPE_ARG4(_)        int (*_) (const struct dirent **, \
                                              const struct dirent **)
#define SCANDIR64_TYPE_ARG3(_)      int (*_) (const struct dirent64 *)
#define SCANDIR64_TYPE_ARG4(_)      int (*_) (const struct dirent64 **, \
                                              const struct dirent64 **)

/* socket/sys/socket.h:58-59 -- glibc spells the sockaddr parameter with these
   transparent-union macros, and the imported sources test for exactly that
   spelling via HAVE_<func>_TYPE_ARG2___(CONST_)SOCKADDR_ARG__.  */
#define BIND_TYPE_ARG2(_)           __CONST_SOCKADDR_ARG _
#define CONNECT_TYPE_ARG2(_)        __CONST_SOCKADDR_ARG _
#define GETSOCKNAME_TYPE_ARG2(_)    __SOCKADDR_ARG _
#define GETPEERNAME_TYPE_ARG2(_)    __SOCKADDR_ARG _
#define HAVE_BIND_TYPE_ARG2___CONST_SOCKADDR_ARG__        1
#define HAVE_CONNECT_TYPE_ARG2___CONST_SOCKADDR_ARG__     1
#define HAVE_GETSOCKNAME_TYPE_ARG2___SOCKADDR_ARG__       1
#define HAVE_GETPEERNAME_TYPE_ARG2___SOCKADDR_ARG__       1

/* time/sys/time.h:162  int utimes (const char *, const struct timeval [2])  */
#define UTIMES_TYPE_ARG2(_)         const struct timeval _[2]

/* io/fts.h  int fts_open (..., int (*)(const FTSENT **, const FTSENT **))  */
#define FTS_OPEN_TYPE_ARG3(_)       int (*_) (const FTSENT **, const FTSENT **)

/* ---------------------------------------------------------------------------
   AC_CHECK_FUNCS / AC_CHECK_HEADERS results.

   MOST OF THE IMPORTED SOURCES WRAP THEIR ENTIRE BODY IN `#ifdef HAVE_<FUNC>`
   (94 of the 163 files do).  A missing entry here is therefore not a missing
   optimisation -- the file preprocesses to NOTHING, the wrapper silently does
   not exist, and the only evidence is an `undefined reference to __<func>`
   raised by some unrelated caller (sunrpc, nss) at the libc.so link, long
   after the rename in the glibc source has already taken effect.  That cost a
   build cycle for bind/getpeername/getsockname; fc-<f>.c now carries a
   compile-time assertion so it cannot recur (nix-on-droid/docs/ANDROID-GLIBC.md, precondition 6).

   Compiled into glibc there is nothing to probe: every function below is one
   glibc 2.42 itself provides, so the answer is fixed.  Listed for the WHOLE
   import, not just the wired subset -- an unwired file is never compiled, so
   an entry costs nothing until its wrapper is wired.  */

#define HAVE_ALLOCA_H                              1
#define HAVE_DIRENT_H                              1
#define HAVE_STRUCT_DIRENT_D_RECLEN                1
#define HAVE_SYS_MOUNT_H                           1
#define HAVE_SYS_PARAM_H                           1
#define HAVE_SYS_STATFS_H                          1
#define HAVE___ALIGNOF__                           1

#define HAVE_BIND                                  1
#define HAVE_BINDTEXTDOMAIN                        1
#define HAVE_CANONICALIZE_FILE_NAME                1
#define HAVE_CLEARENV                              1
#define HAVE_CLOSE_RANGE                           1
#define HAVE_CONNECT                               1
#define HAVE_CREAT64                               1
#define HAVE_DLADDR                                1
#define HAVE_DLMOPEN                               1
#define HAVE_DL_ITERATE_PHDR                       1
#define HAVE_EACCESS                               1
#define HAVE_EUIDACCESS                            1
#define HAVE_FCHMODAT                              1
#define HAVE_FCHOWNAT                              1
#define HAVE_FOPEN64                               1
#define HAVE_FREOPEN64                             1
#define HAVE_FSTATAT                               1
#define HAVE_FSTATAT64                             1
#define HAVE_FTS64_OPEN                            1
#define HAVE_FTW                                   1
#define HAVE_FTW64                                 1
#define HAVE_FUTIMESAT                             1
#define HAVE_GETPEERNAME                           1
#define HAVE_GETSOCKNAME                           1
#define HAVE_GETWD                                 1
#define HAVE_GETXATTR                              1
#define HAVE_GET_CURRENT_DIR_NAME                  1
#define HAVE_GLOB64                                1
#define HAVE_GLOB_PATTERN_P                        1
#define HAVE_INOTIFY_ADD_WATCH                     1
#define HAVE_LCHMOD                                1
#define HAVE_LCKPWDF                               1
#define HAVE_LGETXATTR                             1
#define HAVE_LINKAT                                1
#define HAVE_LISTXATTR                             1
#define HAVE_LLISTXATTR                            1
#define HAVE_LREMOVEXATTR                          1
#define HAVE_LSETXATTR                             1
#define HAVE_LSTAT                                 1
#define HAVE_LSTAT64                               1
#define HAVE_LUTIMES                               1
#define HAVE_MEMPCPY                               1
#define HAVE_MKDIRAT                               1
#define HAVE_MKDTEMP                               1
#define HAVE_MKFIFOAT                              1
#define HAVE_MKNODAT                               1
#define HAVE_MKOSTEMP                              1
#define HAVE_MKOSTEMP64                            1
#define HAVE_MKOSTEMPS                             1
#define HAVE_MKOSTEMPS64                           1
#define HAVE_MKSTEMP64                             1
#define HAVE_MKSTEMPS                              1
#define HAVE_MKSTEMPS64                            1
#define HAVE_NFTW                                  1
#define HAVE_NFTW64                                1
#define HAVE_OPEN64                                1
#define HAVE_OPENAT                                1
#define HAVE_OPENAT64                              1
#define HAVE_POSIX_SPAWN                           1
#define HAVE_POSIX_SPAWNP                          1
#define HAVE_RAWMEMCHR                             1
#define HAVE_READLINKAT                            1
#define HAVE_REMOVEXATTR                           1
#define HAVE_RENAMEAT                              1
#define HAVE_RENAMEAT2                             1
#define HAVE_REVOKE                                1
#define HAVE_SCANDIR                               1
#define HAVE_SCANDIR64                             1
#define HAVE_SETXATTR                              1
#define HAVE_STAT64                                1
#define HAVE_STATFS                                1
#define HAVE_STATFS64                              1
#define HAVE_STATVFS                               1
#define HAVE_STATVFS64                             1
#define HAVE_STATX                                 1
#define HAVE_STPCPY                                1
#define HAVE_SYMLINKAT                             1
#define HAVE_TRUNCATE64                            1
#define HAVE_ULCKPWDF                              1
#define HAVE_UNLINKAT                              1
#define HAVE_UTIMENSAT                             1
#define HAVE___CHK_FAIL                            1
#define HAVE___GETCWD_CHK                          1
#define HAVE___GETWD_CHK                           1
#define HAVE___OPEN                                1
#define HAVE___OPEN64                              1
#define HAVE___OPEN64_2                            1
#define HAVE___OPENAT64_2                          1
#define HAVE___OPENAT_2                            1
#define HAVE___OPEN_2                              1
#define HAVE___READLINKAT_CHK                      1
#define HAVE___READLINK_CHK                        1
#define HAVE___REALPATH_CHK                        1
#define HAVE___STATFS                              1

/* Two guard names in the import do not match the function they protect.  Both
   are satisfied today, so these entries change nothing -- they exist so that
   "fixing" the vendor guard does not silently delete a wrapper:

     faccessat.c:23 tests HAVE_FCHMODAT, not HAVE_FACCESSAT (an upstream
       fakechroot copy-paste bug).  HAVE_FACCESSAT is defined below purely so
       that correcting the guard keeps working; do not correct it in the
       import, which is meant to stay byte-for-byte vendor.
     syscall.c:34 tests HAVE_SYS_SYSCALL_H, a header check standing in for a
       function check.  Consequence: that file cannot be disabled by function
       probe, only by removing it from the Makefile tables.  */
#define HAVE_FACCESSAT                             1

/* NEW_GLIBC is referenced by lstat.c, lstat64.c, stat.c, stat64.c, mknod.c,
   mknodat.c, ftw.c and ftw64.c but is deliberately NOT defined: each use is
   `!defined(HAVE___XSTAT64) || NEW_GLIBC', whose first half is already true
   here (the pre-2.33 stat ABI is gone), so the undefined identifier is never
   evaluated.  Left undefined rather than set to 0 to match what a real
   configure run would produce.  */

/* NOT defined, deliberately -- glibc 2.42 does not provide these, and leaving
   them undefined is what selects the modern branch in the imported sources:

     __xstat/__lxstat/__fxstatat/__xmknod family
                                the pre-2.33 stat ABI.  glibc keeps compat
                                symbols for old binaries but no longer declares
                                them, and ships stat/lstat/fstatat/mknod
                                proper -- which fakechroot wraps instead.
     _xftw, _xftw64             glibc-internal legacy; ftw/nftw are the wrappers.
     __opendir2                 BSD only; absent from glibc entirely (verified).
     ndir.h, sys/dir.h, sys/ndir.h
                                pre-POSIX dirent headers; HAVE_DIRENT_H is the
                                one that applies.
     struct dirent.d_namlen     BSD-only field; Linux dirent has d_reclen only.
     FTSENT.fts_name as char *  glibc declares `char fts_name[1]` (io/fts.h:146),
                                an array -- so the char-pointer branch is wrong.

     HAVE_NDIR_H
     HAVE_STRUCT_DIRENT_D_NAMLEN
     HAVE_STRUCT__FTSENT_FTS_NAME_TYPE_CHAR_P
     HAVE_SYS_DIR_H
     HAVE_SYS_NDIR_H
     HAVE__XFTW
     HAVE__XFTW64
     HAVE___FXSTATAT
     HAVE___FXSTATAT64
     HAVE___LXSTAT
     HAVE___LXSTAT64
     HAVE___OPENDIR2
     HAVE___XMKNOD
     HAVE___XMKNODAT
     HAVE___XSTAT
     HAVE___XSTAT64
   --------------------------------------------------------------------------- */

#endif
