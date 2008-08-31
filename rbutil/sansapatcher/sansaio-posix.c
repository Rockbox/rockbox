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
#include <sys/ioctl.h>
#include <errno.h>

#if defined(linux) || defined (__linux)
#include <sys/mount.h>
#define SANSA_SECTORSIZE_IOCTL BLKSSZGET
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
#include <sys/disk.h>
#define SANSA_SECTORSIZE_IOCTL DIOCGSECTORSIZE
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/disk.h>
#define SANSA_SECTORSIZE_IOCTL DKIOCGETBLOCKSIZE
#else
    #error No sector-size detection implemented for this platform
#endif

#include "sansaio.h"

#if defined(__APPLE__) && defined(__MACH__)
static int sansa_unmount(struct sansa_t* sansa)
{
    char cmd[4096];
    int res;

    sprintf(cmd, "/usr/sbin/diskutil unmount \"%ss1\"",sansa->diskname);
    fprintf(stderr,"[INFO] ");
    res = system(cmd);

    if (res==0) {
        return 0;
    } else {
        perror("Unmount failed");
        return -1;
    }
}
#endif


#ifndef RBUTIL
void print_error(char* msg)
{
    perror(msg);
}
#endif

int sansa_open(struct sansa_t* sansa, int silent)
{
    sansa->dh=open(sansa->diskname,O_RDONLY);
    if (sansa->dh < 0) {
        if (!silent) perror(sansa->diskname);
        if(errno == EACCES) return -2;
        else return -1;
    }

    if(ioctl(sansa->dh,SANSA_SECTORSIZE_IOCTL,&sansa->sector_size) < 0) {
        sansa->sector_size=512;
        if (!silent) {
            fprintf(stderr,"[ERR] ioctl() call to get sector size failed, defaulting to %d\n"
                   ,sansa->sector_size);
        }
    }
    return 0;
}


int sansa_reopen_rw(struct sansa_t* sansa)
{
#if defined(__APPLE__) && defined(__MACH__)
    if (sansa_unmount(sansa) < 0)
        return -1;
#endif

    close(sansa->dh);
    sansa->dh=open(sansa->diskname,O_RDWR);
    if (sansa->dh < 0) {
        perror(sansa->diskname);
        return -1;
    }
    return 0;
}

int sansa_close(struct sansa_t* sansa)
{
    close(sansa->dh);
    return 0;
}

int sansa_alloc_buffer(unsigned char** sectorbuf, int bufsize)
{
    *sectorbuf=malloc(bufsize);
    if (*sectorbuf == NULL) {
        return -1;
    }
    return 0;
}

int sansa_seek(struct sansa_t* sansa, loff_t pos)
{
    off_t res;

    res = lseek64(sansa->dh, pos, SEEK_SET);

    if (res == -1) {
       return -1;
    }
    return 0;
}

int sansa_read(struct sansa_t* sansa, unsigned char* buf, int nbytes)
{
    return read(sansa->dh, buf, nbytes);
}

int sansa_write(struct sansa_t* sansa, unsigned char* buf, int nbytes)
{
    return write(sansa->dh, buf, nbytes);
}
