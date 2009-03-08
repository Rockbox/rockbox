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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "screen_access.h"
#include "font.h"
#include "kernel.h"
#include "string.h" /* for memcmp oO*/
#include "sprintf.h"
#include "sound.h"
#include "settings.h"
#include "viewport.h"
#if CONFIG_CODEC == SWCODEC
#include "metadata.h"
#endif
#include "icons.h"
#include "powermgmt.h"
#include "usb.h"
#include "led.h"

#include "status.h" /* needed for battery_state global var */
#include "action.h" /* for keys_locked */
#include "statusbar.h"
#ifdef HAVE_RECORDING
#include "audio.h"
#include "recording.h"
#include "pcm_record.h"
#endif
#include "appevents.h"
#include "timefuncs.h"

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
#define STATUSBAR_ENCODER_X_POS                 STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                2*ICONS_SPACING - 1
#define STATUSBAR_ENCODER_WIDTH                 18
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
#define STATUSBAR_RECFREQ_X_POS                 STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                3*ICONS_SPACING
#define STATUSBAR_RECFREQ_WIDTH                 12
#define STATUSBAR_RECCHANNELS_X_POS             STATUSBAR_X_POS + \
                                                STATUSBAR_BATTERY_WIDTH + \
                                                STATUSBAR_PLUG_WIDTH + \
                                                STATUSBAR_VOLUME_WIDTH + \
                                                STATUSBAR_PLAY_STATE_WIDTH + \
                                                STATUSBAR_RECFREQ_WIDTH + \
                                                4*ICONS_SPACING
#define STATUSBAR_RECCHANNELS_WIDTH             5
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

/* Prototypes */
#ifdef HAVE_LCD_BITMAP
static void gui_statusbar_icon_battery(struct screen * display, int percent,
                                       int batt_charge_step);
static bool gui_statusbar_icon_volume(struct gui_statusbar * bar, int volume);
static void gui_statusbar_icon_play_state(struct screen * display, int state);
static void gui_statusbar_icon_play_mode(struct screen * display, int mode);
static void gui_statusbar_icon_shuffle(struct screen * display);
static void gui_statusbar_icon_lock(struct screen * display);
#ifdef HAS_REMOTE_BUTTON_HOLD
static void gui_statusbar_icon_lock_remote(struct screen * display);
#endif
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
static void gui_statusbar_led(struct screen * display);
#endif
#ifdef HAVE_RECORDING
static void gui_statusbar_icon_recording_info(struct screen * display);
#endif
#if CONFIG_RTC
static void gui_statusbar_time(struct screen * display, struct tm *time);
#endif
#endif

/* End prototypes */


/*
 * Initializes a status bar
 *  - bar : the bar to initialize
 */
static void gui_statusbar_init(struct gui_statusbar * bar)
{
    bar->redraw_volume = true;
    bar->volume_icon_switch_tick = bar->battery_icon_switch_tick = current_tick;
    memset((void*)&(bar->lastinfo), 0, sizeof(struct status_info));
#if CONFIG_RTC
    bar->last_tm_min = 0;
#endif    
}

void gui_statusbar_draw(struct gui_statusbar * bar, bool force_redraw)
{
    struct screen * display = bar->display;

#ifdef HAVE_LCD_CHARCELLS
    int val;
    (void)force_redraw; /* players always "redraw" */
#endif /* HAVE_LCD_CHARCELLS */

    bar->info.battlevel = battery_level();
#ifdef HAVE_USB_POWER
    bar->info.usb_inserted = usb_inserted();
#endif
#if CONFIG_CHARGING
    bar->info.inserted = (charger_input_state == CHARGER);
    if (bar->info.inserted)
    {
        bar->info.battery_state = true;

#if CONFIG_CHARGING >= CHARGING_MONITOR

        /* zero battery run time if charging */
        if (charge_state > DISCHARGING)
            lasttime = current_tick;

        /* animate battery if charging */
        if ((charge_state == DISCHARGING) || (charge_state == TRICKLE))
        {
            bar->info.batt_charge_step = -1;
        }
        else
        {
#else /* CONFIG_CHARGING < CHARGING_MONITOR */
        lasttime = current_tick;
        {
#endif /* CONFIG_CHARGING < CHARGING_MONITOR */
            /* animate in (max.) 4 steps, starting near the current charge level */
            if (TIME_AFTER(current_tick, bar->battery_icon_switch_tick)) 
            {
                if (++bar->info.batt_charge_step > 3)
                    bar->info.batt_charge_step = bar->info.battlevel / 34;
                bar->battery_icon_switch_tick = current_tick + HZ;
            }
        }
    }
    else
#endif /* CONFIG_CHARGING */
    {
        bar->info.batt_charge_step = -1;
        if (battery_level_safe())
            bar->info.battery_state = true;
        else
            /* blink battery if level is low */
            if (TIME_AFTER(current_tick, bar->battery_icon_switch_tick) &&
               (bar->info.battlevel > -1))
            {
                bar->info.battery_state = !bar->info.battery_state;
                bar->battery_icon_switch_tick = current_tick + HZ;
            }
    }

    bar->info.volume = global_settings.volume;
#ifdef HAVE_LCD_BITMAP
    bar->info.shuffle = global_settings.playlist_shuffle;
#ifdef HAS_BUTTON_HOLD
    bar->info.keylock = button_hold();
#else
    bar->info.keylock = is_keys_locked();
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
#if CONFIG_RTC
    bar->time = get_time();
#endif /* CONFIG_RTC */

    /* only redraw if forced to, or info has changed */
    if (force_redraw || bar->redraw_volume ||
#if CONFIG_RTC
        (bar->time->tm_min != bar->last_tm_min) ||
#endif
        memcmp(&(bar->info), &(bar->lastinfo), sizeof(struct status_info)))
    {
        struct viewport vp;
        viewport_set_defaults(&vp, display->screen_type);
        vp.height = STATUSBAR_HEIGHT;
        vp.x = STATUSBAR_X_POS;
        vp.y = STATUSBAR_Y_POS;
        display->set_viewport(&vp);
        display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        display->fillrect(0, 0, display->getwidth(), STATUSBAR_HEIGHT);
        display->set_drawmode(DRMODE_SOLID);

        if (bar->info.battery_state)
            gui_statusbar_icon_battery(display, bar->info.battlevel, 
                                       bar->info.batt_charge_step);
#ifdef HAVE_USB_POWER
        if (bar->info.usb_inserted)
            display->mono_bitmap(bitmap_icons_7x8[Icon_USBPlug],
                                 STATUSBAR_PLUG_X_POS,
                                 STATUSBAR_Y_POS, STATUSBAR_PLUG_WIDTH,
                                 STATUSBAR_HEIGHT);
#endif /* HAVE_USB_POWER */
#if CONFIG_CHARGING
#ifdef HAVE_USB_POWER
        else
#endif
        /* draw power plug if charging */
        if (bar->info.inserted)
            display->mono_bitmap(bitmap_icons_7x8[Icon_Plug],
                                    STATUSBAR_PLUG_X_POS,
                                    STATUSBAR_Y_POS, STATUSBAR_PLUG_WIDTH,
                                    STATUSBAR_HEIGHT);
#endif /* CONFIG_CHARGING */
#ifdef HAVE_RECORDING
        /* turn off volume display in recording screen */
        bool recscreen_on = in_recording_screen();
        if (!recscreen_on)
#endif
            bar->redraw_volume = gui_statusbar_icon_volume(bar, bar->info.volume);
        gui_statusbar_icon_play_state(display, current_playmode() + Icon_Play);
        
#ifdef HAVE_RECORDING
        /* If in recording screen, replace repeat mode, volume
           and shuffle icons with recording info */
        if (recscreen_on)
            gui_statusbar_icon_recording_info(display);
        else
#endif
        {
            switch (bar->info.repeat) {
#ifdef AB_REPEAT_ENABLE
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
        }
        if (bar->info.keylock)
            gui_statusbar_icon_lock(display);
#ifdef HAS_REMOTE_BUTTON_HOLD
        if (bar->info.keylockremote)
            gui_statusbar_icon_lock_remote(display);
#endif
#if CONFIG_RTC
        gui_statusbar_time(display, bar->time);
        bar->last_tm_min = bar->time->tm_min;
#endif /* CONFIG_RTC */
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
        if(!display->has_disk_led && bar->info.led)
            gui_statusbar_led(display);
#endif
        display->update_viewport();
        display->set_viewport(NULL);
        bar->lastinfo = bar->info;
    }
#endif /* HAVE_LCD_BITMAP */


#ifdef HAVE_LCD_CHARCELLS
    display->icon(ICON_BATTERY, bar->info.battery_state);
    
    if (bar->info.batt_charge_step > -1)
        val = bar->info.batt_charge_step;
    else
        val = (bar->info.battlevel * 3 + 50) / 100;
    display->icon(ICON_BATTERY_1, val >= 1);
    display->icon(ICON_BATTERY_2, val >= 2);
    display->icon(ICON_BATTERY_3, val >= 3);

    val = 10 * (bar->info.volume - sound_min(SOUND_VOLUME))
           / (sound_max(SOUND_VOLUME) - sound_min(SOUND_VOLUME));
    display->icon(ICON_VOLUME, true);
    display->icon(ICON_VOLUME_1, val >= 1);
    display->icon(ICON_VOLUME_2, val >= 3);
    display->icon(ICON_VOLUME_3, val >= 5);
    display->icon(ICON_VOLUME_4, val >= 7);
    display->icon(ICON_VOLUME_5, val >= 9);

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
static void gui_statusbar_icon_battery(struct screen * display, int percent, 
                                       int batt_charge_step)
{
    int fill, endfill;
    char buffer[5];
    unsigned int width, height;
#if LCD_DEPTH > 1
    unsigned int prevfg = 0;
#endif

#if CONFIG_CHARGING
    if (batt_charge_step >= 0)
    {
        fill = percent * (STATUSBAR_BATTERY_WIDTH-3) / 100;
        endfill = 34 * batt_charge_step * (STATUSBAR_BATTERY_WIDTH-3) / 100;
    }
    else
#else
    (void)batt_charge_step;
#endif
    {
        fill = endfill = (percent * (STATUSBAR_BATTERY_WIDTH-3) + 50) / 100;
    }

#if CONFIG_CHARGING == CHARGING_MONITOR && !defined(SIMULATOR) 
    /* Certain charge controlled targets */
    /* show graphical animation when charging instead of numbers */
    if ((global_settings.battery_display) &&
        (charge_state != CHARGING) &&
        (percent > -1)) {
#else /* all others */
    if (global_settings.battery_display && (percent > -1)) {
#endif
        /* Numeric display */
        display->setfont(FONT_SYSFIXED);
        snprintf(buffer, sizeof(buffer), "%3d", percent);
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

        display->fillrect(STATUSBAR_BATTERY_X_POS + 1, STATUSBAR_Y_POS + 1,
                          fill, 5);
#if LCD_DEPTH > 1
        if (display->depth > 1)
        {
            prevfg = display->get_foreground();
            display->set_foreground(LCD_DARKGRAY);
        }
#endif
        display->fillrect(STATUSBAR_BATTERY_X_POS + 1 + fill,
                          STATUSBAR_Y_POS + 1, endfill - fill, 5);
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
static bool gui_statusbar_icon_volume(struct gui_statusbar * bar, int volume)
{
    int i;
    int vol;
    char buffer[4];
    unsigned int width, height;
    bool needs_redraw = false;
    int type = global_settings.volume_type;
    struct screen * display=bar->display; 
    const int minvol = sound_min(SOUND_VOLUME);
    const int maxvol = sound_max(SOUND_VOLUME);

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
static void gui_statusbar_icon_play_state(struct screen * display, int state)
{
    display->mono_bitmap(bitmap_icons_7x8[state], STATUSBAR_PLAY_STATE_X_POS,
                    STATUSBAR_Y_POS, STATUSBAR_PLAY_STATE_WIDTH,
                    STATUSBAR_HEIGHT);
}

/*
 * Print play mode to status bar
 */
static void gui_statusbar_icon_play_mode(struct screen * display, int mode)
{
    display->mono_bitmap(bitmap_icons_7x8[mode], STATUSBAR_PLAY_MODE_X_POS,
                    STATUSBAR_Y_POS, STATUSBAR_PLAY_MODE_WIDTH,
                    STATUSBAR_HEIGHT);
}

/*
 * Print shuffle mode to status bar
 */
static void gui_statusbar_icon_shuffle(struct screen * display)
{
    display->mono_bitmap(bitmap_icons_7x8[Icon_Shuffle],
                    STATUSBAR_SHUFFLE_X_POS, STATUSBAR_Y_POS,
                    STATUSBAR_SHUFFLE_WIDTH, STATUSBAR_HEIGHT);
}

/*
 * Print lock when keys are locked
 */
static void gui_statusbar_icon_lock(struct screen * display)
{
    display->mono_bitmap(bitmap_icons_5x8[Icon_Lock_Main],
                         STATUSBAR_LOCKM_X_POS, STATUSBAR_Y_POS,
                         STATUSBAR_LOCKM_WIDTH, STATUSBAR_HEIGHT);
}

#ifdef HAS_REMOTE_BUTTON_HOLD
/*
 * Print remote lock when remote hold is enabled
 */
static void gui_statusbar_icon_lock_remote(struct screen * display)
{
    display->mono_bitmap(bitmap_icons_5x8[Icon_Lock_Remote],
                         STATUSBAR_LOCKR_X_POS, STATUSBAR_Y_POS,
                         STATUSBAR_LOCKR_WIDTH, STATUSBAR_HEIGHT);
}
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
/*
 * no real LED: disk activity in status bar
 */
static void gui_statusbar_led(struct screen * display)
{
    display->mono_bitmap(bitmap_icon_disk,
                         STATUSBAR_DISK_X_POS(display->getwidth()),
                         STATUSBAR_Y_POS, STATUSBAR_DISK_WIDTH,
                         STATUSBAR_HEIGHT);
}
#endif

#if CONFIG_RTC
/*
 * Print time to status bar
 */
static void gui_statusbar_time(struct screen * display, struct tm *time)
{
    unsigned char buffer[6];
    unsigned int width, height;
    int hour, minute;
    if ( valid_time(time) ) {
        hour = time->tm_hour;
        minute = time->tm_min;
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
        display->putsxy(STATUSBAR_TIME_X_END(display->getwidth()) - width,
                        STATUSBAR_Y_POS, buffer);
    }
    display->setfont(FONT_UI);

}
#endif

#ifdef HAVE_RECORDING
#if CONFIG_CODEC == SWCODEC
/**
 * Write a number to the display using bitmaps and return new position
 */
static int write_bitmap_number(struct screen * display, int value,
                               int x, int y)
{
    char buf[12], *ptr;
    snprintf(buf, sizeof(buf), "%d", value);

    for (ptr = buf; *ptr != '\0'; ptr++, x += BM_GLYPH_WIDTH)
        display->mono_bitmap(bitmap_glyphs_4x8[*ptr - '0'], x, y,
                             BM_GLYPH_WIDTH, STATUSBAR_HEIGHT);
    return x;
}

/**
 * Write format info bitmaps - right justified
 */
static void gui_statusbar_write_format_info(struct screen * display)
{
    /* Can't fit info for sw codec targets in statusbar using FONT_SYSFIXED 
       so must use icons */
    int rec_format = global_settings.rec_format;
    unsigned bitrk = 0; /* compiler warns about unitialized use !! */
    int xpos       = STATUSBAR_ENCODER_X_POS;
    int width      = STATUSBAR_ENCODER_WIDTH;
    const unsigned char *bm = bitmap_formats_18x8[rec_format];

    if (rec_format == REC_FORMAT_MPA_L3)
    {
        /* Special handling for mp3 */
        bitrk = global_settings.mp3_enc_config.bitrate;
        bitrk = mp3_enc_bitr[bitrk];

        width = BM_MPA_L3_M_WIDTH;

        /* Slide 'M' to right if fewer than three digits used */
        if (bitrk > 999)
            bitrk = 999; /* neurotic safety check if corrupted */
        else
        {
            if (bitrk < 100)
                xpos += BM_GLYPH_WIDTH;
            if (bitrk < 10)
                xpos += BM_GLYPH_WIDTH;
        }
    }
       

    /* Show bitmap - clipping right edge if needed */
    display->mono_bitmap_part(bm, 0, 0, STATUSBAR_ENCODER_WIDTH,
        xpos, STATUSBAR_Y_POS, width, STATUSBAR_HEIGHT);

    if (rec_format == REC_FORMAT_MPA_L3)
    {
        xpos += BM_MPA_L3_M_WIDTH; /* to right of 'M' */
        write_bitmap_number(display, bitrk, xpos, STATUSBAR_Y_POS);
    }
}

/**
 * Write sample rate using bitmaps - left justified
 */
static void gui_statusbar_write_samplerate_info(struct screen * display)
{
    unsigned long samprk;
    int xpos;

#ifdef SIMULATOR
    samprk = 44100;
#else
#ifdef HAVE_SPDIF_REC
    if (global_settings.rec_source == AUDIO_SRC_SPDIF)
        /* Use rate in use, not current measured rate if it changed */
        samprk = pcm_rec_sample_rate();
    else
#endif
        samprk = rec_freq_sampr[global_settings.rec_frequency];
#endif /* SIMULATOR */

    samprk /= 1000;
    if (samprk > 99)
        samprk = 99;  /* Limit to 3 glyphs */

    xpos = write_bitmap_number(display, (unsigned)samprk,
                               STATUSBAR_RECFREQ_X_POS, STATUSBAR_Y_POS);

    /* write the 'k' */
    display->mono_bitmap(bitmap_glyphs_4x8[Glyph_4x8_k], xpos, 
                         STATUSBAR_Y_POS, BM_GLYPH_WIDTH,
                         STATUSBAR_HEIGHT);
}
#endif /* CONFIG_CODEC == SWCODEC */

static void gui_statusbar_icon_recording_info(struct screen * display)
{
#if CONFIG_CODEC != SWCODEC
    char buffer[3];
    int width, height;
    display->setfont(FONT_SYSFIXED);
#endif /* CONFIG_CODEC != SWCODEC */

    /* Display Codec info in statusbar */
#if CONFIG_CODEC == SWCODEC
    gui_statusbar_write_format_info(display);
#else /* !SWCODEC */
    display->mono_bitmap(bitmap_icons_5x8[Icon_q],
                             STATUSBAR_ENCODER_X_POS + 8, STATUSBAR_Y_POS,
                             5, STATUSBAR_HEIGHT);

    snprintf(buffer, sizeof(buffer), "%d", global_settings.rec_quality);
    display->getstringsize(buffer, &width, &height);
    if (height <= STATUSBAR_HEIGHT)
        display->putsxy(STATUSBAR_ENCODER_X_POS + 13, STATUSBAR_Y_POS, buffer);
#endif /* CONFIG_CODEC == SWCODEC */

    /* Display Samplerate info in statusbar */
#if CONFIG_CODEC == SWCODEC
    /* SWCODEC targets use bitmaps for glyphs */    
    gui_statusbar_write_samplerate_info(display);
#else /* !SWCODEC */
    /* hwcodec targets have sysfont characters */ 
#ifdef HAVE_SPDIF_REC
    if (global_settings.rec_source == AUDIO_SRC_SPDIF)
    {
        /* Can't measure S/PDIF sample rate on Archos/Sim yet */
        strncpy(buffer, "--", sizeof(buffer));
    }
    else
#endif /* HAVE_SPDIF_IN */
    {
        static char const * const freq_strings[12] =
        { "44", "48", "32", "22", "24", "16" };
        strncpy(buffer, freq_strings[global_settings.rec_frequency],
                sizeof(buffer));
    }

    display->getstringsize(buffer, &width, &height);

    if (height <= STATUSBAR_HEIGHT)
        display->putsxy(STATUSBAR_RECFREQ_X_POS, STATUSBAR_Y_POS, buffer);

    display->setfont(FONT_UI);
#endif /* CONFIG_CODEC == SWCODEC */

    /* Display Channel status in status bar */
    if(global_settings.rec_channels)
    {
        display->mono_bitmap(bitmap_icons_5x8[Icon_Mono],
                             STATUSBAR_RECCHANNELS_X_POS , STATUSBAR_Y_POS,
                             STATUSBAR_RECCHANNELS_WIDTH, STATUSBAR_HEIGHT);
    }
    else
    {
        display->mono_bitmap(bitmap_icons_5x8[Icon_Stereo],
                             STATUSBAR_RECCHANNELS_X_POS, STATUSBAR_Y_POS,
                             STATUSBAR_RECCHANNELS_WIDTH, STATUSBAR_HEIGHT);
    }
}
#endif /* HAVE_RECORDING */

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

void gui_statusbar_changed(bool enabled)
{
    (void)enabled;
    send_event(GUI_EVENT_STATUSBAR_TOGGLE, NULL);
}
