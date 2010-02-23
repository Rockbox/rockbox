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
 * Copyright © 2008-2009 Rafaël Carré
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

#include "config.h" /* for HAVE_MULTIVOLUME */
#include "fat.h"
#include "thread.h"
#include "hotswap.h"
#include "system.h"
#include "kernel.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as3525v2.h"
#include "pl081.h"  /* DMA controller */
#include "dma-target.h" /* DMA request lines */
#include "clock-target.h"
#include "panic.h"
#include "stdbool.h"
#include "ata_idle_notify.h"
#include "sd.h"

#include "lcd.h"
#include <stdarg.h>
#include "sysfont.h"

/* command flags */
#define MCI_NO_RESP     (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)

/* controller registers */
#define SD_BASE 0xC6070000

#define SD_REG(x)       (*(volatile unsigned long *) (SD_BASE+x))

#define MCI_CTRL        SD_REG(0x00)

/* control bits */
#define CTRL_RESET      (1<<0)
#define FIFO_RESET      (1<<1)
#define DMA_RESET       (1<<2)
#define INT_ENABLE      (1<<4)
#define DMA_ENABLE      (1<<5)
#define READ_WAIT       (1<<6)
#define SEND_IRQ_RESP   (1<<7)
#define ABRT_READ_DATA  (1<<8)
#define SEND_CCSD       (1<<9)
#define SEND_AS_CCSD    (1<<10)
#define EN_OD_PULLUP    (1<<24)


#define MCI_PWREN       SD_REG(0x04)    /* power enable */
#define MCI_CLKDIV      SD_REG(0x08)    /* clock divider */
#define MCI_CLKSRC      SD_REG(0x0C)    /* clock source */
#define MCI_CLKENA      SD_REG(0x10)    /* clock enable */
#define MCI_TMOUT       SD_REG(0x14)    /* timeout */

#define MCI_CTYPE       SD_REG(0x18)    /* card type */
                                        /* 1 bit per card, set = wide bus */

#define MCI_BLKSIZ      SD_REG(0x1C)    /* block size */
#define MCI_BYTCNT      SD_REG(0x20)    /* byte count */
#define MCI_MASK        SD_REG(0x24)    /* interrupt mask */

#define MCI_ARGUMENT    SD_REG(0x28)
#define MCI_COMMAND     SD_REG(0x2C)

/* command bits (bits 5:0 are the command index) */
#define CMD_RESP_EXP_BIT        (1<<6)
#define CMD_RESP_LENGTH_BIT     (1<<7)
#define CMD_CHECK_CRC_BIT       (1<<8)
#define CMD_DATA_EXP_BIT        (1<<9)
#define CMD_RW_BIT              (1<<10)
#define CMD_TRANSMODE_BIT       (1<<11)
#define CMD_SENT_AUTO_STOP_BIT  (1<<12)
#define CMD_WAIT_PRV_DAT_BIT    (1<<13)
#define CMD_ABRT_CMD_BIT        (1<<14)
#define CMD_SEND_INIT_BIT       (1<<15)
#define CMD_SEND_CLK_ONLY       (1<<21)
#define CMD_READ_CEATA          (1<<22)
#define CMD_CCS_EXPECTED        (1<<23)
#define CMD_DONE_BIT            (1<<31)


#define MCI_RESP0       SD_REG(0x30)
#define MCI_RESP1       SD_REG(0x34)
#define MCI_RESP2       SD_REG(0x38)
#define MCI_RESP3       SD_REG(0x3C)

#define MCI_MASK_STATUS SD_REG(0x40)    /* masked interrupt status */
#define MCI_RAW_STATUS  SD_REG(0x44)    /* raw interrupt status, also used as
                                         * status clear */
#define MCI_STATUS      SD_REG(0x48)

/*
 *  STATUS register
 *  & 0xBA80    = MCI_INT_DCRC | MCI_INT_DRTO | MCI_INT_FRUN | \
 *                  MCI_INT_HLE | MCI_INT_SBE | MCI_INT_EBE
 *  & 8         = MCI_INT_DTO
 *  & 0x428     = MCI_INT_DTO | MCI_INT_RXDR | MCI_INT_HTO
 *  & 0x418     = MCI_INT_DTO | MCI_INT_TXDR | MCI_INT_HTO
 */

/* interrupt bits */
#define MCI_INT_CRDDET  (1<<0)      /* card detect */
#define MCI_INT_RE      (1<<1)      /* response error */
#define MCI_INT_CD      (1<<2)      /* command done */
#define MCI_INT_DTO     (1<<3)      /* data transfer over */
#define MCI_INT_TXDR    (1<<4)      /* tx fifo data request */
#define MCI_INT_RXDR    (1<<5)      /* rx fifo data request */
#define MCI_INT_RCRC    (1<<6)      /* response crc error */
#define MCI_INT_DCRC    (1<<7)      /* data crc error */
#define MCI_INT_RTO     (1<<8)      /* response timeout */
#define MCI_INT_DRTO    (1<<9)      /* data read timeout */
#define MCI_INT_HTO     (1<<10)     /* data starv timeout */
#define MCI_INT_FRUN    (1<<11)     /* fifo over/underrun */
#define MCI_INT_HLE     (1<<12)     /* hw locked while error */
#define MCI_INT_SBE     (1<<13)     /* start bit error */
#define MCI_INT_ACD     (1<<14)     /* auto command done */
#define MCI_INT_EBE     (1<<15)     /* end bit error */
#define MCI_INT_SDIO    (0xf<<16)

#define MCI_ERROR   (MCI_INT_RE | MCI_INT_RCRC | MCI_INT_DCRC /*| MCI_INT_RTO*/ \
                   | MCI_INT_DRTO | MCI_INT_HTO | MCI_INT_FRUN | MCI_INT_HLE \
                   | MCI_INT_SBE | MCI_INT_EBE)

#define MCI_FIFOTH      SD_REG(0x4C)    /* FIFO threshold */
/* TX watermark :    bits 11:0
 * RX watermark :    bits 27:16
 * DMA MTRANS SIZE : bits 30:28
 * bits 31, 15:12 : unused
 */
#define MCI_FIFOTH_MASK 0x8000f000

#define MCI_CDETECT     SD_REG(0x50)    /* card detect */
#define MCI_WRTPRT      SD_REG(0x54)    /* write protect */
#define MCI_GPIO        SD_REG(0x58)
#define MCI_TCBCNT      SD_REG(0x5C)    /* transferred CIU byte count */
#define MCI_TBBCNT      SD_REG(0x60)    /* transferred host/DMA to/from bytes */
#define MCI_DEBNCE      SD_REG(0x64)    /* card detect debounce */
#define MCI_USRID       SD_REG(0x68)    /* user id */
#define MCI_VERID       SD_REG(0x6C)    /* version id */

#define MCI_HCON        SD_REG(0x70)    /* hardware config */
/* bit 0 : card type
 * bits 5:1 : maximum card index */

#define MCI_BMOD        SD_REG(0x80)    /* bus mode */
#define MCI_PLDMND      SD_REG(0x84)    /* poll demand */
#define MCI_DBADDR      SD_REG(0x88)    /* descriptor base address */
#define MCI_IDSTS       SD_REG(0x8C)    /* internal DMAC status */
#define MCI_IDINTEN     SD_REG(0x90)    /* internal DMAC interrupt enable */
#define MCI_DSCADDR     SD_REG(0x94)    /* current host descriptor address */
#define MCI_BUFADDR     SD_REG(0x98)    /* current host buffer address */

#define MCI_FIFO        ((unsigned long *) (SD_BASE+0x100))

#define UNALIGNED_NUM_SECTORS 10
static unsigned char aligned_buffer[UNALIGNED_NUM_SECTORS* SD_BLOCK_SIZE] __attribute__((aligned(32)));   /* align on cache line size */
static unsigned char *uncached_buffer = UNCACHED_ADDR(&aligned_buffer[0]);

static int sd_init_card(void);
static void init_controller(void);

static tCardInfo card_info;

/* for compatibility */
static long last_disk_activity = -1;

#define MIN_YIELD_PERIOD 5  /* ticks */
static long next_yield = 0;

static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx SHAREDBSS_ATTR;
static struct event_queue sd_queue;
#ifndef BOOTLOADER
bool sd_enabled = false;
#endif

static struct wakeup transfer_completion_signal;
static volatile bool retry;
static volatile bool data_transfer = false;

static inline void mci_delay(void) { int i = 0xffff; while(i--) ; }

void INT_NAND(void)
{
    MCI_CTRL &= ~INT_ENABLE;
    const int status = MCI_MASK_STATUS;

    MCI_RAW_STATUS = status;    /* clear status */

    if(status & MCI_ERROR)
        retry = true;

    if(data_transfer && status & (MCI_INT_DTO|MCI_ERROR))
        wakeup_signal(&transfer_completion_signal);

    MCI_CTRL |= INT_ENABLE;
}

static bool send_cmd(const int cmd, const int arg, const int flags,
        unsigned long *response)
{
    MCI_COMMAND = cmd;

    if(flags & MCI_RESP)
    {
        MCI_COMMAND |= CMD_RESP_EXP_BIT;
        if(flags & MCI_LONG_RESP)
            MCI_COMMAND |= CMD_RESP_LENGTH_BIT;
    }

    if(cmd == SD_READ_MULTIPLE_BLOCK || cmd == SD_WRITE_MULTIPLE_BLOCK)
    {
        MCI_COMMAND |= CMD_WAIT_PRV_DAT_BIT | CMD_DATA_EXP_BIT;
        if(cmd == SD_WRITE_MULTIPLE_BLOCK)
            MCI_COMMAND |= CMD_RW_BIT | CMD_CHECK_CRC_BIT;
    }

    int clkena = MCI_CLKENA;
    MCI_CLKENA = 0;

    MCI_ARGUMENT = arg;
    MCI_COMMAND |= CMD_DONE_BIT;

    int max = 0x40000;
    while(MCI_COMMAND & CMD_DONE_BIT)
    {
        if(--max == 0)  /* timeout */
        {
            MCI_CLKENA = clkena;
            return false;
        }
    }

    MCI_CLKENA = clkena;

    if(flags & MCI_RESP)
    {
        if(flags & MCI_LONG_RESP)
        {
            /* store the response in little endian order for the words */
            response[0] = MCI_RESP3;
            response[1] = MCI_RESP2;
            response[2] = MCI_RESP1;
            response[3] = MCI_RESP0;
        }
        else
            response[0] = MCI_RESP0;
    }
    return true;
}

static int sd_init_card(void)
{
    unsigned long response;
    unsigned long temp_reg[4];
    int max_tries = 100; /* max acmd41 attemps */
    bool sd_v2;
    int i;

    if(!send_cmd(SD_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;

    mci_delay();

    sd_v2 = false;
    if(send_cmd(SD_SEND_IF_COND, 0x1AA, MCI_RESP, &response))
        if((response & 0xFFF) == 0x1AA)
            sd_v2 = true;

    do {
        /* some MicroSD cards seems to need more delays, so play safe */
        mci_delay();
        mci_delay();
        mci_delay();

        /* app_cmd */
        if( !send_cmd(SD_APP_CMD, 0, MCI_RESP, &response) ||
            !(response & (1<<5)))
        {
            return -2;
        }

        /* acmd41 */
        if(!send_cmd(SD_APP_OP_COND, (sd_v2 ? 0x40FF8000 : (1<<23)),
                        MCI_RESP, &card_info.ocr))
            return -3;
    } while(!(card_info.ocr & (1<<31)) && max_tries--);

    if(max_tries < 0)
        return -4;

    mci_delay();
    mci_delay();
    mci_delay();

    /* send CID */
    if(!send_cmd(SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP, card_info.cid))
        return -5;

    /* send RCA */
    if(!send_cmd(SD_SEND_RELATIVE_ADDR, 0, MCI_RESP, &card_info.rca))
        return -6;

    /* send CSD */
    if(!send_cmd(SD_SEND_CSD, card_info.rca,
                 MCI_RESP|MCI_LONG_RESP, temp_reg))
        return -7;

    for(i=0; i<4; i++)
        card_info.csd[3-i] = temp_reg[i];

    sd_parse_csd(&card_info);

    if(!send_cmd(SD_APP_CMD, 0, MCI_RESP, &response) ||
       !send_cmd(42, 0, MCI_NO_RESP, NULL)) /* disconnect the 50 KOhm pull-up
                                               resistor on CD/DAT3 */
        return -13;

    if(!send_cmd(SD_APP_CMD, card_info.rca, MCI_NO_RESP, NULL))
        return -10;

    if(!send_cmd(SD_SET_BUS_WIDTH, card_info.rca | 2, MCI_NO_RESP, NULL))
        return -11;

    if(!send_cmd(SD_SELECT_CARD, card_info.rca, MCI_NO_RESP, NULL))
        return -9;

    /* not sent in init_card() by OF */
    if(!send_cmd(SD_SET_BLOCKLEN, card_info.blocksize, MCI_NO_RESP,
                 NULL))
        return -12;

    card_info.initialized = 1;

    return 0;
}

static void sd_thread(void) __attribute__((noreturn));
static void sd_thread(void)
{
    struct queue_event ev;
    bool idle_notified = false;

    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch ( ev.id )
        {
        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else
            {
                /* never let a timer wrap confuse us */
                next_yield = current_tick;

                if (!idle_notified)
                {
                    call_storage_idle_notifys(false);
                    idle_notified = true;
                }
            }
            break;
#if 0
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);

            break;
        case SYS_USB_DISCONNECTED:
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
            break;
#endif
        }
    }
}

static void init_controller(void)
{
    int idx = (MCI_HCON >> 1) & 31;
    int idx_bits = (1 << idx) -1;

    MCI_PWREN &= ~idx_bits;
    MCI_PWREN = idx_bits;

    mci_delay();

    MCI_CLKSRC = 0;
    MCI_CLKDIV = 0;

    MCI_CTRL |= CTRL_RESET;
    while(MCI_CTRL & CTRL_RESET)
        ;

    MCI_RAW_STATUS = 0xffffffff;

    MCI_TMOUT = 0xffffffff;

    MCI_CTYPE = 0;

    MCI_CLKENA = idx_bits;

    MCI_ARGUMENT = 0;
    MCI_COMMAND = CMD_DONE_BIT|CMD_SEND_CLK_ONLY|CMD_WAIT_PRV_DAT_BIT;
    while(MCI_COMMAND & CMD_DONE_BIT)
        ;

    MCI_DEBNCE = 0xfffff;   /* default value */

    MCI_FIFOTH &= MCI_FIFOTH_MASK;
    MCI_FIFOTH |= 0x503f0080;

    MCI_MASK = 0xffffffff & ~(MCI_INT_ACD|MCI_INT_CRDDET);

    MCI_CTRL |= INT_ENABLE;
}

int sd_init(void)
{
    int ret;
    CGU_PERI |= CGU_MCI_CLOCK_ENABLE;

    CGU_IDE =   (1<<7)  /* AHB interface enable */  |
                (1<<6)  /* interface enable */      |
                ((CLK_DIV(AS3525_PLLA_FREQ, AS3525_IDE_FREQ) - 1) << 2) |
                1;       /* clock source = PLLA */

    CGU_MEMSTICK = (1<<8) | (1<<7) |
        (CLK_DIV(AS3525_PLLA_FREQ, AS3525_MS_FREQ) -1) | 1;

    /* ?? */
    *(volatile int*)(CGU_BASE+0x3C) = (1<<7) |
        (CLK_DIV(AS3525_PLLA_FREQ, 24000000) -1) | 1;

    wakeup_init(&transfer_completion_signal);

    VIC_INT_ENABLE |= INTERRUPT_NAND;

    init_controller();
    ret = sd_init_card();
    if(ret < 0)
        return ret;

    /* init mutex */
    mutex_init(&sd_mtx);

    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

#ifndef BOOTLOADER
    sd_enabled = true;
    sd_enable(false);
#endif
    return 0;
}

#ifdef STORAGE_GET_INFO
void sd_get_info(struct storage_info *info)
{
    info->sector_size=card_info.blocksize;
    info->num_sectors=card_info.numblocks;
    info->vendor="Rockbox";
    info->product = "Internal Storage";
    info->revision="0.00";
}
#endif

static int sd_wait_for_state(unsigned int state)
{
    unsigned long response;
    unsigned int timeout = 100; /* ticks */
    long t = current_tick;

    while (1)
    {
        long tick;

        if(!send_cmd(SD_SEND_STATUS, card_info.rca,
                    MCI_RESP, &response))
            return -1;

        if (((response >> 9) & 0xf) == state)
            return 0;

        if(TIME_AFTER(current_tick, t + timeout))
            return -10 * ((response >> 9) & 0xf);

        if (TIME_AFTER((tick = current_tick), next_yield))
        {
            yield();
            timeout += current_tick - tick;
            next_yield = tick + MIN_YIELD_PERIOD;
        }
    }
}

static int sd_transfer_sectors(unsigned long start, int count, void* buf, bool write)
{
    int ret = 0;

    /* skip SanDisk OF */
    start += 0xf000;

    mutex_lock(&sd_mtx);
#ifndef BOOTLOADER
    sd_enable(true);
#endif

    if (card_info.initialized <= 0)
    {
        ret = sd_init_card();
        if (!(card_info.initialized))
        {
            panicf("card not initialised (%d)", ret);
            goto sd_transfer_error;
        }
    }

    last_disk_activity = current_tick;
    ret = sd_wait_for_state(SD_TRAN);
    if (ret < 0)
    {
        static const char *st[9] = {
            "IDLE", "RDY", "IDENT", "STBY", "TRAN", "DATA", "RCV", "PRG", "DIS"
        };
        if(ret <= -10)
            panicf("wait for state failed (%s)", st[(-ret / 10) % 9]);
        else
            panicf("wait for state failed");
        goto sd_transfer_error;
    }

    dma_retain();

    const int cmd = write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;

    /* Interrupt handler might set this to true during transfer */
    do
    {
        void *dma_buf = aligned_buffer;
        unsigned int transfer = count;
        if(transfer > UNALIGNED_NUM_SECTORS)
            transfer = UNALIGNED_NUM_SECTORS;

        if(write)
            memcpy(uncached_buffer, buf, transfer * SD_BLOCK_SIZE);

        retry = false;

        MCI_BLKSIZ = SD_BLOCK_SIZE;
        MCI_BYTCNT = transfer * SD_BLOCK_SIZE;

        MCI_CTRL |= (FIFO_RESET|DMA_RESET);
        while(MCI_CTRL & (FIFO_RESET|DMA_RESET))
            ;

        MCI_CTRL |= DMA_ENABLE;
        MCI_MASK = MCI_INT_CD|MCI_INT_DTO|MCI_INT_DCRC|MCI_INT_DRTO| \
            MCI_INT_HTO|MCI_INT_FRUN|MCI_INT_HLE|MCI_INT_SBE|MCI_INT_EBE;

        MCI_FIFOTH &= MCI_FIFOTH_MASK;
        MCI_FIFOTH |= 0x503f0080;


        if(card_info.ocr & (1<<30) ) /* SDHC */
            ret = send_cmd(cmd, start, MCI_NO_RESP, NULL);
        else
            ret = send_cmd(cmd, start * SD_BLOCK_SIZE,
                    MCI_NO_RESP, NULL);

        if (ret < 0)
            panicf("transfer multiple blocks failed (%d)", ret);

        if(write)
            dma_enable_channel(0, dma_buf, MCI_FIFO, DMA_PERI_SD,
                DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8, NULL);
        else
            dma_enable_channel(0, MCI_FIFO, dma_buf, DMA_PERI_SD,
                DMAC_FLOWCTRL_PERI_PERI_TO_MEM, false, true, 0, DMA_S8, NULL);

        data_transfer = true;
        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
        data_transfer = false;

        last_disk_activity = current_tick;

        if(!send_cmd(SD_STOP_TRANSMISSION, 0, MCI_NO_RESP, NULL))
        {
            ret = -666;
            panicf("STOP TRANSMISSION failed");
            goto sd_transfer_error;
        }

        ret = sd_wait_for_state(SD_TRAN);
        if (ret < 0)
        {
            panicf(" wait for state TRAN failed (%d)", ret);
            goto sd_transfer_error;
        }

        if(!retry)
        {
            if(!write)
                memcpy(buf, uncached_buffer, transfer * SD_BLOCK_SIZE);
            buf += transfer * SD_BLOCK_SIZE;
            start += transfer;
            count -= transfer;
        }
    } while(retry || count);

    dma_release();

#ifndef BOOTLOADER
    sd_enable(false);
#endif
    mutex_unlock(&sd_mtx);
    return 0;

sd_transfer_error:
    panicf("transfer error : %d",ret);
    card_info.initialized = 0;
    return ret;
}

int sd_read_sectors(unsigned long start, int count, void* buf)
{
    return sd_transfer_sectors(start, count, buf, false);
}

int sd_write_sectors(unsigned long start, int count, const void* buf)
{
#if 1 /* disabled until stable*/ \
    || defined(BOOTLOADER) /* we don't need write support in bootloader */
    (void) start;
    (void) count;
    (void) buf;
    return -1;
#else
    return sd_transfer_sectors(start, count, (void*)buf, true);
#endif
}

#ifndef BOOTLOADER
long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

void sd_enable(bool on)
{
    /* TODO */
    (void)on;
    return;
}

tCardInfo *card_get_info_target(int card_no)
{
    (void)card_no;
    return &card_info;
}

#endif /* BOOTLOADER */
