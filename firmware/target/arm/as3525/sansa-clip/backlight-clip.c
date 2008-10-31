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

#include "backlight-target.h"
#include "as3525.h"

/* TODO : backlight brightness */

/* XXX : xpd is used for SD/MCI interface
 * If interrupts are used to access this interface, they should be
 * disabled in _buttonlight_on/off ()
 */

void _buttonlight_on(void)
{
    int saved_ccu_io;

    saved_ccu_io = CCU_IO;  /* save XPD setting */

    CCU_IO &= ~(3<<2);      /* setup xpd as GPIO */

    GPIOD_DIR |= (1<<7);
    GPIOD_PIN(7) = (1<<7);  /* set pin d7 high */

    CCU_IO = saved_ccu_io;  /* restore the previous XPD setting */
}

void _buttonlight_off(void)
{
    int saved_ccu_io;

    saved_ccu_io = CCU_IO;  /* save XPD setting */

    CCU_IO &= ~(3<<2);      /* setup xpd as GPIO */

    GPIOD_DIR |= (1<<7);
    GPIOD_PIN(7) = 0;       /* set pin d7 low */

    CCU_IO = saved_ccu_io;  /* restore the previous XPD setting */
}
