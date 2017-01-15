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
#include <ctype.h>
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "debug.h"
#include "pathfuncs.h"
#include "string-extra.h"

#define SAME_FILE_INFO(lpInfo1, lpInfo2) \
    ((lpInfo1)->dwVolumeSerialNumber == (lpInfo2)->dwVolumeSerialNumber && \
     (lpInfo1)->nFileIndexHigh == (lpInfo2)->nFileIndexHigh &&             \
     (lpInfo1)->nFileIndexLow == (lpInfo2)->nFileIndexLow)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void win32_last_error_errno(void)
{
    switch (GetLastError())
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        errno = ENOENT;
        break;
    case ERROR_DIR_NOT_EMPTY:
        errno = ENOTEMPTY;
        break;
    default:
        errno = EIO;
    }
}

#ifdef __MINGW32__
#include <wchar.h>
#include "rbunicode.h"

static HANDLE win32_open(const char *ospath);
static int win32_stat(const char *ospath, LPBY_HANDLE_FILE_INFORMATION lpInfo);

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
    /* This won't properly count multiword ucs2 so use the alternative
       below for now which doesn't either */
    size_t length = 0;
    unsigned short ucschar[2];
    for (unsigned char c = *utf8; c;
         ((utf8 = utf8decode(utf8, ucschar)), c = *utf8))
        length++;

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

    /* Above won't ever copy to very end */
    if (bufsize)
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
    int errnum = errno;

    const wchar_t *wchosold = _toucs2(osold);
    const wchar_t *wchosnew = _toucs2(osnew);

    int rc = _wrename(wchosold, wchosnew);
    if (rc < 0 && errno == EEXIST)
    {
        /* That didn't work; do cheap POSIX mimic */
        BY_HANDLE_FILE_INFORMATION info;
        if (win32_stat(osold, &info))
            return -1;

        if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !RemoveDirectoryW(wchosnew))
        {
            win32_last_error_errno();
            return -1;
        }

        if (MoveFileExW(wchosold, wchosnew, MOVEFILE_REPLACE_EXISTING |
                                            MOVEFILE_WRITE_THROUGH))
        {
            errno = errnum;
            return 0;
        }

        errno = EIO;
    }

    return rc;
}

bool os_file_exists(const char *ospath)
{
    HANDLE h = win32_open(ospath);
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
    errno = ENOTSUP;
#else
    errno = ENOSYS;
#endif
    return -1;
    (void)osdirp;
}

int os_opendirfd(const char *osdirname)
{
    HANDLE h = win32_open(osdirname);
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
        int osfd = _open_osfhandle((intptr_t)h, O_RDONLY);
        if (osfd >= 0)
            return osfd;
    }

    CloseHandle(h);
    return -2;
}
#endif /* __MINGW32__ */

static size_t win32_path_strip_root(const char *ospath)
{
    const char *p = ospath;
    int c = toupper(*p);

    if (c >= 'A' && c <= 'Z')
    {
        /* drive */
        if ((c = *++p) == ':')
            return 2;
    }

    if (c == '\\' && *++p == '\\')
    {
        /* UNC */
        while ((c = *++p) && c != '/' && c != '\\');
        return p - ospath;
    }

    return 0;
}

static HANDLE win32_open(const char *ospath)
{
    /* FILE_FLAG_BACKUP_SEMANTICS is required for this to succeed at opening
       a directory */
    HANDLE h = CreateFileW(_toucs2(ospath), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE |
                           FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (h == INVALID_HANDLE_VALUE)
        win32_last_error_errno();

    return h;
}

static int win32_fstat(int osfd, HANDLE hFile,
                       LPBY_HANDLE_FILE_INFORMATION lpInfo)
{
    /* The file descriptor takes precedence over the win32 file handle */
    if (osfd >= 0)
        hFile = (HANDLE)_get_osfhandle(osfd);

    int rc = GetFileInformationByHandle(hFile, lpInfo) ? 0 : -1;
    if (rc < 0)
        win32_last_error_errno();

    return rc;
}

static int win32_stat(const char *ospath, LPBY_HANDLE_FILE_INFORMATION lpInfo)
{
    HANDLE h = win32_open(ospath);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    int rc = win32_fstat(-1, h, lpInfo);

    CloseHandle(h);

    return rc;
}

int os_opendir_and_fd(const char *osdirname, _WDIR **osdirpp,
                      int *osfdp)
{
    /* another possible way is to use open() then fdopendir() */
    *osdirpp = NULL;
    *osfdp = -1;

    _WDIR *dirp = os_opendir(osdirname);
    if (!dirp)
        return -1;

    int rc = 0;
    int errnum = errno;

    int fd = os_dirfd(dirp);
    if (fd < 0)
    {
        fd = os_opendirfd(osdirname);
        rc = 1;
    }

    if (fd < 0)
    {
        os_closedir(dirp);
        return -2;
    }

    errno = errnum;

    *osdirpp = dirp;
    *osfdp = fd;

    return rc;
}

int os_fsamefile(int osfd1, int osfd2)
{
    BY_HANDLE_FILE_INFORMATION info1, info2;

    if (!win32_fstat(osfd1, INVALID_HANDLE_VALUE, &info1) ||
        !win32_fstat(osfd2, INVALID_HANDLE_VALUE, &info2))
        return -1;

    return SAME_FILE_INFO(&info1, &info2) ? 1 : 0;
}

int os_relate(const char *ospath1, const char *ospath2)
{
    DEBUGF("\"%s\" : \"%s\"\n", ospath1, ospath2);

    if (!ospath2 || !*ospath2)
    {
        errno = ospath2 ? ENOENT : EFAULT;
        return -1;
    }

    /* First file must stay open for duration so that its stats don't change */
    HANDLE h1 = win32_open(ospath1);
    if (h1 == INVALID_HANDLE_VALUE)
        return -2;

    BY_HANDLE_FILE_INFORMATION info1;
    if (win32_fstat(-1, h1, &info1))
    {
        CloseHandle(h1);
        return -3;
    }

    char path2buf[strlen(ospath2) + 1];
    *path2buf = 0;

    ssize_t len = 0;
    const char *p = ospath2;
    size_t rootlen = win32_path_strip_root(ospath2);
    const char *sepmo = PA_SEP_SOFT;

    if (rootlen)
    {
        strmemcpy(path2buf, ospath2, rootlen);
        ospath2 += rootlen;
        sepmo = PA_SEP_HARD;
    }

    int rc = RELATE_DIFFERENT;

    while (1)
    {
        if (sepmo != PA_SEP_HARD &&
            !(len = parse_path_component(&ospath2, &p)))
        {
            break;
        }

        char compname[len + 1];
        strmemcpy(compname, p, len);

        path_append(path2buf, sepmo, compname, sizeof (path2buf));
        sepmo = PA_SEP_SOFT;

        int errnum = errno; /* save and restore if not actually failing */
        BY_HANDLE_FILE_INFORMATION info2;

        if (!win32_stat(path2buf, &info2))
        {
            if (SAME_FILE_INFO(&info1, &info2))
            {
                rc = RELATE_SAME;
            }
            else if (rc == RELATE_SAME)
            {
                if (name_is_dot_dot(compname))
                    rc = RELATE_DIFFERENT;
                else if (!name_is_dot(compname))
                    rc = RELATE_PREFIX;
            }
        }
        else if (errno == ENOENT && !*GOBBLE_PATH_SEPCH(ospath2) &&
                 !name_is_dot_dot(compname))
        {
            if (rc == RELATE_SAME)
                rc = RELATE_PREFIX;

            errno = errnum;
            break;
        }
        else
        {
            rc = -4;
            break;
        }
    }

    CloseHandle(h1);

    return rc;
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
