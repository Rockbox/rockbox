/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#ifndef ATA_SD_TARGET_H
#define ATA_SD_TARGET_H

#include "inttypes.h"
#include "hotswap.h"
#include "jz4740.h"

tCardInfo *card_get_info_target(int card_no);
bool       card_detect_target(void);

#ifdef HAVE_HOTSWAP
void       card_enable_monitoring_target(bool on);
void       microsd_int(void); /* ??? */
#endif

int _sd_read_sectors(unsigned long start, int count, void* buf);
int _sd_write_sectors(unsigned long start, int count, const void* buf);
int _sd_init(void);

#define MMC_CD_PIN    (29 + 1 * 32)  /* Pin to check card insertion */
//#define MMC_POWER_PIN (22 + 2 * 32)  /* Pin to enable/disable card power */

#ifdef MMC_POWER_PIN
#define MMC_POWER_OFF()                   \
do {                                      \
          __gpio_clear_pin(MMC_POWER_PIN);  \
} while (0)

#define MMC_POWER_ON()                     \
do {                                       \
          __gpio_set_pin(MMC_POWER_PIN); \
} while (0)
#endif

static inline void mmc_init_gpio(void)
{
    __gpio_as_msc();
    __gpio_as_input(MMC_CD_PIN);
#ifdef MMC_POWER_PIN
    __gpio_as_func0(MMC_POWER_PIN);
    __gpio_as_output(MMC_POWER_PIN);
#endif
    __gpio_enable_pull(32*3+29);
    __gpio_enable_pull(32*3+10);
    __gpio_enable_pull(32*3+11);
    __gpio_enable_pull(32*3+12);
}

#endif
