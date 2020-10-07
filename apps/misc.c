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
#include "language.h" /* is_lang_rtl() */

#ifdef HAVE_DIRCACHE
#include "dircache.h"
#endif
#include "file.h"
#ifndef __PCTOOL__
#include "pathfuncs.h"
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
#include "fixedpoint.h"

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
#if !defined(__PCTOOL__)
#include "bmp.h"
#include "icons.h"
#endif /* !__PCTOOL__ */
#include "bookmark.h"
#include "wps.h"
#include "playback.h"
#include "voice_thread.h"

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
    ID2P(LANG_KIBIBYTE),
    ID2P(LANG_MEBIBYTE),
    ID2P(LANG_GIBIBYTE)
};

const unsigned char * const * const kibyte_units = &byte_units[1];

/* units used with format_time_auto, option_select.c->option_get_valuestring() */
const unsigned char * const unit_strings_core[] =
{
    [UNIT_INT] = "",    [UNIT_MS]  = "ms",
    [UNIT_SEC] = "s",   [UNIT_MIN] = "min",
    [UNIT_HOUR]= "hr",  [UNIT_KHZ] = "kHz",
    [UNIT_DB]  = "dB",  [UNIT_PERCENT] = "%",
    [UNIT_MAH] = "mAh", [UNIT_PIXEL] = "px",
    [UNIT_PER_SEC] = "per sec",
    [UNIT_HERTZ] = "Hz",
    [UNIT_MB]  = "MB",  [UNIT_KBIT]  = "kb/s",
    [UNIT_PM_TICK] = "units/10ms",
};

/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf,
                       int buf_size,
                       int value,
                       const unsigned char * const *units,
                       unsigned int unit_count,
                       bool binary_scale)
{
    unsigned int scale = binary_scale ? 1024 : 1000;
    unsigned int fraction = 0;
    unsigned int unit_no = 0;
    unsigned int value_abs = (value < 0) ? -value : value;
    char tbuf[5];

    while (value_abs >= scale && unit_no < (unit_count - 1))
    {
        fraction = value_abs % scale;
        value_abs /= scale;
        unit_no++;
    }

    value = (value < 0) ? -value_abs : value_abs; /* preserve sign */
    fraction = (fraction * 1000 / scale) / 10;

    if (value_abs >= 100 || fraction >= 100 || !unit_no)
        tbuf[0] = '\0';
    else if (value_abs >= 10)
        snprintf(tbuf, sizeof(tbuf), "%01u", fraction / 10);
    else
        snprintf(tbuf, sizeof(tbuf), "%02u", fraction);

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

        if ( (p = strchr(buf, '\r')) != NULL)
        {
            *p = '\0';
            if (!next)
                next = ++p;
        }

        rc = callback(count, buf, parameters);
        if (rc < 0)
            return rc;

        count++;
        if (next)
        {
            pos = buf_size - ((intptr_t)next - (intptr_t)buf) - 1;
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

    status_save();

#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
    if(!charger_inserted())
#endif
    {
        bool batt_safe = battery_level_safe();
#if defined(HAVE_RECORDING)
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

#ifdef HAVE_DISK_STORAGE
        if (batt_safe) /* do not save on critical battery */
#endif
        {
#if defined(HAVE_RECORDING)
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

#if defined(HAVE_RECORDING)
            audio_close_recording();
#endif
            scrobbler_shutdown(true);

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
#if defined(HAVE_DIRCACHE) && defined(HAVE_DISK_STORAGE)
        else
            dircache_disable();
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
            voice_wait();
        }

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
            bookmark_autobookmark(true);
            audio_stop();
            ret = true;  /* bookmarking can make a refresh necessary */
        }
    }
#if CONFIG_CHARGING
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
static bool paused_on_unplugged = false;
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
            if ((audio_status() & AUDIO_STATUS_PAUSE) && paused_on_unplugged)
            {
                /* delay resume a bit while the engine is cranking */
                play_resume_tick = current_tick + HZ*global_settings.car_adapter_mode_delay;
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
                paused_on_unplugged = true;
            }
            else if (!waiting_to_resume_play)
                paused_on_unplugged = false;
            waiting_to_resume_play = false;
        }
    }
}

static void car_adapter_tick(void)
{
    if (waiting_to_resume_play)
    {
        if ((audio_status() & AUDIO_STATUS_PLAY) &&
                !(audio_status() & AUDIO_STATUS_PAUSE))
                waiting_to_resume_play = false;
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
static void hp_unplug_change(bool inserted)
{
    static bool headphone_caused_pause = true;

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

#ifdef HAVE_SPEAKER
    /* update speaker status */
    audio_enable_speaker(global_settings.speaker_mode);
#endif
}
#endif /*HAVE_HEADPHONE_DETECTION*/

#ifdef HAVE_LINEOUT_DETECTION
static void lo_unplug_change(bool inserted)
{
#ifdef HAVE_LINEOUT_POWEROFF
    lineout_set(inserted);
#else
#ifdef AUDIOHW_HAVE_LINEOUT
    audiohw_set_lineout_volume(0,0); /*hp vol re-set by this function as well*/
#endif
    static bool lineout_caused_pause = true;

    if (global_settings.unplug_mode)
    {
        int audio_stat = audio_status();
        if (inserted)
        {
            backlight_on();
            if ((audio_stat & AUDIO_STATUS_PLAY) &&
                    lineout_caused_pause &&
                    global_settings.unplug_mode > 1 )
                unpause_action(true, true);
            lineout_caused_pause = false;
        } else {
            if ((audio_stat & AUDIO_STATUS_PLAY) &&
                    !(audio_stat & AUDIO_STATUS_PAUSE))
            {
                lineout_caused_pause = true;
                pause_action(false, false);
            }
        }
    }
#endif /*HAVE_LINEOUT_POWEROFF*/
}
#endif /*HAVE_LINEOUT_DETECTION*/

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
            reset_runtime();
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
            hp_unplug_change(true);
            return SYS_PHONE_PLUGGED;

        case SYS_PHONE_UNPLUGGED:
            hp_unplug_change(false);
            return SYS_PHONE_UNPLUGGED;
#endif
#ifdef HAVE_LINEOUT_DETECTION
        case SYS_LINEOUT_PLUGGED:
            lo_unplug_change(true);
            return SYS_LINEOUT_PLUGGED;

        case SYS_LINEOUT_UNPLUGGED:
            lo_unplug_change(false);
            return SYS_LINEOUT_UNPLUGGED;
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
                               global_status.resume_elapsed,
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
                                   global_status.resume_elapsed,
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
    char version[32];
    int font_h, font_w;

    snprintf(version, sizeof(version), "Ver. %s", rbversion);

    lcd_clear_display();
#if defined(SANSA_CLIP) || defined(SANSA_CLIPV2) || defined(SANSA_CLIPPLUS)
    /* display the logo in the blue area of the screen (bottom 48 pixels) */
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
#ifdef SIMULATOR
    sleep(HZ); /* sim is too fast to see logo */
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
    static time_t mtime = 0;
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
            if(mtime && do_rolo)
            {
                if(info.mtime != mtime)
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
            mtime = info.mtime;
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
    if (global_settings.volume > global_settings.volume_limit)
        global_settings.volume = global_settings.volume_limit;

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
        [SOUND_LIST_EDGE_BEEP_NOWRAP] =
        { &global_settings.keyclick,
          1000, 40, 1500 },
        [SOUND_LIST_EDGE_BEEP_WRAP] =
        { &global_settings.keyclick,
          2000, 20, 1500 },

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

/* format a sound value like: -1.05 dB */
int format_sound_value(char *buf, size_t size, int snd, int val)
{
    int numdec = sound_numdecimals(snd);
    const char *unit = sound_unit(snd);
    int physval = sound_val2phys(snd, val);

    unsigned int factor = ipow(10, numdec);
    unsigned int av = abs(physval);
    unsigned int i = av / factor;
    unsigned int d = av - i*factor;
    return snprintf(buf, size, "%c%u%.*s%.*u %s", " -"[physval < 0],
                    i, numdec, ".", numdec, d, unit);
}

#endif /* !defined(__PCTOOL__) */

/* Read (up to) a line of text from fd into buffer and return number of bytes
 * read (which may be larger than the number of bytes stored in buffer). If
 * an error occurs, -1 is returned (and buffer contains whatever could be
 * read). A line is terminated by a LF char. Neither LF nor CR chars are
 * stored in buffer.
 */
int read_line(int fd, char* buffer, int buffer_size)
{
    if (!buffer || buffer_size-- <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    unsigned char rdbuf[32];
    off_t rdbufend = 0;
    int rdbufidx = 0;
    int count = 0;
    int num_read = 0;

    while (count < buffer_size)
    {
        if (rdbufidx >= rdbufend)
        {
            rdbufidx = 0;
            rdbufend = read(fd, rdbuf, sizeof (rdbuf));

            if (rdbufend <= 0)
                break;

            num_read += rdbufend;
        }

        int c = rdbuf[rdbufidx++];

        if (c == '\n')
            break;

        if (c == '\r')
            continue;

        buffer[count++] = c;
    }

    rdbufidx -= rdbufend;

    if (rdbufidx < 0)
    {
        /* "put back" what wasn't read from the buffer */
        num_read += rdbufidx;
        rdbufend = lseek(fd, rdbufidx, SEEK_CUR);
    }

    buffer[count] = '\0';

    return rdbufend >= 0 ? num_read : -1;
}


char* skip_whitespace(char* const str)
{
    char *s = str;

    while (isspace(*s))
        s++;

    return s;
}

#if !defined(CHECKWPS) && !defined(DBTOOL)
/*  time_split_units()
    split time values depending on base unit
    unit_idx: UNIT_HOUR, UNIT_MIN, UNIT_SEC, UNIT_MS
    abs_value: absolute time value
    units_in: array of unsigned ints with UNIT_IDX_TIME_COUNT fields
*/
unsigned int time_split_units(int unit_idx, unsigned long abs_val,
                              unsigned long (*units_in)[UNIT_IDX_TIME_COUNT])
{
    unsigned int base_idx = UNIT_IDX_HR;
    unsigned long hours;
    unsigned long minutes  = 0;
    unsigned long seconds  = 0;
    unsigned long millisec = 0;

    switch (unit_idx & UNIT_IDX_MASK) /*Mask off upper bits*/
    {
            case UNIT_MS:
                base_idx = UNIT_IDX_MS;
                millisec = abs_val;
                abs_val  = abs_val  /  1000U;
                millisec = millisec - (1000U * abs_val);
                /* fallthrough and calculate the rest of the units */
            case UNIT_SEC:
                if (base_idx == UNIT_IDX_HR)
                    base_idx = UNIT_IDX_SEC;
                seconds  = abs_val;
                abs_val  = abs_val  / 60U;
                seconds  = seconds - (60U * abs_val);
                /* fallthrough and calculate the rest of the units */
            case UNIT_MIN:
                if (base_idx == UNIT_IDX_HR)
                    base_idx = UNIT_IDX_MIN;
                minutes  = abs_val;
                abs_val  = abs_val / 60U;
                minutes  = minutes -(60U * abs_val);
                /* fallthrough and calculate the rest of the units */
            case UNIT_HOUR:
            default:
                hours    = abs_val;
                break;
    }

    (*units_in)[UNIT_IDX_HR]  = hours;
    (*units_in)[UNIT_IDX_MIN] = minutes;
    (*units_in)[UNIT_IDX_SEC] = seconds;
    (*units_in)[UNIT_IDX_MS]  = millisec;

    return base_idx;
}

/* format_time_auto - return an auto ranged time string;
   buffer:  needs to be at least 25 characters for full range

   unit_idx: specifies lowest or base index of the value
   add | UNIT_LOCK_ to keep place holder of units that would normally be
   discarded.. For instance, UNIT_LOCK_HR would keep the hours place, ex: string
   00:10:10 (0 HRS 10 MINS 10 SECONDS) normally it would return as 10:10
   add | UNIT_TRIM_ZERO to supress leading zero on the largest unit

   value: should be passed in the same form as unit_idx

   supress_unit: may be set to true and in this case the
   hr, min, sec, ms identifiers will be left off the resulting string but
   since right to left languages are handled it is advisable to leave units
   as an indication of the text direction
*/

const char *format_time_auto(char *buffer, int buf_len, long value,
                                  int unit_idx, bool supress_unit)
{
    const char * const sign        = &"-"[value < 0 ? 0 : 1];
    bool               is_rtl      = lang_is_rtl();
    char               timebuf[25]; /* -2147483648:00:00.00\0 */
    int                len, left_offset;
    unsigned char      base_idx, max_idx;

    unsigned long  units_in[UNIT_IDX_TIME_COUNT];
    unsigned char  fwidth[UNIT_IDX_TIME_COUNT] =
                   {
                        [UNIT_IDX_HR]  = 0, /* hr is variable length */
                        [UNIT_IDX_MIN] = 2,
                        [UNIT_IDX_SEC] = 2,
                        [UNIT_IDX_MS]  = 3,
                   }; /* {0,2,2,3}; Field Widths */
    unsigned char  offsets[UNIT_IDX_TIME_COUNT] =
                   {
                        [UNIT_IDX_HR]  = 10,/* ?:59:59.999 Std offsets    */
                        [UNIT_IDX_MIN] = 7, /*0?:+1:+4.+7 need calculated */
                        [UNIT_IDX_SEC] = 4,/* 999.59:59:0  RTL offsets    */
                        [UNIT_IDX_MS]  = 0,/* 0  .4 :7 :10 won't change   */
                   }; /* {10,7,4,0}; Offsets */
    const uint16_t unitlock[UNIT_IDX_TIME_COUNT] =
                   {
                        [UNIT_IDX_HR]  = UNIT_LOCK_HR,
                        [UNIT_IDX_MIN] = UNIT_LOCK_MIN,
                        [UNIT_IDX_SEC] = UNIT_LOCK_SEC,
                        [UNIT_IDX_MS]  = 0,
                   }; /* unitlock */
    const uint16_t units[UNIT_IDX_TIME_COUNT] =
                   {
                        [UNIT_IDX_HR]  = UNIT_HOUR,
                        [UNIT_IDX_MIN] = UNIT_MIN,
                        [UNIT_IDX_SEC] = UNIT_SEC,
                        [UNIT_IDX_MS]  = UNIT_MS,
                   }; /* units */

#if 0 /* unused */
    if (idx_pos != NULL)
    {
        (*idx_pos)[0] = MIN((*idx_pos)[0], UNIT_IDX_TIME_COUNT - 1);
        unit_idx |= unitlock[(*idx_pos)[0]];
    }
#endif

    base_idx = time_split_units(unit_idx, labs(value), &units_in);

    if (units_in[UNIT_IDX_HR] || (unit_idx & unitlock[UNIT_IDX_HR]))
        max_idx = UNIT_IDX_HR;
    else if (units_in[UNIT_IDX_MIN] || (unit_idx & unitlock[UNIT_IDX_MIN]))
        max_idx = UNIT_IDX_MIN;
    else if (units_in[UNIT_IDX_SEC] || (unit_idx & unitlock[UNIT_IDX_SEC]))
        max_idx = UNIT_IDX_SEC;
    else if (units_in[UNIT_IDX_MS])
        max_idx = UNIT_IDX_MS;
    else /* value is 0 */
        max_idx = base_idx;

    if (!is_rtl)
    {
        len = snprintf(timebuf, sizeof(timebuf),
                       "%02lu:%02lu:%02lu.%03lu",
                       units_in[UNIT_IDX_HR],
                       units_in[UNIT_IDX_MIN],
                       units_in[UNIT_IDX_SEC],
                       units_in[UNIT_IDX_MS]);

        fwidth[UNIT_IDX_HR]   = len - offsets[UNIT_IDX_HR];

        /* calculate offsets of the other fields based on length of previous */
        offsets[UNIT_IDX_MS]  = fwidth[UNIT_IDX_HR] + offsets[UNIT_IDX_MIN];
        offsets[UNIT_IDX_SEC] = fwidth[UNIT_IDX_HR] + offsets[UNIT_IDX_SEC];
        offsets[UNIT_IDX_MIN] = fwidth[UNIT_IDX_HR] + 1;
        offsets[UNIT_IDX_HR]  = 0;

        timebuf[offsets[base_idx] + fwidth[base_idx]] = '\0';

        left_offset  = -(offsets[max_idx]);
        left_offset += strlcpy(buffer, sign, buf_len);

        /* trim leading zero on the max_idx */
        if ((unit_idx & UNIT_TRIM_ZERO) == UNIT_TRIM_ZERO &&
            timebuf[offsets[max_idx]] == '0' && fwidth[max_idx] > 1)
        {
            offsets[max_idx]++;
        }

        strlcat(buffer, &timebuf[offsets[max_idx]], buf_len);

        if (!supress_unit)
        {
            strlcat(buffer, " ", buf_len);
            strlcat(buffer, unit_strings_core[units[max_idx]], buf_len);
        }
    }
    else /*RTL Languages*/
    {
        len = snprintf(timebuf, sizeof(timebuf),
                       "%03lu.%02lu:%02lu:%02lu",
                       units_in[UNIT_IDX_MS],
                       units_in[UNIT_IDX_SEC],
                       units_in[UNIT_IDX_MIN],
                       units_in[UNIT_IDX_HR]);

        fwidth[UNIT_IDX_HR] = len - offsets[UNIT_IDX_HR];

        left_offset = -(offsets[base_idx]);

        /* trim leading zero on the max_idx */
        if ((unit_idx & UNIT_TRIM_ZERO) == UNIT_TRIM_ZERO &&
            timebuf[offsets[max_idx]] == '0' && fwidth[max_idx] > 1)
        {
            timebuf[offsets[max_idx]] = timebuf[offsets[max_idx]+1];
            fwidth[max_idx]--;
        }

        timebuf[offsets[max_idx] + fwidth[max_idx]] = '\0';

        if (!supress_unit)
        {
            strlcpy(buffer, unit_strings_core[units[max_idx]], buf_len);
            left_offset += strlcat(buffer, " ", buf_len);
            strlcat(buffer, &timebuf[offsets[base_idx]], buf_len);
        }
        else
            strlcpy(buffer, &timebuf[offsets[base_idx]], buf_len);

        strlcat(buffer, sign, buf_len);
    }
#if 0 /* unused */
    if (idx_pos != NULL)
    {
        (*idx_pos)[1]= fwidth[*(idx_pos)[0]];
        (*idx_pos)[0]= left_offset + offsets[(*idx_pos)[0]];
    }
#endif

    return buffer;
}

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * t        - time to format, in milliseconds.
 */
void format_time(char* buf, int buf_size, long t)
{
    unsigned long units_in[UNIT_IDX_TIME_COUNT] = {0};
    time_split_units(UNIT_MS, labs(t), &units_in);
    int hashours = units_in[UNIT_IDX_HR] > 0;
    snprintf(buf, buf_size, "%.*s%.0lu%.*s%.*lu:%.2lu",
             t < 0, "-", units_in[UNIT_IDX_HR], hashours, ":",
             hashours+1, units_in[UNIT_IDX_MIN], units_in[UNIT_IDX_SEC]);
}
#endif /* !defined(CHECKWPS) && !defined(DBTOOL)*/

/**
 * Splits str at each occurence of split_char and puts the substrings into vector,
 * but at most vector_lenght items. Empty substrings are ignored.
 *
 * Modifies str by replacing each split_char following a substring with nul
 *
 * Returns the number of substrings found, i.e. the number of valid strings
 * in vector
 */
int split_string(char *str, const char split_char, char *vector[], const int vector_length)
{
    int i;
    char *p = str;

    /* skip leading splitters */
    while(*p == split_char) p++;

    /* *p in the condition takes care of trailing splitters */
    for(i = 0; p && *p && i < vector_length; i++)
    {
        vector[i] = p;
        if ((p = strchr(p, split_char)))
        {
            *p++ = '\0';
            while(*p == split_char) p++; /* skip successive splitters */
        }
    }

    return i;
}


/** Open a UTF-8 file and set file descriptor to first byte after BOM.
 *  If no BOM is present this behaves like open().
 *  If the file is opened for writing and O_TRUNC is set, write a BOM to
 *  the opened file and leave the file pointer set after the BOM.
 */

int open_utf8(const char* pathname, int flags)
{
    ssize_t ret;
    int fd;
    unsigned char bom[BOM_UTF_8_SIZE];

    fd = open(pathname, flags, 0666);
    if(fd < 0)
        return fd;

    if(flags & (O_TRUNC | O_WRONLY))
    {
        ret = write(fd, BOM_UTF_8, BOM_UTF_8_SIZE);
    }
    else
    {
        ret = read(fd, bom, BOM_UTF_8_SIZE);
        /* check for BOM */
        if (ret == BOM_UTF_8_SIZE)
        {
            if(memcmp(bom, BOM_UTF_8, BOM_UTF_8_SIZE))
                lseek(fd, 0, SEEK_SET);
        }
    }
    /* read or write failure, do not continue */
    if (ret < 0)
        close(fd);

    return ret >= 0 ? fd : -1;
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


#ifndef __PCTOOL__

#define MAX_ACTIVITY_DEPTH 12
static enum current_activity
        current_activity[MAX_ACTIVITY_DEPTH] = {ACTIVITY_UNKNOWN};
static int current_activity_top = 0;
void push_current_activity(enum current_activity screen)
{
    current_activity[current_activity_top++] = screen;
    FOR_NB_SCREENS(i)
    {
        skinlist_set_cfg(i, NULL);
        skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_ALL);
    }
}

void pop_current_activity(void)
{
    current_activity_top--;
    FOR_NB_SCREENS(i)
    {
        skinlist_set_cfg(i, NULL);
        skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_ALL);
    }
}
enum current_activity get_current_activity(void)
{
    return current_activity[current_activity_top?current_activity_top-1:0];
}

#endif
