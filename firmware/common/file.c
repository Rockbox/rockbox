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
#include <errno.h>
#include "file.h"
#include "fat.h"
#include "types.h"
#include "dir.h"
#include "debug.h"

/*
  These functions provide a roughly POSIX-compatible file IO API.

  Since the fat32 driver only manages sectors, we maintain a one-sector
  cache for each open file. This way we can provide byte access without
  having to re-read the sector each time. 
  The penalty is the RAM used for the cache and slightly more complex code.
*/

#define MAX_OPEN_FILES 4

struct filedesc {
    unsigned char cache[SECTOR_SIZE];
    int cacheoffset;
    int fileoffset;
    int size;
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
        errno = EINVAL;
        return -1;
    }

    /* find a free file descriptor */
    for ( fd=0; fd<MAX_OPEN_FILES; fd++ )
        if ( !openfiles[fd].busy )
            break;

    if ( fd == MAX_OPEN_FILES ) {
        DEBUGF("Too many files open\n");
        errno = EMFILE;
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
        errno = EIO;
        return -1;
    }

    /* scan dir for name */
    namelen = strlen(name);
    while ((entry = readdir(dir))) {
        if ( !strncmp(name, entry->d_name, namelen) ) {
            fat_open(entry->startcluster, &(openfiles[fd].fatfile));
            openfiles[fd].size = entry->size;
            break;
        }
        else {
            DEBUGF("entry: %s\n",entry->d_name);
        }
    }
    closedir(dir);
    if ( !entry ) {
        DEBUGF("Couldn't find %s in %s\n",name,pathname);
        errno = ENOENT;
        return -1;
    }

    openfiles[fd].cacheoffset = -1;
    openfiles[fd].fileoffset = 0;
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

    if ( !openfiles[fd].busy ) {
        errno = EBADF;
        return -1;
    }

    /* attempt to read past EOF? */
    if ( count > openfiles[fd].size - openfiles[fd].fileoffset )
        count = openfiles[fd].size - openfiles[fd].fileoffset;

    /* any head bytes? */
    if ( openfiles[fd].cacheoffset != -1 ) {
        int headbytes;
        int offs = openfiles[fd].cacheoffset;
        if ( count <= SECTOR_SIZE - openfiles[fd].cacheoffset ) {
            headbytes = count;
            openfiles[fd].cacheoffset += count;
            if ( openfiles[fd].cacheoffset >= SECTOR_SIZE )
                openfiles[fd].cacheoffset = 0;
        }
        else {
            headbytes = SECTOR_SIZE - openfiles[fd].cacheoffset;
            openfiles[fd].cacheoffset = -1;
        }

        /* eof? */
        if ( openfiles[fd].fileoffset + headbytes > openfiles[fd].size )
            headbytes = openfiles[fd].size - openfiles[fd].fileoffset;

        memcpy( buf, openfiles[fd].cache + offs, headbytes );
        nread = headbytes;
        count -= headbytes;
    }

    /* read whole sectors right into the supplied buffer */
    sectors = count / SECTOR_SIZE;
    if ( sectors ) {
        int rc = fat_read(&(openfiles[fd].fatfile), sectors, buf+nread );
        if ( rc < 0 ) {
            DEBUGF("Failed reading %d sectors\n",sectors);
            errno = EIO;
            return -1;
        }
        else {
            if ( rc > 0 ) {
                nread += sectors * SECTOR_SIZE;
                count -= sectors * SECTOR_SIZE;
            }
            else {
                /* eof */
                count=0;
            }

            openfiles[fd].cacheoffset = -1;
        }
    }

    /* any tail bytes? */
    if ( count ) {
        if ( fat_read(&(openfiles[fd].fatfile), 1,
                      &(openfiles[fd].cache)) < 0 ) {
            DEBUGF("Failed caching sector\n");
            errno = EIO;
            return -1;
        }

        /* eof? */
        if ( openfiles[fd].fileoffset + count > openfiles[fd].size )
            count = openfiles[fd].size - openfiles[fd].fileoffset;

        memcpy( buf + nread, openfiles[fd].cache, count );
        nread += count;
        openfiles[fd].cacheoffset = count;
    }

    openfiles[fd].fileoffset += nread;
    return nread;
}

int lseek(int fd, int offset, int whence)
{
    int pos;
    int newsector;
    int oldsector;
    int sectoroffset;
    int rc;

    if ( !openfiles[fd].busy ) {
        errno = EBADF;
        return -1;
    }

    switch ( whence ) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos = openfiles[fd].fileoffset + offset;
            break;

        case SEEK_END:
            pos = openfiles[fd].size - offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }
    if ( (pos < 0) ||
         (pos > openfiles[fd].size) ) {
        errno = EINVAL;
        return -1;
    }

    /* new sector? */
    newsector = pos / SECTOR_SIZE;
    oldsector = openfiles[fd].fileoffset / SECTOR_SIZE;
    sectoroffset = pos % SECTOR_SIZE;

    if ( (newsector != oldsector) ||
         ((openfiles[fd].cacheoffset==-1) && sectoroffset) ) {
        if ( newsector != oldsector ) {
            rc = fat_seek(&(openfiles[fd].fatfile), newsector);
            if ( rc < 0 ) {
                errno = EIO;
                return -1;
            }
        }
        rc = fat_read(&(openfiles[fd].fatfile), 1,
                      &(openfiles[fd].cache));
        if ( rc < 0 ) {
            errno = EIO;
            return -1;
        }
    }

    openfiles[fd].cacheoffset = sectoroffset;
    openfiles[fd].fileoffset = pos;

    return pos;
}

/*
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
