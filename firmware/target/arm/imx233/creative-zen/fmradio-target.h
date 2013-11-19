/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef _FMRADIO_TARGET_H_
#define _FMRADIO_TARGET_H_

#define IMX233_FMRADIO_I2C  FMI_HW

#ifdef CREATIVE_ZENMOZAIC
#define IMX233_FMRADIO_POWER    FMP_GPIO
#define FMP_GPIO_BANK   2
#define FMP_GPIO_PIN    15
#define FMP_GPIO_DELAY  (HZ / 5)
#else
#define IMX233_FMRADIO_POWER    FMP_NONE
#endif

#endif /* _FMRADIO_TARGET_H_ */
 