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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "ata.h"
#include "ata_mmc.h"
#include "ata_idle_notify.h"
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

#define BLOCK_SIZE  512   /* fixed */

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
int ata_spinup_time = 0;
long last_disk_activity = -1;

/* private variables */

static struct mutex mmc_mutex;

#ifdef HAVE_HOTSWAP
static bool mmc_monitor_enabled = true;
static long mmc_stack[((DEFAULT_STACK_SIZE*2) + 0x800)/sizeof(long)];
#else
static long mmc_stack[(DEFAULT_STACK_SIZE*2)/sizeof(long)];
#endif
static const char mmc_thread_name[] = "mmc";
static struct event_queue mmc_queue;
static bool initialized = false;
static bool new_mmc_circuit;

static enum {
    MMC_UNKNOWN,
    MMC_UNTOUCHED,
    MMC_TOUCHED 
} mmc_status = MMC_UNKNOWN;

static enum {
    SER_POLL_WRITE,
    SER_POLL_READ,
    SER_DISABLED
} serial_mode;

static const unsigned char dummy[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/* 2 buffers used alternatively for writing, including start token,
 * dummy CRC and an extra byte to keep word alignment. */
static unsigned char write_buffer[2][BLOCK_SIZE+4];
static int current_buffer = 0;
static const unsigned char *send_block_addr = NULL;

static tCardInfo card_info[2];
#ifndef HAVE_MULTIVOLUME
static int current_card = 0;
#endif
static bool last_mmc_status = false;
static int countdown = HZ/3; /* for mmc switch debouncing */
static bool usb_activity;    /* monitoring the USB bridge */
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
static unsigned char send_cmd(int cmd, unsigned long parameter, void *data);
static int receive_cxd(unsigned char *buf);
static int initialize_card(int card_no);
static int receive_block(unsigned char *inbuf, long timeout);
static void send_block_prepare(void);
static int send_block_send(unsigned char start_token, long timeout,
                           bool prepare_next);
static void mmc_tick(void);

/* implementation */

void mmc_enable_int_flash_clock(bool on)
{
    /* Internal flash clock is enabled by setting PA12 high with the new
     * clock circuit, and by setting it low with the old clock circuit */
    if (on ^ new_mmc_circuit)
        and_b(~0x10, &PADRH);     /* clear clock gate PA12 */
    else
        or_b(0x10, &PADRH);       /* set clock gate PA12 */
}

static int select_card(int card_no)
{
    mutex_lock(&mmc_mutex);
    led(true);
    last_disk_activity = current_tick;

    mmc_enable_int_flash_clock(card_no == 0);

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

    led(false);
    mutex_unlock(&mmc_mutex);
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

/* Send MMC command and get response. Returns R1 byte directly.
 * Returns further R2 or R3 bytes in *data (can be NULL for other commands) */
static unsigned char send_cmd(int cmd, unsigned long parameter, void *data)
{
    static struct {
        unsigned char cmd;
        unsigned long parameter;
        const unsigned char crc7;    /* fixed, valid for CMD0 only */
        const unsigned char trailer;
    } __attribute__((packed)) command = {0x40, 0, 0x95, 0xFF};

    unsigned char ret;

    command.cmd = cmd;
    command.parameter = htobe32(parameter);

    write_transfer((unsigned char *)&command, sizeof(command));

    ret = poll_byte(20);

    switch (cmd)
    {
        case CMD_SEND_CSD:        /* R1 response, leave open */
        case CMD_SEND_CID:
        case CMD_READ_SINGLE_BLOCK:
        case CMD_READ_MULTIPLE_BLOCK:
            return ret;

        case CMD_SEND_STATUS:     /* R2 response, close with dummy */
            read_transfer(data, 1);
            break;

        case CMD_READ_OCR:        /* R3 response, close with dummy */
            read_transfer(data, 4);
            break;

        default:                  /* R1 response, close with dummy */
            break;                /* also catches block writes */
    }
    write_transfer(dummy, 1);
    return ret;
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


static int initialize_card(int card_no)
{
    int rc, i;
    int blk_exp, ts_exp, taac_exp;
    tCardInfo *card = &card_info[card_no];

    static const char mantissa[] = {  /* *10 */
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
    };
    static const int exponent[] = {  /* use varies */
        1, 10, 100, 1000, 10000, 100000, 1000000,
        10000000, 100000000, 1000000000
    };

    if (card_no == 1)
        mmc_status = MMC_TOUCHED;

    /* switch to SPI mode */
    if (send_cmd(CMD_GO_IDLE_STATE, 0, NULL) != 0x01)
        return -1;                /* error or no response */

    /* initialize card */
    for (i = HZ;;)                /* try for 1 second*/
    {
        sleep(1);
        if (send_cmd(CMD_SEND_OP_COND, 0, NULL) == 0)
            break;
        if (--i <= 0)
            return -2;            /* timeout */
    }

    /* get OCR register */
    if (send_cmd(CMD_READ_OCR, 0, &card->ocr))
        return -3;
    card->ocr = betoh32(card->ocr); /* no-op on big endian */

    /* check voltage */
    if (!(card->ocr & 0x00100000)) /* 3.2 .. 3.3 V */
        return -4;

    /* get CSD register */
    if (send_cmd(CMD_SEND_CSD, 0, NULL))
        return -5;
    rc = receive_cxd((unsigned char*)card->csd);
    if (rc)
        return rc * 10 - 5;

    blk_exp = card_extract_bits(card->csd, 44, 4);
    if (blk_exp < 9) /* block size < 512 bytes not supported */
        return -6;

    card->numblocks = (card_extract_bits(card->csd, 54, 12) + 1)
                   << (card_extract_bits(card->csd, 78, 3) + 2 + blk_exp - 9);
    card->blocksize = BLOCK_SIZE;

    /* max transmission speed, clock divider */
    ts_exp = card_extract_bits(card->csd, 29, 3);
    ts_exp = (ts_exp > 3) ? 3 : ts_exp;
    card->speed = mantissa[card_extract_bits(card->csd, 25, 4)]
                * exponent[ts_exp + 4];
    card->bitrate_register = (FREQ/4-1) / card->speed;

    /* NSAC, TSAC, read timeout */
    card->nsac = 100 * card_extract_bits(card->csd, 16, 8);
    card->tsac = mantissa[card_extract_bits(card->csd, 9, 4)];
    taac_exp = card_extract_bits(card->csd, 13, 3);
    card->read_timeout = ((FREQ/4) / (card->bitrate_register + 1)
                         * card->tsac / exponent[9 - taac_exp]
                         + (10 * card->nsac));
    card->read_timeout /= 8;      /* clocks -> bytes */
    card->tsac = card->tsac * exponent[taac_exp] / 10;

    /* r2w_factor, write timeout */
    card->r2w_factor = 1 << card_extract_bits(card->csd, 99, 3);
    card->write_timeout = card->read_timeout * card->r2w_factor;

    if (card->r2w_factor > 32) /* Such cards often need extra read delay */
        card->read_timeout *= 4;

    /* switch to full speed */
    setup_sci1(card->bitrate_register);

    /* always use 512 byte blocks */
    if (send_cmd(CMD_SET_BLOCKLEN, BLOCK_SIZE, NULL))
        return -7;

    /* get CID register */
    if (send_cmd(CMD_SEND_CID, 0, NULL))
        return -8;
    rc = receive_cxd((unsigned char*)card->cid);
    if (rc)
        return rc * 10 - 8;

    card->initialized = true;
    return 0;
}

tCardInfo *mmc_card_info(int card_no)
{
    tCardInfo *card = &card_info[card_no];
    
    if (!card->initialized && ((card_no == 0) || mmc_detect()))
    {
        select_card(card_no);
        deselect_card();
    }
    return card;
}

/* Receive one block with DMA and bitswap it (chasing bitswap). */
static int receive_block(unsigned char *inbuf, long timeout)
{
    unsigned long buf_end;

    if (poll_byte(timeout) != DT_START_BLOCK)
    {
        write_transfer(dummy, 1);
        return -1;                /* not start of data */
    }

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */

    SCR1 = 0;                     /* disable serial */
    SSR1 = 0;                     /* clear all flags */
    
    /* setup DMA channel 0 */
    CHCR0 = 0;                    /* disable */
    SAR0 = RDR1_ADDR;
    DAR0 = (unsigned long) inbuf;
    DTCR0 = BLOCK_SIZE;
    CHCR0 = 0x4601;               /* fixed source address, RXI1, enable */
    DMAOR = 0x0001;
    SCR1 = (SCI_RE|SCI_RIE);      /* kick off DMA */

    /* DMA receives 2 bytes more than DTCR2, but the last 2 bytes are not
     * stored. The first extra byte is available from RDR1 after the DMA ends,
     * the second one is lost because of the SCI overrun. However, this
     * behaviour conveniently discards the crc. */

    yield();                      /* be nice */
    
    /* Bitswap received data, chasing the DMA pointer */
    buf_end = (unsigned long)inbuf + BLOCK_SIZE;
    do
    {
        /* Call bitswap whenever (a multiple of) 8 bytes are
         * available (value optimised by experimentation). */
        int swap_now = (DAR0 - (unsigned long)inbuf) & ~0x00000007;
        if (swap_now)
        {
            bitswap(inbuf, swap_now);
            inbuf += swap_now;
        }
    }
    while ((unsigned long)inbuf < buf_end);

    while (!(CHCR0 & 0x0002));    /* wait for end of DMA */
    while (!(SSR1 & SCI_ORER));   /* wait for the trailing bytes */
    SCR1 = 0;
    serial_mode = SER_DISABLED;

    write_transfer(dummy, 1);     /* send trailer */
    last_disk_activity = current_tick;
    return 0;
}

/* Prepare a block for sending by copying it to the next write buffer
 * and bitswapping it. */
static void send_block_prepare(void)
{
    unsigned char *dest;

    current_buffer ^= 1; /* toggle buffer */
    dest = write_buffer[current_buffer] + 2;

    memcpy(dest, send_block_addr, BLOCK_SIZE);
    bitswap(dest, BLOCK_SIZE);

    send_block_addr += BLOCK_SIZE;
}

/* Send one block with DMA from the current write buffer, possibly preparing
 * the next block within the next write buffer in the background. */
static int send_block_send(unsigned char start_token, long timeout,
                           bool prepare_next)
{
    int rc = 0;
    unsigned char *curbuf = write_buffer[current_buffer];

    curbuf[1] = fliptable[(signed char)start_token];
    *(unsigned short *)(curbuf + BLOCK_SIZE + 2) = 0xFFFF;

    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */

    SCR1 = 0;                     /* disable serial */
    SSR1 = 0;                     /* clear all flags */

    /* setup DMA channel 0 */
    CHCR0 = 0;                    /* disable */
    SAR0 = (unsigned long)(curbuf + 1);
    DAR0 = TDR1_ADDR;
    DTCR0 = BLOCK_SIZE + 3;       /* start token + block + dummy crc */
    CHCR0 = 0x1701;               /* fixed dest. address, TXI1, enable */
    DMAOR = 0x0001;
    SCR1 = (SCI_TE|SCI_TIE);      /* kick off DMA */

    if (prepare_next)
        send_block_prepare();
    yield();                      /* be nice */

    while (!(CHCR0 & 0x0002));    /* wait for end of DMA */
    while (!(SSR1 & SCI_TEND));   /* wait for end of transfer */
    SCR1 = 0;
    serial_mode = SER_DISABLED;

    if ((poll_busy(timeout) & 0x1F) != 0x05) /* something went wrong */
        rc = -1;

    write_transfer(dummy, 1);
    last_disk_activity = current_tick;

    return rc;
}

int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{
    int rc = 0;
    int lastblock = 0;
    unsigned long end_block;
    tCardInfo *card;
#ifndef HAVE_MULTIVOLUME
    int drive = current_card;
#endif

    card = &card_info[drive];
    rc = select_card(drive);
    if (rc)
    {
        rc = rc * 10 - 1;
        goto error;
    }

    end_block = start + incount;
    if (end_block > card->numblocks)
    {
        rc = -2;
        goto error;
    }

    /* Some cards don't like reading the very last block with
     * CMD_READ_MULTIPLE_BLOCK, so make sure this block is always
     * read with CMD_READ_SINGLE_BLOCK. */
    if (end_block == card->numblocks)
        lastblock = 1;
        
    if (incount > 1)
    {
                     /* MMC4.2: make multiplication conditional */
        if (send_cmd(CMD_READ_MULTIPLE_BLOCK, start * BLOCK_SIZE, NULL))
        {
            rc =  -3;
            goto error;
        }
        while (--incount >= lastblock)
        {
            rc = receive_block(inbuf, card->read_timeout);
            if (rc)
            {
                /* If an error occurs during multiple block reading, the
                 * host still needs to send CMD_STOP_TRANSMISSION */
                send_cmd(CMD_STOP_TRANSMISSION, 0, NULL);
                rc = rc * 10 - 4;
                goto error;
            }
            inbuf += BLOCK_SIZE;
            start++;
            /* ^^ necessary for the abovementioned last block special case */
        }
        if (send_cmd(CMD_STOP_TRANSMISSION, 0, NULL))
        {
            rc = -5;
            goto error;
        }
    }
    if (incount > 0)
    {
                     /* MMC4.2: make multiplication conditional */
        if (send_cmd(CMD_READ_SINGLE_BLOCK, start * BLOCK_SIZE, NULL))
        {
            rc = -6;
            goto error;
        }
        rc = receive_block(inbuf, card->read_timeout);
        if (rc)
        {
            rc = rc * 10 - 7;
            goto error;
        }
    }

  error:

    deselect_card();
    
    return rc;
}

int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    int rc = 0;
    int write_cmd;
    unsigned char start_token;
    tCardInfo *card;
#ifndef HAVE_MULTIVOLUME
    int drive = current_card;
#endif

    card = &card_info[drive];
    rc = select_card(drive);
    if (rc)
    {
        rc = rc * 10 - 1;
        goto error;
    }
    
    if (start + count  > card->numblocks)
        panicf("Writing past end of card");

    send_block_addr = buf;
    send_block_prepare();

    if (count > 1)
    {
        write_cmd   = CMD_WRITE_MULTIPLE_BLOCK;
        start_token = DT_START_WRITE_MULTIPLE;
    }
    else
    {
        write_cmd   = CMD_WRITE_BLOCK;
        start_token = DT_START_BLOCK;
    }
                 /* MMC4.2: make multiplication conditional */
    if (send_cmd(write_cmd, start * BLOCK_SIZE, NULL))
    {
        rc = -2;
        goto error;
    }
    while (--count >= 0)
    {
        rc = send_block_send(start_token, card->write_timeout, count > 0);
        if (rc)
        {
            rc = rc * 10 - 3;
            break;
            /* If an error occurs during multiple block writing,
             * the STOP_TRAN token still needs to be sent. */
        }
    }
    if (write_cmd == CMD_WRITE_MULTIPLE_BLOCK)
    {
        static const unsigned char stop_tran = DT_STOP_TRAN;
        write_transfer(&stop_tran, 1);
        poll_busy(card->write_timeout);
    }

  error:

    deselect_card();

    return rc;
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

void ata_sleep(void)
{
}

void ata_spin(void)
{
}

static void mmc_thread(void)
{
    struct queue_event ev;
    bool idle_notified = false;

    while (1) {
        queue_wait_w_tmo(&mmc_queue, &ev, HZ);
        switch ( ev.id ) 
        {
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&mmc_queue);
                break;

#ifdef HAVE_HOTSWAP
            case SYS_HOTSWAP_INSERTED:
                disk_mount(1); /* mount MMC */
                queue_broadcast(SYS_FS_CHANGED, 0);
                break;

            case SYS_HOTSWAP_EXTRACTED:
                disk_unmount(1); /* release "by force" */
                queue_broadcast(SYS_FS_CHANGED, 0);
                break;
#endif
                
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

#ifdef HAVE_HOTSWAP
void mmc_enable_monitoring(bool on)
{
    mmc_monitor_enabled = on;
}
#endif

bool mmc_detect(void)
{
    return adc_read(ADC_MMC_SWITCH) < 0x200 ? true : false;
}

bool mmc_touched(void)
{
    if (mmc_status == MMC_UNKNOWN) /* try to detect */
    {
        mutex_lock(&mmc_mutex);
        setup_sci1(7);             /* safe value */
        and_b(~0x02, &PADRH);      /* assert CS */
        if (send_cmd(CMD_SEND_OP_COND, 0, NULL) == 0xFF)
            mmc_status = MMC_UNTOUCHED;
        else
            mmc_status = MMC_TOUCHED;

        deselect_card();
    }
    return mmc_status == MMC_TOUCHED;
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
#ifndef HAVE_HOTSWAP
    const bool mmc_monitor_enabled = true;
#endif

    if (new_mmc_circuit)
        /* USB bridge activity is 0 on idle, ~527 on active */
        current_status = adc_read(ADC_USB_ACTIVE) > 0x100;
    else
        current_status = adc_read(ADC_USB_ACTIVE) < 0x190;

    if (!current_status && usb_activity)
        last_usb_activity = current_tick;
    usb_activity = current_status;

    if (mmc_monitor_enabled)
    {
        current_status = mmc_detect();
        /* Only report when the status has changed */
        if (current_status != last_mmc_status)
        {
            last_mmc_status = current_status;
            countdown = HZ/3;
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
                    queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
                }
                else
                {
                    queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
                    mmc_status = MMC_UNTOUCHED;
                    card_info[1].initialized = false;
                }
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
    PBCR1 &= ~0x0CF0;      /* PB13, PB11 and PB10 become GPIO,
                            * if not modified below */
    if (on)
        PBCR1 |= 0x08A0;   /* as SCK1, TxD1, RxD1 */

    and_b(~0x80, &PADRL);  /* assert flash reset */
    sleep(HZ/100);
    or_b(0x80, &PADRL);    /* de-assert flash reset */
    sleep(HZ/100);
    card_info[0].initialized = false;
    card_info[1].initialized = false;
}

int ata_init(void)
{
    int rc = 0;

    if (!initialized) 
    {
        mutex_init(&mmc_mutex);
        queue_init(&mmc_queue, true);
    }
    mutex_lock(&mmc_mutex);
    led(false);

    last_mmc_status = mmc_detect();
#ifndef HAVE_MULTIVOLUME
    /* Use MMC if inserted, internal flash otherwise */
    current_card = last_mmc_status ? 1 : 0;
#endif

    if (!initialized)
    {
        if (!last_mmc_status)
            mmc_status = MMC_UNTOUCHED;

        /* Port setup */
        PACR1 &= ~0x0F3C;  /* GPIO function for PA13 (flash busy), PA12
                            * (clk gate), PA10 (flash CS), PA9 (MMC CS) */
        PACR2 &= ~0x4000;  /* GPIO for PA7 (flash reset) */
        PADR  |=  0x0680;  /* set all the selects + reset high (=inactive) */
        PAIOR |=  0x1680;  /* make outputs for them and the PA12 clock gate */

        PBCR1 &= ~0x0CF0;  /* GPIO function for PB13, PB11 and PB10 */
        PBDR  |=  0x2C00;  /* SCK1, TxD1 and RxD1 high in GPIO */
        PBIOR |=  0x2000;  /* SCK1 output */
        PBIOR &= ~0x0C00;  /* TxD1, RxD1 input */

        IPRE  &=  0x0FFF;  /* disable SCI1 interrupts for the CPU */

        new_mmc_circuit = ((HW_MASK & MMC_CLOCK_POLARITY) != 0);

        create_thread(mmc_thread, mmc_stack,
                      sizeof(mmc_stack), 0, mmc_thread_name
                      IF_PRIO(, PRIORITY_SYSTEM)
  		              IF_COP(, CPU));
        tick_add_task(mmc_tick);
        initialized = true;
    }
    ata_enable(true);

    mutex_unlock(&mmc_mutex);
    return rc;
}

