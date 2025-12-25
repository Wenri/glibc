/* Android path translation for nix-on-droid
   Copyright (C) 2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _DL_ANDROID_PATHS_H
#define _DL_ANDROID_PATHS_H

/* nix-on-droid: Path translation for Android environment.

   On Android, the Nix store is not at /nix/store but at:
   /data/data/com.termux.nix/files/usr/nix/store

   ELF binaries from nixpkgs binary cache have hardcoded /nix/store paths
   in their RPATH/RUNPATH. This module intercepts library path lookups
   and redirects them to the real Android location.

   This replaces the pack-audit.so LD_AUDIT module with built-in
   path translation in ld.so itself.  */

#include <string.h>
#include <stdlib.h>

/* The original Nix store path that binaries reference.  */
#define ANDROID_ORIGINAL_STORE "/nix/store"
#define ANDROID_ORIGINAL_STORE_LEN (sizeof (ANDROID_ORIGINAL_STORE) - 1)

/* The actual Nix store path on Android (nix-on-droid).  */
#define ANDROID_REAL_STORE "/data/data/com.termux.nix/files/usr/nix/store"
#define ANDROID_REAL_STORE_LEN (sizeof (ANDROID_REAL_STORE) - 1)

/* Translate a library path from /nix/store to Android prefix.
   Returns a newly allocated string if translation occurred,
   or NULL if no translation was needed.

   The caller must free the returned string if non-NULL.  */
static inline char *
_dl_android_translate_path (const char *name)
{
  /* Quick check: does the path start with /nix/store?  */
  if (name == NULL
      || __builtin_strncmp (name, ANDROID_ORIGINAL_STORE,
                            ANDROID_ORIGINAL_STORE_LEN) != 0)
    return NULL;

  /* Path starts with /nix/store - translate it.  */
  size_t suffix_len = __builtin_strlen (name) - ANDROID_ORIGINAL_STORE_LEN;
  size_t new_len = ANDROID_REAL_STORE_LEN + suffix_len + 1;

  char *result = malloc (new_len);
  if (result == NULL)
    return NULL;

  __builtin_memcpy (result, ANDROID_REAL_STORE, ANDROID_REAL_STORE_LEN);
  __builtin_memcpy (result + ANDROID_REAL_STORE_LEN,
                    name + ANDROID_ORIGINAL_STORE_LEN,
                    suffix_len + 1);

  return result;
}

#endif /* _DL_ANDROID_PATHS_H */
