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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "backlight.h"
#include "lcd.h"

void __backlight_init(void)
{
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    and_l(~0x00020000, &GPIO1_OUT);  /* Start with the backlight ON */
}

void __backlight_on(void)
{
    and_l(~0x00020000, &GPIO1_OUT);
}

void __backlight_off(void)
{
    or_l(0x00020000, &GPIO1_OUT);
}

void __remote_backlight_on(void)
{
    and_l(~0x00000800, &GPIO_OUT);
}

void __remote_backlight_off(void)
{
    or_l(0x00000800, &GPIO_OUT);
}
