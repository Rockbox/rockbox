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

#if defined(linux) || defined (__linux)
#include <sys/mount.h>
#define IPOD_SECTORSIZE_IOCTL BLKSSZGET
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
#include <sys/disk.h>
#define IPOD_SECTORSIZE_IOCTL DIOCGSECTORSIZE
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/disk.h>
#define IPOD_SECTORSIZE_IOCTL DKIOCGETBLOCKSIZE
#else
    #error No sector-size detection implemented for this platform
#endif

#include "ipodio.h"

void print_error(char* msg)
{
    perror(msg);
}

int ipod_open(struct ipod_t* ipod, int silent)
{
    ipod->dh=open(ipod->diskname,O_RDONLY);
    if (ipod->dh < 0) {
        if (!silent) perror(ipod->diskname);
        return -1;
    }

    if(ioctl(ipod->dh,IPOD_SECTORSIZE_IOCTL,&ipod->sector_size) < 0) {
        ipod->sector_size=512;
        if (!silent) {
            fprintf(stderr,"[ERR] ioctl() call to get sector size failed, defaulting to %d\n"
                   ,ipod->sector_size);
	}
    }
    return 0;
}


int ipod_reopen_rw(struct ipod_t* ipod)
{
    close(ipod->dh);
    ipod->dh=open(ipod->diskname,O_RDWR);
    if (ipod->dh < 0) {
        perror(ipod->diskname);
        return -1;
    }
    return 0;
}

int ipod_close(struct ipod_t* ipod)
{
    close(ipod->dh);
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

int ipod_seek(struct ipod_t* ipod, unsigned long pos)
{
    off_t res;

    res = lseek(ipod->dh, pos, SEEK_SET);

    if (res == -1) {
       return -1;
    }
    return 0;
}

int ipod_read(struct ipod_t* ipod, unsigned char* buf, int nbytes)
{
    return read(ipod->dh, buf, nbytes);
}

int ipod_write(struct ipod_t* ipod, unsigned char* buf, int nbytes)
{
    return write(ipod->dh, buf, nbytes);
}
