/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Daniel Ankers
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
#include "pp5024.h"

#define NOINLINE_ATTR __attribute__((noinline)) /* don't inline the loops */

#define BLOCK_SIZE      (512)
#define SECTOR_SIZE     (512)
#define BLOCKS_PER_BANK 0x7a7800

#define STATUS_REG      (*(volatile unsigned int *)(0x70008204))
#define REG_1           (*(volatile unsigned int *)(0x70008208))
#define UNKNOWN         (*(volatile unsigned int *)(0x70008210))
#define BLOCK_SIZE_REG  (*(volatile unsigned int *)(0x7000821c))
#define BLOCK_COUNT_REG (*(volatile unsigned int *)(0x70008220))
#define REG_5           (*(volatile unsigned int *)(0x70008224))
#define CMD_REG0        (*(volatile unsigned int *)(0x70008228))
#define CMD_REG1        (*(volatile unsigned int *)(0x7000822c))
#define CMD_REG2        (*(volatile unsigned int *)(0x70008230))
#define RESPONSE_REG    (*(volatile unsigned int *)(0x70008234))
#define SD_STATE_REG    (*(volatile unsigned int *)(0x70008238))
#define REG_11          (*(volatile unsigned int *)(0x70008240))
#define REG_12          (*(volatile unsigned int *)(0x70008244))
#define DATA_REG        (*(volatile unsigned int *)(0x70008280))

#define DATA_DONE       (1 << 12)
#define CMD_DONE        (1 << 13)
#define ERROR_BITS      (0x3f)
#define FIFO_FULL       (1 << 7)
#define FIFO_EMPTY      (1 << 6)

/* SD States */
#define IDLE            0
#define READY           1
#define IDENT           2
#define STBY            3
#define TRAN            4
#define DATA            5
#define RCV             6
#define PRG             7
#define DIS             8

#define FIFO_SIZE       16          /* FIFO is 16 words deep */

/* SD Commands */
#define GO_IDLE_STATE   0
#define ALL_SEND_CID    2
#define SEND_RELATIVE_ADDR  3
#define SET_DSR         4
#define SWITCH_FUNC     6
#define SELECT_CARD     7
#define DESELECT_CARD   7
#define SEND_CSD        9
#define SEND_CID        10
#define STOP_TRANSMISSION   12
#define SEND_STATUS     13
#define GO_INACTIVE_STATE   15
#define SET_BLOCKLEN    16
#define READ_SINGLE_BLOCK   17
#define READ_MULTIPLE_BLOCK 18
#define WRITE_BLOCK     24
#define WRITE_MULTIPLE_BLOCK    25
#define ERASE_WR_BLK_START  32
#define ERASE_WR_BLK_END    33
#define ERASE           38

/* Application Specific commands */
#define SET_BUS_WIDTH   6
#define SD_APP_OP_COND  41

#define READ_TIMEOUT    5*HZ
#define WRITE_TIMEOUT   0.5*HZ

static unsigned short identify_info[SECTOR_SIZE];
int ata_spinup_time = 0;
long last_disk_activity = -1;
static bool initialized = false;

static unsigned char current_bank = 0; /* The bank that we are working with */

static tSDCardInfo card_info[2];

/* For multi volume support */
static int current_card = 0;

static struct mutex sd_mtx;

static long sd_stack [(DEFAULT_STACK_SIZE*2)/sizeof(long)];

static const char sd_thread_name[] = "sd";
static struct event_queue sd_queue;


/* Private Functions */

bool sd_send_command(unsigned int cmd, unsigned long arg1, unsigned int arg2)
{
    bool result = false;
    do
    {
        CMD_REG0 = cmd;
        CMD_REG1 = (unsigned int)((arg1 & 0xffff0000) >> 16);
        CMD_REG2 = (unsigned int)((arg1 & 0xffff));
        UNKNOWN = arg2;
        while ((STATUS_REG & CMD_DONE) == 0)
        {
            /* Busy wait */
        }
        if ((STATUS_REG & ERROR_BITS) == 0)
        {
            result = true;
        } 
    } while ((STATUS_REG & ERROR_BITS) != 0);
    return result;
}

void sd_read_response(unsigned int *response, int type)
{
    int i;
    int words; /* Number of 16 bit words to read from RESPONSE_REG */
    unsigned int response_from_card[9];
    if(type == 2)
    {
        words = 9; /* R2 types are 8.5 16-bit words long */
    } else {
        words = 3;
    }

    for (i = 0; i < words; i++) /* RESPONSE_REG is read MSB first */
    {
            response_from_card[i] = RESPONSE_REG; /* Read most significant 16-bit word */
    }

    switch (type)
    {
        case 1:
            /* Response type 1 has the following structure:
                Start bit
                Transmission bit
                Command index (6 bits)
                Card Status (32 bits)
                CRC7 (7 bits)
                Stop bit
            */
            /* TODO: Sanity checks */
            response[0] = ((response_from_card[0] & 0xff) << 24)
                       + (response_from_card[1] << 8)
                       + ((response_from_card[2] & 0xff00) >> 8);
            break;
        case 2:
            /* Response type 2 has the following structure:
                Start bit
                Transmission bit
                Reserved (6 bits)
                CSD/CID register (127 bits)
                Stop bit
            */
            response[3] = ((response_from_card[0]&0xff)<<24) +
                           (response_from_card[1]<<8) +
                           ((response_from_card[2]&0xff00)>>8);
            response[2] = ((response_from_card[2]&0xff)<<24) +
                           (response_from_card[3]<<8) +
                           ((response_from_card[4]&0xff00)>>8);
            response[1] = ((response_from_card[4]&0xff)<<24) +
                           (response_from_card[5]<<8) +
                           ((response_from_card[6]&0xff00)>>8);
            response[0] = ((response_from_card[6]&0xff)<<24) +
                           (response_from_card[7]<<8) +
                           ((response_from_card[8]&0xff00)>>8);
            break;
        case 3:
            /* Response type 3 has the following structure:
                Start bit
                Transmission bit
                Reserved (6 bits)
                OCR register (32 bits)
                Reserved (7 bits)
                Stop bit
            */
            response[0] = ((response_from_card[0] & 0xff) << 24)
                       + (response_from_card[1] << 8)
                       + ((response_from_card[2] & 0xff00) >> 8);
        /* Types 4-6 not supported yet */
    }
}

bool sd_send_acommand(unsigned int cmd, unsigned long arg1, unsigned int arg2)
{
    unsigned int returncode;
    if (sd_send_command(55, (card_info[current_card].rca)<<16, 1) == false)
        return false;
    sd_read_response(&returncode, 1);
    if (sd_send_command(cmd, arg1, arg2) == false)
        return false;
    return true;
}

void sd_wait_for_state(tSDCardInfo* card, unsigned int state)
{
    unsigned int response = 0;
    while(((response >> 9) & 0xf) != state)
    {
        sd_send_command(SEND_STATUS, (card->rca) << 16, 1);
        sd_read_response(&response, 1);
        /* TODO: Add a timeout and error handling */
    }
    SD_STATE_REG = state;
}


STATICIRAM void copy_read_sectors(unsigned char* buf, int wordcount)
                NOINLINE_ATTR ICODE_ATTR;

STATICIRAM void copy_read_sectors(unsigned char* buf, int wordcount)
{
    unsigned int tmp = 0;

    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned char* bufend = buf + wordcount*2;
        do
        {
            tmp = DATA_REG;
            *buf++ = tmp & 0xff;
            *buf++ = tmp >> 8;
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {
            *wbuf = DATA_REG;
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
}

STATICIRAM void copy_write_sectors(const unsigned char* buf, int wordcount)
                NOINLINE_ATTR ICODE_ATTR;

STATICIRAM void copy_write_sectors(const unsigned char* buf, int wordcount)
{
    unsigned short tmp = 0;
    const unsigned char* bufend = buf + wordcount*2;
    do
    {
        tmp = (unsigned short) *buf++;
        tmp |= (unsigned short) *buf++ << 8;
        DATA_REG = tmp;
    } while (buf < bufend); /* tail loop is faster */
}


void sd_select_bank(unsigned char bank)
{
    unsigned int response;
    unsigned char card_data[512];
    unsigned char* write_buf;
    int i;
    tSDCardInfo *card = &card_info[0]; /* Bank selection will only be done on
                                        the onboard flash */
    if (current_bank != bank)
    {
        memset(card_data, 0, 512);
        sd_wait_for_state(card, TRAN);
        BLOCK_SIZE_REG = 512;
        BLOCK_COUNT_REG = 1;
        sd_send_command(35, 0, 0x1c0d); /* CMD35 is vendor specific */
        sd_read_response(&response, 1);
        SD_STATE_REG = PRG;

        card_data[0] = bank;

        /* Write the card data */
        write_buf = card_data;
        for (i = 0; i < BLOCK_SIZE / 2; i += FIFO_SIZE)
        {
            /* Wait for the FIFO to be empty */
            while((STATUS_REG & FIFO_EMPTY) == 0) {} /* Erm... is this right? */

            copy_write_sectors(write_buf, FIFO_SIZE);

            write_buf += FIFO_SIZE*2; /* Advance one chunk of 16 words */
        }

        while((STATUS_REG & DATA_DONE) == 0) {}
        current_bank = bank;
    }
}

void sd_init_device(void)
{
/* SD Protocol registers */
    unsigned int dummy;
    int i;

    static unsigned int read_bl_len = 0;
    static unsigned int c_size = 0;
    static unsigned int c_size_mult = 0;
    static unsigned long mult = 0;

    unsigned char carddata[512];
    unsigned char *dataptr;
    tSDCardInfo *card = &card_info[0]; /* Init onboard flash only */

/* Initialise card data as blank */
    card->initialized = false;
    card->ocr = 0;
    card->csd[0] = 0;
    card->csd[1] = 0;
    card->csd[2] = 0;
    card->cid[0] = 0;
    card->cid[1] = 0;
    card->cid[2] = 0;
    card->rca = 0;

    card->capacity = 0;
    card->numblocks = 0;
    card->block_size = 0;
    card->block_exp = 0;

/* Enable and initialise controller */
    GPIOG_ENABLE |= (0x3 << 5);
    GPIOG_OUTPUT_EN |= (0x3 << 5);
    GPIOG_OUTPUT_VAL |= (0x3 << 5);
    outl(inl(0x70000088) & ~(0x4), 0x70000088);
    outl(inl(0x7000008c) & ~(0x4), 0x7000008c);
    outl(inl(0x70000080) | 0x4, 0x70000080);
    outl(inl(0x70000084) | 0x4, 0x70000084);
    REG_1 = 6;
    outl(inl(0x70000014) & ~(0x3ffff), 0x70000014);
    outl((inl(0x70000014) & ~(0x3ffff)) | 0x255aa, 0x70000014);
    outl(0x1010, 0x70000034);

    GPIOA_ENABLE |= (1 << 7);
    GPIOA_OUTPUT_EN &= ~(1 << 7);
    GPIOD_ENABLE |= (0x1f);
    GPIOD_OUTPUT_EN |= (0x1f);
    GPIOD_OUTPUT_VAL |= (0x1f);
    DEV_EN |= DEV_ATA; /* Enable controller */
    DEV_RS |= DEV_ATA; /* Reset controller */
    DEV_RS &=~DEV_ATA; /* Clear Reset */

/* Init NAND */
    REG_11 |= (1 << 15);
    REG_12 |= (1 << 15);
    REG_12 &= ~(3 << 12);
    REG_12 |= (1 << 13);
    REG_11 &= ~(3 << 12);
    REG_11 |= (1 << 13);

    SD_STATE_REG = TRAN;
    REG_5 = 0xf;

    sd_send_command(GO_IDLE_STATE, 0, 256);
    while ((card->ocr & (1 << 31)) == 0) /* Loop until the card is powered up */
    {
        sd_send_acommand(SD_APP_OP_COND, 0x100000, 3);
        sd_read_response(&(card->ocr), 3);

        if (card->ocr == 0)
        {
            /* TODO: Handle failure */
            while (1) {};
        }
    }

    sd_send_command(ALL_SEND_CID, 0, 2);
    sd_read_response(card->cid, 2);
    sd_send_command(SEND_RELATIVE_ADDR, 0, 1);
    sd_read_response(&card->rca, 1);
    card->rca >>= 16; /* The Relative Card Address is the top 16 bits of the
                        32 bits returned.  Whenever it is used, it gets
                        shifted left by 16 bits, so this step could possibly
                        be skipped. */

    sd_send_command(SEND_CSD, card->rca << 16, 2);
    sd_read_response(card->csd, 2);

    /* Parse disk geometry */
    /* These calculations come from the Sandisk SD card product manual */
    read_bl_len = ((card->csd[2] >> 16) & 0xf);
    c_size = ((card->csd[2] & (0x3ff)) << 2) +
        ((card->csd[1] & (0xc0000000)) >> 30);
    c_size_mult = ((card->csd[1] >> 15) & 0x7);
    mult = (1<<(c_size_mult + 2));
    card->max_read_bl_len = (1<<read_bl_len);
    card->block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
    card->numblocks = (c_size + 1) * mult * (card->max_read_bl_len / 512);
    card->capacity = card->numblocks * card->block_size;

    REG_1 = 0;
    sd_send_command(SELECT_CARD, card->rca << 16, 129);
    sd_read_response(&dummy, 1); /* I don't think we use the result from this */
    sd_send_acommand(SET_BUS_WIDTH, (card->rca << 16) | 2, 1);
    sd_read_response(&dummy, 1); /* 4 bit wide bus */
    sd_send_command(SET_BLOCKLEN, card->block_size, 1);
    sd_read_response(&dummy, 1);
    BLOCK_SIZE_REG = card->block_size;

    /* If this card is > 4Gb, then we need to enable bank switching */
    if(card->numblocks >= BLOCKS_PER_BANK)
    {
        SD_STATE_REG = TRAN;
        BLOCK_COUNT_REG = 1;
        sd_send_command(SWITCH_FUNC, 0x80ffffef, 0x1c05);
        sd_read_response(&dummy, 1);
        /* Read 512 bytes from the card.
        The first 512 bits contain the status information
        TODO: Do something useful with this! */
        dataptr = carddata;
        for (i = 0; i < BLOCK_SIZE / 2; i += FIFO_SIZE)
        {
            /* Wait for the FIFO to be full */
            while((STATUS_REG & FIFO_FULL) == 0) {}

            copy_read_sectors(dataptr, FIFO_SIZE);

            dataptr += (FIFO_SIZE*2); /* Advance one chunk of 16 words */
        }
    }
    mutex_init(&sd_mtx);
}

/* API Functions */

void ata_led(bool onoff)
{
    (void)onoff;
}

int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{
    int ret = 0;
    long timeout;
    int count;
    void* buf;
    long spinup_start;
    unsigned int dummy;
    unsigned int response;
    unsigned int i;
    tSDCardInfo *card = &card_info[current_card];

    /* TODO: Add DMA support. */

#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&sd_mtx);

    last_disk_activity = current_tick;
    spinup_start = current_tick;

    ata_enable(true);
    ata_led(true);

    timeout = current_tick + READ_TIMEOUT;

    /* TODO: Select device */
    if(current_card == 0)
    {
        if(start >= BLOCKS_PER_BANK)
        {
            sd_select_bank(1);
            start -= BLOCKS_PER_BANK;
        } else {
            sd_select_bank(0);
        }
    }

    buf = inbuf;
    count = incount;
    while (TIME_BEFORE(current_tick, timeout)) {
        ret = 0;
        last_disk_activity = current_tick;

        SD_STATE_REG = TRAN;
        BLOCK_COUNT_REG = count;
        sd_send_command(READ_MULTIPLE_BLOCK, start * BLOCK_SIZE, 0x1c25);
        sd_read_response(&dummy, 1);
        /* TODO: Don't assume BLOCK_SIZE == SECTOR_SIZE */

        for (i = 0; i < count * card->block_size / 2; i += FIFO_SIZE)
        {
            /* Wait for the FIFO to be full */
            while((STATUS_REG & FIFO_FULL) == 0) {}

            copy_read_sectors(buf, FIFO_SIZE);

            buf += FIFO_SIZE*2; /* Advance one chunk of 16 words */

            /* TODO: Switch bank if necessary */

            last_disk_activity = current_tick;
        }
        udelay(75);
        sd_send_command(STOP_TRANSMISSION, 0, 1);
        sd_read_response(&dummy, 1);

        response = 0;
        sd_wait_for_state(card, TRAN);
        break;
    }
    ata_led(false);
    ata_enable(false);

    mutex_unlock(&sd_mtx);

    return ret;
}


int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
/* Write support is not finished yet */
/* TODO: The standard suggests using ACMD23 prior to writing multiple blocks 
   to improve performance */
    unsigned int response;
    void const* write_buf;
    int ret = 0;
    unsigned int i;
    long timeout;
    tSDCardInfo *card = &card_info[current_card];

    mutex_lock(&sd_mtx);
    ata_enable(true);
    ata_led(true);
    if(current_card == 0)
    {
        if(start < BLOCKS_PER_BANK)
        {
            sd_select_bank(0);
        } else {
            sd_select_bank(1);
            start -= BLOCKS_PER_BANK;
        }
    }

retry:
    sd_wait_for_state(card, TRAN);
    BLOCK_COUNT_REG = count;
    sd_send_command(WRITE_MULTIPLE_BLOCK, start * SECTOR_SIZE, 0x1c2d);
    sd_read_response(&response, 1);
    write_buf = buf;
    for (i = 0; i < count * card->block_size / 2; i += FIFO_SIZE)
    {
        if(i >= (count * card->block_size / 2)-FIFO_SIZE)
        {
            /* Set SD_STATE_REG to PRG for the last buffer fill */
            SD_STATE_REG = PRG;
        }

        /* Wait for the FIFO to be empty */
        while((STATUS_REG & FIFO_EMPTY) == 0) {}
        /* Perhaps we could use bit 8 of card status (READY_FOR_DATA)? */

        copy_write_sectors(write_buf, FIFO_SIZE);

        write_buf += FIFO_SIZE*2; /* Advance one chunk of 16 words */
        /* TODO: Switch bank if necessary */

        last_disk_activity = current_tick;
    }

    timeout = current_tick + WRITE_TIMEOUT;

    while((STATUS_REG & DATA_DONE) == 0) {
        if(current_tick >= timeout)
        {
            sd_send_command(STOP_TRANSMISSION, 0, 1);
            sd_read_response(&response, 1);
            goto retry;
        }
    }
    sd_send_command(STOP_TRANSMISSION, 0, 1);
    sd_read_response(&response, 1);

    sd_wait_for_state(card, TRAN);
    ata_led(false);
    ata_enable(false);
    mutex_unlock(&sd_mtx);

    return ret;
}

static void sd_thread(void)
{
    struct event ev;
    bool idle_notified = false;
    
    while (1) {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);
        switch ( ev.id ) 
        {
            default:
                if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
                {
                    idle_notified = false;
                }
                else
                {
                    if (!idle_notified)
                    {
                        call_ata_idle_notifys(false);
                        idle_notified = true;
                    }
                }
                break;
        }
    }
}


void ata_spindown(int seconds)
{
    (void)seconds;
}

bool ata_disk_is_active(void)
{
  return 0;
}

void ata_sleep(void)
{
}

void ata_spin(void)
{
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
  return 0;
}

int ata_soft_reset(void)
{
  return 0;
}

void ata_enable(bool on)
{
    if(on)
    {
        DEV_EN |= DEV_ATA; /* Enable controller */
    }
    else
    {
        DEV_EN &= ~DEV_ATA; /* Disable controller */
    }
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

int ata_init(void)
{
    sd_init_device();
    if ( !initialized ) 
    {
        queue_init(&sd_queue, true);
        create_thread(sd_thread, sd_stack,
                      sizeof(sd_stack), sd_thread_name IF_PRIO(, PRIORITY_SYSTEM)
		      IF_COP(, CPU, false));
        initialized = true;
    }

    return 0;
}
