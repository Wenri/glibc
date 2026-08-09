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

#if !defined(HAVE___LXSTAT) || NEW_GLIBC

#include <sys/stat.h>
#include <unistd.h>
#include "wrapper.h"
#include "lstat.h"


wrapper(lstat, int, (const char * filename, struct stat * buf))
{
    char abs_filename[FAKECHROOT_PATH_MAX];
    debug("lstat(\"%s\", &buf)", filename);

    if (!fakechroot_localdir(filename)) {
        if (filename != NULL) {
            /* ADAPTED FOR GLIBC: rel2abs signals failure with NULL and does not
               write ABS_FILENAME; upstream points FILENAME at it regardless.
               lstat64.c:47 already checks -- match it.  */
            if (rel2abs(filename, abs_filename) == NULL) {
                return -1;
            }
            filename = abs_filename;
        }
    }

    return lstat_rel(filename, buf);
}


/* Prevent looping with realpath() */
LOCAL int lstat_rel(const char * file_name, struct stat * buf)
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    int retval;
    const char *orig;

    debug("lstat_rel(\"%s\", &buf)", file_name);
    orig = file_name;
    file_name = expand_chroot_rel_path(file_name, fakechroot_buf);
    retval = nextcall(lstat)(file_name, buf);

    /* ADAPTED FOR GLIBC -- not upstream fakechroot.  Two changes, both about
       BUF being the CALLER's uninitialised struct stat when the lstat failed:

       - the retval test.  Upstream reads buf->st_mode unconditionally, so a
         failed lstat decides this branch from stack garbage, and a chance
         S_IFLNK bit pattern then issues a real readlink() and writes st_size
         into a struct the caller must not read.
       - scoping TMP.  Upstream declares it in the function frame, so every
         lstat pays 4KB for a buffer only the symlink fixup uses.  That is half
         of what pushes this chain past a stock SIGSTKSZ sigaltstack.  */
    if (retval == 0 && (buf->st_mode & S_IFMT) == S_IFLNK) {
        char tmp[FAKECHROOT_PATH_MAX];
        READLINK_TYPE_RETURN status;

        /* deal with http://bugs.debian.org/561991 */
        if ((status = readlink(orig, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = status;
    }
    return retval;
}


#else
typedef int empty_translation_unit;
#endif
