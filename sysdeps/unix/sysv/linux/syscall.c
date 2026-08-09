#ifndef WITHOUT_FAKESYSCALL
# include <fakesyscall-base.h>
#else
# include <unistd.h>
#endif
#include <stdarg.h>

/* Deliberately AFTER the includes: <fakesyscall-base.h> pulls in
   fake_epoll_pwait2.c and friends, and renaming the token before that would
   reach into them.  Precondition 9 does not bite here -- the body calls
   syscallS(), not INLINE_SYSCALL_CALL (syscall, ...), so no __NR_ token is
   pasted from the function name and none needs pointing back.

   This is the LOWER half of a two-layer stack.  fakechroot's wrapper becomes
   the public `syscall' and handles path expansion, the no-op/blocked
   categories and its redirect table; what it does not claim falls through to
   here, where DISABLED_SYSCALL_WITH_FAKESYSCALL substitutes a libc call for
   each syscall whose __NR_ was deleted; what THAT does not claim reaches
   syscallS and the kernel.  Same order the LD_PRELOAD build has today.  */
#if !IS_IN (rtld)
# define syscall __android_next_syscall
#endif

long int
syscall (long int number, ...)
{
  va_list args;
  va_start (args, number);
  long int a0 = va_arg (args, long int);
  long int a1 = va_arg (args, long int);
  long int a2 = va_arg (args, long int);
  long int a3 = va_arg (args, long int);
  long int a4 = va_arg (args, long int);
  long int a5 = va_arg (args, long int);
  va_end (args);

#ifndef WITHOUT_FAKESYSCALL
  switch (number)
#endif
  {
#ifndef WITHOUT_FAKESYSCALL
    DISABLED_SYSCALL_WITH_FAKESYSCALL
    default:
#endif
      return syscallS (number, a0, a1, a2, a3, a4, a5);
  }
}
