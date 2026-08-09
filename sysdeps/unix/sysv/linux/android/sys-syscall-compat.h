/* SYS_<name> -> __NR_<name>, for the imported fakechroot sources.

   Inside a glibc build <sys/syscall.h> resolves to glibc's own internal
   wrapper (sysdeps/unix/sysv/linux/include/sys/syscall.h), which deliberately
   defines ONLY __NR_* via <arch-syscall.h> -- the public SYS_* spellings do not
   exist.  Without this header every `#ifdef SYS_foo' in an imported file
   silently takes its portability fallback: getcwd_real.c, for instance, drops
   from a one-line raw SYS_getcwd into a 200-line BSD tree-walk that calls
   lstat/opendir and drags in wrappers that are not wired yet.

   Note where the numbers come from.  Roughly half of these syscalls have had
   their __NR_ REMOVED from <arch-syscall.h> by
   common/pkgs/glibc-termux/process-fakesyscalls.sh -- that is how the existing
   glibc-side seccomp mechanism makes them unissuable by construction.  They are
   moved, not deleted, into <disabled-syscall.h>, which config.h pulls in first.
   fakechroot needs exactly those numbers at RUNTIME (its SIGSYS handler and
   syscall dispatcher emulate the blocked calls), so the two mechanisms meet
   here.  See nix-on-droid/docs/SECCOMP.md "Two seccomp mechanisms".

   Regenerate after re-importing fakechroot with ./regen-sys-syscall-compat.sh,
   and audit an existing header with `./regen-sys-syscall-compat.sh --check'.
   Do NOT hand-maintain this list from a grep: the REDIRECT_ENTRY targets are
   pasted together by BOOST_PP_CAT (SYS_, to), so spellings like SYS_accept4
   appear nowhere in the sources.  Four of them were missing until
   dl-sigsys.c's #error caught it.  */
#ifndef _FAKECHROOT_SYS_SYSCALL_COMPAT_H
#define _FAKECHROOT_SYS_SYSCALL_COMPAT_H

#if !defined SYS_accept && defined __NR_accept
# define SYS_accept __NR_accept
#endif
#if !defined SYS_accept4 && defined __NR_accept4
# define SYS_accept4 __NR_accept4
#endif
#if !defined SYS_add_key && defined __NR_add_key
# define SYS_add_key __NR_add_key
#endif
#if !defined SYS_bpf && defined __NR_bpf
# define SYS_bpf __NR_bpf
#endif
#if !defined SYS_chdir && defined __NR_chdir
# define SYS_chdir __NR_chdir
#endif
#if !defined SYS_chmod && defined __NR_chmod
# define SYS_chmod __NR_chmod
#endif
#if !defined SYS_chown && defined __NR_chown
# define SYS_chown __NR_chown
#endif
#if !defined SYS_chown32 && defined __NR_chown32
# define SYS_chown32 __NR_chown32
#endif
#if !defined SYS_chroot && defined __NR_chroot
# define SYS_chroot __NR_chroot
#endif
#if !defined SYS_clone3 && defined __NR_clone3
# define SYS_clone3 __NR_clone3
#endif
#if !defined SYS_close_range && defined __NR_close_range
# define SYS_close_range __NR_close_range
#endif
#if !defined SYS_delete_module && defined __NR_delete_module
# define SYS_delete_module __NR_delete_module
#endif
#if !defined SYS_epoll_pwait2 && defined __NR_epoll_pwait2
# define SYS_epoll_pwait2 __NR_epoll_pwait2
#endif
#if !defined SYS_faccessat && defined __NR_faccessat
# define SYS_faccessat __NR_faccessat
#endif
#if !defined SYS_faccessat2 && defined __NR_faccessat2
# define SYS_faccessat2 __NR_faccessat2
#endif
#if !defined SYS_fanotify_init && defined __NR_fanotify_init
# define SYS_fanotify_init __NR_fanotify_init
#endif
#if !defined SYS_fanotify_mark && defined __NR_fanotify_mark
# define SYS_fanotify_mark __NR_fanotify_mark
#endif
#if !defined SYS_fchmodat && defined __NR_fchmodat
# define SYS_fchmodat __NR_fchmodat
#endif
#if !defined SYS_fchmodat2 && defined __NR_fchmodat2
# define SYS_fchmodat2 __NR_fchmodat2
#endif
#if !defined SYS_fchownat && defined __NR_fchownat
# define SYS_fchownat __NR_fchownat
#endif
#if !defined SYS_finit_module && defined __NR_finit_module
# define SYS_finit_module __NR_finit_module
#endif
#if !defined SYS_futex_waitv && defined __NR_futex_waitv
# define SYS_futex_waitv __NR_futex_waitv
#endif
#if !defined SYS_get_mempolicy && defined __NR_get_mempolicy
# define SYS_get_mempolicy __NR_get_mempolicy
#endif
#if !defined SYS_get_robust_list && defined __NR_get_robust_list
# define SYS_get_robust_list __NR_get_robust_list
#endif
#if !defined SYS_getcwd && defined __NR_getcwd
# define SYS_getcwd __NR_getcwd
#endif
#if !defined SYS_getpgid && defined __NR_getpgid
# define SYS_getpgid __NR_getpgid
#endif
#if !defined SYS_getpgrp && defined __NR_getpgrp
# define SYS_getpgrp __NR_getpgrp
#endif
#if !defined SYS_getxattr && defined __NR_getxattr
# define SYS_getxattr __NR_getxattr
#endif
#if !defined SYS_init_module && defined __NR_init_module
# define SYS_init_module __NR_init_module
#endif
#if !defined SYS_inotify_add_watch && defined __NR_inotify_add_watch
# define SYS_inotify_add_watch __NR_inotify_add_watch
#endif
#if !defined SYS_io_pgetevents && defined __NR_io_pgetevents
# define SYS_io_pgetevents __NR_io_pgetevents
#endif
#if !defined SYS_io_uring_enter && defined __NR_io_uring_enter
# define SYS_io_uring_enter __NR_io_uring_enter
#endif
#if !defined SYS_io_uring_register && defined __NR_io_uring_register
# define SYS_io_uring_register __NR_io_uring_register
#endif
#if !defined SYS_io_uring_setup && defined __NR_io_uring_setup
# define SYS_io_uring_setup __NR_io_uring_setup
#endif
#if !defined SYS_kcmp && defined __NR_kcmp
# define SYS_kcmp __NR_kcmp
#endif
#if !defined SYS_keyctl && defined __NR_keyctl
# define SYS_keyctl __NR_keyctl
#endif
#if !defined SYS_landlock_add_rule && defined __NR_landlock_add_rule
# define SYS_landlock_add_rule __NR_landlock_add_rule
#endif
#if !defined SYS_landlock_create_ruleset && defined __NR_landlock_create_ruleset
# define SYS_landlock_create_ruleset __NR_landlock_create_ruleset
#endif
#if !defined SYS_landlock_restrict_self && defined __NR_landlock_restrict_self
# define SYS_landlock_restrict_self __NR_landlock_restrict_self
#endif
#if !defined SYS_lgetxattr && defined __NR_lgetxattr
# define SYS_lgetxattr __NR_lgetxattr
#endif
#if !defined SYS_link && defined __NR_link
# define SYS_link __NR_link
#endif
#if !defined SYS_linkat && defined __NR_linkat
# define SYS_linkat __NR_linkat
#endif
#if !defined SYS_listxattr && defined __NR_listxattr
# define SYS_listxattr __NR_listxattr
#endif
#if !defined SYS_llistxattr && defined __NR_llistxattr
# define SYS_llistxattr __NR_llistxattr
#endif
#if !defined SYS_lremovexattr && defined __NR_lremovexattr
# define SYS_lremovexattr __NR_lremovexattr
#endif
#if !defined SYS_lsetxattr && defined __NR_lsetxattr
# define SYS_lsetxattr __NR_lsetxattr
#endif
#if !defined SYS_mbind && defined __NR_mbind
# define SYS_mbind __NR_mbind
#endif
#if !defined SYS_mkdirat && defined __NR_mkdirat
# define SYS_mkdirat __NR_mkdirat
#endif
#if !defined SYS_mknodat && defined __NR_mknodat
# define SYS_mknodat __NR_mknodat
#endif
#if !defined SYS_mount && defined __NR_mount
# define SYS_mount __NR_mount
#endif
#if !defined SYS_mount_setattr && defined __NR_mount_setattr
# define SYS_mount_setattr __NR_mount_setattr
#endif
#if !defined SYS_mq_open && defined __NR_mq_open
# define SYS_mq_open __NR_mq_open
#endif
#if !defined SYS_msgctl && defined __NR_msgctl
# define SYS_msgctl __NR_msgctl
#endif
#if !defined SYS_msgget && defined __NR_msgget
# define SYS_msgget __NR_msgget
#endif
#if !defined SYS_msgrcv && defined __NR_msgrcv
# define SYS_msgrcv __NR_msgrcv
#endif
#if !defined SYS_msgsnd && defined __NR_msgsnd
# define SYS_msgsnd __NR_msgsnd
#endif
#if !defined SYS_name_to_handle_at && defined __NR_name_to_handle_at
# define SYS_name_to_handle_at __NR_name_to_handle_at
#endif
#if !defined SYS_newfstatat && defined __NR_newfstatat
# define SYS_newfstatat __NR_newfstatat
#endif
#if !defined SYS_open_by_handle_at && defined __NR_open_by_handle_at
# define SYS_open_by_handle_at __NR_open_by_handle_at
#endif
#if !defined SYS_openat && defined __NR_openat
# define SYS_openat __NR_openat
#endif
#if !defined SYS_openat2 && defined __NR_openat2
# define SYS_openat2 __NR_openat2
#endif
#if !defined SYS_pidfd_send_signal && defined __NR_pidfd_send_signal
# define SYS_pidfd_send_signal __NR_pidfd_send_signal
#endif
#if !defined SYS_pkey_alloc && defined __NR_pkey_alloc
# define SYS_pkey_alloc __NR_pkey_alloc
#endif
#if !defined SYS_pkey_free && defined __NR_pkey_free
# define SYS_pkey_free __NR_pkey_free
#endif
#if !defined SYS_pkey_mprotect && defined __NR_pkey_mprotect
# define SYS_pkey_mprotect __NR_pkey_mprotect
#endif
#if !defined SYS_process_madvise && defined __NR_process_madvise
# define SYS_process_madvise __NR_process_madvise
#endif
#if !defined SYS_process_mrelease && defined __NR_process_mrelease
# define SYS_process_mrelease __NR_process_mrelease
#endif
#if !defined SYS_ptrace && defined __NR_ptrace
# define SYS_ptrace __NR_ptrace
#endif
#if !defined SYS_readlinkat && defined __NR_readlinkat
# define SYS_readlinkat __NR_readlinkat
#endif
#if !defined SYS_recv && defined __NR_recv
# define SYS_recv __NR_recv
#endif
#if !defined SYS_recvfrom && defined __NR_recvfrom
# define SYS_recvfrom __NR_recvfrom
#endif
#if !defined SYS_removexattr && defined __NR_removexattr
# define SYS_removexattr __NR_removexattr
#endif
#if !defined SYS_renameat && defined __NR_renameat
# define SYS_renameat __NR_renameat
#endif
#if !defined SYS_renameat2 && defined __NR_renameat2
# define SYS_renameat2 __NR_renameat2
#endif
#if !defined SYS_request_key && defined __NR_request_key
# define SYS_request_key __NR_request_key
#endif
#if !defined SYS_rmdir && defined __NR_rmdir
# define SYS_rmdir __NR_rmdir
#endif
#if !defined SYS_rseq && defined __NR_rseq
# define SYS_rseq __NR_rseq
#endif
#if !defined SYS_rt_sigaction && defined __NR_rt_sigaction
# define SYS_rt_sigaction __NR_rt_sigaction
#endif
#if !defined SYS_semctl && defined __NR_semctl
# define SYS_semctl __NR_semctl
#endif
#if !defined SYS_semget && defined __NR_semget
# define SYS_semget __NR_semget
#endif
#if !defined SYS_semop && defined __NR_semop
# define SYS_semop __NR_semop
#endif
#if !defined SYS_semtimedop && defined __NR_semtimedop
# define SYS_semtimedop __NR_semtimedop
#endif
#if !defined SYS_send && defined __NR_send
# define SYS_send __NR_send
#endif
#if !defined SYS_sendto && defined __NR_sendto
# define SYS_sendto __NR_sendto
#endif
#if !defined SYS_set_mempolicy && defined __NR_set_mempolicy
# define SYS_set_mempolicy __NR_set_mempolicy
#endif
#if !defined SYS_set_robust_list && defined __NR_set_robust_list
# define SYS_set_robust_list __NR_set_robust_list
#endif
#if !defined SYS_setfsgid && defined __NR_setfsgid
# define SYS_setfsgid __NR_setfsgid
#endif
#if !defined SYS_setfsgid32 && defined __NR_setfsgid32
# define SYS_setfsgid32 __NR_setfsgid32
#endif
#if !defined SYS_setfsuid && defined __NR_setfsuid
# define SYS_setfsuid __NR_setfsuid
#endif
#if !defined SYS_setfsuid32 && defined __NR_setfsuid32
# define SYS_setfsuid32 __NR_setfsuid32
#endif
#if !defined SYS_setgid && defined __NR_setgid
# define SYS_setgid __NR_setgid
#endif
#if !defined SYS_setgid32 && defined __NR_setgid32
# define SYS_setgid32 __NR_setgid32
#endif
#if !defined SYS_setregid && defined __NR_setregid
# define SYS_setregid __NR_setregid
#endif
#if !defined SYS_setregid32 && defined __NR_setregid32
# define SYS_setregid32 __NR_setregid32
#endif
#if !defined SYS_setresgid && defined __NR_setresgid
# define SYS_setresgid __NR_setresgid
#endif
#if !defined SYS_setresgid32 && defined __NR_setresgid32
# define SYS_setresgid32 __NR_setresgid32
#endif
#if !defined SYS_setresuid && defined __NR_setresuid
# define SYS_setresuid __NR_setresuid
#endif
#if !defined SYS_setresuid32 && defined __NR_setresuid32
# define SYS_setresuid32 __NR_setresuid32
#endif
#if !defined SYS_setreuid && defined __NR_setreuid
# define SYS_setreuid __NR_setreuid
#endif
#if !defined SYS_setreuid32 && defined __NR_setreuid32
# define SYS_setreuid32 __NR_setreuid32
#endif
#if !defined SYS_setuid && defined __NR_setuid
# define SYS_setuid __NR_setuid
#endif
#if !defined SYS_setuid32 && defined __NR_setuid32
# define SYS_setuid32 __NR_setuid32
#endif
#if !defined SYS_setxattr && defined __NR_setxattr
# define SYS_setxattr __NR_setxattr
#endif
#if !defined SYS_shmat && defined __NR_shmat
# define SYS_shmat __NR_shmat
#endif
#if !defined SYS_shmctl && defined __NR_shmctl
# define SYS_shmctl __NR_shmctl
#endif
#if !defined SYS_shmdt && defined __NR_shmdt
# define SYS_shmdt __NR_shmdt
#endif
#if !defined SYS_shmget && defined __NR_shmget
# define SYS_shmget __NR_shmget
#endif
#if !defined SYS_statfs && defined __NR_statfs
# define SYS_statfs __NR_statfs
#endif
#if !defined SYS_statx && defined __NR_statx
# define SYS_statx __NR_statx
#endif
#if !defined SYS_symlink && defined __NR_symlink
# define SYS_symlink __NR_symlink
#endif
#if !defined SYS_symlinkat && defined __NR_symlinkat
# define SYS_symlinkat __NR_symlinkat
#endif
#if !defined SYS_truncate && defined __NR_truncate
# define SYS_truncate __NR_truncate
#endif
#if !defined SYS_unlinkat && defined __NR_unlinkat
# define SYS_unlinkat __NR_unlinkat
#endif
#if !defined SYS_utimensat && defined __NR_utimensat
# define SYS_utimensat __NR_utimensat
#endif

#endif
