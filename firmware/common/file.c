/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <string.h>
#include "file.h"
#include "fat.h"
#include "types.h"
#include "dir.h"
#include "debug.h"

#define MAX_OPEN_FILES 4

struct filedesc {
    unsigned char sector[SECTOR_SIZE];
    int offset;
    struct fat_file fatfile;
    bool busy;
};

static struct filedesc openfiles[MAX_OPEN_FILES];

int open(char* pathname, int flags)
{
    DIR* dir;
    struct dirent* entry;
    int fd;
    char* name;
    int namelen;

    if ( pathname[0] != '/' ) {
        DEBUGF("'%s' is not an absolute path.\n",pathname);
        DEBUGF("Only absolute pathnames supported at the moment\n");
        return -1;
    }

    /* find a free file descriptor */
    for ( fd=0; fd<MAX_OPEN_FILES; fd++ )
        if ( !openfiles[fd].busy )
            break;

    if ( fd == MAX_OPEN_FILES ) {
        DEBUGF("Too many files open\n");
        return -1;
    }

    /* locate filename */
    name=strrchr(pathname+1,'/');
    if ( name ) {
        *name = 0;
        dir = opendir(pathname);
        *name = '/';
        name++;
    }
    else {
        dir = opendir("/");
        name = pathname+1;
    }
    if (!dir) {
        DEBUGF("Failed opening dir\n");
        return -1;
    }

    /* scan dir for name */
    namelen = strlen(name);
    while ((entry = readdir(dir))) {
        if ( !strncmp(name, entry->d_name, namelen) ) {
            fat_open(entry->startcluster, &(openfiles[fd].fatfile));
            break;
        }
        else {
            DEBUGF("entry: %s\n",entry->d_name);
        }
    }
    closedir(dir);
    if ( !entry ) {
        DEBUGF("Couldn't find %s in %s\n",name,pathname);
        /* fixme: we need to use proper error codes */
        return -1;
    }

    openfiles[fd].offset = 0;
    openfiles[fd].busy = TRUE;
    return fd;
}

int close(int fd)
{
    openfiles[fd].busy = FALSE;
    return 0;
}

int read(int fd, void* buf, int count)
{
    int sectors;
    int nread=0;

    /* are we in the middle of a cached sector? */
    if ( openfiles[fd].offset ) {
        if ( count > (SECTOR_SIZE - openfiles[fd].offset) ) {
            memcpy( buf, openfiles[fd].sector,
                    SECTOR_SIZE - openfiles[fd].offset );
            openfiles[fd].offset = 0;
            nread = SECTOR_SIZE - openfiles[fd].offset;
            count -= nread;
        }
        else {
            memcpy( buf, openfiles[fd].sector, count );
            openfiles[fd].offset += count;
            nread = count;
            count = 0;
        }
    }

    /* read whole sectors right into the supplied buffer */
    sectors = count / SECTOR_SIZE;
    if ( sectors ) {
        if ( fat_read(&(openfiles[fd].fatfile), sectors, buf+nread ) < 0 ) {
            DEBUGF("Failed reading %d sectors\n",sectors);
            return -1;
        }
        nread += sectors * SECTOR_SIZE;
        count -= sectors * SECTOR_SIZE;
        openfiles[fd].offset = 0;
    }

    /* trailing odd bytes? */
    if ( count ) {
        /* do we already have the sector cached? */
        if ( count < (SECTOR_SIZE - openfiles[fd].offset) ) {
            memcpy( buf + nread, openfiles[fd].sector, count );
            openfiles[fd].offset += count;
            nread += count;
            count = 0;
        }
        else {
            /* cache one sector and copy the trailing bytes */
            if ( fat_read(&(openfiles[fd].fatfile), 1,
                          &(openfiles[fd].sector)) < 0 ) {
                DEBUGF("Failed reading odd sector\n");
                return -1;
            }
            memcpy( buf + nread, openfiles[fd].sector, count );
            openfiles[fd].offset = nread;
            nread += count;
        }
    }

    return nread;
}

/*
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
