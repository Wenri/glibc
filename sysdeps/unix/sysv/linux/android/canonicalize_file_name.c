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

#ifdef HAVE_CANONICALIZE_FILE_NAME

#include <stdlib.h>
#include "wrapper.h"

#ifdef HAVE___REALPATH_CHK
# include "__realpath_chk.h"
#endif


wrapper(canonicalize_file_name, char *, (const char * name))
{
    debug("canonicalize_file_name(\"%s\")", name);
    /* ADAPTED FOR GLIBC -- not upstream fakechroot.  Upstream malloc'd an 8 KiB
       buffer here, passed it down, and returned the callee's result -- so every
       failure (ENOENT being much the most common: probing PATH entries, -I
       dirs, candidate configs) leaked the whole 8 KiB, and the malloc was
       unchecked besides.

       Passing NULL instead is what stock glibc's canonicalize_file_name does
       (__realpath (name, NULL)), and realpath.c:109-117 already handles it:
       it allocates path_max itself, and its error path frees that buffer
       precisely when RESOLVED was NULL (realpath.c:269-270).  So this removes
       the leak rather than moving it, drops the unchecked malloc, and halves
       the allocation.  The size argument stays >= PATH_MAX to keep
       __realpath_chk from taking __chk_fail; with a NULL buffer it has nothing
       to fortify.  */
#ifdef HAVE___REALPATH_CHK
    return __realpath_chk(name, NULL, FAKECHROOT_PATH_MAX * 2);
#else
    return realpath(name, NULL);
#endif
}

#else
typedef int empty_translation_unit;
#endif
