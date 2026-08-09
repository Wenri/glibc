/* SIGSYS handler for Android's seccomp filter, installed from ld.so.
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

/* Android's seccomp filter answers a blocked syscall with SECCOMP_RET_TRAP,
   i.e. SIGSYS.  Without a handler that is fatal, so this glibc kills the
   process on any blocked syscall -- measured: acct(NULL) gives rc=159 with no
   output.  The deployed LD_PRELOAD libfakechroot survives only because it
   installs this handler from a library constructor.

   It lives in ld.so rather than libc because rtld runs before every ELF
   constructor and before __libc_start_main, so an application constructor that
   issues a blocked syscall is already covered, and because there is then a
   single install point with no "did the other one do it?" coordination.

   NOTE it does NOT cover static binaries, contrary to what nix-on-droid/docs/SECCOMP.md used
   to claim.  fakechroot's execve routes a PT_INTERP-less binary through ld.so
   "for sigaction setup", but rtld_chain_load (elf/rtld.c:1056) finds no
   DT_NEEDED and no PT_INTERP, discards the mapping and calls __rtld_execve --
   a real execve syscall, which resets every signal disposition.  Static
   coverage comes from a second copy of this file in libc.a instead.

   The handler body is a rewrite of android/sigaction.c:172-218
   rather than an include of it: that file's every path calls debug(), which is
   fakechroot_debug -> getenv + snprintf + vfprintf -> stdio -> gconv -> malloc,
   the cascade that breaks the librtld.map discovery link.  The pieces that ARE
   reused below are all static inline pure computation.  */

/* Tripwire, and it must precede every include.  Nothing here may reach
   wrapper.h; if a later edit pulls it in, its own object-like
   `#define debug fakechroot_debug' collides with this function-like one and
   -Werror turns a silent forty-object drag into a build failure.  */
#define debug(...) do { } while (0)

#include <atomic.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdbool.h>
#include <sysdep.h>
#include <sigsetops.h>
#include <dl-sigsys.h>

#ifdef __aarch64__

/* MUST precede <kernel_sigaction.h>.  That header grows its sa_restorer member
   only #ifdef SA_RESTORER, and aarch64/libc_sigaction.c:19 defines the macro
   before including it.  A translation unit that includes the header cold gets
   the three-field layout, putting sa_mask at offset 16 instead of 24 and
   handing the kernel garbage -- which rt_sigaction rejects with EINVAL, so the
   installer silently does nothing and the build is indistinguishable from one
   where this file was never added.  The _Static_assert below pins it.  */
# define SA_RESTORER 0x04000000
# include <kernel_sigaction.h>

_Static_assert (offsetof (struct kernel_sigaction, sa_mask)
		== 3 * sizeof (unsigned long int),
		"struct kernel_sigaction layout differs from "
		"sysdeps/unix/sysv/linux/aarch64/libc_sigaction.c");

/* Order matters.  Inside a glibc build <sys/syscall.h> is glibc's internal
   wrapper and defines only __NR_*; <disabled-syscall.h> restores the numbers
   process-fakesyscalls.sh moved out of <arch-syscall.h>, and only then does
   sys-syscall-compat.h supply the SYS_* spellings the imported headers test.
   Get this wrong and every `#ifdef SYS_x' below reads false SILENTLY.  */
# include <sys/syscall.h>
# include <disabled-syscall.h>
# include "android/sys-syscall-compat.h"

/* Reused unmodified.  Both files are static inline only and reach nothing
   beyond <sys/ucontext.h>, <sys/syscall.h>, <signal.h>, <string.h> and
   <stdbool.h> -- in particular neither reaches wrapper.h.  */
# include "android/android_syscalls.h"	/* is_noop_syscall,
							   is_blocked_syscall */
# include "android/sigaction.h"		/* sigsys_get_arg,
							   sigsys_set_return */

/* The guard for the trap above: if the SYS_* spellings did not resolve, every
   case below would vanish and the handler would chain everything to SIG_DFL,
   with nothing to show for it at build time.  */
# if !defined SYS_faccessat2 || !defined SYS_faccessat || !defined SYS_accept4
#  error "SYS_* spellings unavailable: <disabled-syscall.h> and/or \
sys-syscall-compat.h did not resolve from this translation unit, so every \
`case SYS_x' would silently disappear"
# endif

# ifndef SYS_SECCOMP
#  define SYS_SECCOMP 1
# endif

/* Chain state.  Ordinary .bss, so it stays writable after _dl_protect_relro
   seals RELRO at elf/rtld.c:2363 -- it must NOT be const, carry
   attribute_relro, or live in GLRO().  */
static bool sigsys_active;
static void *sigsys_saved_handler;
static int sigsys_saved_flags;

/* Hand expansion of the entries in REDIRECT_SEQ
   (android/syscall.h:351-363) whose syscall exists on aarch64.  The
   other nine -- chmod, chown, chown32, rmdir, recv, send, getpgrp, symlink,
   link -- have no __NR_ here and expand to nothing.

   Written out rather than expanded from the vendor macros because those need
   Boost.PP, and because SYS_GEN_* emits nextcall(syscall), which is libc-only.
   Faithful to SYS_GEN_AT1 (syscall.h:215-224) and SYS_GEN_FORWARD (:150-159)
   under the signal-handler context macros at sigaction.c:136-148, including
   that the handler does NO path expansion: CTX_EXPAND_PATH_AT is a plain cast
   there, which is what keeps rel2abs/getcwd_real/the path tables -- and with
   them the whole support layer -- out of ld.so.

   INTERNAL_SYSCALL_NCS_CALL, not the named form: NCS takes the number, so this
   keeps building if a target syscall later joins fakesyscall.json and loses its
   __NR_.  INTERNAL_, not INLINE_: under IS_IN (rtld) errno is the single
   non-TLS global rtld_errno, and a signal handler must not clobber what
   dl_main or dlopen left in it.

   Keep in step with REDIRECT_SEQ; verify-fc.sh cross-checks the two.  */
static bool
sigsys_redirect (ucontext_t *ctx, int nr)
{
  long int result;

  switch (nr)
    {
# ifdef SYS_faccessat2
    case SYS_faccessat2:
      /* faccessat2 (dirfd, path, mode, flags) -> faccessat (dirfd, path, mode).
	 The trailing zero is faithful to the vendor expansion; the
	 three-argument kernel entry ignores it.  */
      result = INTERNAL_SYSCALL_NCS_CALL (SYS_faccessat,
					  sigsys_get_arg (ctx, 0),
					  (const char *) sigsys_get_arg (ctx, 1),
					  sigsys_get_arg (ctx, 2), 0);
      break;
# endif

# if defined SYS_fchmodat2 && defined SYS_fchmodat
    case SYS_fchmodat2:
      /* fchmodat2 (dirfd, path, mode, flags) -> fchmodat (dirfd, path, mode).  */
      result = INTERNAL_SYSCALL_NCS_CALL (SYS_fchmodat,
					  sigsys_get_arg (ctx, 0),
					  (const char *) sigsys_get_arg (ctx, 1),
					  sigsys_get_arg (ctx, 2), 0);
      break;
# endif

# ifdef SYS_accept
    case SYS_accept:
      /* accept (fd, addr, addrlen) -> accept4 (fd, addr, addrlen, 0).  */
      result = INTERNAL_SYSCALL_NCS_CALL (SYS_accept4,
					  sigsys_get_arg (ctx, 0),
					  sigsys_get_arg (ctx, 1),
					  sigsys_get_arg (ctx, 2), 0);
      break;
# endif

    default:
      return false;
    }

  sigsys_set_return (ctx, result);
  return true;
}

/* Rewrite of android/sigaction.c:172-218.  */
static void
sigsys_handler (int sig, siginfo_t *info, void *ucontext)
{
  if (info->si_code != SYS_SECCOMP)
    goto chain;

  {
    ucontext_t *const ctx = (ucontext_t *) ucontext;
    const int nr = info->si_syscall;

    /* uid/gid calls cannot do anything on Android; report success.  */
    if (is_noop_syscall (nr))
      {
	sigsys_set_return (ctx, 0);
	return;
      }

    /* Everything else on the blocked list gets ENOSYS, which is what makes
       callers take their own fallback path.  */
    if (is_blocked_syscall (nr))
      {
	sigsys_set_return (ctx, -ENOSYS);
	return;
      }

    if (sigsys_redirect (ctx, nr))
      return;
  }

chain:
  /* Not ours: hand it to whatever the application registered, exactly as
     sigaction.c:202-217 does.  Note this swallows a genuine non-seccomp SIGSYS
     when the application left SIG_DFL -- inherited behaviour, not a
     regression.

     Read the handler FIRST and with acquire: _dl_sigsys_exchange publishes it
     last and with release, so observing a non-NULL handler here guarantees the
     matching flags are visible too.  Reading them in the other order lets a
     signal that lands mid-install pair a new handler with stale flags and call
     an sa_sigaction function through the sa_handler signature.  */
  {
    void *const h = atomic_load_acquire (&sigsys_saved_handler);

    /* SIG_DFL is NULL, so the NULL test covers it; SIG_IGN is (void *) 1 and
       does not.  Both arms need both tests -- the SA_SIGINFO arm used to check
       only NULL, so a handler installed as
       { .sa_handler = SIG_IGN, .sa_flags = SA_SIGINFO } jumped to address 1.  */
    if (h == NULL || h == (void *) SIG_IGN || h == (void *) SIG_DFL)
      return;

    if (atomic_load_relaxed (&sigsys_saved_flags) & SA_SIGINFO)
      ((void (*) (int, siginfo_t *, void *)) h) (sig, info, ucontext);
    else
      ((void (*) (int)) h) (sig);
  }
}

int
_dl_sigsys_exchange (int set, void **handler, int *flags)
{
  if (!sigsys_active)
    return 0;

  void *h = sigsys_saved_handler;
  int f = sigsys_saved_flags;

  if (set)
    {
      /* Publish in three steps so the delivery path above never observes a
	 non-NULL handler alongside flags that do not belong to it.  Retiring
	 the handler first opens a window in which a chained SIGSYS is dropped;
	 that is strictly better than calling one through the wrong signature,
	 and it is two instructions wide.  */
      atomic_store_release (&sigsys_saved_handler, NULL);
      sigsys_saved_flags = *flags;
      atomic_store_release (&sigsys_saved_handler, *handler);
    }

  *handler = h;
  *flags = f;
  return 1;
}

void
_dl_sigsys_install (void)
{
  struct kernel_sigaction kact, koact;

  kact.k_sa_handler = (__sighandler_t) sigsys_handler;
  /* SA_SIGINFO alone, deliberately.  nix-on-droid/docs/SECCOMP.md used to say delivery needs
     an SA_RESTORER trampoline, citing commit 1b08c2f846 -- but that commit
     touched only arm, i386 and x86_64, there is no aarch64 sigrestorer.S, and
     aarch64/libc_sigaction.c:21-25 propagates a restorer only when the caller
     already set the flag.  The deployed preload installs with SA_SIGINFO alone
     (sigaction.c:245) and demonstrably works on this device, because the
     kernel supplies __kernel_rt_sigreturn from the vDSO.  */
  kact.sa_flags = SA_SIGINFO;
  kact.sa_restorer = NULL;
  __sigemptyset (&kact.sa_mask);

  if (INTERNAL_SYSCALL_CALL (rt_sigaction, SIGSYS, &kact, &koact,
			     __NSIG_BYTES) == 0)
    {
      /* Seed the chain from whatever was there -- SIG_DFL after execve --
	 mirroring the &saved_sigsys_handler out-parameter at sigaction.c:248.
	 The kernel hands back its own struct, not the userspace one.

	 Flags before handler, handler with release: our handler is live in the
	 kernel from the moment the syscall above returned, so a SIGSYS can land
	 between these two stores.  Same discipline as _dl_sigsys_exchange.  */
      sigsys_saved_flags = (int) koact.sa_flags;
      atomic_store_release (&sigsys_saved_handler,
			    (void *) koact.k_sa_handler);
      sigsys_active = true;
    }
}

#else /* !__aarch64__ */

int
_dl_sigsys_exchange (int set, void **handler, int *flags)
{
  return 0;
}

void
_dl_sigsys_install (void)
{
}

#endif
