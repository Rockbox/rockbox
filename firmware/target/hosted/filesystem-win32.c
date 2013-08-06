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
#define RB_FILESYSTEM_OS
#include <stdio.h>
#include <errno.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "debug.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef __MINGW32__
#include <wchar.h>
#include "rbunicode.h"

unsigned short * strcpy_utf8ucs2(unsigned short *buffer,
                                 const unsigned char *utf8)
{
    for (wchar_t *ucs2 = buffer;
         ((utf8 = utf8decode(utf8, ucs2)), *ucs2); ucs2++);
    return buffer;
}

#if 0
unsigned char * strcpy_ucs2utf8(unsigned char *buffer,
                                const unsigned short *ucs2)
{
    for (unsigned char *utf8 = buffer;
         ((utf8 = utf8encode(*ucs2, utf8)), *ucs2); ucs2++);
    return buffer;
}

size_t strlen_utf8ucs2(const unsigned char *utf8)
{
    size_t length = 0;
    unsigned short ucschar[2];
    for (unsigned char c = *utf8; c;
         ((utf8 = utf8decode(utf8, ucschar)), c = *utf8))
        length++; /* This won't properly count multipart ucs2 */

    return length;
}
#endif /* 0 */

size_t strlen_utf8ucs2(const unsigned char *utf8)
{
    return utf8length(utf8);
}

size_t strlen_ucs2utf8(const unsigned short *ucs2)
{
    size_t length = 0;
    unsigned char utf8char[4];

    for (unsigned short c = *ucs2; c; (c = *++ucs2))
        length += utf8encode(c, utf8char) - utf8char;

    return length;
}

size_t strlcpy_ucs2utf8(char *buffer, const unsigned short *ucs2,
                        size_t bufsize)
{
    if (!buffer)
        bufsize = 0;

    size_t length = 0;
    unsigned char utf8char[4];

    for (unsigned short c = *ucs2; c; (c = *++ucs2))
    {
        /* If the last character won't fit, this won't split it */
        size_t utf8size = utf8encode(c, utf8char) - utf8char;
        if ((length += utf8size) < bufsize)
            buffer = mempcpy(buffer, utf8char, utf8size);
    }

    *buffer = '\0';
    return length;
}

#define _toucs2(utf8) \
    ({ const char *_utf8 = (utf8);         \
       size_t _l = strlen_utf8ucs2(_utf8); \
       void *_buffer = alloca((_l + 1)*2); \
       strcpy_utf8ucs2(_buffer, _utf8); })

#define _toutf8(ucs2) \
    ({ const char *_ucs2 = (ucs2);         \
       size_t _l = strlen_ucs2utf8(_ucs2); \
       void *_buffer = alloca(_l + 1);     \
       strcpy_ucs2utf8(_buffer, _ucs2); })

static HANDLE open_win32_file(const char *ospath)
{
    /* FILE_FLAG_BACKUP_SEMANTICS is required for this to succeed at opening
       a directory */
    HANDLE h = CreateFileW(_toucs2(ospath), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE |
                           FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (h == INVALID_HANDLE_VALUE)
    {
        switch (GetLastError())
        {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errno = ENOENT;
            break;
        default:
            errno = EIO;
        }
    }

    return h;
}

int os_open(const char *ospath, int oflag, ...)
{
    return _wopen(_toucs2(ospath), oflag __OPEN_MODE_ARG);
}

int os_creat(const char *ospath, mode_t mode)
{
    return _wcreat(_toucs2(ospath), mode);
}

int os_stat(const char *ospath, struct _stat *s)
{
    return _wstat(_toucs2(ospath), s);
}

int os_remove(const char *ospath)
{
    return _wremove(_toucs2(ospath));
}

int os_rename(const char *osold, const char *osnew)
{
    return _wrename(_toucs2(osold), _toucs2(osnew));
}

bool os_file_exists(const char *ospath)
{
    HANDLE h = open_win32_file(ospath);
    if (h == INVALID_HANDLE_VALUE)
        return false;

    CloseHandle(h);
    return true;
}

_WDIR * os_opendir(const char *osdirname)
{
    return _wopendir(_toucs2(osdirname));
}

int os_mkdir(const char *ospath, mode_t mode)
{
    return _wmkdir(_toucs2(ospath));
    (void)mode;
}

int os_rmdir(const char *ospath)
{
    return _wrmdir(_toucs2(ospath));
}

int os_dirfd(_WDIR *osdirp)
{
#ifdef ENOTSUP
    errno = ENOTSUP
#else
    errno = ENOSYS;
#endif
    return -1;
    (void)osdirp;
}

int os_opendirfd(const char *osdirname)
{
    HANDLE h = open_win32_file(osdirname);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(h, &info))
        errno = EIO;
    else if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        errno = ENOTDIR;
    else
    {
        /* Convert OS handle to fd; the fd now owns it */
        int osfd = _open_osfhandle((long)h, O_RDONLY);
        if (osfd >= 0)
            return osfd;
    }

    CloseHandle(h);
    return -2;
}
#endif /* __MINGW32__ */

int os_getdirfd(const char *osdirname, _WDIR *osdirp, bool *is_opened)
{
    *is_opened = false;

    int fd = os_dirfd(osdirp);
    if (fd < 0)
    {
        fd = os_opendirfd(osdirname);
        if (fd >= 0)
            *is_opened = true;
    }

    return fd;
}

int os_fsamefile(int osfd1, int osfd2)
{
    BY_HANDLE_FILE_INFORMATION info1, info2;
    long h1 = _get_osfhandle(osfd1);
    if (h1 == -1)
        return -1;

    long h2 = _get_osfhandle(osfd2);
    if (h2 == -1)
        return -2;

    if (!GetFileInformationByHandle((HANDLE)h1, &info1) ||
        !GetFileInformationByHandle((HANDLE)h2, &info2))
    {
        errno = EIO;
        return -3;
    }

    bool same = info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
                info1.nFileIndexHigh == info2.nFileIndexHigh &&
                info1.nFileIndexLow == info2.nFileIndexLow;

    return same ? 1 : 0;
}

void volume_size(IF_MV(int volume,) unsigned long *sizep, unsigned long *freep)
{
    ULARGE_INTEGER free = { .QuadPart = 0 },
                   size = { .QuadPart = 0 };

    char volpath[MAX_PATH];
    if (os_volume_path(IF_MV(volume, ) volpath, sizeof (volpath)) >= 0)
        GetDiskFreeSpaceExW(_toucs2(volpath), &free, &size, NULL);

    if (sizep)
        *sizep = size.QuadPart / 1024;

    if (freep)
        *freep = free.QuadPart / 1024;
}
