/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Dave Chapman
 *
 * error(), lock_volume() and unlock_volume() functions and inspiration taken
 * from:
 *       RawDisk - Direct Disk Read/Write Access for NT/2000/XP
 *       Copyright (c) 2003 Jan Kiszka
 *       http://www.stud.uni-hannover.de/user/73174/RawDisk/
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __WIN32__
#include <windows.h>
#include <winioctl.h>
#endif

#include "sansaio.h"

static int lock_volume(HANDLE hDisk)
{
  DWORD dummy;

  return DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
			 &dummy, NULL);
}

static int unlock_volume(HANDLE hDisk)
{
  DWORD dummy;

  return DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0,
			 &dummy, NULL);
}

#ifndef RBUTIL
void print_error(char* msg)
{
    char* pMsgBuf;

    printf(msg);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pMsgBuf,
                  0, NULL);
    printf(pMsgBuf);
    LocalFree(pMsgBuf);
}
#endif
int sansa_open(struct sansa_t* sansa, int silent)
{
    DISK_GEOMETRY_EX diskgeometry_ex;
    DISK_GEOMETRY diskgeometry;
    unsigned long n;

    sansa->dh = CreateFileA(sansa->diskname, GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

    if (sansa->dh == INVALID_HANDLE_VALUE) {
        if (!silent) print_error(" Error opening disk: ");
        if(GetLastError() == ERROR_ACCESS_DENIED)
            return -2;
        else
            return -1;
    }

    if (!lock_volume(sansa->dh)) {
        if (!silent) print_error(" Error locking disk: ");
        return -1;
    }

    if (!DeviceIoControl(sansa->dh,
                         IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                         NULL,
                         0,
                         &diskgeometry_ex,
                         sizeof(diskgeometry_ex),
                         &n,
                         NULL)) {
        if (!DeviceIoControl(sansa->dh,
                             IOCTL_DISK_GET_DRIVE_GEOMETRY,
                             NULL,
                             0,
                             &diskgeometry,
                             sizeof(diskgeometry),
                             &n,
                             NULL)) {
            if (!silent) print_error(" Error reading disk geometry: ");
            return -1;
        } else {
            sansa->sector_size=diskgeometry.BytesPerSector;
        }
    } else {
        sansa->sector_size=diskgeometry_ex.Geometry.BytesPerSector;
    }

    return 0;
}

int sansa_reopen_rw(struct sansa_t* sansa)
{
    /* Close existing file and re-open for writing */
    unlock_volume(sansa->dh);
    CloseHandle(sansa->dh);

    sansa->dh = CreateFileA(sansa->diskname, GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                     FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

    if (sansa->dh == INVALID_HANDLE_VALUE) {
        print_error(" Error opening disk: ");
        return -1;
    }

    if (!lock_volume(sansa->dh)) {
        print_error(" Error locking disk: ");
        return -1;
    }

    return 0;
}

int sansa_close(struct sansa_t* sansa)
{
    unlock_volume(sansa->dh);
    CloseHandle(sansa->dh);
    return 0;
}

int sansa_alloc_buffer(unsigned char** sectorbuf, int bufsize)
{
    /* The ReadFile function requires a memory buffer aligned to a multiple of
       the disk sector size. */
    *sectorbuf = (unsigned char*)VirtualAlloc(NULL, bufsize, MEM_COMMIT, PAGE_READWRITE);
    if (*sectorbuf == NULL) {
        print_error(" Error allocating a buffer: ");
        return -1;
    }
    return 0;
}

int sansa_seek(struct sansa_t* sansa, loff_t pos)
{
    LARGE_INTEGER li;

    li.QuadPart = pos;

    li.LowPart = SetFilePointer (sansa->dh, li.LowPart, &li.HighPart, FILE_BEGIN);

    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        print_error(" Seek error ");
        return -1;
    }
    return 0;
}

int sansa_read(struct sansa_t* sansa, unsigned char* buf, int nbytes)
{
    unsigned long count;

    if (!ReadFile(sansa->dh, buf, nbytes, &count, NULL)) {
        print_error(" Error reading from disk: ");
        return -1;
    }

    return count;
}

int sansa_write(struct sansa_t* sansa, unsigned char* buf, int nbytes)
{
    unsigned long count;

    if (!WriteFile(sansa->dh, buf, nbytes, &count, NULL)) {
        print_error(" Error writing to disk: ");
        return -1;
    }

    return count;
}
