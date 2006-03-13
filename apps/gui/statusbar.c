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
#define ICONS_SPACING                           2
#define STATUSBAR_BATTERY_X_POS                 0*ICONS_SPACING
#define STATUSBAR_BATTERY_WIDTH                 18
#define STATUSBAR_PLUG_X_POS                    STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                ICONS_SPACING
#define STATUSBAR_PLUG_WIDTH                    7
#define STATUSBAR_VOLUME_X_POS                  STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                2*ICONS_SPACING
#define STATUSBAR_VOLUME_WIDTH                  16
#define STATUSBAR_PLAY_STATE_X_POS              STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                3*ICONS_SPACING
#define STATUSBAR_PLAY_STATE_WIDTH              7
#define STATUSBAR_PLAY_MODE_X_POS               STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                4*ICONS_SPACING
#define STATUSBAR_PLAY_MODE_WIDTH               7
#define STATUSBAR_SHUFFLE_X_POS                 STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                STATUSBAR_PLAY_MODE_WIDTH + \
                                                5*ICONS_SPACING
#define STATUSBAR_SHUFFLE_WIDTH                 7
#define STATUSBAR_LOCKM_X_POS                   STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                STATUSBAR_PLAY_MODE_WIDTH + \
                                                STATUSBAR_SHUFFLE_WIDTH + \
                                                6*ICONS_SPACING
#define STATUSBAR_LOCKM_WIDTH                   5
#define STATUSBAR_LOCKR_X_POS                   STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                STATUSBAR_PLAY_MODE_WIDTH + \
                                                STATUSBAR_SHUFFLE_WIDTH + \
                                                STATUSBAR_LOCKM_WIDTH + \
                                                7*ICONS_SPACING
#define STATUSBAR_LOCKR_WIDTH                   5

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
#define STATUSBAR_DISK_WIDTH                    12
#define STATUSBAR_DISK_X_POS(statusbar_width)   statusbar_width - \
                                                STATUSBAR_DISK_WIDTH
#else
#define STATUSBAR_DISK_WIDTH                    0
#endif
#define STATUSBAR_TIME_X_END(statusbar_width)   statusbar_width - 1 - \
                                                STATUSBAR_DISK_WIDTH

struct gui_syncstatusbar statusbars;

void gui_statusbar_init(struct gui_statusbar * bar)
{
    bar->last_volume = -1000; /* -1000 means "first update ever" */
    bar->battery_icon_switch_tick = 0;
    bar->animated_level = 0;
}

void gui_statusbar_draw(struct gui_statusbar * bar, bool force_redraw)
{
    struct screen * display = bar->display;
#ifdef CONFIG_RTC
    struct tm* tm; /* For Time */
#endif /* CONFIG_RTC */

#ifdef HAVE_LCD_CHARCELLS
    int vol;
    (void)force_redraw; /* players always "redraw" */
#endif /* HAVE_LCD_CHARCELLS */

    bar->info.volume = sound_val2phys(SOUND_VOLUME, global_settings.volume);
#ifdef HAVE_CHARGING
    bar->info.inserted = (charger_input_state == CHARGER);
#endif
    bar->info.battlevel = battery_level();
    bar->info.battery_safe = battery_level_safe();

#ifdef HAVE_LCD_BITMAP
#ifdef CONFIG_RTC
    tm = get_time();
    bar->info.hour = tm->tm_hour;
    bar->info.minute = tm->tm_min;
#endif /* CONFIG_RTC */

    bar->info.shuffle = global_settings.playlist_shuffle;
#ifdef HAS_BUTTON_HOLD
    bar->info.keylock = button_hold();
#else
    bar->info.keylock = keys_locked;
#endif /* HAS_BUTTON_HOLD */
#ifdef HAS_REMOTE_BUTTON_HOLD
    bar->info.keylockremote = remote_button_hold();
#endif
    bar->info.repeat = global_settings.repeat_mode;
    bar->info.playmode = current_playmode();
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    if(!display->has_disk_led)
        bar->info.led = led_read(HZ/2); /* delay should match polling interval */
#endif

#ifdef HAVE_USB_POWER
    bar->info.usb_power = usb_powered();
#endif /* HAVE_USB_POWER */

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
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_CHARGING
        if (bar->info.inserted) {
            battery_state = true;
#if defined(HAVE_CHARGE_CTRL) || \
    defined(HAVE_CHARGE_STATE) || \
    CONFIG_BATTERY == BATT_LIION2200
            /* zero battery run time if charging */
            if (charge_state > DISCHARGING) {
                lasttime = current_tick;
            }

            /* animate battery if charging */
            if ((charge_state == CHARGING)
#ifdef HAVE_CHARGE_CTRL
                    || (charge_state == TOPOFF)
#endif
                ) {
#else
            lasttime = current_tick;
            {
#endif
                /* animate in three steps (34% per step for a better look) */
#ifndef HAVE_CHARGE_STATE
                bar->info.battlevel = 0;
#endif
                if(TIME_AFTER(current_tick, bar->battery_icon_switch_tick)) {
                    if (bar->animated_level == 100)
                    {
                        bar->animated_level = bar->info.battlevel;
                    }
                    else
                    {
                        bar->animated_level += 34;
                        if (bar->animated_level > 100)
                            bar->animated_level = 100;
                    }
                    bar->battery_icon_switch_tick = current_tick + HZ;
                }
            }
        }
        else
#endif /* HAVE_CHARGING */
        {
            bar->animated_level = 0;
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
            gui_statusbar_icon_battery(display, bar->info.battlevel, 
                                       bar->animated_level);
#ifdef HAVE_USB_POWER
        if (bar->info.usb_power)
            display->mono_bitmap(bitmap_icons_7x8[Icon_USBPlug],
                                 STATUSBAR_PLUG_X_POS,
                                 STATUSBAR_Y_POS, STATUSBAR_PLUG_WIDTH,
                                 STATUSBAR_HEIGHT);
        else
#endif /* HAVE_USB_POWER */
        /* draw power plug if charging */
        if (bar->info.inserted)
            display->mono_bitmap(bitmap_icons_7x8[Icon_Plug],
                                    STATUSBAR_PLUG_X_POS,
                                    STATUSBAR_Y_POS, STATUSBAR_PLUG_WIDTH,
                                    STATUSBAR_HEIGHT);

        bar->info.redraw_volume = gui_statusbar_icon_volume(bar,
                                                bar->info.volume);
        gui_statusbar_icon_play_state(display, current_playmode() +
                                                Icon_Play);

        switch (bar->info.repeat) {
#if (AB_REPEAT_ENABLE == 1)
            case REPEAT_AB:
                gui_statusbar_icon_play_mode(display, Icon_RepeatAB);
                break;
#endif /* AB_REPEAT_ENABLE == 1 */

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
#ifdef HAS_REMOTE_BUTTON_HOLD
        if (bar->info.keylockremote)
            gui_statusbar_icon_lock_remote(display);
#endif
#ifdef CONFIG_RTC
        gui_statusbar_time(display, bar->info.hour, bar->info.minute);
#endif /* CONFIG_RTC */
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
        if(!display->has_disk_led && bar->info.led)
            gui_statusbar_led(display);
#endif
        display->update_rect(0, 0, display->width, STATUSBAR_HEIGHT);
        bar->lastinfo = bar->info;
#endif /* HAVE_LCD_BITMAP */
    }


#ifdef HAVE_LCD_CHARCELLS
    if (bar->info.battlevel > -1)
        display->icon(ICON_BATTERY, battery_state);
    display->icon(ICON_BATTERY_1, bar->info.battlevel > 25);
    display->icon(ICON_BATTERY_2, bar->info.battlevel > 50);
    display->icon(ICON_BATTERY_3, bar->info.battlevel > 75);

    vol = 100 * (bar->info.volume - sound_min(SOUND_VOLUME))
           / (sound_max(SOUND_VOLUME) - sound_min(SOUND_VOLUME));
    display->icon(ICON_VOLUME, true);
    display->icon(ICON_VOLUME_1, vol > 10);
    display->icon(ICON_VOLUME_2, vol > 30);
    display->icon(ICON_VOLUME_3, vol > 50);
    display->icon(ICON_VOLUME_4, vol > 70);
    display->icon(ICON_VOLUME_5, vol > 90);

    display->icon(ICON_PLAY, current_playmode() == STATUS_PLAY);
    display->icon(ICON_PAUSE, current_playmode() == STATUS_PAUSE);

    display->icon(ICON_REPEAT, global_settings.repeat_mode != REPEAT_OFF);
    display->icon(ICON_1, global_settings.repeat_mode == REPEAT_ONE);

    display->icon(ICON_RECORD, record);
    display->icon(ICON_AUDIO, audio);
    display->icon(ICON_PARAM, param);
    display->icon(ICON_USB, usb);
#endif /* HAVE_LCD_CHARCELLS */
}

#ifdef HAVE_LCD_BITMAP
/* from icon.c */
/*
 * Print battery icon to status bar
 */
void gui_statusbar_icon_battery(struct screen * display, int percent, 
                                int animated_percent)
{
    int fill, endfill;
    char buffer[5];
    unsigned int width, height;
#if LCD_DEPTH > 1
    unsigned int prevfg = LCD_DEFAULT_FG;
#endif

    /* fill battery */
    fill = percent;
    if (fill < 0)
        fill = 0;
    if (fill > 100)
        fill = 100;

    endfill = animated_percent;
    if (endfill < 0)
        endfill = 0;
    if (endfill > 100)
        endfill = 100;
    
#if (defined(HAVE_CHARGE_CTRL) || defined(HAVE_CHARGE_STATE)) && \
    !defined(SIMULATOR) /* Certain charge controlled targets */
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
#if LCD_DEPTH > 1
        if (display->depth > 1)
        {
            prevfg = display->get_foreground();
            display->set_foreground(LCD_DARKGRAY);
        }
#endif
        endfill = endfill * 15 / 100 - fill;
        display->fillrect(STATUSBAR_BATTERY_X_POS + 1 + fill, 
                          STATUSBAR_Y_POS + 1, endfill, 5);
#if LCD_DEPTH > 1
        if (display->depth > 1)
            display->set_foreground(prevfg);
#endif
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
bool gui_statusbar_icon_volume(struct gui_statusbar * bar, int volume)
{
    int i;
    int vol;
    char buffer[4];
    unsigned int width, height;
    bool needs_redraw = false;
    int type = global_settings.volume_type;
    struct screen * display=bar->display; 
    int minvol = sound_min(SOUND_VOLUME);
    int maxvol = sound_max(SOUND_VOLUME);

    if (volume < minvol)
        volume = minvol;
    if (volume > maxvol)
        volume = maxvol;

    if (volume == minvol) {
        display->mono_bitmap(bitmap_icons_7x8[Icon_Mute],
                    STATUSBAR_VOLUME_X_POS + STATUSBAR_VOLUME_WIDTH / 2 - 4,
                    STATUSBAR_Y_POS, 7, STATUSBAR_HEIGHT);
    }
    else {
        /* We want to redraw the icon later on */
        if (bar->last_volume != volume && bar->last_volume >= minvol) {
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
            snprintf(buffer, sizeof(buffer), "%2d", volume);
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
            vol = (volume - minvol) * 14 / (maxvol - minvol);
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
    display->mono_bitmap(bitmap_icons_5x8[Icon_Lock_Main],
                         STATUSBAR_LOCKM_X_POS, STATUSBAR_Y_POS,
                         STATUSBAR_LOCKM_WIDTH, STATUSBAR_HEIGHT);
}

/*
 * Print remote lock when remote hold is enabled
 */
void gui_statusbar_icon_lock_remote(struct screen * display)
{
    display->mono_bitmap(bitmap_icons_5x8[Icon_Lock_Remote],
                         STATUSBAR_LOCKR_X_POS, STATUSBAR_Y_POS,
                         STATUSBAR_LOCKR_WIDTH, STATUSBAR_HEIGHT);
}

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
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

#ifdef CONFIG_RTC
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
#ifdef HAVE_LCD_BITMAP
    if(!global_settings.statusbar)
       return;
#endif /* HAVE_LCD_BITMAP */
    int i;
    FOR_NB_SCREENS(i) {
        gui_statusbar_draw( &(bars->statusbars[i]), force_redraw );
    }
}
