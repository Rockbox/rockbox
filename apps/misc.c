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
#include "config.h"
#include "misc.h"
#include "lcd.h"
#include "file.h"
#ifdef __PCTOOL__
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef WPSEDITOR
#include "string.h"
#endif
#else
#include "sprintf.h"
#include "appevents.h"
#include "lang.h"
#include "string.h"
#include "dir.h"
#include "lcd-remote.h"
#include "errno.h"
#include "system.h"
#include "timefuncs.h"
#include "screens.h"
#include "talk.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
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
#include "gui/gwps-common.h"
#include "bookmark.h"

#include "playback.h"

#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_IPODSTYLE)
#include "rolo.h"
#include "yesno.h"
#endif
#endif

/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf, int buf_size, int value,
                       const unsigned char **units, bool bin_scale)
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
        if (strlen(tbuf))
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

/* Performance optimized version of the previous function. */
int fast_readline(int fd, char *buf, int buf_size, void *parameters,
                  int (*callback)(int n, const char *buf, void *parameters))
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
        
        if ( (p = strchr(buf, '\r')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        else
            p = buf;
        
        if ( (p = strchr(p, '\n')) != NULL)
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
    scrobbler_shutdown();
    playlist_shutdown();
    tree_flush();
    call_storage_idle_notifys(true); /*doesnt work on usb and shutdown from ata thread */
}

static void system_restore(void)
{
    tree_restore();
    scrobbler_init();
}

static bool clean_shutdown(void (*callback)(void *), void *parameter)
{
#ifdef SIMULATOR
    (void)callback;
    (void)parameter;
    bookmark_autobookmark();
    call_storage_idle_notifys(true);
    exit(0);
#else
    long msg_id = -1;
    int i;

    scrobbler_poweroff();

#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
    if(!charger_inserted())
#endif
    {
        bool batt_safe = battery_level_safe();
        int audio_stat = audio_status();

        FOR_NB_SCREENS(i)
            screens[i].clear_display();

        if (batt_safe)
        {
#ifdef HAVE_TAGCACHE
            if (!tagcache_prepare_shutdown())
            {
                cancel_shutdown();
                splash(HZ, ID2P(LANG_TAGCACHE_BUSY));
                return false;
            }
#endif
            if (battery_level() > 10)
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
        
        if (global_settings.fade_on_stop
            && (audio_stat & AUDIO_STATUS_PLAY))
        {
            fade(false, false);
        }

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
            bookmark_autobookmark();

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
#endif
    return false;
}

bool list_stop_handler(void)
{
    bool ret = false;

    /* Stop the music if it is playing */
    if(audio_status()) 
    {
        if (!global_settings.party_mode) 
        {
            if (global_settings.fade_on_stop)
                fade(false, false);
            bookmark_autobookmark();
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
                if (global_settings.fade_on_stop)
                    fade(false, false);
                else
                    audio_pause();
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
            if ((audio_stat & AUDIO_STATUS_PLAY) &&
                    headphone_caused_pause &&
                    global_settings.unplug_mode > 1 )
                audio_resume();
            backlight_on();
            headphone_caused_pause = false;
        } else {
            if ((audio_stat & AUDIO_STATUS_PLAY) &&
                    !(audio_stat & AUDIO_STATUS_PAUSE))
            {
                headphone_caused_pause = true;
                audio_pause();

                if (global_settings.unplug_rw)
                {
                    if (audio_current_track()->elapsed >
                            (unsigned long)(global_settings.unplug_rw*1000))
                        audio_ff_rewind(audio_current_track()->elapsed -
                                (global_settings.unplug_rw*1000));
                    else
                        audio_ff_rewind(0);
                }
            }
        }
    }
}
#endif

long default_event_handler_ex(long event, void (*callback)(void *), void *parameter)
{
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
#if (CONFIG_STORAGE & STORAGE_MMC)
            if (!mmc_touched() ||
                (mmc_remove_request() == SYS_HOTSWAP_EXTRACTED))
#endif
            {
                system_flush();
#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_IPODSTYLE)
                check_bootfile(false); /* gets initial size */
#endif
#endif
                usb_screen();
#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_IPODSTYLE)
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
            return SYS_CHARGER_DISCONNECTED;

        case SYS_CAR_ADAPTER_RESUME:
            audio_resume();
            return SYS_CAR_ADAPTER_RESUME;
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

    snprintf(version, sizeof(version), "Ver. %s", appsversion);

    lcd_clear_display();
#ifdef SANSA_CLIP   /* display the logo in the blue area of the screen */
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(version)*font_w)/2),
               0, (unsigned char *)version);
    lcd_bitmap(rockboxlogo, 0, 16, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
#else
    lcd_bitmap(rockboxlogo, 0, 10, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
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
    lcd_puts_scroll(0, 1, appsversion);
#endif
    lcd_update();

#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    lcd_remote_bitmap(remote_rockboxlogo, 0, 10, BMPWIDTH_remote_rockboxlogo,
                      BMPHEIGHT_remote_rockboxlogo);
    lcd_remote_setfont(FONT_SYSFIXED);
    lcd_remote_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_remote_putsxy((LCD_REMOTE_WIDTH/2) - ((strlen(version)*font_w)/2),
                       LCD_REMOTE_HEIGHT-font_h, (unsigned char *)version);
    lcd_remote_setfont(FONT_UI);
    lcd_remote_update();
#endif

    return 0;
}

#if CONFIG_CODEC == SWCODEC
int get_replaygain_mode(bool have_track_gain, bool have_album_gain)
{
    int type;

    bool track = ((global_settings.replaygain_type == REPLAYGAIN_TRACK)
        || ((global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
            && global_settings.playlist_shuffle));

    type = (!track && have_album_gain) ? REPLAYGAIN_ALBUM 
        : have_track_gain ? REPLAYGAIN_TRACK : -1;
    
    return type;
}
#endif

#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_IPODSTYLE)
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
            /* found the bootfile */
            if(wrtdate && do_rolo)
            {
                if((entry->wrtdate != wrtdate) ||
                   (entry->wrttime != wrttime))
                {
                    static const char *lines[] = { ID2P(LANG_BOOT_CHANGED),
                                                   ID2P(LANG_REBOOT_NOW) };
                    static const struct text_message message={ lines, 2 };
                    button_clear_queue(); /* Empty the keyboard buffer */
                    if(gui_syncyesno_run(&message, NULL, NULL) == YESNO_YES)
                    rolo_load(BOOTDIR "/" BOOTFILE);
                }
            }
            wrtdate = entry->wrtdate;
            wrttime = entry->wrttime;
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

char* skip_whitespace(char* const str)
{
    char *s = str;

    while (isspace(*s)) 
        s++;
    
    return s;
}

/* Test file existence, using dircache of possible */
bool file_exists(const char *file)
{
    int fd;

    if (!file || strlen(file) <= 0)
        return false;

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled())
        return (dircache_get_entry_ptr(file) != NULL);
#endif

    fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;
    close(fd);
    return true;
}

bool dir_exists(const char *path)
{
    DIR* d = opendir(path);
    if (!d)
        return false;
    closedir(d);
    return true;
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
        strncpy(buffer, filename, len);
    }
    else
    {
        len = buffer_size;
        strncpy(buffer, filename, buffer_size);
    }

    buffer[len] = 0;

    return buffer;
}
#endif /* !defined(__PCTOOL__) */

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * t        - time to format, in milliseconds.
 */
void format_time(char* buf, int buf_size, long t)
{
    if ( t < 3600000 ) 
    {
      snprintf(buf, buf_size, "%d:%02d",
               (int) (t / 60000), (int) (t % 60000 / 1000));
    } 
    else
    {
      snprintf(buf, buf_size, "%d:%02d:%02d",
               (int) (t / 3600000), (int) (t % 3600000 / 60000),
               (int) (t % 60000 / 1000));
    }
}


/** Open a UTF-8 file and set file descriptor to first byte after BOM.
 *  If no BOM is present this behaves like open().
 *  If the file is opened for writing and O_TRUNC is set, write a BOM to
 *  the opened file and leave the file pointer set after the BOM.
 */
#define BOM "\xef\xbb\xbf"
#define BOM_SIZE 3

int open_utf8(const char* pathname, int flags)
{
    int fd;
    unsigned char bom[BOM_SIZE];

    fd = open(pathname, flags);
    if(fd < 0)
        return fd;

    if(flags & (O_TRUNC | O_WRONLY))
    {
        write(fd, BOM, BOM_SIZE);
    }
    else
    {
        read(fd, bom, BOM_SIZE);
        /* check for BOM */
        if(memcmp(bom, BOM, BOM_SIZE))
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
/* A simplified scanf - used (at time of writing) by wps parsing functions.

   fmt - char array specifying the format of each list option.  Valid values
         are:  d - int
               s - string (sets pointer to string, without copying)
               c - hex colour (RGB888 - e.g. ff00ff)
               g - greyscale "colour" (0-3)
   set_vals - if not NULL 1 is set in the bitplace if the item was read OK
                0 if not read.
                first item is LSB, (max 32 items! )
                Stops parseing if an item is invalid unless the item == '-'
   sep - list separator (e.g. ',' or '|')
   str  - string to parse, must be terminated by 0 or sep
   ... - pointers to store the parsed values

   return value - pointer to char after parsed data, 0 if there was an error.

*/

/* '0'-'3' are ASCII 0x30 to 0x33 */
#define is0123(x) (((x) & 0xfc) == 0x30)

const char* parse_list(const char *fmt, uint32_t *set_vals,
                       const char sep, const char* str, ...)
{
    va_list ap;
    const char* p = str, *f = fmt;
    const char** s;
    int* d;
    bool set;
    int i=0;

    va_start(ap, str);
    if (set_vals)
        *set_vals = 0;
    while (*fmt)
    {
        /* Check for separator, if we're not at the start */
        if (f != fmt)
        {
            if (*p != sep)
                goto err;
            p++;
        }
        set = false;
        switch (*fmt++) 
        {
            case 's': /* string - return a pointer to it (not a copy) */
                s = va_arg(ap, const char **);

                *s = p;
                while (*p && *p != sep)
                   p++;
                set = (s[0][0]!='-') && (s[0][1]!=sep) ;
                break;

            case 'd': /* int */
                d = va_arg(ap, int*);
                if (!isdigit(*p))
                {
                    if (!set_vals || *p != '-')
                        goto err;
                    while (*p && *p != sep)
                        p++;
                }
                else
                {
                   *d = *p++ - '0';
                    while (isdigit(*p))
                       *d = (*d * 10) + (*p++ - '0');
                    set = true;
                }

                break;

#ifdef HAVE_LCD_COLOR
            case 'c': /* colour (rrggbb - e.g. f3c1a8) */
                d = va_arg(ap, int*);

                if (hex_to_rgb(p, d) < 0)
                {
                    if (!set_vals || *p != '-')
                        goto err;
                    while (*p && *p != sep)
                        p++;
                }
                else
                {
                    p += 6;
                    set = true;
                }

                break;
#endif

#if LCD_DEPTH == 2 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH == 2)
            case 'g': /* greyscale colour (0-3) */
                d = va_arg(ap, int*);

                if (is0123(*p))
                {
                    *d = *p++ - '0';
                    set = true;
                }
                else if (!set_vals || *p != '-')
                    goto err;
                else
                {
                    while (*p && *p != sep)
                        p++;
                }

                break;
#endif

            default:  /* Unknown format type */
                goto err;
                break;
        }
        if (set_vals && set)
            *set_vals |= (1<<i);
        i++;
    }

    va_end(ap);
    return p;

err:
    va_end(ap);
    return 0;
}
#endif
