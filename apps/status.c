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
#ifdef HAVE_RTC
#include "timefuncs.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif
#include "powermgmt.h"

static enum playmode current_mode = STATUS_STOP;

#if defined(HAVE_LCD_CHARCELLS) || defined(HAVE_CHARGE_CTRL)
static long switch_tick;
static int  battery_charge_step = 0;
#ifdef HAVE_CHARGE_CTRL
static bool plug_state;
static bool battery_state;
#endif
#endif

void status_init(void)
{
    status_set_playmode(STATUS_STOP);
}

void status_set_playmode(enum playmode mode)
{
    current_mode = mode;
    status_draw();
}

#if defined(HAVE_LCD_CHARCELLS)
static bool record = false;
static bool audio = false;
static bool param = false;
static bool usb = false;

void status_set_record(bool b)
{
    record = b;
}

void status_set_audio(bool b)
{
    audio = b;
}

void status_set_param(bool b)
{
    param = b;
}

void status_set_usb(bool b)
{
    usb = b;
}

#endif /* HAVE_LCD_CHARCELLS */

void status_draw(void)
{
    int battlevel = battery_level();
    int volume = mpeg_val2phys(SOUND_VOLUME, global_settings.volume);
#if defined(HAVE_LCD_BITMAP) && defined(HAVE_RTC)
    struct tm* tm;
#endif

    if ( !global_settings.statusbar )
        return;

#if defined(HAVE_LCD_CHARCELLS)
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

        default:
            break;
    }
    if(charger_inserted())
    {
        global_settings.runtime = 0;
        if(TIME_AFTER(current_tick, switch_tick))
        {
            lcd_icon(ICON_BATTERY, true);
            lcd_icon(ICON_BATTERY_1, false);
            lcd_icon(ICON_BATTERY_2, false);
            lcd_icon(ICON_BATTERY_3, false);
            switch(battery_charge_step)
            {
                case 0:
                    battery_charge_step++;
                    break;
                case 1:
                    lcd_icon(ICON_BATTERY_1, true);
                    battery_charge_step++;
                    break;
                case 2:
                    lcd_icon(ICON_BATTERY_1, true);
                    lcd_icon(ICON_BATTERY_2, true);
                    battery_charge_step++;
                    break;
                case 3:
                    lcd_icon(ICON_BATTERY_1, true);
                    lcd_icon(ICON_BATTERY_2, true);
                    lcd_icon(ICON_BATTERY_3, true);
                    battery_charge_step = 0;
                    break;
                default:
                    battery_charge_step = 0;
                    break;
            }
           switch_tick = current_tick + (HZ/2);
       }
    } else {
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
    }

    lcd_icon(ICON_REPEAT, global_settings.repeat_mode != REPEAT_OFF);
    lcd_icon(ICON_1, global_settings.repeat_mode == REPEAT_ONE);
    
    lcd_icon(ICON_RECORD, record);
    lcd_icon(ICON_AUDIO, audio);
    lcd_icon(ICON_PARAM, param);
    lcd_icon(ICON_USB, usb);

#endif
#ifdef HAVE_LCD_BITMAP
    if (global_settings.statusbar) {
        statusbar_wipe();
#ifdef HAVE_CHARGE_CTRL /* Recorder */
        if(charger_inserted()) {
            battery_state = true;
            plug_state = true;
            if (charge_state > 0) /* charge || top off || trickle */
                global_settings.runtime = 0;
            if (charge_state == 1) { /* animate battery if charging */
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
                
        if (battery_state)
            statusbar_icon_battery(battlevel, plug_state);
#else
#ifdef HAVE_FMADC /* FM */
        statusbar_icon_battery(battlevel, charger_inserted());
#else /* Player */
        statusbar_icon_battery(battlevel, false);
#endif /* HAVE_FMADC */
#endif /* HAVE_CHARGE_CTRL */
        statusbar_icon_volume(volume);
        statusbar_icon_play_state(current_mode + Icon_Play);
        switch (global_settings.repeat_mode) {
            case REPEAT_ONE:
                statusbar_icon_play_mode(Icon_RepeatOne);
                break;

            case REPEAT_ALL:
                statusbar_icon_play_mode(Icon_Repeat);
                break;
        }
        if(global_settings.playlist_shuffle)
            statusbar_icon_shuffle();
        if (keys_locked)
            statusbar_icon_lock();
#ifdef HAVE_RTC
        tm = get_time();
        statusbar_time(tm->tm_hour, tm->tm_min);
#endif

        lcd_update_rect(0, 0, LCD_WIDTH, STATUSBAR_HEIGHT);
    }
#endif
}
