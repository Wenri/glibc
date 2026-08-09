/* Copyright (C) 2000-2025 Free Software Foundation, Inc.
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

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "spawn_int.h"

#ifdef ANDROID_SPAWN_FACTION_TRANSLATE
/* ANDROID: the registered PATH is opened by __spawni_child with
   __open_nocancel (sysdeps/unix/sysv/linux/spawni.c:214), which no wrapper
   covers -- so without this the child opens an UNTRANSLATED path, fails, and
   exits 127 with no diagnostic anywhere.  Wiring the vendor's posix_spawn.c
   would not help: it passes file_actions through untouched.

   Translating at REGISTRATION rather than in the child is deliberate.  This
   runs in ordinary context, so it costs the CLONE_VM|CLONE_VFORK child none of
   the stack spawni.c has to size by hand, and raises no question about what is
   safe to call while the parent's address space is shared.

   The sibling chdir action needs nothing: spawni.c:257 calls __chdir, which IS
   a wrapper (Makefile:781), so that path is already translated.  */
# include <config.h>
# include "wrapper.h"
#endif

/* Add an action to FILE-ACTIONS which tells the implementation to call
   `open' for the given file during the `spawn' call.  */
int
__posix_spawn_file_actions_addopen (posix_spawn_file_actions_t *file_actions,
				    int fd, const char *path, int oflag,
				    mode_t mode)
{
  struct __spawn_action *rec;

  if (!__spawn_valid_fd (fd))
    return EBADF;

#ifdef ANDROID_SPAWN_FACTION_TRANSLATE
  char fakechroot_buf[FAKECHROOT_PATH_MAX];
  const char *const xpath = expand_chroot_path (path, fakechroot_buf);
  /* This interface reports errors as a return value, not via errno.  */
  if (xpath == NULL)
    return ENAMETOOLONG;
  char *path_copy = __strdup (xpath);
#else
  char *path_copy = __strdup (path);
#endif
  if (path_copy == NULL)
    return ENOMEM;

  /* Allocate more memory if needed.  */
  if (file_actions->__used == file_actions->__allocated
      && __posix_spawn_file_actions_realloc (file_actions) != 0)
    {
      /* This can only mean we ran out of memory.  */
      free (path_copy);
      return ENOMEM;
    }

  /* Add the new value.  */
  rec = &file_actions->__actions[file_actions->__used];
  rec->tag = spawn_do_open;
  rec->action.open_action.fd = fd;
  rec->action.open_action.path = path_copy;
  rec->action.open_action.oflag = oflag;
  rec->action.open_action.mode = mode;

  /* Account for the new entry.  */
  ++file_actions->__used;

  return 0;
}
weak_alias (__posix_spawn_file_actions_addopen,
	    posix_spawn_file_actions_addopen)
