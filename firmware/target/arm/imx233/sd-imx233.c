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

/**
 * This code assumes a single SD card slot
 */

#ifdef SANSA_FUZEPLUS
#define SD_SSP      1
#else
#error You need to configure the ssp to use
#endif

static tCardInfo card_info;
static struct mutex sd_mutex;

static void sd_detect_callback(int ssp)
{
    (void)ssp;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if(imx233_ssp_sdmmc_detect(SD_SSP))
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
    printf("sd_detect_callback(%d)", imx233_ssp_sdmmc_detect(SD_SSP));
    imx233_ssp_sdmmc_setup_detect(SD_SSP, true, sd_detect_callback);
}

int sd_init(void)
{
    mutex_init(&sd_mutex);
    
    imx233_ssp_start(SD_SSP);
    imx233_ssp_softreset(SD_SSP);
    imx233_ssp_set_mode(SD_SSP, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    #ifdef SANSA_FUZEPLUS
    imx233_ssp_setup_ssp1_sd_mmc_pins(true, 4, PINCTRL_DRIVE_8mA, false);
    #endif
    imx233_ssp_sdmmc_setup_detect(SD_SSP, true, sd_detect_callback);
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(SD_SSP, 240, 0, 0xffff);
    imx233_ssp_set_bus_width(SD_SSP, 1);
    imx233_ssp_set_block_size(SD_SSP, 9);
    
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
    (void) first_drive;
    return 1;
}

bool sd_present(IF_MD(int drive))
{
    IF_MD((void) drive);
    return imx233_ssp_sdmmc_detect(SD_SSP);
}

