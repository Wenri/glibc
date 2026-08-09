/* Pre-expanded path-policy tables for the in-glibc build.

   fakechroot's own build derives these from Boost.PP sequences handed over by
   configure (--with-android-exclude-path / --with-android-include-path).  glibc
   cannot take a Boost dependency, so the same two lists are expanded here.

   THIS FILE IS THE SOURCE OF TRUTH.  It used to be a copy of
   common/pkgs/android-fakechroot.nix (excludePath / includePath), which
   configured fakechroot's own build; that package is gone, and with it the only
   other copy.  verify-fc.sh check 6 diffed the two, and when the .nix
   disappeared it printed "skipped" WITHOUT failing -- so edit this file knowing
   nothing cross-checks its CONTENT any more.  What is still checked is its
   internal consistency: <name>_length[] must stay index-parallel with
   <name>_list[], which is where a hand edit actually goes wrong.

   Semantics are in libfakechroot.c:match_prefix_list -- a prefix matches only
   when followed by '/' or end-of-string, and the include list overrides the
   exclude list.

   That boundary rule is also what makes translation IDEMPOTENT: the real base
   /data/data/com.termux.nix/... matches exclude "/data" but NOT include
   "/data/data/com.termux" (next char is '.'), so an already-translated path is
   treated as local and left alone.  */

#ifndef _FAKECHROOT_PATH_TABLES_H
#define _FAKECHROOT_PATH_TABLES_H

#include <stddef.h>

static const char * const exclude_list[] = {
    "/3rdmodem",
    "/acct",
    "/apex",
    "/android",
    "/bugreports",
    "/cache",
    "/config",
    "/d",
    "/data",
    "/data_mirror",
    "/debug_ramdisk",
    "/dev",
    "/linkerconfig",
    "/log",
    "/metadata",
    "/mnt",
    "/odm",
    "/odm_dlkm",
    "/oem",
    "/proc",
    "/product",
    "/sdcard",
    "/storage",
    "/sys",
    "/system",
    "/system_ext",
    "/vendor",
    "/vendor_dlkm",
};
static const size_t exclude_length[] = {
    sizeof("/3rdmodem") - 1,
    sizeof("/acct") - 1,
    sizeof("/apex") - 1,
    sizeof("/android") - 1,
    sizeof("/bugreports") - 1,
    sizeof("/cache") - 1,
    sizeof("/config") - 1,
    sizeof("/d") - 1,
    sizeof("/data") - 1,
    sizeof("/data_mirror") - 1,
    sizeof("/debug_ramdisk") - 1,
    sizeof("/dev") - 1,
    sizeof("/linkerconfig") - 1,
    sizeof("/log") - 1,
    sizeof("/metadata") - 1,
    sizeof("/mnt") - 1,
    sizeof("/odm") - 1,
    sizeof("/odm_dlkm") - 1,
    sizeof("/oem") - 1,
    sizeof("/proc") - 1,
    sizeof("/product") - 1,
    sizeof("/sdcard") - 1,
    sizeof("/storage") - 1,
    sizeof("/sys") - 1,
    sizeof("/system") - 1,
    sizeof("/system_ext") - 1,
    sizeof("/vendor") - 1,
    sizeof("/vendor_dlkm") - 1,
};
static const size_t exclude_max =
    sizeof exclude_list / sizeof exclude_list[0];

static const char * const include_list[] = {
    "/dev/shm",
    "/data/data/com.termux",
};
static const size_t include_length[] = {
    sizeof("/dev/shm") - 1,
    sizeof("/data/data/com.termux") - 1,
};
static const size_t include_max =
    sizeof include_list / sizeof include_list[0];

#endif
