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
#include <stdbool.h>
#include "file.h"
#include "fat.h"
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
    bool write;
};

static struct filedesc openfiles[MAX_OPEN_FILES];

int creat(const char *pathname, int mode)
{
    (void)mode;
    return open(pathname, O_WRONLY);
}

int open(const char* pathname, int flags)
{
    DIR* dir;
    struct dirent* entry;
    int fd;
    char* name;

    LDEBUGF("open(\"%s\",%d)\n",pathname,flags);

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
        return -2;
    }

    switch ( flags ) {
        case O_RDONLY:
            openfiles[fd].write = false;
            break;

        case O_WRONLY:
            openfiles[fd].write = true;
            break;
            
        default:
            DEBUGF("Only O_RDONLY and O_WRONLY is supported\n");
            errno = EROFS;
            return -3;
    }
    openfiles[fd].busy = true;

    /* locate filename */
    name=strrchr(pathname+1,'/');
    if ( name ) {
        *name = 0;
        dir = opendir((char*)pathname);
        *name = '/';
        name++;
    }
    else {
        dir = opendir("/");
        name = (char*)pathname+1;
    }
    if (!dir) {
        DEBUGF("Failed opening dir\n");
        errno = EIO;
        openfiles[fd].busy = false;
        return -4;
    }

    /* scan dir for name */
    while ((entry = readdir(dir))) {
        if ( !strcasecmp(name, entry->d_name) ) {
            fat_open(entry->startcluster,
                     &(openfiles[fd].fatfile),
                     &(dir->fatdir));
            openfiles[fd].size = entry->size;
            break;
        }
    }
    closedir(dir);
    if ( !entry ) {
        LDEBUGF("Didn't find file %s\n",name);
        if ( openfiles[fd].write ) {
            if (fat_create_file(name,
                                &(openfiles[fd].fatfile),
                                &(dir->fatdir)) < 0) {
                DEBUGF("Couldn't create %s in %s\n",name,pathname);
                errno = EIO;
                openfiles[fd].busy = false;
                return -5;
            }
            openfiles[fd].size = 0;
        }
        else {
            DEBUGF("Couldn't find %s in %s\n",name,pathname);
            errno = ENOENT;
            openfiles[fd].busy = false;
            return -6;
        }
    }

    openfiles[fd].cacheoffset = -1;
    openfiles[fd].fileoffset = 0;
    return fd;
}

int close(int fd)
{
    int rc = 0;

    LDEBUGF("close(%d)\n",fd);

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!openfiles[fd].busy) {
        errno = EBADF;
        return -2;
    }
    if (openfiles[fd].write) {
        /* flush sector cache */
        if ( openfiles[fd].cacheoffset != -1 ) {
            if ( fat_readwrite(&(openfiles[fd].fatfile), 1,
                               &(openfiles[fd].cache),true) < 0 ) {
                DEBUGF("Failed flushing cache\n");
                errno = EIO;
                rc = -1;
            }
        }
        
        /* tie up all loose ends */
        fat_closewrite(&(openfiles[fd].fatfile), openfiles[fd].fileoffset);
    }
    openfiles[fd].busy = false;
    return rc;
}

int remove(const char* name)
{
    int rc;
    int fd = open(name, O_WRONLY);
    if ( fd < 0 )
        return fd;

    rc = fat_remove(&(openfiles[fd].fatfile));

    close(fd);

    return rc;
}

static int readwrite(int fd, void* buf, int count, bool write)
{
    int sectors;
    int nread=0;

    if ( !openfiles[fd].busy ) {
        errno = EBADF;
        return -1;
    }

    LDEBUGF( "readwrite(%d,%x,%d,%s)\n",
             fd,buf,count,write?"write":"read");

    /* attempt to read past EOF? */
    if (!write && count > openfiles[fd].size - openfiles[fd].fileoffset)
        count = openfiles[fd].size - openfiles[fd].fileoffset;

    /* any head bytes? */
    if ( openfiles[fd].cacheoffset != -1 ) {
        int headbytes;
        int offs = openfiles[fd].cacheoffset;
        if ( count <= SECTOR_SIZE - openfiles[fd].cacheoffset ) {
            headbytes = count;
            openfiles[fd].cacheoffset += count;
            if ( openfiles[fd].cacheoffset >= SECTOR_SIZE )
                openfiles[fd].cacheoffset = -1;
        }
        else {
            headbytes = SECTOR_SIZE - openfiles[fd].cacheoffset;
            openfiles[fd].cacheoffset = -1;
        }

        if (write) {
            memcpy( openfiles[fd].cache + offs, buf, headbytes );
            if (offs+headbytes == SECTOR_SIZE) {
                int rc = fat_readwrite(&(openfiles[fd].fatfile), 1,
                                       openfiles[fd].cache, true );
                if ( rc < 0 ) {
                    DEBUGF("Failed read/writing\n");
                    errno = EIO;
                    return -2;
                }
                openfiles[fd].cacheoffset = -1;
            }
        }
        else {
            memcpy( buf, openfiles[fd].cache + offs, headbytes );
        }

        nread = headbytes;
        count -= headbytes;
    }

    /* read whole sectors right into the supplied buffer */
    sectors = count / SECTOR_SIZE;
    if ( sectors ) {
        int rc = fat_readwrite(&(openfiles[fd].fatfile), sectors, 
                               buf+nread, write );
        if ( rc < 0 ) {
            DEBUGF("Failed read/writing %d sectors\n",sectors);
            errno = EIO;
            return -2;
        }
        else {
            if ( rc > 0 ) {
                nread += rc * SECTOR_SIZE;
                count -= sectors * SECTOR_SIZE;

                /* if eof, skip tail bytes */
                if ( rc < sectors )
                    count = 0;
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
        if (write) {
            memcpy( openfiles[fd].cache, buf + nread, count );
        }
        else {
            if ( fat_readwrite(&(openfiles[fd].fatfile), 1,
                               &(openfiles[fd].cache),false) < 1 ) {
                DEBUGF("Failed caching sector\n");
                errno = EIO;
                return -1;
            }
            memcpy( buf + nread, openfiles[fd].cache, count );
        }
            
        nread += count;
        openfiles[fd].cacheoffset = count;
    }

    openfiles[fd].fileoffset += nread;
    LDEBUGF("fileoffset: %d\n", openfiles[fd].fileoffset);

    /* adjust file size to length written */
    if ( write && openfiles[fd].fileoffset > openfiles[fd].size )
        openfiles[fd].size = openfiles[fd].fileoffset;

    return nread;
}

int write(int fd, void* buf, int count)
{
    return readwrite(fd, buf, count, true);
}

int read(int fd, void* buf, int count)
{
    return readwrite(fd, buf, count, false);
}


int lseek(int fd, int offset, int whence)
{
    int pos;
    int newsector;
    int oldsector;
    int sectoroffset;
    int rc;

    LDEBUGF("lseek(%d,%d,%d)\n",fd,offset,whence);

    if ( !openfiles[fd].busy ) {
        errno = EBADF;
        return -1;
    }

    if ( openfiles[fd].write ) {
        DEBUGF("lseek() is not supported when writing\n");
        errno = EROFS;
        return -2;
    }


    switch ( whence ) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos = openfiles[fd].fileoffset + offset;
            break;

        case SEEK_END:
            pos = openfiles[fd].size + offset;
            break;

        default:
            errno = EINVAL;
            return -2;
    }
    if ((pos < 0) || (pos > openfiles[fd].size)) {
        errno = EINVAL;
        return -3;
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
                return -4;
            }
        }
        if ( sectoroffset ) {
            rc = fat_readwrite(&(openfiles[fd].fatfile), 1,
                               &(openfiles[fd].cache),false);
            if ( rc < 0 ) {
                errno = EIO;
                return -5;
            }
            openfiles[fd].cacheoffset = sectoroffset;
        }
        else
            openfiles[fd].cacheoffset = -1;
    }
    else
        if ( openfiles[fd].cacheoffset != -1 )
            openfiles[fd].cacheoffset = sectoroffset;

    openfiles[fd].fileoffset = pos;

    return pos;
}

/*
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
