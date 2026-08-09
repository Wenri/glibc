/* execve -- Linux implementation, written so the android port can rename it.

   glibc normally GENERATES this from sysdeps/unix/sysv/linux/syscalls.list:12
   (`execve - execve i:spp __execve execve'), which leaves no .c to rename.  IT
   LIVES HERE, in the arch dir, and not beside that list, because
   make-syscalls.sh:44-48 truncates $sysdirs to directories STRICTLY ABOVE the
   one holding the list before looking for a .c -- so a file next to the list
   does not suppress its entry and the generated stub still defines the symbol,
   colliding with the wrapper.  A file here does outrank it.  See nix-on-droid/docs/ANDROID-GLIBC.md
   "Precondition 10".

   Unlike the other precondition-10 cases in this directory, no
   android-syscall-<subdir> Makefile entry is needed: execve is already named
   in posix/Makefile's routines, so suppressing the stub still leaves something
   that compiles this file.

   Copyright (C) 2025 Free Software Foundation, Inc.
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

#include <unistd.h>
#include <sysdep.h>

#if !IS_IN (rtld)
# define __execve __android_next_execve
/* Precondition 9 does NOT apply here, and adding the usual line would be
   wrong.  INLINE_SYSCALL_CALL pastes the FUNCTION name into __NR_##name, but
   the name being renamed is __execve while the token below is still `execve',
   so __NR_execve resolves as it always did.  Compare aarch64/fchownat.c:37,
   which is a bare-name function and does need the redirect.  */
#endif

int
__execve (const char *path, char *const argv[], char *const envp[])
{
  return INLINE_SYSCALL_CALL (execve, path, argv, envp);
}

/* Reproduce exactly what the generated stub exported, so that dropping execve
   from the wrapper table restores the stock symbol set.  The reference libc has
   __execve, __GI___execve and __GI_execve at one address with `execve' WEAK.
   When the wrapper IS wired it supplies all four itself, via the glue's
   strong_alias plus two libc_hidden_ver.

   strong_alias, NOT hidden_def/libc_hidden_def.  The stub came from
   syscall-template.S:127, i.e. the ASSEMBLY branch of hidden_def, where it is
   just `strong_alias (name, __GI_##name)' (libc-symbols.h:497).  The C branch
   at :467 is a different thing entirely -- it emits __EI_##name aliased to
   __GI_##name and expects a libc_hidden_proto to have redirected the
   definition, which is why using it here failed with

     error: '__EI___execve' aliased to undefined symbol '__GI___execve'

   There is no libc_hidden_proto for execve or __execve (include/unistd.h:115
   declares __execve with plain attribute_hidden), so no redirect exists and the
   aliases must be written out.  */
#if IS_IN (rtld)
strong_alias (__execve, __GI___execve)
weak_alias (__execve, execve)
strong_alias (__execve, __GI_execve)
#endif
