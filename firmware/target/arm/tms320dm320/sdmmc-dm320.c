/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
 
#include "sd.h"
#include "system.h"
#include <string.h>
#include "gcc_extensions.h"
#include "thread.h"
#include "panic.h"
#include "kernel.h"
#include "dma-target.h"
#include "ata_idle_notify.h"

//#define SD_DEBUG

#ifdef SD_DEBUG
#include "lcd-target.h"
#include "lcd.h"
#include "font.h"
#ifdef BOOTLOADER
#include "common.h"
#else
#include "debug.h"
#endif
#endif
#include "sdmmc.h"
#include "disk.h"
#include "system-target.h"

/* The configuration method is not very flexible. */
#define CARD_NUM_SLOT   1
#define NUM_CARDS       2

#define EC_OK                    0
#define EC_FAILED                1
#define EC_NOCARD                2
#define EC_WAIT_STATE_FAILED     3
#define EC_POWER_UP              4
#define EC_FIFO_WR_EMPTY         5
#define EC_FIFO_WR_DONE          6
#define EC_TRAN_READ_ENTRY       7
#define EC_TRAN_READ_EXIT        8
#define EC_TRAN_WRITE_ENTRY      9
#define EC_TRAN_WRITE_EXIT       10
#define EC_COMMAND               11
#define EC_WRITE_PROTECT         12
#define EC_DATA_TIMEOUT          13
#define EC_RESP_TIMEOUT          14
#define EC_CRC_ERROR             15
#define NUM_EC                   16

#define MIN_YIELD_PERIOD        5
#define UNALIGNED_NUM_SECTORS   10
#define MAX_TRANSFER_ERRORS     10

#define BLOCKS_PER_BANK    0x7A7800

/* command flags for send_cmd */
#define SDHC_RESP_FMT_NONE 0x0000
#define SDHC_RESP_FMT_1    0x0200
#define SDHC_RESP_FMT_2    0x0400
#define SDHC_RESP_FMT_3    0x0600

#define INITIAL_CLK    312500   /* Initial clock */
#define SD_CLK       24000000   /* Clock for SD cards */
#define MMC_CLK      15000000   /* Clock for MMC cards */

#ifdef SD_DEBUG
#ifdef BOOTLOADER
#define dbgprintf printf
#else
#define dbgprintf DEBUGF
#endif
#else
#define dbgprintf(...)
#endif

struct sd_card_status
{
    int retry;
    int retry_max;
};

/** static, private data **/ 

/* for compatibility */
static long last_disk_activity = -1;

static bool initialized = false;
static unsigned int sd_thread_id = 0;

static bool sd_enabled = false;
static long next_yield = 0;

static tCardInfo card_info [NUM_CARDS];
static tCardInfo *currcard;

static struct sd_card_status sd_status[NUM_CARDS] =
{
#if NUM_CARDS > 1
    {0, 10},
#endif
    {0, 10}
};

/* Shoot for around 75% usage */
static long             sd_stack [(DEFAULT_STACK_SIZE*2 + 0x1c0)/sizeof(long)];
static const char               sd_thread_name[] = "sd";
static struct mutex             sd_mtx SHAREDBSS_ATTR;
static struct event_queue       sd_queue;
static volatile unsigned int    transfer_error[NUM_DRIVES];
/* align on cache line size */
static unsigned char    aligned_buffer[UNALIGNED_NUM_SECTORS * SD_BLOCK_SIZE] 
                        __attribute__((aligned(32)));

static void sd_card_mux(int card_no)
{
#ifdef HAVE_MULTIDRIVE
#ifdef SANSA_CONNECT
    /* GIO6 - select Card; GIO5 - select iNAND (both active low) */
    if (card_no == CARD_NUM_SLOT)
    {
        IO_GIO_BITSET0 = (1 << 5); /* deselect iNAND (GIO5) */
        IO_GIO_BITCLR0 = (1 << 6); /* select card (GIO6) */
    }
    else
    {
        IO_GIO_BITSET0 = (1 << 6); /* deselect card (GIO6) */
        IO_GIO_BITCLR0 = (1 << 5); /* select iNAND (GIO5) */
    }
#else /* Different players */
    (void)card_no;
#endif
#else /* No multidrive */
    (void)card_no;
#endif
}


void sd_enable(bool on)
{
    if (sd_enabled == on)
        return; /* nothing to do */

    if (on)
    {
        sd_enabled = true;
    }
    else
    {
        sd_enabled = false;
    }
}

/* sets clock rate just like OF does */
static void sd_set_clock_rate(unsigned long rate)
{
    unsigned char rate_val = 0;

    if (rate == INITIAL_CLK)
    {
        rate_val = 0x3B;
    }
    else if (rate > INITIAL_CLK)
    {
        rate_val = 0;
    }
    else
    {
        rate_val = 0xFF;
    }

    IO_MMC_MEM_CLK_CONTROL = (IO_MMC_MEM_CLK_CONTROL & 0xFF00) | rate_val;
}

static int sd_poll_status(int st_reg_num, volatile unsigned int flag)
{
    unsigned int status;
    unsigned int status1;
    bool done;

    do
    {
        long time = current_tick;

        if (TIME_AFTER(time, next_yield))
        {
            long ty = current_tick;
            yield();
            next_yield = ty + MIN_YIELD_PERIOD;
        }

        status = IO_MMC_STATUS0;
        status1 = IO_MMC_STATUS1;

        if (status & MMC_ST0_CMD_TIMEOUT)
        {
            dbgprintf("CMD timeout");
            return -EC_RESP_TIMEOUT;
        }
        if (status & MMC_ST0_DATA_TIMEOUT)
        {
            dbgprintf("DATA timeout");
            return -EC_DATA_TIMEOUT;
        }

        if (status &
            (MMC_ST0_WR_CRCERR | MMC_ST0_RD_CRCERR | MMC_ST0_RESP_CRCERR))
        {
            dbgprintf("CRC error");
            return -EC_CRC_ERROR;
        }

        if (st_reg_num == 0)
        {
            done = status & flag;
        }
        else
        {
            done = status1 & flag;
        }
    } while (!done);

    return EC_OK;
}

static int dma_wait_for_completion(void)
{
    unsigned short dma_status;

    do
    {
        long time = current_tick;

        if (TIME_AFTER(time, next_yield))
        {
            long ty = current_tick;
            yield();
            next_yield = ty + MIN_YIELD_PERIOD;
        }

        dma_status = IO_MMC_SD_DMA_STATUS1;
        if (dma_status & (1 << 13))
        {
            return -EC_DATA_TIMEOUT;
        }
    } while (dma_status & (1 << 12));

    return EC_OK;
}

static int sd_command(int cmd, unsigned long arg,
                      int cmdat, unsigned long *response)
{
    int ret;

    /* Clear response registers */
    IO_MMC_RESPONSE0 = 0;
    IO_MMC_RESPONSE1 = 0;
    IO_MMC_RESPONSE2 = 0;
    IO_MMC_RESPONSE3 = 0;
    IO_MMC_RESPONSE4 = 0;
    IO_MMC_RESPONSE5 = 0;
    IO_MMC_RESPONSE6 = 0;
    IO_MMC_RESPONSE7 = 0;
    IO_MMC_COMMAND_INDEX = 0;
    IO_MMC_SPI_DATA = 0;

    IO_MMC_ARG_LOW = (unsigned int)((arg & 0xFFFF));
    IO_MMC_ARG_HI = (unsigned int)((arg & 0xFFFF0000) >> 16);

    /* SD is always in push-pull mode */
    cmdat |= MMC_CMD_PPLEN;

    cmdat |= (cmd & MMC_CMD_CMD_MASK);

    if (cmdat & MMC_CMD_DATA)
        cmdat |= MMC_CMD_DCLR;

    IO_MMC_COMMAND = cmdat;

    if (cmdat & MMC_CMD_DATA)
    {
        /* Command requires data - do not wait for RSPDNE */
        ret = EC_OK;
    }
    else
    {
        ret = sd_poll_status(0, MMC_ST0_RSPDNE);
    }

    if (ret != EC_OK)
    {
        dbgprintf("Command failed (ret %d)", ret);
        return ret;
    }

    if (response == NULL)
    {
        /* discard response */
    }
    else if ((cmdat & SDHC_RESP_FMT_1) || (cmdat & SDHC_RESP_FMT_3))
    {
        response[0] = (IO_MMC_RESPONSE7 << 16) | IO_MMC_RESPONSE6;
    }
    else if (cmdat & SDHC_RESP_FMT_2)
    {
        response[0] = (IO_MMC_RESPONSE7 << 16) | IO_MMC_RESPONSE6;
        response[1] = (IO_MMC_RESPONSE5 << 16) | IO_MMC_RESPONSE4;
        response[2] = (IO_MMC_RESPONSE3 << 16) | IO_MMC_RESPONSE2;
        response[3] = (IO_MMC_RESPONSE1 << 16) | IO_MMC_RESPONSE0;
    }

    return 0;
}

static int sd_init_card(const int card_no)
{
    bool sdhc = false;
    unsigned long response[4];
    int ret;
    int i;

    memset(currcard, 0, sizeof(*currcard));
    sd_card_mux(card_no);

    /* Set data bus width to 1 bit */
    bitclr16(&IO_MMC_CONTROL, MMC_CTRL_WIDTH);
    sd_set_clock_rate(INITIAL_CLK);

    /* Prevent controller lock */
    udelay(100);

    ret = sd_command(SD_GO_IDLE_STATE, 0, MMC_CMD_INITCLK, NULL);

    if (ret < 0)
        return -1;

    ret = sd_command(SD_SEND_IF_COND, 0x1AA,
                     SDHC_RESP_FMT_3, response);
    if ((response[0] & 0xFFF) == 0x1AA)
    {
        sdhc = true;
        dbgprintf("found sdhc card");
    }

    while ((currcard->ocr & (1 << 31)) == 0) /* until card is powered up */
    {
        ret = sd_command(SD_APP_CMD, currcard->rca,
                         SDHC_RESP_FMT_1, NULL);
        if (ret < 0)
        {
            dbgprintf("SD_APP_CMD failed");
            return -1;
        }

        ret = sd_command(SD_APP_OP_COND,
                         (1 << 20) /* 3.2-3.3V */ |
                         (1 << 21) /* 3.3-3.4V */ |
                         (sdhc ? (1 << 30) : 0),
                         SDHC_RESP_FMT_3, &currcard->ocr);

        if (ret < 0)
        {
            dbgprintf("SD_APP_OP_COND failed");
            return -1;
        }
    }

    dbgprintf("Card powered up");

    ret = sd_command(SD_ALL_SEND_CID, 0,
                     SDHC_RESP_FMT_2, response);
    if (ret < 0)
    {
        dbgprintf("SD_ALL_SEND_CID failed");
        return -1;
    }

    for (i = 0; i<4; i++)
    {
        currcard->cid[i] = response[i];
    }

    ret = sd_command(SD_SEND_RELATIVE_ADDR, 0,
                     SDHC_RESP_FMT_1, &currcard->rca);
    if (ret < 0)
    {
        dbgprintf("SD_SEND_RELATIVE_ADDR failed"); 
        return -1;
    }

    ret = sd_command(SD_SEND_CSD, currcard->rca,
                     SDHC_RESP_FMT_2, response);
    if (ret < 0)
    {
        dbgprintf("SD_SEND_CSD failed");
        return -1;
    }

    for (i = 0; i<4; i++)
    {
        currcard->csd[i] = response[i];
    }

    sd_parse_csd(currcard);

    sd_set_clock_rate(currcard->speed);

    /* Prevent controller lock */
    udelay(100);

    ret = sd_command(SD_SELECT_CARD, currcard->rca,
                     SDHC_RESP_FMT_1, NULL);
    if (ret < 0)
    {
        dbgprintf("SD_SELECT_CARD failed");
        return -1;
    }

    ret = sd_command(SD_APP_CMD, currcard->rca,
                     SDHC_RESP_FMT_1, NULL);
    if (ret < 0)
    {
        dbgprintf("SD_APP_CMD failed");
        return -1;
    }

    ret = sd_command(SD_SET_BUS_WIDTH, currcard->rca | 2,
                     SDHC_RESP_FMT_1, NULL); /* 4 bit */
    if (ret < 0)
    {
        dbgprintf("SD_SET_BUS_WIDTH failed");
        return -1;
    }

    /* Set data bus width to 4 bits */
    bitset16(&IO_MMC_CONTROL, MMC_CTRL_WIDTH);

    ret = sd_command(SD_SET_BLOCKLEN, currcard->blocksize,
                     SDHC_RESP_FMT_1, NULL);
    if (ret < 0)
    {
        dbgprintf("SD_SET_BLOCKLEN failed");
        return -1;
    }

    IO_MMC_BLOCK_LENGTH = currcard->blocksize;

    dbgprintf("Card initialized");
    currcard->initialized = 1;

    return EC_OK;
}

/* lock must already by aquired */
static void sd_select_device(int card_no)
{
    currcard = &card_info[card_no];

    if (card_no == 0)
    {
        /* Main card always gets a chance */
        sd_status[0].retry = 0;
    }

    if (currcard->initialized > 0)
    {
        /* This card is already initialized - switch to it */
        sd_card_mux(card_no);
        return;
    }

    if (currcard->initialized == 0)
    {
        /* Card needs (re)init */
        sd_init_card(card_no);
    }
}

static inline bool card_detect_target(void)
{
#ifdef SANSA_CONNECT
    bool removed;

    removed = IO_GIO_BITSET0 & (1 << 14);

    return !removed;
#else
    return false;
#endif
}


#ifdef HAVE_HOTSWAP

static int sd1_oneshot_callback(struct timeout *tmo)
{
    (void)tmo;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if (card_detect_target())
    {
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    }
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
    return 0;
}

#ifdef SANSA_CONNECT
void GIO14(void) __attribute__ ((section(".icode")));
void GIO14(void)
{
    static struct timeout sd1_oneshot;

    /* clear interrupt */
    IO_INTC_IRQ2 = (1<<3);

    timeout_register(&sd1_oneshot, sd1_oneshot_callback, (3*HZ/10), 0);
}
#endif

bool sd_removable(IF_MD_NONVOID(int card_no))
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif

    return (card_no == CARD_NUM_SLOT);
}

bool sd_present(IF_MD_NONVOID(int card_no))
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif

    return (card_no == CARD_NUM_SLOT) ? card_detect_target() :
#ifdef SANSA_CONNECT
    true; /* iNAND is always present */
#else
    false;
#endif
}

#else /* no hotswap */

bool sd_removable(IF_MD_NONVOID(int card_no))
{
#ifdef HAVE_MULTIDRIVE
    (void)card_no;
#endif
    
    /* not applicable */
    return false;
}

#endif /* HAVE_HOTSWAP */

static void sd_thread(void) NORETURN_ATTR;
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
        case SYS_HOTSWAP_EXTRACTED:;
            int success = 1;

            disk_unmount(0);     /* release "by force" */

            mutex_lock(&sd_mtx); /* lock-out card activity */

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[0].initialized = 0;

            mutex_unlock(&sd_mtx);

            if (ev.id == SYS_HOTSWAP_INSERTED)
                success = disk_mount(0); /* 0 if fail */

            /* notify the system about the changed filesystems */
            if (success)
                queue_broadcast(SYS_FS_CHANGED, 0);

            break;
#endif /* HAVE_HOTSWAP */

        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else if (!idle_notified)
            {
                call_storage_idle_notifys(false);
                idle_notified = true;
            }
            break;
        }        
    }
}

static int sd_wait_for_state(unsigned int state)
{
    unsigned long response = 0;
    unsigned int timeout = HZ; /* ticks */
    long t = current_tick;

    while (1)
    {
        long tick;
        int ret = sd_command(SD_SEND_STATUS, currcard->rca,
                             SDHC_RESP_FMT_1, &response);
        if (ret < 0)
            return ret;

        if ((SD_R1_CURRENT_STATE(response) == state))
        {
            return EC_OK;
        }

        if(TIME_AFTER(current_tick, t + timeout))
            return -2;

        if (TIME_AFTER((tick = current_tick), next_yield))
        {
            yield();
            timeout += current_tick - tick;
            next_yield = tick + MIN_YIELD_PERIOD;
        }
    }
}

static int sd_transfer_sectors(int card_no, unsigned long start,
                               int count, void *buffer, bool write)
{
    int ret;
    unsigned long start_addr;
    int dma_channel = -1;
    bool use_direct_dma;
    int count_per_dma;
    unsigned long rel_addr;

    dbgprintf("transfer %d %d %d", card_no, start, count);
    mutex_lock(&sd_mtx);
    sd_enable(true);

sd_transfer_retry:
    if (card_no == CARD_NUM_SLOT && !card_detect_target())
    {
        /* no external sd-card inserted */
        ret = -EC_NOCARD;
        goto sd_transfer_error;
    }

    sd_select_device(card_no);

    if (currcard->initialized < 0)
    {
        ret = currcard->initialized;
        goto sd_transfer_error;
    }

    last_disk_activity = current_tick;

    ret = sd_wait_for_state(SD_TRAN);
    if (ret < EC_OK)
    {
        goto sd_transfer_error;
    }

    IO_MMC_BLOCK_LENGTH = currcard->blocksize;

    start_addr = start;

    do
    {
        count_per_dma = count;

        if (((unsigned long)buffer) & 0x1F)
        {
            /* MMC/SD interface requires 32-byte alignment of buffer */
            use_direct_dma = false;
            if (count > UNALIGNED_NUM_SECTORS)
            {
                count_per_dma = UNALIGNED_NUM_SECTORS;
            }
        }
        else
        {
            use_direct_dma = true;
        }

        if (write == true)
        {
            if (use_direct_dma == false)
            {
                memcpy(aligned_buffer, buffer, count_per_dma*SD_BLOCK_SIZE);
            }
            commit_dcache_range(use_direct_dma ? buffer : aligned_buffer,
                                count_per_dma*SD_BLOCK_SIZE);
        }

        IO_MMC_NR_BLOCKS = count_per_dma;

        /* Set start_addr to the correct unit (blocks or bytes) */
        if (!(card_info[card_no].ocr & SD_OCR_CARD_CAPACITY_STATUS))
            start_addr *= SD_BLOCK_SIZE; /* not SDHC */

        ret = sd_command(write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK,
                         start_addr, MMC_CMD_DCLR | MMC_CMD_DATA |
                         SDHC_RESP_FMT_1 | (write ? MMC_CMD_WRITE : 0),
                         NULL);

        if (ret < 0)
            goto sd_transfer_error;

        /* other burst modes are not supported for this peripheral */
        dma_channel = dma_request_channel(DMA_PERIPHERAL_MMCSD,
                                          DMA_MODE_8_BURST);

        if (use_direct_dma == true)
        {
            rel_addr = ((unsigned long)buffer)-CONFIG_SDRAM_START;
        }
        else
        {
            rel_addr = ((unsigned long)aligned_buffer)-CONFIG_SDRAM_START;
        }

        IO_MMC_SD_DMA_ADDR_LOW = rel_addr & 0xFFFF;
        IO_MMC_SD_DMA_ADDR_HI = (rel_addr & 0xFFFF0000) >> 16;

        IO_MMC_SD_DMA_MODE |= MMC_DMAMODE_ENABLE;
        if (write == true)
        {
            IO_MMC_SD_DMA_MODE |= MMC_DMAMODE_WRITE;
        }

        IO_MMC_SD_DMA_TRIGGER = 1;

        dbgprintf("SD DMA transfer in progress");

        ret = dma_wait_for_completion();
        dma_release_channel(dma_channel);

        dbgprintf("SD DMA transfer complete");

        if (ret != EC_OK)
        {
            goto sd_transfer_error;
        }

        count -= count_per_dma;

        if (write == false)
        {
            discard_dcache_range(use_direct_dma ? buffer : aligned_buffer,
                                 count_per_dma*SD_BLOCK_SIZE);

            if (use_direct_dma == false)
            {
                memcpy(buffer, aligned_buffer, count_per_dma*SD_BLOCK_SIZE);
            }
        }

        buffer += count_per_dma*SD_BLOCK_SIZE;
        start_addr += count_per_dma;

        last_disk_activity = current_tick;

        ret = sd_command(SD_STOP_TRANSMISSION, 0, SDHC_RESP_FMT_1, NULL);
        if (ret < 0)
        {
            goto sd_transfer_error;
        }

        ret = sd_wait_for_state(SD_TRAN);
        if (ret < 0)
        {
            goto sd_transfer_error;
        }
    } while (count > 0);

    while (1)
    {
        sd_enable(false);
        mutex_unlock(&sd_mtx);

        return ret;

sd_transfer_error:
        if (sd_status[card_no].retry < sd_status[card_no].retry_max
            && ret != -EC_NOCARD)
        {
            sd_status[card_no].retry++;
            currcard->initialized = 0;
            goto sd_transfer_retry;
        }
    }
}

int sd_read_sectors(IF_MD(int card_no,) unsigned long start, int incount,
                     void* inbuf)
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    return sd_transfer_sectors(card_no, start, incount, inbuf, false);
}

int sd_write_sectors(IF_MD(int card_no,) unsigned long start, int count,
                      const void* outbuf)
{
#ifndef BOOTLOADER
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    return sd_transfer_sectors(card_no, start, count, (void*)outbuf, true);
#else /* we don't need write support in bootloader */
#ifdef HAVE_MULTIDRIVE
    (void)card_no;
#endif
    (void)start;
    (void)count;
    (void)outbuf;
    return 0;
#endif
}

int sd_init(void)
{
    int ret = EC_OK;

#ifndef BOOTLOADER
    sd_enabled = true;
    sd_enable(false);
#endif
    mutex_init(&sd_mtx);

    mutex_lock(&sd_mtx);
    initialized = true;

    /* based on linux/drivers/mmc/dm320mmc.c
       Copyright (C) 2006 ZSI, All Rights Reserved.
       Written by: Ben Bostwick */

    bitclr16(&IO_CLK_MOD2, CLK_MOD2_MMC);
    bitset16(&IO_CLK_INV, CLK_INV_MMC);

    /* mmc module clock: 75 Mhz (AHB) / 2 = ~37.5 Mhz
     * (Frequencies above are taken from Sansa Connect's OF source code) */
    IO_CLK_DIV3 = (IO_CLK_DIV3 & 0xFF00) | 0x02; /* OF uses 1 */

    bitset16(&IO_CLK_MOD2, CLK_MOD2_MMC);

    /* set mmc module into reset */
    bitset16(&IO_MMC_CONTROL, (MMC_CTRL_DATRST | MMC_CTRL_CMDRST));

    /* set resp timeout to max */
    IO_MMC_RESPONSE_TIMEOUT |= 0x1FFF;
    IO_MMC_READ_TIMEOUT = 0xFFFF;

    /* all done, take mmc module out of reset */
    bitclr16(&IO_MMC_CONTROL, (MMC_CTRL_DATRST | MMC_CTRL_CMDRST));

#ifdef SANSA_CONNECT
    /* GIO37 - Power Card; GIO38 - Power iNAND (both active low) */
    IO_GIO_DIR2 &= ~((1 << 5) /* GIO37 */ | (1 << 6) /* GIO38 */);
    IO_GIO_INV2 &= ~((1 << 5) /* GIO37 */ | (1 << 6) /* GIO38 */);
    IO_GIO_BITCLR2 = (1 << 5) | (1 << 6);

    /* GIO6 - select Card; GIO5 - select iNAND (both active low) */
    IO_GIO_DIR0 &= ~((1 << 6) /* GIO6 */ | (1 << 5) /* GIO5 */);
    IO_GIO_INV0 &= ~((1 << 6) /* GIO6 */ | (1 << 5) /* GIO5 */);
    IO_GIO_BITSET0 = (1 << 6) | (1 << 5);

#ifdef HAVE_HOTSWAP
    /* GIO14 is card detect */
    IO_GIO_DIR0 |= (1 << 14); /* Set GIO14 as input */
    IO_GIO_INV0 &= ~(1 << 14); /* GIO14 not inverted */
    IO_GIO_IRQPORT |= (1 << 14); /* Enable GIO14 external interrupt */
    IO_GIO_IRQEDGE |= (1 << 14); /* Any edge detection */

    /* Enable GIO14 interrupt */
    IO_INTC_EINT2 |= INTR_EINT2_EXT14;
#endif
#endif

    sd_select_device(1);

    /* Disable Memory Card CLK - it is enabled on demand by TMS320DM320 */
    bitclr16(&IO_MMC_MEM_CLK_CONTROL, (1 << 8));

    queue_init(&sd_queue, true);
    sd_thread_id = create_thread(sd_thread, sd_stack, sizeof(sd_stack),
        0, sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
        IF_COP(, CPU));

    mutex_unlock(&sd_mtx);

    return ret;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

tCardInfo *card_get_info_target(int card_no)
{    
    return &card_info[card_no];
}

void sd_sleepnow(void)
{
}

