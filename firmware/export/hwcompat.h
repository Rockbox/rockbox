/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef HWCOMPAT_H
#define HWCOMPAT_H

#include <stdbool.h>
#include "config.h"

/* Bit mask values for HW compatibility */
#define ATA_ADDRESS_200 0x0100
#define USB_ACTIVE_HIGH 0x0100
#define PR_ACTIVE_HIGH  0x0100
#define LCD_CONTRAST_BIAS 0x0200
#define MMC_CLOCK_POLARITY 0x0400
#define TUNER_MODEL 0x0800

int read_rom_version(void);
int read_hw_mask(void);

#ifdef ARCHOS_PLAYER
bool has_new_lcd(void);
bool has_ata_power_control(void) __attribute__ ((alias ("has_new_lcd")));
#endif

#endif
