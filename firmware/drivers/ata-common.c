/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2024 Solomon Peachy
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

/* This is intended to be #included into the ATA driver */

#ifdef MAX_PHYS_SECTOR_SIZE

#ifdef MAX_LOG_SECTOR_SIZE
#define __MAX_LOG_SECTOR_SIZE MAX_LOG_SECTOR_SIZE
#else
#define __MAX_LOG_SECTOR_SIZE SECTOR_SIZE
#endif

struct sector_cache_entry {
    unsigned char data[MAX_PHYS_SECTOR_SIZE];
    sector_t sectornum;  /* logical sector */
    bool inuse;
};
/* buffer for reading and writing large physical sectors */
static struct sector_cache_entry sector_cache STORAGE_ALIGN_ATTR;
static uint16_t phys_sector_mult = 1;

static int cache_sector(sector_t sector)
{
    int rc;

    /* round down to physical sector boundary */
    sector &= ~(phys_sector_mult - 1);

    /* check whether the sector is already cached */
    if (sector_cache.inuse && (sector_cache.sectornum == sector))
        return 0;

    /* not found: read the sector */
    sector_cache.inuse = false;
    rc = ata_transfer_sectors(sector, phys_sector_mult, sector_cache.data, false);
    if (!rc)
    {
        sector_cache.sectornum = sector;
        sector_cache.inuse = true;
    }
    return rc;
}

static inline int flush_current_sector(void)
{
    return ata_transfer_sectors(sector_cache.sectornum, phys_sector_mult,
                                sector_cache.data, true);
}

int ata_read_sectors(IF_MD(int drive,)
                     sector_t start,
                     int incount,
                     void* inbuf)
{
    int rc = 0;
    int offset;

#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mutex);

    offset = start & (phys_sector_mult - 1);

    if (offset) /* first partial sector */
    {
        int partcount = MIN(incount, phys_sector_mult - offset);

        rc = cache_sector(start);
        if (rc)
        {
            rc = rc * 10 - 1;
            goto error;
        }
        memcpy(inbuf, sector_cache.data + offset * log_sector_size,
               partcount * log_sector_size);

        start += partcount;
        inbuf += partcount * log_sector_size;
        incount -= partcount;
    }
    if (incount)
    {
        offset = incount & (phys_sector_mult - 1);
        incount -= offset;

        if (incount)
        {
            rc = ata_transfer_sectors(start, incount, inbuf, false);
            if (rc)
            {
                rc = rc * 10 - 2;
                goto error;
            }
            start += incount;
            inbuf += incount * log_sector_size;
        }
        if (offset)
        {
            rc = cache_sector(start);
            if (rc)
            {
                rc = rc * 10 - 3;
                goto error;
            }
            memcpy(inbuf, sector_cache.data, offset * log_sector_size);
        }
    }

  error:
    mutex_unlock(&ata_mutex);

    return rc;
}

int ata_write_sectors(IF_MD(int drive,)
                      sector_t start,
                      int count,
                      const void* buf)
{
    int rc = 0;
    int offset;

#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mutex);

    offset = start & (phys_sector_mult - 1);

    if (offset) /* first partial sector */
    {
        int partcount = MIN(count, phys_sector_mult - offset);

        rc = cache_sector(start);
        if (rc)
        {
            rc = rc * 10 - 1;
            goto error;
        }
        memcpy(sector_cache.data + offset * log_sector_size, buf,
               partcount * log_sector_size);
        rc = flush_current_sector();
        if (rc)
        {
            rc = rc * 10 - 2;
            goto error;
        }
        start += partcount;
        buf += partcount * log_sector_size;
        count -= partcount;
    }
    if (count)
    {
        offset = count & (phys_sector_mult - 1);
        count -= offset;

        if (count)
        {
            rc = ata_transfer_sectors(start, count, (void*)buf, true);
            if (rc)
            {
                rc = rc * 10 - 3;
                goto error;
            }
            start += count;
            buf += count * log_sector_size;
        }
        if (offset)
        {
            rc = cache_sector(start);
            if (rc)
            {
                rc = rc * 10 - 4;
                goto error;
            }
            memcpy(sector_cache.data, buf, offset * log_sector_size);
            rc = flush_current_sector();
            if (rc)
            {
                rc = rc * 10 - 5;
                goto error;
            }
        }
    }

  error:
    mutex_unlock(&ata_mutex);

    return rc;
}

static int ata_get_phys_sector_mult(void)
{
    int rc = 0;

    /* Find out the physical sector size */
    if((identify_info[106] & 0xe000) == 0x6000) /* B14, B13 */
        phys_sector_mult = BIT_N(identify_info[106] & 0x000f);
    else
        phys_sector_mult = 1;

    DEBUGF("ata: %d logical sectors per phys sector", phys_sector_mult);

    if (phys_sector_mult > 1)
    {
        /* Check if drive really needs emulation  - if we can access
           sector 1 then assume the drive supports "512e" and will handle
           it better than us, so ignore the large physical sectors.
        */
        char throwaway[__MAX_LOG_SECTOR_SIZE];
        rc = ata_transfer_sectors(1, 1, &throwaway, false);
        if (rc == 0)
            phys_sector_mult = 1;
    }

    if (phys_sector_mult > (MAX_PHYS_SECTOR_SIZE/log_sector_size))
        panicf("Unsupported physical sector size: %ld",
               phys_sector_mult * log_sector_size);

    memset(&sector_cache, 0, sizeof(sector_cache));

    return 0;
}

#endif  /* MAX_PHYS_SECTOR_SIZE */
