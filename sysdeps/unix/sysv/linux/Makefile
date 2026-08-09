# Option to pass to Python scripts to set the C compiler.  Rewriting
# MODULE_NAME is required to enable the _ISOMAC verbatim header
# environment.
sysdeps-linux-python-cc = \
  --cc="$(CC) $(patsubst -DMODULE_NAME=%,-DMODULE_NAME=testsuite,$(CPPFLAGS))"

# Additional dependencies for Python scripts.
sysdeps-linux-python-deps = \
  $(..)scripts/glibcextract.py \
  $(..)sysdeps/unix/sysv/linux/glibcsyscalls.py \
  # sysdeps-linux-python-deps

# Invocation of the Python interpreter with the Python search path.
sysdeps-linux-python = \
  PYTHONPATH=$(..)scripts:$(..)sysdeps/unix/sysv/linux $(PYTHON)

ifndef subdir
# This target performs two actions:
#
# Replace <arch-syscall.h> with a file generated from kernel headers
# and <fixup-asm-unistd.h>.  Both files are located via the sysdeps
# override search path.
#
# Update sysdeps/unix/sysv/linux/syscall-names.list with additional
# names found in the generated <arch-syscall.h> file, so that the
# global system call names list is a superset of the
# architecture-specific system call names.
#
# To bootstrap a new architecture, create an empty file in the right
# place and run `make update-syscall-lists' from the top-level of a
# configured, but not-yet-built glibc tree.
#
# --lock points to a file not replaced during the update operation, so
# that mutual exclusion is achieved.
.PHONY: update-syscall-lists
update-syscall-lists: arch-syscall.h
	$(sysdeps-linux-python) \
	  sysdeps/unix/sysv/linux/update-syscall-lists.py \
	    $(sysdeps-linux-python-cc) \
	    --lock=sysdeps/unix/sysv/linux/update-syscall-lists.py $< \
	    sysdeps/unix/sysv/linux/syscall-names.list
endif

ifeq ($(subdir),csu)
sysdep_routines += \
  errno-loc \
  libc-dl-sigsys \
  # sysdep_routines

# The SIGSYS handler goes into libc.a only.  A dynamic process gets it from
# ld.so (sysdep-rtld-routines, below); a static one has no ld.so and no way to
# be given one -- rtld_chain_load re-execs it -- so it needs its own copy, and
# the install point is csu/libc-start.c's #ifndef SHARED arm.  Keeping it out of
# libc.so.6 is what guarantees a dynamic process has exactly one copy of the
# chain state; two would make sigaction chaining depend on link order.
static-only-routines += libc-dl-sigsys
endif

ifeq ($(subdir),assert)
CFLAGS-assert.c += -DFATAL_PREPARE_INCLUDE='<fatal-prepare.h>'
CFLAGS-assert-perr.c += -DFATAL_PREPARE_INCLUDE='<fatal-prepare.h>'
endif

ifeq ($(subdir),malloc)
CFLAGS-malloc.c += -DMORECORE_CLEARS=2
endif

ifeq ($(subdir),misc)
sysdep_routines += \
  adjtimex \
  clock_adjtime \
  clone \
  clone-internal \
  clone-pidfd-support \
  clone3 \
  closefrom_fallback \
  convert_scm_timestamps \
  epoll_create \
  epoll_pwait \
  epoll_pwait2 \
  epoll_wait \
  eventfd \
  eventfd_read \
  eventfd_write \
  fanotify_mark \
  fxstat \
  fxstat64 \
  fxstatat \
  fxstatat64 \
  inotify_init \
  lxstat \
  lxstat64 \
  mlock2 \
  mremap \
  open_by_handle_at \
  personality \
  pkey_get \
  pkey_mprotect \
  pkey_set \
  prctl \
  prlimit \
  prlimit64 \
  process_vm_readv \
  process_vm_writev \
  pselect32 \
  readahead \
  sched_getattr \
  sched_setattr \
  setfsgid \
  setfsuid \
  setvmaname \
  signalfd \
  splice \
  sysctl \
  tee \
  timerfd_gettime \
  timerfd_settime \
  umount \
  umount2 \
  vmsplice \
  xmknod \
  xmknodat \
  xstat \
  xstat64 \
  # sysdep_routines

CFLAGS-gethostid.c = -fexceptions
CFLAGS-tee.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-vmsplice.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-splice.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-open_by_handle_at.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-sync_file_range.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-pselect32.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-tst-writev.c += "-DARTIFICIAL_LIMIT=(0x80000000-sysconf(_SC_PAGESIZE))"

# The test triggers FORTIFY abortion due to purposely wrong parameters
CFLAGS-test-errno-linux.c += $(no-fortify-source)

sysdep_headers += \
  bits/a.out.h \
  bits/cloexec.h \
  bits/epoll.h \
  bits/eventfd.h \
  bits/inotify.h \
  bits/mman-linux.h \
  bits/mman-map-flags-generic.h \
  bits/mman-shared.h \
  bits/procfs-extra.h \
  bits/procfs-id.h \
  bits/procfs-prregset.h \
  bits/procfs.h \
  bits/pthread_stack_min-dynamic.h \
  bits/pthread_stack_min.h \
  bits/ptrace-shared.h \
  bits/rseq.h \
  bits/shmlba.h \
  bits/siginfo-arch.h \
  bits/siginfo-consts-arch.h \
  bits/signalfd.h \
  bits/socket_type.h \
  bits/struct_stat.h \
  bits/struct_stat_time64_helper.h \
  bits/syscall.h \
  bits/timerfd.h \
  bits/types/struct_msqid64_ds.h \
  bits/types/struct_msqid64_ds_helper.h \
  bits/types/struct_semid64_ds.h \
  bits/types/struct_semid64_ds_helper.h \
  bits/types/struct_semid_ds.h \
  bits/types/struct_shmid64_ds.h \
  bits/types/struct_shmid64_ds_helper.h \
  scsi/scsi.h \
  scsi/scsi_ioctl.h \
  scsi/sg.h \
  sys/acct.h \
  sys/epoll.h \
  sys/eventfd.h \
  sys/fanotify.h \
  sys/fsuid.h \
  sys/inotify.h \
  sys/kd.h \
  sys/klog.h \
  sys/mount.h \
  sys/pci.h \
  sys/personality.h \
  sys/pidfd.h \
  sys/prctl.h \
  sys/quota.h \
  sys/raw.h \
  sys/rseq.h \
  sys/signalfd.h \
  sys/soundcard.h \
  sys/timerfd.h \
  sys/user.h \
  sys/vt.h \
  # sysdep_headers

tests += \
  test-errno-linux \
  tst-adjtimex \
  tst-clock_adjtime \
  tst-clone \
  tst-clone2 \
  tst-clone3 \
  tst-epoll \
  tst-epoll-ioctls \
  tst-fanotify \
  tst-fdopendir-o_path \
  tst-getauxval \
  tst-gettid \
  tst-gettid-kill \
  tst-linux-mremap1 \
  tst-memfd_create \
  tst-misalign-clone \
  tst-mlock2 \
  tst-mount \
  tst-ntp_adjtime \
  tst-ntp_gettime \
  tst-ntp_gettimex \
  tst-ofdlocks \
  tst-personality \
  tst-pidfd \
  tst-pidfd_getpid \
  tst-pkey \
  tst-ppoll \
  tst-prctl \
  tst-process_mrelease \
  tst-quota \
  tst-rlimit-infinity \
  tst-sched-affinity-inheritance \
  tst-sched_setattr \
  tst-sched_setattr-thread \
  tst-scm_rights \
  tst-sigtimedwait \
  tst-sync_file_range \
  tst-syscall-restart \
  tst-sysconf-iov_max \
  tst-sysvmsg-linux \
  tst-sysvsem-linux \
  tst-sysvshm-linux \
  tst-tgkill \
  tst-timerfd \
  tst-ttyname-direct \
  tst-ttyname-namespace \
  # tests

# process_madvise requires CAP_SYS_ADMIN.
xtests += \
  tst-process_madvise \
  # xtests

# For +depfiles in Makerules.
extra-test-objs += \
  tst-sysconf-iov_max-uapi.o \
  # extra-test-objs

# Test for the symbol version of fcntl that was replaced in glibc 2.28.
ifeq ($(have-GLIBC_2.27)$(build-shared),yesyes)
tests += \
  tst-ofdlocks-compat \
  # tests
endif

tests-internal += \
  tst-rseq \
  tst-sigcontext-get_pc \
  # tests-internal

tests-internal += \
  tst-rseq-disable \
  # tests-internal

tests-static += \
  tst-rseq-disable-static \
  tst-rseq-static \
  # tests-static

tests-time64 += \
  tst-adjtimex-time64 \
  tst-clock_adjtime-time64 \
  tst-epoll-time64 \
  tst-ntp_adjtime-time64 \
  tst-ntp_gettime-time64 \
  tst-ntp_gettimex-time64 \
  tst-ppoll-time64 \
  tst-prctl-time64 \
  tst-scm_rights-time64 \
  tst-sigtimedwait-time64 \
  tst-timerfd-time64 \
  # tests-time64

tests-clone-internal = \
  tst-align-clone-internal \
  tst-clone2-internal \
  tst-clone3-internal \
  tst-getpid1-internal
  # tests-clone-internal
tests-internal += \
  $(tests-clone-internal) \
  # tests-internal
tests-static += \
  $(tests-clone-internal) \
  # tests-static

CFLAGS-tst-sigcontext-get_pc.c = -fasynchronous-unwind-tables

# Generate the list of SYS_* macros for the system calls (__NR_*
# macros).  The file syscall-names.list contains all possible system
# call names, and the generated header file produces SYS_* macros for
# the __NR_* macros which are actually defined.

generated += bits/syscall.h
$(objpfx)bits/syscall.h: \
  ../sysdeps/unix/sysv/linux/gen-syscall-h.awk \
  ../sysdeps/unix/sysv/linux/syscall-names.list
	$(make-target-directory)
	LC_ALL=C $(AWK) -f $^ > $@-tmp
	$(move-if-change) $@-tmp $@
before-compile += $(objpfx)bits/syscall.h

# All macros defined by <sys/syscall.h>.  Include <bits/syscall.h>
# explicitly because <sys/sycall.h> skips it if _LIBC is defined.
$(objpfx)tst-syscall-list-macros.list: \
  $(objpfx)bits/syscall.h ../sysdeps/unix/sysv/linux/sys/syscall.h
	printf '#include <linux/version.h>\n\
#include <sys/syscall.h>\n#include <bits/syscall.h>\n' | \
	  $(CC) -E -o $@-tmp $(CFLAGS) $(CPPFLAGS) -x c - -dM
	$(move-if-change) $@-tmp $@

# __NR_* system call names.  Used by the test below.
$(objpfx)tst-syscall-list-nr.list: \
  ../sysdeps/unix/sysv/linux/filter-nr-syscalls.awk \
  $(objpfx)tst-syscall-list-macros.list
	LC_ALL=C $(AWK) -f $^ > $@-tmp
	$(move-if-change) $@-tmp $@

# SYS_* system call names.  Used by the test below.
$(objpfx)tst-syscall-list-sys.list: $(objpfx)tst-syscall-list-macros.list
	LC_ALL=C $(AWK) '/^#define SYS_/ { print substr($$2, 5) }' $< > $@-tmp
	$(move-if-change) $@-tmp $@

tests-special += \
  $(objpfx)tst-syscall-list.out
  # tests-special
$(objpfx)tst-syscall-list.out: \
  ../sysdeps/unix/sysv/linux/tst-syscall-list.sh \
  $(objpfx)tst-syscall-list-macros.list \
  $(objpfx)tst-syscall-list-nr.list \
  $(objpfx)tst-syscall-list-sys.list
	$(BASH) $^ $(AWK) > $@; $(evaluate-test)

tests-special += \
  $(objpfx)tst-glibcsyscalls.out
  # tests-special
# arch-syscall.h is located via the sysdeps override search path.
$(objpfx)tst-glibcsyscalls.out: arch-syscall.h \
  ../sysdeps/unix/sysv/linux/syscall-names.list
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-glibcsyscalls.py \
	    $(sysdeps-linux-python-cc) $< \
	    ../sysdeps/unix/sysv/linux/syscall-names.list \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-glibcsyscalls.out: \
  ../sysdeps/unix/sysv/linux/tst-glibcsyscalls.py \
  $(sysdeps-linux-python-deps)

# Separate object file for access to the constant from the UAPI header.
$(objpfx)tst-sysconf-iov_max: $(objpfx)tst-sysconf-iov_max-uapi.o

tests-special += \
  $(objpfx)tst-mman-consts.out \
  # tests-special
$(objpfx)tst-mman-consts.out: ../sysdeps/unix/sysv/linux/tst-mman-consts.py
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-mman-consts.py \
	    $(sysdeps-linux-python-cc) \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-mman-consts.out: $(sysdeps-linux-python-deps)

tests-special += \
  $(objpfx)tst-pidfd-consts.out \
  # tests-special
$(objpfx)tst-pidfd-consts.out: ../sysdeps/unix/sysv/linux/tst-pidfd-consts.py
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-pidfd-consts.py \
	    $(sysdeps-linux-python-cc) \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-pidfd-consts.out: $(sysdeps-linux-python-deps)

tests-special += \
  $(objpfx)tst-mount-consts.out \
  # tests-special
$(objpfx)tst-mount-consts.out: ../sysdeps/unix/sysv/linux/tst-mount-consts.py
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-mount-consts.py \
	    $(sysdeps-linux-python-cc) \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-mount-consts.out: $(sysdeps-linux-python-deps)

tests-special += \
  $(objpfx)tst-mount-compile.out \
  # tests-special
$(objpfx)tst-mount-compile.out: ../sysdeps/unix/sysv/linux/tst-mount-compile.py
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-mount-compile.py \
	    $(sysdeps-linux-python-cc) \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-mount-compile.out: $(sysdeps-linux-python-deps)

tests-special += \
  $(objpfx)tst-sched-consts.out \
  # tests-special
$(objpfx)tst-sched-consts.out: ../sysdeps/unix/sysv/linux/tst-sched-consts.py
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-sched-consts.py \
	    $(sysdeps-linux-python-cc) \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-sched-consts.out: $(sysdeps-linux-python-deps)

tst-rseq-disable-ENV = GLIBC_TUNABLES=glibc.pthread.rseq=0
tst-rseq-disable-static-ENV = GLIBC_TUNABLES=glibc.pthread.rseq=0

endif # $(subdir) == misc

ifeq ($(subdir),termios)
sysdep_headers += \
  bits/termios-c_cc.h \
  bits/termios-c_cflag.h \
  bits/termios-c_iflag.h \
  bits/termios-c_lflag.h \
  bits/termios-c_oflag.h \
  bits/termios-cbaud.h \
  bits/termios-misc.h \
  bits/termios-struct.h \
  bits/termios-tcflow.h \
  # sysdep_headers

tests += \
  tst-termios-linux \
  # tests
endif

ifeq ($(subdir),time)
sysdep_headers += \
  bits/timex.h \
  sys/timex.h \
  # sysdep_headers

sysdep_routines += \
  ntp_gettime \
  ntp_gettimex \
  # sysdep_routines

tests += \
  tst-clock_gettime-clobber \
  tst-gettimeofday-clobber \
  tst-time-clobber \
  # tests
endif

ifeq ($(subdir),signal)
tests-special += \
  $(objpfx)tst-signal-numbers.out \
  # tests-special

# Depending on signal.o* is a hack.  What we actually want is a dependency
# on signal.h and everything it includes.  That's impractical to write
# in this context, but signal.c includes signal.h and not much else so it'll
# be conservatively correct.
$(objpfx)tst-signal-numbers.out: \
		../sysdeps/unix/sysv/linux/tst-signal-numbers.py \
		$(objpfx)signal.o*
	$(sysdeps-linux-python) \
	  ../sysdeps/unix/sysv/linux/tst-signal-numbers.py \
	    $(sysdeps-linux-python-cc) \
	  < /dev/null > $@ 2>&1; $(evaluate-test)
$(objpfx)tst-signal-numbers.out: $(sysdeps-linux-python-deps)
endif

ifeq ($(subdir),socket)
sysdep_headers += \
  bits/socket-constants.h \
  net/ethernet.h \
  net/if_arp.h \
  net/if_packet.h \
  net/if_ppp.h \
  net/if_shaper.h \
  net/if_slip.h \
  net/ppp-comp.h \
  net/ppp_defs.h \
  net/route.h \
  # sysdep_headers

sysdep_routines += \
  cmsg_nxthdr \
  # sysdep_routines

CFLAGS-recvmmsg.c = -fexceptions -fasynchronous-unwind-tables
CFLAGS-sendmmsg.c = -fexceptions -fasynchronous-unwind-tables

tests += \
  tst-socket-timestamp \
  tst-socket-timestamp-compat \
  # tests
tests-time64 += \
  tst-socket-timestamp-compat-time64 \
  tst-socket-timestamp-time64 \
  # tests-time64

tests-special += \
  $(objpfx)tst-socket-consts.out \
  # tests-special
$(objpfx)tst-socket-consts.out: ../sysdeps/unix/sysv/linux/tst-socket-consts.py
	PYTHONPATH=../scripts \
	$(PYTHON) ../sysdeps/unix/sysv/linux/tst-socket-consts.py \
		   --cc="$(CC) $(patsubst -DMODULE_NAME=%, \
					  -DMODULE_NAME=testsuite, \
					  $(CPPFLAGS)) -D_ISOMAC" \
	< /dev/null > $@ 2>&1; $(evaluate-test)
endif # $(subdir) == socket

ifeq ($(subdir),sunrpc)
sysdep_headers += \
  nfs/nfs.h \
  # sysdep_headers
endif

ifeq ($(subdir),posix)
sysdep_headers += \
  bits/initspin.h \
  # sysdep_headers

sysdep_routines += \
  getcpu \
  oldglob \
  pidfd_getpid \
  pidfd_spawn \
  pidfd_spawnp \
  procutils \
  sched_getcpu \
  spawnattr_getcgroup_np \
  spawnattr_setcgroup_np \
  # sysdep_routines

tests += \
  tst-affinity \
  tst-affinity-pid \
  tst-posix_spawn-setsid-pidfd \
  tst-spawn-cgroup \
  tst-spawn-chdir-pidfd \
  tst-spawn-pidfd \
  tst-spawn2-pidfd \
  tst-spawn3-pidfd \
  tst-spawn4-pidfd \
  tst-spawn5-pidfd \
  tst-spawn6-pidfd \
  tst-spawn7-pidfd \
  # tests

tests-static += \
  tst-affinity-static \
  # tests-static

tests += \
  $(tests-static) \
  # tests

CFLAGS-fork.c = $(libio-mtsafe)
CFLAGS-getpid.o = -fomit-frame-pointer
CFLAGS-getpid.os = -fomit-frame-pointer
CFLAGS-tst-spawn3-pidfd.c += -DOBJPFX=\"$(objpfx)\"

tst-spawn-cgroup-ARGS = -- $(host-test-program-cmd)
tst-spawn-pidfd-ARGS = -- $(host-test-program-cmd)
tst-spawn5-pidfd-ARGS = -- $(host-test-program-cmd)
tst-spawn6-pidfd-ARGS = -- $(host-test-program-cmd)
tst-spawn7-pidfd-ARGS = -- $(host-test-program-cmd)
tst-posix_spawn-setsid-pidfd-ARGS = -- $(host-test-program-cmd)
endif

ifeq ($(subdir),inet)
sysdep_headers += \
  netash/ash.h \
  netatalk/at.h \
  netax25/ax25.h \
  neteconet/ec.h \
  netinet/if_fddi.h \
  netinet/if_tr.h \
  netipx/ipx.h \
  netiucv/iucv.h \
  netpacket/packet.h \
  netrom/netrom.h \
  netrose/rose.h \
  # sysdep_headers

sysdep_routines += \
  netlink_assert_response \
  # sysdep_routines

CFLAGS-check_pf.c += -fexceptions
endif

# Don't compile the ctype glue code, since there is no old non-GNU C library.
inhibit-glue = yes

ifeq ($(subdir),dirent)
sysdep_routines += \
  getdirentries \
  getdirentries64 \
  # sysdep_routines

tests += \
  tst-getdents64 \
  tst-readdir64-compat \
  # tests
endif # $(subdir) == dirent

ifeq ($(subdir),nis)
CFLAGS-ypclnt.c = -DUSE_BINDINGDIR=1
endif

ifeq ($(subdir),io)
sysdep_routines += \
  close_nocancel \
  close_nocancel_nostatus \
  fallocate \
  fallocate64 \
  fcntl_nocancel \
  internal_statvfs \
  open64_nocancel \
  open_nocancel \
  openat64_nocancel \
  openat_nocancel \
  pread64_nocancel \
  read_nocancel \
  stat_t64_cp \
  statx_cp \
  sync_file_range \
  write_nocancel \
  xstatconv \
  # sysdep_routines

sysdep_headers += \
  bits/fcntl-linux.h \
  # sysdep_headers

tests += \
  tst-fallocate \
  tst-fallocate64 \
  tst-getcwd-smallbuff \
  tst-o_path-locks \
  # tests
endif

ifeq ($(subdir),elf)
dl-routines += \
  dl-rseq-symbols \
  # dl-routines

sysdep-rtld-routines += \
  dl-brk \
  dl-getcwd \
  dl-openat64 \
  dl-opendir \
  dl-sbrk \
  dl-access \
  dl-readlink \
  dl-stat64 \
  dl-lstat64 \
  dl-fstatat64 \
  dl-sigsys \
  # sysdep-rtld-routines

others += \
  pldd \
  # others

install-bin += \
  pldd \
  # install-bin

$(objpfx)pldd: $(objpfx)xmalloc.o

tests += tst-rseq-tls-range tst-rseq-tls-range-4096
tests-static += tst-rseq-tls-range-static tst-rseq-tls-range-4096-static
modules-names += tst-rseq-tls-range-mod
CFLAGS-tst-rseq-tls-range.c += -DMAIN_TLS_ALIGN=4
CFLAGS-tst-rseq-tls-range-4096.c += -DMAIN_TLS_ALIGN=4096
CFLAGS-tst-rseq-tls-range-static.c += -DMAIN_TLS_ALIGN=4
CFLAGS-tst-rseq-tls-range-4096-static.c += -DMAIN_TLS_ALIGN=4096
$(objpfx)tst-rseq-tls-range.out: $(objpfx)tst-rseq-tls-range-mod.so
$(objpfx)tst-rseq-tls-range-4096.out: $(objpfx)tst-rseq-tls-range-mod.so
$(objpfx)tst-rseq-tls-range-static.out: $(objpfx)tst-rseq-tls-range-mod.so
$(objpfx)tst-rseq-tls-range-4096-static.out: $(objpfx)tst-rseq-tls-range-mod.so
tst-rseq-tls-range-static-ENV = LD_LIBRARY_PATH=$(objpfx):$(common-objpfx)
tst-rseq-tls-range-4096-static-ENV = LD_LIBRARY_PATH=$(objpfx):$(common-objpfx)

test-internal-extras += tst-nolink-libc
ifeq ($(run-built-tests),yes)
tests-special += \
  $(objpfx)tst-nolink-libc-1.out \
  $(objpfx)tst-nolink-libc-2.out \
  # tests-special
endif
endif # $(subdir) == elf

ifeq ($(subdir),rt)
CFLAGS-mq_send.c += -fexceptions
CFLAGS-mq_receive.c += -fexceptions
endif

ifeq ($(subdir),nscd)
sysdep-CFLAGS += -DHAVE_EPOLL -DHAVE_INOTIFY -DHAVE_NETLINK
CFLAGS-gai.c += -DNEED_NETLINK
endif

ifeq ($(subdir),nptl)
tests += \
  tst-align-clone \
  tst-getpid1 \
  tst-sem_getvalue-affinity \
  # tests

# tst-rseq-nptl is an internal test because it requires a definition of
# __NR_rseq from the internal system call list.
tests-internal += \
  tst-rseq-nptl \
  # tests-internal

tests-static += \
  tst-rseq-nptl-static \
  # tests-static
endif

# ---------------------------------------------------------------------------
# android: path virtualisation absorbed from the LD_PRELOAD library.
# Sources are a byte-for-byte vendor import under android/ (see
# nix-on-droid/docs/ANDROID-GLIBC.md); the fc-*.c files here are the build-visible
# wrappers.  Every build-visible source is fc-PREFIXED and lives here, NOT in
# the module directory, and there is deliberately NO vpath: the module holds 163
# files named after libc functions (getcwd.c, opendir.c, ...), so a vpath makes
# them silently SHADOW glibc's own sources of the same name.  That is invisible
# until a shadowed routine happens to be wired -- it first showed up as
# duplicate __getcwd/_dl_start/malloc in the rtld link.
# Each glibc implementation whose name a wrapper takes over renames itself to
# __android_next_<f> and suppresses its public alias, both under `IS_IN (rtld)'
# in its own source -- e.g. mkdir.c's weak_alias (__mkdir, mkdir).  ld.so is the
# only build that keeps the original names, which is what the seven dl-*.c twins
# rely on.  This used to be a -DANDROID_TRANSLATE keyed off the tables below;
# the flag was redundant with IS_IN (rtld) and is gone.
# ---------------------------------------------------------------------------
# Wiring is table-driven, and everything below the tables is UNCONDITIONAL --
# deliberately, because both silent-failure modes this port has hit were a
# per-subdir `ifeq' scoping mistake:
#
#   1. bind/getpeername/getsockname were listed in the io block, but glibc
#      builds them in `socket'.  sysdep_routines and CFLAGS-<file> only apply
#      inside the ifeq for the subdir that compiles the file, so the sources
#      kept their own __bind/__getsockname and the fc-*.os were built but never
#      linked.  Green build, wrappers absent, nothing warned.
#
#   2. After moving them to a socket block, `android-inc' was still DEFINED
#      in the io block, so it expanded to empty for socket.  The imported
#      sources' `#include <config.h>' then picked up glibc's OWN build config.h,
#      leaving HAVE_BIND undefined -- and 91 of the 158 imported files wrap
#      their whole body in #ifdef HAVE_<FUNC>, so the file compiled to nothing.
#
# So: name each function ONCE, in the list for the subdir glibc compiles it in
# (grep a build log for `-o .../<subdir>/<f>.os' to find out).  The subdir still
# matters for LINKING -- sysdep_routines below only takes effect in the subdir
# that compiles the file, so a name in the wrong list builds its fc-<f>.os and
# never links it.  It no longer affects the RENAME, which each glibc source now
# does for itself under IS_IN (rtld).

# Wrappers: the glibc source of the same name renames itself out of the way and
# suppresses its public alias, so the fc- wrapper can take the name over.
android-wrappers-io     := mkdir access chmod chown lchown link readlink \
                              rmdir symlink unlink \
                              faccessat mknodat \
                              utimensat mknod utime \
                              lchmod mkfifo mkfifoat getwd get_current_dir_name statx \
                              fchmodat \
                              open open64 openat openat64 \
                              chdir getcwd \
                              fchownat linkat mkdirat readlinkat \
                              symlinkat unlinkat \
                              stat stat64 lstat lstat64 fstatat fstatat64 \
                              creat creat64 statfs statfs64 statvfs statvfs64 \
                              eaccess euidaccess
android-wrappers-socket := bind getpeername getsockname connect
# renameat/renameat2 are NOT io: glibc compiles them in stdio-common, next to
# rename.  Reading the name is not enough -- check the build log.
android-wrappers-stdio-common := renameat renameat2 rename tempnam tmpnam remove
android-wrappers-misc   := futimesat lutimes utimes revoke mktemp mkdtemp \
                              acct chroot \
                              setxattr lsetxattr getxattr lgetxattr \
                              listxattr llistxattr removexattr lremovexattr \
                              inotify_add_watch \
                              truncate truncate64 \
                              mkstemp mkstemp64 mkostemp mkostemp64 \
                              mkstemps mkstemps64 mkostemps mkostemps64 \
                              syscall
android-wrappers-dirent := opendir scandir scandir64 scandirat64
android-wrappers-posix  := pathconf glob_pattern_p execv \
                              execl execle execlp execvp execve execveat \
                              glob glob_lstat_compat
android-wrappers-stdlib := canonicalize_file_name system clearenv realpath
android-wrappers-nss    := lckpwdf ulckpwdf
android-wrappers-intl   := bindtextdomain
android-wrappers-libio  := freopen freopen64 fopen fopen64

# Functions glibc GENERATES from sysdeps/unix/sysv/linux/syscalls.list.  Writing
# a .c is not enough for these: make-syscalls.sh:44-48 truncates $(sysdirs) to
# dirs STRICTLY ABOVE the one holding the list, so a .c in the same directory
# does NOT suppress the entry and the generated stub still defines the symbol --
# colliding with the wrapper.  (chdir, acct and chroot are unaffected: their
# entries live in the lower-priority sysdeps/unix/syscalls.list, which our
# linux/*.c genuinely outranks.)
#
# Naming the base function in sysdep_routines gives its .c a rule that overrides
# the generated one.  glibc itself relies on this for setfsuid and setfsgid,
# which likewise have a linux/syscalls.list entry, a linux/*.c, and an entry
# here.  Expect "ignoring old recipe" warnings -- a working build already has
# twenty of them.
android-syscall-io   := fchownat linkat mkdirat readlinkat symlinkat unlinkat
android-syscall-misc := setxattr lsetxattr getxattr lgetxattr listxattr \
                           llistxattr removexattr lremovexattr inotify_add_watch
sysdep_routines += $(android-syscall-$(subdir))

# Support objects: compiled the same way, but they take over no glibc name, so
# no glibc source is renamed for them -- they only need the include path.
android-support-io      := libfakechroot rel2abs getcwd_real dedotdot \
                              rel2absat

android-wrappers := $(android-wrappers-$(subdir))
android-objs     := $(android-wrappers) $(android-support-$(subdir))

# Defined UNCONDITIONALLY, unlike android-inc below, because two files that need
# it live in subdirs with no android objects at all: catgets/open_catalog.c and
# login/utmpname.c are translated in place rather than wrapped.  android-inc is
# assigned inside the ifneq, so for those subdirs it expands to EMPTY -- which is
# silent-failure mode 2 from the comment above, where the imported sources'
# #include <config.h> picked up glibc's own and 91 files compiled to nothing.
# One definition, referenced twice, so the two cannot drift.
android-inc-path := -I$(..)sysdeps/unix/sysv/linux/android

ifneq ($(android-objs),)

# The imported sources are unmodified, so their `#include <config.h>' and
# `#include "wrapper.h"' must resolve inside the module directory.
# Scoped per-file: `config.h' is too generic a name to put on the global path.
#
# DO NOT add -I$(..)sysdeps/unix/sysv/linux here for the generated shims'
# `#include "android/<f>.c"'.  It looks harmless -- +sysdep-includes carries
# that same directory anyway -- but android-inc is on CFLAGS, which precedes
# CPPFLAGS, so it lands ahead of the -I$(..)sysdeps/unix/sysv/linux/include
# that +sysdep-includes emits FIRST.  <sys/syscall.h> then resolves to the
# PUBLIC linux/sys/syscall.h instead of glibc's internal include/ wrapper, and
# the build dies in whichever subdir has not yet generated misc/bits/syscall.h:
#
#   ../sysdeps/unix/sysv/linux/include/bits/syscall.h:3:10: fatal error:
#     misc/bits/syscall.h: No such file or directory
#
# The quoted include needs no help: it misses in $(objpfx), then walks the -I
# list to +sysdep-includes' own -I$(..)sysdeps/unix/sysv/linux, which is
# present for every TU in the tree and is ordered AFTER include/ on purpose.
android-inc := $(android-inc-path)

# android/syscall.h includes nine Boost.PP headers, so syscall.c and
# sigaction.c do not compile without them.  Boost.PP is header-only AND
# preprocessor-only: nothing is linked, no runtime dependency exists, and the
# ABI is unaffected -- it expands REDIRECT_SEQ into case labels.  Supplied by
# common/pkgs/android-glibc.nix as `make boost-pp-inc=<boost.dev>/include'; the
# glibc build is not otherwise Boost-aware, and it stays scoped to these objects
# rather than going on the global path.  -isystem, not -I, so Boost's own
# warnings are not glibc's problem under -Werror.
#
# No dependency cycle: common/pkgs/default.nix calls android-glibc.nix as a
# plain function and takes only `.glibc', so the overlay is never applied to the
# package set and boost builds against the stock nixpkgs glibc.  Verified: a
# binary-cache hit, 14.7 MiB.
ifneq ($(boost-pp-inc),)
android-inc += -isystem $(boost-pp-inc)
endif

# --- shims: 112 generated, 5 checked in ------------------------------------
# 112 wrappers + 5 support objects = 117, minus the 5 in android-shim-hand.
# (Was 106/111 until verify-fc.sh's table regex was corrected: [a-z]* never
# matched android-wrappers-stdio-common, so six wrappers were uncounted AND
# unverified by check 3.)
# verify-fc.sh check 5 asserts the checked-in set equals android-shim-hand and
# check 7 asserts these numbers match the tables -- they had gone stale twice
# before that check existed.
#
# A shim's whole job is to be the build-visible file that #includes a vendor
# body (the module is deliberately off the source path -- see the vpath note
# above).  For 106 of the 111 that is fully determined by the function name plus
# three table lookups -- binding, assert, asm -- so they are generated into
# $(objpfx) by android/gen-shim.sh rather than tracked.  THE INVARIANT IS THE
# POINT: a checked-in fc-*.c means something no table can express.
#
# The 5 below are that set, and NONE of them is the .globl/.set case: that used
# to require a checked-in file, but gen-shim.sh grew a fourth argument and the
# fourteen android-asm-* entries below are now generated like everything else.
# What is left cannot be generated for a different reason each time:
#
#   execve, syscall              real adaptations -- 62 and 197 lines of glue
#                                ahead of the vendor body
#   glob_lstat_compat            compat_symbol, and no vendor body to include
#   execveat, scandirat64        no vendor body AT ALL -- upstream fakechroot
#                                never wrapped either, so there is nothing for
#                                the generator to #include
android-shim-hand := execve syscall glob_lstat_compat execveat \
                     scandirat64

# The precondition-8 remedy.  These functions have a libc_hidden_proto in scope
# in the shim's TU, so every C-level alias lands on __GI_<sym> and the public
# name stops existing; it is emitted in assembly instead, below where the
# redirect can reach.
#
# <binding> <symbol>.  The target is always __GI_<symbol>, so there is no third
# column.  Neither of the two is derivable from the function name -- mktemp
# aliases __mktemp while its siblings alias the bare name, and statvfs aliases
# statvfs while its sibling statfs aliases __statfs -- which is exactly why this
# is a table.  Get a binding wrong and the public symbol changes from weak to
# global against the reference; verify-fc.sh check 3 is what catches that.
android-asm-connect  := weak  __connect
android-asm-execl    := globl execl
android-asm-execle   := globl execle
android-asm-execlp   := globl execlp
android-asm-execvp   := globl execvp
android-asm-fchmodat := globl fchmodat
android-asm-mktemp   := globl __mktemp
android-asm-open     := weak  __open
android-asm-open64   := weak  __open64
android-asm-openat   := weak  __openat
android-asm-openat64 := weak  __openat64
android-asm-statfs   := weak  __statfs
android-asm-statvfs  := weak  statvfs
android-asm-utime    := globl utime

# Which generated shims need a strong public alias.  Grouped by REASON, because
# the reason is what the per-file comments used to carry and is the only part a
# table would otherwise lose.  The rule is always the same -- the public alias
# must keep the binding the reference libc has, or a legitimate user override of
# e.g. mkdir becomes a duplicate-symbol error at static link.
#
# glibc GENERATES these from a syscalls.list as a bare name, and the reference
# exports them GLOBAL.
# Derived, not spelled out.  These are exactly the functions glibc generates
# from a syscalls.list (precondition 10), which is why they have no weak_alias
# to suppress and the reference exports them GLOBAL -- plus acct and chroot,
# which are hand-written but bare for the same reason.  Listing them twice let
# the two copies drift: adding a syscall wrapper to android-syscall-* and
# forgetting it here silently downgrades its binding from GLOBAL to WEAK, which
# no build-time check would catch and only a readelf diff would.  Verified
# identical to the previous explicit list, 17 names.
android-strong-generated := $(android-syscall-io) $(android-syscall-misc) \
                            acct chroot
# glibc defines these bare -- no weak_alias -- and the reference exports them
# GLOBAL (precondition 8 scope).
android-strong-bare      := execv freopen freopen64 get_current_dir_name \
                            getwd lchmod mkdtemp mkfifo mkfifoat rename \
                            statx tempnam tmpnam
# Bare as well, but the collapse is inverted: the 64 partner is the weak alias
# rather than the other way round.
android-strong-64partner := mkstemp mkstemps mkostemp mkostemps
# fopen is GLOBAL because it comes from a versioned_symbol, not a weak_alias.
android-strong-versioned := fopen
# `remove' has a libc_hidden_proto (include/stdio.h) but the vendor file
# includes only <config.h>, so it is NOT in scope here: the plain C alias does
# define `remove', and only the binding needs stating.  A binding argument, not
# a scope one -- which is why it is its own entry rather than folded above.
android-strong-scope     := remove

android-strong := $(android-strong-generated) $(android-strong-bare) \
                  $(android-strong-64partner) $(android-strong-versioned) \
                  $(android-strong-scope)
android-shim-gen := $(filter-out $(android-shim-hand),$(android-objs))

# Three guards, because a half-converted tree builds green.  The first two catch
# a name with nothing behind it; the third catches the conversion hazard -- a
# checked-in fc-<f>.c SILENTLY BEATS a generated one, since sysd-rules is
# -include'd (Makerules:369) before the $(objpfx)%.c rules (Makerules:413).
$(foreach f,$(android-shim-hand), \
  $(if $(filter $(f),$(android-objs)), \
    $(if $(wildcard $(..)sysdeps/unix/sysv/linux/fc-$(f).c),, \
      $(error android: fc-$(f).c is listed hand-written but does not exist))))
$(foreach f,$(android-shim-gen), \
  $(if $(wildcard $(..)sysdeps/unix/sysv/linux/android/$(f).c),, \
    $(error android: android/$(f).c does not exist (subdir $(subdir)))))
$(foreach f,$(android-shim-gen), \
  $(if $(wildcard $(..)sysdeps/unix/sysv/linux/fc-$(f).c), \
    $(error android: fc-$(f).c is checked in AND generated -- the checked-in \
	    copy wins silently; delete it or add $(f) to android-shim-hand)))

# Static pattern rule: one recipe per file, so -j is safe and touching one input
# regenerates one file.  A plain rule listing all 95 targets would run the
# recipe 95 times and race.  move-if-change keeps mtimes stable so a .d-driven
# rebuild does not cascade.  Same idiom as hurd/Makefile:174, nss/Makefile:459.
$(android-shim-gen:%=$(objpfx)fc-%.c): $(objpfx)fc-%.c: \
		$(..)sysdeps/unix/sysv/linux/android/gen-shim.sh \
		$(..)sysdeps/unix/sysv/linux/Makefile
	$(make-target-directory)
# noassert for support objects is STRUCTURAL, not an oversight: the assertion
# gen-shim.sh emits is `extern __typeof (__fc_<f>) __fc_<f>;', and a support
# object defines no __fc_<f> -- it takes over no glibc name -- so there is
# nothing to assert against.  Their exposure to precondition 6 is correspondingly
# small: libfakechroot.c, rel2abs.c and dedotdot.c carry no whole-body HAVE_
# guard at all, getcwd_real.c uses HAVE___LXSTAT64 as a branch selector with a
# live #else, and only rel2absat.c has a true body guard (HAVE_FCHDIR, which
# config.h defines).  If a support object ever grows a body guard on a macro
# that could be unset, it needs a hand-written check here.
	$(SHELL) $< $* \
	  $(if $(filter $*,$(android-strong)),strong,plain) \
	  $(if $(filter $*,$(android-support-$(subdir))),noassert,assert) \
	  $(if $(android-asm-$*),$(subst $() ,:,$(strip $(android-asm-$*))),none) > $@T
	$(move-if-change) $@T $@

generated += $(android-shim-gen:%=fc-%.c)

sysdep_routines += $(addprefix fc-,$(android-objs))

$(foreach f,$(android-objs),$(eval CFLAGS-fc-$(f).c += $(android-inc)))

# posix_spawn's OPEN file action carries a path that the child opens with
# __open_nocancel (spawni.c:214) -- no wrapper covers that, so the child would
# open it untranslated and exit 127 silently.  Translate where the action is
# REGISTERED, which is ordinary context: no child stack, no vfork-safety
# question.  The chdir action needs nothing; spawni.c:257 uses wrapped __chdir.
#
# Its own flag, not a shared one: this file's symbol is not being taken over by
# a wrapper, so it needs no rename guard -- same distinction spawni.c makes
# below.
#
# Inside this ifneq on purpose: android-inc is only assigned here, and posix is
# a subdir with android-objs, which is the subdir that compiles this file.
CFLAGS-spawn_faction_addopen.c += -DANDROID_SPAWN_FACTION_TRANSLATE $(android-inc)

endif

# Two more files translated IN PLACE rather than wrapped, for the same reason as
# spawn_faction_addopen: each records or opens a path through __open_nocancel,
# which no wrapper covers, and neither needs new exported-symbol surface to fix.
#
#   catgets/open_catalog.c  two opens -- and wrapping catopen would miss the
#                           second, whose path is built here from an NLSPATH
#                           template and never passes through catopen's argument
#   login/utmpname.c        records a name that __libc_setutent opens later
#
# OUTSIDE the ifneq, and using android-inc-path rather than android-inc: neither
# `catgets' nor `login' has any android objects, so android-inc is unassigned for
# those subdirs and would expand to empty -- the exact scoping mistake the long
# comment above this section was written about.
CFLAGS-open_catalog.c += -DANDROID_CATGETS_TRANSLATE $(android-inc-path)
CFLAGS-utmpname.c     += -DANDROID_UTMP_TRANSLATE    $(android-inc-path)

# posix_spawn runs args->exec IN the CLONE_VM|CLONE_VFORK child, on the stack
# spawni.c sizes for itself.  Wiring execve makes that pointer __fc_execve,
# whose chain needs ~37KiB against spawni.c's 32KiB -- with no guard page and
# the parent's address space shared.  Derived from the wrapper table rather
# than hardcoded, so dropping execve from it reverts spawni.c to stock.
#
# Its own flag: spawni.c is not renamed and its symbol is not taken over, so it
# carries no IS_IN (rtld) rename guard -- this only sizes the child stack.
ifneq ($(filter execve,$(android-wrappers-posix)),)
CFLAGS-spawni.c += -DANDROID_SPAWN_STACK
endif
