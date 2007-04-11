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
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "logf.h"
#include "pcf50605.h"
#include "usb.h"
#include "lcd.h"

void power_init(void)
{
    pcf50605_init();
}

bool charger_inserted(void)
{
#ifdef IPOD_VIDEO
    return (GPIOL_INPUT_VAL & 0x08)?false:true;
#else
    /* This needs filling in for other ipods. */
    return false;
#endif
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
    return (GPIOB_INPUT_VAL & 0x01)?false:true;
}


void ide_power_enable(bool on)
{
    /* We do nothing on the iPod */
    (void)on;
}

bool ide_powered(void)
{
    /* pretend we are always powered - we don't turn it off on the ipod */
    return true;
}

void power_off(void)
{
#if defined(HAVE_LCD_COLOR)
    /* Clear the screen and backdrop to
    remove ghosting effect on shutdown */
    lcd_set_backdrop(NULL);
    lcd_set_background(LCD_WHITE);
    lcd_clear_display();
    lcd_update();
    sleep(HZ/16);
#endif

#ifndef BOOTLOADER
    /* We don't turn off the ipod, we put it in a deep sleep */
    pcf50605_standby_mode();
#endif
}
