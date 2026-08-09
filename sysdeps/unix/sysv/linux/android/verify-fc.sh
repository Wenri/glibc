#!/usr/bin/env bash
# Verify a fakechroot-in-glibc build (in-tree copy -- referenced by nix-on-droid/docs/ANDROID-GLIBC.md).  A green build is NOT evidence -- every
# silent-absence mode this port has hit produced a clean compile.
#
#   verify-fc.sh <out-path-of-glibc-2.42-fc> [wrapper ...]
set -uo pipefail

SRCDIR=$(cd "$(dirname "$0")" && pwd)
OUT=${1:?usage: verify-fc.sh /nix/store/...-glibc-2.42-fc [wrapper ...]}
shift
WRAPPERS=("$@")
# Default to EVERY wrapper, derived from the Makefile tables rather than a
# literal list.  The list here used to be 13 hand-written names out of ~110, so
# check 3 -- the only thing that proves an alias set sits at one address with the
# right binding -- covered an eighth of the port and silently ignored whatever
# was added after the list was written.  Deriving it means a new wrapper is
# covered the day it is named.
MK=$SRCDIR/../Makefile
if [ ${#WRAPPERS[@]} -eq 0 ]; then
  mapfile -t WRAPPERS < <(sed -n '/^android-wrappers-[a-z]* *:=/,/[^\\]$/p' "$MK" |
                            sed 's/#.*//; s/^android-wrappers-[a-z-]* *:= *//' |
                            tr -s ' \\\n' '\n' | grep -vE '^$' | sort -u)
  [ ${#WRAPPERS[@]} -lt 100 ] && {
    echo "  BUG: derived only ${#WRAPPERS[@]} wrappers from $MK -- table format changed?"
    exit 1; }
fi

BASE=${FC_ABI_REF:-/nix/store/qccvr8lh5g67h04ca1l60viyfvbh344l-glibc-2.42-a1}   # deployed reference; override via FC_ABI_REF
LIBC=$OUT/lib/libc.so.6
LD=$(echo "$OUT"/lib/ld-linux-*.so.1)
fail=0
note() { printf '%-58s %s\n' "$1" "$2"; }

echo
echo "== 0. nextcall overrides must not name a symbol the glue claims =="
# A target of `f' or `__f' is infinite recursion: wrapper() aliases both to
# __fc_<f>, so nextcall(f) re-enters the wrapper.  Every build-time check still
# passes -- the symbols are all correct and distinct -- so only this static
# audit or actually running the code will catch it.  euidaccess shipped this way
# and segfaulted localedef during glibc's own install phase.
awk '/^#define __android_next_/{
       f=$2; sub(/^__android_next_/,"",f); t=$3;
       if (t=="__" f || t==f) { printf "  RECURSIVE: %s -> %s\n", $2, t; bad++ }
     } END { if (!bad) printf "  %-56s %s\n", "all overrides point outside the claimed set", "ok" }' \
  "$SRCDIR/nextcall-overrides.h"
if awk '/^#define __android_next_/{f=$2; sub(/^__android_next_/,"",f);
        if ($3=="__" f || $3==f) exit 1}' "$SRCDIR/nextcall-overrides.h"; then :; else fail=1; fi

echo "== 1. ld.so must be fakechroot-free =="
# Plain nm, NOT nm -u.  The failure mode is not an undefined symbol:
# elf/Makefile link-probes dl-allobjs.os against libc_pic.a, so a stray
# reference to fakechroot_debug is SATISFIED by dragging fc-libfakechroot.os in,
# after which the symbol is defined and nm -u reports nothing -- and -Wl,-z,defs
# is silent for the same reason.  Checked safe against false positives: ld.so's
# own dl-getcwd/dl-access define __getcwd/__access, which match no pattern here.
leak=$(nm "$LD" 2>/dev/null | grep -Ei 'fakechroot|__fc_|rel2abs|getcwd_real' | wc -l)
[ "$leak" -eq 0 ] && note "undefined fakechroot syms in ld.so" "0  ok" \
                  || { note "undefined fakechroot syms in ld.so" "$leak  FAIL"; fail=1; }
# ALL SEVEN dual-compiled sources, not just the first two.  access.c, readlink.c,
# stat64.c, lstat64.c, fstatat64.c, getcwd.c and opendir.c are each compiled
# twice from the same text -- once for libc, once via their dl-*.c twin for
# ld.so.  In the rtld copy the source's `#if IS_IN (rtld)' arm is the ONLY thing
# re-emitting these names; get that arm wrong and the loader silently loses a
# symbol it needs (elf/dl-android-paths.h:218,225, elf/dl-load.c:1833,
# sysdeps/generic/dl-unistd.h).  This check only covered __access and __readlink
# while the other five were unprotected.
for s in __access __readlink __stat64 __lstat64 __fstatat64 __getcwd __opendir; do
  n=$(nm "$LD" 2>/dev/null | grep -c " $s\$")
  [ "$n" -ge 1 ] && note "ld.so has its own $s" "ok" || { note "ld.so has its own $s" "MISSING"; fail=1; }
done

echo
echo "== 2. exported ABI vs deployed glibc-2.42-a1 =="
if [ -d "$BASE" ]; then
  # BINDING IS PART OF THE ABI.  A name-only diff reported zero drift while every
  # wrapped symbol had silently gone weak -> global, which turns a legitimate
  # user override of e.g. mkdir into a duplicate-symbol error at static link.
  syms() { readelf --dyn-syms --wide "$1" \
             | awk 'NF>7 && $4!="SECTION"{print $5, $8}' | sort -u; }
  # Deliberate additions, subtracted below and PRINTED so they cannot become
  # invisible.  Do not instead filter UND rows out of syms(): the seccomp half
  # adds an undefined reference to libc and a matching definition to ld.so, and
  # dropping UND would blind this check to that whole class of drift.
  # WEAK, not GLOBAL: libc's reference is deliberately weak so libc.a (no ld.so)
  # and a libc.so under a mismatched loader both degrade to no interception.
  libc_allow=${FC_ABI_EXPECTED_ADDS_LIBC:-'WEAK _dl_sigsys_exchange@GLIBC_PRIVATE'}
  ld_allow=${FC_ABI_EXPECTED_ADDS_LD:-'GLOBAL _dl_sigsys_exchange@@GLIBC_PRIVATE'}

  # Both libraries, same comparison.  ld.so matters more than libc here: it is
  # what proves the port adds exactly ONE export and not two.
  for pair in "libc.so.6|$LIBC|$libc_allow" "ld.so|$LD|$ld_allow"; do
    IFS='|' read -r what new allow <<<"$pair"
    base=$BASE/lib/$(basename "$new")
    [ -e "$base" ] || base=$(echo "$BASE"/lib/ld-linux-*.so.1)
    syms "$base" > /tmp/abi.base
    syms "$new"  > /tmp/abi.new
    printf '%s\n' "$allow" | sort -u > /tmp/abi.allow
    miss=$(comm -23 /tmp/abi.base /tmp/abi.new | wc -l)
    add=$(comm -13 /tmp/abi.base /tmp/abi.new | comm -23 - /tmp/abi.allow | wc -l)
    tot=$(wc -l < /tmp/abi.base)
    note "$what dynamic symbols (base $tot)" "missing=$miss added=$add"
    while read -r a; do [ -n "$a" ] && note "  allowed addition" "$a"; done < /tmp/abi.allow
    [ "$miss" -eq 0 ] && [ "$add" -eq 0 ] || { fail=1; echo "  --- drift ---"
      { comm -23 /tmp/abi.base /tmp/abi.new
        comm -13 /tmp/abi.base /tmp/abi.new | comm -23 - /tmp/abi.allow
      } | head -20 | sed 's/^/    /'; }
  done
else
  note "reference $BASE" "NOT PRESENT - skipped"
fi

echo
echo "== 2b. SIGSYS redirect table: dl-sigsys.c vs REDIRECT_SEQ =="
# This is what replaces the safety Boost.PP was buying.  dl-sigsys.c hand-expands
# the redirect cases because the vendor macros need Boost and emit
# nextcall(syscall), which is libc-only -- so nothing but this check couples the
# two.  A REDIRECT_ENTRY added upstream would otherwise be silently unhandled.
DLS=$(dirname "$SRCDIR")/dl-sigsys.c
tbl=$(sed -n 's|^#define REDIRECT_ENTRY_[a-z0-9_]* ((\([A-Z0-9]*\), *\([a-z0-9_]*\),.*|\2|p' \
        "$SRCDIR/syscall.h" | sort -u)
imp=$(sed -n 's|^ *case SYS_\([a-z0-9_]*\):.*|\1|p' "$DLS" | sort -u)
# Only the entries whose source syscall exists on THIS arch are required: the
# rest expand to nothing in the vendor sequence too.  aarch64 explicitly, because
# dl-sigsys.c's body is #ifdef __aarch64__ -- if it ever grows a second arch,
# this has to grow with it.  Globbing */arch-syscall.h instead matches every
# architecture at once and reports all twelve entries as missing.
ARCHSYS=$(dirname "$SRCDIR")/aarch64/arch-syscall.h
need=$(for n in $tbl; do
         grep -qE "^# *define __NR_$n " "$ARCHSYS" 2>/dev/null && echo "$n"
       done | sort -u)
# gap is measured against the ARCH-FILTERED set: an entry whose __NR_ does not
# exist here needs no case.  extra is measured against the FULL table, not the
# filtered one -- a case in dl-sigsys.c is wrong only if REDIRECT_SEQ does not
# describe it at all.
#
# Comparing extra against $need instead reports every case as bogus WHENEVER THE
# TREE HAS BEEN BUILT, because process-fakesyscalls.sh deletes __NR_ for exactly
# the syscalls the handler exists to emulate -- so arch-syscall.h in a build
# directory has no __NR_faccessat2, __NR_accept or __NR_fchmodat2 and $need comes
# out empty.  Running from a pristine source tree hid this; it surfaced the first
# time the check ran inside the build.
gap=$(comm -23 <(echo "$need") <(echo "$imp"))
extra=$(comm -13 <(echo "$tbl") <(echo "$imp"))
if [ -z "$gap" ] && [ -z "$extra" ]; then
  note "redirect cases match REDIRECT_SEQ" \
       "$(echo "$imp" | grep -c .) cases, $(echo "$need" | grep -c .) required here  ok"
else
  [ -n "$gap" ]   && note "in REDIRECT_SEQ, missing from dl-sigsys.c" "$(echo $gap)"
  [ -n "$extra" ] && note "in dl-sigsys.c, absent from REDIRECT_SEQ"  "$(echo $extra)"
  fail=1
fi

# The case labels agreeing is not enough -- a case redirecting to the WRONG
# target passes that and every other check.  So diff the from->to pairs.
# Over $imp, not $need: the cases that actually exist are the ones whose targets
# can be wrong, and $need is empty in a built tree for the reason above.
badto=$(for n in $imp; do
          want=$(sed -n "s|^#define REDIRECT_ENTRY_$n ((\([A-Z0-9]*\), *$n, *\([a-z0-9_]*\).*|\2|p" \
                   "$SRCDIR/syscall.h")
          got=$(awk -v n="$n" '$0 ~ "case SYS_"n":" {f=1}
                               f && /INTERNAL_SYSCALL_NCS_CALL/ {
                                 if (match ($0, /SYS_[a-z0-9_]+/))
                                   { print substr ($0, RSTART+4, RLENGTH-4); exit } }' "$DLS")
          [ "$want" = "$got" ] || echo "$n: REDIRECT_SEQ says $want, dl-sigsys.c calls ${got:-nothing}"
        done)
if [ -z "$badto" ]; then
  note "redirect targets match REDIRECT_SEQ" "ok"
else
  echo "$badto" | sed 's/^/    /'
  fail=1
fi

echo
echo "== 3. each wrapper: all aliases at one address, next-call present =="
for f in "${WRAPPERS[@]}"; do
  addrs=$(nm "$LIBC" | awk -v f="$f" '
      $3=="__fc_"f || $3==f || $3=="__"f || $3=="__GI_"f || $3=="__GI___"f {print $1}' | sort -u)
  n=$(nm "$LIBC" | awk -v f="$f" '
      $3=="__fc_"f || $3==f || $3=="__"f || $3=="__GI_"f || $3=="__GI___"f' | wc -l)
  # nextcall(f) is either a renamed glibc symbol (__android_next_f exists) or
  # a #define to a name glibc already has (nextcall-overrides.h).  Both are
  # correct; which one applies is a property of how f was wired, so read the
  # header rather than assuming the renamed form.
  ovr=$(sed -n "s/^#define[[:space:]]*__android_next_$f[[:space:]]\+\([A-Za-z_0-9]*\).*/\1/p" \
          "$SRCDIR/nextcall-overrides.h" 2>/dev/null | head -1)
  if [ -n "$ovr" ]; then
    # the override target must exist and must NOT be the wrapper itself
    nxt=$(nm "$LIBC" | awk -v t="$ovr" '$3==t || $3=="__GI_"t' | wc -l)
    [ "$nxt" -gt 0 ] && nxt=1
  else
    nxt=$(nm "$LIBC" | grep -c " __android_next_$f\$")
  fi
  na=$(echo "$addrs" | grep -c .)
  # The PUBLIC symbol must exist in the dynamic table at the wrapper's address.
  # Counting aliases is not a usable test: how many of __f / __GI_f / __GI___f
  # exist varies with whether a libc_hidden_proto is in scope (precondition 8),
  # and FC_NO_PUBLIC_ALIAS shims emit the public name from a versioned_symbol
  # rather than from the glue.  What must always hold is that every name that
  # DOES exist shares one address, and that the exported one is among them.
  wrap=$(nm "$LIBC" | awk -v f="$f" '$3=="__fc_"f{print $1; exit}')
  # Match the DEFAULT version (f or f@@VER).  A multi-version function such as
  # glob also exports f@VER from a different implementation with its own
  # wrapper, so matching a single "@" would compare against the wrong arm.
  #
  # A few wrappers do not export their own name: the compat glob arm is built
  # as glob_lstat_compat but exports glob@GLIBC_2.17, so look up what it
  # actually publishes rather than reporting a spurious FAIL.
  case $f in
    glob_lstat_compat) pubname='glob@GLIBC_2.17' ;;
    *)                 pubname=$f ;;
  esac
  pub=$(readelf --dyn-syms --wide "$LIBC" \
          | awk -v f="$pubname" '$8==f || $8 ~ "^"f"@@"{print $2; exit}')
  # Compare as normalised STRINGS, never arithmetic.  $((16#$pub)) with an empty
  # $pub is a fatal arithmetic error that ABORTS THE LOOP -- and because the
  # abort skipped the fail=1 below, the script went on to print ALL CHECKS
  # PASSED having silently checked only the wrappers before the first such name.
  # That is exactly the shape of failure this whole script exists to catch.
  if [ -n "$wrap" ] && [ -n "$pub" ] && [ "$na" -eq 1 ] && [ "$nxt" -eq 1 ] \
     && [ "${pub#"${pub%%[!0]*}"}" = "${wrap#"${wrap%%[!0]*}"}" ]; then
    note "  $f" "names=$n one-address exported next=$nxt  ok"
  else
    note "  $f" "names=$n addresses=$na exported=${pub:-NONE} wrapper=${wrap:-NONE} next=$nxt  FAIL"
    fail=1
  fi
done

echo
echo "== 4. vendor files must differ only where the port says they do =="
# The split between "imported unchanged" and "adapted" is the port's core
# contract, and nothing enforced it until this check existed.
#
# Renaming libfakechroot.h to wrapper.h touched the #include line in 142
# otherwise-pristine files, so "differs from upstream" is no longer the same
# question as "was adapted".  Comparing name lists would now pass while someone
# quietly rewrote the body of open.c.  So compare CONTENT: a file outside the
# adapted set may differ from upstream ONLY by the mechanical include rename.
VENDOR=${FC_VENDOR_SRC:-$SRCDIR/../../../../../../fakechroot/src}
if [ -d "$VENDOR" ]; then
  # Keep in step with nix-on-droid/docs/ANDROID-GLIBC.md "The adapted files".  wrapper.h is compared
  # against upstream's libfakechroot.h, its name before the rename.
  adapted_set=" __lxstat64.c __readlink_chk.c __readlinkat_chk.c bind.c chroot.c clearenv.c connect.c execve.c glob.c libfakechroot.c lstat.c lstat64.c mkdtemp.c mkostemp.c mkostemp64.c mkostemps.c mkostemps64.c mkstemp.c mkstemp64.c mkstemps.c mkstemps64.c mktemp.c readlink.c readlinkat.c realpath.c rel2abs.c rel2absat.c tmpnam.c wrapper.h "
  n_adapted=0; n_renameonly=0; n_bad=0
  for f in "$SRCDIR"/*.c "$SRCDIR"/*.h; do
    b=$(basename "$f")
    u=$VENDOR/$b
    [ "$b" = wrapper.h ] && u=$VENDOR/libfakechroot.h
    [ -e "$u" ] || continue                       # port-owned file, no counterpart
    cmp -s "$f" "$u" && continue                  # pristine
    case $adapted_set in *" $b "*) n_adapted=$((n_adapted+1)); continue;; esac
    # Not declared adapted: every differing line must be the include rename.
    stray=$(diff "$u" "$f" | grep -E '^[<>]' |
              grep -vE '^[<>][[:space:]]*#[[:space:]]*include[[:space:]]+"(libfakechroot|wrapper)\.h"' |
              grep -vE '^[<>].*\b(libfakechroot|wrapper)\.h\b')
    if [ -z "$stray" ]; then
      n_renameonly=$((n_renameonly+1))
    else
      n_bad=$((n_bad+1))
      note "  UNDECLARED EDIT: $b" "$(printf '%s' "$stray" | grep -c .) lines beyond the rename"
      printf '%s\n' "$stray" | head -4 | sed 's/^/      /'
    fi
  done
  if [ "$n_bad" -eq 0 ]; then
    note "vendor files" "$n_adapted adapted, $n_renameonly rename-only, 0 undeclared  ok"
  else
    note "vendor files" "$n_bad UNDECLARED"
    fail=1
  fi
else
  note "vendor files" "skipped (no $VENDOR)"
fi

echo "== 5. checked-in shims must equal android-shim-hand =="
# sysd-rules is included BEFORE Makerules' $(objpfx)%.o: $(objpfx)%.c rule, so a
# checked-in fc-<f>.c silently WINS over its generated twin.  A stale shim left
# behind after a function moves to the generated set is therefore invisible: the
# build is green and the wrong source is compiled.
# Range-based, so a BACKSLASH CONTINUATION is read too.  The single-line form
# silently saw only the first line: adding scandirat64 on a continuation made
# this report MISMATCH against a list that was in fact correct.
hand=$(sed -n '/^android-shim-hand *:=/,/[^\\]$/p' "$MK" |
         sed 's/#.*//; s/^android-shim-hand *:= *//' |
         tr -s ' \\\n' '\n' | grep -vE '^$' | sort)
tracked=$(cd "$SRCDIR/.." && ls fc-*.c 2>/dev/null | sed 's/^fc-//; s/\.c$//' | sort)
if [ "$hand" = "$tracked" ]; then
  note "checked-in fc-*.c == android-shim-hand" "$(printf '%s' "$hand" | grep -c .)  ok"
else
  note "checked-in fc-*.c == android-shim-hand" "MISMATCH"
  diff <(printf '%s\n' "$hand") <(printf '%s\n' "$tracked") |
    sed 's/^</  in android-shim-hand but no file: /; s/^>/  file present but not declared: /' |
    grep -E 'android-shim-hand|declared'
  fail=1
fi

echo "== 6. path-tables.h must match its source of truth =="
# path-tables.h is the whole translation POLICY -- which prefixes are local and
# which get rewritten -- pre-expanded because glibc cannot take the Boost.PP
# dependency fakechroot's own build uses.  Its header says "SOURCE OF TRUTH:
# common/pkgs/android-fakechroot.nix", and nothing checked that.  Drift here does
# not fail a build or move a symbol: it silently changes which paths are
# translated, which is the most consequential thing in the port and the least
# visible.  Order matters as well as membership -- the arrays are index-parallel
# with exclude_length/include_length.
NIXSRC=${FC_PATHS_NIX:-$SRCDIR/../../../../../../../common/pkgs/android-fakechroot.nix}
if [ -r "$NIXSRC" ]; then
  # From the .nix: colon-separated strings.  From the header: the quoted entries
  # between `<name>_list[] = {' and the closing brace.
  nixlist() { sed -n "s/.*$1 *= *\"\([^\"]*\)\".*/\1/p" "$NIXSRC" | head -1 | tr ':' '\n' | grep -vE '^$'; }
  hdrlist() { awk -v n="$1" '$0 ~ n"_list\\[\\] *= *\\{" {f=1; next} f && /^\};/ {exit}
                             f {while (match ($0, /"[^"]+"/))
                                  { print substr ($0, RSTART+1, RLENGTH-2)
                                    $0 = substr ($0, RSTART+RLENGTH) } }' "$SRCDIR/path-tables.h"; }
  pt_fail=0
  for pair in "excludePath|exclude" "includePath|include"; do
    IFS='|' read -r nixname hdrname <<<"$pair"
    a=$(nixlist "$nixname"); b=$(hdrlist "$hdrname")
    if [ "$a" = "$b" ]; then
      note "  ${hdrname}_list vs $nixname" "$(printf '%s\n' "$a" | grep -c .) entries  ok"
    else
      note "  ${hdrname}_list vs $nixname" "DRIFT"
      diff <(printf '%s\n' "$a") <(printf '%s\n' "$b") |
        sed 's/^</    only in android-fakechroot.nix: /; s/^>/    only in path-tables.h:          /' |
        grep -E 'only in'
      pt_fail=1
    fi
  done
  [ $pt_fail -eq 0 ] || fail=1
else
  note "path-tables.h vs android-fakechroot.nix" "skipped (no $NIXSRC)"
fi

echo "== 7. the counts quoted in prose must match the tables =="
# These went stale TWICE -- once when the shim set grew, and once when
# gen-shim.sh took over the .globl/.set cases and android-shim-hand shrank from
# 20 to 5 -- and each time the wrong numbers sat in the very files that explain
# the machinery.  A count is a claim; check it like any other.
cnt_wrappers=$(sed -n '/^android-wrappers-[a-z]* *:=/,/[^\\]$/p' "$MK" |
                 sed 's/#.*//; s/^android-wrappers-[a-z-]* *:= *//' |
                 tr -s ' \\\n' '\n' | grep -vE '^$' | sort -u | grep -c .)
cnt_support=$(sed -n '/^android-support-[a-z]* *:=/,/[^\\]$/p' "$MK" |
                sed 's/#.*//; s/^android-support-[a-z-]* *:= *//' |
                tr -s ' \\\n' '\n' | grep -vE '^$' | sort -u | grep -c .)
cnt_hand=$(printf '%s\n' "$hand" | grep -c .)
cnt_asm=$(grep -c '^android-asm-' "$MK")
cnt_objs=$(( cnt_wrappers + cnt_support ))
cnt_gen=$(( cnt_objs - cnt_hand ))
cnt_vendor=$(ls "$SRCDIR"/*.c 2>/dev/null | grep -c .)

c7_fail=0
# c7 <label> <derived-value> <file> <literal with @ standing for the value>
c7() {
  c7_want=${4//@/$2}
  if grep -qF "$c7_want" "$3"; then
    note "  $1" "$2  ok"
  else
    note "  $1" "$2  STALE in $(basename "$3")"
    echo "      no line contains: $c7_want"
    c7_fail=1
  fi
}
c7 "generated shims (Makefile)"  "$cnt_gen"    "$MK" "shims: @ generated"
c7 "checked-in shims (Makefile)" "$cnt_hand"   "$MK" "generated, @ checked in"
c7 "objects (Makefile)"          "$cnt_objs"   "$MK" "support objects = @"
c7 "vendor .c (Makefile)"        "$cnt_vendor" "$MK" "the module holds @"
c7 "generated shims (gen-shim)"  "$cnt_gen"    "$SRCDIR/gen-shim.sh" "# @ of the port's"
c7 "objects (gen-shim)"          "$cnt_objs"   "$SRCDIR/gen-shim.sh" "port's @ shims"

DOC=${FC_DOC:-$SRCDIR/../../../../../../../docs/ANDROID-GLIBC.md}
if [ -r "$DOC" ]; then
  c7 "generated shims (docs)" "$cnt_gen"    "$DOC" "@ of the 111 objects"
  c7 "vendor .c (docs)"       "$cnt_vendor" "$DOC" "module's @ libc-named files"
else
  note "  docs/ANDROID-GLIBC.md" "skipped (no $DOC)"
fi
[ $c7_fail -eq 0 ] || fail=1

echo
[ $fail -eq 0 ] && echo "ALL CHECKS PASSED" || echo "FAILURES PRESENT"
exit $fail
