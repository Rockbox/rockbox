/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "config.h"
#include "system.h"
#include "sd.h"
#include "sdmmc.h"
#include "ssp-imx233.h"
#include "pinctrl-imx233.h"
#include "button-target.h"
#include "fat.h"
#include "disk.h"
#include "usb.h"
#include "debug.h"

/**
 * This code assumes a single SD card slot
 */

#ifdef SANSA_FUZEPLUS
#define SD_SSP      1
#else
#error You need to configure the ssp to use
#endif

static tCardInfo card_info;
static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static struct mutex sd_mutex;
static const char sd_thread_name[] = "sd";
static struct event_queue sd_queue;
static int sd_first_drive;
static int last_disk_activity;

static void sd_detect_callback(int ssp)
{
    (void)ssp;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if(imx233_ssp_sdmmc_detect(SD_SSP))
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
    imx233_ssp_sdmmc_setup_detect(SD_SSP, true, sd_detect_callback);
}

void sd_enable(bool on)
{
    static bool sd_enable = false;
    if(sd_enable == on)
        return;
    
    mutex_lock(&sd_mutex);
    if(on)
        imx233_ssp_start(SD_SSP);
    else
        imx233_ssp_stop(SD_SSP);
    mutex_unlock(&sd_mutex);
    sd_enable = on;
}

static int sd_init_card(void)
{
    imx233_ssp_start(SD_SSP);
    imx233_ssp_softreset(SD_SSP);
    imx233_ssp_set_mode(SD_SSP, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(SD_SSP, 240, 0, 0xffff);
    imx233_ssp_set_bus_width(SD_SSP, 1);
    imx233_ssp_set_block_size(SD_SSP, 9);

    card_info.rca = 0;
    bool is_v2 = false;
    uint32_t resp;
    /* go to idle state */
    int ret = imx233_ssp_sd_mmc_transfer(SD_SSP, SD_GO_IDLE_STATE, 0, SSP_NO_RESP, NULL, 0, false, false, NULL);
    if(ret != 0)
        return -1;
    /* CMD8 Check for v2 sd card.  Must be sent before using ACMD41
       Non v2 cards will not respond to this command*/
    ret = imx233_ssp_sd_mmc_transfer(SD_SSP, SD_SEND_IF_COND, 0x1AA, SSP_SHORT_RESP, NULL, 0, false, false, &resp);
    if(ret == 0 && (resp & 0xFFF) == 0x1AA)
        is_v2 = true;
    
    return -10;
}

static void sd_thread(void) NORETURN_ATTR;
static void sd_thread(void)
{
    struct queue_event ev;

    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch(ev.id)
        {
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
        {
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mutex); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and sd */

            disk_unmount(sd_first_drive);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */
            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info.initialized = 0;

            if(ev.id == SYS_HOTSWAP_INSERTED)
            {
                int ret = sd_init_card();
                if(ret == 0)
                {
                    ret = disk_mount(sd_first_drive); /* 0 if fail */
                    if(ret < 0)
                        DEBUGF("disk_mount failed: %d", ret);
                }
                else
                    DEBUGF("sd_init_card failed: %d", ret);
            }

            /*
             * Mount succeeded, or this was an EXTRACTED event,
             * in both cases notify the system about the changed filesystems
             */
            if(card_info.initialized)
                queue_broadcast(SYS_FS_CHANGED, 0);

            /* Access is now safe */
            mutex_unlock(&sd_mutex);
            fat_unlock();
            }
            break;
        case SYS_TIMEOUT:
            if(!TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
                sd_enable(false);
            break;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);
            break;
        }
    }
}

int sd_init(void)
{
    mutex_init(&sd_mutex);
    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

    #ifdef SANSA_FUZEPLUS
    imx233_ssp_setup_ssp1_sd_mmc_pins(true, 4, PINCTRL_DRIVE_8mA, false);
    #endif
    imx233_ssp_sdmmc_setup_detect(SD_SSP, true, sd_detect_callback);

    if(imx233_ssp_sdmmc_detect(SD_SSP))
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    
    return 0;
}

int sd_read_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     void* buf)
{
    IF_MD((void) drive);
    (void) start;
    (void) count;
    (void) buf;
    return -1;
}

int sd_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                     const void* buf)
{
    IF_MD((void) drive);
    (void) start;
    (void) count;
    (void) buf;
    return -1;
}

tCardInfo *card_get_info_target(int card_no)
{
    (void)card_no;
    return NULL;
}

int sd_num_drives(int first_drive)
{
    sd_first_drive = first_drive;
    return 1;
}

bool sd_present(IF_MD(int drive))
{
    IF_MD((void) drive);
    return imx233_ssp_sdmmc_detect(SD_SSP);
}

bool sd_removable(IF_MD(int drive))
{
    IF_MD((void) drive);
    return true;
}

