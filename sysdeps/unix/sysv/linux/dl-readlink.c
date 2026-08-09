/* Same as dl-access.c.  The __readlink reference is the fork's OWN: it comes
   from elf/dl-android-paths.h:225, included by elf/dl-load.c.  ld.so does its
   path translation there, so it must reach the raw readlink.  */
#include "readlink.c"
