#!/bin/sh
# Regenerate sys-syscall-compat.h after re-importing fakechroot.
#
# Run from this directory:   ./regen-sys-syscall-compat.sh > sys-syscall-compat.h
# Or just audit the current header:   ./regen-sys-syscall-compat.sh --check
#
# Why this is a script and not a comment in the header: the second half below is
# a sed expression containing `*/', which silently terminates a C block comment.
# It did exactly that once.
#
# Why the second half exists at all: a plain grep for SYS_[a-z0-9_]+ misses the
# REDIRECT_ENTRY targets.  The table names each target only as the third field
# of a tuple, and SYS_GEN_* pastes the spelling together with
# BOOST_PP_CAT (SYS_, to) -- so the token `SYS_accept4' appears nowhere in the
# sources for a grep to find.  SYS_accept4, SYS_recvfrom, SYS_sendto and
# SYS_getpgid were all absent until dl-sigsys.c's #error caught it.
set -e

cd "$(dirname "$0")"

names ()
{
  # Half 1: every SYS_* token written out literally, minus this header's own
  # prose (SYS_foo) and the seccomp si_code constant, which is not a syscall.
  grep -rhoE 'SYS_[a-z0-9_]+' ./*.c ./*.h \
    | grep -vx 'SYS_foo' \
    | grep -vx 'SYS_SECCOMP'

  # Half 2: the pasted REDIRECT_ENTRY targets.
  sed -n 's|^#define REDIRECT_ENTRY_[a-z0-9_]* ((\([A-Z0-9]*\), *\([a-z0-9_]*\), *\([a-z0-9_]*\).*|SYS_\3|p' \
    syscall.h
}

if [ "$1" = "--check" ]; then
  missing=$(names | sort -u | while read -r n; do
    grep -q "^# define $n " sys-syscall-compat.h || echo "$n"
  done)
  if [ -n "$missing" ]; then
    echo "sys-syscall-compat.h is missing:" >&2
    echo "$missing" >&2
    exit 1
  fi
  echo "sys-syscall-compat.h: all $(names | sort -u | wc -l) spellings present"
  exit 0
fi

sed -n '1,/^#define _FAKECHROOT_SYS_SYSCALL_COMPAT_H$/p' sys-syscall-compat.h
echo

names | sort -u | while read -r n; do
  nr="__NR_${n#SYS_}"
  printf '#if !defined %s && defined %s\n# define %s %s\n#endif\n' "$n" "$nr" "$n" "$nr"
done

echo
echo '#endif /* _FAKECHROOT_SYS_SYSCALL_COMPAT_H */'
