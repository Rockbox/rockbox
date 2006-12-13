/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include <sys/mount.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/disk.h>
#endif

#if defined(linux) || defined (__linux)
    #define IPOD_SECTORSIZE_IOCTL BLKSSZGET
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
    #define IPOD_SECTORSIZE_IOCTL DIOCGSECTORSIZE
#elif defined(__APPLE__) && defined(__MACH__)
    #define IPOD_SECTORSIZE_IOCTL DKIOCGETBLOCKSIZE
#else
    #error No sector-size detection implemented for this platform
#endif

#include "ipodio.h"

void print_error(char* msg)
{
    perror(msg);
}

int ipod_open(HANDLE* dh, char* diskname, int* sector_size)
{
    *dh=open(diskname,O_RDONLY);
    if (*dh < 0) {
        perror(diskname);
        return -1;
    }

    if(ioctl(*dh,IPOD_SECTORSIZE_IOCTL,sector_size) < 0) {
        fprintf(stderr,"[ERR] ioctl() call to get sector size failed\n");
    }
    return 0;
}


int ipod_reopen_rw(HANDLE* dh, char* diskname)
{
    close(*dh);
    *dh=open(diskname,O_RDWR);
    if (*dh < 0) {
        perror(diskname);
        return -1;
    }
    return 0;
}

int ipod_close(HANDLE dh)
{
    close(dh);
    return 0;
}

int ipod_alloc_buffer(unsigned char** sectorbuf, int bufsize)
{
    *sectorbuf=malloc(bufsize);
    if (*sectorbuf == NULL) {
        return -1;
    }
    return 0;
}

int ipod_seek(HANDLE dh, unsigned long pos)
{
    off_t res;

    res = lseek(dh, pos, SEEK_SET);

    if (res == -1) {
       return -1;
    }
    return 0;
}

int ipod_read(HANDLE dh, unsigned char* buf, int nbytes)
{
    return read(dh, buf, nbytes);
}

int ipod_write(HANDLE dh, unsigned char* buf, int nbytes)
{
    return write(dh, buf, nbytes);
}
