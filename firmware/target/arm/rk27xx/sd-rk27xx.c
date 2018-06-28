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
 * Copyright (C) 2011 Marcin Bukat
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
#include "thread.h"
#include "gcc_extensions.h"
#include "led.h"
#include "sdmmc.h"
#include "system.h"
#include "kernel.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "panic.h"
#include "stdbool.h"
#include "storage.h"

#include "lcd.h"
#include <stdarg.h>
#include "sysfont.h"

#define RES_NO (-1)

/* debug stuff */
unsigned long sd_debug_time_rd = 0;
unsigned long sd_debug_time_wr = 0;

static tCardInfo card_info;

/* for compatibility */
static long last_disk_activity = -1;

static unsigned char aligned_buf[512] STORAGE_ALIGN_ATTR;

static struct mutex       sd_mtx SHAREDBSS_ATTR;
#ifndef BOOTLOADER
bool sd_enabled = false;
#endif

#ifdef CONFIG_STORAGE_MULTI
static int sd_first_drive = 0;
#else
#define    sd_first_drive 0
#endif

static struct semaphore transfer_completion_signal;
static struct semaphore command_completion_signal;
static volatile bool retry;
static volatile int cmd_error;

static void enable_controller(bool on)
{
    /* enable or disable clock signal for SD module */
    if (on)
    {
        SCU_CLKCFG &= ~CLKCFG_SD;
        led(true);
    }
    else
    {
        SCU_CLKCFG |= CLKCFG_SD;
        led(false);
    }
}

/* interrupt handler for SD */
void INT_SD(void)
{
    const int status = SD_INT;

    SD_INT = 0; /* disable sd interrupts, clear pending interrupts */

    /* cmd and response status pending */
    if(status & CMD_RES_STAT)
    {
        /* get the status */
        cmd_error = SD_CMDRES;
        semaphore_release(&command_completion_signal);
    }    

    /* data transfer status pending */
    if(status & DATA_XFER_STAT)
    {
        cmd_error = SD_DATAT;
        if (cmd_error & DATA_XFER_ERR)
            retry = true;

        semaphore_release(&transfer_completion_signal);
    }

    SD_INT = CMD_RES_INT_EN | DATA_XFER_INT_EN;
}

/* Exchange buffers - the one where SD module puts into/reads from
 * data and the one controlled by MCU. This allows some overlap
 * in transfer operations and should increase throuput.
 */
static void mmu_switch_buff(void)
{
    static unsigned int i = 0;

    if (i++ & 0x01)
    {
        MMU_CTRL = MMU_MMU0_BUFII | MMU_CPU_BUFI | MMU_BUFII_RESET |
                   MMU_BUFII_BYTE | MMU_BUFI_RESET | MMU_BUFI_WORD;
    }
    else
    {
        MMU_CTRL = MMU_MMU0_BUFI | MMU_CPU_BUFII | MMU_BUFII_RESET |
                   MMU_BUFII_WORD | MMU_BUFI_RESET | MMU_BUFI_BYTE;
    }
}

/* Reset internal pointers of the MMU submodule */
static void mmu_buff_reset(void)
{
    MMU_CTRL |= MMU_BUFII_RESET | MMU_BUFI_RESET;
}

static inline bool card_detect_target(void)
{
#if defined(RK27_GENERIC)
/* My generic device uses PC7 pin, active low */
    return !(GPIO_PCDR & 0x80);
#elif defined(HM60X) || defined(HM801)
    return !(GPIO_PFDR & (1<<2));
#elif defined(MA9) || defined(MA9C) || defined(MA8) || defined(MA8C)
    return (GPIO_PCDR & 0x80);
#elif defined(IHIFI770) || defined(IHIFI770C) || defined(IHIFI800)
    return !(GPIO_PCDR & (1<<7));
#elif defined(IHIFI760) || defined(IHIFI960)
    /* TODO: find out pin */
    return true;
#else
#error "Unknown target"
#endif
}

/* Send command to the SD card. Command finish is signaled in ISR */
static bool send_cmd(const int cmd, const int arg, const int res,
        unsigned long *response)
{
    SD_CMD = arg;

    if (res > 0)
       SD_CMDREST = CMD_XFER_START | RES_XFER_START | res | cmd;
    else
       SD_CMDREST = CMD_XFER_START | RES_XFER_END | RES_R1 | cmd;

    semaphore_wait(&command_completion_signal, TIMEOUT_BLOCK);

    /*  Handle command responses & errors */
    if(res != RES_NO)
    {
        if(cmd_error & STAT_CMD_RES_ERR)
            return false;

        if(res == RES_R2)
        {
            response[0] = SD_RES3;
            response[1] = SD_RES2;
            response[2] = SD_RES1;
            response[3] = SD_RES0;
        }
        else
            response[0] = SD_RES3;
    }
    return true;
}

#if 0
/* for some misterious reason the card does not report itself as being in TRAN
 * but transfers are successful. Rockchip OF does not check the card state
 * after SELECT. I checked two different cards. 
 */
static void print_card_status(void)
{
    unsigned long response;
    send_cmd(SD_SEND_STATUS, card_info.rca, RES_R1,
             &response);

    printf("card status: 0x%0x, state: 0x%0x", response, (response>>9)&0xf);
}

static int sd_wait_for_tran_state(void)
{
    unsigned long response;
    unsigned int timeout = current_tick + 5*HZ;
    int cmd_retry = 10;

    while (1)
    {
        while (!send_cmd(SD_SEND_STATUS, card_info.rca, RES_R1,
                         &response) && cmd_retry > 0)
        {
            cmd_retry--;
        }

        if (cmd_retry <= 0)
        {
            return -1;
        }

        if (((response >> 9) & 0xf) == SD_TRAN)
        {
            return 0;
        }
    
        if(TIME_AFTER(current_tick, timeout))
        {
            return -10 * ((response >> 9) & 0xf);
        }

        last_disk_activity = current_tick;
    }
}
#endif

static bool sd_wait_card_busy(void)
{
    unsigned int timeout = current_tick + 5*HZ;

    while (!(SD_CARD & SD_CARD_BSY))
    {
        if(TIME_AFTER(current_tick, timeout))
            return false;
    }

    return true;
}

static int sd_init_card(void)
{
    unsigned long response;
    long init_timeout;
    bool sd_v2 = false;

    card_info.rca = 0;

    /*  assume 50 MHz APB freq / 125 = 400 kHz  */
    SD_CTRL = (SD_CTRL & ~(0x7FF)) | 0x7D;

    /* 100 - 400kHz clock required for Identification Mode  */
    /*  Start of Card Identification Mode ************************************/

    /* CMD0 Go Idle  */
    if(!send_cmd(SD_GO_IDLE_STATE, 0, RES_NO, NULL))
        return -1;
    
    sleep(1);

    /* CMD8 Check for v2 sd card.  Must be sent before using ACMD41
      Non v2 cards will not respond to this command*/
    if(send_cmd(SD_SEND_IF_COND, 0x1AA, RES_R6, &response))
        if((response & 0xFFF) == 0x1AA)
            sd_v2 = true;

    /* Timeout for inintialization is 2 sec.
       According to SD Specification 2.00 it should be >= 1,
       but it's not enough in some rare cases. */
    init_timeout = current_tick + 2*HZ;

    do {
        /* this timeout is the only valid error for this loop*/
        if(TIME_AFTER(current_tick, init_timeout))
            return -2;

        if(!send_cmd(SD_APP_CMD, card_info.rca, RES_R1, &response))
            return -3;

        sleep(1); /* bus conflict otherwise */

         /* ACMD41 For v2 cards set HCS bit[30] & send host voltage range to all */
        if(!send_cmd(SD_APP_OP_COND, (0x00FF8000 | (sd_v2 ? 1<<30 : 0)),
                     RES_R3, &card_info.ocr))
            return -4;
    } while(!(card_info.ocr & (1<<31)) );

    /* CMD2 send CID */
    if(!send_cmd(SD_ALL_SEND_CID, 0, RES_R2, card_info.cid))
        return -5;

    /* CMD3 send RCA */
    if(!send_cmd(SD_SEND_RELATIVE_ADDR, 0, RES_R6, &card_info.rca))
        return -6;

    /*  End of Card Identification Mode   ************************************/


    /* CMD9 send CSD */
    if(!send_cmd(SD_SEND_CSD, card_info.rca, RES_R2, card_info.csd))
        return -11;

    sd_parse_csd(&card_info);

    if(!send_cmd(SD_SELECT_CARD, card_info.rca, RES_R1b, &response))
        return -20;

    if (!sd_wait_card_busy())
        return -21;

    /* CMD6 */
    if(!send_cmd(SD_SWITCH_FUNC, 0x80fffff1, RES_R1, &response))
        return -8;
    sleep(HZ/10);

    /*  Card back to full speed  25MHz*/
    SD_CTRL = (SD_CTRL & ~0x7FF);
    card_info.initialized = 1;

    return 0;
}

static void init_controller(void)
{
    /* reset SD module */
    SCU_RSTCFG |= RSTCFG_SD;
    sleep(1);
    SCU_RSTCFG &= ~RSTCFG_SD;

    /* set pins functions as SD signals */
    SCU_IOMUXA_CON |= IOMUX_SD;

    /* enable and unmask SD interrupts in interrupt controller */
    SCU_CLKCFG &= ~CLKCFG_SD;
    INTC_IMR |= IRQ_ARM_SD;
    INTC_IECR |= IRQ_ARM_SD;

    SD_CTRL = SD_PWR_CPU | SD_DETECT_MECH | SD_CLOCK_EN | 0x7D;
    SD_INT = CMD_RES_INT_EN | DATA_XFER_INT_EN;
    SD_CARD = SD_CARD_SELECT | SD_CARD_PWR_EN;

    /* setup mmu buffers */
    MMU_PNRI = 0x1ff;
    MMU_PNRII = 0x1ff;
    MMU_CTRL = MMU_MMU0_BUFII | MMU_CPU_BUFI | MMU_BUFII_RESET |
               MMU_BUFII_BYTE | MMU_BUFI_RESET | MMU_BUFI_WORD;

    /* setup A2A DMA CH0 for SD reads */
    A2A_ISRC0 = (unsigned long)(&MMU_DATA);
    A2A_ICNT0 = 512;
    A2A_LCNT0 = 1;

    /* setup A2A DMA CH1 for SD writes */
    A2A_IDST1 = (unsigned long)(&MMU_DATA);
    A2A_ICNT1 = 512;
    A2A_LCNT1 = 1;

    /* src and dst for CH0 and CH1 is AHB0 */
    A2A_DOMAIN = 0;

#ifdef RK27XX_SD_DEBUG
    /* setup Timer1 for profiling purposes */
    TMR1CON = (1<<8)|(1<<3);
#endif
}

int sd_init(void)
{
    int ret;

    mutex_init(&sd_mtx);
    semaphore_init(&transfer_completion_signal, 1, 0);
    semaphore_init(&command_completion_signal, 1, 0);

    init_controller();

    ret = sd_init_card();
    if(ret < 0)
        return ret;

    return 0;
}

static inline void read_sd_data(unsigned char **dst)
{
    void *buf = *dst;

    if (!IS_ALIGNED(((unsigned long)*dst), CACHEALIGN_SIZE))
        buf = aligned_buf;

    commit_discard_dcache_range((const void *)buf, 512);


    A2A_IDST0 = (unsigned long)buf;
    A2A_CON0  = (3<<9) |    /* burst 16 */
                (1<<6) |    /* fixed src */
                (1<<3) |    /* DMA start */
                (2<<1) |    /* word transfer size */
                (1<<0);     /* software mode */

    /* wait for DMA engine to finish transfer */
    while (A2A_DMA_STS & 1);

    if (buf == aligned_buf)
        memcpy(*dst, aligned_buf, 512);

    *dst += 512;
}

static inline void write_sd_data(unsigned char **src)
{
    void *buf = *src;

    if (!IS_ALIGNED(((unsigned long)*src), CACHEALIGN_SIZE))
    {
        buf = aligned_buf;
        memcpy(aligned_buf, *src, 512);
    }

    commit_discard_dcache_range((const void *)buf, 512);

    A2A_ISRC1 = (unsigned long)buf;
    A2A_CON1  = (3<<9) |    /* burst 16 */
                (1<<5) |    /* fixed dst */
                (1<<3) |    /* DMA start */
                (2<<1) |    /* word transfer size */
                (1<<0);     /* software mode */

    /* wait for DMA engine to finish transfer */
    while (A2A_DMA_STS & 2);

    *src += 512;
}

int sd_read_sectors(IF_MD(int drive,) unsigned long start, int count,
                    void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    unsigned long response;
    unsigned int retry_cnt = 0;
    int cnt, ret = 0;
    unsigned char *dst;

    mutex_lock(&sd_mtx);
    enable_controller(true);

    if (count <= 0 || start + count > card_info.numblocks)
        return -1;

    if(!(card_info.ocr & (1<<30)))
        start <<= 9; /* not SDHC */

    while (retry_cnt++ < 20)
    {
        cnt = count;
        dst = (unsigned char *)buf;

        ret = 0;
        retry = false;       /* reset retry flag */

        mmu_buff_reset();

        if (cnt == 1)
        {
            /* last block to transfer */
            SD_DATAT = DATA_XFER_START | DATA_XFER_READ |
                       DATA_BUS_1LINE | DATA_XFER_DMA_DIS |
                       DATA_XFER_SINGLE;
        }
        else
        {
            /* more than one block to transfer */
            SD_DATAT = DATA_XFER_START | DATA_XFER_READ |
                       DATA_BUS_1LINE | DATA_XFER_DMA_DIS |
                       DATA_XFER_MULTI;
        }

        /* issue read command to the card */
        if (!send_cmd(SD_READ_MULTIPLE_BLOCK, start, RES_R1, &response))
        {
            ret = -2;
            continue;
        }

        while (cnt > 0)
        {
#ifdef RK27XX_SD_DEBUG
            /* debug stuff */
            TMR1LR = 0xffffffff;
#endif
            /* wait for transfer completion */
            semaphore_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

#ifdef RK27XX_SD_DEBUG
            /* debug stuff */
            sd_debug_time_rd = 0xffffffff - TMR1CVR;
#endif
            if (retry)
            {
                /* data transfer error */
                ret = -3;
                break;
            }

            /* exchange buffers */
            mmu_switch_buff();

            cnt--;

            if (cnt == 0)
            {
                if (!send_cmd(SD_STOP_TRANSMISSION, 0, RES_R1b, &response))
                    ret = -4;

                read_sd_data(&dst);

                break;
            }
            else if (cnt == 1)
            {
                /* last block to transfer */
                SD_DATAT = DATA_XFER_START | DATA_XFER_READ |
                           DATA_BUS_1LINE | DATA_XFER_DMA_DIS |
                           DATA_XFER_SINGLE;

                read_sd_data(&dst);

            }
            else
            {
                /* more than one block to transfer */
                SD_DATAT = DATA_XFER_START | DATA_XFER_READ |
                           DATA_BUS_1LINE | DATA_XFER_DMA_DIS |
                           DATA_XFER_MULTI;

                read_sd_data(&dst);
            }

            last_disk_activity = current_tick;

        } /* while (cnt > 0) */

        /* transfer successfull - leave retry loop */
        if (ret == 0)
            break;

    } /* while (retry_cnt++ < 20) */

    enable_controller(false);
    mutex_unlock(&sd_mtx);

    return ret;
}

/* Not tested */
int sd_write_sectors(IF_MD(int drive,) unsigned long start, int count,
                     const void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void) drive;
#endif
#if defined(BOOTLOADER) /* we don't need write support in bootloader */
    (void) start;
    (void) count;
    (void) buf;
    return -1;
#else

#ifdef RK27XX_SD_DEBUG
    /* debug stuff */
    TMR1LR = 0xffffffff;
#endif

    unsigned long response;
    unsigned int retry_cnt = 0;
    int cnt, ret = 0;
    unsigned char *src;
    /* bool card_selected = false; */

    mutex_lock(&sd_mtx);
    enable_controller(true);

    if (count <= 0 || start + count > card_info.numblocks)
        return -1;

    if(!(card_info.ocr & (1<<30)))
        start <<= 9; /* not SDHC */

    while (retry_cnt++ < 20)
    {
        cnt = count;
        src = (unsigned char *)buf;

        ret = 0;
        retry = false;       /* reset retry flag */
        mmu_buff_reset();    /* reset recive buff state */

        write_sd_data(&src); /* put data into transfer buffer */

        if (!send_cmd(SD_WRITE_MULTIPLE_BLOCK, start, RES_R1, &response))
        {
            ret = -3;
            continue;
        }

        while (cnt > 0)
        {
            /* exchange buffers */
            mmu_switch_buff();

            if (cnt == 1)
            {
                /* last block to transfer */
                SD_DATAT = DATA_XFER_START | DATA_XFER_WRITE |
                           DATA_BUS_1LINE | DATA_XFER_DMA_DIS |
                           DATA_XFER_SINGLE;

            }
            else
            {
                /* more than one block to transfer */
                SD_DATAT = DATA_XFER_START | DATA_XFER_WRITE |
                           DATA_BUS_1LINE | DATA_XFER_DMA_DIS |
                           DATA_XFER_MULTI;

                /* put more data */
                write_sd_data(&src);
            }
            /* wait for transfer completion */
            semaphore_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

            if (retry)
            {
                /* data transfer error */
                ret = -3;
                break;
            }

            cnt--;
        } /* while (cnt > 0) */

        if (!send_cmd(SD_STOP_TRANSMISSION, 0, RES_R1b, &response))
            ret = -4;

        if (!sd_wait_card_busy())
            ret = -5;

        /* transfer successfull - leave retry loop */
        if (ret == 0)
            break;
    }

    enable_controller(false);
    mutex_unlock(&sd_mtx);

#ifdef RK27XX_SD_DEBUG
    /* debug stuff */
    sd_debug_time_wr = 0xffffffff - TMR1CVR;
#endif

    return ret;
   
#endif /* defined(BOOTLOADER) */
}

void sd_enable(bool on)
{
    mutex_lock(&sd_mtx);
    enable_controller(on);
    mutex_unlock(&sd_mtx);
}

#ifndef BOOTLOADER
long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

tCardInfo *card_get_info_target(int card_no)
{
    (void)card_no;
    return &card_info;
}
#endif /* BOOTLOADER */

#ifdef HAVE_HOTSWAP
/* Not complete and disabled in config */
bool sd_removable(IF_MD_NONVOID(int drive))
{
    (void)drive;
    return true;
}

bool sd_present(IF_MD_NONVOID(int drive))
{
    (void)drive;
    return card_detect_target();
}

static int sd_oneshot_callback(struct timeout *tmo)
{
    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    queue_broadcast(card_detect_target() ? SYS_HOTSWAP_INSERTED :
                                           SYS_HOTSWAP_EXTRACTED,
                    sd_first_drive);
    return 0;
    (void)tmo;
}

/* interrupt handler for SD detect */

#endif /* HAVE_HOTSWAP */

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    /* we have only one SD drive */
    sd_first_drive = first_drive;
    return 1;
}
#endif /* CONFIG_STORAGE_MULTI */

int sd_event(long id, intptr_t data)
{
    int rc = 0;

    switch (id)
    {
#ifdef HAVE_HOTSWAP
    case SYS_HOTSWAP_INSERTED:
    case SYS_HOTSWAP_EXTRACTED:
        mutex_lock(&sd_mtx); /* lock-out card activity */

        /* Force card init for new card, re-init for re-inserted one or
         * clear if the last attempt to init failed with an error. */
        card_info.initialized = 0;

        if (id == SYS_HOTSWAP_INSERTED)
        {
            enable_controller(true);
            rc = sd_init_card();
            enable_controller(false);
        }

        mutex_unlock(&sd_mtx);
        break;
#endif /* HAVE_HOTSWAP */
    default:
        rc = storage_event_default_handler(id, data, last_disk_activity,
                                           STORAGE_SD);
        break;
    }

    return rc;
}
