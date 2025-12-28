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

   Additionally, binaries built against standard glibc are redirected
   to use our Android-patched glibc at runtime.

   Returns malloc'd string. Caller must free.  */

#include <string.h>
#include <stdlib.h>

/* The original Nix store path that binaries reference.  */
#define ANDROID_ORIGINAL_STORE "/nix/store"
#define ANDROID_ORIGINAL_STORE_LEN (sizeof (ANDROID_ORIGINAL_STORE) - 1)

/* The actual Nix store path on Android (nix-on-droid).  */
#define ANDROID_REAL_STORE "/data/data/com.termux.nix/files/usr/nix/store"
#define ANDROID_REAL_STORE_LEN (sizeof (ANDROID_REAL_STORE) - 1)

/* Android glibc library path - set via CFLAGS at build time.
   Format: full path to android glibc lib directory, e.g.
   /data/data/com.termux.nix/files/usr/nix/store/XXX-glibc-android-2.40/lib  */
#ifndef ANDROID_GLIBC_LIB
#define ANDROID_GLIBC_LIB NULL
#endif

#ifdef ANDROID_GLIBC_LIB
#define ANDROID_GLIBC_LIB_LEN (sizeof (ANDROID_GLIBC_LIB) - 1)
#else
#define ANDROID_GLIBC_LIB_LEN 0
#endif

/* Check if a path references standard glibc (not android glibc).
   Standard glibc paths match: .../HASH-glibc-VERSION/lib/...
   but do NOT contain "-glibc-android" in the derivation name.  */
static inline int
_dl_is_standard_glibc_path (const char *path)
{
  if (path == NULL)
    return 0;

  /* Look for "-glibc-" pattern in path.  */
  const char *glibc_marker = __builtin_strstr (path, "-glibc-");
  if (glibc_marker == NULL)
    return 0;

  /* Check it's NOT android glibc.  */
  if (__builtin_strncmp (glibc_marker, "-glibc-android", 14) == 0)
    return 0;

  /* Check it's a lib directory path.  */
  if (__builtin_strstr (glibc_marker, "/lib") == NULL)
    return 0;

  return 1;
}

/* Process a library path for Android environment.
   Performs two operations:
   1. Translate /nix/store -> /data/data/.../nix/store
   2. Redirect standard glibc paths -> android glibc

   Returns malloc'd processed path, or NULL if no processing needed.
   Caller must free the returned pointer.  */
static inline char *
_dl_android_process_path (const char *path)
{
  if (path == NULL)
    return NULL;

  const char *current = path;
  char *translated = NULL;

  /* Step 1: Translate /nix/store prefix if present.  */
  if (__builtin_strncmp (path, ANDROID_ORIGINAL_STORE,
                         ANDROID_ORIGINAL_STORE_LEN) == 0)
    {
      size_t suffix_len = __builtin_strlen (path + ANDROID_ORIGINAL_STORE_LEN);
      size_t new_len = ANDROID_REAL_STORE_LEN + suffix_len;

      translated = (char *) malloc (new_len + 1);
      if (translated == NULL)
        return NULL;

      __builtin_memcpy (translated, ANDROID_REAL_STORE, ANDROID_REAL_STORE_LEN);
      __builtin_memcpy (translated + ANDROID_REAL_STORE_LEN,
                        path + ANDROID_ORIGINAL_STORE_LEN,
                        suffix_len + 1);
      current = translated;
    }

  /* Step 2: Check if path needs glibc redirect.  */
  if (ANDROID_GLIBC_LIB != NULL && _dl_is_standard_glibc_path (current))
    {
      /* Find /lib in the path - could be "/lib" at end (RPATH directory)
         or "/lib/" followed by library name.  */
      const char *lib_start = __builtin_strstr (current, "/lib");
      if (lib_start != NULL)
        {
          /* Check what follows /lib: either end of string, '/', or nothing valid.  */
          char next_char = lib_start[4];
          if (next_char == '\0' || next_char == '/')
            {
              /* Extract suffix after "/lib" (empty for directories, or "/libfoo.so").  */
              const char *suffix = lib_start + 4;
              size_t suffix_len = __builtin_strlen (suffix);
              size_t new_len = ANDROID_GLIBC_LIB_LEN + suffix_len;

              char *redirected = (char *) malloc (new_len + 1);
              if (redirected != NULL)
                {
                  __builtin_memcpy (redirected, ANDROID_GLIBC_LIB,
                                    ANDROID_GLIBC_LIB_LEN);
                  __builtin_memcpy (redirected + ANDROID_GLIBC_LIB_LEN,
                                    suffix, suffix_len + 1);
                  free (translated);
                  return redirected;
                }
            }
        }
    }

  /* Return translated path (may be NULL if no translation needed).  */
  return translated;
}

/* Resolve symlinks in a path, translating /nix/store paths along the way.
   This is needed because symlinks in the nix store may point to /nix/store/...
   which doesn't exist on Android - we need to translate to the real path.

   Returns malloc'd resolved path, or NULL on error.
   Caller must free the returned pointer.  */
static inline char *
_dl_android_resolve_path (const char *path)
{
  if (path == NULL)
    return NULL;

  char current[PATH_MAX];
  char link_target[PATH_MAX];
  int loop_count = 0;
  const int max_loops = 40;  /* Same as MAXSYMLINKS */

  /* Start with the input path, possibly translated.  */
  char *translated = _dl_android_process_path (path);
  if (translated != NULL)
    {
      if (__builtin_strlen (translated) >= PATH_MAX)
        {
          free (translated);
          return NULL;
        }
      __builtin_strcpy (current, translated);
      free (translated);
    }
  else
    {
      if (__builtin_strlen (path) >= PATH_MAX)
        return NULL;
      __builtin_strcpy (current, path);
    }

  /* Resolve symlinks iteratively.  */
  while (loop_count++ < max_loops)
    {
      struct stat64 st;
      if (__lstat64 (current, &st) != 0)
        break;  /* Can't stat, return what we have */

      if (!S_ISLNK (st.st_mode))
        break;  /* Not a symlink, we're done */

      /* Read the symlink target.  */
      ssize_t len = __readlink (current, link_target, PATH_MAX - 1);
      if (len < 0)
        break;  /* Can't read link, return what we have */
      link_target[len] = '\0';

      /* Translate the symlink target if needed.  */
      translated = _dl_android_process_path (link_target);
      if (translated != NULL)
        {
          if (__builtin_strlen (translated) >= PATH_MAX)
            {
              free (translated);
              break;
            }
          __builtin_strcpy (current, translated);
          free (translated);
        }
      else
        {
          /* Handle relative symlinks.  */
          if (link_target[0] != '/')
            {
              /* Find directory of current path.  */
              char *last_slash = __builtin_strrchr (current, '/');
              if (last_slash != NULL)
                {
                  size_t dir_len = last_slash - current + 1;
                  if (dir_len + __builtin_strlen (link_target) >= PATH_MAX)
                    break;
                  __builtin_memmove (current + dir_len, link_target,
                                     __builtin_strlen (link_target) + 1);
                }
              else
                {
                  __builtin_strcpy (current, link_target);
                }
            }
          else
            {
              __builtin_strcpy (current, link_target);
            }
        }
    }

  /* Return a copy of the resolved path.  */
  size_t len = __builtin_strlen (current);
  char *result = (char *) malloc (len + 1);
  if (result != NULL)
    __builtin_memcpy (result, current, len + 1);
  return result;
}

#endif /* _DL_ANDROID_PATHS_H */
