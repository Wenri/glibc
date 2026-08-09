/* Runtime test for the SIGSYS handler that ld.so installs (dl-sigsys.c).
 *
 * Why raw `svc #0' and not syscall().  glibc's own interposer
 * (sysdeps/unix/sysv/linux/syscall.c) substitutes a libc call for every number
 * whose __NR_ process-fakesyscalls.sh deleted, so syscall(SYS_faccessat2, ...)
 * would never reach the kernel and the handler would never be exercised.  Raw
 * is also the real scenario: Go and Rust issue syscalls without libc.
 *
 * Why the numbers are hardcoded.  This test must not agree with the libc it is
 * testing.  <sys/syscall.h> from the fork is missing exactly the SYS_* the test
 * cares about -- that is the mechanism under test -- so taking them from a
 * header would silently turn the interesting probes into no-ops.  aarch64 only.
 *
 *   gcc -O0 -g -o sigsys-test sigsys-test.c
 *   <fc>/lib/ld-linux-aarch64.so.1 --library-path <fc>/lib ./sigsys-test
 *
 * and, for the libc.a copy of the handler, which nothing else in the suite
 * exercises.  The -B directory is needed because the crt objects and libc.a
 * live in DIFFERENT outputs of the same derivation, and gcc will otherwise
 * happily link the ambient libc.a and report a pass that means nothing:
 *
 *   mkdir B && ln -s <fc>/lib/crt[1in].o <fc-static>/lib/libc.a B/
 *   gcc -O0 -g -static -BB -o sigsys-test-static sigsys-test.c
 *   ./sigsys-test-static                       # no loader at all
 *
 * Confirm with `nm sigsys-test-static | grep sigsys' that _dl_sigsys_install
 * really came from libc-dl-sigsys.o.
 *
 * Run the dynamic build under OUR loader.  Under the system loader the ambient
 * /etc/ld.so.preload libfakechroot installs its own handler first and every
 * probe below passes for the wrong reason.
 *
 * Each probe runs in a forked child, so a probe that dies of SIGSYS is reported
 * as such instead of ending the run -- which is the whole point, since "dies of
 * SIGSYS" is precisely the before state this is measuring against.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifndef __aarch64__
int main (void) { puts ("sigsys-test: aarch64 only, skipping"); return 0; }
#else

/* Upstream aarch64 numbers.  Those marked (deleted) have had their __NR_
   removed from the fork's <arch-syscall.h> by process-fakesyscalls.sh and exist
   at runtime only through the SIGSYS handler.  */
#define NR_faccessat2   439     /* (deleted) redirected to faccessat  */
#define NR_faccessat     48
#define NR_setuid       146     /* (deleted) no-op category           */
#define NR_getuid       174
#define NR_acct          89     /* in no category: reaches the chain  */
#define NR_keyctl       219     /* blocked category                   */
#define NR_add_key      217     /* blocked category                   */
#define NR_clone3       435     /* (deleted) blocked category         */
#define NR_landlock     444     /* blocked category                   */

/* The six fakechroot blocks but the fork emulates properly, which fc-syscall.c
   exempts from is_blocked_syscall on the syscall() path.  Verified against
   sysdeps/unix/sysv/linux/aarch64/arch-syscall.h.  */
#define NR_shmget       194     /* (deleted) android_shmem: ashmem + mmap  */
#define NR_shmctl       195
#define NR_shmat        196
#define NR_shmdt        197
#define NR_close_range  436     /* (deleted) io/close_range.c emulation    */
#define NR_epoll_pwait2 441     /* (deleted) fake_epoll_pwait2 -> epoll_pwait */

static long
raw (long nr, long a0, long a1, long a2, long a3, long a4)
{
  register long x8 asm ("x8") = nr;
  register long x0 asm ("x0") = a0;
  register long x1 asm ("x1") = a1;
  register long x2 asm ("x2") = a2;
  register long x3 asm ("x3") = a3;
  register long x4 asm ("x4") = a4;
  asm volatile ("svc #0"
		: "+r" (x0)
		: "r" (x1), "r" (x2), "r" (x3), "r" (x4), "r" (x8)
		: "memory", "cc");
  return x0;
}

/* ------------------------------------------------------------------ */

static int failures;
static int probes;

/* _exit does not flush stdio, so a child that reports through printf and then
   _exits prints nothing -- which reads exactly like a probe that produced no
   diagnostic.  Every child exit goes through here.  */
static void __attribute__ ((noreturn))
die (int code)
{
  fflush (NULL);
  _exit (code);
}

/* Run the body in a child.  It reports its result by exiting with a small code;
   anything the parent cannot express that way it prints itself.  Variadic
   because a braced body has top-level commas of its own.  The flush before the
   fork is the other half of the same problem: without it the child inherits the
   parent's pending buffer and duplicates it.  */
#define CHILD(name, ...)						\
  do {									\
    fflush (NULL);							\
    pid_t _p = fork ();							\
    if (_p == 0) { __VA_ARGS__; die (0); }				\
    int _st; waitpid (_p, &_st, 0);					\
    ++probes;								\
    if (WIFSIGNALED (_st))						\
      {									\
	printf ("  %-46s KILLED by %s\n", name,				\
		strsignal (WTERMSIG (_st)));				\
	++failures;							\
      }									\
    else if (WEXITSTATUS (_st) != 0)					\
      {									\
	printf ("  %-46s FAIL (exit %d)\n", name, WEXITSTATUS (_st));	\
	++failures;							\
      }									\
    else								\
      printf ("  %-46s ok\n", name);					\
  } while (0)

/* Inside a child: compare and report.  Never assert on a measured-only probe --
   use SHOW for those.  */
#define WANT(what, got, expected)					\
  do {									\
    long _g = (got), _e = (expected);					\
    if (_g != _e)							\
      {									\
	printf ("      %s: got %ld, wanted %ld\n", what, _g, _e);	\
	die (1);							\
      }									\
  } while (0)

#define SHOW(what, got)   printf ("      %-24s = %ld\n", what, (long) (got))

/* ------------------------------------------------------------------ */

static volatile sig_atomic_t app_handler_ran;

static void
app_sigsys (int sig, siginfo_t *info, void *ctx)
{
  (void) sig; (void) info; (void) ctx;
  app_handler_ran = 1;
}

int
main (void)
{
  puts ("\n== redirect category: faccessat2 -> faccessat ==");

  CHILD ("faccessat2(AT_FDCWD, \"/\", F_OK, 0) == 0",
	 WANT ("rc", raw (NR_faccessat2, -100, (long) "/", 0, 0, 0), 0));

  /* The strongest single check in the suite.  -ENOENT cannot be manufactured by
     a blanket -ENOSYS or by a handler that merely returns: only an actually
     executed faccessat produces it.  So this distinguishes a real redirect from
     "something caught the signal".  */
  CHILD ("faccessat2 on a missing path == -ENOENT",
	 WANT ("rc", raw (NR_faccessat2, -100, (long) "/no-such-xyzzy", 0, 0, 0),
	       -ENOENT));

  /* Delivery must work more than once.  This is the empirical answer to the
     SA_RESTORER question: aarch64 has no sigrestorer.S and the handler is
     installed with SA_SIGINFO alone, relying on the kernel's vDSO
     __kernel_rt_sigreturn.  If that were wrong, the second return would not
     happen and this child would die rather than fail.  */
  CHILD ("50 consecutive traps all return (sigreturn)",
	 {
	   for (int i = 0; i < 50; i++)
	     WANT ("rc", raw (NR_faccessat2, -100, (long) "/", 0, 0, 0), 0);
	 });

  puts ("\n== blocked category: expect -ENOSYS ==");

  CHILD ("keyctl -> -ENOSYS",
	 WANT ("rc", raw (NR_keyctl, 0, 0, 0, 0, 0), -ENOSYS));
  CHILD ("add_key -> -ENOSYS",
	 WANT ("rc", raw (NR_add_key, 0, 0, 0, 0, 0), -ENOSYS));
  CHILD ("landlock_create_ruleset -> -ENOSYS",
	 WANT ("rc", raw (NR_landlock, 0, 0, 0, 0, 0), -ENOSYS));
  CHILD ("clone3 -> -ENOSYS",
	 WANT ("rc", raw (NR_clone3, 0, 0, 0, 0, 0), -ENOSYS));

  puts ("\n== no-op category: uid/gid calls report success, change nothing ==");

  /* If this syscall were NOT trapped the real call would return -EPERM, so
     "0 and still not root" is what separates the emulation from the kernel. */
  CHILD ("setuid(0) -> 0 and still not root",
	 {
	   long rc = raw (NR_setuid, 0, 0, 0, 0, 0);
	   long uid = raw (NR_getuid, 0, 0, 0, 0, 0);
	   SHOW ("setuid(0)", rc);
	   SHOW ("getuid() after", uid);
	   if (rc != 0) die (1);
	   if (uid == 0) die (2);       /* it actually worked -- wrong */
	 });

  puts ("\n== uncategorised: reaches the chain, app has SIG_DFL ==");

  /* The coarse deployment gate: acct(NULL) went from rc=159 (killed by SIGSYS)
     to returning.  But it proves less than it looks, and this is the experiment
     that shows why.  acct is in no category, so it falls through to the chain
     path, finds SIG_DFL and returns having written nothing to x0 -- and on
     aarch64 x0 is both the first argument register and the return register.  So
     the "return value" is just whatever was passed as argument 0.
     acct(NULL) == 0 is not a success indication; it is the NULL coming back.
     That also explains the reference glibc-plus-preload's 0, which the port
     notes had recorded as unexplained.  */
  CHILD ("uncategorised syscall returns its own first argument",
	 {
	   SHOW ("acct(NULL)", raw (NR_acct, 0, 0, 0, 0, 0));
	   SHOW ("acct((char *) 0x1234)", raw (NR_acct, 0x1234, 0, 0, 0, 0));
	   WANT ("acct(NULL)", raw (NR_acct, 0, 0, 0, 0, 0), 0);
	   WANT ("acct(0x1234)", raw (NR_acct, 0x1234, 0, 0, 0, 0), 0x1234);
	 });

  puts ("\n== the syscall() path: answered in userspace, never trapped ==");

  /* Everything above went through raw svc, which is the Go/Rust case and the
     only thing the SIGSYS handler can serve.  Code that calls syscall() is
     caught two layers earlier: fakechroot's wrapper, then the fork's
     DISABLED_SYSCALL_WITH_FAKESYSCALL substitution, then the kernel.

     For the 22 blocked syscalls the fork does NOT substitute -- keyctl here --
     this is the whole point.  Before syscall() was wired they reached the
     kernel, Android's filter trapped them, a signal was delivered, the handler
     wrote -ENOSYS and sigreturned.  Now the answer comes from userspace with
     no trap at all.  Same result, one less round trip.  */
  CHILD ("syscall(keyctl) -> -1/ENOSYS, no trap",
	 {
	   errno = 0;
	   long rc = syscall (NR_keyctl, 0, 0, 0, 0, 0);
	   WANT ("rc", rc, -1);
	   WANT ("errno", errno, ENOSYS);
	 });

  CHILD ("syscall(faccessat2) on a missing path -> -1/ENOENT",
	 {
	   errno = 0;
	   long rc = syscall (NR_faccessat2, AT_FDCWD, "/no-such-xyzzy", 0, 0);
	   WANT ("rc", rc, -1);
	   WANT ("errno", errno, ENOENT);
	 });

  CHILD ("syscall(setuid, 0) -> 0 and still not root",
	 {
	   WANT ("rc", syscall (NR_setuid, 0), 0);
	   if (getuid () == 0) die (2);
	 });

  puts ("\n== ...except the six the fork emulates properly ==");
  puts ("   (fakechroot blocks these; blocking them would replace working");
  puts ("    code with a stub, so fc-syscall.c narrows the predicate)");

  /* android_shmem: real shared memory over ashmem and mmap.  If the exemption
     were dropped, shmget would come back -1/ENOSYS instead of an id.  */
  CHILD ("shm{get,at,dt,ctl} round-trip through syscall()",
	 {
	   long id = syscall (NR_shmget, IPC_PRIVATE, 4096, IPC_CREAT | 0600);
	   SHOW ("shmget", id);
	   if (id < 0) { perror ("      shmget"); die (1); }

	   long addr = syscall (NR_shmat, id, 0, 0);
	   SHOW ("shmat", addr != -1);
	   if (addr == -1) { perror ("      shmat"); die (2); }

	   /* Actually use it -- an id that cannot be written to is not shared
	      memory, and a stub could still have handed one back.  */
	   *(volatile int *) addr = 0x5eed;
	   if (*(volatile int *) addr != 0x5eed) die (3);

	   if (syscall (NR_shmdt, addr) != 0) { perror ("      shmdt"); die (4); }
	   syscall (NR_shmctl, id, IPC_RMID, 0);
	 });

  /* io/close_range.c's emulation over __getdtablesize and
     __close_nocancel_nostatus.  Reached precisely BECAUSE __NR_close_range was
     deleted; fakechroot's answer would be ENOSYS.  */
  CHILD ("syscall(close_range) on an empty range -> 0",
	 {
	   errno = 0;
	   WANT ("rc", syscall (NR_close_range, 900, 950, 0), 0);
	 });

  /* fake_epoll_pwait2 converts the timespec to milliseconds and calls
     epoll_pwait.  Zero timeout on an empty set must return 0, not -ENOSYS.  */
  CHILD ("syscall(epoll_pwait2) with a 0 timeout -> 0",
	 {
	   int epfd = epoll_create1 (0);
	   if (epfd < 0) { perror ("      epoll_create1"); die (1); }
	   struct epoll_event ev[1];
	   struct timespec ts = { 0, 0 };
	   errno = 0;
	   long rc = syscall (NR_epoll_pwait2, epfd, ev, 1, &ts, NULL, 8);
	   SHOW ("epoll_pwait2", rc);
	   close (epfd);
	   WANT ("rc", rc, 0);
	 });

  puts ("\n== libc functions whose own __NR_ was deleted ==");
  puts ("   (these route through syscall() and depend on the substitution");
  puts ("    table; they were ENOSYS while that table was empty)");

  /* THE COVERAGE GAP THAT LET THIS HIDE.  Every other probe here reaches the
     substitution through syscall() directly.  These two are the only public
     libc functions the __NR_ deletions broke unintentionally, and nothing
     called them AS FUNCTIONS -- so the whole class was invisible.

     Mechanism, since the port notes had it wrong: <fakesyscall.h> re-includes
     <disabled-syscall.h>, which puts __NR_epoll_pwait2 back inside the
     translation unit.  epoll_pwait2.c therefore compiles and issues
     syscall (__NR_epoll_pwait2, ...) like any other caller; the substitution
     table is what turns that into fake_epoll_pwait2.  There is no compile-time
     fallback and none is needed.  */
  CHILD ("epoll_pwait2() the libc function",
	 {
	   int epfd = epoll_create1 (0);
	   if (epfd < 0) { perror ("      epoll_create1"); die (1); }
	   struct epoll_event ev[1];
	   struct timespec ts = { 0, 0 };
	   errno = 0;
	   int rc = epoll_pwait2 (epfd, ev, 1, &ts, NULL);
	   SHOW ("epoll_pwait2()", rc);
	   close (epfd);
	   if (rc != 0) { printf ("      errno=%d (%s)\n", errno,
				  strerror (errno)); die (2); }
	 });

  CHILD ("statx() the libc function",
	 {
	   struct statx stx;
	   errno = 0;
	   int rc = statx (AT_FDCWD, "/", 0, STATX_BASIC_STATS, &stx);
	   SHOW ("statx(\"/\")", rc);
	   if (rc != 0) { printf ("      errno=%d (%s)\n", errno,
				  strerror (errno)); die (2); }
	   if (!(stx.stx_mask & STATX_MODE)) die (3);
	 });

  puts ("\n== chaining: an application's own SIGSYS handler ==");
  puts ("   (this probe is what the __libc_sigaction hook buys; without the");
  puts ("    hook it fails, and faccessat2 comes back as its own argument)");

  CHILD ("app handler runs, and does not displace ours",
	 {
	   struct sigaction sa, old;
	   memset (&sa, 0, sizeof sa);
	   sa.sa_sigaction = app_sigsys;
	   sa.sa_flags = SA_SIGINFO;
	   sigemptyset (&sa.sa_mask);
	   if (sigaction (SIGSYS, &sa, NULL) != 0)
	     { perror ("      sigaction"); die (1); }

	   /* A raised SIGSYS has si_code != SYS_SECCOMP, so ld.so's handler
	      must chain to this one.  */
	   raise (SIGSYS);
	   if (!app_handler_ran)
	     { puts ("      app handler did not run"); die (2); }

	   /* ...and ld.so's handler must still be the installed one.  This is
	      the Go-safety property: an application that registers a SIGSYS
	      handler must not turn every blocked syscall back into a kill.  */
	   WANT ("rc after app handler installed",
		 raw (NR_faccessat2, -100, (long) "/no-such-xyzzy", 0, 0, 0),
		 -ENOENT);

	   /* And ld.so's handler address must never leak to the application. */
	   if (sigaction (SIGSYS, NULL, &old) != 0)
	     { perror ("      sigaction query"); die (3); }
	   if (old.sa_sigaction != app_sigsys)
	     {
	       printf ("      queried handler is %p, wanted %p\n",
		       (void *) old.sa_sigaction, (void *) app_sigsys);
	       die (4);
	     }
	 });

  printf ("\n%d/%d probes ok\n", probes - failures, probes);
  return failures != 0;
}

#endif /* __aarch64__ */
