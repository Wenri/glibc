/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013 Piotr Roszatycki <dexter@debian.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/


#include <config.h>

#ifdef HAVE_FUTIMESAT

#define _ATFILE_SOURCE
#include <sys/time.h>
#include "wrapper.h"


wrapper(futimesat, int, (int fd, const char * filename, const struct timeval tv [2]))
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("futimesat(%d, \"%s\", &tv)", fd, filename);
    /* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream calls
       expand_chroot_path here, which resolves a relative FILENAME against the
       CWD instead of FD.  The result is absolute, so the kernel then ignores FD
       entirely: futimesat(fd, "data", tv) with fd = /a and cwd = /b rewrites
       /b/data's timestamps, or fails ENOENT with no hint that FD was dropped.
       This was the only live *at wrapper not using the _at form.  */
    filename = expand_chroot_path_at(fd, filename, fakechroot_buf);
    return nextcall(futimesat)(fd, filename, tv);
}

#else
typedef int empty_translation_unit;
#endif
