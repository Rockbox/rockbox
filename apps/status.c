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
#include "wps.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif
#include "powermgmt.h"

static enum playmode current_mode = STATUS_STOP;

#ifdef HAVE_LCD_BITMAP
bool statusbar_enabled = true;
long switch_tick;
bool plug_state;
bool battery_state;
int  battery_charge_step = 0;
#endif

void status_init(void)
{
    status_set_playmode(STATUS_STOP);
}

void status_set_playmode(enum playmode mode)
{
    current_mode = mode;
}

#ifdef HAVE_LCD_BITMAP
bool statusbar(bool state)
{
    bool laststate = statusbar_enabled;

    statusbar_enabled = state;

    return(laststate);
}
#endif

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
#ifdef HAVE_LCD_BITMAP
    int battlevel = battery_level();
    int volume = mpeg_val2phys(SOUND_VOLUME, global_settings.volume);

    if(global_settings.statusbar && statusbar_enabled) {
        statusbar_wipe();
#ifdef HAVE_CHARGE_CTRL
        if(charger_inserted()) {
            battery_state = true;
            plug_state = true;
            if(charger_enabled) { /* animate battery if charging */
                battlevel = battery_charge_step * 34; /* 34 for a better look */
                battlevel = battlevel > 100 ? 100 : battlevel;
                if(TIME_AFTER(current_tick, switch_tick)) {
                    battery_charge_step=(battery_charge_step+1)%4;
                    switch_tick = current_tick + HZ;
                }
            }
        }
        else {
            plug_state=false;
            if(battery_level_safe())
                battery_state = true;
            else /* blink battery if level is low */
                if(TIME_AFTER(current_tick, switch_tick)) {
                    switch_tick = current_tick+HZ;
                    battery_state =! battery_state;
                }
        }
                
        if(battery_state) statusbar_icon_battery(battlevel, plug_state);
#else
        statusbar_icon_battery(battlevel, false);
#endif
        statusbar_icon_volume(volume);
        statusbar_icon_play_state(current_mode + Icon_Play);
        if (global_settings.loop_playlist)
            statusbar_icon_play_mode(Icon_Repeat);
        else
            statusbar_icon_play_mode(Icon_Normal);
        if(global_settings.playlist_shuffle)
            statusbar_icon_shuffle();
        if (keys_locked)
            statusbar_icon_lock();
#ifdef HAVE_RTC
        statusbar_time();
#endif
#ifdef SIMULATOR
        lcd_update();
#else
        lcd_update_rect(0, 0, LCD_WIDTH, STATUSBAR_HEIGHT);
#endif
    }
#endif
}
