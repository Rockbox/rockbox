/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "config.h"
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "ascodec.h"
#include "as3514.h"

int buttonlight_is_on = 0;

void backlight_hw_brightness(int brightness)
{
    ascodec_write(AS3514_DCDC15, brightness);
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
#if (CONFIG_BACKLIGHT_FADING != BACKLIGHT_FADING_SW_SETTING) /* in bootloader/sim */
    /* if we set the brightness to the settings value, then fading up
     * is glitchy */
    ascodec_write(AS3514_DCDC15, backlight_brightness);
#endif
}

void backlight_hw_off(void)
{
    ascodec_write(AS3514_DCDC15, 0x0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

void buttonlight_hw_on(void)
{
    /* Needed for buttonlight and MicroSD to work at the same time */
    /* Turn ROD control on, as the OF does */
    GPIOD_DIR |= (1<<7);
    SD_MCI_POWER |= (1<<7);
    GPIOD_PIN(7) = (1<<7);
    buttonlight_is_on = 1;
}

void buttonlight_hw_off(void)
{
    /* Needed for buttonlight and MicroSD to work at the same time */
    /* Turn ROD control off, as the OF does */
    SD_MCI_POWER &= ~(1<<7);
    GPIOD_PIN(7) = 0;
    GPIOD_DIR &= ~(1<<7);
    buttonlight_is_on = 0;
}
