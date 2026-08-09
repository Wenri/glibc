/* android wrapper shim for execve.  Why shims exist and every
   wiring precondition: nix-on-droid/docs/ANDROID-GLIBC.md.

   This is the `plain' variant.  Precondition 8 is clean -- there is no
   libc_hidden_proto for execve or __execve (include/unistd.h:115 declares
   __execve with plain attribute_hidden), so no asm remedy is needed -- and the
   reference exports execve WEAK, which is what the glue emits by default, so no
   FC_PUBLIC_STRONG either.

   Wiring this is what turns exec-family path translation ON.  execv, execl,
   execle, execlp and execvp are already wired but expand nothing themselves
   (android/execv.c:35 and friends just marshal arguments and call
   the public execve), and /nix does not exist at the real root -- measured
   under our loader, a raw newfstatat of "/nix" returns -ENOENT.  So all five
   have been handing untranslated store paths to the kernel and failing.
   smoke-test did not catch it because it only checks for signals, not
   success.  */

/* config.h FIRST -- it pulls <disabled-syscall.h> and sys-syscall-compat.h,
   and it is where ANDROID_ELFLOADER and ANDROID_ARGV0_OPT are derived.  Then
   wrapper.h, so that its extern declarations at :301-302 are parsed
   BEFORE the #define below rewrites the name.  */
#include <config.h>
#include "android/wrapper.h"

/* Adaptation 1: make execve honour envp exactly, as POSIX says.

   exec_preserve_env (execve.c:218-278) injects the preserve_env_list variables
   from the CALLING process's environment into the child whenever envp lacks
   them -- ANDROID_DEBUG and LD_LIBRARY_PATH (libfakechroot.c:87-96), and
   upstream additionally FAKEROOTKEY, FAKED_MODE and LD_PRELOAD.  Under
   LD_PRELOAD that was load-bearing -- it is how libfakechroot survived an exec.
   Compiled into libc it is unnecessary, since the child's own libc carries the
   translation layer, and it silently defeats `env -i', sudo-style environment
   scrubbing and nix build isolation by handing LD_LIBRARY_PATH back to a child
   that was denied it.

   Zeroing the count collapses exec_preserve_env to "copy envp's pointers and
   NUL-terminate", which is exactly right: the running total only accumulates
   inside the preserved-variable loop (execve.c:253), so it stays 0, envbuf[1]
   is never written, and newenvp[envc + 0 + 1] holds envc pointers plus the
   terminator.  Scoped to this TU, so clearenv() keeps its own preservation.  */
#define preserve_env_list_count 0

/* Adaptation 2: namespace the three cross-TU helpers.

   exec_preserve_env, exec_prepare and exec_build_argv (execve.c:218, :286,
   :520) are LOCAL, which is visibility("hidden") (wrapper.h:42) -- hidden
   but NOT static.  Hidden visibility governs dynamic export, not static-link
   resolution, so in libc.a these are ordinary global symbols with generic
   names that a user program can collide with.  They are non-static only
   because android/posix_spawn.c calls them across TU boundaries,
   and posix_spawn is not wired.

   Hygiene, not a new hazard: rel2abs, rel2absat, dedotdot, getcwd_real and
   lstat_rel are already GLOBAL HIDDEN in libc.a.  Namespacing those is a
   separate sweep -- wrapper.h's inline expand_chroot_path calls rel2abs
   from all 111 TUs, so it needs one force-included header, not per-shim
   defines.  */
#define exec_prepare       __android_exec_prepare
#define exec_preserve_env  __android_exec_preserve_env
#define exec_build_argv    __android_exec_build_argv

#include "android/execve.c"

/* Assert the vendor body was actually compiled (precondition 6): if a guard
   ever appears above wrapper(), this becomes a compile error rather than an
   empty object.  */
extern __typeof (__fc_execve) __fc_execve;
