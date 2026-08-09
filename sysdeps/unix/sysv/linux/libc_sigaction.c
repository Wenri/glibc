/* Copyright (C) 1997-2025 Free Software Foundation, Inc.
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
#include <signal.h>
#include <string.h>

#include <sysdep.h>
#include <sys/syscall.h>
#include <dl-sigsys.h>

/* ld.so owns SIGSYS on Android -- see sysdeps/unix/sysv/linux/dl-sigsys.c.  The
   reference is WEAK on purpose: it is undefined in libc.a (a static binary has
   no ld.so, and gets its own copy of the handler from csu/libc-start.c), and a
   libc.so.6 running under a mismatched loader must degrade to "no interception"
   rather than fail to resolve.  */
weak_extern (_dl_sigsys_exchange)

/* New ports should not define the obsolete SA_RESTORER, however some
   architecture requires for compat mode and/or due old ABI.  */
#include <kernel_sigaction.h>

#ifndef SA_RESTORER
# define SET_SA_RESTORER(kact, act)
# define RESET_SA_RESTORER(act, kact)
#endif

/* SPARC passes the restore function as an argument to rt_sigaction.  */
#ifndef STUB
# define STUB(act, sigsetsize) (sigsetsize)
#endif

/* If ACT is not NULL, change the action for SIG to *ACT.
   If OACT is not NULL, put the old action for SIG in *OACT.  */
int
__libc_sigaction (int sig, const struct sigaction *act, struct sigaction *oact)
{
  int result;

  struct kernel_sigaction kact, koact;

  /* Hand a SIGSYS disposition to ld.so instead of installing it.  ld.so's
     handler stays the registered one and chains to whatever is recorded here,
     so an application that installs its own SIGSYS handler -- Go does -- does
     not turn every seccomp-blocked syscall back into a kill.  Measured before
     this hook: after the application called sigaction, a trapped faccessat2
     came back as its own first argument instead of the redirected result.

     This is deliberately __libc_sigaction and not __sigaction.  It is the sole
     userspace chokepoint: signal(), sigset() and sigvec() all reach it through
     __sigaction (sysdeps/posix/signal.c:45), so hooking the outer name alone
     would let signal (SIGSYS, SIG_DFL) disarm us.  Note this is strictly more
     coverage than the LD_PRELOAD build had, where those three escaped entirely.

     What crosses to ld.so is two machine words, never a struct: ld.so speaks
     struct kernel_sigaction, which differs from this one in field order, size
     and arch-conditional membership.  One pointer covers both sa_handler and
     sa_sigaction because they are a union, with SA_SIGINFO selecting.  sa_mask
     is dropped, which changes nothing observable -- ld.so's handler is the
     installed one either way, so a recorded mask would never reach the kernel,
     and the chain call runs under ld.so's mask exactly as the LD_PRELOAD
     version's did.  */
  if (__glibc_unlikely (sig == SIGSYS) && &_dl_sigsys_exchange != NULL)
    {
      void *handler = NULL;
      int flags = 0;

      if (act != NULL)
	{
	  flags = act->sa_flags;
	  handler = (flags & SA_SIGINFO
		     ? (void *) act->sa_sigaction : (void *) act->sa_handler);
	}

      if (_dl_sigsys_exchange (act != NULL, &handler, &flags))
	{
	  if (oact != NULL)
	    {
	      memset (oact, 0, sizeof *oact);
	      oact->sa_flags = flags;
	      if (flags & SA_SIGINFO)
		oact->sa_sigaction = handler;
	      else
		oact->sa_handler = handler;
	    }
	  return 0;
	}
    }

  if (act)
    {
      kact.k_sa_handler = act->sa_handler;
      memcpy (&kact.sa_mask, &act->sa_mask, sizeof (sigset_t));
      kact.sa_flags = (unsigned int) act->sa_flags;
      SET_SA_RESTORER (&kact, act);
    }

  /* XXX The size argument hopefully will have to be changed to the
     real size of the user-level sigset_t.  */
  result = INLINE_SYSCALL_CALL (rt_sigaction, sig,
				act ? &kact : NULL,
				oact ? &koact : NULL, STUB (act,
							    __NSIG_BYTES));

  if (oact && result >= 0)
    {
      oact->sa_handler = koact.k_sa_handler;
      memcpy (&oact->sa_mask, &koact.sa_mask, sizeof (sigset_t));
      oact->sa_flags = koact.sa_flags;
      RESET_SA_RESTORER (oact, &koact);
    }
  return result;
}
libc_hidden_def (__libc_sigaction)
