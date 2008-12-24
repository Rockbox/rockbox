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

#define AS3525_PCLK_FREQ        65000000

#define AS3525_IDE_FREQ         90000000

#define AS3525_SD_IDENT_FREQ    400000      /* must be between 100 & 400 kHz */

#define AS3525_I2C_FREQ         400000

/* LCD controller : varies on the models */
#if     defined(SANSA_CLIP)
#define AS3525_DBOP_FREQ        6000000
#elif   defined(SANSA_M200V4)
#define AS3525_DBOP_FREQ        8000000
#elif   defined(SANSA_FUZE)
#define AS3525_DBOP_FREQ        24000000
#elif   defined(SANSA_E200V2)
#define AS3525_DBOP_FREQ        8000000
#elif   defined(SANSA_C200V2)
#define AS3525_DBOP_FREQ        8000000
#endif

/* macro for not giving a target clock > at the one provided */
#define CLK_DIV(ref, target) ((ref + target - 1) / target)

#endif /* CLOCK_TARGET_H */
