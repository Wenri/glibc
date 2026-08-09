/* Runtime test for the exec family.
 *
 * Why it exists.  smoke-test calls execv/execl/execle/execlp/execvp but only
 * asks whether they RETURN without a signal, so it passed while all five were
 * functional no-ops: those wrappers expand nothing themselves
 * (android/execv.c:35 and friends just marshal arguments and call
 * the public execve), and until execve was wired that public name was glibc's
 * untranslated syscall stub.  /nix does not exist at the real root -- a raw
 * newfstatat of "/nix" returns -ENOENT -- so every store path they were handed
 * failed with ENOENT.  This test asks whether the child actually RAN.
 *
 * BUILD AND RUN.  Both halves matter, and getting either wrong makes every
 * probe pass against the wrong libc:
 *
 *   A=/data/data/com.termux.nix/files/usr
 *   gcc -O0 -g -o exec-test exec-test.c \
 *       -Wl,--dynamic-linker=$A<fc>/lib/ld-linux-aarch64.so.1 \
 *       -Wl,-rpath,$A<fc>/lib
 *   <fc>/lib/ld-linux-aarch64.so.1 --library-path <fc>/lib ./exec-test
 *
 * The ANDROID-PREFIXED PT_INTERP is what a real patched nix binary carries,
 * and it is load-bearing twice: the kernel resolves PT_INTERP itself and cannot
 * see /nix, and it is also the string is_direct_exec_interp (execve.c:186)
 * compares against ANDROID_ELFLOADER.  Since it names OUR loader, the children
 * this test re-execs take the "already patched, execute directly" arm and stay
 * on this libc.  Point it at the bare /nix path and the binary will not start.
 *
 * The explicit loader on the FIRST invocation is not redundant with that.  The
 * shell running this is itself under the deployed /etc/ld.so.preload
 * libfakechroot, whose own execve ELF-sniffs this binary, finds a PT_INTERP
 * that is not in ITS allowlist, and re-execs it through the DEPLOYED loader --
 * so a bare ./exec-test silently runs against the deployed glibc and reports
 * 12/12 for the wrong build.  Our loader reads ld-nix.so.preload, not
 * ld.so.preload, so entering through it is what escapes that.  The `libc =`
 * line printed below says which one actually won; check it.
 *
 * Statically, against <fc-static>/lib/libc.a (see sigsys-test.c's header for
 * the -B directory, needed because the crt objects and libc.a are in different
 * outputs).  No PT_INTERP and no preload are involved there:
 *
 *   gcc -O0 -g -static -BB -o exec-test-static exec-test.c && ./exec-test-static
 *
 * The test re-execs ITSELF with a marker argument, so it needs no external
 * program and no assumption about what is installed.  Each probe runs in a
 * forked child; the child reports through its exit status, and prints the one
 * thing the parent cannot see (its own argv[0] / environment).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>

extern char **environ;

/* Same literal as android/config.h ANDROID_BASE and behave-test.c
   BASE.  The test compiles standalone, so it cannot include config.h.  */
#define BASE      "/data/data/com.termux.nix/files/usr"
#define MARK      "FC_EXEC_MARK"
#define CHILD_OK  42          /* what a successfully exec'd child exits with */

static char self[4096];       /* absolute path to this binary */
static int failures, probes;

static void __attribute__ ((noreturn))
die (int code)
{
  fflush (NULL);
  _exit (code);
}

/* Reference probes must reach the kernel UNTRANSLATED -- syscall() is a
   fakechroot wrapper in this libc, so it is not raw.  See behave-test.c.  */
#ifndef __aarch64__
# error "exec-test's raw probes are aarch64 inline asm"
#endif
static long
raw (long nr, long a0, long a1, long a2, long a3)
{
  register long x8 asm ("x8") = nr, x0 asm ("x0") = a0, x1 asm ("x1") = a1,
                x2 asm ("x2") = a2, x3 asm ("x3") = a3;
  asm volatile ("svc #0" : "+r" (x0)
		: "r" (x1), "r" (x2), "r" (x3), "r" (x8) : "memory", "cc");
  return x0;
}

static int
raw_exists (const char *p)
{
  struct stat st;
  return raw (__NR_newfstatat, AT_FDCWD, (long) p, (long) &st,
	      AT_SYMLINK_NOFOLLOW) == 0;
}

#define PROBE(name, ...)						\
  do {									\
    fflush (NULL);							\
    pid_t _p = fork ();							\
    if (_p == 0) { __VA_ARGS__; die (1); }  /* exec must not return */	\
    int _st; waitpid (_p, &_st, 0);					\
    ++probes;								\
    if (WIFSIGNALED (_st))						\
      { printf ("  %-44s KILLED by %s\n", name,				\
		strsignal (WTERMSIG (_st))); ++failures; }		\
    else if (WEXITSTATUS (_st) != CHILD_OK)				\
      { printf ("  %-44s FAIL (exit %d)\n", name, WEXITSTATUS (_st));	\
	++failures; }							\
    else printf ("  %-44s ok\n", name);					\
  } while (0)

/* ------------------------------------------------------------------ */
/* The re-exec'd child.  argv[1] selects what it checks.               */

static int
child_main (int argc, char **argv)
{
  const char *what = argv[1];

  if (strcmp (what, "plain") == 0)
    return CHILD_OK;

  if (strcmp (what, "argv0") == 0)
    {
      /* A wrapped exec goes through ld.so with --argv0, so argv[0] must be
	 what the caller passed, not the loader.  */
      if (strstr (argv[0], "ld-linux") != NULL)
	{ printf ("      argv[0] leaked the loader: %s\n", argv[0]); return 3; }
      if (strcmp (argv[0], "fc-argv0-probe") != 0)
	{ printf ("      argv[0] = %s, wanted fc-argv0-probe\n", argv[0]);
	  return 4; }
      return CHILD_OK;
    }

  if (strcmp (what, "envexact") == 0)
    {
      /* The env-injection decision: the child must see EXACTLY what was
	 passed, with none of fakechroot's five inherited variables added.  */
      int n = 0;
      for (char **e = environ; *e != NULL; ++e, ++n)
	if (strncmp (*e, MARK "=", sizeof MARK) != 0)
	  { printf ("      unexpected inherited var: %s\n", *e); return 5; }
      if (n != 1)
	{ printf ("      environ has %d entries, wanted 1\n", n); return 6; }
      return CHILD_OK;
    }

  if (strcmp (what, "bigenv") == 0)
    {
      /* Sized to stress execve.c's VLAs; just has to survive.  */
      return CHILD_OK;
    }

  printf ("      child: unknown probe '%s'\n", what);
  return 7;
}

/* ------------------------------------------------------------------ */

int
main (int argc, char **argv)
{
  if (argc > 1)
    return child_main (argc, argv);

  /* argv[0], NOT /proc/self/exe.  Under an explicit loader that symlink names
     the LOADER, so re-execing it runs ld.so with the probe name as its program
     ("cannot open shared object file: plain") -- which is how this test first
     reported a working port as 1/12.  argv[0] is what the caller actually
     used, and needs no /proc.  */
  if (argv[0][0] == '/')
    snprintf (self, sizeof self, "%s", argv[0]);
  else
    {
      char cwd[2048];
      if (getcwd (cwd, sizeof cwd) == NULL) { perror ("getcwd"); return 2; }
      snprintf (self, sizeof self, "%s/%s", cwd, argv[0]);
    }

  printf ("\n  self = %s\n", self);
  printf ("  raw_exists(\"/nix\") = %d   (0 => an untranslated exec of a store"
	  " path cannot work)\n", raw_exists ("/nix"));

  /* Say which libc is actually mapped.  Every probe below is meaningless if
     this is not the build under test -- the ambient /etc/ld.so.preload plus
     the system loader will happily make them all pass.  */
  {
    FILE *m = fopen ("/proc/self/maps", "r");
    char line[1024];
    if (m != NULL)
      {
	while (fgets (line, sizeof line, m) != NULL)
	  if (strstr (line, "libc.so.6") != NULL)
	    { char *p = strchr (line, '/');
	      if (p != NULL) printf ("  libc = %s", p); break; }
	fclose (m);
      }
    if (raw_exists ("/etc/ld.so.preload"))
      puts ("  NOTE: /etc/ld.so.preload exists -- if the system loader ran"
	    " this, results are meaningless");
  }

  char *av[]  = { (char *) "exec-test", (char *) "plain", NULL };
  char *env[] = { (char *) MARK "=1", NULL };

  /* A path that CANNOT be exec'd without translation.  self lives at a real
     Android path, so exec'ing it proves only that exec works -- it would
     succeed with an untranslated execve too.  Build the /nix spelling of this
     libc's own loader instead by stripping ANDROID_BASE off the mapping found
     above; raw_exists says the kernel cannot see that path, so an exec of it
     succeeds only if the wrapper rewrote it.  */
  char nixldso[4096] = "";
  {
    FILE *m = fopen ("/proc/self/maps", "r");
    char line[1024];
    if (m != NULL)
      {
	while (fgets (line, sizeof line, m) != NULL)
	  {
	    char *p = strstr (line, BASE "/nix/store/");
	    char *q = p != NULL ? strstr (p, "/lib/libc.so.6") : NULL;
	    if (q != NULL)
	      {
		*q = '\0';
		snprintf (nixldso, sizeof nixldso, "%s/lib/ld-linux-aarch64.so.1",
			  p + sizeof (BASE) - 1);
		break;
	      }
	  }
	fclose (m);
      }
  }

  puts ("\n== a path only translation can reach ==");
  if (nixldso[0] == '\0')
    puts ("  (skipped: could not derive the /nix spelling from /proc/self/maps)");
  else
    {
      printf ("  target = %s  (raw_exists = %d)\n", nixldso,
	      raw_exists (nixldso));
      char libdir[4096];
      snprintf (libdir, sizeof libdir, "%s", nixldso);
      char *slash = strrchr (libdir, '/');
      if (slash != NULL) *slash = '\0';
      char *lv[] = { (char *) "ld", (char *) "--library-path", libdir,
		     self, (char *) "plain", NULL };
      PROBE ("execve a /nix path (fails untranslated)",
	     execve (nixldso, lv, environ));
    }

  puts ("\n== the exec family runs the child ==");

  PROBE ("execve", execve (self, av, environ));
  PROBE ("execv",  execv (self, av));
  PROBE ("execl",  execl (self, "exec-test", "plain", (char *) NULL));
  PROBE ("execle", execle (self, "exec-test", "plain", (char *) NULL, environ));
  PROBE ("execvp", execvp (self, av));     /* has a slash: no PATH search */
  PROBE ("execlp", execlp (self, "exec-test", "plain", (char *) NULL));

  puts ("\n== argv[0] survives the ld.so hop ==");
  {
    char *a0[] = { (char *) "fc-argv0-probe", (char *) "argv0", NULL };
    PROBE ("wrapped exec keeps the caller's argv[0]", execve (self, a0, environ));
  }

  puts ("\n== envp is honoured exactly (no inherited injection) ==");
  {
    char *ae[] = { (char *) "exec-test", (char *) "envexact", NULL };
    PROBE ("child sees only " MARK, execve (self, ae, env));
  }

  puts ("\n== posix_spawn: the CLONE_VM|CLONE_VFORK child stack ==");
  {
    /* ~300 variables plus a long LD_LIBRARY_PATH is the shape that overflows
       spawni.c's stack once __execve is __fc_execve.  See spawni.c's
       ANDROID_SPAWN_STACK block.  */
    static char *big[512];
    static char buf[300][64];
    int k = 0;
    static char llp[4096];
    strcpy (llp, "LD_LIBRARY_PATH=");
    for (int i = 0; i < 24; i++)
      strcat (llp, "/nix/store/0123456789abcdefghijklmnopqrstuv-pad/lib:");
    big[k++] = llp;
    for (int i = 0; i < 300; i++)
      { snprintf (buf[i], sizeof buf[i], "FC_PAD_%03d=0123456789abcdef", i);
	big[k++] = buf[i]; }
    big[k] = NULL;

    char *sv[] = { (char *) "exec-test", (char *) "bigenv", NULL };

    fflush (NULL);
    pid_t p; int st;
    ++probes;
    int rc = posix_spawn (&p, self, NULL, NULL, sv, big);
    if (rc != 0)
      { printf ("  %-44s FAIL (posix_spawn: %s)\n", "posix_spawn, ~300 vars",
		strerror (rc)); ++failures; }
    else
      {
	waitpid (p, &st, 0);
	if (WIFSIGNALED (st))
	  { printf ("  %-44s KILLED by %s\n", "posix_spawn, ~300 vars",
		    strsignal (WTERMSIG (st))); ++failures; }
	else if (WEXITSTATUS (st) != CHILD_OK)
	  { printf ("  %-44s FAIL (exit %d)\n", "posix_spawn, ~300 vars",
		    WEXITSTATUS (st)); ++failures; }
	else printf ("  %-44s ok\n", "posix_spawn, ~300 vars");
      }

    /* posix_spawnp adds __execvpe_common's PATH buffer to the same stack. */
    ++probes;
    rc = posix_spawnp (&p, self, NULL, NULL, sv, big);
    if (rc != 0)
      { printf ("  %-44s FAIL (posix_spawnp: %s)\n", "posix_spawnp, ~300 vars",
		strerror (rc)); ++failures; }
    else
      {
	waitpid (p, &st, 0);
	if (WIFSIGNALED (st) || WEXITSTATUS (st) != CHILD_OK)
	  { printf ("  %-44s FAIL\n", "posix_spawnp, ~300 vars"); ++failures; }
	else printf ("  %-44s ok\n", "posix_spawnp, ~300 vars");
      }

    /* With debugging on, fakechroot_debug's 2,400-byte frame nests inside
       rel2abs at the deepest point of that same chain.  */
    ++probes;
    setenv ("FAKECHROOT_DEBUG", "1", 1);
    rc = posix_spawn (&p, self, NULL, NULL, sv, big);
    unsetenv ("FAKECHROOT_DEBUG");
    if (rc != 0)
      { printf ("  %-44s FAIL (%s)\n", "posix_spawn under FAKECHROOT_DEBUG",
		strerror (rc)); ++failures; }
    else
      {
	waitpid (p, &st, 0);
	if (WIFSIGNALED (st) || WEXITSTATUS (st) != CHILD_OK)
	  { printf ("  %-44s FAIL\n", "posix_spawn under FAKECHROOT_DEBUG");
	    ++failures; }
	else printf ("  %-44s ok\n", "posix_spawn under FAKECHROOT_DEBUG");
      }
  }

  puts ("\n== shebang script ==");
  {
    /* Written under a path the loader must translate.  #! routes through
       exec_build_script_argv, which is the other half of execve.c.  */
    char script[4096];
    snprintf (script, sizeof script, "/tmp/fc-exec-%d.sh", (int) getpid ());
    int fd = open (script, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd < 0)
      { printf ("  %-44s SKIP (%s)\n", "shebang script", strerror (errno)); }
    else
      {
	const char *body = "#!/bin/sh\nexit 42\n";
	ssize_t w = write (fd, body, strlen (body));
	close (fd);
	if (w != (ssize_t) strlen (body))
	  printf ("  %-44s SKIP (short write)\n", "shebang script");
	else
	  {
	    char *sa[] = { script, NULL };
	    PROBE ("shebang script runs", execve (script, sa, environ));
	  }
	unlink (script);
      }
  }

  printf ("\n%d/%d probes ok\n", probes - failures, probes);
  return failures != 0;
}
