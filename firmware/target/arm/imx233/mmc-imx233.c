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
#include "mmc.h"
#include "sdmmc.h"
#include "storage.h"
#include "ssp-imx233.h"
#include "pinctrl-imx233.h"

/**
 * This code assumes a single eMMC internal flash
 */

#ifdef SANSA_FUZEPLUS
#define MMC_SSP     2
#else
#error You need to configure the ssp to use
#endif

#define MMC_RCA     1

/** When set, this values restrict the windows of the read and writes */
static unsigned mmc_window_start = 0;
static unsigned mmc_window_end = INT_MAX;
static bool mmc_window_enable = true;

static struct mutex mmc_mutex;

void imx233_mmc_disable_window(void)
{
    mmc_window_enable = false;
}

int mmc_init(void)
{
    mutex_init(&mmc_mutex);
    
    imx233_ssp_start(MMC_SSP);
    imx233_ssp_softreset(MMC_SSP);
    imx233_ssp_set_mode(MMC_SSP, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    #ifdef SANSA_FUZEPLUS
    /** Sansa Fuze+ has an internal eMMC 8-bit wide flash, power gate is pin PWM3
     * and power up time is 20ms */
    imx233_set_pin_function(1, 29, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(1, 29, true);
    imx233_set_gpio_output(1, 29, false);
    sleep(HZ / 5);

    imx233_ssp_setup_ssp2_sd_mmc_pins(true, 8, PINCTRL_DRIVE_8mA);
    #endif
    /* SSPCLK @ 96MHz
     * gives bitrate of 96000 / 240 / 1 = 400kHz */
    imx233_ssp_set_timings(MMC_SSP, 240, 0, 0xffff);
    imx233_ssp_sd_mmc_power_up_sequence(MMC_SSP);
    imx233_ssp_set_bus_width(MMC_SSP, 1);
    imx233_ssp_set_block_size(MMC_SSP, 9);
    /* go to idle state */
    int ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 0, 0, SSP_NO_RESP, NULL, 0, false, false, NULL);
    if(ret != 0)
        return -1;
    /* send op cond until the card respond with busy bit set; it must complete within 1sec */
    unsigned timeout = current_tick + HZ;
    do
    {
        uint32_t ocr;
        ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 1, 0x40ff8000, SSP_SHORT_RESP, NULL, 0, false, false, &ocr);
        if(ret == 0 && ocr & (1 << 31))
            break;
    }while(!TIME_AFTER(current_tick, timeout));

    if(ret != 0)
        return -2;
    /* get CID */
    uint32_t cid[4];
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 2, 0, SSP_LONG_RESP, NULL, 0, false, false, cid);
    if(ret != 0)
        return -3;
    /* Set RCA */
    uint32_t status;
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 3, MMC_RCA << 16, SSP_SHORT_RESP, NULL, 0, false, false, &status);
    if(ret != 0)
        return -4;
    /* Select card */
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 7, MMC_RCA << 16, SSP_SHORT_RESP, NULL, 0, false, false, &status);
    if(ret != 0)
        return -5;
    /* Check TRAN state */
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 13, MMC_RCA << 16, SSP_SHORT_RESP, NULL, 0, false, false, &status);
    if(ret != 0)
        return -6;
    if(((status >> 9) & 0xf) != 4)
        return -7;
    /* Switch to 8-bit bus */
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 6, 0x3b70200, SSP_SHORT_RESP, NULL, 0, true, false, &status);
    if(ret != 0)
        return -8;
    /* switch error ? */
    if(status & 0x80)
        return -9;
    imx233_ssp_set_bus_width(MMC_SSP, 8);
    /* Switch to high speed mode */
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 6, 0x3b90100, SSP_SHORT_RESP, NULL, 0, true, false, &status);
    if(ret != 0)
        return -10;
    /* switch error ?*/
    if(status & 0x80)
        return -11;
    /* SSPCLK @ 96MHz
     * gives bitrate of 96 / 2 / 1 = 48MHz */
    imx233_ssp_set_timings(MMC_SSP, 2, 0, 0xffff);

    #ifdef SANSA_FUZEPLUS
    if(mmc_window_enable)
    {
        /**
         * The Fuze+ uses a strange layout: is has a first MBR at sector 0 with four entries:
         * 1) Actual user partition
         * 2) Sigmatel boot partition
         * 3)4) Other (certificate related ?) partitions
         * The partition 1) has type 1 but it's actually a type 5 (logical partition) with
         * a second partition table with usually one entry which is the FAT32 one.
         * The first table uses 512-byte sector size and the second one usually uses
         * 2048-byte logical sector size.
         *
         * We restrict mmc window to the user partition */
        uint8_t mbr[512];
        mmc_window_start = 0;
        mmc_window_end = INT_MAX;
        ret = mmc_read_sectors(IF_MD2(0,) 0, 1, mbr);
        if(ret != 0)
            return -100;
        if(mbr[510] != 0x55 || mbr[511] != 0xAA)
            return -101; /* invalid MBR */
        /* sanity check that the first partition is greater than 2Gib */
        uint8_t *ent = &mbr[446];
        mmc_window_start = ent[8] | ent[9] << 8 | ent[10] << 16 | ent[11] << 24;
        mmc_window_end = (ent[12] | ent[13] << 8 | ent[14] << 16 | ent[15] << 24) +
            mmc_window_start;
        if(ent[4] == 0x53)
            return -102; /* sigmatel partition */
        if((mmc_window_end - mmc_window_start) < 4 * 1024 * 1024)
            return -103; /* partition too small */
    }
    #endif

    return 0;
}

int mmc_num_drives(int first_drive)
{
    (void) first_drive;
    return 1;
}

#ifdef STORAGE_GET_INFO
void mmc_get_info(IF_MD2(int drive,) struct storage_info *info)
{
#ifdef HAVE_MULTIDRIVE
    (void) drive;
#endif
    info->sector_size = 512;
    info->num_sectors = 0xECC000 - 0x0001ac00;
    info->vendor = "Rockbox";
    info->product = "Internal Storage";
    info->revision = "0.00";
}
#endif

int mmc_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void *buf)
{
    IF_MD((void) drive);
    /* check window */
    start += mmc_window_start;
    if((start + count) > mmc_window_end)
        return -201;
    /* get mutex (needed because we done multiple commands for count > 0 */
    mutex_lock(&mmc_mutex);
    int ret = 0;
    uint32_t resp;

    if(count == 1)
    {
        ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 17, start, SSP_SHORT_RESP, buf,
            count, false, true, &resp);
    }
    else
    {
        ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 23, count, SSP_SHORT_RESP, NULL,
            0, false, false, &resp);
        if(ret == 0)
            ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 18, start, SSP_SHORT_RESP, buf,
                count, false, true, &resp);
    }

    mutex_unlock(&mmc_mutex);

    return ret;
}

int mmc_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf)
{
    IF_MD((void) drive);
    (void) start;
    (void) count;
    (void) buf;
    return -1;
}

bool mmc_present(IF_MD(int drive))
{
    IF_MD((void) drive);
    return true;
}

bool mmc_removable(IF_MD(int drive))
{
    IF_MD((void) drive);
    return false;
}
