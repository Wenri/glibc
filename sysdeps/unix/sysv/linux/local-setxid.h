/* SETxID functions which only have to change the local thread and
   none of the possible other threads.  */
#include <sysdep.h>
#include <fakesyscall.h>

#ifdef __NR_setresuid32
# define local_seteuid(id) syscall (__NR_setresuid32, 3, -1, id, -1)
#else
# define local_seteuid(id) syscall (__NR_setresuid, 3, -1, id, -1)
#endif


#ifdef __NR_setresgid32
# define local_setegid(id) syscall (__NR_setresgid32, 3, -1, id, -1)
#else
# define local_setegid(id) syscall (__NR_setresgid, 3, -1, id, -1)
#endif
