/* Runtime smoke test: call EVERY wired wrapper once.
 *
 * Why this exists.  euidaccess shipped with its nextcall override pointing at
 * __euidaccess, which wrapper() aliases to the wrapper itself, so __fc_euidaccess
 * called itself until the stack ran out.  Every build-time check passed -- the
 * ABI diff was clean, all aliases sat at one address, and verify-fc.sh's next=
 * check said 1, because __android_next_euidaccess DID resolve to a real,
 * distinct symbol.  Symbol verification cannot see recursion.  The defect
 * surfaced only when glibc's own install phase ran localedef and it segfaulted.
 *
 * So: actually call them.  Each call runs in a forked child, so a crash is
 * reported with its signal instead of ending the run, and one bad wrapper does
 * not hide the other 85.
 *
 *   gcc -O0 -g -o smoke-test smoke-test.c
 *   <fc>/lib/ld-linux-aarch64.so.1 --library-path <fc>/lib ./smoke-test
 *
 * Run it via OUR loader -- see behave-test.c for why the ambient
 * /etc/ld.so.preload otherwise intercepts first.
 *
 * A wrapper FAILING is fine and expected (ENOENT, EPERM, ...); this asks only
 * whether it RETURNS.  Only a signal is a failure.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <glob.h>
#include <libintl.h>
#include <utime.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <shadow.h>
#include <sys/xattr.h>
#include <sys/inotify.h>
#include <sys/syscall.h>

static char dir[256];          /* scratch directory, inside the chroot view */
static char pa[320], pb[320];  /* two scratch paths under it */

/* Raw cleanup, so tearing down a probe's artefacts never re-enters a
   wrapper and cannot itself be the thing that fails.  */
static void raw_rmdir (const char *p)
{ syscall (SYS_unlinkat, AT_FDCWD, p, AT_REMOVEDIR); }
static void raw_unlink (const char *p)
{ syscall (SYS_unlinkat, AT_FDCWD, p, 0); }

/* Run one call in a child; only a signal counts as a failure.  A do/while
   macro rather than a helper taking a function pointer: nested functions cannot
   be static, and non-static ones would need executable-stack trampolines.  */
static int fails, ran;
/* There is deliberately no "expected SIGSYS" escape hatch.  acct used to have
   one, because Android's seccomp filter killed it and this glibc had no
   handler; ld.so installs one now, so a SIGSYS here is a real regression.
   Keeping the annotation around would invite someone to silence the next one
   the same way, which is exactly how the absence would go unnoticed.  */
#define P(name, body)                                                        \
  do {                                                                       \
    fflush (NULL);                                                           \
    pid_t _p = fork ();                                                      \
    if (_p == 0) { alarm (5); { body } _exit (0); }                          \
    int _st; waitpid (_p, &_st, 0); ran++;                                   \
    if (WIFSIGNALED (_st))                                                   \
      {                                                                      \
        int _s = WTERMSIG (_st);                                             \
        printf ("  %-24s CRASHED (%s)\n", name,                              \
                _s == SIGSEGV ? "SIGSEGV -- recursion?"                      \
                : _s == SIGSYS  ? "SIGSYS -- no seccomp handler installed"   \
                : _s == SIGALRM ? "SIGALRM -- hung" : strsignal (_s));       \
        fails++;                                                             \
      }                                                                      \
  } while (0)

int main (void)
{
  snprintf (dir, sizeof dir, "/fc-smoke-%d", (int) getpid ());
  mkdir (dir, 0755);
  snprintf (pa, sizeof pa, "%s/a", dir);
  snprintf (pb, sizeof pb, "%s/b", dir);
  { int fd = creat (pa, 0644); if (fd >= 0) close (fd); }

  struct stat st; struct stat64 st64; struct statfs sf; struct statvfs sv;
  struct statx sx; struct dirent **nl; struct dirent64 **nl64; glob_t g;
  char buf[512], tmpl[320]; DIR *d; FILE *f;

  /* --- path predicates and metadata ------------------------------------ */
  P ("access",      { access (pa, F_OK); });
  P ("eaccess",     { eaccess (pa, F_OK); });
  P ("euidaccess",  { euidaccess (pa, F_OK); });
  P ("faccessat",   { faccessat (AT_FDCWD, pa, F_OK, 0); });
  P ("stat",        { stat (pa, &st); });
  P ("stat64",      { stat64 (pa, &st64); });
  P ("lstat",       { lstat (pa, &st); });
  P ("lstat64",     { lstat64 (pa, &st64); });
  P ("fstatat",     { fstatat (AT_FDCWD, pa, &st, 0); });
  P ("fstatat64",   { fstatat64 (AT_FDCWD, pa, &st64, 0); });
  P ("statx",       { statx (AT_FDCWD, pa, 0, STATX_BASIC_STATS, &sx); });
  P ("statfs",      { statfs (pa, &sf); });
  P ("statfs64",    { statfs64 (pa, (struct statfs64 *) &sf); });
  P ("statvfs",     { statvfs (pa, &sv); });
  P ("statvfs64",   { statvfs64 (pa, (struct statvfs64 *) &sv); });
  P ("pathconf",    { pathconf (pa, _PC_NAME_MAX); });

  /* --- permissions and ownership --------------------------------------- */
  P ("chmod",       { chmod (pa, 0644); });
  P ("lchmod",      { lchmod (pa, 0644); });
  P ("fchmodat",    { fchmodat (AT_FDCWD, pa, 0644, 0); });
  P ("chown",       { chown (pa, getuid (), getgid ()); });
  P ("lchown",      { lchown (pa, getuid (), getgid ()); });

  /* --- times ------------------------------------------------------------ */
  P ("utime",       { utime (pa, NULL); });
  P ("utimes",      { utimes (pa, NULL); });
  P ("lutimes",     { lutimes (pa, NULL); });
  P ("futimesat",   { futimesat (AT_FDCWD, pa, NULL); });
  P ("utimensat",   { utimensat (AT_FDCWD, pa, NULL, 0); });

  /* --- directory and link management ------------------------------------ */
  P ("mkdir",       { char q[400]; snprintf (q, sizeof q, "%s/d1", dir); mkdir (q, 0755); rmdir (q); });
  P ("rmdir",       { char q[400]; snprintf (q, sizeof q, "%s/d2", dir); mkdir (q, 0755); rmdir (q); });
  P ("link",        { unlink (pb); link (pa, pb); unlink (pb); });
  P ("symlink",     { unlink (pb); symlink (pa, pb); });
  P ("readlink",    { readlink (pb, buf, sizeof buf); });
  P ("unlink",      { unlink (pb); });
  P ("remove",      { char q[400]; snprintf (q, sizeof q, "%s/r", dir);
                      int fd = creat (q, 0644); if (fd >= 0) close (fd); remove (q); });
  P ("rename",      { char q[400]; snprintf (q, sizeof q, "%s/r2", dir); rename (pa, q); rename (q, pa); });
  P ("renameat",    { char q[400]; snprintf (q, sizeof q, "%s/r3", dir);
                      renameat (AT_FDCWD, pa, AT_FDCWD, q); renameat (AT_FDCWD, q, AT_FDCWD, pa); });
  P ("renameat2",   { renameat2 (AT_FDCWD, pa, AT_FDCWD, pa, 0); });
  P ("mknod",       { char q[400]; snprintf (q, sizeof q, "%s/n", dir); mknod (q, S_IFIFO | 0644, 0); unlink (q); });
  P ("mknodat",     { char q[400]; snprintf (q, sizeof q, "%s/n2", dir); mknodat (AT_FDCWD, q, S_IFIFO | 0644, 0); unlink (q); });
  P ("mkfifo",      { char q[400]; snprintf (q, sizeof q, "%s/f", dir); mkfifo (q, 0644); unlink (q); });
  P ("mkfifoat",    { char q[400]; snprintf (q, sizeof q, "%s/f2", dir); mkfifoat (AT_FDCWD, q, 0644); unlink (q); });
  P ("revoke",      { revoke (pa); });

  /* --- descriptors and streams ------------------------------------------ */
  P ("open",        { int fd = open (pa, O_RDONLY); if (fd >= 0) close (fd); });
  P ("open64",      { int fd = open64 (pa, O_RDONLY); if (fd >= 0) close (fd); });
  P ("openat",      { int fd = openat (AT_FDCWD, pa, O_RDONLY); if (fd >= 0) close (fd); });
  P ("openat64",    { int fd = openat64 (AT_FDCWD, pa, O_RDONLY); if (fd >= 0) close (fd); });
  P ("creat",       { char q[400]; snprintf (q, sizeof q, "%s/c", dir);
                      int fd = creat (q, 0644); if (fd >= 0) close (fd); unlink (q); });
  P ("creat64",     { char q[400]; snprintf (q, sizeof q, "%s/c2", dir);
                      int fd = creat64 (q, 0644); if (fd >= 0) close (fd); unlink (q); });
  P ("truncate",    { truncate (pa, 0); });
  P ("truncate64",  { truncate64 (pa, 0); });
  P ("fopen",       { FILE *x = fopen (pa, "r"); if (x) fclose (x); });
  P ("fopen64",     { FILE *x = fopen64 (pa, "r"); if (x) fclose (x); });
  P ("freopen",     { FILE *x = fopen (pa, "r"); if (x) { freopen (pa, "r", x); fclose (x); } });
  P ("freopen64",   { FILE *x = fopen (pa, "r"); if (x) { freopen64 (pa, "r", x); fclose (x); } });

  /* --- directory reading ------------------------------------------------ */
  P ("opendir",     { d = opendir (dir); if (d) closedir (d); });
  P ("scandir",     { int n = scandir (dir, &nl, NULL, alphasort); if (n > 0) free (nl); });
  P ("scandir64",   { int n = scandir64 (dir, &nl64, NULL, alphasort64); if (n > 0) free (nl64); });

  /* --- cwd and canonicalisation ----------------------------------------- */
  P ("getwd",       { getwd (buf); });
  P ("get_current_dir_name",        { char *c = get_current_dir_name (); free (c); });
  P ("canonicalize_file_name",{ char *c = canonicalize_file_name (pa); free (c); });
  P ("glob_pattern_p",  { glob_pattern_p ("a*b", 1); });

  /* --- temporary names --------------------------------------------------- */
  P ("mktemp",      { snprintf (tmpl, sizeof tmpl, "%s/tXXXXXX", dir); mktemp (tmpl); });
  P ("mkdtemp",     { snprintf (tmpl, sizeof tmpl, "%s/dXXXXXX", dir);
                      char *r = mkdtemp (tmpl); if (r) rmdir (r); });
  P ("mkstemp",     { snprintf (tmpl, sizeof tmpl, "%s/sXXXXXX", dir);
                      int fd = mkstemp (tmpl); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkstemp64",   { snprintf (tmpl, sizeof tmpl, "%s/SXXXXXX", dir);
                      int fd = mkstemp64 (tmpl); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkostemp",    { snprintf (tmpl, sizeof tmpl, "%s/oXXXXXX", dir);
                      int fd = mkostemp (tmpl, 0); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkostemp64",  { snprintf (tmpl, sizeof tmpl, "%s/OXXXXXX", dir);
                      int fd = mkostemp64 (tmpl, 0); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkstemps",    { snprintf (tmpl, sizeof tmpl, "%s/pXXXXXX.s", dir);
                      int fd = mkstemps (tmpl, 2); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkstemps64",  { snprintf (tmpl, sizeof tmpl, "%s/PXXXXXX.s", dir);
                      int fd = mkstemps64 (tmpl, 2); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkostemps",   { snprintf (tmpl, sizeof tmpl, "%s/qXXXXXX.s", dir);
                      int fd = mkostemps (tmpl, 2, 0); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("mkostemps64", { snprintf (tmpl, sizeof tmpl, "%s/QXXXXXX.s", dir);
                      int fd = mkostemps64 (tmpl, 2, 0); if (fd >= 0) { close (fd); unlink (tmpl); } });
  P ("tempnam",     { char *c = tempnam (dir, "fc"); free (c); });
  P ("tmpnam",      { char b2[L_tmpnam]; tmpnam (b2); });

  /* --- exec family: each replaces the child, so a bad path just returns -- */
  P ("execl",       { execl (pb, "x", (char *) NULL); });
  P ("execle",      { char *e[] = { NULL }; execle (pb, "x", (char *) NULL, e); });
  P ("execlp",      { execlp ("fc-no-such-binary", "x", (char *) NULL); });
  P ("execv",       { char *a[2]; a[0] = "x"; a[1] = NULL; execv (pb, a); });
  P ("execvp",      { char *a[2]; a[0] = "x"; a[1] = NULL; execvp ("fc-no-such-binary", a); });
  P ("system",      { system ("exit 0"); });

  /* --- sockets ----------------------------------------------------------- */
  P ("bind",        { int s = socket (AF_UNIX, SOCK_STREAM, 0);
                      struct sockaddr_un u; memset (&u, 0, sizeof u); u.sun_family = AF_UNIX;
                      snprintf (u.sun_path, sizeof u.sun_path, "%s/sk", dir);
                      bind (s, (struct sockaddr *) &u, sizeof u);
                      unlink (u.sun_path); close (s); });
  P ("connect",     { int s = socket (AF_UNIX, SOCK_STREAM, 0);
                      struct sockaddr_un u; memset (&u, 0, sizeof u); u.sun_family = AF_UNIX;
                      snprintf (u.sun_path, sizeof u.sun_path, "%s/nope", dir);
                      connect (s, (struct sockaddr *) &u, sizeof u); close (s); });
  P ("getsockname", { int s = socket (AF_UNIX, SOCK_STREAM, 0);
                      struct sockaddr_un u; socklen_t l = sizeof u;
                      getsockname (s, (struct sockaddr *) &u, &l); close (s); });
  P ("getpeername", { int s = socket (AF_UNIX, SOCK_STREAM, 0);
                      struct sockaddr_un u; socklen_t l = sizeof u;
                      getpeername (s, (struct sockaddr *) &u, &l); close (s); });

  /* --- misc -------------------------------------------------------------- */
  P ("chdir",       { char c[512]; if (getcwd (c, sizeof c)) { chdir (dir); chdir (c); } });
  /* Android's seccomp filter traps acct.  It used to kill the process here;
     ld.so's handler now catches it, so this is an ordinary probe.  What it
     returns is meaningless -- see sigsys-test's "returns its own first
     argument" probe -- but it must not die.  */
  P ("acct",        { acct (NULL); });
  /* chroot only sets fakechroot state; harmless in the forked child. */
  P ("chroot",      { chroot (dir); });
  P ("fchownat",    { fchownat (AT_FDCWD, pa, getuid (), getgid (), 0); });
  P ("linkat",      { raw_unlink (pb); linkat (AT_FDCWD, pa, AT_FDCWD, pb, 0); raw_unlink (pb); });
  P ("mkdirat",     { char q[400]; snprintf (q, sizeof q, "%s/da", dir);
                      mkdirat (AT_FDCWD, q, 0755); raw_rmdir (q); });
  P ("symlinkat",   { raw_unlink (pb); symlinkat (pa, AT_FDCWD, pb); });
  P ("readlinkat",  { readlinkat (AT_FDCWD, pb, buf, sizeof buf); });
  P ("unlinkat",    { unlinkat (AT_FDCWD, pb, 0); });
  P ("setxattr",    { setxattr (pa, "user.fc", "v", 1, 0); });
  P ("lsetxattr",   { lsetxattr (pa, "user.fc", "v", 1, 0); });
  P ("getxattr",    { char v[8]; getxattr (pa, "user.fc", v, sizeof v); });
  P ("lgetxattr",   { char v[8]; lgetxattr (pa, "user.fc", v, sizeof v); });
  P ("listxattr",   { listxattr (pa, buf, sizeof buf); });
  P ("llistxattr",  { llistxattr (pa, buf, sizeof buf); });
  P ("removexattr", { removexattr (pa, "user.fc"); });
  P ("lremovexattr",{ lremovexattr (pa, "user.fc"); });
  P ("inotify_add_watch", { int i = inotify_init ();
                            if (i >= 0) { inotify_add_watch (i, dir, IN_MODIFY); close (i); } });
  P ("getcwd",      { char c[512]; getcwd (c, sizeof c); });
  P ("realpath",    { char *r = realpath (dir, NULL); free (r); });
  P ("lckpwdf",     { lckpwdf (); });
  P ("ulckpwdf",    { ulckpwdf (); });
  P ("bindtextdomain", { bindtextdomain ("fcsmoke", dir); });
  P ("clearenv",    { clearenv (); });

  unlink (pa); unlink (pb); rmdir (dir);
  printf ("\n  %d calls, %d crashed\n", ran, fails);
  printf ("%s\n", fails ? "SMOKE FAILURES" : "no wrapper crashed");
  return fails != 0;
}
