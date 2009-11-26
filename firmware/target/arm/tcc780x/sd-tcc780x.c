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
 * Copyright (C) 2009 Rob Purchase
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
#include "hotswap.h"
#include "storage.h"
#include "led.h"
#include "thread.h"
#include "disk.h"
#include "fat.h"
#include "ata_idle_notify.h"
#include "usb.h"

#if defined(HAVE_INTERNAL_SD) && defined(HAVE_HOTSWAP)
#define CARD_NUM_INTERNAL 0
#define CARD_NUM_SLOT     1
#elif !defined(HAVE_INTERNAL_SD) && defined(HAVE_HOTSWAP)
#define CARD_NUM_SLOT     0
#endif

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

/* for compatibility */
static long last_disk_activity = -1;

/** static, private data **/ 
static bool initialized = false;

static long next_yield = 0;
#define MIN_YIELD_PERIOD 1000

static tCardInfo card_info[NUM_DRIVES];
static tCardInfo *currcard = NULL; /* current active card */

struct sd_card_status
{
    int retry;
    int retry_max;
};

static struct sd_card_status sd_status[NUM_DRIVES] =
{
#ifdef HAVE_INTERNAL_SD
    { 0, 1  },
#endif
#ifdef HAVE_HOTSWAP
    { 0, 10 }
#endif
};

/* Shoot for around 75% usage */
static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x1c0)/sizeof(long)];
static const char         sd_thread_name[] = "sd";
static struct mutex       sd_mtx SHAREDBSS_ATTR;
static struct event_queue sd_queue;

static int sd_first_drive = 0;


static bool sd_poll_status(unsigned int trigger, long timeout)
{
    long t = USEC_TIMER;

    while ((SDISTATUS & trigger) != trigger)
    {
        long time = USEC_TIMER;

        if (TIME_AFTER(time, next_yield))
        {
            long ty = USEC_TIMER;
            yield();
            timeout += USEC_TIMER - ty;
            next_yield = ty + MIN_YIELD_PERIOD;
        }

        if (TIME_AFTER(time, t + timeout))
            return false;
    }

    return true;
}

static int sd_command(unsigned int cmd, unsigned int arg, 
                      unsigned long* response, unsigned int resp_type)
{
    int sdi_cmd = cmd;
    
    sdi_cmd |= (127<<12) | (1<<11); /* max wait time | enable */
    
    if (resp_type)
    {
        /* response type & response required flag */
        sdi_cmd |= (resp_type<<7) | (1<<6);
    }
    
    if (cmd == SD_READ_SINGLE_BLOCK ||
        cmd == SD_READ_MULTIPLE_BLOCK ||
        cmd == SD_WRITE_BLOCK ||
        cmd == SD_WRITE_MULTIPLE_BLOCK)
    {
        sdi_cmd |= (1<<10); /* request data transfer */
    }
    
    if (!sd_poll_status(SDISTATUS_CMD_PATH_RDY, 100000))
        return -EC_COMMAND;
    
    SDIARGU = arg;
    SDICMD = sdi_cmd;
    
    udelay(10);
    
    if (response == NULL)
        return 0;
    
    if (!sd_poll_status(SDISTATUS_RESP_RCVD, 100000))
        return -EC_COMMAND;

    if (resp_type == SDICMD_RES_TYPE2)
    {
        response[0] = SDIRSPARGU0;
        response[1] = SDIRSPARGU1;
        response[2] = SDIRSPARGU2;
        response[3] = SDIRSPARGU3;
    }
    else
    {
        response[0] = SDIRSPARGU0;
    }
    
    return 0;
}

static int sd_wait_for_state(unsigned int state, int id)
{
    unsigned long response = 0;
    unsigned int timeout = 0x80000;

    long start_time = USEC_TIMER;

    while (1)
    {
        int ret = sd_command
            (SD_SEND_STATUS, currcard->rca, &response, SDICMD_RES_TYPE1);

        long us;

        if (ret < 0)
            return ret*100 - id;

        if (((response >> 9) & 0xf) == state)
        {
            return 0;
        }

        if (TIME_AFTER(USEC_TIMER, start_time + timeout))
            return -EC_WAIT_STATE_FAILED*100 - id;

        us = USEC_TIMER;
        if (TIME_AFTER(us, next_yield))
        {
            yield();
            timeout += USEC_TIMER - us;
            next_yield = us + MIN_YIELD_PERIOD;
        }
    }
}

static void sd_card_mux(int card_no)
{
    /* We only support the default card */
    (void)card_no;
}

#ifdef HAVE_HOTSWAP

static inline bool card_detect_target(void)
{
#ifdef HAVE_HOTSWAP
    return (GPIOB & (1<<26)) == 0; /* low active */
#else
    return false;
#endif
}

void card_enable_monitoring_target(bool on)
{
    if (on)
    {
        IEN |= EXT0_IRQ_MASK;
    }
    else
    {
        IEN &= ~EXT0_IRQ_MASK;
    }
}

static int sd1_oneshot_callback(struct timeout *tmo)
{
    (void)tmo;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if (card_detect_target())
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);

    return 0;
}

void EXT0(void)
{
    static struct timeout sd1_oneshot;
    
    timeout_register(&sd1_oneshot, sd1_oneshot_callback, (3*HZ/10), 0);
}

bool sd_removable(IF_MD_NONVOID(int card_no))
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    return (card_no == CARD_NUM_SLOT);
}

bool sd_present(IF_MD_NONVOID(int card_no))
{
#ifdef HAVE_MULTIDRIVE
    (void)card_no;
#endif
    return card_detect_target();
}

#else

bool sd_removable(IF_MD_NONVOID(int card_no))
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    (void)card_no;
    
    return false;
}

#endif /* HAVE_HOTSWAP */


static void sd_init_device(int card_no)
{
    int ret;
    unsigned long response;
    
    /* Initialise card data as blank */
    memset(currcard, 0, sizeof(*currcard));

    /* Switch card mux to card to initialize */
    sd_card_mux(card_no);

#ifdef HAVE_HOTSWAP
    /* Check card is inserted */
    if (card_no == CARD_NUM_SLOT)
    {
        if (GPIOB & (1<<26))
        {
            ret = -EC_NOCARD;
            goto card_init_error;
        }

        /* Card will not power up unless this is done */
        GPIOC_CLEAR = (1<<24);
    }
#endif

    ret = sd_command(SD_GO_IDLE_STATE, 0, NULL, SDICMD_RES_TYPE1);
    
    if (ret < 0)
        goto card_init_error;

    /* Use slow clock during identification (24MHz / 60 = 400kHz) */
    SDICLK = (1<<12) | 59;

    sd_command(SD_SEND_IF_COND, 0x1aa, &response, SDICMD_RES_TYPE3);
    
    if (!sd_poll_status(SDISTATUS_CMD_PATH_RDY, 100000))
        goto card_init_error;
    
    currcard->ocr = 0;
    
    long start_tick = current_tick;
    
    while ((currcard->ocr & (1<<31)) == 0
            && TIME_BEFORE(current_tick, start_tick + HZ))
    {
        udelay(100);
        sd_command(SD_APP_CMD, 0, NULL, SDICMD_RES_TYPE1);
        
        int arg = 0x100000 | ((response == 0x1aa) ? (1<<30):0);
        sd_command(SD_APP_OP_COND, arg, &currcard->ocr, SDICMD_RES_TYPE3);
    }
    
    if ((currcard->ocr & (1<<31)) == 0)
    {
        ret = -EC_POWER_UP;
        goto card_init_error;
    }
    
    ret = sd_command
        (SD_ALL_SEND_CID, 0, currcard->cid, SDICMD_RES_TYPE2);

    if (ret < 0)
        goto card_init_error;

    ret = sd_command
        (SD_SEND_RELATIVE_ADDR, 0, &currcard->rca, SDICMD_RES_TYPE1);
    
    if (ret < 0)
        goto card_init_error;
    
    ret = sd_command
        (SD_SEND_CSD, currcard->rca, currcard->csd, SDICMD_RES_TYPE2);
    
    if (ret < 0)
        goto card_init_error;
    
    sd_parse_csd(currcard);

    ret = sd_command
        (SD_SELECT_CARD, currcard->rca, NULL, SDICMD_RES_TYPE1);
    
    if (ret < 0)
        goto card_init_error;

    ret = sd_command
        (SD_APP_CMD, currcard->rca, NULL, SDICMD_RES_TYPE1);
    
    if (ret < 0)
        goto card_init_error;

    ret = sd_command             /* 4 bit */
        (SD_SET_BUS_WIDTH, currcard->rca | 2, NULL, SDICMD_RES_TYPE1);
    
    if (ret < 0)
        goto card_init_error;

    ret = sd_command
        (SD_SET_BLOCKLEN, currcard->blocksize, NULL, SDICMD_RES_TYPE1);
    
    if (ret < 0)
        goto card_init_error;

    currcard->initialized = 1;
    return;

    /* Card failed to initialize so disable it */
card_init_error:
    currcard->initialized = ret;
    return;
}

/* lock must already be acquired */
static void sd_select_device(int card_no)
{
    currcard = &card_info[card_no];

    if (currcard->initialized > 0)
    {
        /* This card is already initialized - switch to it */
        sd_card_mux(card_no);
        return;
    }

    if (currcard->initialized == 0)
    {
        /* Card needs (re)init */
        sd_init_device(card_no);
    }
}

int sd_read_sectors(IF_MD2(int card_no,) unsigned long start, int incount,
                     void* inbuf)
{
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif

    int ret = 0;
    bool aligned;
    unsigned char* buf_end;

    mutex_lock(&sd_mtx);
    sd_enable(true);
    led(true);

sd_read_retry:
    if ((card_no == CARD_NUM_SLOT) && !card_detect_target())
    {
        /* no external sd-card inserted */
        ret = -EC_NOCARD;
        goto sd_read_error;
    }

    sd_select_device(card_no);

    if (currcard->initialized < 0)
    {
        ret = currcard->initialized;
        goto sd_read_error;
    }
    
    last_disk_activity = current_tick;

    ret = sd_wait_for_state(SD_TRAN, EC_TRAN_READ_ENTRY);
    
    if (ret < 0)
        goto sd_read_error;
    
    /* Use full SD clock for data transfer (PCK_SDMMC) */
    SDICLK = (1<<13) | (1<<12); /* bypass divider | enable */
            
    /*  Block count | FIFO count | Block size (2^9) | 4-bit bus */
    SDIDCTRL = (incount << 13) | (4<<8) | (9<<4) | (1<<2);
    SDIDCTRL |= (1<<12);    /* nReset */
    
    SDIDCTRL2 = (1<<2); /* multi block, read */

    if (currcard->ocr & (1<<30))
        ret = sd_command(SD_READ_MULTIPLE_BLOCK, start, NULL, SDICMD_RES_TYPE1);
    else
        ret = sd_command(SD_READ_MULTIPLE_BLOCK, start * 512, NULL, SDICMD_RES_TYPE1);

    if (ret < 0)
        goto sd_read_error;

    aligned = (((int)inbuf & 3) == 0);

    buf_end = (unsigned char *)inbuf + incount * currcard->blocksize;

    while (inbuf < (void*)buf_end)
    {
        if (!sd_poll_status(SDISTATUS_FIFO_FETCH_REQ, 100000))
            goto sd_read_error;

        if (aligned)
        {
            unsigned int* ptr = (unsigned int*)inbuf;
            *ptr++ = SDIRDATA;
            *ptr++ = SDIRDATA;
            *ptr++ = SDIRDATA;
            *ptr   = SDIRDATA;

        }
        else
        {
            int tmp_buf[4];

            tmp_buf[0] = SDIRDATA;
            tmp_buf[1] = SDIRDATA;
            tmp_buf[2] = SDIRDATA;
            tmp_buf[3] = SDIRDATA;

            memcpy(inbuf, tmp_buf, 16);
        }
        inbuf += 16;
    }

    ret = sd_command(SD_STOP_TRANSMISSION, 0, NULL, SDICMD_RES_TYPE1);
    if (ret < 0)
        goto sd_read_error;

    ret = sd_wait_for_state(SD_TRAN, EC_TRAN_READ_EXIT);
    if (ret < 0)
        goto sd_read_error;

    while (1)
    {
        led(false);
        sd_enable(false);
        mutex_unlock(&sd_mtx);

        return ret;

sd_read_error:
        if (sd_status[card_no].retry < sd_status[card_no].retry_max
            && ret != -EC_NOCARD)
        {
            sd_status[card_no].retry++;
            currcard->initialized = 0;
            goto sd_read_retry;
        }
    }
}

int sd_write_sectors(IF_MD2(int card_no,) unsigned long start, int count,
                      const void* outbuf)
{
/* Write support is not finished yet */
/* TODO: The standard suggests using ACMD23 prior to writing multiple blocks
   to improve performance */
#ifndef HAVE_MULTIDRIVE
    const int card_no = 0;
#endif
    int ret;
    const unsigned char *buf_end;
    bool aligned;

    if ((card_no == CARD_NUM_SLOT) && (GPIOA & 0x10))
    {
        /* write protect tab set */
        return -EC_WRITE_PROTECT;
    }

    mutex_lock(&sd_mtx);
    sd_enable(true);
    led(true);

sd_write_retry:
    if ((card_no == CARD_NUM_SLOT) && !card_detect_target())
    {
        /* no external sd-card inserted */
        ret = -EC_NOCARD;
        goto sd_write_error;
    }

    sd_select_device(card_no);

    if (currcard->initialized < 0)
    {
        ret = currcard->initialized;
        goto sd_write_error;
    }
    
    ret = sd_wait_for_state(SD_TRAN, EC_TRAN_WRITE_ENTRY);
    
    if (ret < 0)
        goto sd_write_error;

    /* Use full SD clock for data transfer (PCK_SDMMC) */
    SDICLK = (1<<13) | (1<<12); /* bypass divider | enable */
            
    /* Block count | FIFO count | Block size (2^9) | 4-bit bus */
    SDIDCTRL = (count<<13) | (4<<8) | (9<<4) | (1<<2);
    SDIDCTRL |= (1<<12);    /* nReset */
    
    SDIDCTRL2 = (1<<2) | (1<<1); /* multi block, write */

    if (currcard->ocr & (1<<30))
        ret = sd_command(SD_WRITE_MULTIPLE_BLOCK, start, NULL, SDICMD_RES_TYPE1);
    else
        ret = sd_command(SD_WRITE_MULTIPLE_BLOCK, start * 512, NULL, SDICMD_RES_TYPE1);

    if (ret < 0)
        goto sd_write_error;

    aligned = (((int)outbuf & 3) == 0);

    buf_end = (unsigned char *)outbuf + count * currcard->blocksize;

    while (outbuf < (void*)buf_end)
    {
        if (aligned)
        {
            unsigned int* ptr = (unsigned int*)outbuf;
            SDIWDATA = *ptr++;
            SDIWDATA = *ptr++;
            SDIWDATA = *ptr++;
            SDIWDATA = *ptr;
        }
        else
        {
            int tmp_buf[4];
            
            memcpy(tmp_buf, outbuf, 16);

            SDIWDATA = tmp_buf[0];
            SDIWDATA = tmp_buf[1];
            SDIWDATA = tmp_buf[2];
            SDIWDATA = tmp_buf[3];
        }
        outbuf += 16;

        /* Wait for the FIFO to empty */
        if (!sd_poll_status(SDISTATUS_FIFO_LOAD_REQ, 0x80000))
        {
            ret = -EC_FIFO_WR_EMPTY;
            goto sd_write_error;
        }
    }

    last_disk_activity = current_tick;

    if (!sd_poll_status(SDISTATUS_MULTIBLOCK_END, 0x80000))
    {
        ret = -EC_FIFO_WR_DONE;
        goto sd_write_error;
    }

    ret = sd_command(SD_STOP_TRANSMISSION, 0, NULL, SDICMD_RES_TYPE1);
    if (ret < 0)
        goto sd_write_error;

    ret = sd_wait_for_state(SD_TRAN, EC_TRAN_WRITE_EXIT);
    if (ret < 0)
        goto sd_write_error;

    while (1)
    {
        led(false);
        sd_enable(false);
        mutex_unlock(&sd_mtx);

        return ret;

sd_write_error:
        if (sd_status[card_no].retry < sd_status[card_no].retry_max
            && ret != -EC_NOCARD)
        {
            sd_status[card_no].retry++;
            currcard->initialized = 0;
            goto sd_write_retry;
        }
    }
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

            /* Release "by force", ensure file descriptors aren't leaked and
               any busy ones are invalid if mounting */
            disk_unmount(sd_first_drive + CARD_NUM_SLOT); 

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[CARD_NUM_SLOT].initialized = 0;
            sd_status[CARD_NUM_SLOT].retry = 0; 

            if (ev.id == SYS_HOTSWAP_INSERTED)
                disk_mount(sd_first_drive + CARD_NUM_SLOT);

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
                next_yield = USEC_TIMER;

                if (!idle_notified)
                {
                    call_storage_idle_notifys(false);
                    idle_notified = true;
                }
            }
            break;

        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);
            break;

        case SYS_USB_DISCONNECTED:
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
            break;
        }
    }
}

void sd_enable(bool on)
{
    if(on)
    {
        /* Enable controller & clock */
        BCLKCTR |= DEV_SDMMC;
        PCLK_SDMMC = PCK_EN | (CKSEL_PLL0<<24) | 7; /* 192/8 = 24MHz */
    }
    else
    {
        /* Disable controller & clock */
        BCLKCTR &= ~DEV_SDMMC;
        PCLK_SDMMC &= ~PCK_EN;
    }
}
    
int sd_init(void)
{
    int ret = 0;
    
    if (!initialized)
        mutex_init(&sd_mtx);

    mutex_lock(&sd_mtx);

    led(false);

    if (!initialized)
    {
        initialized = true;

        SWRESET |= DEV_SDMMC;
        SWRESET &= ~DEV_SDMMC;

        /* Configure dual-purpose pins for SD usage */
        PORTCFG0 &= ~(3<<16);
        PORTCFG0 |= (1<<16);              /* SD_D0 & SD_D1 */

        PORTCFG2 &= ~((3<<2) | (3<<0));
        PORTCFG2 |= ((1<<2) | (1<<0));    /* SD_D2/D3/CK/CMD */

        /* Configure card detection GPIO as input */
        GPIOB_DIR &= ~(1<<26);

        /* Configure card power(?) GPIO as output */
        GPIOC_DIR |= (1<<24);

        queue_init(&sd_queue, true);
        create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
            IF_COP(, CPU));
        
        sleep(HZ/10);
        
#ifdef HAVE_HOTSWAP
        /* Configure interrupts for the card slot */
        TMODE &= ~EXT0_IRQ_MASK; /* edge-triggered */
        TMODEA |= EXT0_IRQ_MASK; /* trigger on both edges */
#endif
    }

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

#ifdef CONFIG_STORAGE_MULTI

int sd_num_drives(int first_drive)
{
    /* Store which logical drive number(s) we have been assigned */
    sd_first_drive = first_drive;
    
#if defined(HAVE_INTERNAL_SD) && defined(HAVE_HOTSWAP)
    return 2;
#else
    return 1;
#endif
}

void sd_sleepnow(void)
{
}

bool sd_disk_is_active(void)
{
    return false;
}

int sd_soft_reset(void)
{
    return 0;
}

int sd_spinup_time(void)
{
    return 0;
}

#endif /* CONFIG_STORAGE_MULTI */
