/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __NAND_TARGET_H__
#define __NAND_TARGET_H__

/* The max page size (main + spare) of all NAND chips used by this target */
#define NAND_MAX_PAGE_SIZE (2048 + 64)

/* The clock source to use for the SFC controller. Note the SPL has special
 * handling which ignores this choice, so it only applies to bootloader & app.
 */
#define NAND_CLOCK_SOURCE X1000_CLK_SCLK_A

/* The clock speed to use for the SFC controller during chip identification */
#define NAND_INIT_CLOCK_SPEED 150000000

/* Initial value to program SFC_DEV_CONF register with */
#define NAND_INIT_SFC_DEV_CONF \
    jz_orf(SFC_DEV_CONF, CE_DL(1), HOLD_DL(1), WP_DL(1), \
           CPHA(0), CPOL(0), TSH(7), TSETUP(0), THOLD(0), \
           STA_TYPE_V(1BYTE), CMD_TYPE_V(8BITS), SMP_DELAY(1))

#endif /* __NAND_TARGET_H__ */
