/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)paths.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _PATHS_H_
#define	_PATHS_H_

/* Default search path. */
#define	_PATH_DEFPATH	"/data/data/com.termux.nix/files/usr/bin:/data/data/com.termux.nix/files/bin:/system/bin"
/* All standard utilities path. */
#define	_PATH_STDPATH \
	"/data/data/com.termux.nix/files/usr/bin:/data/data/com.termux.nix/files/bin:/system/bin"

#define	_PATH_BSHELL	"/data/data/com.termux.nix/files/usr/bin/sh"
#define	_PATH_CONSOLE	"/dev/console"
#define	_PATH_CSHELL	"/data/data/com.termux.nix/files/usr/bin/csh"
#define	_PATH_DEVDB	"/data/data/com.termux.nix/files/usr/var/run/dev.db"
#define	_PATH_DEVNULL	"/dev/null"
#define	_PATH_DRUM	"/dev/drum"
#define	_PATH_GSHADOW	"/data/data/com.termux.nix/files/usr/etc/gshadow"
#define	_PATH_KLOG	"/proc/kmsg"
#define	_PATH_KMEM	"/dev/kmem"
#define	_PATH_LASTLOG	"/data/data/com.termux.nix/files/usr/var/log/lastlog"
#define	_PATH_MAILDIR	"/data/data/com.termux.nix/files/usr/var/mail"
#define	_PATH_MAN	"/data/data/com.termux.nix/files/usr/share/man"
#define	_PATH_MEM	"/dev/mem"
#define	_PATH_MNTTAB	"/data/data/com.termux.nix/files/usr/etc/fstab"
#define	_PATH_MOUNTED	"/data/data/com.termux.nix/files/usr/etc/mtab"
#define	_PATH_NOLOGIN	"/data/data/com.termux.nix/files/usr/etc/nologin"
#define	_PATH_PRESERVE	"/data/data/com.termux.nix/files/usr/var/lib"
#define	_PATH_RWHODIR	"/data/data/com.termux.nix/files/usr/var/spool/rwho"
#define	_PATH_SENDMAIL	"/data/data/com.termux.nix/files/usr/bin/sendmail"
#define	_PATH_SHADOW	"/data/data/com.termux.nix/files/usr/etc/shadow"
#define	_PATH_SHELLS	"/data/data/com.termux.nix/files/usr/etc/shells"
#define	_PATH_TTY	"/dev/tty"
#define	_PATH_UNIX	"/boot/vmlinux"
#define	_PATH_UTMP	"/data/data/com.termux.nix/files/usr/var/run/utmp"
#define	_PATH_VI	"/data/data/com.termux.nix/files/usr/bin/vi"
#define	_PATH_WTMP	"/data/data/com.termux.nix/files/usr/var/log/wtmp"

/* Provide trailing slash, since mostly used for building pathnames. */
#define	_PATH_DEV	"/dev/"
#define	_PATH_TMP	"/data/data/com.termux.nix/files/tmp/"
#define	_PATH_VARDB	"/data/data/com.termux.nix/files/usr/var/db/"
#define	_PATH_VARRUN	"/data/data/com.termux.nix/files/usr/var/run/"
#define	_PATH_VARTMP	"/data/data/com.termux.nix/files/usr/var/tmp/"

#endif /* !_PATHS_H_ */
