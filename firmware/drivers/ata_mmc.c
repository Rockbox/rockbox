/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "ata.h"
#include "ata_mmc.h"
#include "kernel.h"
#include "thread.h"
#include "led.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
#include "hwcompat.h"
#include "adc.h"
#include "bitswap.h"
#include "disk.h" /* for mount/unmount */

#define SECTOR_SIZE     512
#define MAX_BLOCK_SIZE  2048

/* Command definitions */
#define CMD_GO_IDLE_STATE        0x40  /* R1 */
#define CMD_SEND_OP_COND         0x41  /* R1 */
#define CMD_SEND_CSD             0x49  /* R1 */
#define CMD_SEND_CID             0x4a  /* R1 */
#define CMD_STOP_TRANSMISSION    0x4c  /* R1 */
#define CMD_SEND_STATUS          0x4d  /* R2 */
#define CMD_SET_BLOCKLEN         0x50  /* R1 */
#define CMD_READ_SINGLE_BLOCK    0x51  /* R1 */
#define CMD_READ_MULTIPLE_BLOCK  0x52  /* R1 */
#define CMD_WRITE_BLOCK          0x58  /* R1b */
#define CMD_WRITE_MULTIPLE_BLOCK 0x59  /* R1b */
#define CMD_READ_OCR             0x7a  /* R3 */

/* Response formats:
   R1  = single byte, msb=0, various error flags
   R1b = R1 + busy token(s)
   R2  = 2 bytes (1st byte identical to R1), additional flags
   R3  = 5 bytes (R1 + OCR register)
*/

#define R1_PARAMETER_ERR 0x40
#define R1_ADDRESS_ERR   0x20
#define R1_ERASE_SEQ_ERR 0x10
#define R1_COM_CRC_ERR   0x08
#define R1_ILLEGAL_CMD   0x04
#define R1_ERASE_RESET   0x02
#define R1_IN_IDLE_STATE 0x01

#define R2_OUT_OF_RANGE  0x80
#define R2_ERASE_PARAM   0x40
#define R2_WP_VIOLATION  0x20
#define R2_CARD_ECC_FAIL 0x10
#define R2_CC_ERROR      0x08
#define R2_ERROR         0x04
#define R2_ERASE_SKIP    0x02
#define R2_CARD_LOCKED   0x01

/* Data start tokens */

#define DT_START_BLOCK               0xfe
#define DT_START_WRITE_MULTIPLE      0xfc
#define DT_STOP_TRAN                 0xfd

/* for compatibility */
bool old_recorder = false; /* FIXME: get rid of this cross-dependency */
int ata_spinup_time = 0;
long last_disk_activity = -1;

/* private variables */

static struct mutex mmc_mutex;

#ifdef HAVE_HOTSWAP
static long mmc_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char mmc_thread_name[] = "mmc";
static struct event_queue mmc_queue;
#endif
static bool initialized = false;
static bool new_mmc_circuit;
static bool delayed_write = false;
static unsigned char delayed_sector[SECTOR_SIZE];
static int delayed_sector_num;

static enum {
    SER_POLL_WRITE,
    SER_POLL_READ,
    SER_DISABLED
} serial_mode;

static const unsigned char dummy[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

struct block_cache_entry {
    bool inuse;
#ifdef HAVE_MULTIVOLUME
    int drive;
#endif
    unsigned long blocknum;
    unsigned char data[MAX_BLOCK_SIZE+4];
    /* include start token, dummy crc, and an extra byte at the start 
     * to keep the data word aligned. */
};

/* 2 buffers used alternatively for writing, and also for reading
 * and sub-block writing if block size > sector size */
#define NUMCACHES 2
static struct block_cache_entry block_cache[NUMCACHES];
static int current_cache = 0;

static tCardInfo card_info[2];
#ifndef HAVE_MULTIVOLUME
static int current_card = 0;
#endif
static bool last_mmc_status = false;
static int countdown;  /* for mmc switch debouncing */
static bool usb_activity; /* monitoring the USB bridge */
static long last_usb_activity;

/* private function declarations */

static int select_card(int card_no);
static void deselect_card(void);
static void setup_sci1(int bitrate_register);
static void set_sci1_poll_read(void);
static void write_transfer(const unsigned char *buf, int len)
            __attribute__ ((section(".icode")));
static void read_transfer(unsigned char *buf, int len)
            __attribute__ ((section(".icode")));
static unsigned char poll_byte(long timeout);
static unsigned char poll_busy(long timeout);
static int send_cmd(int cmd, unsigned long parameter, unsigned char *response);
static int receive_cxd(unsigned char *buf);
static int initialize_card(int card_no);
static void swapcopy(void *dst, const void *src, unsigned long size);
static int receive_block(unsigned char *inbuf, unsigned char *swapbuf,
                         int size, long timeout);
static void swapcopy_block(const unsigned char *buf, int size);
static int send_block(const unsigned char *nextbuf, int size,
                      unsigned char start_token, long timeout);
static int cache_block(IF_MV2(int drive,) unsigned long blocknum,
                       int size, long timeout);
static void mmc_tick(void);

/* implementation */

void mmc_select_clock(int card_no)
{
    /* set clock gate for external card / reset for internal card if the
     * MMC clock polarity bit is 0, vice versa if it is 1 */
    if ((card_no != 0) ^ new_mmc_circuit)
        or_b(0x10, &PADRH);       /* set clock gate PA12 */
    else
        and_b(~0x10, &PADRH);     /* clear clock gate PA12 */
}

static int select_card(int card_no)
{
    mmc_select_clock(card_no);
    last_disk_activity = current_tick;

    if (!card_info[card_no].initialized)
    {
        setup_sci1(7); /* Initial rate: 375 kbps (need <= 400 per mmc specs) */
        write_transfer(dummy, 10); /* allow the card to synchronize */
        while (!(SSR1 & SCI_TEND));
    }

    if (card_no == 0)             /* internal */
        and_b(~0x04, &PADRH);     /* assert CS */
    else                          /* external */
        and_b(~0x02, &PADRH);     /* assert CS */

    if (card_info[card_no].initialized)
    {
        setup_sci1(card_info[card_no].bitrate_register);
        return 0;
    }
    else
    {
        return initialize_card(card_no);
    }
}

static void deselect_card(void)
{
    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    or_b(0x06, &PADRH);           /* deassert CS (both cards) */

    last_disk_activity = current_tick;
}

static void setup_sci1(int bitrate_register)
{
    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */

    SCR1 = 0;                     /* disable serial port */
    SMR1 = SYNC_MODE;             /* no prescale */
    BRR1 = bitrate_register;
    SSR1 = 0;

    SCR1 = SCI_TE;                /* enable transmitter */
    serial_mode = SER_POLL_WRITE;
}

static void set_sci1_poll_read(void)
{
    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    SCR1 = 0;                     /* disable transmitter (& receiver) */
    SCR1 = (SCI_TE|SCI_RE);       /* re-enable transmitter & receiver */
    while (!(SSR1 & SCI_TEND));   /* wait for SCI init completion (!) */
    serial_mode = SER_POLL_READ;
    TDR1 = 0xFF;                  /* send do-nothing while reading */
}

static void write_transfer(const unsigned char *buf, int len)
{
    const unsigned char *buf_end = buf + len;
    register unsigned char data;

    if (serial_mode != SER_POLL_WRITE)
    {
        while (!(SSR1 & SCI_TEND)); /* wait for end of transfer */
        SCR1 = 0;                   /* disable transmitter & receiver */
        SSR1 = 0;                   /* clear all flags */
        SCR1 = SCI_TE;              /* enable transmitter only */
        serial_mode = SER_POLL_WRITE;
    }

    while (buf < buf_end)
    {
        data = fliptable[(signed char)(*buf++)]; /* bitswap */
        while (!(SSR1 & SCI_TDRE));              /* wait for end of transfer */
        TDR1 = data;                             /* write byte */
        SSR1 = 0;                                /* start transmitting */
    }
}

/* don't call this with len == 0 */
static void read_transfer(unsigned char *buf, int len)
{
    unsigned char *buf_end = buf + len - 1;
    register signed char data;

    if (serial_mode != SER_POLL_READ)
        set_sci1_poll_read();

    SSR1 = 0;                     /* start receiving first byte */
    while (buf < buf_end)
    {
        while (!(SSR1 & SCI_RDRF)); /* wait for data */
        data = RDR1;                /* read byte */
        SSR1 = 0;                   /* start receiving */
        *buf++ = fliptable[data];   /* bitswap */
    }
    while (!(SSR1 & SCI_RDRF));     /* wait for last byte */
    *buf = fliptable[(signed char)(RDR1)]; /* read & bitswap */
}

/* returns 0xFF on timeout, timeout is in bytes */
static unsigned char poll_byte(long timeout)
{
    long i;
    unsigned char data = 0;       /* stop the compiler complaining */

    if (serial_mode != SER_POLL_READ)
        set_sci1_poll_read();

    i = 0;
    do {
        SSR1 = 0;                   /* start receiving */
        while (!(SSR1 & SCI_RDRF)); /* wait for data */
        data = RDR1;                /* read byte */
    } while ((data == 0xFF) && (++i < timeout));

    return fliptable[(signed char)data];
}

/* returns 0 on timeout, timeout is in bytes */
static unsigned char poll_busy(long timeout)
{
    long i;
    unsigned char data, dummy;
    
    if (serial_mode != SER_POLL_READ)
        set_sci1_poll_read();

    /* get data response */
    SSR1 = 0;                     /* start receiving */
    while (!(SSR1 & SCI_RDRF));   /* wait for data */
    data = fliptable[(signed char)(RDR1)];  /* read byte */

    /* wait until the card is ready again */
    i = 0;
    do {
        SSR1 = 0;                   /* start receiving */
        while (!(SSR1 & SCI_RDRF)); /* wait for data */
        dummy = RDR1;               /* read byte */
    } while ((dummy != 0xFF) && (++i < timeout));
    
    return (dummy == 0xFF) ? data : 0;
}

/* Send MMC command and get response */
static int send_cmd(int cmd, unsigned long parameter, unsigned char *response)
{
    unsigned char command[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95, 0xFF};

    command[0] = cmd;
    
    if (parameter != 0)
    {
        command[1] = (parameter >> 24) & 0xFF;
        command[2] = (parameter >> 16) & 0xFF;
        command[3] = (parameter >> 8) & 0xFF;
        command[4] = parameter & 0xFF;
    }
    
    write_transfer(command, 7);

    response[0] = poll_byte(20);

    if (response[0] != 0x00)
    {
        write_transfer(dummy, 1);
        return -1;
    }

    switch (cmd)
    {
        case CMD_SEND_CSD:        /* R1 response, leave open */
        case CMD_SEND_CID:
        case CMD_READ_SINGLE_BLOCK:
        case CMD_READ_MULTIPLE_BLOCK:
            break;
            
        case CMD_SEND_STATUS:     /* R2 response, close with dummy */
            read_transfer(response + 1, 1);
            write_transfer(dummy, 1);
            break;
            
        case CMD_READ_OCR:        /* R3 response, close with dummy */
            read_transfer(response + 1, 4);
            write_transfer(dummy, 1);
            break;

        default:                  /* R1 response, close with dummy */
            write_transfer(dummy, 1);
            break;                /* also catches block writes */
    }

    return 0;
}

/* Receive CID/ CSD data (16 bytes) */
static int receive_cxd(unsigned char *buf)
{
    if (poll_byte(20) != DT_START_BLOCK)
    {
        write_transfer(dummy, 1);
        return -1;                /* not start of data */
    }
    
    read_transfer(buf, 16);
    write_transfer(dummy, 3);     /* 2 bytes dontcare crc + 1 byte trailer */
    return 0;
}

/* helper function to extract n (<=32) bits from an arbitrary position.
   counting from MSB to LSB */
unsigned long mmc_extract_bits(
    const unsigned long *p, /* the start of the bitfield array */
    unsigned int start,     /* bit no. to start reading  */
    unsigned int size)      /* how many bits to read */
{
    unsigned int bit_index;
    unsigned int bits_to_use;
    unsigned long mask;
    unsigned long result;

    if (size == 1)
    {   /* short cut */
        return ((p[start/32] >> (31 - (start % 32))) & 1);
    }

    result = 0;
    while (size)
    {
        bit_index = start % 32;
        bits_to_use = MIN(32 - bit_index, size);
        mask = 0xFFFFFFFF >> (32 - bits_to_use);

        result <<= bits_to_use; /* start last round */
        result |= (p[start/32] >> (32 - bits_to_use - bit_index)) & mask;

        start += bits_to_use;
        size -= bits_to_use;
    }

    return result;
}

static int initialize_card(int card_no)
{
    int rc, i, temp;
    unsigned char response[5];
    tCardInfo *card = &card_info[card_no];

    static const char mantissa[] = {  /* *10 */
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
    };
    static const int exponent[] = {  /* use varies */
        1, 10, 100, 1000, 10000, 100000, 1000000,
        10000000, 100000000, 1000000000
    };

    /* switch to SPI mode */
    send_cmd(CMD_GO_IDLE_STATE, 0, response);
    if (response[0] != 0x01)
        return -1;                /* error response */

    /* initialize card */
    for (i = 0; i < 100; i++)     /* timeout 1 sec */
    {
        sleep(1);
        if (send_cmd(CMD_SEND_OP_COND, 0, response) == 0)
            break;
    }
    if (response[0] != 0x00)
        return -2;                /* not ready */
        
    /* get OCR register */
    rc = send_cmd(CMD_READ_OCR, 0, response);
    if (rc)
        return rc * 10 - 3;
    card->ocr = (response[1] << 24) | (response[2] << 16)
              | (response[3] << 8) | response[4];
        
    /* check voltage */
    if (!(card->ocr & 0x00100000)) /* 3.2 .. 3.3 V */
        return -4;
    
    /* get CSD register */
    rc = send_cmd(CMD_SEND_CSD, 0, response);
    if (rc)
        return rc * 10 - 5;
    rc = receive_cxd((unsigned char*)card->csd);
    if (rc)
        return rc * 10 - 6;

    /* check block sizes */
    card->block_exp = mmc_extract_bits(card->csd, 44, 4);
    card->blocksize = 1 << card->block_exp;
    if ((mmc_extract_bits(card->csd, 102, 4) != card->block_exp)
        || card->blocksize > MAX_BLOCK_SIZE)
    {
        return -7;
    }

    if (card->blocksize != SECTOR_SIZE)
    {
        rc = send_cmd(CMD_SET_BLOCKLEN, card->blocksize, response);
        if (rc)
            return rc * 10 - 8;
    }

    /* max transmission speed, clock divider */
    temp = mmc_extract_bits(card->csd, 29, 3);
    temp = (temp > 3) ? 3 : temp;
    card->speed = mantissa[mmc_extract_bits(card->csd, 25, 4)]
                * exponent[temp + 4];
    card->bitrate_register = (FREQ/4-1) / card->speed;

    /* NSAC, TSAC, read timeout */
    card->nsac = 100 * mmc_extract_bits(card->csd, 16, 8);
    card->tsac = mantissa[mmc_extract_bits(card->csd, 9, 4)];
    temp = mmc_extract_bits(card->csd, 13, 3);
    card->read_timeout = ((FREQ/4) / (card->bitrate_register + 1)
                         * card->tsac / exponent[9 - temp]
                         + (10 * card->nsac));
    card->read_timeout /= 8;      /* clocks -> bytes */
    card->tsac = card->tsac * exponent[temp] / 10;

    /* r2w_factor, write timeout */
    card->r2w_factor = 1 << mmc_extract_bits(card->csd, 99, 3);
    if (card->r2w_factor > 32)    /* dirty MMC spec violation */
    {
        card->read_timeout *= 4;  /* add safety factor */
        card->write_timeout = card->read_timeout * 8;
    }
    else
        card->write_timeout = card->read_timeout * card->r2w_factor;

    /* card size */
    card->numblocks = (mmc_extract_bits(card->csd, 54, 12) + 1)
                      * (1 << (mmc_extract_bits(card->csd, 78, 3) + 2));
    card->size = card->numblocks * card->blocksize;

    /* switch to full speed */
    setup_sci1(card->bitrate_register);
    
    /* get CID register */
    rc = send_cmd(CMD_SEND_CID, 0, response);
    if (rc)
        return rc * 10 - 9;
    rc = receive_cxd((unsigned char*)card->cid);
    if (rc)
        return rc * 10 - 9;

    card->initialized = true;
    return 0;
}

tCardInfo *mmc_card_info(int card_no)
{
    tCardInfo *card = &card_info[card_no];
    
    if (!card->initialized && ((card_no == 0) || mmc_detect()))
    {
        mutex_lock(&mmc_mutex);
        select_card(card_no);
        deselect_card();
        mutex_unlock(&mmc_mutex);
    }
    return card;
}

static void swapcopy(void *dst, const void *src, unsigned long size)
{
    memcpy(dst, src, size);
    bitswap(dst, size);
}

/* Receive one block with dma, possibly swapping the previously received
 * block in the background */
static int receive_block(unsigned char *inbuf, unsigned char *swapbuf,
                         int size, long timeout)
{
    if (poll_byte(timeout) != DT_START_BLOCK)
    {
        write_transfer(dummy, 1);
        return -1;                /* not start of data */
    }

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */

    SCR1 = 0;                     /* disable serial */
    SSR1 = 0;                     /* clear all flags */
    
    /* setup DMA channel 2 */
    CHCR2 = 0;                    /* disable */
    SAR2 = RDR1_ADDR;
    DAR2 = (unsigned long) inbuf;
    DTCR2 = size;
    CHCR2 = 0x4601;               /* fixed source address, RXI1, enable */
    DMAOR = 0x0001;
    SCR1 = (SCI_RE|SCI_RIE);      /* kick off DMA */
    
    /* dma receives 2 bytes more than DTCR2, but the last 2 bytes are not
     * stored. The first extra byte is available from RDR1 after the DMA ends,
     * the second one is lost because of the SCI overrun. However, this 
     * behaviour conveniently discards the crc. */

    if (swapbuf != NULL)          /* bitswap previous block */
        bitswap(swapbuf, size);
    yield();                      /* be nice */

    while (!(CHCR2 & 0x0002));    /* wait for end of DMA */
    while (!(SSR1 & SCI_ORER));   /* wait for the trailing bytes */
    SCR1 = 0;
    serial_mode = SER_DISABLED;

    write_transfer(dummy, 1);     /* send trailer */
    return 0;
}

/* copies one block into the next-current block cache, then bitswaps */
static void swapcopy_block(const unsigned char *buf, int size)
{
    current_cache = (current_cache + 1) % NUMCACHES; /* next cache */

    block_cache[current_cache].inuse = false;
    swapcopy(block_cache[current_cache].data + 2, buf, size);
}

/* Send one block with dma from the current block cache, possibly preparing
 * the next block within the next block cache in the background. */
static int send_block(const unsigned char *nextbuf, int size,
                      unsigned char start_token, long timeout)
{
    int rc = 0;
    unsigned char *curbuf = block_cache[current_cache].data;
    
    curbuf[1] = fliptable[(signed char)start_token];
    *(unsigned short *)(curbuf + size + 2) = 0xFFFF;

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */

    SCR1 = 0;                     /* disable serial */
    SSR1 = 0;                     /* clear all flags */

    /* setup DMA channel 2 */
    CHCR2 = 0;                    /* disable */
    SAR2 = (unsigned long)(curbuf + 1);
    DAR2 = TDR1_ADDR;
    DTCR2 = size + 3;             /* start token + block + dummy crc */
    CHCR2 = 0x1701;               /* fixed dest. address, TXI1, enable */
    DMAOR = 0x0001;
    SCR1 = (SCI_TE|SCI_TIE);      /* kick off DMA */

    if (nextbuf != NULL)          /* prepare next sector */
        swapcopy_block(nextbuf, size);
    yield();                      /* be nice */

    while (!(CHCR2 & 0x0002));    /* wait for end of DMA */
    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    SCR1 = 0;
    serial_mode = SER_DISABLED;

    if ((poll_busy(timeout) & 0x1F) != 0x05) /* something went wrong */
        rc = -1;

    write_transfer(dummy, 1);

    return rc;
}

static int cache_block(IF_MV2(int drive,) unsigned long blocknum,
                       int size, long timeout)
{
    int rc, i;
    unsigned char response;
    
    /* check whether the block is already cached */
    for (i = 0; i < NUMCACHES; i++)
    {
        if (block_cache[i].inuse && (block_cache[i].blocknum == blocknum)
#ifdef HAVE_MULTIVOLUME
            && (block_cache[i].drive == drive)
#endif
           )
        {
            current_cache = i;
            return 0;
        }
    }
    /* not found: read the block */
    current_cache = (current_cache + 1) % NUMCACHES;
    rc = send_cmd(CMD_READ_SINGLE_BLOCK, blocknum * size, &response);
    if (rc)
        return rc * 10 - 1;

    block_cache[current_cache].inuse = false;
    rc = receive_block(block_cache[current_cache].data + 2, NULL,
                       size, timeout);
    if (rc)
        return rc * 10 - 2;

#ifdef HAVE_MULTIVOLUME
    block_cache[current_cache].drive = drive;
#endif
    block_cache[current_cache].blocknum = blocknum;
    block_cache[current_cache].inuse = true;
    last_disk_activity = current_tick;

    return 0;
}

int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{
    int rc = 0;
    unsigned int blocksize, offset;
    unsigned long c_addr, c_end_addr;
    unsigned long c_block, c_end_block;
    unsigned char response;
    void *inbuf_prev = NULL;
    tCardInfo *card;

    c_addr = start * SECTOR_SIZE;
    c_end_addr = c_addr + incount * SECTOR_SIZE;
    
    mutex_lock(&mmc_mutex);
    led(true);
#ifdef HAVE_MULTIVOLUME
    card = &card_info[drive];
    rc = select_card(drive);
#else
    card = &card_info[current_card];
    rc = select_card(current_card);
#endif
    if (rc)
    {
        rc = rc * 10 - 1;
        goto error;
    }
    if (c_end_addr > card->size)
    {
        rc = -2;
        goto error;
    }

    blocksize = card->blocksize;
    offset = c_addr & (blocksize - 1);
    c_block = c_addr >> card->block_exp;
    c_end_block = c_end_addr >> card->block_exp;

    if (offset) /* first partial block */
    {
        unsigned long len = MIN(c_end_addr - c_addr, blocksize - offset);

        rc = cache_block(IF_MV2(drive,) c_block, blocksize,
                         card->read_timeout);
        if (rc)
        {
            rc = rc * 10 - 3;
            goto error;
        }
        swapcopy(inbuf, block_cache[current_cache].data + 2 + offset, len);
        inbuf += len;
        c_addr += len;
        c_block++;
    }
    /* some cards don't like reading the very last block with
     * CMD_READ_MULTIPLE_BLOCK, so make sure this block is always
     * read with CMD_READ_SINGLE_BLOCK. Let the 'last partial block'
     * read catch this. */
    if (c_end_block == card->numblocks)
        c_end_block--;

    if (c_block < c_end_block)
    {
        int read_cmd = (c_end_block - c_block > 1) ?
                       CMD_READ_MULTIPLE_BLOCK : CMD_READ_SINGLE_BLOCK;

        rc = send_cmd(read_cmd, c_addr, &response);
        if (rc)
        {
            rc = rc * 10 - 4;
            goto error;
        }
        while (c_block < c_end_block)
        {
            rc = receive_block(inbuf, inbuf_prev, blocksize,
                               card->read_timeout);
            if (rc)
            {
                rc = rc * 10 - 5;
                goto error;
            }
            last_disk_activity = current_tick;
            inbuf_prev = inbuf;
            inbuf += blocksize;
            c_addr += blocksize;
            c_block++;
        }
        if (read_cmd == CMD_READ_MULTIPLE_BLOCK)
        {
            rc = send_cmd(CMD_STOP_TRANSMISSION, 0, &response);
            if (rc)
            {
                rc = rc * 10 - 6;
                goto error;
            }
        }
        bitswap(inbuf_prev, blocksize);
    }
    if (c_addr < c_end_addr) /* last partial block */
    {
        rc = cache_block(IF_MV2(drive,) c_block, blocksize,
                         card->read_timeout);
        if (rc)
        {
            rc = rc * 10 - 7;
            goto error;
        }
        swapcopy(inbuf, block_cache[current_cache].data + 2,
                 c_end_addr - c_addr);
    }

  error:

    deselect_card();
    led(false);
    mutex_unlock(&mmc_mutex);
    
    /* only flush if reading went ok */
    if ( (rc == 0) && delayed_write )
        ata_flush();

    return rc;
}

int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    int rc = 0;
    unsigned int blocksize, offset;
    unsigned long c_addr, c_end_addr;
    unsigned long c_block, c_end_block;
    unsigned char response;
    tCardInfo *card;

    if (start == 0)
        panicf("Writing on sector 0\n");

    c_addr = start * SECTOR_SIZE;
    c_end_addr = c_addr + count * SECTOR_SIZE;

    mutex_lock(&mmc_mutex);
    led(true);
#ifdef HAVE_MULTIVOLUME
    card = &card_info[drive];
    rc = select_card(drive);
#else
    card = &card_info[current_card];
    rc = select_card(current_card);
#endif
    if (rc)
    {
        rc = rc * 10 - 1;
        goto error;
    }

    if (c_end_addr  > card->size)
        panicf("Writing past end of card\n");

    blocksize = card->blocksize;
    offset = c_addr & (blocksize - 1);
    c_block = c_addr >> card->block_exp;
    c_end_block = c_end_addr >> card->block_exp;

    /* Special case: first block is trimmed at both ends. May only happen
     * if (blocksize > 2 * sectorsize), i.e. blocksize == 2048 */
    if ((c_block == c_end_block) && offset)
        c_end_block++;

    if (c_block < c_end_block)
    {
        int write_cmd;
        unsigned char start_token;
        
        if (c_end_block - c_block > 1)
        {
            write_cmd   = CMD_WRITE_MULTIPLE_BLOCK;
            start_token = DT_START_WRITE_MULTIPLE;
        }
        else
        {
            write_cmd   = CMD_WRITE_BLOCK;
            start_token = DT_START_BLOCK;
        }
        
        if (offset)
        {
            unsigned long len = MIN(c_end_addr - c_addr, blocksize - offset);
            
            rc = cache_block(IF_MV2(drive,) c_block, blocksize,
                         card->read_timeout);
            if (rc)
            {
                rc = rc * 10 - 2;
                goto error;
            }
            swapcopy(block_cache[current_cache].data + 2 + offset, buf, len);
            buf += len;
            c_addr -= offset;
        }
        else
        {
            swapcopy_block(buf, blocksize);
            buf += blocksize;
        }
        rc = send_cmd(write_cmd, c_addr, &response);
        if (rc)
        {
            rc = rc * 10 - 3;
            goto error;
        }
        c_block++;  /* early increment to simplify the loop */
        
        while (c_block < c_end_block)
        {
            rc = send_block(buf, blocksize, start_token, card->write_timeout);
            if (rc)
            {
                rc = rc * 10 - 4;
                goto error;
            }
            last_disk_activity = current_tick;
            buf += blocksize;
            c_addr += blocksize;
            c_block++;
        }
        rc = send_block(NULL, blocksize, start_token, card->write_timeout);
        if (rc)
        {
            rc = rc * 10 - 5;
            goto error;
        }
        last_disk_activity = current_tick;
        c_addr += blocksize;
        /* c_block++ was done early */

        if (write_cmd == CMD_WRITE_MULTIPLE_BLOCK)
        {
            response = DT_STOP_TRAN;
            write_transfer(&response, 1);
            poll_busy(card->write_timeout);
        }
    }
    
    if (c_addr < c_end_addr) /* last partial block */
    {
        rc = cache_block(IF_MV2(drive,) c_block, blocksize,
                         card->read_timeout);
        if (rc)
        {
            rc = rc * 10 - 6;
            goto error;
        }
        swapcopy(block_cache[current_cache].data + 2, buf,
                 c_end_addr - c_addr);
        rc = send_cmd(CMD_WRITE_BLOCK, c_addr, &response);
        if (rc)
        {
            rc = rc * 10 - 7;
            goto error;
        }
        rc = send_block(NULL, blocksize, DT_START_BLOCK, card->write_timeout);
        if (rc)
        {
            rc = rc * 10 - 8;
            goto error;
        }
    }

  error:

    deselect_card();
    led(false);
    mutex_unlock(&mmc_mutex);

    /* only flush if writing went ok */
    if ( (rc == 0) && delayed_write )
        ata_flush();

    return rc;
}

/* While there is no spinup, the delayed write is still here to avoid
   wearing the flash unnecessarily */
extern void ata_delayed_write(unsigned long sector, const void* buf)
{
    memcpy(delayed_sector, buf, SECTOR_SIZE);
    delayed_sector_num = sector;
    delayed_write = true;
}

/* write the delayed sector to volume 0 */
extern void ata_flush(void)
{
    if ( delayed_write ) 
    {
        DEBUGF("ata_flush()\n");
        delayed_write = false;
        ata_write_sectors(IF_MV2(0,) delayed_sector_num, 1, delayed_sector);
    }
}

void ata_spindown(int seconds)
{
    (void)seconds;
}

bool ata_disk_is_active(void)
{
    /* this is correct unless early return from write gets implemented */
    return mmc_mutex.locked;
}

int ata_standby(int time)
{
    (void)time;

    return 0;
}

int ata_sleep(void)
{
    return 0;
}

void ata_spin(void)
{
}

#ifdef HAVE_HOTSWAP
static void mmc_thread(void)
{
    struct event ev;
    
    while (1) {
        queue_wait(&mmc_queue, &ev);
        switch ( ev.id ) 
        {
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&mmc_queue);
                break;

            case SYS_MMC_INSERTED:
                disk_mount(1); /* mount MMC */
                queue_broadcast(SYS_FS_CHANGED, NULL);
                break;

            case SYS_MMC_EXTRACTED:
                disk_unmount(1); /* release "by force" */
                queue_broadcast(SYS_FS_CHANGED, NULL);
                break;
        }
    }
}
#endif /* #ifdef HAVE_HOTSWAP */

bool mmc_detect(void)
{
    return adc_read(ADC_MMC_SWITCH) < 0x200 ? true : false;
}

bool mmc_usb_active(int delayticks)
{
    /* reading "inactive" is delayed by user-supplied monoflop value */
    return (usb_activity ||
            TIME_BEFORE(current_tick, last_usb_activity + delayticks));
}

static void mmc_tick(void)
{
    bool current_status;

    if (new_mmc_circuit)
        /* USB bridge activity is 0 on idle, ~527 on active */
        current_status = adc_read(ADC_USB_ACTIVE) > 0x100;
    else
        current_status = adc_read(ADC_USB_ACTIVE) < 0x190;

    if (!current_status && usb_activity)
        last_usb_activity = current_tick;
    usb_activity = current_status;

    current_status = mmc_detect();
    /* Only report when the status has changed */
    if (current_status != last_mmc_status)
    {
        last_mmc_status = current_status;
        countdown = 30;
    }
    else
    {
        /* Count down until it gets negative */
        if (countdown >= 0)
            countdown--;

        if (countdown == 0)
        {
            if (current_status)
            {
                queue_broadcast(SYS_MMC_INSERTED, NULL);
            }
            else
            {
                queue_broadcast(SYS_MMC_EXTRACTED, NULL);
                card_info[1].initialized = false;
            }
        }
    }
}

int ata_soft_reset(void)
{
    return 0;
}

void ata_enable(bool on)
{
    PBCR1 &= ~0x0CF0; /* PB13, PB11 and PB10 become GPIOs, if not modified below */
    PACR2 &= ~0x4000; /* use PA7 (bridge reset) as GPIO */
    if (on)
    {
        PBCR1 |= 0x08A0;    /* as SCK1, TxD1, RxD1 */
        IPRE &= 0x0FFF;     /* disable SCI1 interrupts for the CPU */
    }
    and_b(~0x80, &PADRL); /* assert reset */
    sleep(HZ/20);
    or_b(0x80, &PADRL);   /* de-assert reset */
    sleep(HZ/20);
    card_info[0].initialized = false;
    card_info[1].initialized = false;
}

int ata_init(void)
{
    int rc = 0;

    mutex_init(&mmc_mutex);

    led(false);

    /* Port setup */
    PACR1 &= ~0x0F00; /* GPIO function for PA12, /IRQ1 for PA13 */
    PACR1 |= 0x0400;
    PADR |= 0x0680;   /* set all the selects + reset high (=inactive) */
    PAIOR |= 0x1680;  /* make outputs for them and the PA12 clock gate */

    PBDR |= 0x2C00;   /* SCK1, TxD1 and RxD1 high when GPIO  CHECKME: mask */
    PBIOR |= 0x2000;  /* SCK1 output */
    PBIOR &= ~0x0C00; /* TxD1, RxD1 input */

    last_mmc_status = mmc_detect();
#ifndef HAVE_MULTIVOLUME
    if (last_mmc_status)
    {   /* MMC inserted */
        current_card = 1;
    }
    else
    {   /* no MMC, use internal memory */
        current_card = 0;
    }
#endif

    ata_enable(true);
    
    if ( !initialized ) 
    {
        new_mmc_circuit = ((read_hw_mask() & MMC_CLOCK_POLARITY) != 0);
#ifdef HAVE_HOTSWAP
        queue_init(&mmc_queue);
        create_thread(mmc_thread, mmc_stack,
                      sizeof(mmc_stack), mmc_thread_name);
#endif
        tick_add_task(mmc_tick);
        initialized = true;
    }

    return rc;
}

