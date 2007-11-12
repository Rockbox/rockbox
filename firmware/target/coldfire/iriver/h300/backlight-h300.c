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
#include "pcf50606.h"
#include "lcd.h"

bool _backlight_init(void)
{
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    or_l(0x00020000, &GPIO1_OUT);  /* Start with the backlight ON */
    
    return true; /* Backlight always ON after boot. */
}

void _backlight_on(void)
{
    lcd_enable(true);
    sleep(HZ/100); /* lcd needs time - avoid flashing for dark screens */
    or_l(0x00020000, &GPIO1_OUT);
}

void _backlight_off(void)
{
    and_l(~0x00020000, &GPIO1_OUT);
    lcd_enable(false);
}

/* set brightness by changing the PWM */
void _backlight_set_brightness(int val)
{
    /* disable IRQs while bitbanging */
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x35, (val << 1) | 0x01); /* 512Hz, Enable PWM */
    /* enable IRQs again */
    set_irq_level(old_irq_level);
}

void _remote_backlight_on(void)
{
    and_l(~0x00000002, &GPIO1_OUT);
}

void _remote_backlight_off(void)
{
    or_l(0x00000002, &GPIO1_OUT);
}
