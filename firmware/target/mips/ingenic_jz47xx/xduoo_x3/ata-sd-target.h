/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

#include "cpu.h"
#include "system.h"

#define PIN_SD1_CD    (32*0+29)  /* Pin to check card insertion */
#define IRQ_SD1_CD    GPIO_IRQ(PIN_SD1_CD)
#define GPIO_SD1_CD   GPIO29

#define PIN_SD2_CD    (32*0+28)  /* Pin to check card insertion */
#define IRQ_SD2_CD    GPIO_IRQ(PIN_SD2_CD)
#define GPIO_SD2_CD   GPIO28

static inline void sd_init_gpio(void)
{
    __gpio_as_msc1_pd_4bit();
    __gpio_as_msc2_pb_4bit();

    __gpio_as_input(PIN_SD1_CD);
    __gpio_as_input(PIN_SD2_CD);

    __gpio_disable_pull(PIN_SD1_CD);
    __gpio_disable_pull(PIN_SD2_CD);
}

#endif
