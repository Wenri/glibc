/* The ld.so SIGSYS handler, compiled a second time into libc.a.
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

/* A statically linked program has no ld.so, so it cannot get the handler the
   way a dynamic one does -- and there is no route by which ld.so could give it
   one.  ld.so does map a static executable (_dl_map_object at elf/rtld.c:1592),
   but at :1600 it hands the map to rtld_chain_load (:1056), which finds no
   DT_NEEDED and no PT_INTERP, discards the mapping and calls __rtld_execve.
   That is a real execve, which resets every signal disposition; and it happens
   at :1601, 789 lines before dl_main's install point, so the installer would
   not have run anyway.  There is no other path: chain_load is skipped only when
   state.mode != rtld_mode_normal, and every such mode (trace, verify, help,
   list-tunables, list-diagnostics) declines to run the program at all.  Hosting
   a static binary in-process instead would mean a second ELF loader inside
   rtld, with hand-rolled TLS, auxv and stack fixup.

   This is worth having rather than accepting parity, because libc.a already
   carries 111 fc-*.o objects -- a statically linked program gets the full path
   translation layer, which LD_PRELOAD could never give it.  The handler is the
   other half, and it is the case Go cares about most, since Go links static by
   default.  (Go's own runtime installs a SIGSYS handler with a raw
   rt_sigaction, bypassing libc, so it still displaces this one; see
   nix-on-droid/docs/SECCOMP.md.  This covers cgo-free C and Rust static binaries.)

   Object identity, not a Makefile subtlety: this is a distinct name from
   dl-sigsys so the rtld .os and the libc.a .o cannot collide in elf/, which
   builds both.  static-only-routines keeps it out of libc.so.6, so a dynamic
   process still has exactly one copy of the state -- ld.so's.  */

#ifndef SHARED
/* Tripwire.  static-only-routines is what actually keeps this out of the shared
   object; the guard makes a mistake there compile to nothing instead of
   silently giving libc.so.6 a second, never-installed copy of the state, which
   would make sigaction chaining depend on which one the linker picked.  */
# include "dl-sigsys.c"
#endif
