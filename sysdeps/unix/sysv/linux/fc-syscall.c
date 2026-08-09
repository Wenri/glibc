/* android wrapper shim for syscall.  Why shims exist and every
   wiring precondition: nix-on-droid/docs/ANDROID-GLIBC.md.  */

/* glibc defines `syscall' bare and the reference exports it GLOBAL, so the
   public alias must be strong.  */
#define FC_PUBLIC_STRONG 1

/* config.h FIRST: it is what pulls <disabled-syscall.h> and
   sys-syscall-compat.h, without which every `#ifdef SYS_x' below and in
   android_syscalls.h reads false and the tables silently empty out.  Both
   headers are guarded, so syscall.c re-including them is a no-op.  */
#include <config.h>
#include "android/wrapper.h"
#include "android/android_syscalls.h"

#include <signal.h>
#include <unistd.h>

/* --- Adaptation 0: rtld sets the process name now ------------------------

   syscall.c also carries fakechroot_set_process_name, a CONSTRUCTOR that
   readlinks /proc/self/exe, strstr's for "ld-linux", then reads
   /proc/self/cmdline to recover argv[0] and prctl's comm.  As an ELF
   constructor in libc.so that would run in EVERY process -- paying a readlink
   to discover what rtld already knows -- so elf/rtld.c does it directly from
   _dl_argv[0] instead (sysdeps/unix/sysv/linux/dl-progname.h).

   Neutralise the attribute rather than deleting the function, so syscall.c
   stays byte-for-byte vendor.  It becomes dead code the compiler drops; it is
   still PARSED, which is why the declaration below is needed -- nextcall(open)
   resolves via nextcall-overrides.h to __libc_open, which glibc declares, but
   nextcall(readlink) does not, and only wrapper() declares its own.  */
#undef CONSTRUCTOR
#define CONSTRUCTOR __attribute__ ((unused))
extern ssize_t __android_next_readlink (const char *__path, char *__buf,
					   size_t __len);

/* --- Adaptation 1: keep the six syscalls the fork emulates properly -------

   fakechroot's is_blocked_syscall answers ENOSYS for 53 syscalls.  For 47 of
   them that is either identical to what the fork does or strictly faster:

     22  the fork does NOT substitute (their __NR_ survives), so today
         syscall(SYS_keyctl, ...) reaches the kernel, Android's filter traps
         it, a SIGSYS is delivered, the handler writes -ENOSYS and sigreturns.
         Answering in userspace here is the same result without the round trip.
     25  the fork substitutes INLINE_SYSCALL_ERROR_RETURN_VALUE (ENOSYS).
         Same answer, both in userspace.  No change.

   The remaining six the fork substitutes with something that WORKS, so
   blocking them would replace working code with a stub -- the same reasoning
   that put close_range on nix-on-droid/docs/ANDROID-GLIBC.md's never-wire list:

     close_range   io/close_range.c, a userspace emulation over
                   __getdtablesize and __close_nocancel_nostatus
     epoll_pwait2  fake_epoll_pwait2, which converts the timespec to
                   milliseconds and calls epoll_pwait.  The exemption matters
                   twice over here: the LIBC function epoll_pwait2() reaches
                   the substitution through this very syscall() wrapper, so
                   blocking the number would break the function too, not just
                   raw callers.  (An earlier note claimed that function was
                   ENOSYS because its __NR_ is deleted and glibc has no
                   fallback.  Both halves were wrong -- fakesyscall.h restores
                   the __NR_ inside the TU, and it was ENOSYS only while the
                   substitution table was empty.)
     shm{at,ctl,dt,get}
                   Termux's android_shmem: real shared memory over ashmem and
                   mmap, not a stub

   Narrow the vendor predicate rather than editing it.  The inner name is not
   re-expanded inside its own replacement list, so this resolves to the vendor
   static inline and adds a term -- no vendor file changes, and the guard lives
   in a glibc source exactly as nix-on-droid/docs/ANDROID-GLIBC.md requires.

   Deliberately NOT mirrored in dl-sigsys.c.  There the syscall has already
   been issued and trapped, and a signal handler cannot run these emulations;
   -ENOSYS is the only sane answer on that path.  */
static inline int
fc_glibc_emulates (long int number)
{
  switch (number)
    {
#ifdef SYS_close_range
    case SYS_close_range:
#endif
#ifdef SYS_epoll_pwait2
    case SYS_epoll_pwait2:
#endif
#ifdef SYS_shmat
    case SYS_shmat:
#endif
#ifdef SYS_shmctl
    case SYS_shmctl:
#endif
#ifdef SYS_shmdt
    case SYS_shmdt:
#endif
#ifdef SYS_shmget
    case SYS_shmget:
#endif
      return 1;
    default:
      return 0;
    }
}

#define is_blocked_syscall(n) \
  (is_blocked_syscall (n) && !fc_glibc_emulates (n))

/* --- Adaptation 2: SIGSYS chain state lives in ld.so now -----------------

   The vendor's handle_sigsys_sigaction reads and writes `saved_sigsys_handler',
   a struct sigaction defined in sigaction.c -- which is never wired, because
   ld.so owns the handler and its chain state (dl-sigsys.c).  Keeping a second
   copy of that state here would be two sources of truth that disagree.

   __libc_sigaction is exactly the right target and needs no new code: it
   already offers the disposition to ld.so via _dl_sigsys_exchange and falls
   back to a real rt_sigaction when no handler is installed.  Routing through
   it also means the syscall() path and the sigaction() path share ONE
   chokepoint, so they cannot drift apart.

   Defined after the header, so the vendor's static inline is parsed intact and
   only the CALL site is redirected; the inline is then unused, and
   `saved_sigsys_handler' stays an extern declaration that nothing references.

   THE STRUCTS ARE NOT THE SAME SHAPE, and the vendor gets this wrong.  Its
   syscall.c declares the two pointers `struct sigaction *', but a raw
   syscall(SYS_rt_sigaction, ...) caller passes the KERNEL's layout --
   handler(0), flags(8), restorer(16), mask(24), 32 bytes to the kernel -- not
   glibc's userspace one, where sa_mask sits at offset 8 and sa_flags at 136 and
   the whole object is 152 bytes.  Handing that pointer to __libc_sigaction
   makes it read sa_flags 104 bytes past the end of a 32-byte object, and the
   absorb path's `memset (oact, 0, sizeof *oact)' writes 152 bytes over it: a
   120-byte out-of-bounds write into the caller's stack.  Every runtime that
   installs its SIGSYS handler with a raw syscall -- which is precisely the
   audience this handler exists for -- hits it.

   So marshal.  Keeping __libc_sigaction as the target preserves the one
   chokepoint the paragraph above is about, including its fall-back to a real
   rt_sigaction when no handler is installed; only the ABI is translated.  */
# define SA_RESTORER 0x04000000  /* see dl-sigsys.c:59-66 -- MUST precede */
# include <kernel_sigaction.h>
# include <sigsetops.h>
# include <stddef.h>
# include <string.h>

_Static_assert (offsetof (struct kernel_sigaction, sa_mask)
		== 3 * sizeof (unsigned long int),
		"struct kernel_sigaction layout differs from "
		"sysdeps/unix/sysv/linux/aarch64/libc_sigaction.c");

static int
fc_sigsys_rt_sigaction (const struct kernel_sigaction *kact,
			struct kernel_sigaction *koact, size_t sigsetsize)
{
  struct sigaction act, oact;
  int result;

  /* The kernel validates this first; match it, or a caller passing the wrong
     size gets our emulation where it should have got EINVAL.  */
  if (sigsetsize != __NSIG_BYTES)
    {
      __set_errno (EINVAL);
      return -1;
    }

  if (kact != NULL)
    {
      memset (&act, 0, sizeof act);
      /* sa_handler and sa_sigaction are a union, so this carries an
	 SA_SIGINFO handler just as well.  */
      act.sa_handler = kact->k_sa_handler;
      act.sa_flags = kact->sa_flags;
      memcpy (&act.sa_mask, &kact->sa_mask, sigsetsize);
    }

  result = __libc_sigaction (SIGSYS, kact != NULL ? &act : NULL,
			     koact != NULL ? &oact : NULL);

  /* Write back within the kernel object's bounds -- never sizeof (oact).  */
  if (result == 0 && koact != NULL)
    {
      koact->k_sa_handler = oact.sa_handler;
      koact->sa_flags = oact.sa_flags;
      koact->sa_restorer = NULL;
      memcpy (&koact->sa_mask, &oact.sa_mask, sigsetsize);
    }
  return result;
}

/* SIGSETSIZE comes from the call site's own va_arg -- the vendor's macro takes
   only two arguments and this is its single expansion.  */
#define handle_sigsys_sigaction(act, oldact) \
  fc_sigsys_rt_sigaction ((const struct kernel_sigaction *) (act), \
			  (struct kernel_sigaction *) (oldact), sigsetsize)

#include "android/syscall.c"

/* Assert the vendor HAVE_ guard was satisfied (precondition 6): an
   unsatisfied guard makes this a compile error, not an empty object.  */
extern __typeof (__fc_syscall) __fc_syscall;
