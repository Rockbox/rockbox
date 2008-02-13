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
#include "ata_idle_notify.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "led.h"
#include "disk.h"
#include "panic.h"
#include "usb.h"

/* for compatibility */
int ata_spinup_time = 0;

long last_disk_activity = -1;

/** static, private data **/
static bool initialized = false;

static long next_yield = 0;
#define MIN_YIELD_PERIOD 2000

static struct mutex ata_mtx NOCACHEBSS_ATTR;

#define SECTOR_SIZE 512

/* TCC780x NAND Flash Controller */

#define NFC_CMD    (*(volatile unsigned long *)0xF0053000)
#define NFC_SADDR  (*(volatile unsigned long *)0xF005300C)
#define NFC_SDATA  (*(volatile unsigned long *)0xF0053040)
#define NFC_WDATA  (*(volatile unsigned long *)0xF0053010)
#define NFC_CTRL   (*(volatile unsigned long *)0xF0053050)
#define NFC_IREQ   (*(volatile unsigned long *)0xF0053060)
#define NFC_RST    (*(volatile unsigned long *)0xF0053064)

/* NFC_CTRL flags */
#define NFC_16BIT (1<<26)
#define NFC_CS0   (1<<23)
#define NFC_CS1   (1<<22)
#define NFC_READY (1<<20)

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

/* Maximum values for static buffers */

#define MAX_PAGE_SIZE       4096
#define MAX_SPARE_SIZE      128
#define MAX_BLOCKS_PER_BANK 8192
#define MAX_BANKS           4

/*
  Block translation table - maps logical Segment Number to physical page address
  Format: 0xBTPPPPPP (B = Bank; T = Block Type flag; P = Page Address)
 */
static int segment_location[MAX_BLOCKS_PER_BANK * MAX_BANKS / 4];


static void nand_chip_select(int chip)
{
    if (chip == -1)
    {
        /* Disable both chip selects */
        GPIOB_CLEAR = (1<<21);
        NFC_CTRL |= NFC_CS0 | NFC_CS1;
    }
    else
    {
        /* NFC chip select */
        if (chip & 1)
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
        if (chip & 2)
        {
            GPIOB_SET = (1<<21);
        }
        else
        {
            GPIOB_CLEAR = (1<<21);
        }
    }
}


static void nand_read_id(int chip, unsigned char* id_buf)
{
    /* Enable NFC bus clock */
    BCLKCTR |= DEV_NAND;

    /* Reset NAND controller */
    NFC_RST = 0;

    /* Set slow cycle timings since the chip is as yet unidentified */
    NFC_CTRL = (NFC_CTRL &~0xFFF) | 0x353;

    nand_chip_select(chip);

    /* Set write protect */
    GPIOB_CLEAR = (1<<19);

    /* Reset command */
    NFC_CMD = 0xFF;

    /* Set 8-bit data width */
    NFC_CTRL &= ~NFC_16BIT;

    /* Read ID command, single address cycle */
    NFC_CMD   = 0x90;
    NFC_SADDR = 0x00;

    /* Read the 5 single bytes */
    id_buf[0] = NFC_SDATA & 0xFF;
    id_buf[1] = NFC_SDATA & 0xFF;
    id_buf[2] = NFC_SDATA & 0xFF;
    id_buf[3] = NFC_SDATA & 0xFF;
    id_buf[4] = NFC_SDATA & 0xFF;

    nand_chip_select(-1);

    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


static void nand_read_uid(int chip, unsigned int* uid_buf)
{
    int i;

    /* Enable NFC bus clock */
    BCLKCTR |= DEV_NAND;

    /* Set cycle timing (stp = 1, pw = 3, hold = 1) */
    NFC_CTRL = (NFC_CTRL &~0xFFF) | 0x131;

    nand_chip_select(chip);

    /* Set write protect */
    GPIOB_CLEAR = 1<<19;

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


/* NB: size must be divisible by 4 due to 32-bit read */
static void nand_read_raw(int chip, int row, int column, int size, void* buf)
{
    int i;
    
    /* Currently this relies on a word-aligned input buffer */
    unsigned int* int_buf = (unsigned int*)buf;
    if ((unsigned int)buf & 3) panicf("nand_read_raw() non-aligned input buffer");

    /* Enable NFC bus clock */
    BCLKCTR |= DEV_NAND;

    /* Set cycle timing (stp = 1, pw = 3, hold = 1) */
    NFC_CTRL = (NFC_CTRL &~0xFFF) | 0x131;

    nand_chip_select(chip);

    /* Set write protect */
    GPIOB_CLEAR = (1<<19);

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
    for (i = 0; i < (size/4); i++)
    {
        int_buf[i] = NFC_WDATA;
    }

    nand_chip_select(-1);

    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


/* NB: Output buffer must currently be word-aligned */
static bool nand_read_sector(int segment, int sector, void* buf)
{
    int physaddr = segment_location[segment];
    int bank = physaddr >> 28;
    int page = physaddr & 0xffffff;

    int page_in_seg = sector / sectors_per_page;
    int sec_in_page = sector % sectors_per_page;
    
    /* TODO: Check if there are any 0x15 pages referring to this segment/sector
       combination. If present we need to read that data instead. */

    if (physaddr == -1) return false;

    if (page_in_seg & 1)
    {
        /* Data is located in block+1 */
        page += pages_per_block; 
    }

    if (page_in_seg & 2)
    {
        /* Data is located in second plane */
        page += (blocks_per_bank/2) * pages_per_block;
    }

    page += page_in_seg/4;

    nand_read_raw(bank, page,
                  sec_in_page * (SECTOR_SIZE+16),
                  SECTOR_SIZE, buf);

    /* TODO: Read the 16 spare bytes, perform ECC correction */
    
    return true;
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
    bytes_per_segment   = page_size * pages_per_block * 4;
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
    
    /* Check block 0, page 0 for "BMPM" string & total_banks byte. If this is
       confirmed for all D2s we can remove the above code & nand_read_uid(). */
       
    nand_read_raw(0,          /* bank */
                  0,          /* page */
                  page_size,  /* offset */
                  8, id_buf);
    
    if (strncmp(id_buf, "BMPM", 4)) panicf("BMPM tag not present");
    if (id_buf[4] != total_banks) panicf("BMPM total_banks mismatch");
}


/* TEMP testing function */

#ifdef BOOTLOADER
#include "lcd.h"

extern int line;
unsigned int buf[(MAX_PAGE_SIZE + MAX_SPARE_SIZE) / 4];

static void nand_test(void)
{
    int i;
    unsigned int seq_segments = 0;
#if 0
    int chip,page;
#endif

    printf("%d banks", total_banks);
    printf("* %d pages", pages_per_bank);
    printf("* %d bytes per page", page_size);

    i = 0;
    while (segment_location[i] != -1
            && i++ < (blocks_per_bank * total_banks / 4))
    {
        seq_segments++;
    }
    printf("%d sequential segments found (%dMb)", seq_segments,
           (seq_segments*bytes_per_segment)>>20);

    while (!button_read_device()) {};
    while (button_read_device()) {};

#if 0
    /* Read & display sequential pages */
    for (chip = 0; chip < total_banks; chip++)
    {
        for (page = 0x0; page < 0x100; page++)
        {
            nand_read_raw(chip, page, 0, page_size+spare_size, buf);

            for (i = 0; i < (page_size+spare_size)/4; i += 132)
            {
                int j,interesting = 0;
                line = 0;
                printf("c:%d p:%lx i:%d", chip, page, i);

                for (j=i; j<(i+131); j++)
                {
                    if (buf[j] != 0xffffffff) interesting = 1;
                }

                if (interesting)
                {
                    for (j=i; j<(i+63); j+=4)
                    {
                        printf("%lx %lx %lx %lx",
                               buf[j], buf[j+1], buf[j+2], buf[j+3]);
                    }
                    printf("--->");
                    while (!button_read_device()) {};
                    while (button_read_device()) {};

                    line = 1;
                    printf("<---");
                    for (j=j; j<(i+131); j+=4)
                    {
                        printf("%lx %lx %lx %lx",
                               buf[j], buf[j+1], buf[j+2], buf[j+3]);
                    }
                    while (!button_read_device()) {};
                    while (button_read_device()) {};
                    reset_screen();
                }
            }
        }
    }
#endif
}
#endif


/* API Functions */

void ata_led(bool onoff)
{
    led(onoff);
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
            if (!nand_read_sector(segment, secmod, inbuf))
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
    #warning function not implemented
    (void)start;
    (void)count;
    (void)outbuf;
    return 0;
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
    int i, bank, page;
    unsigned int spare_buf[4];
    
    if (initialized) return 0;

    /* Get chip characteristics and number of banks */
    nand_get_chip_info();

    for (i = 0; i < (MAX_BLOCKS_PER_BANK * MAX_BANKS / 4); i++)
    {
        segment_location[i] = -1;
    }
    
    /* Scan banks to build up block translation table */
    for (bank = 0; bank < total_banks; bank++)
    {
        for (page = 0; page < pages_per_bank/2; page += pages_per_block*2)
        {
            unsigned char segment_flag;
            unsigned char stored_flag;
            unsigned short segment_id;
            
            unsigned char* buf_ptr = (unsigned char*)spare_buf;

            /* Read spare bytes from first sector of each segment */
            nand_read_raw(bank, page,
                          SECTOR_SIZE, /* offset */
                          16, spare_buf);

            segment_id = (buf_ptr[6] << 8) | buf_ptr[7];
            segment_flag = buf_ptr[4];
            
            stored_flag = (segment_location[segment_id] >> 24) & 0xf;
            
#if defined(BOOTLOADER) && 0
            if (segment_flag == 0x15)
            {
                printf("Segment %lx: c:%lx p:%lx, type:%lx, stored:x%lx",
                        segment_id, bank, page, segment_flag, stored_flag);
                while (!button_read_device()) {};
                while (button_read_device()) {};
            }
#endif
            
            if (segment_flag == 0x13 || segment_flag == 0x17)
            {
                if (segment_id < (blocks_per_bank * total_banks / 4))
                {   
#if defined(BOOTLOADER) && 0
                    if (segment_location[segment_id] != -1 && stored_flag != 0x3)
                    {
                        int orig_bank = segment_location[segment_id] >> 28;
                        int orig_page = segment_location[segment_id] & 0xFFFFFF;

                        printf("Segment %d already set! (stored flag:x%lx)",
                                segment_id, stored_flag);
                        
                        printf("0x%08x 0x%08x 0x%08x 0x%08x",
                               spare_buf[0],spare_buf[1],spare_buf[2],spare_buf[3]);
                               
                        nand_read_raw(orig_bank, orig_page,
                                      SECTOR_SIZE,
                                      16, spare_buf);
                                  
                        printf("0x%08x 0x%08x 0x%08x 0x%08x",
                               spare_buf[0],spare_buf[1],spare_buf[2],spare_buf[3]);
                    }
#endif
                    /* Write bank, block type & physical address into table */
                    segment_location[segment_id]
                      = page | (bank << 28) | ((segment_flag & 0xf) << 24);
                }
                else
                {
                    panicf("Invalid segment id:%d found", segment_id);
                }
            }
        }
    }

    initialized = true;
    
#ifdef BOOTLOADER
    /* TEMP - print out some diagnostics */
    nand_test();
#endif

    return 0;
}


/* TEMP: This will return junk, it's here for compilation only */
unsigned short* ata_get_identify(void)
{
    return (unsigned short*)0x21000000; /* Unused DRAM */
}
