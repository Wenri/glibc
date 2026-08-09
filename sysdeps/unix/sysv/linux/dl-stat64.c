/* ld.so needs __stat64 but must NOT get the android wrapper
   (nix-on-droid/docs/ANDROID-GLIBC.md precondition 4).  Referenced by elf/dl-load.c:1833 via the __stat64_time64 macro.

   Safe to add even if rtld turns out not to need it: this family is RENAMED,
   so stat64.os defines none of these names and cannot collide with this copy --
   unlike the suppress-only open family, where a needless dl-open64.c
   duplicated __libc_open64.  This unit compiles with MODULE_NAME=rtld, so the
   source's IS_IN (rtld) arms skip the rename and re-emit the names.  */
#include "stat64.c"
