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
#include "pcf50606.h"
#include "lcd.h"

void __backlight_on(void)
{
    int level;
    lcd_enable(true);
    level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x38, 0xb0); /* Backlight ON, GPO1INV=1, GPO1ACT=011 */
    set_irq_level(level);
}

void __backlight_off(void)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x38, 0x80); /* Backlight OFF, GPO1INV=1, GPO1ACT=000 */
    set_irq_level(level);
    lcd_enable(false);
}

/* set brightness by changing the PWM */
void __backlight_set_brightness(int val)
{
    /* disable IRQs while bitbanging */
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);
    pcf50606_write(0x35, (val << 1) | 0x01); /* 512Hz, Enable PWM */
    /* enable IRQs again */
    set_irq_level(old_irq_level);
}

void __remote_backlight_on(void)
{
    and_l(~0x00200000, &GPIO_OUT);
}

void __remote_backlight_off(void)
{
    or_l(0x00200000, &GPIO_OUT);
}
