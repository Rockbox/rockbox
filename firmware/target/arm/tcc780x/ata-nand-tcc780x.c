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


/* TCC780x NAND Flash Controller */

#define NFC_CMD    (*(volatile unsigned long *)0xF0053000)
#define NFC_SADDR  (*(volatile unsigned long *)0xF005300C)
#define NFC_SDATA  (*(volatile unsigned long *)0xF0053040)
#define NFC_WDATA  (*(volatile unsigned long *)0xF0053010)
#define NFC_CTRL   (*(volatile unsigned long *)0xF0053050)
#define NFC_IREQ   (*(volatile unsigned long *)0xF0053060)
#define NFC_RST    (*(volatile unsigned long *)0xF0053064)

#define NFC_16BIT (1<<26)
#define NFC_CS0   (1<<23)
#define NFC_CS1   (1<<22)
#define NFC_READY (1<<20)


#if defined(COWON_D2)
/*
   ===== Temporary D2 testing code =====

   (assumes SAMSUNG K9LAG08UOM (2GB) in 1, 2 or 4 banks)

   Manufacturer Id: {0xec, 0xd5, 0x55, 0x25, 0x68}

*/
#define PAGE_SIZE       2048
#define SPARE_SIZE      64
#define PAGES_PER_BLOCK 128
#define TOTAL_BLOCKS    8192
#define TOTAL_PAGES     (TOTAL_BLOCKS * PAGES_PER_BLOCK)
#define COL_CYCLES      2
#define ROW_CYCLES      3

static int page_buf[PAGE_SIZE/4];

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
    int i;

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


static void nand_read_uid(int chip)
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
    for (i = 0; i < COL_CYCLES; i++) NFC_SADDR = 0;
    for (i = 0; i < ROW_CYCLES; i++) NFC_SADDR = 0;

    /* End of read */
    NFC_CMD = 0x30;

    /* Wait until complete */
    while (!(NFC_CTRL & NFC_READY)) {};

    /* Copy data to buffer (data repeats after 8 words) */
    for (i = 0; i < 8; i++)
    {
        page_buf[i] = NFC_WDATA;
    }

    /* Reset the chip back to normal mode */
    NFC_CMD = 0xFF;

    nand_chip_select(-1);

    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


/* NB: size must be divisible by 4 due to 32-bit read */
static void nand_read(int chip, int row, int column, int size)
{
    int i;

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
    for (i = 0; i < COL_CYCLES; i++)
    {
        NFC_SADDR = column & 0xFF;
        column = column >> 8;
    }

    /* Write row address */
    for (i = 0; i < ROW_CYCLES; i++)
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
        page_buf[i] = NFC_WDATA;
    }

    nand_chip_select(-1);

    /* Disable NFC bus clock */
    BCLKCTR &= ~DEV_NAND;
}


/* TEMP testing function */
#include <string.h>
#include "lcd.h"

extern int line;

static void nand_test(void)
{
    int i,j,row;
    unsigned char id_buf[5];
    unsigned char str_buf[PAGE_SIZE];

    /* Display ID codes & UID block for each bank */
    for (i = 0; i < 4; i++)
    {
        printf("NAND bank %d:", i);

        nand_read_id(i, id_buf);

        printf("ID: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                id_buf[0],id_buf[1],id_buf[2],id_buf[3],id_buf[4]);

        nand_read_uid(i);

        for (j = 0; j < 8; j += 4)
        {
            printf("0x%08x 0x%08x 0x%08x 0x%08x",
                    page_buf[j],page_buf[j+1],page_buf[j+2],page_buf[j+3]);
        }

        line++;
    }

    while (!button_read_device()) {};

    /* Now for fun, scan the raw pages for 'TAG' and display the contents */

    row = 0;
    while (row < TOTAL_PAGES)
    {
        bool found = false;
        unsigned char* buf_ptr = (unsigned char*)page_buf;

        line = 0;

        /* Read a page from chip 0 */
        nand_read(0, row, 0, PAGE_SIZE);

        if (row % 512 == 0) printf("%dMb", row/512);

        for (j = 0; j < PAGE_SIZE; j++)
        {
            if (buf_ptr[j] == 'T' && buf_ptr[j+1] == 'A' && buf_ptr[j+2] == 'G')
                found = true;
        }

        if (found)
        {
            unsigned char* str_ptr = str_buf;

            printf("Row %d:", row);

            /* Copy ascii-readable parts out to a string */
            for (i = 0; i < PAGE_SIZE; i++)
            {
                str_buf[i] = ' ';
                if (buf_ptr[i] > 31 && buf_ptr[i] < 128)
                {
                    *str_ptr++ = buf_ptr[i];
                }
            }

            str_ptr = str_buf;

            /* Nasty piece of code to display the text in a readable manner */
            for (i = 1; i < 30; i++)
            {
                for (j = 0; j < 48; j++)
                {
                    /* In the absence of a putc() we have this mess... */
                    unsigned char buf2[2];
                    buf2[0] = *str_ptr++;
                    buf2[1] = '\0';
                    lcd_puts(j,i,buf2);
                }
            }

            /* Alternate hex display code
            for (i = 0; i<112; i+=4)
            {
                printf("0x%08x 0x%08x 0x%08x 0x%08x",buf[i],buf[i+1],buf[i+2],buf[i+3]);
            }
            */

            while (!button_read_device()) {};

            lcd_clear_display();
        }
        row++;
    }
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
    #warning function not implemented
    (void)start;
    (void)incount;
    (void)inbuf;
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
    #warning function not implemented
    return 0;
}

void ata_sleep(void)
{
    #warning function not implemented
}

void ata_spin(void)
{
    /* null */
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    #warning function not implemented
    return 0;
}

int ata_soft_reset(void)
{
    #warning function not implemented
    return 0;
}

void ata_enable(bool on)
{
    #warning function not implemented
    (void)on;
}

int ata_init(void)
{
    #warning function not implemented

    /* This needs to:
        a) establish how many banks are present
            (using nand_read_id() and nand_read_uid() above)
        b) scan all banks for bad blocks
        c) use this info to build a physical->logical address translation
            (using an as yet unknown scheme)
     */

    /* TEMP - print out some diagnostics */
    nand_test();

    return 0;
}
