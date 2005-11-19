/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Robert E. Hak (2002), Linus Nielsen Feltzing (2002)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "screen_access.h"
#include "lcd.h"
#include "font.h"
#include "kernel.h"
#include "string.h" /* for memcmp oO*/
#include "sprintf.h"
#include "sound.h"
#include "power.h"
#include "settings.h"
#include "icons.h"
#include "powermgmt.h"
#include "button.h"
#include "usb.h"
#include "led.h"

#include "status.h" /* needed for battery_state global var */
#include "gwps.h" /* for keys_locked */
#include "statusbar.h"


/* FIXME: should be removed from icon.h to avoid redefinition,
   but still needed for compatibility with old system */

#define STATUSBAR_BATTERY_X_POS                 0
#define STATUSBAR_BATTERY_WIDTH                 18
#define STATUSBAR_PLUG_X_POS                    STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH +2
#define STATUSBAR_PLUG_WIDTH                    7
#define STATUSBAR_VOLUME_X_POS                  STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH +2+2
#define STATUSBAR_VOLUME_WIDTH                  16
#define STATUSBAR_PLAY_STATE_X_POS              STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH+2+2+2
#define STATUSBAR_PLAY_STATE_WIDTH              7
#define STATUSBAR_PLAY_MODE_X_POS               STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                2+2+2+2
#define STATUSBAR_PLAY_MODE_WIDTH               7
#define STATUSBAR_SHUFFLE_X_POS                 STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                STATUSBAR_PLAY_MODE_WIDTH + \
                                                2+2+2+2+2
#define STATUSBAR_SHUFFLE_WIDTH                 7
#define STATUSBAR_LOCK_X_POS                    STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                STATUSBAR_PLAY_MODE_WIDTH + \
                                                STATUSBAR_SHUFFLE_WIDTH + \
                                                2+2+2+2+2+2
#define STATUSBAR_LOCK_WIDTH                    5
#define STATUSBAR_DISK_WIDTH                    12
#define STATUSBAR_DISK_X_POS(statusbar_width)   statusbar_width - \
                                                STATUSBAR_DISK_WIDTH
#define STATUSBAR_TIME_X_END(statusbar_width)   statusbar_width-1

struct gui_syncstatusbar statusbars;

void gui_statusbar_init(struct gui_statusbar * bar)
{
    bar->last_volume = -1; /* -1 means "first update ever" */
    bar->battery_icon_switch_tick = 0;
#ifdef HAVE_CHARGING
    bar->battery_charge_step = 0;
#endif
}

void gui_statusbar_set_screen(struct gui_statusbar * bar,
                              struct screen * display)
{
    bar->display = display;
    gui_statusbar_draw(bar, false);
}

void gui_statusbar_draw(struct gui_statusbar * bar, bool force_redraw)
{
#ifdef HAVE_LCD_BITMAP
    if(!global_settings.statusbar)
       return;
#endif

    struct screen * display = bar->display;

#ifdef HAVE_RTC
    struct tm* tm; /* For Time */
#endif

#ifdef HAVE_LCD_CHARCELLS
    (void)force_redraw; /* players always "redraw" */
#endif

    bar->info.volume = sound_val2phys(SOUND_VOLUME, global_settings.volume);
    bar->info.inserted = charger_inserted();
    bar->info.battlevel = battery_level();
    bar->info.battery_safe = battery_level_safe();

#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_RTC
    tm = get_time();
    bar->info.hour = tm->tm_hour;
    bar->info.minute = tm->tm_min;
#endif

    bar->info.shuffle = global_settings.playlist_shuffle;
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    bar->info.keylock = button_hold();
#else
    bar->info.keylock = keys_locked;
#endif
    bar->info.repeat = global_settings.repeat_mode;
    bar->info.playmode = current_playmode();
#if CONFIG_LED == LED_VIRTUAL
    bar->info.led = led_read(HZ/2); /* delay should match polling interval */
#endif
#ifdef HAVE_USB_POWER
    bar->info.usb_power = usb_powered();
#endif

    /* only redraw if forced to, or info has changed */
    if (force_redraw ||
        bar->info.inserted ||
        !bar->info.battery_safe ||
        bar->info.redraw_volume ||
        memcmp(&(bar->info), &(bar->lastinfo), sizeof(struct status_info)))
    {
        display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        display->fillrect(0,0,display->width,8);
        display->set_drawmode(DRMODE_SOLID);

#else

    /* players always "redraw" */
    {
#endif

#ifdef HAVE_CHARGING
        if (bar->info.inserted) {
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
                bar->info.battlevel = bar->battery_charge_step * 34;
                if (bar->info.battlevel > 100)
                    bar->info.battlevel = 100;
                if(TIME_AFTER(current_tick, bar->battery_icon_switch_tick)) {
                    bar->battery_charge_step=(bar->battery_charge_step+1)%4;
                    bar->battery_icon_switch_tick = current_tick + HZ;
                }
            }
        }
        else
#endif /* HAVE_CHARGING */
        {
            if (bar->info.battery_safe)
                battery_state = true;
            else {
                /* blink battery if level is low */
                if(TIME_AFTER(current_tick, bar->battery_icon_switch_tick) &&
                   (bar->info.battlevel > -1)) {
                    bar->battery_icon_switch_tick = current_tick+HZ;
                    battery_state = !battery_state;
                }
            }
        }
#ifdef HAVE_LCD_BITMAP
        if (battery_state)
            gui_statusbar_icon_battery(display, bar->info.battlevel);
        /* draw power plug if charging */
        if (bar->info.inserted)
            display->mono_bitmap(bitmap_icons_7x8[Icon_Plug],
                                    STATUSBAR_PLUG_X_POS,
                                    STATUSBAR_Y_POS, STATUSBAR_PLUG_WIDTH,
                                    STATUSBAR_HEIGHT);
#ifdef HAVE_USB_POWER
        else if (bar->info.usb_power)
            display->mono_bitmap(bitmap_icons_7x8[Icon_USBPlug],
                                 STATUSBAR_PLUG_X_POS,
                                 STATUSBAR_Y_POS, STATUSBAR_PLUG_WIDTH,
                                 STATUSBAR_HEIGHT);
#endif

        bar->info.redraw_volume = gui_statusbar_icon_volume(bar,
                                                bar->info.volume);
        gui_statusbar_icon_play_state(display, current_playmode() +
                                                Icon_Play);

        switch (bar->info.repeat) {
#ifdef AB_REPEAT_ENABLE
            case REPEAT_AB:
                gui_statusbar_icon_play_mode(display, Icon_RepeatAB);
                break;
#endif

            case REPEAT_ONE:
                gui_statusbar_icon_play_mode(display, Icon_RepeatOne);
                break;

            case REPEAT_ALL:
            case REPEAT_SHUFFLE:
                gui_statusbar_icon_play_mode(display, Icon_Repeat);
                break;
        }
        if (bar->info.shuffle)
            gui_statusbar_icon_shuffle(display);
        if (bar->info.keylock)
            gui_statusbar_icon_lock(display);
#ifdef HAVE_RTC
        gui_statusbar_time(display, bar->info.hour, bar->info.minute);
#endif
#if CONFIG_LED == LED_VIRTUAL
        if (bar->info.led)
            statusbar_led();
#endif
        display->update_rect(0, 0, display->width, STATUSBAR_HEIGHT);
        bar->lastinfo = bar->info;
#endif
    }


#ifdef HAVE_LCD_CHARCELLS
    if (bar->info.battlevel > -1)
        display->icon(ICON_BATTERY, battery_state);
    display->icon(ICON_BATTERY_1, bar->info.battlevel > 25);
    display->icon(ICON_BATTERY_2, bar->info.battlevel > 50);
    display->icon(ICON_BATTERY_3, bar->info.battlevel > 75);

    display->icon(ICON_VOLUME, true);
    display->icon(ICON_VOLUME_1, bar->info.volume > 10);
    display->icon(ICON_VOLUME_2, bar->info.volume > 30);
    display->icon(ICON_VOLUME_3, bar->info.volume > 50);
    display->icon(ICON_VOLUME_4, bar->info.volume > 70);
    display->icon(ICON_VOLUME_5, bar->info.volume > 90);

    display->icon(ICON_PLAY, current_playmode() == STATUS_PLAY);
    display->icon(ICON_PAUSE, current_playmode() == STATUS_PAUSE);

    display->icon(ICON_REPEAT, global_settings.repeat_mode != REPEAT_OFF);
    display->icon(ICON_1, global_settings.repeat_mode == REPEAT_ONE);

    display->icon(ICON_RECORD, record);
    display->icon(ICON_AUDIO, audio);
    display->icon(ICON_PARAM, param);
    display->icon(ICON_USB, usb);
#endif
}

#ifdef HAVE_LCD_BITMAP
/* from icon.c */
/*
 * Print battery icon to status bar
 */
void gui_statusbar_icon_battery(struct screen * display, int percent)
{
    int fill;
    char buffer[5];
    unsigned int width, height;

    /* fill battery */
    fill = percent;
    if (fill < 0)
        fill = 0;
    if (fill > 100)
        fill = 100;

#if defined(HAVE_CHARGE_CTRL) && !defined(SIMULATOR) /* Rec v1 target only */
    /* show graphical animation when charging instead of numbers */
    if ((global_settings.battery_display) &&
        (charge_state != 1) &&
        (percent > -1)) {
#else /* all others */
    if (global_settings.battery_display && (percent > -1)) {
#endif
        /* Numeric display */
        display->setfont(FONT_SYSFIXED);
        snprintf(buffer, sizeof(buffer), "%3d", fill);
        display->getstringsize(buffer, &width, &height);
        if (height <= STATUSBAR_HEIGHT)
            display->putsxy(STATUSBAR_BATTERY_X_POS
                             + STATUSBAR_BATTERY_WIDTH / 2
                             - width/2, STATUSBAR_Y_POS, buffer);
        display->setfont(FONT_UI);

    }
    else {
        /* draw battery */
        display->drawrect(STATUSBAR_BATTERY_X_POS, STATUSBAR_Y_POS, 17, 7);
        display->vline(STATUSBAR_BATTERY_X_POS + 17, STATUSBAR_Y_POS + 2,
                       STATUSBAR_Y_POS + 4);

        fill = fill * 15 / 100;
        display->fillrect(STATUSBAR_BATTERY_X_POS + 1, STATUSBAR_Y_POS + 1,
                          fill, 5);
    }

    if (percent == -1) {
        display->setfont(FONT_SYSFIXED);
        display->putsxy(STATUSBAR_BATTERY_X_POS + STATUSBAR_BATTERY_WIDTH / 2
                         - 4, STATUSBAR_Y_POS, "?");
        display->setfont(FONT_UI);
    }
}

/*
 * Print volume gauge to status bar
 */
bool gui_statusbar_icon_volume(struct gui_statusbar * bar, int percent)
{
    int i;
    int volume;
    int vol;
    char buffer[4];
    unsigned int width, height;
    bool needs_redraw = false;
    int type = global_settings.volume_type;
    struct screen * display=bar->display;

    volume = percent;
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;

    if (volume == 0) {
        display->mono_bitmap(bitmap_icons_7x8[Icon_Mute],
                    STATUSBAR_VOLUME_X_POS + STATUSBAR_VOLUME_WIDTH / 2 - 4,
                    STATUSBAR_Y_POS, 7, STATUSBAR_HEIGHT);
    }
    else {
        /* We want to redraw the icon later on */
        if (bar->last_volume != volume && bar->last_volume >= 0) {
            bar->volume_icon_switch_tick = current_tick + HZ;
        }

        /* If the timeout hasn't yet been reached, we show it numerically
           and tell the caller that we want to be called again */
        if (TIME_BEFORE(current_tick,bar->volume_icon_switch_tick)) {
            type = 1;
            needs_redraw = true;
        }

        /* display volume level numerical? */
        if (type)
        {
            display->setfont(FONT_SYSFIXED);
            snprintf(buffer, sizeof(buffer), "%2d", percent);
            display->getstringsize(buffer, &width, &height);
            if (height <= STATUSBAR_HEIGHT)
            {
                display->putsxy(STATUSBAR_VOLUME_X_POS
                                 + STATUSBAR_VOLUME_WIDTH / 2
                                 - width/2, STATUSBAR_Y_POS, buffer);
            }
            display->setfont(FONT_UI);
        } else {
            /* display volume bar */
            vol = volume * 14 / 100;
            for(i=0; i < vol; i++) {
                display->vline(STATUSBAR_VOLUME_X_POS + i,
                               STATUSBAR_Y_POS + 6 - i / 2,
                               STATUSBAR_Y_POS + 6);
            }
        }
    }
    bar->last_volume = volume;

    return needs_redraw;
}

/*
 * Print play state to status bar
 */
void gui_statusbar_icon_play_state(struct screen * display, int state)
{
    display->mono_bitmap(bitmap_icons_7x8[state], STATUSBAR_PLAY_STATE_X_POS,
                    STATUSBAR_Y_POS, STATUSBAR_PLAY_STATE_WIDTH,
                    STATUSBAR_HEIGHT);
}

/*
 * Print play mode to status bar
 */
void gui_statusbar_icon_play_mode(struct screen * display, int mode)
{
    display->mono_bitmap(bitmap_icons_7x8[mode], STATUSBAR_PLAY_MODE_X_POS,
                    STATUSBAR_Y_POS, STATUSBAR_PLAY_MODE_WIDTH,
                    STATUSBAR_HEIGHT);
}

/*
 * Print shuffle mode to status bar
 */
void gui_statusbar_icon_shuffle(struct screen * display)
{
    display->mono_bitmap(bitmap_icons_7x8[Icon_Shuffle],
                    STATUSBAR_SHUFFLE_X_POS, STATUSBAR_Y_POS,
                    STATUSBAR_SHUFFLE_WIDTH, STATUSBAR_HEIGHT);
}

/*
 * Print lock when keys are locked
 */
void gui_statusbar_icon_lock(struct screen * display)
{
    display->mono_bitmap(bitmap_icons_5x8[Icon_Lock], STATUSBAR_LOCK_X_POS,
                    STATUSBAR_Y_POS, 5, 8);
}

#if CONFIG_LED == LED_VIRTUAL
/*
 * no real LED: disk activity in status bar
 */
void gui_statusbar_led(struct screen * display)
{
    display->mono_bitmap(bitmap_icon_disk,
                         STATUSBAR_DISK_X_POS(display->width),
                         STATUSBAR_Y_POS, STATUSBAR_DISK_WIDTH,
                         STATUSBAR_HEIGHT);
}
#endif


#ifdef HAVE_RTC
/*
 * Print time to status bar
 */
void gui_statusbar_time(struct screen * display, int hour, int minute)
{
    unsigned char buffer[6];
    unsigned int width, height;
    if ( hour >= 0 &&
         hour <= 23 &&
         minute >= 0 &&
         minute <= 59 ) {
        if ( global_settings.timeformat ) { /* 12 hour clock */
            hour %= 12;
            if ( hour == 0 ) {
                hour += 12;
            }
        }
        snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
    }
    else {
        strncpy(buffer, "--:--", sizeof buffer);
    }
    display->setfont(FONT_SYSFIXED);
    display->getstringsize(buffer, &width, &height);
    if (height <= STATUSBAR_HEIGHT) {
        display->putsxy(STATUSBAR_TIME_X_END(display->width) - width,
                        STATUSBAR_Y_POS, buffer);
    }
    display->setfont(FONT_UI);

}
#endif

#endif /* HAVE_LCD_BITMAP */

void gui_syncstatusbar_init(struct gui_syncstatusbar * bars)
{
    int i;
    FOR_NB_SCREENS(i) {
        gui_statusbar_init( &(bars->statusbars[i]) );
        gui_statusbar_set_screen( &(bars->statusbars[i]), &(screens[i]) );
    }
}

void gui_syncstatusbar_draw(struct gui_syncstatusbar * bars,
                            bool force_redraw)
{
    int i;
    FOR_NB_SCREENS(i) {
        gui_statusbar_draw( &(bars->statusbars[i]), force_redraw );
    }
}
