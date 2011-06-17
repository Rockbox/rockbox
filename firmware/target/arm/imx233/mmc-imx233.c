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
#include "ssp-imx233.h"
#include "pinctrl-imx233.h"
#include "button-target.h"

#ifdef SANSA_FUZEPLUS
#define MMC_SSP     2
#else
#error You need to configure the ssp to use
#endif

int mmc_init(void)
{
    imx233_ssp_start(MMC_SSP);
    imx233_ssp_softreset(MMC_SSP);
    imx233_ssp_set_mode(MMC_SSP, HW_SSP_CTRL1__SSP_MODE__SD_MMC);
    #ifdef SANSA_FUZEPLUS
    /** Sansa Fuze+ has an internal eMMC 8-bit wide flash, power gate is pin PWM3 */
    imx233_set_pin_function(1, 29, PINCTRL_FUNCTION_GPIO);
    imx233_enable_gpio_output(1, 29, true);
    imx233_set_gpio_output(1, 29, false);

    imx233_ssp_setup_ssp2_sd_mmc_pins(true, 8, PINCTRL_DRIVE_8mA);
    #endif
    /* SSPCLK @ 120MHz
     * gives bitrate of 120 / 100 / 3 = 400kHz */
    imx233_ssp_set_timings(MMC_SSP, 100, 2);
    imx233_ssp_set_timeout(MMC_SSP, 0xffff);
    imx233_ssp_sd_mmc_power_up_sequence(MMC_SSP);
    /* go to idle state */
    int ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, SD_GO_IDLE_STATE, 0, SSP_NO_RESP, NULL, 0, false, NULL);
    if(ret != 0)
        return -1;
    /* send op cond until the card respond with busy bit set; it must complete within 1sec */
    unsigned timeout = current_tick + HZ;
    do
    {
        uint32_t ocr;
        ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 1, 0x40ff8000, SSP_SHORT_RESP, NULL, 0, false, &ocr);
        if(ret == 0 && ocr & (1 << 31))
            break;
    }while(!TIME_AFTER(current_tick, timeout));

    if(ret != 0)
        return -2;
    
    uint32_t cid[4];
    ret = imx233_ssp_sd_mmc_transfer(MMC_SSP, 2, 0, SSP_LONG_RESP, NULL, 0, false, cid);
    if(ret != 0)
        return -3;

    return 0;
}

int mmc_num_drives(int first_drive)
{
    (void) first_drive;
    return 1;
}

int mmc_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf)
{
    IF_MD((void) drive);
    (void) start;
    (void) count;
    (void) buf;
    return -1;
}

int mmc_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf)
{
    IF_MD((void) drive);
    (void) start;
    (void) count;
    (void) buf;
    return -1;
}
