/* Regression probes for the memory-safety defects found proofreading the
 * imported fakechroot sources against upstream.  Each one FAILED before its
 * fix; none is hypothetical.
 *
 *   gcc -O1 -g -fstack-protector-all -o safety-test safety-test.c
 *   <fc>/lib/ld-linux-aarch64.so.1 --library-path <fc>/lib ./safety-test
 *
 * Run it via OUR loader -- see behave-test.c for why the ambient
 * /etc/ld.so.preload otherwise intercepts first.  Build it with
 * -fstack-protector-all deliberately: probe 1 is a 35-byte overrun of a stack
 * buffer, and without a canary on every frame it can land in padding and look
 * like a pass.  A canary-only run can still miss it, so probe 1 also brackets
 * its own buffer explicitly.
 *
 * Every probe runs in a forked child, so a crash is reported as a signal rather
 * than ending the run -- the pattern smoke-test.c uses and for the same reason.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <glob.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int pass, fail;

static void ok(const char *what, int good, const char *detail)
{
  if (good) { pass++; printf("  ok    %-46s %s\n", what, detail ? detail : ""); }
  else      { fail++; printf("  FAIL  %-46s %s\n", what, detail ? detail : ""); }
}

/* Run FN in a child; a signal is a failure with the signal named.  */
static void probe(const char *what, int (*fn)(void))
{
  fflush(NULL);
  pid_t p = fork();
  if (p == 0) _exit(fn() ? 0 : 1);

  int st;
  waitpid(p, &st, 0);
  if (WIFSIGNALED(st))
    {
      char b[64];
      snprintf(b, sizeof b, "killed by signal %d (%s)", WTERMSIG(st),
	       strsignal(WTERMSIG(st)));
      ok(what, 0, b);
    }
  else
    ok(what, WIFEXITED(st) && WEXITSTATUS(st) == 0,
       WEXITSTATUS(st) == 1 ? "assertion failed in child" : NULL);
}

/* ------------------------------------------------------------------------
   1.  expand_chroot_rel_path() overran its buffer by ANDROID_BASE_LEN.

   rel2abs fills the caller's char[PATH_MAX] to 4095, then the prefix is
   memmove'd in on top with no bound -- 35 bytes past the end for any
   absolutised path of 4061+.  Reachable from ~115 wrappers, and in syscall.c
   the destination is alloca'd, so nothing sits between it and the return
   address.  The call must now fail cleanly instead.  */
static int t_longpath (void)
{
  static char path[8192];
  int bad = 0;

  for (size_t len = 4050; len <= 4200; len += 10)
    {
      path[0] = '/';
      memset (path + 1, 'a', len - 1);
      path[len] = '\0';

      /* Each of these takes a different route into the expander.  */
      errno = 0; if (open (path, O_RDONLY) != -1) bad = 1;
      struct stat sb;
      errno = 0; if (stat (path, &sb) != -1) bad = 1;
      errno = 0; if (unlink (path) != -1) bad = 1;
      errno = 0; if (openat (AT_FDCWD, path, O_RDONLY) != -1) bad = 1;
      errno = 0; if (access (path, F_OK) != -1) bad = 1;
      errno = 0; if (rename (path, path) != -1) bad = 1;   /* two buffers */
    }
  return !bad;
}

/* ------------------------------------------------------------------------
   2.  rel2absat() left the process CWD moved.

   It resolves a dirfd by fchdir'ing the WHOLE PROCESS to it and back.  If it
   failed in between -- upstream's error path closed the descriptor and returned
   without restoring -- the process stayed parked in the dirfd, and every later
   relative path resolved against the wrong directory.  */
static int t_at_cwd (void)
{
  char before[4096], after[4096];
  if (getcwd (before, sizeof before) == NULL) return 0;

  /* A closed and a never-valid descriptor: both make rel2absat fail, one of
     them after the first fchdir has already succeeded.  */
  int fd = open ("/", O_RDONLY | O_DIRECTORY);
  if (fd >= 0) close (fd);

  struct stat sb;
  for (int i = 0; i < 50; i++)
    {
      fstatat (fd, "x", &sb, 0);
      fstatat (7777, "x", &sb, 0);
      faccessat (fd, "x", F_OK, 0);
      unlinkat (fd, "x", 0);
    }

  if (getcwd (after, sizeof after) == NULL) return 0;
  if (strcmp (before, after) != 0)
    {
      fprintf (stderr, "    cwd moved: %s -> %s\n", before, after);
      return 0;
    }
  return 1;
}

/* ------------------------------------------------------------------------
   3.  glob() indexed gl_pathv from 0 under GLOB_DOOFFS.

   glibc reserves gl_offs leading slots and NULLs them, storing matches above.
   The result-narrowing loop walked from 0, so the first strcpy read a NULL
   pointer.  Also: it tested `rc < 0', but glob reports errors as POSITIVE
   codes, so GLOB_NOMATCH fell through into a gl_pathv that was never filled. */
static int t_glob_dooffs (void)
{
  glob_t g;
  memset (&g, 0, sizeof g);
  g.gl_offs = 2;

  int rc = glob ("/*", GLOB_DOOFFS, NULL, &g);
  if (rc == 0)
    {
      /* The reserved slots must still be NULL, and the matches must start
	 above them.  */
      if (g.gl_pathv[0] != NULL || g.gl_pathv[1] != NULL) return 0;
      for (size_t i = 0; i < g.gl_pathc; i++)
	if (g.gl_pathv[g.gl_offs + i] == NULL) return 0;
      globfree (&g);
    }

  /* The no-match path must not walk an unfilled vector.  */
  glob_t n;
  memset (&n, 0, sizeof n);
  rc = glob ("/nonexistent-aXbYcZ/*", 0, NULL, &n);
  if (rc == 0) globfree (&n);
  return 1;
}

/* The SAME defects, on the COMPAT arm -- glob@GLIBC_2.17.
 *
 * This probe exists because the modern arm above cannot reach it: the two are
 * different implementations at different addresses, selected by symbol
 * version, so a binary linked against pre-2.27 headers gets the other one.
 * android/glob.c was fixed and fc-glob_lstat_compat.c was not, for as long as
 * its header comment claimed the bodies "mirror exactly" -- nobody re-read the
 * copy that was asserted to be identical.  Both now share
 * android/glob-narrow.h, and this is what keeps that true.
 *
 * The fourth defect, an unbounded strcpy of a match into a
 * FAKECHROOT_PATH_MAX stack buffer, is NOT probed here: reaching it needs a
 * match path longer than PATH_MAX, i.e. a directory tree thousands of levels
 * deep.  It is instead eliminated structurally -- the shared helper narrows in
 * place via narrow_chroot_path and copies nothing, so there is no buffer left
 * to overflow.
 */
static int t_glob_compat (void)
{
  int (*g17) (const char *, int, int (*) (const char *, int), glob_t *)
    = dlvsym (RTLD_DEFAULT, "glob", "GLIBC_2.17");
  if (g17 == NULL)
    return 1;              /* no compat arm in this build; nothing to test */

  /* GLOB_DOOFFS: glibc NULLs the reserved slots and stores matches above them.
     Indexing from 0 read a NULL and copied from it.  */
  glob_t g;
  memset (&g, 0, sizeof g);
  g.gl_offs = 2;
  int rc = g17 ("/*", GLOB_DOOFFS, NULL, &g);
  if (rc == 0)
    {
      if (g.gl_pathv[0] != NULL || g.gl_pathv[1] != NULL) return 0;
      for (size_t i = 0; i < g.gl_pathc; i++)
	if (g.gl_pathv[g.gl_offs + i] == NULL) return 0;
      globfree (&g);
    }

  /* Errors are POSITIVE.  `rc < 0' never fired, so a failed glob fell through
     and walked a gl_pathv that was never filled.  */
  glob_t n;
  memset (&n, 0, sizeof n);
  rc = g17 ("/nonexistent-aXbYcZ/*", 0, NULL, &n);
  if (rc == 0) globfree (&n);

  /* Over-long pattern: expand_chroot_rel_path returns NULL now, and glibc's
     glob dereferences the pattern immediately.  */
  char big[5000];
  memset (big, 'a', sizeof big);
  big[0] = '/';
  big[sizeof big - 3] = '/';
  big[sizeof big - 2] = '*';
  big[sizeof big - 1] = '\0';
  glob_t l;
  memset (&l, 0, sizeof l);
  rc = g17 (big, 0, NULL, &l);
  if (rc == 0) globfree (&l);

  return 1;
}

/* ------------------------------------------------------------------------
   4.  syscall(SYS_rt_sigaction) confused two ABIs.

   A raw caller passes the KERNEL's layout -- handler, flags, restorer, 8 bytes
   of mask, 32 bytes total.  The wrapper handed it to __libc_sigaction, which
   reads sa_flags at offset 136 of glibc's 152-byte userspace struct and, on the
   absorb path, memsets all 152 bytes over it: a 120-byte out-of-bounds write
   into the caller's stack.  Runtimes that install a SIGSYS handler by raw
   syscall -- the audience this handler exists for -- hit it.  */
struct k_sigaction              /* what the kernel actually reads */
{
  void *handler;
  unsigned long flags;
  void *restorer;
  unsigned long mask;
};

static int t_raw_rt_sigaction (void)
{
  struct { unsigned long lo; struct k_sigaction ka; unsigned long hi; } p;
  const unsigned long C = 0x5A5AC0FFEE5A5A5AUL;

  memset (&p, 0, sizeof p);
  p.lo = p.hi = C;

  /* Query only: the absorb path is what used to memset past the end.  */
  long r = syscall (SYS_rt_sigaction, SIGSYS, (void *) 0, &p.ka, 8);
  if (p.lo != C || p.hi != C)
    {
      fprintf (stderr, "    canary clobbered (lo=%#lx hi=%#lx)\n", p.lo, p.hi);
      return 0;
    }
  (void) r;

  /* And a set, which takes the other half of the exchange.  */
  struct { unsigned long lo; struct k_sigaction ka; unsigned long hi; } q;
  memset (&q, 0, sizeof q);
  q.lo = q.hi = C;
  q.ka.handler = (void *) SIG_DFL;
  q.ka.flags = 0;
  syscall (SYS_rt_sigaction, SIGSYS, &q.ka, (void *) 0, 8);
  if (q.lo != C || q.hi != C)
    {
      fprintf (stderr, "    canary clobbered on set\n");
      return 0;
    }

  /* Wrong sigsetsize must be rejected, as the kernel does.  */
  errno = 0;
  if (syscall (SYS_rt_sigaction, SIGSYS, (void *) 0, &q.ka, 7) != -1
      || errno != EINVAL)
    return 0;
  return 1;
}

/* ------------------------------------------------------------------------
   5.  lstat_rel() read st_mode before checking the return.

   On failure glibc leaves the caller's struct stat untouched, so upstream
   decided its symlink fixup from stack garbage -- and a chance S_IFLNK bit
   pattern made it issue a real readlink and write st_size into a struct the
   caller must not read.  Three of the four siblings had the guard; lstat.c and
   lstat64.c did not.  */
static int t_lstat_poisoned (void)
{
  struct stat sb;
  memset (&sb, 0xA5, sizeof sb);       /* S_IFLNK is 0xA000; 0xA5A5 matches */
  off_t before = sb.st_size;

  if (lstat ("/nonexistent-aXbYcZ-qq", &sb) != -1) return 0;
  if (sb.st_size != before)
    {
      fprintf (stderr, "    st_size rewritten after a failed lstat\n");
      return 0;
    }
  return 1;
}

/* ------------------------------------------------------------------------
   6.  posix_spawn's open file action carried an untranslated path.

   __spawni_child opens it with __open_nocancel, which no wrapper covers, so the
   child failed and exited 127 with no diagnostic.  Translating at registration
   fixes it.  Child mode: assert the action's fd is open, exit 42.  */
static const char *self_path;

static int t_spawn_addopen (void)
{
  if (self_path == NULL || self_path[0] != '/')
    { printf ("  skip  spawn addopen (need absolute argv[0])\n"); return 1; }

  posix_spawn_file_actions_t fa;
  if (posix_spawn_file_actions_init (&fa) != 0) return 0;
  /* /etc/hostname-ish would be guesswork; use a path we know translates and
     exists in the chroot view: the test binary itself.  */
  if (posix_spawn_file_actions_addopen (&fa, 9, self_path, O_RDONLY, 0) != 0)
    return 0;

  char *const argv[] = { (char *) self_path, (char *) "--spawn-child", NULL };
  pid_t pid;
  if (posix_spawn (&pid, self_path, &fa, NULL, argv, environ) != 0) return 0;

  int st;
  waitpid (pid, &st, 0);
  posix_spawn_file_actions_destroy (&fa);

  if (WIFEXITED (st) && WEXITSTATUS (st) == 127)
    {
      fprintf (stderr, "    child exited 127 -- the file action failed\n");
      return 0;
    }
  return WIFEXITED (st) && WEXITSTATUS (st) == 42;
}

/* ------------------------------------------------------------------------
   7.  execveat() took a real path and had no wrapper at all.

   It is exported (posix/Versions:156) but upstream fakechroot never wrapped it,
   so it handed untranslated paths to the kernel -- the same bug wiring execve
   fixed, and it matters for the same reason: /nix does not exist at the real
   root.  If the path is translated the child runs; if not, exec fails.  */
static int t_execveat (void)
{
  /* The path has to be one that ONLY resolves after translation, or the probe
     proves nothing: argv[0] is relative and the process's real CWD is already
     inside the translated tree, so an untranslated execveat of it succeeds --
     this probe passed against a build with no execveat wrapper at all before
     that was noticed.  /nix is the discriminator this port is built around: it
     does not exist at the real root, only under ANDROID_BASE.  realpath() is
     itself wrapped, so it hands back the narrowed /nix/store/... form.  */
  char interp[PATH_MAX];
  if (realpath ("/bin/sh", interp) == NULL)
    { printf ("  skip  execveat (cannot resolve /bin/sh)\n"); return 1; }
  if (strncmp (interp, "/nix/", 5) != 0)
    { printf ("  skip  execveat (%s is not under /nix)\n", interp); return 1; }

  fflush (NULL);
  pid_t p = fork ();
  if (p == 0)
    {
      char *const av[] = { interp, (char *) "-c", (char *) "exit 43", NULL };
      execveat (AT_FDCWD, interp, av, environ, 0);
      _exit (127);                       /* exec failed */
    }
  int st;
  waitpid (p, &st, 0);
  if (WIFEXITED (st) && WEXITSTATUS (st) == 127)
    {
      fprintf (stderr, "    execveat(\"%s\") failed -- path not translated\n",
	       interp);
      return 0;
    }
  return WIFEXITED (st) && WEXITSTATUS (st) == 43;
}

/* ------------------------------------------------------------------------
   8.  scandirat() reached __opendirat, which no wrapper covers.

   scandir and scandir64 were fine -- glibc routes those through __opendir,
   which IS wrapped -- so this hole was invisible from the obvious direction.
   Same discriminator as probe 7: /nix exists only under ANDROID_BASE, so a
   scandirat of it succeeds when translated and fails ENOENT when not.  */
static int sel_all (const struct dirent *d) { (void) d; return 1; }

static int t_scandirat (void)
{
  struct dirent **nl = NULL;
  int n = scandirat (AT_FDCWD, "/nix", &nl, sel_all, alphasort);

  if (n < 0)
    {
      fprintf (stderr, "    scandirat(\"/nix\") failed (%s) -- not translated\n",
	       strerror (errno));
      return 0;
    }
  for (int i = 0; i < n; i++)
    free (nl[i]);
  free (nl);
  return n > 0;
}

int main (int argc, char **argv)
{
  if (argc > 1 && strcmp (argv[1], "--spawn-child") == 0)
    _exit (fcntl (9, F_GETFD) != -1 ? 42 : 127);
  if (argc > 1 && strcmp (argv[1], "--exec-child") == 0)
    _exit (43);

  self_path = argv[0];

  puts ("safety-test: regressions for the proofread memory-safety fixes");
  probe ("expand_chroot_rel_path 35-byte overrun", t_longpath);
  probe ("rel2absat leaves CWD moved",             t_at_cwd);
  probe ("glob GLOB_DOOFFS / positive rc",         t_glob_dooffs);
  probe ("glob@GLIBC_2.17 compat arm",             t_glob_compat);
  probe ("raw syscall(SYS_rt_sigaction) OOB write", t_raw_rt_sigaction);
  probe ("lstat reads st_mode before retval",      t_lstat_poisoned);
  probe ("posix_spawn addopen untranslated",       t_spawn_addopen);
  probe ("execveat untranslated path",             t_execveat);
  probe ("scandirat untranslated path",            t_scandirat);

  printf ("\n%d passed, %d failed\n", pass, fail);
  return fail != 0;
}
