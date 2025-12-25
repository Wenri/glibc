/* Initialization in nss_files module.
   Copyright (C) 2011-2024 Free Software Foundation, Inc.
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

#ifdef USE_NSCD

#include <string.h>
#include <nscd/nscd.h>
#include <nss.h>
#include <nss_files.h>

static void
register_file (void (*cb) (size_t, struct traced_file *),
               int db, const char *path, int crinit)
{
  size_t pathlen = strlen (path) + 1;
  struct traced_file *file = malloc (sizeof (struct traced_file) + pathlen);
  /* Do not register anything on memory allocation failure.  nscd will
     fail soon anyway.  */
  if (file != NULL)
    {
      init_traced_file (file, path, crinit);
      cb (db, file);
    }
}

void
_nss_files_init (void (*cb) (size_t, struct traced_file *))
{
  register_file (cb, pwddb, "/data/data/com.termux.nix/files/usr/etc/passwd", 0);
  register_file (cb, grpdb, "/data/data/com.termux.nix/files/usr/etc/group", 0);
  register_file (cb, hstdb, "/system/etc/hosts", 0);
  register_file (cb, hstdb, "/data/data/com.termux.nix/files/usr/etc/resolv.conf", 1);
  register_file (cb, servdb, "/data/data/com.termux.nix/files/usr/etc/services", 0);
  register_file (cb, netgrdb, "/data/data/com.termux.nix/files/usr/etc/netgroup", 0);
}
libc_hidden_def (_nss_files_init)

#endif
