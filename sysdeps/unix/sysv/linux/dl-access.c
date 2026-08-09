/* ld.so needs __access (elf/rtld.c, for /etc/ld-nix.so.preload) but must NOT get
   the android wrapper: pulling it out of libc_pic.a drags the whole
   fakechroot support layer, stdio and malloc into the loader.  Give ld.so its
   own untranslated copy, exactly as dl-getcwd/dl-opendir already do.  This unit
   compiles with MODULE_NAME=rtld, so access.c's `#if !IS_IN (rtld)' skips the
   rename and its `#if IS_IN (rtld)' re-emits __access/access for the loader --
   which is the ONLY thing defining them here.  */
#include "access.c"
