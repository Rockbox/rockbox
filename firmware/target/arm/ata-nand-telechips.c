/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rob Purchase
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
#include "nand.h"
#include "ata-nand-target.h"
#include "system.h"
#include <string.h>
#include "led.h"
#include "panic.h"
#include "nand_id.h"
#include "storage.h"
#include "buffer.h"

#define SECTOR_SIZE 512

/* #define USE_ECC_CORRECTION */

/* for compatibility */
int ata_spinup_time = 0;

long last_disk_activity = -1;

/** static, private data **/
static bool initialized = false;

static struct mutex ata_mtx SHAREDBSS_ATTR;

#if defined(COWON_D2) || defined(IAUDIO_7)
#define FTL_V2
#define MAX_WRITE_CACHES    8
#else
#define FTL_V1
#define MAX_WRITE_CACHES    4
#endif

/* Sector type identifiers - main data area */

#define SECTYPE_MAIN_LPT           0x12
#define SECTYPE_MAIN_DATA          0x13
#define SECTYPE_MAIN_RANDOM_CACHE  0x15
#define SECTYPE_MAIN_INPLACE_CACHE 0x17

/* We don't touch the hidden area at all - these are for reference */
#define SECTYPE_HIDDEN_LPT           0x22
#define SECTYPE_HIDDEN_DATA          0x23
#define SECTYPE_HIDDEN_RANDOM_CACHE  0x25
#define SECTYPE_HIDDEN_INPLACE_CACHE 0x27

#ifdef FTL_V1
#define SECTYPE_FIRMWARE 0x40
#else
#define SECTYPE_FIRMWARE 0xE0
#endif

/* Offsets to data within sector's spare area */

#define OFF_CACHE_PAGE_LOBYTE 2
#define OFF_CACHE_PAGE_HIBYTE 3
#define OFF_SECTOR_TYPE       4

#ifdef FTL_V2
#define OFF_LOG_SEG_LOBYTE    7
#define OFF_LOG_SEG_HIBYTE    6
#else
#define OFF_LOG_SEG_LOBYTE    6
#define OFF_LOG_SEG_HIBYTE    7
#endif

/* Chip characteristics, initialised by nand_get_chip_info() */

static struct nand_info* nand_data = NULL;

static int total_banks         = 0;
static int pages_per_bank      = 0;
static int sectors_per_page    = 0;
static int bytes_per_segment   = 0;
static int sectors_per_segment = 0;
static int segments_per_bank   = 0;
static int pages_per_segment   = 0;

/* Maximum values for static buffers */

#define MAX_PAGE_SIZE          4096
#define MAX_SPARE_SIZE         128
#define MAX_BLOCKS_PER_BANK    8192
#define MAX_PAGES_PER_BLOCK    128
#define MAX_BANKS              4
#define MAX_BLOCKS_PER_SEGMENT 4

#define MAX_SEGMENTS (MAX_BLOCKS_PER_BANK * MAX_BANKS / MAX_BLOCKS_PER_SEGMENT)

/* Logical/Physical translation table */

struct lpt_entry
{
    short bank;
    short phys_segment;
};
#ifdef BOOTLOADER
static struct lpt_entry lpt_lookup[MAX_SEGMENTS];
#else
/* buffer_alloc'd in nand_init() when the correct size has been determined */
static struct lpt_entry* lpt_lookup = NULL;
#endif

/* Write Caches */

struct write_cache
{
    short log_segment;
    short inplace_bank;
    short inplace_phys_segment;
    short inplace_pages_used;
    short random_bank;
    short random_phys_segment;
    short page_map[MAX_PAGES_PER_BLOCK * MAX_BLOCKS_PER_SEGMENT];
};
static struct write_cache write_caches[MAX_WRITE_CACHES];

static int write_caches_in_use = 0;

#ifdef USE_ECC_CORRECTION
static unsigned int ecc_sectors_corrected = 0;
static unsigned int ecc_bits_corrected = 0;
static unsigned int ecc_fail_count = 0;
#endif


/* Conversion functions */

static int phys_segment_to_page_addr(int phys_segment, int page_in_seg)
{
    int page_addr = 0;

    switch (nand_data->planes)
    {
        case 1:
        {
            page_addr = (phys_segment * nand_data->pages_per_block);
            break;
        }

        case 2:
        case 4:
        {
            page_addr = phys_segment * nand_data->pages_per_block * 2;

            if (page_in_seg & 1)
            {
                /* Data is located in block+1 */
                page_addr += nand_data->pages_per_block;
            }

            if (nand_data->planes == 4 && page_in_seg & 2)
            {
                /* Data is located in 2nd half of bank */
                page_addr +=
                    (nand_data->blocks_per_bank/2) * nand_data->pages_per_block;
            }

            break;
        }
    }
    
    page_addr += (page_in_seg / nand_data->planes);

    return page_addr;
}


/* NAND physical access functions */

static void nand_chip_select(int bank)
{
    if (bank == -1)
    {
        /* Disable both chip selects */
        NAND_GPIO_CLEAR(CS_GPIO_BIT);
        NFC_CTRL |= NFC_CS0 | NFC_CS1;
    }
    else
    {
        /* NFC chip select */
        if (bank & 1)
        {
            NFC_CTRL &= ~NFC_CS0;
            NFC_CTRL |= NFC_CS1;
        }
        else
        {
            NFC_CTRL |= NFC_CS0;
            NFC_CTRL &= ~NFC_CS1;
        }

        /* Secondary chip select */
        if (bank & 2)
            NAND_GPIO_SET(CS_GPIO_BIT);
        else
            NAND_GPIO_CLEAR(CS_GPIO_BIT);
    }
}


static void nand_read_id(int bank, unsigned char* id_buf)
{
    int i;
    
    /* Enable NFC bus clock */
    BCLKCTR |= DEV_NAND;

    /* Reset NAND controller */
    NFC_RST = 0;

    /* Set slow cycle timings since the chip is as yet unidentified */
    NFC_CTRL = (NFC_CTRL &~0xFFF) | 0x353;

    nand_chip_select(bank);

    /* Set write protect */
    NAND_GPIO_CLEAR(WE_GPIO_BIT);

    /* Reset command */
    NFC_CMD = 0xFF;

    /* Set 8-bit data width */
    NFC_CTRL &= ~NFC_16BIT;

    /* Read ID command, single address cycle */
    NFC_CMD   = 0x90;
    NFC_SADDR = 0x00;

    /* Read the 5 chip ID bytes */
    for (i = 0; i < 5; i++)
    {
        id_buf[i] = NFC_SDATA & 0xFF;
    }

    nand_chip_select(-1);

    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


static void nand_read_uid(int bank, unsigned int* uid_buf)
{
    int i;

    /* Enable NFC bus clock */
    BCLKCTR |= DEV_NAND;

    /* Set cycle timing (stp = 1, pw = 3, hold = 1) */
    NFC_CTRL = (NFC_CTRL &~0xFFF) | 0x131;

    nand_chip_select(bank);

    /* Set write protect */
    NAND_GPIO_CLEAR(WE_GPIO_BIT);

    /* Set 8-bit data width */
    NFC_CTRL &= ~NFC_16BIT;

    /* Undocumented (SAMSUNG specific?) commands set the chip into a
       special mode allowing a normally-hidden UID block to be read. */
    NFC_CMD = 0x30;
    NFC_CMD = 0x65;

    /* Read command */
    NFC_CMD = 0x00;

    /* Write row/column address */
    for (i = 0; i < nand_data->col_cycles; i++) NFC_SADDR = 0;
    for (i = 0; i < nand_data->row_cycles; i++) NFC_SADDR = 0;

    /* End of read */
    NFC_CMD = 0x30;

    /* Wait until complete */
    while (!(NFC_CTRL & NFC_READY)) {};

    /* Copy data to buffer (data repeats after 8 words) */
    for (i = 0; i < 8; i++)
    {
        uid_buf[i] = NFC_WDATA;
    }

    /* Reset the chip back to normal mode */
    NFC_CMD = 0xFF;

    nand_chip_select(-1);

    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


static void nand_read_raw(int bank, int row, int column, int size, void* buf)
{
    int i;

    /* Enable NFC bus clock */
    BCLKCTR |= DEV_NAND;

    /* Set cycle timing (stp = 1, pw = 3, hold = 1) */
    NFC_CTRL = (NFC_CTRL &~0xFFF) | 0x131;

    nand_chip_select(bank);

    /* Set write protect */
    NAND_GPIO_CLEAR(WE_GPIO_BIT);

    /* Set 8-bit data width */
    NFC_CTRL &= ~NFC_16BIT;

    /* Read command */
    NFC_CMD = 0x00;

    /* Write column address */
    for (i = 0; i < nand_data->col_cycles; i++)
    {
        NFC_SADDR = column & 0xFF;
        column = column >> 8;
    }

    /* Write row address */
    for (i = 0; i < nand_data->row_cycles; i++)
    {
        NFC_SADDR = row & 0xFF;
        row = row >> 8;
    }

    /* End of read command */
    NFC_CMD = 0x30;

    /* Wait until complete */
    while (!(NFC_CTRL & NFC_READY)) {};

    /* Read data into page buffer */
    if (((unsigned int)buf & 3) || (size & 3))
    {
        /* Use byte copy since either the buffer or size are not word-aligned */
        /* TODO: Byte copy only where necessary (use words for mid-section) */
        for (i = 0; i < size; i++)
        {
            ((unsigned char*)buf)[i] = NFC_SDATA;
        }
    }
    else
    {
        /* Use 4-byte copy as buffer and size are both word-aligned */
        for (i = 0; i < (size/4); i++)
        {
            ((unsigned int*)buf)[i] = NFC_WDATA;
        }
    }

    nand_chip_select(-1);
        
    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


static void nand_get_chip_info(void)
{
    unsigned char manuf_id;
    unsigned char id_buf[8];

    /* Read chip id from bank 0 */
    nand_read_id(0, id_buf);

    manuf_id = id_buf[0];

    /* Identify the chip geometry */
    nand_data = nand_identify(id_buf);

    if (nand_data == NULL)
    {
        panicf("Unknown NAND: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                id_buf[0],id_buf[1],id_buf[2],id_buf[3],id_buf[4]);
    }

    pages_per_bank = nand_data->blocks_per_bank * nand_data->pages_per_block;

    segments_per_bank = nand_data->blocks_per_bank / nand_data->planes;

    bytes_per_segment = nand_data->page_size * nand_data->pages_per_block
        * nand_data->planes;

    sectors_per_page = nand_data->page_size / SECTOR_SIZE;

    sectors_per_segment = bytes_per_segment / SECTOR_SIZE;
    
    pages_per_segment = sectors_per_segment / sectors_per_page;

    /* Establish how many banks are present */
    nand_read_id(1, id_buf);

    if (id_buf[0] == manuf_id)
    {
        /* Bank 1 is populated, now check if banks 2/3 are valid */
        nand_read_id(2, id_buf);

        if (id_buf[0] == manuf_id)
        {
            /* Bank 2 returned matching id - check if 2/3 are shadowing 0/1 */
            unsigned int uid_buf0[8];
            unsigned int uid_buf2[8];

            nand_read_uid(0, uid_buf0);
            nand_read_uid(2, uid_buf2);

            if (memcmp(uid_buf0, uid_buf2, 32) == 0)
            {
                /* UIDs match, assume banks 2/3 are shadowing 0/1 */
                total_banks = 2;
            }
            else
            {
                /* UIDs differ, assume banks 2/3 are valid */
                total_banks = 4;
            }
        }
        else
        {
            /* Bank 2 returned differing id - assume 2/3 are junk */
            total_banks = 2;
        }
    }
    else
    {
        /* Bank 1 returned differing id - assume it is junk */
        total_banks = 1;
    }

    /*
       Sanity checks:
       1. "BMP" tag at block 0, page 0, offset <page_size> [always present]
       2. On most D2s, <page_size>+3 is 'M' and <page_size>+4 is no. of banks.
          This is not present on some older players (formatted with early FW?)
     */

    nand_read_raw(0, 0,                 /* bank, page */
                  nand_data->page_size, /* offset */
                  8, id_buf);           /* length, dest */

    if (strncmp(id_buf, "BMP", 3)) panicf("BMP tag not present");

    if (id_buf[3] == 'M')
    {
        if (id_buf[4] != total_banks) panicf("BMPM total_banks mismatch");
    }
}


static bool nand_read_sector_of_phys_page(int bank, int page,
                                          int sector, void* buf)
{
#ifndef USE_ECC_CORRECTION
    nand_read_raw(bank, page,
                  sector * (SECTOR_SIZE+16),
                  SECTOR_SIZE, buf);
    return true;
#else
    /* Not yet implemented */
    return false;
#endif
}


static bool nand_read_sector_of_phys_segment(int bank, int phys_segment,
                                             int page_in_seg, int sector,
                                             void* buf)
{
    int page_addr = phys_segment_to_page_addr(phys_segment,
                                              page_in_seg);

    return nand_read_sector_of_phys_page(bank, page_addr, sector, buf);
}


static bool nand_read_sector_of_logical_segment(int log_segment, int sector,
                                                void* buf)
{
    int page_in_segment = sector / sectors_per_page;
    int sector_in_page  = sector % sectors_per_page;

    int bank = lpt_lookup[log_segment].bank;
    int phys_segment = lpt_lookup[log_segment].phys_segment;

    /* Check if any of the write caches refer to this segment/page.
       If present we need to read the cached page instead. */

    int cache_num = 0;
    bool found = false;
    
    while (!found && cache_num < write_caches_in_use)
    {
        if (write_caches[cache_num].log_segment == log_segment)
        {
            if (write_caches[cache_num].page_map[page_in_segment] != -1)
            {
                /* data is located in random pages cache */
                found = true;
                
                bank = write_caches[cache_num].random_bank;
                phys_segment = write_caches[cache_num].random_phys_segment;
                
                page_in_segment =
                    write_caches[cache_num].page_map[page_in_segment];
            }
            else if (write_caches[cache_num].inplace_pages_used != -1 &&
                     write_caches[cache_num].inplace_pages_used > page_in_segment)
            {
                /* data is located in in-place pages cache */
                found = true;
                
                bank = write_caches[cache_num].inplace_bank;
                phys_segment = write_caches[cache_num].inplace_phys_segment;
            }
        }
        cache_num++;
    }

    return nand_read_sector_of_phys_segment(bank, phys_segment,
                                            page_in_segment,
                                            sector_in_page, buf);
}


/* Miscellaneous helper functions */

static inline unsigned char get_sector_type(char* spare_buf)
{
    return spare_buf[OFF_SECTOR_TYPE];
}

static inline unsigned short get_log_segment_id(int phys_seg, char* spare_buf)
{
    (void)phys_seg;
    
    return ((spare_buf[OFF_LOG_SEG_HIBYTE] << 8) |
             spare_buf[OFF_LOG_SEG_LOBYTE])
#if defined(FTL_V1)
            + 984 * (phys_seg / 1024)
#endif
            ;
}

static inline unsigned short get_cached_page_id(char* spare_buf)
{
    return (spare_buf[OFF_CACHE_PAGE_HIBYTE] << 8) |
            spare_buf[OFF_CACHE_PAGE_LOBYTE];
}

static int find_write_cache(int log_segment)
{
    int i;

    for (i = 0; i < write_caches_in_use; i++)
        if (write_caches[i].log_segment == log_segment)
            return i;

    return -1;
}


static void read_random_writes_cache(int bank, int phys_segment)
{
    int page = 0;
    short log_segment;
    unsigned char spare_buf[16];

    nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, page),
                  SECTOR_SIZE, /* offset to first sector's spare */
                  16, spare_buf);

    log_segment = get_log_segment_id(phys_segment, spare_buf);
    
    if (log_segment == -1)
        return;

    /* Find which cache this is related to */
    int cache_no = find_write_cache(log_segment);

    if (cache_no == -1)
    {
        if (write_caches_in_use < MAX_WRITE_CACHES)
        {
            cache_no = write_caches_in_use;
            write_caches_in_use++;
        }
        else
        {
            panicf("Max NAND write caches reached");
        }
    }

    write_caches[cache_no].log_segment = log_segment;
    write_caches[cache_no].random_bank = bank;
    write_caches[cache_no].random_phys_segment = phys_segment;

#ifndef FTL_V1
    /* Loop over each page in the phys segment (from page 1 onwards).
       Read spare for 1st sector, store location of page in array. */
    for (page = 1;
         page < (nand_data->pages_per_block * nand_data->planes);
         page++)
    {
        unsigned short cached_page;
        
        nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, page),
                      SECTOR_SIZE, /* offset to first sector's spare */
                      16, spare_buf);

        cached_page = get_cached_page_id(spare_buf);
        
        if (cached_page != 0xFFFF)
            write_caches[cache_no].page_map[cached_page] = page;
    }
#endif /* !FTL_V1 */
}


static void read_inplace_writes_cache(int bank, int phys_segment)
{
    int page = 0;
    short log_segment;
    unsigned char spare_buf[16];

    nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, page),
                  SECTOR_SIZE, /* offset to first sector's spare */
                  16, spare_buf);

    log_segment = get_log_segment_id(phys_segment, spare_buf);
    
    if (log_segment == -1)
        return;
    
    /* Find which cache this is related to */
    int cache_no = find_write_cache(log_segment);

    if (cache_no == -1)
    {
        if (write_caches_in_use < MAX_WRITE_CACHES)
        {
            cache_no = write_caches_in_use;
            write_caches_in_use++;
        }
        else
        {
            panicf("Max NAND write caches reached");
        }
    }

    write_caches[cache_no].log_segment = log_segment;
    
    /* Find how many pages have been written to the new segment */
    while (log_segment != -1 &&
           page < (nand_data->pages_per_block * nand_data->planes) - 1)
    {
        page++;
        nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, page),
                      SECTOR_SIZE, 16, spare_buf);

        log_segment = get_log_segment_id(phys_segment, spare_buf);
    }
    
    if (page != 0)
    {
        write_caches[cache_no].inplace_bank = bank;
        write_caches[cache_no].inplace_phys_segment = phys_segment;
        write_caches[cache_no].inplace_pages_used = page;
    }
}


int nand_read_sectors(IF_MV2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{
#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mtx);

    while (incount > 0)
    {
        int done = 0;
        int segment = start / sectors_per_segment;
        int secmod = start % sectors_per_segment;

        while (incount > 0 && secmod < sectors_per_segment)
        {
            if (!nand_read_sector_of_logical_segment(segment, secmod, inbuf))
            {
                mutex_unlock(&ata_mtx);
                return -1;
            }

            inbuf += SECTOR_SIZE;
            incount--;
            secmod++;
            done++;
        }
    
        if (done < 0)
        {
            mutex_unlock(&ata_mtx);
            return -1;
        }
        start += done;
    }

    mutex_unlock(&ata_mtx);
    return 0;
}


int nand_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif

    /* TODO: Learn more about TNFTL and implement this one day... */
    (void)start;
    (void)count;
    (void)outbuf;
    return -1;
}


#ifdef STORAGE_GET_INFO
void nand_get_info(struct storage_info *info)
{
    /* firmware version */
    info->revision="0.00";

    info->vendor="Rockbox";
    info->product="Internal Storage";

    /* blocks count */
    info->num_sectors = sectors_per_segment * segments_per_bank * total_banks;
    info->sector_size = SECTOR_SIZE;
}
#endif


int nand_init(void)
{
    int bank, phys_segment, lptbuf_size;
    unsigned char spare_buf[16];

    if (initialized) return 0;

#ifdef CPU_TCC77X
    CSCFG2 = 0x318a8010;

    GPIOC_FUNC &= ~(CS_GPIO_BIT | WE_GPIO_BIT);
    GPIOC_FUNC |= 0x1;
#endif
    
    /* Set GPIO direction for chip select & write protect */
    NAND_GPIO_OUT_EN(CS_GPIO_BIT | WE_GPIO_BIT);

    /* Get chip characteristics and number of banks */
    nand_get_chip_info();

#ifndef BOOTLOADER
    /* Use chip info to allocate the correct size LPT buffer */
    lptbuf_size = sizeof(struct lpt_entry) * segments_per_bank * total_banks;
    lpt_lookup = buffer_alloc(lptbuf_size);
#else
    /* Use a static array in the bootloader */
    lptbuf_size = sizeof(lpt_lookup);
#endif

    memset(lpt_lookup, 0xff, lptbuf_size);
    memset(write_caches, 0xff, sizeof(write_caches));
    
    write_caches_in_use = 0;

    /* Scan banks to build up block translation table */
    for (bank = 0; bank < total_banks; bank++)
    {
        for (phys_segment = 0; phys_segment < segments_per_bank; phys_segment++)
        {
            /* Read spare bytes from first sector of each segment */
            nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, 0),
                          SECTOR_SIZE, /* offset */
                          16, spare_buf);
            
            int type = get_sector_type(spare_buf);
            
            if (type == SECTYPE_MAIN_INPLACE_CACHE)
            {
                /* Check last sector of sequential write cache block */
                nand_read_raw(bank, phys_segment_to_page_addr
                                (phys_segment, pages_per_segment - 1),
                              nand_data->page_size + nand_data->spare_size - 16,
                              16, spare_buf);
                
                /* If last sector has been written, treat block as main data */
                if (get_sector_type(spare_buf) != 0xff)
                {
                    type = SECTYPE_MAIN_DATA;
                }
            }

            switch (type)
            {
                case SECTYPE_MAIN_DATA:
                {
                    /* Main data area segment */
                    unsigned short log_segment =
                        get_log_segment_id(phys_segment, spare_buf);

                    if (log_segment < segments_per_bank * total_banks)
                    {
                        if (lpt_lookup[log_segment].bank == -1 ||
                            lpt_lookup[log_segment].phys_segment == -1)
                        {
                            lpt_lookup[log_segment].bank = bank;
                            lpt_lookup[log_segment].phys_segment = phys_segment;
                        }
                        else
                        {
                            //panicf("duplicate data segment 0x%x!", log_segment);
                        }
                    }
                    break;
                }
                
                case SECTYPE_MAIN_RANDOM_CACHE:
                {
                    /* Newly-written random page data (Main data area) */
                    read_random_writes_cache(bank, phys_segment);
                    break;
                }
                
                case SECTYPE_MAIN_INPLACE_CACHE:
                {
                    /* Newly-written sequential page data (Main data area) */
                    read_inplace_writes_cache(bank, phys_segment);
                    break;
                }
            }
        }
    }
    
    initialized = true;

    return 0;
}

long nand_last_disk_activity(void)
{
    return last_disk_activity;
}

void nand_sleep(void)
{
}

void nand_spin(void)
{
}

void nand_spindown(int seconds)
{
    (void)seconds;
}
