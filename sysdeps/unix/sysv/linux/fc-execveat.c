/* android wrapper shim for execveat.  Why shims exist and every wiring
   precondition: nix-on-droid/docs/ANDROID-GLIBC.md.

   There is no vendor android/execveat.c -- upstream fakechroot never wrapped
   this one, because under LD_PRELOAD an unwrapped execveat simply escaped
   interception and nobody noticed.  Compiled into libc it is worse than that:
   execveat is exported (posix/Versions:156), takes a real path, and had no
   wrapper, so it handed UNTRANSLATED paths straight to the kernel.  That is the
   same class of bug wiring execve fixed, and the same reason it mattered --
   /nix does not exist at the real root.

   The `strong' variant.  Precondition 8 is clean, exactly as for execve:
   include/unistd.h:117 declares __execveat with plain attribute_hidden and no
   libc_hidden_proto, so every C-level alias lands where it should and no asm
   remedy is needed.  Public binding is GLOBAL rather than execve's WEAK,
   because linux/execveat.c defines the name bare with no weak_alias to
   suppress -- confirmed against the reference:

     readelf --dyn-syms libc.so.6 | grep execveat   ->  GLOBAL execveat@@GLIBC_2.34

   Hand-written rather than generated only because the generator's job is to
   emit `#include "android/<f>.c"', and there is no such file to include.  */

/* config.h FIRST -- it is where ANDROID_BASE comes from.  Then the strong
   opt-in, which wrapper.h reads when it decides the public alias.  */
#include <config.h>
#define FC_PUBLIC_STRONG 1
#include "android/wrapper.h"

#include <fcntl.h>
#include <unistd.h>

wrapper(execveat, int, (int dirfd, const char * path, char *const argv[],
                        char *const envp[], int flags))
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("execveat(%d, \"%s\", &argv, &envp, %d)", dirfd, path, flags);

    /* AT_EMPTY_PATH means PATH is "" and DIRFD itself names the file.  There is
       nothing to translate, and expanding "" would hand the kernel the base
       directory instead of an empty string.  */
    if (!(flags & AT_EMPTY_PATH)) {
        path = expand_chroot_path_at(dirfd, path, fakechroot_buf);
        if (path == NULL) {
            return -1;
        }
    }

    /* Deliberately NOT routed through __fc_execve.  That wrapper reads the file
       header and rebuilds argv to run scripts and unpatched ELFs under the
       elfloader, which cannot be expressed here: it takes a path, not a
       dirfd+path+flags triple, so anything relative to DIRFD or carrying
       AT_SYMLINK_NOFOLLOW would have to be flattened first and would then no
       longer mean what the caller asked for.  Translating the path is the fix
       for the gap this shim exists to close; execveat on a script or an
       unpatched binary still fails, as it does today, and is recorded under
       "Known limitations" rather than papered over.  */
    return nextcall(execveat)(dirfd, path, argv, envp, flags);
}
