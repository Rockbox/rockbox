/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

#include "config.h"
#include "cpu.h"

static inline bool _backlight_init(void)
{
    PACR1 &= ~0x3000;    /* Set PA14 (backlight control) to GPIO */
    and_b(~0x40, &PADRH); /* drive and set low */
    or_b(0x40, &PAIORH); /* ..and output */
    return true;
}

static inline void _backlight_on(void)
{
    and_b(~0x40, &PADRH); /* drive and set low */
    or_b(0x40, &PAIORH);
}

static inline void _backlight_off(void)
{
    and_b(~0x40, &PAIORH); /* let it float (up) */
}

#endif
