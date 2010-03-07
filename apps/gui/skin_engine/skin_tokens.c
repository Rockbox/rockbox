/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002-2007 Björn Stenberg
 * Copyright (C) 2007-2008 Nicolas Pennequin
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
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "action.h"
#include "system.h"
#include "settings.h"
#include "settings_list.h"
#include "rbunicode.h"
#include "timefuncs.h"
#include "status.h"
#include "power.h"
#include "powermgmt.h"
#include "sound.h"
#include "debug.h"
#ifdef HAVE_LCD_CHARCELLS
#include "hwcompat.h"
#endif
#include "abrepeat.h"
#include "lang.h"
#include "misc.h"
#include "led.h"
#ifdef HAVE_LCD_BITMAP
/* Image stuff */
#include "albumart.h"
#endif
#include "dsp.h"
#include "playlist.h"
#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#include "tdspeed.h"
#endif
#include "viewport.h"

#include "wps_internals.h"
#include "root_menu.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#include "pcm_record.h"
#endif
#include "language.h"
#include "usb.h"

extern struct wps_state wps_state;

static const char* get_codectype(const struct mp3entry* id3)
{
    if (id3 && id3->codectype < AFMT_NUM_CODECS) {
        return audio_formats[id3->codectype].label;
    } else {
        return NULL;
    }
}

/* Extract a part from a path.
 *
 * buf      - buffer extract part to.
 * buf_size - size of buffer.
 * path     - path to extract from.
 * level    - what to extract. 0 is file name, 1 is parent of file, 2 is
 *            parent of parent, etc.
 *
 * Returns buf if the desired level was found, NULL otherwise.
 */
char* get_dir(char* buf, int buf_size, const char* path, int level)
{
    const char* sep;
    const char* last_sep;
    int len;

    sep = path + strlen(path);
    last_sep = sep;

    while (sep > path)
    {
        if ('/' == *(--sep))
        {
            if (!level)
                break;

            level--;
            last_sep = sep - 1;
        }
    }

    if (level || (last_sep <= sep))
        return NULL;

    len = MIN(last_sep - sep, buf_size - 1);
    strlcpy(buf, sep + 1, len + 1);
    return buf;
}

#if (CONFIG_CODEC != MAS3507D)
/* A helper to determine the enum value for pitch/speed.

   When there are two choices (i.e. boolean), return 1 if the value is
   different from normal value and 2 if the value is the same as the
   normal value.  E.g. "%?Sp<%Sp>" would show the pitch only when
   playing at a modified pitch.

   When there are more than two choices (i.e. enum), the left half of
   the choices are to show 0..normal range, and the right half of the
   choices are to show values over that.  The last entry is used when
   it is set to the normal setting, following the rockbox convention
   to use the last entry for special values.

   E.g.

   2 items: %?Sp<0..99 or 101..infinity|100>
   3 items: %?Sp<0..99|101..infinity|100>
   4 items: %?Sp<0..49|50..99|101..infinity|100>
   5 items: %?Sp<0..49|50..99|101..149|150..infinity|100>
   6 items: %?Sp<0..33|34..66|67..99|101..133|134..infinity|100>
   7 items: %?Sp<0..33|34..66|67..99|101..133|134..167|167..infinity|100>
*/
static int pitch_speed_enum(int range, int32_t val, int32_t normval)
{
    int center;
    int n;

    if (range < 3)
        return (val == normval) + 1;
    if (val == normval)
        return range;
    center = range / 2;
    n = (center * val) / normval + 1;
    return (range <= n) ? (range - 1) : n;
}
#endif


/* All tokens which only need the info to return a value go in here */
const char *get_id3_token(struct wps_token *token, struct mp3entry *id3,
                          char *buf, int buf_size, int limit, int *intval)
{
    struct wps_state *state = &wps_state;
    if (id3)
    {
        unsigned long length = id3->length;
        unsigned long elapsed = id3->elapsed + state->ff_rewind_count;
        switch (token->type)
        {
            case WPS_TOKEN_METADATA_ARTIST:
                return id3->artist;
            case WPS_TOKEN_METADATA_COMPOSER:
                return id3->composer;
            case WPS_TOKEN_METADATA_ALBUM:
                return id3->album;
            case WPS_TOKEN_METADATA_ALBUM_ARTIST:
                return id3->albumartist;
            case WPS_TOKEN_METADATA_GROUPING:
                return id3->grouping;
            case WPS_TOKEN_METADATA_GENRE:
                return id3->genre_string;
            case WPS_TOKEN_METADATA_DISC_NUMBER:
                if (id3->disc_string)
                    return id3->disc_string;
                if (id3->discnum) {
                    snprintf(buf, buf_size, "%d", id3->discnum);
                    return buf;
                }
                return NULL;
            case WPS_TOKEN_METADATA_TRACK_NUMBER:
                if (id3->track_string)
                    return id3->track_string;
                if (id3->tracknum) {
                    snprintf(buf, buf_size, "%d", id3->tracknum);
                    return buf;
                }
                return NULL;
            case WPS_TOKEN_METADATA_TRACK_TITLE:
                return id3->title;
            case WPS_TOKEN_METADATA_VERSION:
                switch (id3->id3version)
                {
                    case ID3_VER_1_0:
                        return "1";
                    case ID3_VER_1_1:
                        return "1.1";
                    case ID3_VER_2_2:
                        return "2.2";
                    case ID3_VER_2_3:
                        return "2.3";
                    case ID3_VER_2_4:
                        return "2.4";
                    default:
                        break;
                }
                return NULL;
            case WPS_TOKEN_METADATA_YEAR:
                if( id3->year_string )
                    return id3->year_string;
                if (id3->year) {
                    snprintf(buf, buf_size, "%d", id3->year);
                    return buf;
                }
                return NULL;
            case WPS_TOKEN_METADATA_COMMENT:
                return id3->comment;
            case WPS_TOKEN_FILE_PATH:
                return id3->path;
            case WPS_TOKEN_FILE_BITRATE:
                if(id3->bitrate)
                    snprintf(buf, buf_size, "%d", id3->bitrate);
                else
                    return "?";
                return buf;
            case WPS_TOKEN_TRACK_TIME_ELAPSED:
                format_time(buf, buf_size, elapsed);
                return buf;

            case WPS_TOKEN_TRACK_TIME_REMAINING:
                format_time(buf, buf_size, length - elapsed);
                return buf;

            case WPS_TOKEN_TRACK_LENGTH:
                format_time(buf, buf_size, length);
                return buf;

            case WPS_TOKEN_TRACK_ELAPSED_PERCENT:
                if (length <= 0)
                    return NULL;

                if (intval)
                {
                    *intval = limit * elapsed / length + 1;
                }
                snprintf(buf, buf_size, "%d", 100 * elapsed / length);
                return buf;


            case WPS_TOKEN_FILE_CODEC:
                if (intval)
                {
                    if(id3->codectype == AFMT_UNKNOWN)
                        *intval = AFMT_NUM_CODECS;
                    else
                        *intval = id3->codectype;
                }
                return get_codectype(id3);

            case WPS_TOKEN_FILE_FREQUENCY:
                snprintf(buf, buf_size, "%ld", id3->frequency);
                return buf;
            case WPS_TOKEN_FILE_FREQUENCY_KHZ:
                /* ignore remainders < 100, so 22050 Hz becomes just 22k */
                if ((id3->frequency % 1000) < 100)
                    snprintf(buf, buf_size, "%ld", id3->frequency / 1000);
                else
                    snprintf(buf, buf_size, "%ld.%d",
                            id3->frequency / 1000,
                            (id3->frequency % 1000) / 100);
                return buf;
            case WPS_TOKEN_FILE_NAME:
                if (get_dir(buf, buf_size, id3->path, 0)) {
                    /* Remove extension */
                    char* sep = strrchr(buf, '.');
                    if (NULL != sep) {
                        *sep = 0;
                    }
                    return buf;
                }
                return NULL;
            case WPS_TOKEN_FILE_NAME_WITH_EXTENSION:
                return get_dir(buf, buf_size, id3->path, 0);
            case WPS_TOKEN_FILE_SIZE:
                snprintf(buf, buf_size, "%ld", id3->filesize / 1024);
                return buf;
            case WPS_TOKEN_FILE_VBR:
                return (id3->vbr) ? "(avg)" : NULL;
            case WPS_TOKEN_FILE_DIRECTORY:
                return get_dir(buf, buf_size, id3->path, token->value.i);

#ifdef HAVE_TAGCACHE
        case WPS_TOKEN_DATABASE_PLAYCOUNT:
            if (intval)
                *intval = id3->playcount + 1;
            snprintf(buf, buf_size, "%ld", id3->playcount);
            return buf;
        case WPS_TOKEN_DATABASE_RATING:
            if (intval)
                *intval = id3->rating + 1;
            snprintf(buf, buf_size, "%ld", id3->rating);
            return buf;
        case WPS_TOKEN_DATABASE_AUTOSCORE:
            if (intval)
                *intval = id3->score + 1;
            snprintf(buf, buf_size, "%ld", id3->score);
            return buf;
#endif

            default:
                return NULL;
        }
    }
    else /* id3 == NULL, handle the error based on the expected return type */
    {
        switch (token->type)
        {
            /* Most tokens expect NULL on error so leave that for the default case,
             * The ones that expect "0" need to be handled */
            case WPS_TOKEN_FILE_FREQUENCY:
            case WPS_TOKEN_FILE_FREQUENCY_KHZ:
            case WPS_TOKEN_FILE_SIZE:
#ifdef HAVE_TAGCACHE
            case WPS_TOKEN_DATABASE_PLAYCOUNT:
            case WPS_TOKEN_DATABASE_RATING:
            case WPS_TOKEN_DATABASE_AUTOSCORE:
#endif
                if (intval)
                    *intval = 0;
                return "0";
            default:
                return NULL;
        }
    }
    return buf;
}

/* Return the tags value as text. buf should be used as temp storage if needed.

   intval is used with conditionals/enums: when this function is called,
   intval should contain the number of options in the conditional/enum.
   When this function returns, intval is -1 if the tag is non numeric or,
   if the tag is numeric, *intval is the enum case we want to go to (between 1
   and the original value of *intval, inclusive).
   When not treating a conditional/enum, intval should be NULL.
*/
const char *get_token_value(struct gui_wps *gwps,
                           struct wps_token *token,
                           char *buf, int buf_size,
                           int *intval)
{
    if (!gwps)
        return NULL;

    struct wps_data *data = gwps->data;
    struct wps_state *state = gwps->state;
    struct mp3entry *id3; /* Think very carefully about using this. 
                             maybe get_id3_token() is the better place? */
    const char *out_text = NULL;

    if (!data || !state)
        return NULL;


    if (token->next)
        id3 = state->nid3;
    else
        id3 = state->id3;
        
#if CONFIG_RTC
    struct tm* tm = NULL;

    /* if the token is an RTC one, update the time
       and do the necessary checks */
    if (token->type >= WPS_TOKENS_RTC_BEGIN
        && token->type <= WPS_TOKENS_RTC_END)
    {
        tm = get_time();

        if (!valid_time(tm))
            return NULL;
    }
#endif

    int limit = 1;
    if (intval)
    {
        limit = *intval;
        *intval = -1;
    }
    
    out_text = get_id3_token(token, id3, buf, buf_size, limit, intval);
    if (out_text)
        return out_text;

    switch (token->type)
    {
        case WPS_TOKEN_CHARACTER:
            if (token->value.c == '\n')
                return NULL;
            return &(token->value.c);

        case WPS_TOKEN_STRING:
            return (char*)token->value.data;
            
        case WPS_TOKEN_TRANSLATEDSTRING:
            return (char*)P2STR(ID2P(token->value.i));

        case WPS_TOKEN_PLAYLIST_ENTRIES:
            snprintf(buf, buf_size, "%d", playlist_amount());
            return buf;
        
        case WPS_TOKEN_LIST_TITLE_TEXT:
            return (char*)token->value.data;
        case WPS_TOKEN_LIST_TITLE_ICON:
            if (intval)
                *intval = MIN(token->value.i, limit-1);
            snprintf(buf, buf_size, "%d", token->value.i);
            return buf;

        case WPS_TOKEN_PLAYLIST_NAME:
            return playlist_name(NULL, buf, buf_size);

        case WPS_TOKEN_PLAYLIST_POSITION:
            snprintf(buf, buf_size, "%d", playlist_get_display_index());
            return buf;

        case WPS_TOKEN_PLAYLIST_SHUFFLE:
            if ( global_settings.playlist_shuffle )
                return "s";
            else
                return NULL;
            break;

        case WPS_TOKEN_VOLUME:
            snprintf(buf, buf_size, "%d", global_settings.volume);
            if (intval)
            {
                int minvol = sound_min(SOUND_VOLUME);
                if (global_settings.volume == minvol)
                {
                    *intval = 1;
                }
                else if (global_settings.volume == 0)
                {
                    *intval = limit - 1;
                }
                else if (global_settings.volume > 0)
                {
                    *intval = limit;
                }
                else
                {
                    *intval = (limit-3) * (global_settings.volume - minvol - 1)
                                / (-1 - minvol) + 2;
                }
            }
            return buf;
#ifdef HAVE_ALBUMART
        case WPS_TOKEN_ALBUMART_FOUND:
            if (data->albumart) {
                if (playback_current_aa_hid(data->playback_aa_slot) >= 0)
                    return "C";
            }
            return NULL;
            
        case WPS_TOKEN_ALBUMART_DISPLAY:
            if (!data->albumart)
                return NULL;
            if (!data->albumart->draw)
                data->albumart->draw = true;
            return NULL;
#endif

        case WPS_TOKEN_BATTERY_PERCENT:
        {
            int l = battery_level();

            if (intval)
            {
                limit = MAX(limit, 2);
                if (l > -1) {
                    /* First enum is used for "unknown level". */
                    *intval = (limit - 1) * l / 100 + 2;
                } else {
                    *intval = 1;
                }
            }

            if (l > -1) {
                snprintf(buf, buf_size, "%d", l);
                return buf;
            } else {
                return "?";
            }
        }

        case WPS_TOKEN_BATTERY_VOLTS:
        {
            unsigned int v = battery_voltage();
            snprintf(buf, buf_size, "%d.%02d", v / 1000, (v % 1000) / 10);
            return buf;
        }

        case WPS_TOKEN_BATTERY_TIME:
        {
            int t = battery_time();
            if (t >= 0)
                snprintf(buf, buf_size, "%dh %dm", t / 60, t % 60);
            else
                return "?h ?m";
            return buf;
        }

#if CONFIG_CHARGING
        case WPS_TOKEN_BATTERY_CHARGER_CONNECTED:
        {
            if(charger_input_state==CHARGER)
                return "p";
            else
                return NULL;
        }
#endif
#if CONFIG_CHARGING >= CHARGING_MONITOR
        case WPS_TOKEN_BATTERY_CHARGING:
        {
            if (charge_state == CHARGING || charge_state == TOPOFF) {
                return "c";
            } else {
                return NULL;
            }
        }
#endif
#ifdef HAVE_USB_POWER
        case WPS_TOKEN_USB_POWERED:
            if (usb_powered())
                return "u";
            return NULL;
#endif
        case WPS_TOKEN_BATTERY_SLEEPTIME:
        {
            if (get_sleep_timer() == 0)
                return NULL;
            else
            {
                format_time(buf, buf_size, get_sleep_timer() * 1000);
                return buf;
            }
        }

        case WPS_TOKEN_PLAYBACK_STATUS:
        {
            int status = current_playmode();
            /* music */
            int mode = 1; /* stop */
            if (status == STATUS_PLAY)
                mode = 2; /* play */
            if (state->is_fading || 
               (status == STATUS_PAUSE  && !status_get_ffmode()))
                mode = 3; /* pause */
            else
            {   /* ff / rwd */
                if (status_get_ffmode() == STATUS_FASTFORWARD)
                    mode = 4;
                if (status_get_ffmode() == STATUS_FASTBACKWARD)
                    mode = 5;
            }
#ifdef HAVE_RECORDING
            /* recording */
            if (status == STATUS_RECORD)
                mode = 6;
            else if (status == STATUS_RECORD_PAUSE)
                mode = 7;
#endif
#if CONFIG_TUNER
            /* radio */
            if (status == STATUS_RADIO)
                mode = 8;
            else if (status == STATUS_RADIO_PAUSE)
                mode = 9;
#endif

            if (intval) {
                *intval = mode;
            }

            snprintf(buf, buf_size, "%d", mode-1);
            return buf;
        }

        case WPS_TOKEN_REPEAT_MODE:
            if (intval)
                *intval = global_settings.repeat_mode + 1;
            snprintf(buf, buf_size, "%d", global_settings.repeat_mode);
            return buf;

        case WPS_TOKEN_RTC_PRESENT:
#if CONFIG_RTC
                return "c";
#else
                return NULL;
#endif

#if CONFIG_RTC
        case WPS_TOKEN_RTC_12HOUR_CFG:
            if (intval)
                *intval = global_settings.timeformat + 1;
            snprintf(buf, buf_size, "%d", global_settings.timeformat);
            return buf;

        case WPS_TOKEN_RTC_DAY_OF_MONTH:
            /* d: day of month (01..31) */
            snprintf(buf, buf_size, "%02d", tm->tm_mday);
            if (intval)
                *intval = tm->tm_mday - 1;
            return buf;

        case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
            /* e: day of month, blank padded ( 1..31) */
            snprintf(buf, buf_size, "%2d", tm->tm_mday);
            if (intval)
                *intval = tm->tm_mday - 1;
            return buf;

        case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
            /* H: hour (00..23) */
            snprintf(buf, buf_size, "%02d", tm->tm_hour);
            if (intval)
                *intval = tm->tm_hour;
            return buf;

        case WPS_TOKEN_RTC_HOUR_24:
            /* k: hour ( 0..23) */
            snprintf(buf, buf_size, "%2d", tm->tm_hour);
            if (intval)
                *intval = tm->tm_hour;
            return buf;

        case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
            /* I: hour (01..12) */
            snprintf(buf, buf_size, "%02d",
                     (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12);
            if (intval)
                *intval = (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12;
            return buf;

        case WPS_TOKEN_RTC_HOUR_12:
            /* l: hour ( 1..12) */
            snprintf(buf, buf_size, "%2d",
                     (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12);
            if (intval)
                *intval = (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12;
            return buf;

        case WPS_TOKEN_RTC_MONTH:
            /* m: month (01..12) */
            if (intval)
                *intval = tm->tm_mon + 1;
            snprintf(buf, buf_size, "%02d", tm->tm_mon + 1);
            return buf;

        case WPS_TOKEN_RTC_MINUTE:
            /* M: minute (00..59) */
            snprintf(buf, buf_size, "%02d", tm->tm_min);
            if (intval)
                *intval = tm->tm_min;
            return buf;

        case WPS_TOKEN_RTC_SECOND:
            /* S: second (00..59) */
            snprintf(buf, buf_size, "%02d", tm->tm_sec);
            if (intval)
                *intval = tm->tm_sec;
            return buf;

        case WPS_TOKEN_RTC_YEAR_2_DIGITS:
            /* y: last two digits of year (00..99) */
            snprintf(buf, buf_size, "%02d", tm->tm_year % 100);
            if (intval)
                *intval = tm->tm_year % 100;
            return buf;

        case WPS_TOKEN_RTC_YEAR_4_DIGITS:
            /* Y: year (1970...) */
            snprintf(buf, buf_size, "%04d", tm->tm_year + 1900);
            if (intval)
                *intval = tm->tm_year + 1900;
            return buf;

        case WPS_TOKEN_RTC_AM_PM_UPPER:
            /* p: upper case AM or PM indicator */
            if (intval)
                *intval = tm->tm_hour/12 == 0 ? 0 : 1;
            return tm->tm_hour/12 == 0 ? "AM" : "PM";

        case WPS_TOKEN_RTC_AM_PM_LOWER:
            /* P: lower case am or pm indicator */
            if (intval)
                *intval = tm->tm_hour/12 == 0 ? 0 : 1;
            return tm->tm_hour/12 == 0 ? "am" : "pm";

        case WPS_TOKEN_RTC_WEEKDAY_NAME:
            /* a: abbreviated weekday name (Sun..Sat) */
            return str(LANG_WEEKDAY_SUNDAY + tm->tm_wday);

        case WPS_TOKEN_RTC_MONTH_NAME:
            /* b: abbreviated month name (Jan..Dec) */
            return str(LANG_MONTH_JANUARY + tm->tm_mon);

        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
            /* u: day of week (1..7); 1 is Monday */
            if (intval)
                *intval = (tm->tm_wday == 0) ? 7 : tm->tm_wday;
            snprintf(buf, buf_size, "%1d", tm->tm_wday + 1);
            return buf;

        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
            /* w: day of week (0..6); 0 is Sunday */
            if (intval)
                *intval = tm->tm_wday + 1;
            snprintf(buf, buf_size, "%1d", tm->tm_wday);
            return buf;
#else
        case WPS_TOKEN_RTC_DAY_OF_MONTH:
        case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
        case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
        case WPS_TOKEN_RTC_HOUR_24:
        case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
        case WPS_TOKEN_RTC_HOUR_12:
        case WPS_TOKEN_RTC_MONTH:
        case WPS_TOKEN_RTC_MINUTE:
        case WPS_TOKEN_RTC_SECOND:
        case WPS_TOKEN_RTC_AM_PM_UPPER:
        case WPS_TOKEN_RTC_AM_PM_LOWER:
        case WPS_TOKEN_RTC_YEAR_2_DIGITS:
            return "--";
        case WPS_TOKEN_RTC_YEAR_4_DIGITS:
            return "----";
        case WPS_TOKEN_RTC_WEEKDAY_NAME:
        case WPS_TOKEN_RTC_MONTH_NAME:
            return "---";
        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
            return "-";
#endif

#ifdef HAVE_LCD_CHARCELLS
        case WPS_TOKEN_PROGRESSBAR:
        {
            char *end = utf8encode(data->wps_progress_pat[0], buf);
            *end = '\0';
            return buf;
        }

        case WPS_TOKEN_PLAYER_PROGRESSBAR:
            if(is_new_player())
            {
                /* we need 11 characters (full line) for
                    progress-bar */
                strlcpy(buf, "           ", buf_size);
            }
            else
            {
                /* Tell the user if we have an OldPlayer */
                strlcpy(buf, " <Old LCD> ", buf_size);
            }
            return buf;
#endif



#if (CONFIG_CODEC == SWCODEC)
        case WPS_TOKEN_CROSSFADE:
#ifdef HAVE_CROSSFADE
            if (intval)
                *intval = global_settings.crossfade + 1;
            snprintf(buf, buf_size, "%d", global_settings.crossfade);
#else
            snprintf(buf, buf_size, "%d", 0);
#endif
            return buf;

        case WPS_TOKEN_REPLAYGAIN:
        {
            int val;

            if (global_settings.replaygain_type == REPLAYGAIN_OFF)
                val = 1; /* off */
            else
            {
                int type;
                if (LIKELY(id3))
                    type = get_replaygain_mode(id3->track_gain_string != NULL,
                                        id3->album_gain_string != NULL);
                else
                    type = -1;

                if (type < 0)
                    val = 6;    /* no tag */
                else
                    val = type + 2;

                if (global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
                    val += 2;
            }

            if (intval)
                *intval = val;

            switch (val)
            {
                case 1:
                case 6:
                    return "+0.00 dB";
                    break;
                /* due to above, coming here with !id3 shouldn't be possible */
                case 2:
                case 4:
                    strlcpy(buf, id3->track_gain_string, buf_size);
                    break;
                case 3:
                case 5:
                    strlcpy(buf, id3->album_gain_string, buf_size);
                    break;
            }
            return buf;
        }
#endif  /* (CONFIG_CODEC == SWCODEC) */

#if (CONFIG_CODEC != MAS3507D)
        case WPS_TOKEN_SOUND_PITCH:
        {
            int32_t pitch = sound_get_pitch();
            snprintf(buf, buf_size, "%ld.%ld",
                     pitch / PITCH_SPEED_PRECISION,
             (pitch  % PITCH_SPEED_PRECISION) / (PITCH_SPEED_PRECISION / 10));

        if (intval)
            *intval = pitch_speed_enum(limit, pitch,
                           PITCH_SPEED_PRECISION * 100);
            return buf;
        }
#endif

#if CONFIG_CODEC == SWCODEC
    case WPS_TOKEN_SOUND_SPEED:
    {
        int32_t pitch = sound_get_pitch();
        int32_t speed;
        if (dsp_timestretch_available())
            speed = GET_SPEED(pitch, dsp_get_timestretch());
        else
            speed = pitch;
        snprintf(buf, buf_size, "%ld.%ld",
                     speed / PITCH_SPEED_PRECISION,
             (speed  % PITCH_SPEED_PRECISION) / (PITCH_SPEED_PRECISION / 10));
        if (intval)
            *intval = pitch_speed_enum(limit, speed,
                           PITCH_SPEED_PRECISION * 100);
            return buf;
    }
#endif

        case WPS_TOKEN_MAIN_HOLD:
#ifdef HAS_BUTTON_HOLD
            if (button_hold())
#else
            if (is_keys_locked())
#endif /*hold switch or softlock*/
                return "h";
            else
                return NULL;

#ifdef HAS_REMOTE_BUTTON_HOLD
        case WPS_TOKEN_REMOTE_HOLD:
            if (remote_button_hold())
                return "r";
            else
                return NULL;
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
        case WPS_TOKEN_VLED_HDD:
            if(led_read(HZ/2))
                return "h";
            else
                return NULL;
#endif
        case WPS_TOKEN_BUTTON_VOLUME:
            if (global_status.last_volume_change &&
                TIME_BEFORE(current_tick, global_status.last_volume_change +
                                          token->value.i * TIMEOUT_UNIT))
                return "v";
            return NULL;
        case WPS_TOKEN_LASTTOUCH:
#ifdef HAVE_TOUCHSCREEN
            if (TIME_BEFORE(current_tick, token->value.i * TIMEOUT_UNIT +
                                          touchscreen_last_touch()))
                return "t";
#endif
            return NULL;

        case WPS_TOKEN_SETTING:
        {
            const struct settings_list *s = settings+token->value.i;
            if (intval)
            {
                /* Handle contionals */
                switch (s->flags&F_T_MASK)
                {
                    case F_T_INT:
                    case F_T_UINT:
                        if (s->flags&F_T_SOUND)
                        {
                            /* %?St|name|<min|min+1|...|max-1|max> */
                            int sound_setting = s->sound_setting->setting;
                            /* settings with decimals can't be used in conditionals */
                            if (sound_numdecimals(sound_setting) == 0)
                            {
                                *intval = (*(int*)s->setting-sound_min(sound_setting))
                                      /sound_steps(sound_setting) + 1;
                            }
                            else
                                *intval = -1;
                        }
                        else if (s->flags&F_RGB)
                            /* %?St|name|<#000000|#000001|...|#FFFFFF> */
                            /* shouldn't overflow since colors are stored
                             * on 16 bits ...
                             * but this is pretty useless anyway */
                            *intval = *(int*)s->setting + 1;
                        else if (s->cfg_vals == NULL)
                            /* %?St|name|<1st choice|2nd choice|...> */
                            *intval = (*(int*)s->setting-s->int_setting->min)
                                      /s->int_setting->step + 1;
                        else
                            /* %?St|name|<1st choice|2nd choice|...> */
                            /* Not sure about this one. cfg_name/vals are
                             * indexed from 0 right? */
                            *intval = *(int*)s->setting + 1;
                        break;
                    case F_T_BOOL:
                        /* %?St|name|<if true|if false> */
                        *intval = *(bool*)s->setting?1:2;
                        break;
                    case F_T_CHARPTR:
                    case F_T_UCHARPTR:
                        /* %?St|name|<if non empty string|if empty>
                         * The string's emptyness discards the setting's
                         * prefix and suffix */
                        *intval = ((char*)s->setting)[0]?1:2;
                        break;
                    default:
                        /* This shouldn't happen ... but you never know */
                        *intval = -1;
                        break;
                }
            }
            /* Special handlng for filenames because we dont want to show the prefix */
            if ((s->flags&F_T_MASK) == F_T_UCHARPTR ||
                (s->flags&F_T_MASK) == F_T_UCHARPTR)
            {
                if (s->filename_setting->prefix)
                    return (char*)s->setting;
            }
            cfg_to_string(token->value.i,buf,buf_size);
            return buf;
        }
        /* Recording tokens */
        case WPS_TOKEN_HAVE_RECORDING:
#ifdef HAVE_RECORDING
            return "r";
#else
            return NULL;
#endif

#ifdef HAVE_RECORDING
        case WPS_TOKEN_REC_FREQ: /* order from REC_FREQ_CFG_VAL_LIST */
        {
#if CONFIG_CODEC == SWCODEC
            unsigned long samprk;
            int rec_freq = global_settings.rec_frequency;

#ifdef SIMULATOR
            samprk = 44100;
#else
#if defined(HAVE_SPDIF_REC)
            if (global_settings.rec_source == AUDIO_SRC_SPDIF)
            {
                /* Use rate in use, not current measured rate if it changed */
                samprk = pcm_rec_sample_rate();
                rec_freq = 0;
                while (rec_freq < SAMPR_NUM_FREQ &&
                       audio_master_sampr_list[rec_freq] != samprk)
                {
                    rec_freq++;
                }
            }
            else
#endif
                samprk = rec_freq_sampr[rec_freq];
#endif /* SIMULATOR */
            if (intval)
            {
                switch (rec_freq)
                {
                    REC_HAVE_96_(case REC_FREQ_96:
                        *intval = 1;
                        break;)
                    REC_HAVE_88_(case REC_FREQ_88:
                        *intval = 2;
                        break;)
                    REC_HAVE_64_(case REC_FREQ_64:
                        *intval = 3;
                        break;)
                    REC_HAVE_48_(case REC_FREQ_48:
                        *intval = 4;
                        break;)
                    REC_HAVE_44_(case REC_FREQ_44:
                        *intval = 5;
                        break;)
                    REC_HAVE_32_(case REC_FREQ_32:
                        *intval = 6;
                        break;)
                    REC_HAVE_24_(case REC_FREQ_24:
                        *intval = 7;
                        break;)
                    REC_HAVE_22_(case REC_FREQ_22:
                        *intval = 8;
                        break;)
                    REC_HAVE_16_(case REC_FREQ_16:
                        *intval = 9;
                        break;)
                    REC_HAVE_12_(case REC_FREQ_12:
                        *intval = 10;
                        break;)
                    REC_HAVE_11_(case REC_FREQ_11:
                        *intval = 11;
                        break;)
                    REC_HAVE_8_(case REC_FREQ_8:
                        *intval = 12;
                        break;)
                }
            }
            snprintf(buf, buf_size, "%d.%1d", samprk/1000,samprk%1000);
#else /* HWCODEC */
            
            static const char * const freq_strings[] =
                {"--", "44", "48", "32", "22", "24", "16"};
            int freq = 1 + global_settings.rec_frequency;
#ifdef HAVE_SPDIF_REC
            if (global_settings.rec_source == AUDIO_SRC_SPDIF)
            {
                /* Can't measure S/PDIF sample rate on Archos/Sim yet */
                freq = 0;
            }
#endif /* HAVE_SPDIF_IN */
            if (intval)
                *intval = freq+1; /* so the token gets a value 1<=x<=7 */
            snprintf(buf, buf_size, "%d\n",
                        freq_strings[global_settings.rec_frequency]);
#endif
            return buf;
        }
#if CONFIG_CODEC == SWCODEC
        case WPS_TOKEN_REC_ENCODER:
        {
            int rec_format = global_settings.rec_format+1; /* WAV, AIFF, WV, MPEG */
            if (intval)
                *intval = rec_format;
            switch (rec_format)
            {
                case REC_FORMAT_PCM_WAV:
                    return "wav";
                case REC_FORMAT_AIFF:
                    return "aiff";
                case REC_FORMAT_WAVPACK:
                    return "wv";
                case REC_FORMAT_MPA_L3:
                    return "MP3";
                default:
                    return NULL;
            }
            break;
        }
#endif
        case WPS_TOKEN_REC_BITRATE:
#if CONFIG_CODEC == SWCODEC
            if (global_settings.rec_format == REC_FORMAT_MPA_L3)
            {
                if (intval)
                {
                    #if 0 /* FIXME: I dont know if this is needed? */
                    switch (1<<global_settings.mp3_enc_config.bitrate)
                    {
                        case MP3_BITR_CAP_8:
                            *intval = 1;
                            break;
                        case MP3_BITR_CAP_16:
                            *intval = 2;
                            break;
                        case MP3_BITR_CAP_24:
                            *intval = 3;
                            break;
                        case MP3_BITR_CAP_32:
                            *intval = 4;
                            break;
                        case MP3_BITR_CAP_40:
                            *intval = 5;
                            break;
                        case MP3_BITR_CAP_48:
                            *intval = 6;
                            break;
                        case MP3_BITR_CAP_56:
                            *intval = 7;
                            break;
                        case MP3_BITR_CAP_64:
                            *intval = 8;
                            break;
                        case MP3_BITR_CAP_80:
                            *intval = 9;
                            break;
                        case MP3_BITR_CAP_96:
                            *intval = 10;
                            break;
                        case MP3_BITR_CAP_112:
                            *intval = 11;
                            break;
                        case MP3_BITR_CAP_128:
                            *intval = 12;
                            break;
                        case MP3_BITR_CAP_144:
                            *intval = 13;
                            break;
                        case MP3_BITR_CAP_160:
                            *intval = 14;
                            break;
                        case MP3_BITR_CAP_192:
                            *intval = 15;
                            break;
                    }
                    #endif
                    *intval = global_settings.mp3_enc_config.bitrate+1;
                }
                snprintf(buf, buf_size, "%d", global_settings.mp3_enc_config.bitrate+1);
                return buf;
            }
            else
                return NULL; /* Fixme later */
#else /* CONFIG_CODEC == HWCODEC */
            if (intval)
                *intval = global_settings.rec_quality+1;
            snprintf(buf, buf_size, "%d", global_settings.rec_quality);
            return buf;
#endif
        case WPS_TOKEN_REC_MONO:
            if (!global_settings.rec_channels)
                return "m";
            return NULL;

#endif /* HAVE_RECORDING */
        case WPS_TOKEN_CURRENT_SCREEN:
        {
            int curr_screen = current_screen();

#ifdef HAVE_RECORDING
            /* override current_screen() for recording screen since it may
             * be entered from the radio screen */
            if (in_recording_screen())
                curr_screen = GO_TO_RECSCREEN;
#endif
            
            switch (curr_screen)
            {
                case GO_TO_WPS:
                    curr_screen = 2;
                    break;
#ifdef HAVE_RECORDING
                case GO_TO_RECSCREEN:
                    curr_screen = 3;
                    break;
#endif
#if CONFIG_TUNER
                case GO_TO_FM:
                    curr_screen = 4;
                    break;
#endif
                case GO_TO_PLAYLIST_VIEWER:
                    curr_screen = 5;
                    break;
                default: /* lists */
                    curr_screen = 1;
                    break;
            }
            if (intval)
            {
                *intval = curr_screen;
            }
            snprintf(buf, buf_size, "%d", curr_screen);
            return buf;
        }

        case WPS_TOKEN_LANG_IS_RTL:
            return lang_is_rtl() ? "r" : NULL;

        default:
            return NULL;
    }
}
