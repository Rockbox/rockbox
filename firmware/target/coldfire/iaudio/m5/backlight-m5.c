/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"
#include "pcf50606.h"
#include "lcd.h"

bool _backlight_init(void)
{
    _backlight_on();

    return true; /* Backlight always ON after boot. */
}

void _backlight_on(void)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);

    pcf50606_write(0x39, 0x07);
    set_irq_level(level);
}

void _backlight_off(void)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);

    pcf50606_write(0x39, 0x00);
    set_irq_level(level);
}

void _remote_backlight_on(void)
{
    and_l(~0x00200000, &GPIO_OUT);
}

void _remote_backlight_off(void)
{
    or_l(0x00200000, &GPIO_OUT);
}
