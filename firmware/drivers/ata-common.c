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

#ifdef MAX_VARIABLE_LOG_SECTOR
#define __MAX_VARIABLE_LOG_SECTOR MAX_VARIABLE_LOG_SECTOR
#else
#define __MAX_VARIABLE_LOG_SECTOR SECTOR_SIZE
#endif

#ifndef STORAGE_ALIGN_ATTR
#define STORAGE_ALIGN_ATTR __attribute__((aligned(sizeof(uint32_t))))
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

    if (offset) /* first partial physical sector */
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
    offset = incount & (phys_sector_mult - 1);
    incount -= offset;

    if (incount) /* all complete physical sectors */
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

    if (offset) /* Trailing partial logical sector */
    {
        rc = cache_sector(start);
        if (rc)
        {
            rc = rc * 10 - 3;
            goto error;
        }
        memcpy(inbuf, sector_cache.data, offset * log_sector_size);
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

    if (offset) /* first partial physical sector */
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

    offset = count & (phys_sector_mult - 1);
    count -= offset;

    if (count) /* all complete physical sectors */
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

    if (offset) /* Trailing partial logical sector */
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

    if((identify_info[209] & 0xc000) == 0x4000) { /* B14 */
        if (identify_info[209] & 0x3fff) {
            panicf("ata: Unaligned logical/physical sector mapping");
            // XXX we can probably handle this by adding a fixed offset
            // to all operations and subtracting this from the reported
            // size.  But we don't do tihs until we find a real-world need.
        }
    }

    if (phys_sector_mult > 1)
    {
        /* Check if drive really needs emulation  - if we can access
           sector 1 then assume the drive supports "512e" and will handle
           it better than us, so ignore the large physical sectors.

           The exception here is if the device is partitioned to use
           larger-than-logical "virtual" sectors; in that case we will
           use whichever one (ie physical/"virtual") is larger.
        */
        char throwaway[__MAX_VARIABLE_LOG_SECTOR];
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

void ata_set_phys_sector_mult(unsigned int mult)
{
    unsigned int max = MAX_PHYS_SECTOR_SIZE/log_sector_size;
    /* virtual sector could be larger than pyhsical sector */
    if (!mult || mult > max)
        mult = max;
    /* It needs to be at _least_ the size of the real multiplier */
    if (mult > phys_sector_mult)
        phys_sector_mult = mult;
}

#endif  /* MAX_PHYS_SECTOR_SIZE */
