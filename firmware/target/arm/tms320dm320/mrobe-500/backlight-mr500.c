/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "power.h"
#include "spi-target.h"

void __backlight_on(void)
{
    __backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
}

void __backlight_off(void)
{
    __backlight_set_brightness(0);
}

/* Assumes that the backlight has been initialized */
void __backlight_set_brightness(int brightness)
{
    uint8_t bl_command[] = {0xa4, 0x00, brightness, 0xbb};
    spi_block_transfer(SPI_target_BACKLIGHT, bl_command, 4, 0, 0);
}

void __backlight_dim(bool dim_now)
{
    __backlight_set_brightness(dim_now ?
        DEFAULT_BRIGHTNESS_SETTING :
        DEFAULT_DIMNESS_SETTING);
}

bool __backlight_init(void)
{
    __backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}
