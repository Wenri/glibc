/* Behavioural check for the fakechroot-in-glibc port.
 *
 * The build-time checks (ABI diff, per-wrapper alias/address, ld.so purity) all
 * pass even when a wrapper translates NOTHING -- they only prove the symbol
 * resolves to __fc_<f>.  This proves the path is actually rewritten.
 *
 *   gcc -O0 -g -o behave-test behave-test.c
 *   <fc>/lib/ld-linux-aarch64.so.1 --library-path <fc>/lib ./behave-test \
 *        --expect-translation
 *
 * MUST be run via OUR loader.  Every process on the device is otherwise under
 * /etc/ld.so.preload's libfakechroot, which resolves ahead of the in-glibc
 * wrappers; elf/rtld.c reads ld-nix.so.preload instead precisely so this loader
 * is free of it.
 *
 * Our loader is no longer merely "the one without the preload": it also installs
 * the SIGSYS handler (sysdeps/unix/sysv/linux/dl-sigsys.c).  So do not read a
 * probe surviving a seccomp-blocked syscall as evidence that a preload is
 * present -- see sigsys-test.c, which tests that half directly.
 *
 * Also worth running -static against this libc.a, which has no loader at all
 * and no preload by construction:
 *
 *   mkdir B && ln -s <fc>/lib/crt[1in].o <fc-static>/lib/libc.a B/
 *   gcc -O0 -g -static -BB -o behave-test-static behave-test.c
 *   ./behave-test-static --expect-translation
 *
 * That is what shows libc.a's 111 fc-*.o objects really do translate, which is
 * something LD_PRELOAD could never have given a static binary.
 *
 * Two earlier versions of this test were WRONG, both in the direction of
 * reporting a healthy port as broken:
 *
 *   take 1 compared a "raw" syscall path against a translated one -- but the
 *     preload wraps syscall() too and translates its path arguments, so both
 *     views agreed and the probe could only ever say "no translation".
 *   take 2 fixed that but probed stat(), which is NOT wired (the stat family is
 *     deferred with the LFS-collapse work), so it could only say the same.
 *
 * Hence: probe only functions that are actually wired, and run under a loader
 * with no preload.  If raw / and raw BASE ever become indistinguishable again,
 * the probe is being intercepted -- do not trust the result.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glob.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/xattr.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
/* Standalone test: takes -DANDROID_BASE from the build command line like
   the library proper (and exec-test.c) -- see config.h.  */
#ifndef ANDROID_BASE
# error "behave-test: compile with -DANDROID_BASE=\"<prefix>\""
#endif
#define BASE ANDROID_BASE
/* Dynamically linked?  _DYNAMIC is defined by the link editor only when there
   is a PT_DYNAMIC, so a weak reference to it is the standard discriminator --
   and it needs no build flag, so a -static run cannot forget to pass one. */
extern char _DYNAMIC[] __attribute__ ((weak));
static int is_dynamic(void){ return _DYNAMIC != NULL; }

/* Inline svc, NOT syscall().  This is take 1's mistake and it came back: the
   reference probes below must reach the kernel UNTRANSLATED, and syscall() is
   now a fakechroot wrapper in glibc itself -- newfstatat and unlinkat are both
   in PASSTHROUGH_SEQ, so it expands their path arguments.  Using it here made
   raw_exists agree with the translated view and report "present under the real
   root", turning a working port into three MISMATCHes.
   syscall() can never be the raw reference again, under preload or not. */
#ifndef __aarch64__
# error "behave-test's raw probes are aarch64 inline asm; port them before building elsewhere"
#endif
static long raw(long nr,long a0,long a1,long a2,long a3){
  register long x8 asm("x8")=nr, x0 asm("x0")=a0, x1 asm("x1")=a1,
                x2 asm("x2")=a2, x3 asm("x3")=a3;
  asm volatile("svc #0":"+r"(x0):"r"(x1),"r"(x2),"r"(x3),"r"(x8):"memory","cc");
  return x0; }
static int raw_exists(const char *p){ struct stat st;
  return raw(__NR_newfstatat,AT_FDCWD,(long)p,(long)&st,AT_SYMLINK_NOFOLLOW)==0; }
static void raw_rmdir(const char *p){ raw(__NR_unlinkat,AT_FDCWD,(long)p,AT_REMOVEDIR,0); }
static void raw_unlink(const char *p){ raw(__NR_unlinkat,AT_FDCWD,(long)p,0,0); }
int main(int argc,char**argv){
  int expect=(argc>1&&!strcmp(argv[1],"--expect-translation")), fails=0;
  const char *rel="/fc-behave-probe"; char ub[512]; snprintf(ub,sizeof ub,"%s%s",BASE,rel);
  printf("  raw sees / and BASE as %s objects\n",
         raw_exists("/")&&raw_exists(BASE)?"(both present)":"?");

  /* mkdir is WIRED.  The real Android root is read-only, so success alone is
     evidence of translation; confirm by raw-stat'ing both candidates. */
  raw_rmdir(ub); raw_rmdir(rel);
  int r=mkdir(rel,0755), e=errno;
  int at_base=raw_exists(ub), at_root=raw_exists(rel);
  printf("  mkdir(\"%s\") ret=%d errno=%d | under BASE:%s under real root:%s\n",
         rel,r,r?e:0, at_base?"YES":"no", at_root?"YES":"no");
  int tr = (r==0 && at_base && !at_root);
  printf("  -> translation %s (expected %s)  %s\n", tr?"ON":"OFF",
         expect?"ON":"OFF", tr==expect?"ok":"MISMATCH");
  if(tr!=expect) fails++;
  raw_rmdir(ub); raw_rmdir(rel);

  /* symlink + readlink, both WIRED: readlink must round-trip the value. */
  raw_unlink(ub); raw_unlink(rel);
  if(symlink("target-value",rel)==0){
    char b[64]={0}; ssize_t n=readlink(rel,b,sizeof b-1);
    int ok=(n>0&&!strcmp(b,"target-value")&&raw_exists(ub)==expect);
    printf("  symlink+readlink n=%zd \"%s\" under BASE:%s  %s\n",n,b,
           raw_exists(ub)?"YES":"no", ok?"ok":"MISMATCH");
    if(!ok) fails++;
    raw_unlink(ub); raw_unlink(rel);
  } else { printf("  symlink FAILED errno=%d %s\n",errno,expect?"MISMATCH":"ok"); if(expect) fails++; }

  /* statx is WIRED and yields identity, so it can prove translation directly. */
  { struct statx sx; memset(&sx,0,sizeof sx);
    if(statx(AT_FDCWD,"/",0,STATX_INO,&sx)==0){
      struct stat rr,rb;
      syscall(__NR_newfstatat,AT_FDCWD,"/",&rr,0);
      syscall(__NR_newfstatat,AT_FDCWD,BASE,&rb,0);
      int to_base=(sx.stx_ino==rb.st_ino), to_root=(sx.stx_ino==rr.st_ino);
      printf("  statx(\"/\") ino=%llu -> %s  %s\n",(unsigned long long)sx.stx_ino,
             to_base?"BASE":(to_root?"real root":"?"),
             (to_base==expect)?"ok":"MISMATCH");
      if(to_base!=expect) fails++;
    } else printf("  statx failed errno=%d\n",errno);
  }

  /* Collapsed families need EVERY entry point probed, not just one.  open,
     open64, openat and openat64 are a single object here (precondition 7), so a
     wrapper can be symbol-perfect on one name while another still reaches
     untranslated code -- invisible to verify-fc.sh, which would see correct
     aliases at one address for all of them. */
  { struct { const char *n, *p; int at; } t[] = {
      {"open","/fc-behave-oa",0}, {"open64","/fc-behave-ob",0},
      {"openat","/fc-behave-oc",1}, {"openat64","/fc-behave-od",1} };
    for (unsigned i = 0; i < 4; i++) {
      char u[512]; snprintf (u, sizeof u, "%s%s", BASE, t[i].p);
      raw_unlink (u); raw_unlink (t[i].p);
      int fd = t[i].at
        ? (i == 2 ? openat (AT_FDCWD, t[i].p, O_CREAT|O_RDWR, 0644)
                  : openat64 (AT_FDCWD, t[i].p, O_CREAT|O_RDWR, 0644))
        : (i == 0 ? open (t[i].p, O_CREAT|O_RDWR, 0644)
                  : open64 (t[i].p, O_CREAT|O_RDWR, 0644));
      int ok;
      if (fd < 0) ok = !expect;
      else { close (fd); ok = (raw_exists (u) && !raw_exists (t[i].p)) == expect; }
      printf ("  %-9s entry point            %s\n", t[i].n, ok ? "ok" : "MISMATCH");
      if (!ok) fails++;
      raw_unlink (u); raw_unlink (t[i].p);
    }
  }

  /* The stat family is collapsed the same way; probe each entry point.  Using
     "/" as the subject means no file has to be created, and the inode is the
     identity that proves which root was consulted. */
  { struct stat st; struct stat64 st64; unsigned long long b = 0;
    struct stat rb; if (syscall (__NR_newfstatat, AT_FDCWD, BASE, &rb, 0) == 0) b = rb.st_ino;
    unsigned long long g[6] = {0};
    if (stat ("/", &st) == 0) g[0] = st.st_ino;
    if (stat64 ("/", &st64) == 0) g[1] = st64.st_ino;
    if (lstat ("/", &st) == 0) g[2] = st.st_ino;
    if (lstat64 ("/", &st64) == 0) g[3] = st64.st_ino;
    if (fstatat (AT_FDCWD, "/", &st, 0) == 0) g[4] = st.st_ino;
    if (fstatat64 (AT_FDCWD, "/", &st64, 0) == 0) g[5] = st64.st_ino;
    static const char *n[6] = {"stat","stat64","lstat","lstat64","fstatat","fstatat64"};
    for (int i = 0; i < 6; i++) {
      int ok = ((g[i] == b) == expect) && g[i] != 0;
      printf ("  %-9s entry point            %s\n", n[i], ok ? "ok" : "MISMATCH");
      if (!ok) fails++;
    }
  }

  /* creat/truncate/statfs/statvfs -- the remaining collapsed pairs.
     For statfs/statvfs the inode trick does not discriminate (both candidate
     paths are on one filesystem), so use a path that exists ONLY under BASE:
     "/nix" is absent from the real Android root, so a successful stat of it is
     itself the proof of translation. */
  { const char *p = "/fc-behave-creat"; char u[512];
    snprintf (u, sizeof u, "%s%s", BASE, p);
    raw_unlink (u); raw_unlink (p);
    int fd = creat (p, 0644);
    int ok = fd >= 0 && (raw_exists (u) && !raw_exists (p)) == expect;
    if (fd >= 0) close (fd);
    printf ("  creat                        %s\n", ok ? "ok" : "MISMATCH");
    if (!ok) fails++;
    if (fd >= 0) { int r = truncate (p, 0); ok = (r == 0) == expect;
      printf ("  truncate                     %s\n", ok ? "ok" : "MISMATCH");
      if (!ok) fails++; }
    raw_unlink (u); raw_unlink (p); }
  { struct statfs sf; struct statvfs sv;
    int seen_raw = raw_exists ("/nix");
    int a = statfs ("/nix", &sf) == 0, b = statvfs ("/nix", &sv) == 0;
    if (seen_raw)
      printf ("  statfs/statvfs               INCONCLUSIVE (/nix visible raw)\n");
    else {
      printf ("  statfs(\"/nix\")               %s\n", (a == expect) ? "ok" : "MISMATCH");
      printf ("  statvfs(\"/nix\")              %s\n", (b == expect) ? "ok" : "MISMATCH");
      if (a != expect) fails++;
      if (b != expect) fails++;
    } }

  /* Directory readers, using the same /nix discriminator. */
  { if (!raw_exists ("/nix"))
      { struct dirent **nl; struct dirent64 **nl64;
        int a = scandir ("/nix", &nl, NULL, alphasort) >= 0;
        int b = scandir64 ("/nix", &nl64, NULL, alphasort64) >= 0;
        DIR *d = opendir ("/nix"); int c = d != NULL; if (d) closedir (d);
        printf ("  scandir(\"/nix\")              %s\n", (a == expect) ? "ok" : "MISMATCH");
        printf ("  scandir64(\"/nix\")            %s\n", (b == expect) ? "ok" : "MISMATCH");
        printf ("  opendir(\"/nix\")              %s\n", (c == expect) ? "ok" : "MISMATCH");
        if (a != expect) fails++;
        if (b != expect) fails++;
        if (c != expect) fails++; }
    else printf ("  scandir/opendir              INCONCLUSIVE (/nix visible raw)\n"); }

  /* Stream openers, same /nix discriminator. */
  { if (!raw_exists ("/nix"))
      { FILE *a = fopen ("/nix", "r"), *b = fopen64 ("/nix", "r");
        printf ("  fopen(\"/nix\")                %s\n", ((a != NULL) == expect) ? "ok" : "MISMATCH");
        printf ("  fopen64(\"/nix\")              %s\n", ((b != NULL) == expect) ? "ok" : "MISMATCH");
        if ((a != NULL) != expect) fails++;
        if ((b != NULL) != expect) fails++;
        if (a) fclose (a); if (b) fclose (b); } }

  /* glob has TWO implementations at two addresses -- glob@@GLIBC_2.27 and
     glob@GLIBC_2.17 -- and wiring only the default would leave binaries bound
     to the compat version reaching untranslated code.  The ABI diff cannot see
     that (both symbols would still exist), so reach the compat one by version
     and check it translates too. */
  { if (!raw_exists ("/nix"))
      { glob_t g; int ok, leaked;
        typedef int (*globfn) (const char *, int, int (*) (const char *, int), glob_t *);
        static const char *nm[2] = { "glob (default)", "glob@GLIBC_2.17" };
        globfn fn[2] = { (globfn) glob,
                         (globfn) dlvsym (RTLD_DEFAULT, "glob", "GLIBC_2.17") };
        for (int i = 0; i < 2; i++) {
          /* Symbol versions do not exist in a static link, so dlvsym cannot
             find the compat arm there and its absence proves nothing.  Report
             it as skipped rather than as a failure -- this test is also run
             -static, to show libc.a's 111 fc-*.o really do translate.  */
          if (!fn[i] && i == 1 && !is_dynamic ())
            { printf ("  %-28s skipped (static: no symbol versions)\n", nm[i]);
              continue; }
          if (!fn[i]) { printf ("  %-28s MISSING\n", nm[i]); fails++; continue; }
          memset (&g, 0, sizeof g);
          ok = fn[i] ("/nix/*", 0, NULL, &g) == 0 && g.gl_pathc > 0;
          leaked = ok && strncmp (g.gl_pathv[0], BASE, strlen (BASE)) == 0;
          printf ("  %-28s %s\n", nm[i],
                  ((ok && !leaked) == expect) ? "ok" : "MISMATCH");
          if ((ok && !leaked) != expect) fails++;
          if (ok) globfree (&g);
        } } }

  /* The syscalls.list group: glibc generates these, so they need a hand-written
     .c in the ARCH dir to suppress the stub (make-syscalls.sh:44-48).  If that
     suppression ever regresses the stub wins and translation silently stops,
     so probe them. */
  { const char *d = "/fc-behave-mkdirat"; char u[512];
    snprintf (u, sizeof u, "%s%s", BASE, d);
    raw_rmdir (u); raw_rmdir (d);
    int ok = mkdirat (AT_FDCWD, d, 0755) == 0
             && (raw_exists (u) && !raw_exists (d)) == expect;
    printf ("  mkdirat                      %s\n", ok ? "ok" : "MISMATCH");
    if (!ok) fails++;
    raw_rmdir (u); raw_rmdir (d); }
  { const char *l = "/fc-behave-symlinkat"; char u[512], b[64] = {0};
    snprintf (u, sizeof u, "%s%s", BASE, l);
    raw_unlink (u); raw_unlink (l);
    int ok = symlinkat ("target-value", AT_FDCWD, l) == 0
             && readlinkat (AT_FDCWD, l, b, sizeof b - 1) == 12
             && !strcmp (b, "target-value")
             && raw_exists (u) == expect;
    printf ("  symlinkat + readlinkat       %s\n", ok ? "ok" : "MISMATCH");
    if (!ok) fails++;
    ok = unlinkat (AT_FDCWD, l, 0) == 0 && !raw_exists (u);
    printf ("  unlinkat                     %s\n", ok ? "ok" : "MISMATCH");
    if (!ok) fails++;
    raw_unlink (u); raw_unlink (l); }
  { /* "/nix" exists only under BASE.  ENODATA means the file WAS found and
       merely lacks the attribute; ENOENT would mean the path was not
       translated. */
    if (!raw_exists ("/nix"))
      { char v[64]; errno = 0;
        getxattr ("/nix", "user.fc-nonexistent", v, sizeof v);
        int ok = (errno != ENOENT) == expect;
        printf ("  getxattr(\"/nix\") errno=%-3d    %s\n", errno, ok ? "ok" : "MISMATCH");
        if (!ok) fails++; } }

  /* getcwd must NARROW the result back to chroot space -- that is the whole
     body of the wrapper.  realpath is a WHOLE-IMPLEMENTATION replacement (it
     never calls nextcall), working only because its internal getcwd/readlink/
     lstat calls reach our wrappers, so it is worth proving directly. */
  { if (!raw_exists ("/nix"))
      { char cwd[PATH_MAX];
        int ok = chdir ("/nix") == 0 && getcwd (cwd, sizeof cwd)
                 && (!strcmp (cwd, "/nix") == expect);
        printf ("  getcwd narrows to /nix       %s\n", ok ? "ok" : "MISMATCH");
        if (!ok) fails++;
        char *rp = realpath ("/nix/../nix", NULL);
        ok = rp && (!strcmp (rp, "/nix") == expect);
        printf ("  realpath(\"/nix/../nix\")      %s\n", ok ? "ok" : "MISMATCH");
        if (!ok) fails++;
        free (rp); } }
  printf("%s\n",fails?"BEHAVIOURAL FAILURES":"behaviour as expected");
  return fails!=0;
}
