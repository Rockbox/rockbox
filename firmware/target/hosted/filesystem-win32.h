/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _FILESYSTEM_WIN32_H_
#define _FILESYSTEM_WIN32_H_

#ifndef OSFUNCTIONS_DECLARED

#ifdef __MINGW32__
/* filesystem-win32.c contains some string functions that could be useful
 * elsewhere; just move them away to unicode.c or something if they prove
 * so. */
size_t strlcpy_ucs2utf8(char *buffer, const unsigned short *ucs,
                        size_t bufsize);

#define strlcpy_from_os strlcpy_ucs2utf8
#endif /* __MINGW32__ */

#endif /* !OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_WIN32_H_ */

#ifdef __MINGW32__

#ifdef _FILE_H_
#ifndef _FILESYSTEM_WIN32__FILE_H_
#define _FILESYSTEM_WIN32__FILE_H_

#include <unistd.h>
#include <sys/stat.h>

#define OS_STAT_T       struct _stat

#ifndef OSFUNCTIONS_DECLARED
/* Wrap for off_t <=> long conversions */
static inline off_t os_filesize_(int osfd)
    { return _filelength(osfd); }
static inline int os_ftruncate_(int osfd, off_t length)
    { return _chsize(osfd, length); }

#define os_filesize     os_filesize_
#define os_ftruncate    os_ftruncate_
#define os_fsync        _commit
#define os_fstat        _fstat
#define os_close        close
#define os_lseek        lseek
#ifndef os_read
#define os_read         read
#endif
#ifndef os_write
#define os_write        write
#endif

/* These need string type conversion from utf8 to ucs2; that's done inside */
int os_open(const char *ospath, int oflag, ...);
int os_creat(const char *ospath, mode_t mode);
int os_stat(const char *ospath, struct _stat *s);
int os_remove(const char *ospath);
int os_rename(const char *osold, const char *osnew);

#endif /* !OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_WIN32__FILE_H_ */
#endif /* _FILE_H_ */

#ifdef _DIR_H_
#ifndef _FILESYSTEM_WIN32__DIR_H_
#define _FILESYSTEM_WIN32__DIR_H_

#include <dirent.h>

#define OS_DIRENT_CONVERT /* needs string conversion */
#define OS_DIR_T        _WDIR
#define OS_DIRENT_T     struct _wdirent

#ifndef OSFUNCTIONS_DECLARED

_WDIR * os_opendir(const char *osdirname);
int os_opendirfd(const char *osdirname);
#define os_readdir      _wreaddir
#define os_closedir     _wclosedir
int os_mkdir(const char *ospath, mode_t mode);
int os_rmdir(const char *ospath);

#endif /* OSFUNCTIONS_DECLARED */

#endif /* _FILESYSTEM_WIN32__DIR_H_ */
#endif /* _DIR_H_ */

#else /* !__MINGW32__ */

#include "filesystem-unix.h"

#endif /* __MINGW32__ */
