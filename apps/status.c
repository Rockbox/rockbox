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
#include "lcd.h"
#include "debug.h"
#include "kernel.h"
#include "power.h"
#include "thread.h"
#include "settings.h"
#include "status.h"
#include "mpeg.h"
static enum playmode current_mode;

void status_init(void)
{
    status_set_playmode(STATUS_STOP);
}

void status_set_playmode(enum playmode mode)
{
    current_mode = mode;
}

void status_draw(void)
{
#if defined(HAVE_LCD_CHARCELLS) && !defined(SIMULATOR)
    int battlevel = battery_level();
    int volume = mpeg_val2phys(SOUND_VOLUME, global_settings.volume);
    
    lcd_icon(ICON_BATTERY, true);
    if(battlevel > 25)
        lcd_icon(ICON_BATTERY_1, true);
    else
        lcd_icon(ICON_BATTERY_1, false);
    if(battlevel > 50)
        lcd_icon(ICON_BATTERY_2, true);
    else
        lcd_icon(ICON_BATTERY_2, false);
    if(battlevel > 75)
        lcd_icon(ICON_BATTERY_3, true);
    else
        lcd_icon(ICON_BATTERY_3, false);


    DEBUGF("vol: %d\n", volume);
    lcd_icon(ICON_VOLUME, true);
    if(volume > 10)
        lcd_icon(ICON_VOLUME_1, true);
    else
        lcd_icon(ICON_VOLUME_1, false);
    if(volume > 30)
        lcd_icon(ICON_VOLUME_2, true);
    else
        lcd_icon(ICON_VOLUME_2, false);
    if(volume > 50)
        lcd_icon(ICON_VOLUME_3, true);
    else
        lcd_icon(ICON_VOLUME_3, false);
    if(volume > 70)
        lcd_icon(ICON_VOLUME_4, true);
    else
        lcd_icon(ICON_VOLUME_4, false);
    if(volume > 90)
        lcd_icon(ICON_VOLUME_5, true);
    else
        lcd_icon(ICON_VOLUME_5, false);

    switch(current_mode)
    {
        case STATUS_PLAY:
            lcd_icon(ICON_PLAY, true);
            lcd_icon(ICON_PAUSE, false);
            break;
    
        case STATUS_STOP:
            lcd_icon(ICON_PLAY, false);
            lcd_icon(ICON_PAUSE, false);
            break;
    
        case STATUS_PAUSE:
            lcd_icon(ICON_PLAY, false);
            lcd_icon(ICON_PAUSE, true);
            break;
    }
#endif
}
