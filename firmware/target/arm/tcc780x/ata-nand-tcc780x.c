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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "ata.h"
#include "ata-target.h"
#include "system.h"
#include <string.h>
#include "led.h"
#include "panic.h"

/* The NAND driver is currently work-in-progress and as such contains
   some dead code and debug stuff, such as the next few lines. */
#include "lcd.h"
#include "font.h"
#include "button.h"
#include <sprintf.h>

/* #define USE_TCC_LPT */
/* #define USE_ECC_CORRECTION */

/* for compatibility */
int ata_spinup_time = 0;

long last_disk_activity = -1;

/** static, private data **/
static bool initialized = false;

static struct mutex ata_mtx SHAREDBSS_ATTR;

#define SECTOR_SIZE 512

/* TCC780x NAND Flash Controller */

#define NFC_CMD    (*(volatile unsigned long *)0xF0053000)
#define NFC_SADDR  (*(volatile unsigned long *)0xF005300C)
#define NFC_SDATA  (*(volatile unsigned long *)0xF0053040)
#define NFC_WDATA  (*(volatile unsigned long *)0xF0053010)
#define NFC_CTRL   (*(volatile unsigned long *)0xF0053050)
    #define NFC_16BIT (1<<26)
    #define NFC_CS0   (1<<23)
    #define NFC_CS1   (1<<22)
    #define NFC_READY (1<<20)
#define NFC_IREQ   (*(volatile unsigned long *)0xF0053060)
#define NFC_RST    (*(volatile unsigned long *)0xF0053064)

/* TCC780x ECC Controller */

#define ECC_CTRL    (*(volatile unsigned long *)0xF005B000)
    #define ECC_M4EN  (1<<6)
    #define ECC_ENC   (1<<27)
    #define ECC_READY (1<<26)
#define ECC_BASE    (*(volatile unsigned long *)0xF005B004)
#define ECC_CLR     (*(volatile unsigned long *)0xF005B00C)
#define ECC_MLC0W   (*(volatile unsigned long *)0xF005B030)
#define ECC_MLC1W   (*(volatile unsigned long *)0xF005B034)
#define ECC_MLC2W   (*(volatile unsigned long *)0xF005B038)
#define ECC_ERRADDR (*(volatile unsigned long *)0xF005B050)
#define ECC_ERRDATA (*(volatile unsigned long *)0xF005B060)
#define ECC_ERR     (*(volatile unsigned long *)0xF005B070)

/* GPIOs */

#define NAND_GPIO_SET(n)    GPIOB_SET = n
#define NAND_GPIO_CLEAR(n)  GPIOB_CLEAR = n
#define NAND_GPIO_OUT_EN(n) GPIOB_DIR |= n

#define WE_GPIO_BIT (1<<19) /* Write Enable */
#define CS_GPIO_BIT (1<<21) /* Chip Select (4 banks when used with NFC_CSx) */

/* Chip characteristics, initialised by nand_get_chip_info() */

static int page_size       = 0;
static int spare_size      = 0;
static int pages_per_block = 0;
static int blocks_per_bank = 0;
static int pages_per_bank  = 0;
static int row_cycles      = 0;
static int col_cycles      = 0;
static int total_banks     = 0;
static int sectors_per_page    = 0;
static int bytes_per_segment   = 0;
static int sectors_per_segment = 0;
static int segments_per_bank   = 0;

/* Maximum values for static buffers */

#define MAX_PAGE_SIZE       4096
#define MAX_SPARE_SIZE      128
#define MAX_BLOCKS_PER_BANK 8192
#define MAX_PAGES_PER_BLOCK 128
#define BLOCKS_PER_SEGMENT  4
#define MAX_BANKS           4

#define MAX_SEGMENTS (MAX_BLOCKS_PER_BANK * MAX_BANKS / BLOCKS_PER_SEGMENT)

/* Logical/Physical translation table */

struct lpt_entry
{
    short bank;
    short phys_segment;
};
static struct lpt_entry lpt_lookup[MAX_SEGMENTS];

/* Write Caches */

#define MAX_WRITE_CACHES 8

struct write_cache
{
    short bank;
    short phys_segment;
    short log_segment;
    short page_map[MAX_PAGES_PER_BLOCK * BLOCKS_PER_SEGMENT];
};
static struct write_cache write_caches[MAX_WRITE_CACHES];

static int write_caches_in_use = 0;

#ifdef USE_TCC_LPT
/* Read buffer (used for reading LPT blocks only) */
static unsigned char page_buf[MAX_PAGE_SIZE + MAX_SPARE_SIZE]
       __attribute__ ((aligned (4)));
#endif

#ifdef USE_ECC_CORRECTION
static unsigned int ecc_sectors_corrected = 0;
static unsigned int ecc_bits_corrected = 0;
static unsigned int ecc_fail_count = 0;
#endif


/* Conversion functions */

static inline int phys_segment_to_page_addr(int phys_segment, int page_in_seg)
{
    int page_addr = phys_segment * pages_per_block * 2;

    if (page_in_seg & 1)
    {
        /* Data is located in block+1 */
        page_addr += pages_per_block;
    }

    if (page_in_seg & 2)
    {
        /* Data is located in second plane */
        page_addr += (blocks_per_bank/2) * pages_per_block;
    }

    page_addr += page_in_seg/4;

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
#ifdef USE_TCC_LPT
        if (!(bank & 1))
#else
        if (bank & 1)
#endif
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
    for (i = 0; i < col_cycles; i++) NFC_SADDR = 0;
    for (i = 0; i < row_cycles; i++) NFC_SADDR = 0;

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
    for (i = 0; i < col_cycles; i++)
    {
        NFC_SADDR = column & 0xFF;
        column = column >> 8;
    }

    /* Write row address */
    for (i = 0; i < row_cycles; i++)
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
    bool found = false;
    unsigned char manuf_id;
    unsigned char id_buf[8];

    /* Read chip id from bank 0 */
    nand_read_id(0, id_buf);

    manuf_id = id_buf[0];

    switch (manuf_id)
    {
        case 0xEC:  /* SAMSUNG */

            switch(id_buf[1]) /* Chip Id */
            {
                case 0xD5:  /* K9LAG08UOM */

                    page_size       = 2048;
                    spare_size      = 64;
                    pages_per_block = 128;
                    blocks_per_bank = 8192;
                    col_cycles      = 2;
                    row_cycles      = 3;

                    found = true;
                    break;

                case 0xD7:  /* K9LBG08UOM */

                    page_size       = 4096;
                    spare_size      = 128;
                    pages_per_block = 128;
                    blocks_per_bank = 8192;
                    col_cycles      = 2;
                    row_cycles      = 3;

                    found = true;
                    break;
            }
            break;
    }

    if (!found)
    {
        panicf("Unknown NAND: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                id_buf[0],id_buf[1],id_buf[2],id_buf[3],id_buf[4]);
    }

    pages_per_bank      = blocks_per_bank * pages_per_block;
    segments_per_bank   = blocks_per_bank / BLOCKS_PER_SEGMENT;
    bytes_per_segment   = page_size * pages_per_block * BLOCKS_PER_SEGMENT;
    sectors_per_page    = page_size / SECTOR_SIZE;
    sectors_per_segment = bytes_per_segment / SECTOR_SIZE;

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
       2. Byte at <page_size>+4 contains number of banks [or 0xff if 1 bank]
        
       If this is confirmed for all D2s we can simplify the above code and
       also remove the icky nand_read_uid() function.
     */

    nand_read_raw(0,          /* bank */
                  0,          /* page */
                  page_size,  /* offset */
                  8, id_buf);

    if (strncmp(id_buf, "BMP", 3)) panicf("BMP tag not present");
    
    if (total_banks > 1)
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
        if (write_caches[cache_num].log_segment == log_segment
            && write_caches[cache_num].page_map[page_in_segment] != -1)
        {
            found = true;
            bank = write_caches[cache_num].bank;
            phys_segment = write_caches[cache_num].phys_segment;
            page_in_segment = write_caches[cache_num].page_map[page_in_segment];
        }
        else
        {
            cache_num++;
        }
    }

    return nand_read_sector_of_phys_segment(bank, phys_segment,
                                            page_in_segment,
                                            sector_in_page, buf);
}


#ifdef USE_TCC_LPT

/* Reading the LPT from NAND is not yet fully understood. This code is therefore
   not enabled by default, as it gives much worse results than the bank-scanning
   approach currently used. */

static void read_lpt_block(int bank, int phys_segment)
{
    int page = 1;   /* table starts at page 1 of segment */
    bool cont = true;
    
    struct lpt_entry* lpt_ptr = NULL;

    while (cont && page < pages_per_block)
    {
        int i = 0;
        unsigned int* int_buf = (int*)page_buf;
        
        nand_read_sector_of_phys_segment(bank, phys_segment,
                                         page, 0, /* only sector 0 is used */
                                         page_buf);

        /* Find out which chunk of the LPT table this section contains.
           Do this by reading the logical segment number of entry 0 */
        if (lpt_ptr == NULL)
        {
            int first_bank = int_buf[0] / segments_per_bank;
            int first_phys_segment = int_buf[0] % segments_per_bank;

            unsigned char spare_buf[16];

            nand_read_raw(first_bank,
                          phys_segment_to_page_addr(first_phys_segment, 0),
                          SECTOR_SIZE, /* offset */
                          16, spare_buf);

            int first_log_segment = (spare_buf[6] << 8) | spare_buf[7];

            lpt_ptr = &lpt_lookup[first_log_segment];

#if defined(BOOTLOADER) && 1
            printf("lpt @ %lx:%lx (ls:%lx)",
                   first_bank, first_phys_segment, first_log_segment);
#endif
        }

        while (cont && (i < SECTOR_SIZE/4))
        {
            if (int_buf[i] != 0xFFFFFFFF)
            {
                lpt_ptr->bank = int_buf[i] / segments_per_bank;
                lpt_ptr->phys_segment = int_buf[i] % segments_per_bank;

                lpt_ptr++;
                i++;
            }
            else cont = false;
        }
        page++;
    }
}

#endif /* USE_TCC_LPT */


static void read_write_cache_segment(int bank, int phys_segment)
{
    int page;
    unsigned char spare_buf[16];
    
    if (write_caches_in_use == MAX_WRITE_CACHES)
        panicf("Max NAND write caches reached");

    write_caches[write_caches_in_use].bank = bank;
    write_caches[write_caches_in_use].phys_segment = phys_segment;
    
    /* Loop over each page in the phys segment (from page 1 onwards).
       Read spare for 1st sector, store location of page in array. */
    for (page = 1; page < pages_per_block * BLOCKS_PER_SEGMENT; page++)
    {
        unsigned short cached_page;
        unsigned short log_segment;
        
        nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, page),
                      SECTOR_SIZE, /* offset to first sector's spare */
                      16, spare_buf);

        cached_page = (spare_buf[3] << 8) | spare_buf[2]; /* why does endian */
        log_segment = (spare_buf[6] << 8) | spare_buf[7]; /* -ness differ? */

        if (cached_page != 0xFFFF)
        {
            write_caches[write_caches_in_use].log_segment = log_segment;
            write_caches[write_caches_in_use].page_map[cached_page] = page;
        }
    }
    write_caches_in_use++;
}


int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int incount,
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

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
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

void ata_spindown(int seconds)
{
    /* null */
    (void)seconds;
}

bool ata_disk_is_active(void)
{
    /* null */
    return 0;
}

void ata_sleep(void)
{
    /* null */
}

void ata_spin(void)
{
    /* null */
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    /* null */
    return 0;
}

int ata_soft_reset(void)
{
    /* null */
    return 0;
}

void ata_enable(bool on)
{
    /* null - flash controller is enabled/disabled as needed. */
    (void)on;
}

int ata_init(void)
{
    int i, bank, phys_segment;
    unsigned char spare_buf[16];

    if (initialized) return 0;

    /* Set GPIO direction for chip select & write protect */
    NAND_GPIO_OUT_EN(CS_GPIO_BIT | WE_GPIO_BIT);

    /* Get chip characteristics and number of banks */
    nand_get_chip_info();
    
    for (i = 0; i < MAX_SEGMENTS; i++)
    {
        lpt_lookup[i].bank = -1;
        lpt_lookup[i].phys_segment = -1;
    }
    
    write_caches_in_use = 0;

    for (i = 0; i < MAX_WRITE_CACHES; i++)
    {
        int page;
        
        write_caches[i].log_segment = -1;
        write_caches[i].bank = -1;
        write_caches[i].phys_segment = -1;
        
        for (page = 0; page < MAX_PAGES_PER_BLOCK * 4; page++)
        {
            write_caches[i].page_map[page] = -1;
        }
    }

    /* Scan banks to build up block translation table */
    for (bank = 0; bank < total_banks; bank++)
    {
        for (phys_segment = 0; phys_segment < segments_per_bank; phys_segment++)
        {
            /* Read spare bytes from first sector of each segment */
            nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, 0),
                          SECTOR_SIZE, /* offset */
                          16, spare_buf);

            switch (spare_buf[4]) /* block type */
            {
#ifdef USE_TCC_LPT
                case 0x12:
                {
                    /* Log->Phys Translation table (for Main data area) */
                    read_lpt_block(bank, phys_segment);
                    break;
                }
#else
                case 0x17:
                {
                    /* Main data area segment */
                    unsigned short segment = (spare_buf[6] << 8) | spare_buf[7];

                    if (segment < MAX_SEGMENTS)
                    {
                        lpt_lookup[segment].bank = bank;
                        lpt_lookup[segment].phys_segment = phys_segment;
                    }
                    break;
                }
#endif

                case 0x15:
                {
                    /* Recently-written page data (for Main data area) */
                    read_write_cache_segment(bank, phys_segment);
                    break;
                }
            }
        }
    }

#ifndef USE_TCC_LPT
    /* Scan banks a second time as 0x13 segments appear to override 0x17 */
    for (bank = 0; bank < total_banks; bank++)
    {
        for (phys_segment = 0; phys_segment < segments_per_bank; phys_segment++)
        {
            /* Read spare bytes from first sector of each segment */
            nand_read_raw(bank, phys_segment_to_page_addr(phys_segment, 0),
                          SECTOR_SIZE, /* offset */
                          16, spare_buf);

            switch (spare_buf[4]) /* block type */
            {
                case 0x13:
                {
                    /* Main data area segment */
                    unsigned short segment = (spare_buf[6] << 8) | spare_buf[7];

                    if (segment < MAX_SEGMENTS)
                    {
                        /* 0x17 seems to override 0x13, so store in our LPT */
                        lpt_lookup[segment].bank = bank;
                        lpt_lookup[segment].phys_segment = phys_segment;
                    }
                    break;
                }
            }
        }
    }
#endif
    
    initialized = true;

    return 0;
}


/* TEMP: This will return junk, it's here for compilation only */
unsigned short* ata_get_identify(void)
{
    return (unsigned short*)0x21000000; /* Unused DRAM */
}
