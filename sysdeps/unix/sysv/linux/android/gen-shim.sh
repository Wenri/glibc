#!/bin/sh
# Emit one android wrapper shim on stdout.
#
#   gen-shim.sh <name> <strong|plain> <assert|noassert> [none|<globl|weak>:<sym>]
#
# 112 of the port's 117 shims carry no information beyond their own name plus
# three table lookups -- binding, assert, asm -- so they are generated into
# $(objpfx) rather than tracked.  The other 5 have content no table can express:
# a compat_symbol, a real adaptation, or no vendor body to include at all.
# That is the invariant: A CHECKED-IN fc-*.c MEANS SOMETHING NON-OBVIOUS IS
# HAPPENING.  See nix-on-droid/docs/ANDROID-GLIBC.md.
#
# Note the .globl/.set remedy is NOT in that set: it used to need a checked-in
# file, but this script grew its fourth argument and the fourteen android-asm-*
# entries are generated like everything else.
#
# A script rather than a $(if $(filter ...)) chain inside a make recipe on
# purpose.  Both silent failures this port has hit were make-variable scoping
# mistakes (sysdeps/unix/sysv/linux/Makefile documents them), and a recipe is the
# least reviewable place to put branching.  This runs by hand:
#
#   ./gen-shim.sh rmdir plain assert
#
# Which name gets which shape is the android-strong-* tables in
# sysdeps/unix/sysv/linux/Makefile, where each list carries the reason its
# members need a strong public alias.
set -eu

name=${1:?usage: gen-shim.sh <name> <strong|plain> <assert|noassert>}
bind=${2:?usage: gen-shim.sh <name> <strong|plain> <assert|noassert>}
assert=${3:?usage: gen-shim.sh <name> <strong|plain> <assert|noassert> [asm]}
asm=${4:-none}

# Reject anything unrecognised rather than defaulting.  A typo in the Makefile
# table would otherwise silently produce a `plain' shim where `strong' was
# needed, which is a weak public alias where the reference has a global one --
# ABI drift that builds clean and that only verify-fc.sh's binding check
# catches, one function at a time.
case $bind in
  strong|plain) ;;
  *) echo "gen-shim.sh: $name: bad binding '$bind' (want strong|plain)" >&2
     exit 1 ;;
esac
case $assert in
  assert|noassert) ;;
  *) echo "gen-shim.sh: $name: bad assert '$assert' (want assert|noassert)" >&2
     exit 1 ;;
esac

# The precondition-8 remedy, as <globl|weak>:<symbol>, or `none'.  Two columns,
# not three: the target is always __GI_<symbol>.  Both vary per function with no
# rule derivable from the name -- mktemp aliases __mktemp while its siblings
# alias the bare name, statvfs aliases statvfs while statfs aliases __statfs --
# which is why this is table data rather than something the script decides.
asm_bind=${asm%%:*}
asm_sym=${asm#*:}
case $asm in
  none) ;;
  globl:?*|weak:?*) ;;
  *) echo "gen-shim.sh: $name: bad asm '$asm' (want none or globl:<sym>/weak:<sym>)" >&2
     exit 1 ;;
esac

cat <<EOF
/* android wrapper shim for $name -- GENERATED, do not edit and do not commit.
   Generator: android/gen-shim.sh.  Which functions get which shape: the
   android-shim-hand and android-strong-* tables in
   sysdeps/unix/sysv/linux/Makefile.  Why shims exist at all, and every wiring
   precondition: nix-on-droid/docs/ANDROID-GLIBC.md.  */
EOF

if [ "$bind" = strong ]; then
  cat <<EOF

/* The public alias must be strong.  The android-strong-* list naming $name in
   sysdeps/unix/sysv/linux/Makefile carries the reason.  */
#define FC_PUBLIC_STRONG 1
EOF
fi

# Quoted, matching the 20 hand-written shims.  From \$(objpfx) it no longer
# resolves relative to the including file, so it walks the -I list instead and
# lands on +sysdep-includes' -I for sysdeps/unix/sysv/linux.  Do NOT try to make
# that explicit by adding the directory to android-inc -- see the comment there;
# it shadows glibc's internal include/sys/syscall.h wrapper and breaks the
# build.
cat <<EOF

#include "android/$name.c"
EOF

if [ "$assert" = assert ]; then
  cat <<EOF

/* Assert the vendor HAVE_ guard was satisfied (precondition 6): an
   unsatisfied guard makes this a compile error, not an empty object.  */
extern __typeof (__fc_$name) __fc_$name;
EOF
fi

# AFTER the C layer, which is the whole point: this function has a
# libc_hidden_proto in scope in this TU, so every C-level alias lands on
# __GI_<sym> and the public name would stop existing.  Emitting it in assembly
# is below where the redirect can reach (precondition 8).
if [ "$asm" != none ]; then
  cat <<EOF

/* The android-asm-$name entry in sysdeps/unix/sysv/linux/Makefile carries the
   reason this one needs the precondition-8 remedy.  */
__asm__ (".$asm_bind $asm_sym\n\t"
         ".type $asm_sym, %function\n\t"
         ".set $asm_sym, __GI_$asm_sym");
EOF
fi
