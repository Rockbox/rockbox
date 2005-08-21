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
#include "string.h"
#include "lcd.h"
#include "debug.h"
#include "kernel.h"
#include "power.h"
#include "thread.h"
#include "settings.h"
#include "status.h"
#include "mp3_playback.h"
#include "audio.h"
#include "wps.h"
#include "abrepeat.h"
#ifdef HAVE_RTC
#include "timefuncs.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "font.h"
#endif
#include "powermgmt.h"
#include "led.h"
#include "sound.h"
#if CONFIG_KEYPAD == IRIVER_H100_PAD
#include "button.h"
#endif
#include "usb.h"

static enum playmode ff_mode;

static long switch_tick;
static bool battery_state = true;
#ifdef HAVE_CHARGING
static int  battery_charge_step = 0;
#endif

struct status_info {
    int battlevel;
    int volume;
    int hour;
    int minute;
    int playmode;
    int repeat;
    bool inserted;
    bool shuffle;
    bool keylock;
    bool battery_safe;
    bool redraw_volume; /* true if the volume gauge needs updating */
#if CONFIG_LED == LED_VIRTUAL
    bool led; /* disk LED simulation in the status bar */
#endif
#ifdef HAVE_USB_POWER
    bool usb_power;
#endif

};

void status_init(void)
{
    ff_mode = 0;
}

void status_set_ffmode(enum playmode mode)
{
    ff_mode = mode; /* Either STATUS_FASTFORWARD or STATUS_FASTBACKWARD */
    status_draw(false);
}

enum playmode status_get_ffmode(void)
{
    /* only use this function for STATUS_FASTFORWARD or STATUS_FASTBACKWARD */
    /* use audio_status() for other modes */
    return ff_mode;
}

int current_playmode(void)
{
    int audio_stat = audio_status();

    /* ff_mode can be either STATUS_FASTFORWARD or STATUS_FASTBACKWARD
       and that supercedes the other modes */
    if(ff_mode)
        return ff_mode;
    
    if(audio_stat & AUDIO_STATUS_PLAY)
    {
        if(audio_stat & AUDIO_STATUS_PAUSE)
            return STATUS_PAUSE;
        else
            return STATUS_PLAY;
    }
#if CONFIG_HWCODEC == MAS3587F
    else
    {
        if(audio_stat & AUDIO_STATUS_RECORD)
        {
            if(audio_stat & AUDIO_STATUS_PAUSE)
                return STATUS_RECORD_PAUSE;
            else
                return STATUS_RECORD;
        }
    }
#endif
    
    return STATUS_STOP;
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

void status_draw(bool force_redraw)
{
    struct status_info info;

#ifdef HAVE_LCD_BITMAP
    static struct status_info lastinfo;
    struct tm* tm;

    if ( !global_settings.statusbar )
        return;
#else
    (void)force_redraw; /* players always "redraw" */
#endif

    info.volume = sound_val2phys(SOUND_VOLUME, global_settings.volume);
    info.inserted = charger_inserted();
    info.battlevel = battery_level();
    info.battery_safe = battery_level_safe();

#ifdef HAVE_LCD_BITMAP
    tm = get_time();
    info.hour = tm->tm_hour;
    info.minute = tm->tm_min;
    info.shuffle = global_settings.playlist_shuffle;
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    info.keylock = button_hold();
#else    
    info.keylock = keys_locked;
#endif
    info.repeat = global_settings.repeat_mode;
    info.playmode = current_playmode();
#if CONFIG_LED == LED_VIRTUAL
    info.led = led_read(HZ/2); /* delay should match polling interval */
#endif
#ifdef HAVE_USB_POWER
    info.usb_power = usb_powered();
#endif

    /* only redraw if forced to, or info has changed */
    if (force_redraw ||
        info.inserted ||
        !info.battery_safe ||
        info.redraw_volume ||
        memcmp(&info, &lastinfo, sizeof(struct status_info)))
    {
        lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        lcd_fillrect(0,0,LCD_WIDTH,8);
        lcd_set_drawmode(DRMODE_SOLID);
#else
    /* players always "redraw" */
    {
#endif

#ifdef HAVE_CHARGING
        if (info.inserted) {
            battery_state = true;
#if defined(HAVE_CHARGE_CTRL) || CONFIG_BATTERY == BATT_LIION2200
            /* zero battery run time if charging */
            if (charge_state > 0) {
                global_settings.runtime = 0;
                lasttime = current_tick;
            }
            
            /* animate battery if charging */
            if ((charge_state == 1) ||
                (charge_state == 2)) {
#else
            global_settings.runtime = 0;
            lasttime = current_tick;
            {
#endif
                /* animate in three steps (34% per step for a better look) */
                info.battlevel = battery_charge_step * 34;
                if (info.battlevel > 100)
                    info.battlevel = 100;
                if(TIME_AFTER(current_tick, switch_tick)) {
                    battery_charge_step=(battery_charge_step+1)%4;
                    switch_tick = current_tick + HZ;
                }
            }
        }
        else
#endif /* HAVE_CHARGING */
        {
            if (info.battery_safe)
                battery_state = true;
            else {
                /* blink battery if level is low */
                if(TIME_AFTER(current_tick, switch_tick) &&
                   (info.battlevel > -1)) {
                    switch_tick = current_tick+HZ;
                    battery_state =! battery_state;
                }
            }
        }

#ifdef HAVE_LCD_BITMAP
        if (battery_state)
            statusbar_icon_battery(info.battlevel);

        /* draw power plug if charging */
        if (info.inserted)
            lcd_mono_bitmap(bitmap_icons_7x8[Icon_Plug], ICON_PLUG_X_POS,
                            STATUSBAR_Y_POS, ICON_PLUG_WIDTH, STATUSBAR_HEIGHT);
#ifdef HAVE_USB_POWER
        else if (info.usb_power)
            lcd_mono_bitmap(bitmap_icons_7x8[Icon_USBPlug], ICON_PLUG_X_POS,
                            STATUSBAR_Y_POS, ICON_PLUG_WIDTH, STATUSBAR_HEIGHT);
#endif

        info.redraw_volume = statusbar_icon_volume(info.volume);
        statusbar_icon_play_state(current_playmode() + Icon_Play);
        switch (info.repeat) {
#ifdef AB_REPEAT_ENABLE
            case REPEAT_AB:
                statusbar_icon_play_mode(Icon_RepeatAB);
                break;
#endif

            case REPEAT_ONE:
                statusbar_icon_play_mode(Icon_RepeatOne);
                break;

            case REPEAT_ALL:
            case REPEAT_SHUFFLE:
                statusbar_icon_play_mode(Icon_Repeat);
                break;
        }
        if (info.shuffle)
            statusbar_icon_shuffle();
        if (info.keylock)
            statusbar_icon_lock();
#ifdef HAVE_RTC
        statusbar_time(info.hour, info.minute);
#endif
#if CONFIG_LED == LED_VIRTUAL
        if (info.led)
            statusbar_led();
#endif
        lcd_update_rect(0, 0, LCD_WIDTH, STATUSBAR_HEIGHT);
        lastinfo = info;
#endif
    }


#if defined(HAVE_LCD_CHARCELLS)
    if (info.battlevel > -1)
    lcd_icon(ICON_BATTERY, battery_state);
    lcd_icon(ICON_BATTERY_1, info.battlevel > 25);
    lcd_icon(ICON_BATTERY_2, info.battlevel > 50);
    lcd_icon(ICON_BATTERY_3, info.battlevel > 75);

    lcd_icon(ICON_VOLUME, true);
    lcd_icon(ICON_VOLUME_1, info.volume > 10);
    lcd_icon(ICON_VOLUME_2, info.volume > 30);
    lcd_icon(ICON_VOLUME_3, info.volume > 50);
    lcd_icon(ICON_VOLUME_4, info.volume > 70);
    lcd_icon(ICON_VOLUME_5, info.volume > 90);

    lcd_icon(ICON_PLAY, current_playmode() == STATUS_PLAY);
    lcd_icon(ICON_PAUSE, current_playmode() == STATUS_PAUSE);

    lcd_icon(ICON_REPEAT, global_settings.repeat_mode != REPEAT_OFF);
    lcd_icon(ICON_1, global_settings.repeat_mode == REPEAT_ONE);
    
    lcd_icon(ICON_RECORD, record);
    lcd_icon(ICON_AUDIO, audio);
    lcd_icon(ICON_PARAM, param);
    lcd_icon(ICON_USB, usb);
#endif

}

#if defined(HAVE_LCD_BITMAP) && (CONFIG_KEYPAD == RECORDER_PAD)
static void draw_buttonbar_btn(int num, const char* caption)
{
    int xpos, ypos, button_width, text_width;
    int fw, fh;

    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize("M", &fw, &fh);

    button_width = LCD_WIDTH/3;
    xpos = num * button_width;
    ypos = LCD_HEIGHT - fh;
    
    if(caption)
    {
        /* center the text */
        text_width = fw * strlen(caption);
        lcd_putsxy(xpos + (button_width - text_width)/2, ypos, caption);
    }
    
    lcd_set_drawmode(DRMODE_COMPLEMENT);
    lcd_fillrect(xpos, ypos, button_width - 1, fh);
    lcd_set_drawmode(DRMODE_SOLID);
}

static char stored_caption1[8];
static char stored_caption2[8];
static char stored_caption3[8];

void buttonbar_set(const char* caption1, const char *caption2,
                   const char *caption3)
{
    buttonbar_unset();
    if(caption1)
    {
        strncpy(stored_caption1, caption1, 7);
        stored_caption1[7] = 0;
    }
    if(caption2)
    {
        strncpy(stored_caption2, caption2, 7);
        stored_caption2[7] = 0;
    }
    if(caption3)
    {
        strncpy(stored_caption3, caption3, 7);
        stored_caption3[7] = 0;
    }
}

void buttonbar_unset(void)
{
    stored_caption1[0] = 0;
    stored_caption2[0] = 0;
    stored_caption3[0] = 0;
}

void buttonbar_draw(void)
{
    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    lcd_fillrect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
    lcd_set_drawmode(DRMODE_SOLID);
    draw_buttonbar_btn(0, stored_caption1);
    draw_buttonbar_btn(1, stored_caption2);
    draw_buttonbar_btn(2, stored_caption3);
}

bool buttonbar_isset(void)
{
    /* If all buttons are unset, the button bar is considered disabled */
    return (global_settings.buttonbar &&
            ((stored_caption1[0] != 0) ||
             (stored_caption2[0] != 0) ||
             (stored_caption3[0] != 0)));
}

#endif
