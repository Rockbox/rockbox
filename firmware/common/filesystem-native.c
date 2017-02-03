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
#include "config.h"
#include "system.h"
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "file.h"
#include "fileobj_mgr.h"
#include "disk_cache.h"
#include "rb_namespace.h"
#include "string-extra.h"

/* return the file size in sectors */
static inline unsigned long filesize_sectors(file_size_t size)
{
    /* overflow proof whereas "(x + y - 1) / y" is not */
    unsigned long numsectors = size / SECTOR_SIZE;

    if (size % SECTOR_SIZE)
        numsectors++;

    return numsectors;
}

/* flush a dirty cache buffer */
static int flush_cache(struct filestr_desc *file)
{
    int rc;
    struct filestr_cache *cachep = file->stream.cachep;

    DEBUGF("Flushing dirty sector cache (%lu)\n", cachep->sector);

    if (fctrl_io_query_sectornum(&file->stream) != cachep->sector)
    {
        /* get on the correct sector */
        rc = fctrl_io_seek(&file->stream, cachep->sector);
        if (rc < 0)
            FILE_ERROR(EIO, rc * 10 - 1);
    }

    rc = ctrl_io_readwrite(&file->stream, 1, cachep->buffer, true);
    if (rc < 0)
    {
        if (rc == FAT_RC_ENOSPC)
            FILE_ERROR(ENOSPC, RC);
        else
            FILE_ERROR(EIO, rc * 10 - 2);
    }

    cachep->flags = 0;
    return 1;
file_error:
    DEBUGF("Failed flushing cache: %d\n", rc);
    return rc;
}

static void discard_cache(struct filestr_desc *file)
{
    struct filestr_cache *const cachep = file->stream.cachep;
    cachep->flags = 0;
}

/* fill a cache buffer with a new sector */
static int readwrite_fill_cache(struct filestr_desc *file, unsigned long sector,
                                unsigned long filesectors, bool write)
{
    /* sector != cachep->sector should have been checked by now */

    int rc;
    struct filestr_cache *cachep = filestr_get_cache(&file->stream);

    if (cachep->flags & FSC_DIRTY)
    {
        rc = flush_cache(file);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 1);
    }

    if (fctrl_io_query_sectornum(&file->stream) != sector)
    {
        /* get on the correct sector */
        rc = fctrl_io_seek(&file->stream, sector);
        if (rc < 0)
            FILE_ERROR(EIO, rc * 10 - 2);
    }

    if (!write || sector < filesectors)
    {
        /* only reading or this sector would have been flushed if the cache
           was previously needed for a different sector */
        rc = fctrl_io_readwrite(&file->stream, 1, cachep->buffer, false);
        if (rc < 0)
            FILE_ERROR(rc == FAT_RC_ENOSPC ? ENOSPC : EIO, rc * 10 - 3);
    }
    else
    {
        /* create a fresh, shiny, new sector with that new sector smell */
        cachep->flags = FSC_NEW;
    }

    cachep->sector = sector;
    return 1;
file_error:
    DEBUGF("Failed caching sector: %d\n", rc);
    return rc;
}

/* read or write to part or all of the cache buffer */
static inline void readwrite_cache(struct filestr_cache *cachep, void *buf,
                                   unsigned long secoffset, size_t nbyte,
                                   bool write)
{
    void *dst, *cbufp = cachep->buffer + secoffset;

    if (write)
    {
        dst = cbufp;
        cachep->flags |= FSC_DIRTY;
    }
    else
    {
        dst = buf;
        buf = cbufp;
    }

    memcpy(dst, buf, nbyte);
}

/* read or write a partial sector using the file's cache */
static inline ssize_t readwrite_partial(struct filestr_desc *file,
                                        struct filestr_cache *cachep,
                                        unsigned long sector,
                                        unsigned long secoffset,
                                        void *buf,
                                        size_t nbyte,
                                        unsigned long filesectors,
                                        unsigned int flags)
{
    if (sector != cachep->sector)
    {
        /* wrong sector in buffer */
        int rc = readwrite_fill_cache(file, sector, filesectors, flags);
        if (rc <= 0)
            return rc;
    }

    readwrite_cache(cachep, buf, secoffset, nbyte, flags);
    return nbyte;
}

/* read from or write to the file; back end to read() and write() */
ssize_t file_readwrite(struct filestr_desc *file, void *buf, size_t nbyte, bool write)
{
    DEBUGF("readwrite(%p,%lx,%lu,%s)\n",
           file, (long)buf, (unsigned long)nbyte, write ? "write" : "read");

    const file_size_t size = *file->sizep;
    file_size_t filerem;

    if (write)
    {
        /* if opened in append mode, move pointer to end */
        if (file->stream.flags & FD_APPEND)
            file->offset = MIN(size, FILE_SIZE_MAX);

        filerem = FILE_SIZE_MAX - file->offset;
    }
    else
    {
        /* limit to maximum possible offset (EOF or FILE_SIZE_MAX) */
        filerem = MIN(size, FILE_SIZE_MAX) - file->offset;
    }

    if (nbyte > filerem)
    {
        nbyte = filerem;
        if (nbyte > 0)
            {}
        else if (write)
            FILE_ERROR_RETURN(EFBIG, -1);     /* would get too large */
        else if (file->offset >= FILE_SIZE_MAX)
            FILE_ERROR_RETURN(EOVERFLOW, -2); /* can't read here */
    }

    if (nbyte == 0)
        return 0;

    int rc = 0;

    struct filestr_cache * const cachep = file->stream.cachep;
    void * const bufstart = buf;

    const unsigned long filesectors = filesize_sectors(size);
    unsigned long sector = file->offset / SECTOR_SIZE;
    unsigned long sectoroffs = file->offset % SECTOR_SIZE;

    /* any head bytes? */
    if (sectoroffs)
    {
        size_t headbytes = MIN(nbyte, SECTOR_SIZE - sectoroffs);
        rc = readwrite_partial(file, cachep, sector, sectoroffs, buf, headbytes,
                               filesectors, write);
        if (rc <= 0)
        {
            if (rc < 0)
                FILE_ERROR(ERRNO, rc * 10 - 3);

            nbyte = 0; /* eof, skip the rest */
        }
        else
        {
            buf += rc;
            nbyte -= rc;
            sector++;  /* if nbyte goes to 0, the rest is skipped anyway */
        }
    }

    /* read/write whole sectors right into/from the supplied buffer */
    unsigned long sectorcount = nbyte / SECTOR_SIZE;

    while (sectorcount)
    {
        unsigned long runlen = sectorcount;

        /* if a cached sector is inside the transfer range, split the transfer
           into two parts and use the cache for that sector to keep it coherent
           without writeback */
        if (UNLIKELY(cachep->sector >= sector &&
                     cachep->sector < sector + sectorcount))
        {
            runlen = cachep->sector - sector;
        }

        if (runlen)
        {
            if (fctrl_io_query_sectornum(&file->stream) != sector)
            {
                /* get on the correct sector */
                rc = 0;

                /* If the dirty bit isn't set, we're somehow beyond the file
                   size and you can't explain _that_ */
                if (sector >= filesectors && cachep->flags == (FSC_NEW|FSC_DIRTY))
                {
                    rc = flush_cache(file);
                    if (rc < 0)
                        FILE_ERROR(ERRNO, rc * 10 - 4);

                    if (cachep->sector + 1 == sector)
                        rc = 1; /* if now ok, don't seek */
                }

                if (rc == 0)
                {
                    rc = fctrl_io_seek(&file->stream, sector);
                    if (rc < 0)
                        FILE_ERROR(EIO, rc * 10 - 5);
                }
            }

            rc = fctrl_io_readwrite(&file->stream, runlen, buf, write);
            if (rc < 0)
            {
                DEBUGF("I/O error %sing %ld sectors\n",
                       write ? "writ" : "read", runlen);
                FILE_ERROR(rc == FAT_RC_ENOSPC ? ENOSPC : EIO,
                           rc * 10 - 6);
            }
            else
            {
                buf += rc * SECTOR_SIZE;
                nbyte -= rc * SECTOR_SIZE;
                sector += rc;
                sectorcount -= rc;

                /* if eof, skip tail bytes */
                if ((unsigned long)rc < runlen)
                    nbyte = 0;

                if (!nbyte)
                    break;
            }
        }

        if (UNLIKELY(sectorcount && sector == cachep->sector))
        {
            /* do this one sector with the cache */
            readwrite_cache(cachep, buf, 0, SECTOR_SIZE, write);
            buf += SECTOR_SIZE;
            nbyte -= SECTOR_SIZE;
            sector++;
            sectorcount--;
        }
    }

    /* any tail bytes? */
    if (nbyte)
    {
        /* tail bytes always start at sector offset 0 */
        rc = readwrite_partial(file, cachep, sector, 0, buf, nbyte,
                               filesectors, write);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 7);

        buf += rc;
    }

file_error:;
#ifdef DEBUG
    if (errno == ENOSPC)
        DEBUGF("No space left on device\n");
#endif

    size_t done = buf - bufstart;
    if (done)
    {
        /* error or not, update the file offset and size if anything was
           transferred */
        file->offset += done;
        DEBUGF("file offset: %ld\n", file->offset);

        /* adjust file size to length written */
        if (write && file->offset > size)
            *file->sizep = file->offset;

        if (rc > 0)
            return done;
    }

    return rc;
}

/* Handle syncing all file's streams to the truncation */ 
static void handle_truncate(struct filestr_desc * const file, file_size_t size)
{
    unsigned long filesectors = filesize_sectors(size);

    struct filestr_base *s = NULL;
    while ((s = fileobj_get_next_stream(&file->stream, s)))
    {
        /* caches with data beyond new extents are invalid */
        unsigned long sector = s->cachep->sector;
        if (sector != INVALID_SECNUM && sector >= filesectors)
            filestr_discard_cache(s);

        /* files outside bounds must be rewound */
        if (fctrl_io_query_sectornum(&s->ioctrl) > filesectors)
            fctrl_io_seek_to_stream(&s->ioctrl, &file->stream.ioctrl);

        /* clip file offset too if needed */
        struct filestr_desc *f = (struct filestr_desc *)s;
        if (f->offset > size)
            f->offset = size;
    }
}

/* truncate the file to the specified length */
int file_ftruncate(struct filestr_desc *file, file_size_t size, bool write_now)
{
    int rc = 0, rc2 = 1;

    file_size_t cursize = *file->sizep;
    file_size_t truncsize = MIN(size, cursize);

    if (write_now)
    {
        unsigned long sector = filesize_sectors(truncsize);
        struct filestr_cache *const cachep = file->stream.cachep;

        if (cachep->flags == (FSC_NEW|FSC_DIRTY) &&
            cachep->sector + 1 == sector)
        {
            /* sector created but may have never been added to the cluster
               chain; flush it now or the subsequent may fail */
            rc2 = flush_cache(file);
            if (rc2 == FAT_RC_ENOSPC)
            {
                /* no space left on device; further truncation needed */
                discard_cache(file);
                truncsize = ALIGN_DOWN(truncsize - 1, SECTOR_SIZE);
                sector--;
                rc = rc2;
            }
            else if (rc2 < 0)
                FILE_ERROR(ERRNO, rc2 * 10 - 1);
        }

        rc2 = fctrl_io_seek(&file->stream, sector);
        if (rc2 < 0)
            FILE_ERROR(EIO, rc2 * 10 - 2);

        rc2 = fctrl_io_truncate(&file->stream.ioctrl);
        if (rc2 < 0)
            FILE_ERROR(EIO, rc2 * 10 - 3);

        /* never needs to be done this way again since any data beyond the
           cached size is now gone */
        fileobj_change_flags(&file->stream, 0, FO_TRUNC);
    }
    /* else just change the cached file size */

    if (truncsize < cursize)
    {
        *file->sizep = truncsize;
        handle_truncate(file, truncsize);
    }

    /* if truncation was partially successful, it effectively destroyed
       everything after the truncation point; still, indicate failure
       after adjusting size */
    if (rc2 == 0)
        FILE_ERROR(EIO, -4);
    else if (rc2 < 0)
        FILE_ERROR(ERRNO, rc2);

file_error:
    return rc;
}

/* set the file pointer */
off_t file_lseek(struct filestr_desc *file, off_t offset, int whence)
{
    off_t rc;
    file_size_t pos;

    file_size_t size = MIN(*file->sizep, FILE_SIZE_MAX);

    switch (whence)
    {
    case SEEK_SET:
        if (offset < 0 || (file_size_t)offset > size)
            FILE_ERROR(EINVAL, -1);

        pos = offset;
        break;

    case SEEK_CUR:
        if ((offset < 0 && (file_size_t)-offset > file->offset) ||
            (offset > 0 && (file_size_t)offset > size - file->offset))
            FILE_ERROR(EINVAL, -1);

        pos = file->offset + offset;
        break;

    case SEEK_END:
        if (offset > 0 || (file_size_t)-offset > size)
            FILE_ERROR(EINVAL, -1);

        pos = size + offset;
        break;

    default:
        FILE_ERROR(EINVAL, -1);
    }

    file->offset = pos;

    return pos;
file_error:
    return rc;
}

/* flush back all outstanding writes to the file */
int file_fsync(struct filestr_desc *file)
{
    /* call only when holding WRITER lock (updates directory entries) */
    int rc = 0;

    file_size_t size = *file->sizep;
    unsigned int foflags = fileobj_get_flags(&file->stream);

    /* flush sector cache? */
    struct filestr_cache *const cachep = file->stream.cachep;
    if (cachep->flags & FSC_DIRTY)
    {
        int rc2 = flush_cache(file);
        if (rc2 == FAT_RC_ENOSPC && (cachep->flags & FSC_NEW))
        {
            /* no space left on device so this must be dropped */
            discard_cache(file);
            size = ALIGN_DOWN(size - 1, SECTOR_SIZE);
            foflags |= FO_TRUNC;
            rc = rc2;
        }
        else if (rc2 < 0)
            FILE_ERROR(ERRNO, rc2 * 10 - 1);
    }

    /* truncate? */
    if (foflags & FO_TRUNC)
    {
        int rc2 = file_ftruncate(file, size, rc == 0);
        if (rc2 < 0)
            FILE_ERROR(ERRNO, rc2 * 10 - 2);
    }

file_error:;
    /* tie up all loose ends (try to sync the file even if failing) */
    int rc2 = fctrl_io_sync(&file->stream.ioctrl, size);
    if (rc2 >= 0)
        fileop_onsync_internal(&file->stream);

    if (rc2 < 0 && rc >= 0)
    {
        errno = EIO;
        rc = rc2 * 10 - 3;
    }

    return rc;
}
