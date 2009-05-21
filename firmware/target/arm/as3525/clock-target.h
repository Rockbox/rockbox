/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#ifndef CLOCK_TARGET_H
#define CLOCK_TARGET_H

/* returns clock divider, given maximal target frequency and clock reference */
#define CLK_DIV(ref, target) ((ref + target - 1) / target)

/* PLL */

#define AS3525_PLLA_FREQ        248000000
#define AS3525_PLLA_SETTING     0x261F

/* CPU */

/* ensure that PLLA_FREQ * prediv == CPUFREQ_MAX */
#define AS3525_CPU_PREDIV       0 /* div = 1/1 */

#define CPUFREQ_MAX             248000000

#define CPUFREQ_DEFAULT         24800000

#define CPUFREQ_NORMAL          31000000

/* peripherals */

#define AS3525_PCLK_FREQ        62000000
#if (CLK_DIV(AS3525_PLLA_FREQ, AS3525_PCLK_FREQ) - 1) >= (1<<4) /* 4 bits */
#error PCLK frequency is too low : clock divider will not fit !
#endif

#define AS3525_IDE_FREQ         90000000    /* The OF uses 66MHz maximal freq
                                               but sd transfers fail on some
                                               players with this limit */
#if (CLK_DIV(AS3525_PLLA_FREQ, AS3525_IDE_FREQ) - 1) >= (1<<4) /* 4 bits */
#error IDE frequency is too low : clock divider will not fit !
#endif

#define AS3525_I2C_FREQ         400000
#if (CLK_DIV(AS3525_PCLK_FREQ, AS3525_I2C_FREQ)) >= (1<<10) /* 2+8 bits */
#error I2C frequency is too low : clock divider will not fit !
#endif

#define AS3525_DBOP_FREQ        32000000
#if (CLK_DIV(AS3525_PCLK_FREQ, AS3525_DBOP_FREQ) - 1) >= (1<<3) /* 3 bits */
#error DBOP frequency is too low : clock divider will not fit !
#endif

#define AS3525_SD_IDENT_FREQ    400000      /* must be between 100 & 400 kHz */
#if ((CLK_DIV(AS3525_PCLK_FREQ, AS3525_SD_IDENT_FREQ) / 2) - 1) >= (1<<8) /* 8 bits */
#error SD IDENTIFICATION frequency is too low : clock divider will not fit !
#endif

#endif /* CLOCK_TARGET_H */
