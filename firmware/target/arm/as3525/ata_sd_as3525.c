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
 * Copyright © 2008 Rafaël Carré
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

/* Driver for the ARM PL180 SD/MMC controller inside AS3525 SoC */

#include "config.h" /* for HAVE_MULTIVOLUME */
#include "fat.h"
#include "thread.h"
#include "hotswap.h"
#include "system.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as3525.h"
#include "pl180.h"  /* SD controller */
#include "pl081.h"  /* DMA controller */
#include "dma-target.h" /* DMA request lines */
#include "clock-target.h"
#include "panic.h"
#include "stdbool.h"
#include "ata_idle_notify.h"
#include "sd.h"

#ifdef HAVE_HOTSWAP
#include "disk.h"
#endif

/* command flags */
#define MCI_NO_FLAGS    (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ARG         (1<<2)

/* ARM PL180 registers */
#define MCI_POWER(i)       (*(volatile unsigned char *) (pl180_base[i]+0x00))
#define MCI_CLOCK(i)       (*(volatile unsigned long *) (pl180_base[i]+0x04))
#define MCI_ARGUMENT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x08))
#define MCI_COMMAND(i)     (*(volatile unsigned long *) (pl180_base[i]+0x0C))
#define MCI_RESPCMD(i)     (*(volatile unsigned long *) (pl180_base[i]+0x10))
#define MCI_RESP0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x14))
#define MCI_RESP1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x18))
#define MCI_RESP2(i)       (*(volatile unsigned long *) (pl180_base[i]+0x1C))
#define MCI_RESP3(i)       (*(volatile unsigned long *) (pl180_base[i]+0x20))
#define MCI_DATA_TIMER(i)  (*(volatile unsigned long *) (pl180_base[i]+0x24))
#define MCI_DATA_LENGTH(i) (*(volatile unsigned short*) (pl180_base[i]+0x28))
#define MCI_DATA_CTRL(i)   (*(volatile unsigned char *) (pl180_base[i]+0x2C))
#define MCI_DATA_CNT(i)    (*(volatile unsigned short*) (pl180_base[i]+0x30))
#define MCI_STATUS(i)      (*(volatile unsigned long *) (pl180_base[i]+0x34))
#define MCI_CLEAR(i)       (*(volatile unsigned long *) (pl180_base[i]+0x38))
#define MCI_MASK0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x3C))
#define MCI_MASK1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x40))
#define MCI_SELECT(i)      (*(volatile unsigned long *) (pl180_base[i]+0x44))
#define MCI_FIFO_CNT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x48))

#define MCI_FIFO(i)        ((unsigned long *) (pl180_base[i]+0x80))
/* volumes */
#define     INTERNAL_AS3525 0   /* embedded SD card */
#define     SD_SLOT_AS3525   1   /* SD slot if present */

static const int pl180_base[NUM_VOLUMES] = {
            NAND_FLASH_BASE
#ifdef HAVE_MULTIVOLUME
            , SD_MCI_BASE
#endif
};

/* TODO : BLOCK_SIZE != SECTOR_SIZE ? */
#define BLOCK_SIZE      512
#define SECTOR_SIZE     512

static tSDCardInfo card_info[NUM_VOLUMES];

/* for compatibility */
static long last_disk_activity = -1;

#define MIN_YIELD_PERIOD 5  /* ticks */
static long next_yield = 0;

static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx SHAREDBSS_ATTR;
static struct event_queue sd_queue;

static inline void mci_delay(void) { int i = 0xffff; while(i--) ; }

static void mci_set_clock_divider(const int drive, int divider)
{
    int clock = MCI_CLOCK(drive);

    if(divider > 1)
    {
        /* use divide logic */
        clock &= ~MCI_CLOCK_BYPASS;

        /* convert divider to MCI_CLOCK logic */
        divider = (divider/2) - 1;
        if(divider >= 256)
            divider = 255;
    }
    else
    {
        /* bypass dividing logic */
        clock |= MCI_CLOCK_BYPASS;
        divider = 0;
    }

    MCI_CLOCK(drive) = clock | divider;

    mci_delay();
}

static bool send_cmd(const int drive, const int cmd, const int arg,
                     const int flags, int *response)
{
    int val, status;

    while(MCI_STATUS(drive) & MCI_CMD_ACTIVE);

    if(MCI_COMMAND(drive) & MCI_COMMAND_ENABLE) /* clears existing command */
    {
        MCI_COMMAND(drive) = 0;
        mci_delay();
    }

    val = cmd | MCI_COMMAND_ENABLE;
    if(flags & MCI_RESP)
    {
        val |= MCI_COMMAND_RESPONSE;
        if(flags & MCI_LONG_RESP)
            val |= MCI_COMMAND_LONG_RESPONSE;
    }

    MCI_CLEAR(drive) = 0x7ff;

    MCI_ARGUMENT(drive) = (flags & MCI_ARG) ? arg : 0;
    MCI_COMMAND(drive) = val;

    while(MCI_STATUS(drive) & MCI_CMD_ACTIVE);  /* wait for cmd completion */

    MCI_COMMAND(drive) = 0;
    MCI_ARGUMENT(drive) = ~0;

    status = MCI_STATUS(drive);
    MCI_CLEAR(drive) = 0x7ff;

    if(flags & MCI_RESP)
    {
        if(status & MCI_CMD_TIMEOUT)
            return false;
        else if(status & (MCI_CMD_CRC_FAIL /* FIXME? */ | MCI_CMD_RESP_END))
        {   /* resp received */
            if(flags & MCI_LONG_RESP)
            {
                /* store the response in little endian order for the words */
                response[0] = MCI_RESP3(drive);
                response[1] = MCI_RESP2(drive);
                response[2] = MCI_RESP1(drive);
                response[3] = MCI_RESP0(drive);
            }
            else
                response[0] = MCI_RESP0(drive);
            return true;
        }
    }
    else if(status & MCI_CMD_SENT)
        return true;

    return false;
}

static int sd_init_card(const int drive)
{
    unsigned int  c_size;
    unsigned long c_mult;
    int response;
    int max_tries = 100; /* max acmd41 attemps */
    bool sdhc;

    if(!send_cmd(drive, SD_GO_IDLE_STATE, 0, MCI_NO_FLAGS, NULL))
        return -1;

    mci_delay();

    sdhc = false;
    if(send_cmd(drive, SD_SEND_IF_COND, 0x1AA, MCI_RESP|MCI_ARG, &response))
        if((response & 0xFFF) == 0x1AA)
            sdhc = true;

    do {
        mci_delay();

        /* app_cmd */
        if( !send_cmd(drive, SD_APP_CMD, 0, MCI_RESP|MCI_ARG, &response) ||
            !(response & (1<<5)) )
        {
            return -2;
        }

        /* acmd41 */
        if(!send_cmd(drive, SD_APP_OP_COND, (sdhc ? 0x40FF8000 : (1<<23)),
                        MCI_RESP|MCI_ARG, &card_info[drive].ocr))
            return -3;

    } while(!(card_info[drive].ocr & (1<<31)) && max_tries--);

    if(!max_tries)
        return -4;

    /* send CID */
    if(!send_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP|MCI_ARG,
                            card_info[drive].cid))
        return -5;

    /* send RCA */
    if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP|MCI_ARG,
                &card_info[drive].rca))
        return -6;

    /* send CSD */
    if(!send_cmd(drive, SD_SEND_CSD, card_info[drive].rca,
                 MCI_RESP|MCI_LONG_RESP|MCI_ARG, card_info[drive].csd))
        return -7;

    /* These calculations come from the Sandisk SD card product manual */
    if( (card_info[drive].csd[3]>>30) == 0)
    {
        /* CSD version 1.0 */
        c_size = ((card_info[drive].csd[2] & 0x3ff) << 2) + (card_info[drive].csd[1]>>30) + 1;
        c_mult = 4 << ((card_info[drive].csd[1] >> 15) & 7);
        card_info[drive].max_read_bl_len = 1 << ((card_info[drive].csd[2] >> 16) & 15);
        card_info[drive].block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        card_info[drive].numblocks = c_size * c_mult * (card_info[drive].max_read_bl_len/512);
        card_info[drive].capacity = card_info[drive].numblocks * card_info[drive].block_size;
    }
#ifdef HAVE_MULTIVOLUME
    else if( (card_info[drive].csd[3]>>30) == 1)
    {
        /* CSD version 2.0 */
        c_size = ((card_info[drive].csd[2] & 0x3f) << 16) + (card_info[drive].csd[1]>>16) + 1;
        card_info[drive].max_read_bl_len = 1 << ((card_info[drive].csd[2] >> 16) & 0xf);
        card_info[drive].block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        card_info[drive].numblocks = c_size << 10;
        card_info[drive].capacity = card_info[drive].numblocks * card_info[drive].block_size;
    }
#endif

    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_ARG, NULL))
        return -9;

    if(!send_cmd(drive, SD_APP_CMD, card_info[drive].rca, MCI_ARG, NULL))
        return -10;

    if(!send_cmd(drive, SD_SET_BUS_WIDTH, card_info[drive].rca | 2, MCI_ARG, NULL))
        return -11;

    if(!send_cmd(drive, SD_SET_BLOCKLEN, card_info[drive].block_size, MCI_ARG,
                 NULL))
        return -12;

    card_info[drive].initialized = 1;

    mci_set_clock_divider(drive, 1); /* full speed */

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
#ifdef HAVE_HOTSWAP
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mtx); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and ata */

            disk_unmount(1);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[1].initialized = 0;

            if (ev.id == SYS_HOTSWAP_INSERTED)
                disk_mount(1);

            queue_broadcast(SYS_FS_CHANGED, 0);

            /* Access is now safe */
            mutex_unlock(&sd_mtx);
            fat_unlock();
            break;
#endif
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
static void init_pl180_controller(const int drive)
{
    MCI_COMMAND(drive) = MCI_DATA_CTRL(drive) = 0;
    MCI_CLEAR(drive) = 0x7ff;

    MCI_MASK0(drive) = MCI_MASK1(drive) = 0;  /* disable all interrupts */

    MCI_POWER(drive) = MCI_POWER_UP|(10 /*voltage*/ << 2); /* use OF voltage */
    mci_delay();

    MCI_POWER(drive) |= MCI_POWER_ON;
    mci_delay();

    MCI_SELECT(drive) = 0;

    MCI_CLOCK(drive) = MCI_CLOCK_ENABLE;
    MCI_CLOCK(drive) &= ~MCI_CLOCK_POWERSAVE;

    /* set MCLK divider */
    mci_set_clock_divider(drive,
        CLK_DIV(AS3525_PCLK_FREQ, AS3525_SD_IDENT_FREQ));

}

int sd_init(void)
{
    int ret;

    CGU_IDE =   (1<<7)  /* AHB interface enable */  |
                (1<<6)  /* interface enable */      |
                ((CLK_DIV(AS3525_PLLA_FREQ, AS3525_IDE_FREQ) - 1) << 2) |
                1       /* clock source = PLLA */;

    CGU_PERI |= CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
    CGU_PERI |= CGU_MCI_CLOCK_ENABLE;
#endif

    init_pl180_controller(INTERNAL_AS3525);
    ret = sd_init_card(INTERNAL_AS3525);
    if(ret < 0)
        return ret;

#ifdef HAVE_MULTIVOLUME
    CCU_IO &= ~8;           /* bits 3:2 = 01, xpd is SD interface */
    CCU_IO |= 4;

    init_pl180_controller(SD_SLOT_AS3525);
    sd_init_card(SD_SLOT_AS3525);
#endif
    /* init mutex */

#ifndef BOOTLOADER
    sd_enable(false);
#endif

    mutex_init(&sd_mtx);

    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

    return 0;
}

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MV2(int drive,) struct storage_info *info)
{
#ifndef HAVE_MULTIVOLUME
    const int drive=0;
#endif
    info->sector_size=card_info[drive].block_size;
    info->num_sectors=card_info[drive].numblocks;
    info->vendor="Rockbox";
    info->product = (drive == 0) ?  "Internal Storage" : "SD Card Slot";
    info->revision="0.00";
}
#endif

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MV_NONVOID(int drive))
{
#ifndef HAVE_MULTIVOLUME
    const int drive=0;
#endif
    return (drive==1);
}

bool sd_present(IF_MV_NONVOID(int drive))
{
#ifndef HAVE_MULTIVOLUME
    const int drive=0;
#endif
    return (card_info[drive].initialized && card_info[drive].numblocks > 0);
}
#endif

static int sd_wait_for_state(const int drive, unsigned int state)
{
    unsigned int response = 0;
    unsigned int timeout = 100; /* ticks */

    long t = current_tick;

    while (1)
    {
        long tick;

        if(!send_cmd(drive, SD_SEND_STATUS, card_info[drive].rca,
                    MCI_RESP|MCI_ARG, &response))
            return -1;

        if (((response >> 9) & 0xf) == state)
            return 0;

        if(TIME_AFTER(current_tick, t + timeout))
            return -1;

        if (TIME_AFTER((tick = current_tick), next_yield))
        {
            yield();
            timeout += current_tick - tick;
            next_yield = tick + MIN_YIELD_PERIOD;
        }
    }
}

static int sd_transfer_sectors(IF_MV2(int drive,) unsigned long start,
                               int count, void* buf, bool write)
{
#ifndef HAVE_MULTIVOLUME
    const int drive = 0;
#endif
    int ret;

    /* skip SanDisk OF */
    if (drive == INTERNAL_AS3525)
#if defined(SANSA_E200V2) || defined(SANSA_FUZE)
        start += 61440;
#else
        start += 20480;
#endif

    mutex_lock(&sd_mtx);
#ifndef BOOTLOADER
    sd_enable(true);
#endif

#ifdef HAVE_MULTIVOLUME
    if (drive != 0 && !card_detect_target())
    {
        /* no external sd-card inserted */
        ret = -88;
        goto sd_transfer_error;
    }
#endif

    if (card_info[drive].initialized < 0)
    {
        ret = card_info[drive].initialized;
        panicf("card not initialised");
        goto sd_transfer_error;
    }

    last_disk_activity = current_tick;

    ret = sd_wait_for_state(drive, SD_TRAN);
    if (ret < 0)
    {
        panicf("wait for state failed");
        goto sd_transfer_error;
    }

    while(count)
    {
        /* 128 * 512 = 2^16, and doesn't fit in the 16 bits of DATA_LENGTH
         * register, so we have to transfer maximum 127 sectors at a time. */
        unsigned int transfer = (count >= 128) ? 127 : count; /* sectors */
        const int cmd =
            write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;

        if(card_info[drive].ocr & (1<<30) ) /* SDHC */
            ret = send_cmd(drive, cmd, start, MCI_ARG, NULL);
        else
            ret = send_cmd(drive, cmd, start * BLOCK_SIZE,
                    MCI_ARG, NULL);

        if (ret < 0)
        {
            panicf("transfer multiple blocks failed");
            goto sd_transfer_error;
        }

        if(write)
            dma_enable_channel(0, buf, MCI_FIFO(drive),
                    (drive == INTERNAL_AS3525) ? DMA_PERI_SD : DMA_PERI_SD_SLOT,
                    DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8);
        else
            dma_enable_channel(0, MCI_FIFO(drive), buf,
                    (drive == INTERNAL_AS3525) ? DMA_PERI_SD : DMA_PERI_SD_SLOT,
                    DMAC_FLOWCTRL_PERI_PERI_TO_MEM, false, true, 0, DMA_S8);

        MCI_DATA_TIMER(drive) = 0x1000000; /* FIXME: arbitrary */
        MCI_DATA_LENGTH(drive) = transfer * card_info[drive].block_size;
        MCI_DATA_CTRL(drive) =  (1<<0) /* enable */                     |
                                (!write<<1) /* transfer direction */    |
                                (1<<3) /* DMA */                        |
                                (9<<4) /* 2^9 = 512 */ ;

        dma_wait_transfer(0);

        buf += transfer * SECTOR_SIZE;
        start += transfer;
        count -= transfer;
        last_disk_activity = current_tick;

        if(!send_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_NO_FLAGS, NULL))
        {
            ret = -666;
            panicf("STOP TRANSMISSION failed");
            goto sd_transfer_error;
        }

        ret = sd_wait_for_state(drive, SD_TRAN);
        if (ret < 0)
        {
            panicf(" wait for state TRAN failed");
            goto sd_transfer_error;
        }
    }

    while (1)
    {
#ifndef BOOTLOADER
        sd_enable(false);
#endif
        mutex_unlock(&sd_mtx);

        return ret;

sd_transfer_error:
        panicf("transfer error : %d",ret);
        card_info[drive].initialized = 0;
    }
}

int sd_read_sectors(IF_MV2(int drive,) unsigned long start, int count,
                     void* buf)
{
    return sd_transfer_sectors(IF_MV2(drive,) start, count, buf, false);
}

int sd_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
                     const void* buf)
{

#ifdef BOOTLOADER /* we don't need write support in bootloader */
#ifdef HAVE_MULTIVOLUME
    (void) drive;
#endif
    (void) start;
    (void) count;
    (void) buf;
    return -1;
#else
    return sd_transfer_sectors(IF_MV2(drive,) start, count, (void*)buf, true);
#endif
}

#ifndef BOOTLOADER
void sd_sleep(void)
{
}

void sd_spin(void)
{
}

void sd_spindown(int seconds)
{
    (void)seconds;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

void sd_enable(bool on)
{
    if(on)
    {
        CGU_PERI |= CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
        CGU_PERI |= CGU_MCI_CLOCK_ENABLE;
#endif
        CGU_IDE |= (1<<7)  /* AHB interface enable */  |
                   (1<<6)  /* interface enable */;
    }
    else
    {
        CGU_PERI &= ~CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
        CGU_PERI &= ~CGU_MCI_CLOCK_ENABLE;
#endif
        CGU_IDE &= ~((1<<7)|(1<<6));
    }
}

/* move the sd-card info to mmc struct */
tCardInfo *card_get_info_target(int card_no)
{
    int i, temp;
    static tCardInfo card;
    static const char mantissa[] = {  /* *10 */
        0,  10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
    static const int exponent[] = {  /* use varies */
      1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };

    card.initialized  = card_info[card_no].initialized;
    card.ocr          = card_info[card_no].ocr;
    for(i=0; i<4; i++)  card.csd[i] = card_info[card_no].csd[i];
    for(i=0; i<4; i++)  card.cid[i] = card_info[card_no].cid[i];
    card.numblocks    = card_info[card_no].numblocks;
    card.blocksize    = card_info[card_no].block_size;
    temp              = card_extract_bits(card.csd, 29, 3);
    card.speed        = mantissa[card_extract_bits(card.csd, 25, 4)]
                      * exponent[temp > 2 ? 7 : temp + 4];
    card.nsac         = 100 * card_extract_bits(card.csd, 16, 8);
    temp              = card_extract_bits(card.csd, 13, 3);
    card.tsac         = mantissa[card_extract_bits(card.csd, 9, 4)]
                      * exponent[temp] / 10;
    card.cid[0]       = htobe32(card.cid[0]); /* ascii chars here */
    card.cid[1]       = htobe32(card.cid[1]); /* ascii chars here */
    temp = *((char*)card.cid+13); /* adjust year<=>month, 1997 <=> 2000 */
    *((char*)card.cid+13) = (unsigned char)((temp >> 4) | (temp << 4)) + 3;

    return &card;
}

bool card_detect_target(void)
{
#ifdef HAVE_HOTSWAP
    /* TODO */
    return false;
#else
    return false;
#endif
}

#ifdef HAVE_HOTSWAP
void card_enable_monitoring_target(bool on)
{
    if (on)
    {
        /* TODO */
    }
    else
    {
        /* TODO */
    }
}
#endif

#endif /* BOOTLOADER */
