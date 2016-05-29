/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#ifndef __SDMMC_IMX233_H__
#define __SDMMC_IMX233_H__

#include "system.h"

/** Information about SD/MMC slot */
struct sdmmc_info_t
{
    int drive; /* drive number (for queries like storage_removable(drive) */
    const char *slot_name; /* name of the slot: 'internal' or 'microsd' */
    bool window; /* is window enabled for this slot? */
    int bus_width; /* current bus width */
    bool hs_capable; /* is device high-speed capable? */
    bool hs_enabled; /* is high-speed enabled? */
    bool has_sbc; /* device support SET_BLOCK_COUNT */
};

#if CONFIG_STORAGE & STORAGE_SD
/* return information about a particular sd device (index=0..n-1) */
struct sdmmc_info_t imx233_sd_get_info(int card_no);
#endif

#if CONFIG_STORAGE & STORAGE_MMC
/* return information about a particular mmc device (index=0..n-1) */
struct sdmmc_info_t imx233_mmc_get_info(int card_no);
#endif

#endif /* __SSP_IMX233_H__ */
