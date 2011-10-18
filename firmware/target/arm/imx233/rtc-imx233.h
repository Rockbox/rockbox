/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 by Amaury Pouly
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
#ifndef RTC_IMX233_H
#define RTC_IMX233_H

#include "config.h"
#include "system.h"
#include "cpu.h"

#define HW_RTC_BASE     0x8005C000 

#define HW_RTC_CTRL         (*(volatile uint32_t *)(HW_RTC_BASE + 0x0))

#define HW_RTC_PERSISTENT0  (*(volatile uint32_t *)(HW_RTC_BASE + 0x60))
#define HW_RTC_PERSISTENT0__SPARE_BP    18
#define HW_RTC_PERSISTENT0__SPARE_BM    (0x3fff << 18)
#define HW_RTC_PERSISTENT0__SPARE__RELEASE_GND  (1 << 19)

#endif /* RTC_IMX233_H */
