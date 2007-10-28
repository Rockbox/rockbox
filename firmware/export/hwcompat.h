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

#if (CONFIG_CPU == SH7034) && !defined(SIMULATOR)

#define ROM_VERSION (*(short *)0x020000fe)

/* Bit mask values for HW compatibility */
#define ATA_ADDRESS_200 0x0100
#define USB_ACTIVE_HIGH 0x0100
#define PR_ACTIVE_HIGH  0x0100
#define LCD_CONTRAST_BIAS 0x0200
#define MMC_CLOCK_POLARITY 0x0400
#define TUNER_MODEL 0x0800

#ifdef ARCHOS_PLAYER
#define HW_MASK 0
#else /* Recorders, Ondios */
#define HW_MASK (*(short *)0x020000fc)
#endif

#ifdef CONFIG_TUNER_MULTI
static inline int tuner_detect_type(void)
{
    return (HW_MASK & TUNER_MODEL) ? TEA5767 : S1A0903X01;
}
#endif

#endif /* (CONFIG_CPU == SH7034) && !SIMULATOR */

#ifdef ARCHOS_PLAYER
bool is_new_player(void);
#endif

#ifdef IPOD_ARCH
#ifdef IPOD_VIDEO
#ifdef BOOTLOADER
#define IPOD_HW_REVISION (*((unsigned long*)(0x0000405c)))
#else  /* ROM is remapped */
#define IPOD_HW_REVISION (*((unsigned long*)(0x2000405c)))
#endif
#else /* !IPOD_VIDEO */
#ifdef BOOTLOADER
#define IPOD_HW_REVISION (*((unsigned long*)(0x00002084)))
#else  /* ROM is remapped */
#define IPOD_HW_REVISION (*((unsigned long*)(0x20002084)))
#endif
#endif /* !IPOD_VIDEO */
#endif /* IPOD_ARCH */

#endif /* HWCOMPAT_H */
