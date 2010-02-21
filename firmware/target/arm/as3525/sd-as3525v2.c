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

static int line = 0;
static void printf(const char *format, ...)
{
    char buf[50];
    int len;
    va_list ap;
    va_start(ap, format);

    len = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    lcd_puts(0, line++, buf);
    lcd_update();
    if(line >= LCD_HEIGHT/SYSFONT_HEIGHT)
        line = 0;
}

/* command flags */
#define MCI_NO_RESP    (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)

/* controller registers */
#define SD_BASE 0xC6070000

/*
 *  STATUS register
 *  & 0xBA80
 *  & 8
 *  & 0x428
 *  & 0x418
 */

/*
 *  INFO on CMD register
 *
 * if(cmd >= 200) cmd -= 200; (>= 200 = acmd?)
 *
 *          COMMANDS        (| (x<<16)        BITS                          RESPONSE
 *
 * 1    ? reserved                  & ~0x80, | 0x40, | 0x8000               ?
 * 5    ? reserved for I/O cards    & ~0x80, | 0x40                         ?
 * 11   ? reserved                  & ~0x80, | 0x40, | 0x2200, | 0x800      ?
 * 14   ? reserved                  & ~0x80, | 0x40, | 0x2200, ~0x1000      ?
 * 19   ? reserved                  & ~0x80, |0x40, | 0x2700, & ~0x1000     ?
 * 20   ? reserved                  & ~0x80, |0x40, | 0x2700, | 0x800       ?
 * 23   ? reserved                  & ~0x80, | 0x40                         ?
 * 39   ? reserved                  & ~0x80, | 0x40                         ?
 * 51   ? reserved                  & ~0x80, | 0x40, | 0x2000, | 0x200      ?
 * 52   ? reserved for I/O          & ~0x80, | 0x40                         ?
 * 53   ? reserved for I/O          & ~0x80, | 0x40, | 0x2200, & ~0x1000    ?
 * 253  ?                           & ~0x80, |0x40, | 0x2700, & ~0x1000     ?
 *
 * 0    GO IDLE STATE               & ~0x4000, & ~0xC0, | 0x4000            no
 * 2    ALL SEND CID                & ~0x4000, |0xC0                        r2
 * 3    SEND RCA                    & ~0x80, | 0x40                         r6
 * 6    SWITCH_FUNC                 & ~0x80, | 0x40                         r1
 * 7    SELECT CARD                 & ~0x80, | 0x40                         r1b
 * 8    SEND IF COND                & ~0x80, | 0x40, | 0x2200, & ~0x1000    r7
 * 9    SEND CSD                    & ~0x4000, | 0xc0                       r2
 * 12   STOP TRANSMISSION           & ~0x80, | 0x40, | 0x4000               r1b
 * 13   SEND STATUS                 & ~0x80, | 0x40                         r1
 * 15   GO INACTIVE STATE           & ~0x4000, & ~0xC0                      no
 * 16   SET BLOCKLEN                & ~0x80, | 0x40                         r1
 * 17   READ SINGLE BLOCK           & ~0x80, | 0x40, | 0x2200               r1
 * 18   READ MULTIPLE BLOCK         & ~0x80, | 0x40, | 0x2200               r1
 * 24   WRITE BLOCK                 & ~0x80, |0x40, | 0x2700                r1
 * 25   WRITE MULTIPLE BLOCK        & ~0x80, |0x40, | 0x2700                r1
 * 41   SEND APP OP COND            & ~0x80, | 0x40                         r3
 * 42   LOCK UNLOCK                 & ~0x80, |0x40, | 0x2700                r1
 * 55   APP CMD                     & ~0x80, | 0x40                         r1
 * 206  SET BUS WIDTH               & ~0x80, | 0x40, | 0x2000               r1
 * 207  SELECT CARD ?               & ~0x4000, & ~0xC0                      r1b
 *
 *
 * bits 5:0 = cmd
 * bit 6 (0x40) = response
 * bit 7 (0x80) = long response
 *      => like pl180 <=
 *                              BIT SET IN COMANDS:
 *
 * bit 8  (0x100) ?     write block, write multi_block, lock/unlock
 * bit 9  (0x200) ?     send if cond, read block, read multi_block, write block, write multi_block, lock/unlock
 * bit 10 (0x400) ?     write block, write multi_block, lock/unlock
 * bit 11 (0x800) ?
 * bit 12 (0x1000) ?
 * bit 13 (0x2000) ?    send if cond, read block, read multi_block, write block, write multi_block, lock/unlock, set bus width
 * bit 14 (0x4000) ?    go idle state, stop transmission
 * bit 15 (0x8000) ?
 *
 */

#define SD_REG(x)       (*(volatile unsigned long *) (SD_BASE+x))

#define MCI_CTRL        SD_REG(0x00)

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
#define MCI_BLKSIZ      SD_REG(0x1C)    /* block size */
#define MCI_BYTCNT      SD_REG(0x20)    /* byte count */
#define MCI_MASK        SD_REG(0x24)    /* interrupt mask */

#define MCI_ARGUMENT    SD_REG(0x28)
#define MCI_COMMAND     SD_REG(0x2C)

#define MCI_RESP0       SD_REG(0x30)
#define MCI_RESP1       SD_REG(0x34)
#define MCI_RESP2       SD_REG(0x38)
#define MCI_RESP3       SD_REG(0x3C)

#define MCI_MASK_STATUS SD_REG(0x40)    /* masked interrupt status */
#define MCI_RAW_STATUS  SD_REG(0x44)    /* raw interrupt status, also used as
                                         * status clear */
#define MCI_STATUS      SD_REG(0x48)

#define MCI_FIFOTH      SD_REG(0x4C)    /* FIFO threshold */
#define MCI_CDETECT     SD_REG(0x50)    /* card detect */
#define MCI_WRTPRT      SD_REG(0x54)    /* write protect */
#define MCI_GPIO        SD_REG(0x58)
#define MCI_TCBCNT      SD_REG(0x5C)    /* transferred CIU byte count */
#define MCI_TBBCNT      SD_REG(0x60)    /* transferred host/DMA to/from bytes */
#define MCI_DEBNCE      SD_REG(0x64)    /* card detect debounce */
#define MCI_USRID       SD_REG(0x68)    /* user id */
#define MCI_VERID       SD_REG(0x6C)    /* version id */
#define MCI_HCON        SD_REG(0x70)    /* hardware config */

#define MCI_BMOD        SD_REG(0x80)    /* bus mode */
#define MCI_PLDMND      SD_REG(0x84)    /* poll demand */
#define MCI_DBADDR      SD_REG(0x88)    /* descriptor base address */
#define MCI_IDSTS       SD_REG(0x8C)    /* internal DMAC status */
#define MCI_IDINTEN     SD_REG(0x90)    /* internal DMAC interrupt enable */
#define MCI_DSCADDR     SD_REG(0x94)    /* current host descriptor address */
#define MCI_BUFADDR     SD_REG(0x98)    /* current host buffer address */

#define MCI_ERROR 0     /* FIXME */

#define MCI_FIFO        ((unsigned long *) (SD_BASE+0x100))

#define MCI_COMMAND_ENABLE          (1<<31)
#define MCI_COMMAND_ACTIVE          MCI_COMMAND_ENABLE
#define MCI_COMMAND_RESPONSE        (1<<6)
#define MCI_COMMAND_LONG_RESPONSE   (1<<7)



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
static bool sd_enabled = false;
#endif

static struct wakeup transfer_completion_signal;
static volatile bool retry;

static inline void mci_delay(void) { int i = 0xffff; while(i--) ; }

void INT_NAND(void)
{
    MCI_CTRL &= INT_ENABLE;
    const int status = MCI_STATUS;

#if 0
    if(status & MCI_ERROR)
        retry = true;
#endif

//    wakeup_signal(&transfer_completion_signal);
    MCI_RAW_STATUS = status;

    //static int x = 0;
    switch(status)
    {
        case 0x4:       /* cmd received ? */
        case 0x104:     /* ? 1 time in init (10th interrupt) */
        case 0x2000:    /* ? after cmd read_mul_blocks | 0x2200 */

        case 0x820:     /* ? 1 time while copy from FIFO (not DMA) */
        case 0x20:      /* ? rx fifo empty */
            break;
#if 0
        default:
            printf("%2d NAND 0x%x", ++x, status);
            int delay = 0x100000; while(delay--) ;
#endif
    }
    /*
     * 0x48 = some kind of status
     *  0x106
     *  0x4106
     *  1B906
     *  1F906
     *  1B906
     *  1F906
     *  1F906
     *  1906
     *  ...
     *  6906
     *  6D06 (dma)
     *
     *  read resp (6, 7, 12, 42) : while bit 9 is unset ;
     *
     */
    //printf("%x %x", status, MCI_STATUS);
    //while(!button_read_device());
    //while(button_read_device());

    MCI_CTRL |= INT_ENABLE;
}

static bool send_cmd(const int cmd, const int arg, const int flags,
        unsigned long *response)
{
    int val;
    val = cmd | MCI_COMMAND_ENABLE;
    if(flags & MCI_RESP)
    {
        val |= MCI_COMMAND_RESPONSE;
        if(flags & MCI_LONG_RESP)
            val |= MCI_COMMAND_LONG_RESPONSE;
    }

    if(cmd == 18)       /* r */
        val |= 0x2200;
    else if(cmd == 25)  /* w */
        val |= 0x2700;

    int tmp = MCI_CLKENA;
    MCI_CLKENA = 0;

    MCI_COMMAND = 0x80202000;
    MCI_ARGUMENT = 0;
    int max = 10;
    while(max-- && MCI_COMMAND & MCI_COMMAND_ACTIVE);

    MCI_CLKDIV &= ~0xff;
    MCI_CLKDIV |= 0;

    MCI_COMMAND = 0x80202000;
    MCI_ARGUMENT = 0;
    max = 10;
    while(max-- && MCI_COMMAND & MCI_COMMAND_ACTIVE);

    MCI_CLKENA = tmp;

    MCI_COMMAND = 0x80202000;
    MCI_ARGUMENT = 0;
    max = 10;
    while(max-- && MCI_COMMAND & MCI_COMMAND_ACTIVE);

    mci_delay();

    MCI_ARGUMENT = arg;
    MCI_COMMAND = val;

    MCI_CTRL |= INT_ENABLE;

    max = 1000;
    while(max-- && MCI_COMMAND & MCI_COMMAND_ACTIVE); /* wait for cmd completion */
    if(!max)
        return false;

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
    bool sdhc;
    int i;

    if(!send_cmd(SD_GO_IDLE_STATE, 0, MCI_NO_RESP, NULL))
        return -1;

    mci_delay();

    sdhc = false;
    if(send_cmd(SD_SEND_IF_COND, 0x1AA, MCI_RESP, &response))
        if((response & 0xFFF) == 0x1AA)
            sdhc = true;

    do {
        /* some MicroSD cards seems to need more delays, so play safe */
        mci_delay();
        mci_delay();
        mci_delay();

        /* app_cmd */
        if( !send_cmd(SD_APP_CMD, 0, MCI_RESP, &response) /*||
            !(response & (1<<5))*/ )
        {
            return -2;
        }

        /* acmd41 */
        if(!send_cmd(SD_APP_OP_COND, (sdhc ? 0x40FF8000 : (1<<23)),
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

    MCI_CTYPE &= ~(0x10001);
    MCI_CTYPE |= 0x1;

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
    int tmp = MCI_HCON;
    int shift = 1 + ((tmp << 26) >> 27);

    MCI_PWREN &= ~((1 << shift) -1);
    MCI_PWREN = (1 << shift) -1;

    mci_delay();

    MCI_CTRL |= CTRL_RESET;     /* FIXME: FIFO & DMA reset? */
    while(MCI_CTRL & CTRL_RESET)
        ;

    MCI_RAW_STATUS = 0xffffffff;
    MCI_MASK = 0xffffbffe;

    MCI_CTRL |= INT_ENABLE;
    MCI_TMOUT = 0xffffffff;

    MCI_CLKENA = (1<<shift) - 1;

    MCI_ARGUMENT = 0;
    MCI_COMMAND = 0x80202000;
    int max = 10;
    while(max-- && (MCI_COMMAND & (1<<31))) ;

    MCI_DEBNCE = 0xfffff;

    MCI_FIFOTH = ~0x7fff0fff;
    MCI_FIFOTH |= 0x503f0080;

    MCI_MASK = 0xffffbffe;
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
#if 1
    /* This is debug code, not functional yet */
    line = 0;
    lcd_clear_display();
    printf("Entering SD transfer");
    printf("THIS IS DEBUG CODE !");
    printf("");
    printf("All your controllers");
    printf("are belong to us.");
    volatile int delay = 0x500000;
    while(delay--) ;
    line = 0;
    lcd_clear_display();
#endif /* debug warning */

    int ret = 0;

    if((int)buf & 3)
        panicf("unaligned transfer");

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

    while(count)
    {
        /* Interrupt handler might set this to true during transfer */
        retry = false;
        /* 128 * 512 = 2^16, and doesn't fit in the 16 bits of DATA_LENGTH
         * register, so we have to transfer maximum 127 sectors at a time. */
        //unsigned int transfer = (count >= 128) ? 127 : count; /* sectors */
        unsigned int transfer = count;

        const int cmd =
            write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;


        MCI_CTRL |= FIFO_RESET;
        while(MCI_CTRL & FIFO_RESET)
            ;

        //MCI_BLKSIZ = 512;
        MCI_BYTCNT = transfer * 512;

        MCI_CTRL |= FIFO_RESET;
        while(MCI_CTRL & FIFO_RESET)
            ;

        MCI_FIFOTH &= ~0x7fff0fff;

        MCI_CTRL |= DMA_ENABLE;
        MCI_MASK = 0xBE8C;
        MCI_FIFOTH |= 0x503f0080;


        if(card_info.ocr & (1<<30) ) /* SDHC */
            ret = send_cmd(cmd, start, MCI_NO_RESP, NULL);
        else
            ret = send_cmd(cmd, start * SD_BLOCK_SIZE,
                    MCI_NO_RESP, NULL);

        if (ret < 0)
            panicf("transfer multiple blocks failed (%d)", ret);

        if(write)
            dma_enable_channel(0, buf, MCI_FIFO, DMA_PERI_SD,
                DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8, NULL);
        else
            dma_enable_channel(0, MCI_FIFO, buf, DMA_PERI_SD,
                DMAC_FLOWCTRL_PERI_PERI_TO_MEM, false, true, 0, DMA_S8, NULL);

        line = 0;
        lcd_clear_display();
        printf("dma ->");

        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

        printf("dma <-");
        int delay = 0x1000000; while(delay--) ;

        if(!retry)
        {
            buf += transfer * SECTOR_SIZE;
            start += transfer;
            count -= transfer;
        }

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
    }

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
#if defined(BOOTLOADER) /* we don't need write support in bootloader */
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
