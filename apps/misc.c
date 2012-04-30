/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "string-extra.h"
#include "config.h"
#include "misc.h"
#include "system.h"
#include "lcd.h"
#include "file.h"
#ifndef __PCTOOL__
#include "filefuncs.h"
#include "lang.h"
#include "dir.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "action.h"
#include "timefuncs.h"
#include "screens.h"
#include "usb_screen.h"
#include "talk.h"
#include "audio.h"
#include "settings.h"
#include "storage.h"
#include "ata_idle_notify.h"
#include "kernel.h"
#include "power.h"
#include "powermgmt.h"
#include "backlight.h"
#include "version.h"
#include "font.h"
#include "splash.h"
#include "tagcache.h"
#include "scrobbler.h"
#include "sound.h"
#include "playlist.h"
#include "yesno.h"
#include "viewport.h"
#include "list.h"

#include "debug.h"

#if CONFIG_TUNER
#include "radio.h"
#endif

#ifdef IPOD_ACCESSORY_PROTOCOL
#include "iap.h"
#endif

#if (CONFIG_STORAGE & STORAGE_MMC)
#include "ata_mmc.h"
#endif
#include "tree.h"
#include "eeprom_settings.h"
#if defined(HAVE_RECORDING) && !defined(__PCTOOL__)
#include "recording.h"
#endif
#if defined(HAVE_LCD_BITMAP) && !defined(__PCTOOL__)
#include "bmp.h"
#include "icons.h"
#endif /* End HAVE_LCD_BITMAP */
#include "bookmark.h"
#include "wps.h"
#include "playback.h"
#if CONFIG_CODEC == SWCODEC
#include "voice_thread.h"
#else
#include "mp3_playback.h"
#endif

#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_HANDLED_BY_OF) \
        || defined(HAVE_HOTSWAP_STORAGE_AS_MAIN)
#include "rolo.h"
#endif
#endif

#ifdef HAVE_HARDWARE_CLICK
#include "piezo.h"
#endif

/* units used with output_dyn_value */
const unsigned char * const byte_units[] =
{
    ID2P(LANG_BYTE),
    ID2P(LANG_KILOBYTE),
    ID2P(LANG_MEGABYTE),
    ID2P(LANG_GIGABYTE)
};

const unsigned char * const * const kbyte_units = &byte_units[1];

/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf, int buf_size, int value,
                       const unsigned char * const *units, bool bin_scale)
{
    int scale = bin_scale ? 1024 : 1000;
    int fraction = 0;
    int unit_no = 0;
    char tbuf[5];

    while (value >= scale)
    {
        fraction = value % scale;
        value /= scale;
        unit_no++;
    }
    if (bin_scale)
        fraction = fraction * 1000 / 1024;

    if (value >= 100 || !unit_no)
        tbuf[0] = '\0';
    else if (value >= 10)
        snprintf(tbuf, sizeof(tbuf), "%01d", fraction / 100);
    else
        snprintf(tbuf, sizeof(tbuf), "%02d", fraction / 10);

    if (buf)
    {
        if (*tbuf)
            snprintf(buf, buf_size, "%d%s%s%s", value, str(LANG_POINT),
                     tbuf, P2STR(units[unit_no]));
        else
            snprintf(buf, buf_size, "%d%s", value, P2STR(units[unit_no]));
    }
    else
    {
        talk_fractional(tbuf, value, P2ID(units[unit_no]));
    }
    return buf;
}

/* Ask the user if they really want to erase the current dynamic playlist
 * returns true if the playlist should be replaced */
bool warn_on_pl_erase(void)
{
    if (global_settings.warnon_erase_dynplaylist &&
        !global_settings.party_mode &&
        playlist_modified(NULL))
    {
        static const char *lines[] =
            {ID2P(LANG_WARN_ERASEDYNPLAYLIST_PROMPT)};
        static const struct text_message message={lines, 1};

        return (gui_syncyesno_run(&message, NULL, NULL) == YESNO_YES);
    }
    else
        return true;
}


/* Performance optimized version of the read_line() (see below) function. */
int fast_readline(int fd, char *buf, int buf_size, void *parameters,
                  int (*callback)(int n, char *buf, void *parameters))
{
    char *p, *next;
    int rc, pos = 0;
    int count = 0;

    while ( 1 )
    {
        next = NULL;

        rc = read(fd, &buf[pos], buf_size - pos - 1);
        if (rc >= 0)
            buf[pos+rc] = '\0';

        if ( (p = strchr(buf, '\n')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }

        rc = callback(count, buf, parameters);
        if (rc < 0)
            return rc;

        count++;
        if (next)
        {
            pos = buf_size - ((long)next - (long)buf) - 1;
            memmove(buf, next, pos);
        }
        else
            break ;
    }

    return 0;
}

/* parse a line from a configuration file. the line format is:

   name: value

   Any whitespace before setting name or value (after ':') is ignored.
   A # as first non-whitespace character discards the whole line.
   Function sets pointers to null-terminated setting name and value.
   Returns false if no valid config entry was found.
*/

bool settings_parseline(char* line, char** name, char** value)
{
    char* ptr;

    line = skip_whitespace(line);

    if ( *line == '#' )
        return false;

    ptr = strchr(line, ':');
    if ( !ptr )
        return false;

    *name = line;
    *ptr = 0;
    ptr++;
    ptr = skip_whitespace(ptr);
    *value = ptr;
    return true;
}

static void system_flush(void)
{
    playlist_shutdown();
    tree_flush();
    call_storage_idle_notifys(true); /*doesnt work on usb and shutdown from ata thread */
}

static void system_restore(void)
{
    tree_restore();
}

static bool clean_shutdown(void (*callback)(void *), void *parameter)
{
    long msg_id = -1;

    scrobbler_poweroff();

#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
    if(!charger_inserted())
#endif
    {
        bool batt_safe = battery_level_safe();
#if CONFIG_CODEC != SWCODEC || defined(HAVE_RECORDING)
        int audio_stat = audio_status();
#endif

        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();
            screens[i].update();
        }

        if (batt_safe)
        {
            int level;
#ifdef HAVE_TAGCACHE
            if (!tagcache_prepare_shutdown())
            {
                cancel_shutdown();
                splash(HZ, ID2P(LANG_TAGCACHE_BUSY));
                return false;
            }
#endif
            level = battery_level();
            if (level > 10 || level < 0)
                splash(0, str(LANG_SHUTTINGDOWN));
            else
            {
                msg_id = LANG_WARNING_BATTERY_LOW;
                splashf(0, "%s %s", str(LANG_WARNING_BATTERY_LOW),
                                    str(LANG_SHUTTINGDOWN));
            }
        }
        else
        {
            msg_id = LANG_WARNING_BATTERY_EMPTY;
            splashf(0, "%s %s", str(LANG_WARNING_BATTERY_EMPTY),
                                str(LANG_SHUTTINGDOWN));
        }
#if CONFIG_CODEC != SWCODEC
        if (global_settings.fade_on_stop
            && (audio_stat & AUDIO_STATUS_PLAY))
        {
            fade(false, false);
        }
#endif

        if (batt_safe) /* do not save on critical battery */
        {
#if defined(HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
            if (audio_stat & AUDIO_STATUS_RECORD)
            {
                rec_command(RECORDING_CMD_STOP);
                /* wait for stop to complete */
                while (audio_status() & AUDIO_STATUS_RECORD)
                    sleep(1);
            }
#endif
            bookmark_autobookmark(false);

            /* audio_stop_recording == audio_stop for HWCODEC */
            audio_stop();

            if (callback != NULL)
                callback(parameter);

#if CONFIG_CODEC != SWCODEC
            /* wait for audio_stop or audio_stop_recording to complete */
            while (audio_status())
                sleep(1);
#endif

#if defined(HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
            audio_close_recording();
#endif

            if(global_settings.talk_menu)
            {
                bool enqueue = false;
                if(msg_id != -1)
                {
                    talk_id(msg_id, enqueue);
                    enqueue = true;
                }
                talk_id(LANG_SHUTTINGDOWN, enqueue);
#if CONFIG_CODEC == SWCODEC
                voice_wait();
#endif
            }

            system_flush();
#ifdef HAVE_EEPROM_SETTINGS
            if (firmware_settings.initialized)
            {
                firmware_settings.disk_clean = true;
                firmware_settings.bl_version = 0;
                eeprom_settings_store();
            }
#endif
        }
#ifdef HAVE_DIRCACHE
        else
            dircache_disable();
#endif

        shutdown_hw();
    }
    return false;
}

bool list_stop_handler(void)
{
    bool ret = false;

#if CONFIG_TUNER
    radio_stop();
#endif

    /* Stop the music if it is playing */
    if(audio_status())
    {
        if (!global_settings.party_mode)
        {
#if CONFIG_CODEC != SWCODEC
            if (global_settings.fade_on_stop)
                fade(false, false);
#endif
            bookmark_autobookmark(true);
            audio_stop();
            ret = true;  /* bookmarking can make a refresh necessary */
        }
    }
#if CONFIG_CHARGING
#if (CONFIG_KEYPAD == RECORDER_PAD) && !defined(HAVE_SW_POWEROFF)
    else
    {
        if (charger_inserted())
            charging_splash();
        else
            shutdown_screen(); /* won't return if shutdown actually happens */

        ret = true;  /* screen is dirty, caller needs to refresh */
    }
#endif
#ifndef HAVE_POWEROFF_WHILE_CHARGING
    {
        static long last_off = 0;

        if (TIME_BEFORE(current_tick, last_off + HZ/2))
        {
            if (charger_inserted()) 
            {
                charging_splash();
                ret = true;  /* screen is dirty, caller needs to refresh */
            }
        }
        last_off = current_tick;
    }
#endif
#endif /* CONFIG_CHARGING */
    return ret;
}

#if CONFIG_CHARGING
static bool waiting_to_resume_play = false;
static long play_resume_tick;

static void car_adapter_mode_processing(bool inserted)
{
    if (global_settings.car_adapter_mode)
    {
        if(inserted)
        {
            /*
             * Just got plugged in, delay & resume if we were playing
             */
            if (audio_status() & AUDIO_STATUS_PAUSE)
            {
                /* delay resume a bit while the engine is cranking */
                play_resume_tick = current_tick + HZ*5;
                waiting_to_resume_play = true;
            }
        }
        else
        {
            /*
             * Just got unplugged, pause if playing
             */
            if ((audio_status() & AUDIO_STATUS_PLAY) &&
                !(audio_status() & AUDIO_STATUS_PAUSE))
            {
                pause_action(true, true);
            }
            waiting_to_resume_play = false;
        }
    }
}

static void car_adapter_tick(void)
{
    if (waiting_to_resume_play)
    {
        if (TIME_AFTER(current_tick, play_resume_tick))
        {
            if (audio_status() & AUDIO_STATUS_PAUSE)
            {
                queue_broadcast(SYS_CAR_ADAPTER_RESUME, 0);
            }
            waiting_to_resume_play = false;
        }
    }
}

void car_adapter_mode_init(void)
{
    tick_add_task(car_adapter_tick);
}
#endif

#ifdef HAVE_HEADPHONE_DETECTION
static void unplug_change(bool inserted)
{
    static bool headphone_caused_pause = false;

    if (global_settings.unplug_mode)
    {
        int audio_stat = audio_status();
        if (inserted)
        {
            backlight_on();
            if ((audio_stat & AUDIO_STATUS_PLAY) &&
                    headphone_caused_pause &&
                    global_settings.unplug_mode > 1 )
                unpause_action(true, true);
            headphone_caused_pause = false;
        } else {
            if ((audio_stat & AUDIO_STATUS_PLAY) &&
                    !(audio_stat & AUDIO_STATUS_PAUSE))
            {
                headphone_caused_pause = true;
                pause_action(false, false);
            }
        }
    }
}
#endif

long default_event_handler_ex(long event, void (*callback)(void *), void *parameter)
{
#if CONFIG_PLATFORM & (PLATFORM_ANDROID|PLATFORM_MAEMO)
    static bool resume = false;
#endif

    switch(event)
    {
        case SYS_BATTERY_UPDATE:
            if(global_settings.talk_battery_level)
            {
                talk_ids(true, VOICE_PAUSE, VOICE_PAUSE,
                         LANG_BATTERY_TIME,
                         TALK_ID(battery_level(), UNIT_PERCENT),
                         VOICE_PAUSE);
                talk_force_enqueue_next();
            }
            break;
        case SYS_USB_CONNECTED:
            if (callback != NULL)
                callback(parameter);
#if (CONFIG_STORAGE & STORAGE_MMC) && (defined(ARCHOS_ONDIOSP) || defined(ARCHOS_ONDIOFM))
            if (!mmc_touched() ||
                (mmc_remove_request() == SYS_HOTSWAP_EXTRACTED))
#endif
            {
                system_flush();
#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_HANDLED_BY_OF)
                check_bootfile(false); /* gets initial size */
#endif
#endif
                gui_usb_screen_run(false);
#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_HANDLED_BY_OF)
                check_bootfile(true);
#endif
#endif
                system_restore();
            }
            return SYS_USB_CONNECTED;

        case SYS_POWEROFF:
            if (!clean_shutdown(callback, parameter))
                return SYS_POWEROFF;
            break;
#if CONFIG_CHARGING
        case SYS_CHARGER_CONNECTED:
            car_adapter_mode_processing(true);
            return SYS_CHARGER_CONNECTED;

        case SYS_CHARGER_DISCONNECTED:
            car_adapter_mode_processing(false);
            /*reset rockbox battery runtime*/
            global_status.runtime = 0;
            return SYS_CHARGER_DISCONNECTED;

        case SYS_CAR_ADAPTER_RESUME:
            unpause_action(true, true);
            return SYS_CAR_ADAPTER_RESUME;
#endif
#ifdef HAVE_HOTSWAP_STORAGE_AS_MAIN
        case SYS_FS_CHANGED:
        {
            /* simple sanity: assume rockbox is on the first hotswappable
             * driver, abort out if that one isn't inserted */
            int i;
            for (i = 0; i < NUM_DRIVES; i++)
            {
                if (storage_removable(i) && !storage_present(i))
                    return SYS_FS_CHANGED;
            }
            system_flush();
            check_bootfile(true); /* state gotten in main.c:init() */
            system_restore();
        }
            return SYS_FS_CHANGED;
#endif
#ifdef HAVE_HEADPHONE_DETECTION
        case SYS_PHONE_PLUGGED:
            unplug_change(true);
            return SYS_PHONE_PLUGGED;

        case SYS_PHONE_UNPLUGGED:
            unplug_change(false);
            return SYS_PHONE_UNPLUGGED;
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
        case SYS_IAP_PERIODIC:
            iap_periodic();
            return SYS_IAP_PERIODIC;
        case SYS_IAP_HANDLEPKT:
            iap_handlepkt();
            return SYS_IAP_HANDLEPKT;
#endif
#if CONFIG_PLATFORM & (PLATFORM_ANDROID|PLATFORM_MAEMO)
        /* stop playback if we receive a call */
        case SYS_CALL_INCOMING:
            resume = audio_status() == AUDIO_STATUS_PLAY;
            list_stop_handler();
            return SYS_CALL_INCOMING;
        /* resume playback if needed */
        case SYS_CALL_HUNG_UP:
            if (resume && playlist_resume() != -1)
            {
                playlist_start(global_status.resume_index,
                               global_status.resume_offset);
            }
            resume = false;
            return SYS_CALL_HUNG_UP;
#endif
#if (CONFIG_PLATFORM & PLATFORM_HOSTED) && defined(PLATFORM_HAS_VOLUME_CHANGE)
        case SYS_VOLUME_CHANGED:
        {
            static bool firstvolume = true;
            /* kludge: since this events go to the button_queue,
             * event data is available in the last button data */
            int volume = button_get_data();
            DEBUGF("SYS_VOLUME_CHANGED: %d\n", volume);
            if (global_settings.volume != volume) {
                global_settings.volume = volume;
                if (firstvolume) {
                    setvol();
                    firstvolume = false;
                }
            }
            return 0;
        }
#endif
#ifdef HAVE_MULTIMEDIA_KEYS
        /* multimedia keys on keyboards, headsets */
        case BUTTON_MULTIMEDIA_PLAYPAUSE:
        {
            int status = audio_status();
            if (status & AUDIO_STATUS_PLAY)
            {
                if (status & AUDIO_STATUS_PAUSE)
                    unpause_action(true, true);
                else
                    pause_action(true, true);
            }
            else
                if (playlist_resume() != -1)
                {
                    playlist_start(global_status.resume_index,
                                   global_status.resume_offset);
                }
            return event;
        }
        case BUTTON_MULTIMEDIA_NEXT:
            audio_next();
            return event;
        case BUTTON_MULTIMEDIA_PREV:
            audio_prev();
            return event;
        case BUTTON_MULTIMEDIA_STOP:
            list_stop_handler();
            return event;
        case BUTTON_MULTIMEDIA_REW:
        case BUTTON_MULTIMEDIA_FFWD:
            /* not supported yet, needs to be done in the WPS */
            return 0;
#endif
    }
    return 0;
}

long default_event_handler(long event)
{
    return default_event_handler_ex(event, NULL, NULL);
}

int show_logo( void )
{
#ifdef HAVE_LCD_BITMAP
    char version[32];
    int font_h, font_w;

    snprintf(version, sizeof(version), "Ver. %s", rbversion);

    lcd_clear_display();
#if defined(SANSA_CLIP) || defined(SANSA_CLIPV2) || defined(SANSA_CLIPPLUS)
    /* display the logo in the blue area of the screen */
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(version)*font_w)/2),
               0, (unsigned char *)version);
    lcd_bmp(&bm_rockboxlogo, (LCD_WIDTH - BMPWIDTH_rockboxlogo) / 2, 16);
#else
    lcd_bmp(&bm_rockboxlogo, (LCD_WIDTH - BMPWIDTH_rockboxlogo) / 2, 10);
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(version)*font_w)/2),
               LCD_HEIGHT-font_h, (unsigned char *)version);
#endif
    lcd_setfont(FONT_UI);

#else
    char *rockbox = "  ROCKbox!";

    lcd_clear_display();
    lcd_double_height(true);
    lcd_puts(0, 0, rockbox);
    lcd_puts_scroll(0, 1, rbversion);
#endif
    lcd_update();

#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    lcd_remote_bmp(&bm_remote_rockboxlogo, 0, 10);
    lcd_remote_setfont(FONT_SYSFIXED);
    lcd_remote_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_remote_putsxy((LCD_REMOTE_WIDTH/2) - ((strlen(version)*font_w)/2),
                       LCD_REMOTE_HEIGHT-font_h, (unsigned char *)version);
    lcd_remote_setfont(FONT_UI);
    lcd_remote_update();
#endif

    return 0;
}

#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_HANDLED_BY_OF) || defined(HAVE_HOTSWAP_STORAGE_AS_MAIN)
/*
    memorize/compare details about the BOOTFILE
    we don't use dircache because it may not be up to date after
    USB disconnect (scanning in the background)
*/
void check_bootfile(bool do_rolo)
{
    static unsigned short wrtdate = 0;
    static unsigned short wrttime = 0;
    DIR* dir = NULL;
    struct dirent* entry = NULL;

    /* 1. open BOOTDIR and find the BOOTFILE dir entry */
    dir = opendir(BOOTDIR);

    if(!dir) return; /* do we want an error splash? */

    /* loop all files in BOOTDIR */
    while(0 != (entry = readdir(dir)))
    {
        if(!strcasecmp(entry->d_name, BOOTFILE))
        {
            struct dirinfo info = dir_get_info(dir, entry);
            /* found the bootfile */
            if(wrtdate && do_rolo)
            {
                if((info.wrtdate != wrtdate) ||
                   (info.wrttime != wrttime))
                {
                    static const char *lines[] = { ID2P(LANG_BOOT_CHANGED),
                                                   ID2P(LANG_REBOOT_NOW) };
                    static const struct text_message message={ lines, 2 };
                    button_clear_queue(); /* Empty the keyboard buffer */
                    if(gui_syncyesno_run(&message, NULL, NULL) == YESNO_YES)
                    {
                        audio_hard_stop();
                        rolo_load(BOOTDIR "/" BOOTFILE);
                    }
                }
            }
            wrtdate = info.wrtdate;
            wrttime = info.wrttime;
        }
    }
    closedir(dir);
}
#endif
#endif

/* check range, set volume and save settings */
void setvol(void)
{
    const int min_vol = sound_min(SOUND_VOLUME);
    const int max_vol = sound_max(SOUND_VOLUME);
    if (global_settings.volume < min_vol)
        global_settings.volume = min_vol;
    if (global_settings.volume > max_vol)
        global_settings.volume = max_vol;
    sound_set_volume(global_settings.volume);
    global_status.last_volume_change = current_tick;
    settings_save();
}

char* strrsplt(char* str, int c)
{
    char* s = strrchr(str, c);

    if (s != NULL)
    {
        *s++ = '\0';
    }
    else
    {
        s = str;
    }

    return s;
}

/*
 * removes the extension of filename (if it doesn't start with a .)
 * puts the result in buffer
 */
char *strip_extension(char* buffer, int buffer_size, const char *filename)
{
    char *dot = strrchr(filename, '.');
    int len;

    if (buffer_size <= 0)
    {
        return NULL;
    }

    buffer_size--;  /* Make room for end nil */

    if (dot != 0 && filename[0] != '.')
    {
        len = dot - filename;
        len = MIN(len, buffer_size);
    }
    else
    {
        len = buffer_size;
    }

    strlcpy(buffer, filename, len + 1);

    return buffer;
}

#if CONFIG_CODEC == SWCODEC
/* Play a standard sound */
void system_sound_play(enum system_sound sound)
{
    static const struct beep_params
    {
        int *setting;
        unsigned short frequency;
        unsigned short duration;
        unsigned short amplitude;
    } beep_params[] =
    {
        [SOUND_KEYCLICK] =
        { &global_settings.keyclick,
          4000, KEYCLICK_DURATION, 2500 },
        [SOUND_TRACK_SKIP] =
        { &global_settings.beep,
          2000, 100, 2500 },
        [SOUND_TRACK_NO_MORE] =
        { &global_settings.beep,
          1000, 100, 1500 },
    };

    const struct beep_params *params = &beep_params[sound];

    if (*params->setting)
    {
        beep_play(params->frequency, params->duration,
                  params->amplitude * *params->setting);
    }
}

static keyclick_callback keyclick_current_callback = NULL;
static void* keyclick_data = NULL;
void keyclick_set_callback(keyclick_callback cb, void* data)
{
    keyclick_current_callback = cb;
    keyclick_data = data;
}

/* Produce keyclick based upon button and global settings */
void keyclick_click(bool rawbutton, int action)
{
    int button = action;
    static long last_button = BUTTON_NONE;
    bool do_beep = false;

    if (!rawbutton)
        get_action_statuscode(&button);

    /* Settings filters */
    if (
#ifdef HAVE_HARDWARE_CLICK
        (global_settings.keyclick || global_settings.keyclick_hardware)
#else
        global_settings.keyclick
#endif
        )
    {
        if (global_settings.keyclick_repeats || !(button & BUTTON_REPEAT))
        {
            /* Button filters */
            if (button != BUTTON_NONE && !(button & BUTTON_REL)
                && !(button & (SYS_EVENT|BUTTON_MULTIMEDIA)) )
            {
                do_beep = true;
            }
        }
        else if ((button & BUTTON_REPEAT) && (last_button == BUTTON_NONE))
        {
            do_beep = true;
        }
#ifdef HAVE_SCROLLWHEEL
        else if (button & (BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD))
        {
            do_beep  = true;
        }
#endif
    }
    if (button&BUTTON_REPEAT)
        last_button = button;
    else
        last_button = BUTTON_NONE;

    if (do_beep && keyclick_current_callback)
        do_beep = keyclick_current_callback(action, keyclick_data);
    keyclick_current_callback = NULL;

    if (do_beep)
    {
#ifdef HAVE_HARDWARE_CLICK
        if (global_settings.keyclick)
        {
            system_sound_play(SOUND_KEYCLICK);
        }
        if (global_settings.keyclick_hardware)
        {
#if !defined(SIMULATOR)
            piezo_button_beep(false, false);
#endif
        }
#else
        system_sound_play(SOUND_KEYCLICK);
#endif
    }
}

/* Return the ReplayGain mode adjusted by other relevant settings */
static int replaygain_setting_mode(int type)
{
    switch (type)
    {
    case REPLAYGAIN_SHUFFLE:
        type = global_settings.playlist_shuffle ?
            REPLAYGAIN_TRACK : REPLAYGAIN_ALBUM;
    case REPLAYGAIN_ALBUM:
    case REPLAYGAIN_TRACK:
    case REPLAYGAIN_OFF:
    default:
        break;
    }

    return type;
}

/* Return the ReplayGain mode adjusted for display purposes */
int id3_get_replaygain_mode(const struct mp3entry *id3)
{
    if (!id3)
        return -1;

    int type = global_settings.replaygain_settings.type;
    type = replaygain_setting_mode(type);

    return (type != REPLAYGAIN_TRACK && id3->album_gain != 0) ?
        REPLAYGAIN_ALBUM : (id3->track_gain != 0 ? REPLAYGAIN_TRACK : -1);
}

/* Update DSP's replaygain from global settings */
void replaygain_update(void)
{
    struct replaygain_settings settings = global_settings.replaygain_settings;
    settings.type = replaygain_setting_mode(settings.type);
    dsp_replaygain_set_settings(&settings);
}
#endif /* CONFIG_CODEC == SWCODEC */

#endif /* !defined(__PCTOOL__) */

/* Read (up to) a line of text from fd into buffer and return number of bytes
 * read (which may be larger than the number of bytes stored in buffer). If
 * an error occurs, -1 is returned (and buffer contains whatever could be
 * read). A line is terminated by a LF char. Neither LF nor CR chars are
 * stored in buffer.
 */
int read_line(int fd, char* buffer, int buffer_size)
{
    int count = 0;
    int num_read = 0;

    errno = 0;

    while (count < buffer_size)
    {
        unsigned char c;

        if (1 != read(fd, &c, 1))
            break;

        num_read++;

        if ( c == '\n' )
            break;

        if ( c == '\r' )
            continue;

        buffer[count++] = c;
    }

    buffer[MIN(count, buffer_size - 1)] = 0;

    return errno ? -1 : num_read;
}


char* skip_whitespace(char* const str)
{
    char *s = str;

    while (isspace(*s))
        s++;

    return s;
}

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * t        - time to format, in milliseconds.
 */
void format_time(char* buf, int buf_size, long t)
{
    int const time = abs(t / 1000);
    int const hours = time / 3600;
    int const minutes = time / 60 - hours * 60;
    int const seconds = time % 60;
    const char * const sign = &"-"[t < 0 ? 0 : 1];

    if ( hours == 0 ) 
    {
        snprintf(buf, buf_size, "%s%d:%02d", sign, minutes, seconds);
    }
    else
    {
        snprintf(buf, buf_size, "%s%d:%02d:%02d", sign, hours, minutes,
                 seconds);
    }
}


/** Open a UTF-8 file and set file descriptor to first byte after BOM.
 *  If no BOM is present this behaves like open().
 *  If the file is opened for writing and O_TRUNC is set, write a BOM to
 *  the opened file and leave the file pointer set after the BOM.
 */

int open_utf8(const char* pathname, int flags)
{
    int fd;
    unsigned char bom[BOM_UTF_8_SIZE];

    fd = open(pathname, flags, 0666);
    if(fd < 0)
        return fd;

    if(flags & (O_TRUNC | O_WRONLY))
    {
        write(fd, BOM_UTF_8, BOM_UTF_8_SIZE);
    }
    else
    {
        read(fd, bom, BOM_UTF_8_SIZE);
        /* check for BOM */
        if(memcmp(bom, BOM_UTF_8, BOM_UTF_8_SIZE))
            lseek(fd, 0, SEEK_SET);
    }
    return fd;
}


#ifdef HAVE_LCD_COLOR
/*
 * Helper function to convert a string of 6 hex digits to a native colour
 */

static int hex2dec(int c)
{
    return  (((c) >= '0' && ((c) <= '9')) ? (c) - '0' :
                                            (toupper(c)) - 'A' + 10);
}

int hex_to_rgb(const char* hex, int* color)
{
    int red, green, blue;
    int i = 0;

    while ((i < 6) && (isxdigit(hex[i])))
        i++;

    if (i < 6)
        return -1;

    red = (hex2dec(hex[0]) << 4) | hex2dec(hex[1]);
    green = (hex2dec(hex[2]) << 4) | hex2dec(hex[3]);
    blue = (hex2dec(hex[4]) << 4) | hex2dec(hex[5]);

    *color = LCD_RGBPACK(red,green,blue);

    return 0;
}
#endif /* HAVE_LCD_COLOR */

#ifdef HAVE_LCD_BITMAP
/* '0'-'3' are ASCII 0x30 to 0x33 */
#define is0123(x) (((x) & 0xfc) == 0x30)
#if !defined(__PCTOOL__) || defined(CHECKWPS)
bool parse_color(enum screen_type screen, char *text, int *value)
{
    (void)text; (void)value; /* silence warnings on mono bitmap */
    (void)screen;
    
#ifdef HAVE_LCD_COLOR
    if (screens[screen].depth > 2)
    {
        if (hex_to_rgb(text, value) < 0)
            return false;
        else
            return true;
    }
#endif

#if LCD_DEPTH == 2 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH == 2)
    if (screens[screen].depth == 2)
    {
        if (text[1] != '\0' || !is0123(*text))
            return false;
        *value = *text - '0';
        return true;
    }
#endif
    return false;
}
#endif /* !defined(__PCTOOL__) || defined(CHECKWPS) */

/* only used in USB HID and set_time screen */
#if defined(USB_ENABLE_HID) || (CONFIG_RTC != 0)
int clamp_value_wrap(int value, int max, int min)
{
    if (value > max)
        return min;
    if (value < min)
        return max;
    return value;
}
#endif
#endif
#define MAX_ACTIVITY_DEPTH 12
static enum current_activity 
        current_activity[MAX_ACTIVITY_DEPTH] = {ACTIVITY_UNKNOWN};
static int current_activity_top = 0;
void push_current_activity(enum current_activity screen)
{
    current_activity[current_activity_top++] = screen;
#if defined(HAVE_LCD_BITMAP) && !defined(__PCTOOL__)
    FOR_NB_SCREENS(i)
        skinlist_set_cfg(i, NULL);
#endif
}
void pop_current_activity(void)
{
    current_activity_top--;
#if defined(HAVE_LCD_BITMAP) && !defined(__PCTOOL__)
    FOR_NB_SCREENS(i)
        skinlist_set_cfg(i, NULL);
#endif
}
enum current_activity get_current_activity(void)
{
    return current_activity[current_activity_top?current_activity_top-1:0];
}
