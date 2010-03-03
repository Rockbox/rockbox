/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin, Dan Everton, Matthias Mohr
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

#if defined(DEBUG) || defined(SIMULATOR)

#include <stdio.h>
#include <string.h>
#include "wps.h"
#include "wps_internals.h"
#include "skin_buffer.h"
#include "settings_list.h"
#ifdef __PCTOOL__
#ifdef WPSEDITOR
#include "proxy.h"
#else
#define DEBUGF printf
#endif
#else
#include "debug.h"
#endif

#if defined(SIMULATOR) || defined(__PCTOOL__)
extern bool debug_wps;
extern int wps_verbose_level;
#endif

struct debug_token_table
{
    enum wps_token_type start_marker;
    char *desc;
};
#define X(name) name, #name
struct debug_token_table tokens[] = {
    { X(TOKEN_MARKER_CONTROL_TOKENS) },
    { X(TOKEN_MARKER_BATTERY) },
    { X(TOKEN_MARKER_SOUND) },
    { X(TOKEN_MARKER_RTC) },
    { X(TOKEN_MARKER_DATABASE) },
    { X(TOKEN_MARKER_FILE) },
    { X(TOKEN_MARKER_IMAGES) },
    { X(TOKEN_MARKER_METADATA) },   
    { X(TOKEN_MARKER_PLAYBACK_INFO) },
    { X(TOKEN_MARKER_PLAYLIST) },
    { X(TOKEN_MARKER_MISC) },
    { X(TOKEN_MARKER_RECORDING) },
    { X(TOKEN_MARKER_END) },
};
#undef X

static char *next_str(bool next) {
    return next ? "next " : "";
}

static char *get_token_desc(struct wps_token *token, char *buf,
                            int bufsize, struct wps_data *data)
{
    unsigned i;
#ifndef HAVE_LCD_BITMAP
    (void)data; /* kill charcell warning */
#endif
    bool next = token->next;
    switch(token->type)
    {
        case WPS_NO_TOKEN:
            snprintf(buf, bufsize, "No token");
            break;

        case WPS_TOKEN_UNKNOWN:
            snprintf(buf, bufsize, "Unknown token");
            break;

        case WPS_TOKEN_CHARACTER:
            if (token->value.c == '\n')
                snprintf(buf, bufsize, "Character '\\n'");
            else
                snprintf(buf, bufsize, "Character '%c'",
                        token->value.c);
            break;

        case WPS_TOKEN_STRING:
            snprintf(buf, bufsize, "String '%s'",
                     (char*)token->value.data);
            break;
        case WPS_TOKEN_TRANSLATEDSTRING:
            snprintf(buf, bufsize, "String ID '%d'", token->value.i);
            break;

#ifdef HAVE_LCD_BITMAP
        case WPS_TOKEN_ALIGN_LEFT:
            snprintf(buf, bufsize, "align left");
            break;

        case WPS_TOKEN_ALIGN_CENTER:
            snprintf(buf, bufsize, "align center");
            break;

        case WPS_TOKEN_ALIGN_RIGHT:
            snprintf(buf, bufsize, "align right");
            break;
#endif

        case WPS_TOKEN_SUBLINE_TIMEOUT:
            snprintf(buf, bufsize, "subline timeout value: %d",
                    token->value.i);
            break;

        case WPS_TOKEN_CONDITIONAL:
            snprintf(buf, bufsize, "conditional, %d options",
                    token->value.i);
            break;

        case WPS_TOKEN_CONDITIONAL_START:
            snprintf(buf, bufsize, "conditional start, next cond: %d",
                    token->value.i);
            break;

        case WPS_TOKEN_CONDITIONAL_OPTION:
            snprintf(buf, bufsize, "conditional option, next cond: %d",
                    token->value.i);
            break;

        case WPS_TOKEN_CONDITIONAL_END:
            snprintf(buf, bufsize, "conditional end");
            break;

#ifdef HAVE_LCD_BITMAP
        case WPS_TOKEN_LIST_TITLE_TEXT:
            snprintf(buf, bufsize, "list title text");
            break;
        case WPS_TOKEN_LIST_TITLE_ICON:
            snprintf(buf, bufsize, "list title icon");
            break;
        case WPS_TOKEN_IMAGE_PRELOAD:
            snprintf(buf, bufsize, "preload image");
            break;

        case WPS_TOKEN_IMAGE_PRELOAD_DISPLAY:
        {
            char subimage = '\0';
            char label = token->value.i&0xFF;
            struct gui_img *img = find_image(label, data);
            if (img && img->num_subimages > 1)
            {
                int item = token->value.i>>8;
                if (item >= 26)
                    subimage = 'A' + item-26;
                else
                    subimage = 'a' + item;
            }
            snprintf(buf, bufsize, "display preloaded image '%c%c'",
                    label, subimage);
        }
            break;

        case WPS_TOKEN_IMAGE_DISPLAY:
            snprintf(buf, bufsize, "display image");
            break;
#endif

#ifdef HAS_BUTTON_HOLD
        case WPS_TOKEN_MAIN_HOLD:
            snprintf(buf, bufsize, "mode hold");
            break;
#endif

#ifdef HAS_REMOTE_BUTTON_HOLD
        case WPS_TOKEN_REMOTE_HOLD:
            snprintf(buf, bufsize, "mode remote hold");
            break;
#endif

        case WPS_TOKEN_REPEAT_MODE:
            snprintf(buf, bufsize, "mode repeat");
            break;

        case WPS_TOKEN_PLAYBACK_STATUS:
            snprintf(buf, bufsize, "mode playback");
            break;
            
        case WPS_TOKEN_RTC_PRESENT:
            snprintf(buf, bufsize, "rtc: present?");
            break;
        case WPS_TOKEN_RTC_DAY_OF_MONTH:
            snprintf(buf, bufsize, "rtc: day of month (01..31)");
            break;
        case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
            snprintf(buf, bufsize,
                    "rtc: day of month, blank padded ( 1..31)");
            break;
        case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
            snprintf(buf, bufsize, "rtc: hour (00..23)");
            break;
        case WPS_TOKEN_RTC_HOUR_24:
            snprintf(buf, bufsize, "rtc: hour ( 0..23)");
            break;
        case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
            snprintf(buf, bufsize, "rtc: hour (01..12)");
            break;
        case WPS_TOKEN_RTC_HOUR_12:
            snprintf(buf, bufsize, "rtc: hour ( 1..12)");
            break;
        case WPS_TOKEN_RTC_MONTH:
            snprintf(buf, bufsize, "rtc: month (01..12)");
            break;
        case WPS_TOKEN_RTC_MINUTE:
            snprintf(buf, bufsize, "rtc: minute (00..59)");
            break;
        case WPS_TOKEN_RTC_SECOND:
            snprintf(buf, bufsize, "rtc: second (00..59)");
            break;
        case WPS_TOKEN_RTC_YEAR_2_DIGITS:
            snprintf(buf, bufsize,
                    "rtc: last two digits of year (00..99)");
            break;
        case WPS_TOKEN_RTC_YEAR_4_DIGITS:
            snprintf(buf, bufsize, "rtc: year (1970...)");
            break;
        case WPS_TOKEN_RTC_AM_PM_UPPER:
            snprintf(buf, bufsize,
                    "rtc: upper case AM or PM indicator");
            break;
        case WPS_TOKEN_RTC_AM_PM_LOWER:
            snprintf(buf, bufsize,
                    "rtc: lower case am or pm indicator");
            break;
        case WPS_TOKEN_RTC_WEEKDAY_NAME:
            snprintf(buf, bufsize,
                    "rtc: abbreviated weekday name (Sun..Sat)");
            break;
        case WPS_TOKEN_RTC_MONTH_NAME:
            snprintf(buf, bufsize,
                    "rtc: abbreviated month name (Jan..Dec)");
            break;
        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
            snprintf(buf, bufsize,
                    "rtc: day of week (1..7); 1 is Monday");
            break;
        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
            snprintf(buf, bufsize,
                    "rtc: day of week (0..6); 0 is Sunday");
            break;

#if (CONFIG_CODEC == SWCODEC)
        case WPS_TOKEN_CROSSFADE:
            snprintf(buf, bufsize, "crossfade");
            break;

        case WPS_TOKEN_REPLAYGAIN:
            snprintf(buf, bufsize, "replaygain");
            break;
#endif

#ifdef HAVE_ALBUMART
        case WPS_TOKEN_ALBUMART_DISPLAY:
            snprintf(buf, bufsize, "album art display");
            break;

        case WPS_TOKEN_ALBUMART_FOUND:
            snprintf(buf, bufsize, "%strack album art conditional",
                     next_str(next));
            break;
#endif

#ifdef HAVE_LCD_BITMAP
        case WPS_TOKEN_IMAGE_BACKDROP:
            snprintf(buf, bufsize, "backdrop image");
            break;

        case WPS_TOKEN_IMAGE_PROGRESS_BAR:
            snprintf(buf, bufsize, "progressbar bitmap");
            break;

        case WPS_TOKEN_PEAKMETER:
            snprintf(buf, bufsize, "peakmeter");
            break;
#endif

        case WPS_TOKEN_PROGRESSBAR:
            snprintf(buf, bufsize, "progressbar");
            break;

#ifdef HAVE_LCD_CHARCELLS
        case WPS_TOKEN_PLAYER_PROGRESSBAR:
            snprintf(buf, bufsize, "full line progressbar");
            break;
#endif

        case WPS_TOKEN_TRACK_TIME_ELAPSED:
            snprintf(buf, bufsize, "time elapsed in track");
            break;

        case WPS_TOKEN_TRACK_ELAPSED_PERCENT:
            snprintf(buf, bufsize, "played percentage of track");
            break;

        case WPS_TOKEN_PLAYLIST_ENTRIES:
            snprintf(buf, bufsize, "number of entries in playlist");
            break;

        case WPS_TOKEN_PLAYLIST_NAME:
            snprintf(buf, bufsize, "playlist name");
            break;

        case WPS_TOKEN_PLAYLIST_POSITION:
            snprintf(buf, bufsize, "position in playlist");
            break;

        case WPS_TOKEN_TRACK_TIME_REMAINING:
            snprintf(buf, bufsize, "time remaining in track");
            break;

        case WPS_TOKEN_PLAYLIST_SHUFFLE:
            snprintf(buf, bufsize, "playlist shuffle mode");
            break;

        case WPS_TOKEN_TRACK_LENGTH:
            snprintf(buf, bufsize, "track length");
            break;

        case WPS_TOKEN_VOLUME:
            snprintf(buf, bufsize, "volume");
            break;

        case WPS_TOKEN_METADATA_ARTIST:
            snprintf(buf, bufsize, "%strack artist",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_COMPOSER:
            snprintf(buf, bufsize, "%strack composer",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_ALBUM:
            snprintf(buf, bufsize, "%strack album",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_GROUPING:
            snprintf(buf, bufsize, "%strack grouping",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_GENRE:
            snprintf(buf, bufsize, "%strack genre",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_DISC_NUMBER:
            snprintf(buf, bufsize, "%strack disc", next_str(next));
            break;

        case WPS_TOKEN_METADATA_TRACK_NUMBER:
            snprintf(buf, bufsize, "%strack number",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_TRACK_TITLE:
            snprintf(buf, bufsize, "%strack title",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_VERSION:
            snprintf(buf, bufsize, "%strack ID3 version",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_ALBUM_ARTIST:
            snprintf(buf, bufsize, "%strack album artist",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_COMMENT:
            snprintf(buf, bufsize, "%strack comment",
                    next_str(next));
            break;

        case WPS_TOKEN_METADATA_YEAR:
            snprintf(buf, bufsize, "%strack year", next_str(next));
            break;

#ifdef HAVE_TAGCACHE
        case WPS_TOKEN_DATABASE_PLAYCOUNT:
            snprintf(buf, bufsize, "track playcount (database)");
            break;

        case WPS_TOKEN_DATABASE_RATING:
            snprintf(buf, bufsize, "track rating (database)");
            break;

        case WPS_TOKEN_DATABASE_AUTOSCORE:
            snprintf(buf, bufsize, "track autoscore (database)");
            break;
#endif

        case WPS_TOKEN_BATTERY_PERCENT:
            snprintf(buf, bufsize, "battery percentage");
            break;

        case WPS_TOKEN_BATTERY_VOLTS:
            snprintf(buf, bufsize, "battery voltage");
            break;

        case WPS_TOKEN_BATTERY_TIME:
            snprintf(buf, bufsize, "battery time left");
            break;

        case WPS_TOKEN_BATTERY_CHARGER_CONNECTED:
            snprintf(buf, bufsize, "battery charger connected");
            break;

        case WPS_TOKEN_BATTERY_CHARGING:
            snprintf(buf, bufsize, "battery charging");
            break;

        case WPS_TOKEN_BATTERY_SLEEPTIME:
            snprintf(buf, bufsize, "sleep timer");
            break;

        case WPS_TOKEN_FILE_BITRATE:
            snprintf(buf, bufsize, "%sfile bitrate", next_str(next));
            break;

        case WPS_TOKEN_FILE_CODEC:
            snprintf(buf, bufsize, "%sfile codec", next_str(next));
            break;

        case WPS_TOKEN_FILE_FREQUENCY:
            snprintf(buf, bufsize, "%sfile audio frequency in Hz",
                    next_str(next));
            break;

        case WPS_TOKEN_FILE_FREQUENCY_KHZ:
            snprintf(buf, bufsize, "%sfile audio frequency in KHz",
                    next_str(next));
            break;

        case WPS_TOKEN_FILE_NAME:
            snprintf(buf, bufsize, "%sfile name", next_str(next));
            break;

        case WPS_TOKEN_FILE_NAME_WITH_EXTENSION:
            snprintf(buf, bufsize, "%sfile name with extension",
                    next_str(next));
            break;

        case WPS_TOKEN_FILE_PATH:
            snprintf(buf, bufsize, "%sfile path", next_str(next));
            break;

        case WPS_TOKEN_FILE_SIZE:
            snprintf(buf, bufsize, "%sfile size", next_str(next));
            break;

        case WPS_TOKEN_FILE_VBR:
            snprintf(buf, bufsize, "%sfile is vbr", next_str(next));
            break;

        case WPS_TOKEN_FILE_DIRECTORY:
            snprintf(buf, bufsize, "%sfile directory, level: %d",
                    next_str(next), token->value.i);
            break;

#if (CONFIG_CODEC != MAS3507D)
        case WPS_TOKEN_SOUND_PITCH:
            snprintf(buf, bufsize, "pitch value");
            break;
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
        case WPS_TOKEN_VLED_HDD:
            snprintf(buf, bufsize, "display virtual HDD LED");
            break;
#endif
        case WPS_VIEWPORT_ENABLE:
            snprintf(buf, bufsize, "enable VP: %c",
                     (char)token->value.i);
            break;
        case WPS_TOKEN_BUTTON_VOLUME:
            snprintf(buf, bufsize, "Volume button timeout: %d",
                     token->value.i);
            break;
        case WPS_TOKEN_SETTING:
            snprintf(buf, bufsize, "Setting value: '%s'",
                 settings[token->value.i].cfg_name);
            break;
        case WPS_TOKEN_LANG_IS_RTL:
            snprintf(buf, bufsize, "lang: is_rtl?");
            break;
        default:
            for(i=1; i<sizeof(tokens)/sizeof(*token); i++)
            {
                if (token->type < tokens[i].start_marker)
                {
                    snprintf(buf, bufsize, "FIXME: %s + %d\n", tokens[i-1].desc,
                             token->type - tokens[i-1].start_marker);
                    break;
                }
            } 
            break;
    }

    return buf;
}

#if defined(SIMULATOR) || defined(__PCTOOL__)
static void dump_skin(struct wps_data *data)
{
    int indent = 0;
    char buf[64];
    int i, j;

    struct skin_token_list *viewport_list;
    for (viewport_list = data->viewports;
         viewport_list; viewport_list = viewport_list->next)
    {
        struct skin_viewport *skin_viewport =
                        (struct skin_viewport *)viewport_list->token->value.data;
        indent = 0;
        DEBUGF("Viewport: '%c'\n", skin_viewport->label);
        struct skin_line *line;
        for (line = skin_viewport->lines; line; line = line->next)
        {
            struct skin_subline *subline;
            indent = 1;
            for(subline = &line->sublines; subline; subline = subline->next)
            {
                DEBUGF("  Subline: tokens %d => %d",
                       subline->first_token_idx,subline->last_token_idx);
                if (subline->line_type & WPS_REFRESH_SCROLL)
                    DEBUGF(", scrolled");
                else if (subline->line_type & WPS_REFRESH_PLAYER_PROGRESS)
                    DEBUGF(", progressbar");
                else if (subline->line_type & WPS_REFRESH_PEAK_METER)
                    DEBUGF(", peakmeter");
                DEBUGF("\n");

                for (i = subline->first_token_idx; i <= subline->last_token_idx; i++)
                {
                    struct wps_token *token = &data->tokens[i];
                    get_token_desc(token, buf, sizeof(buf), data);

                    switch(token->type)
                    {

                        case WPS_TOKEN_CONDITIONAL_START:
                            indent++;
                            break;

                        case WPS_TOKEN_CONDITIONAL_END:
                            indent--;
                            break;

                        default:
                            break;
                    }

                    if (wps_verbose_level > 2)
                    {
                        for(j = 0; j < indent; j++) {
                            DEBUGF("\t");
                        }

                        DEBUGF("[%3d] = (%2d) %s\n", i, token->type, buf);
                    }
                }
            }
        }
    }
}
#endif

void print_debug_info(struct wps_data *data, enum wps_parse_error fail, int line)
{
#if defined(SIMULATOR) || defined(__PCTOOL__)
    if (debug_wps && wps_verbose_level)
    {
        dump_skin(data);
    }
#endif /* SIMULATOR */

    if (fail != PARSE_OK)
    {
        char buf[64];

        DEBUGF("ERR: Failed parsing on line %d : ", line);
        switch (fail)
        {
            case PARSE_OK:
                break;

            case PARSE_FAIL_UNCLOSED_COND:
                DEBUGF("ERR: Unclosed conditional");
                break;

            case PARSE_FAIL_INVALID_CHAR:
                DEBUGF("ERR: Unexpected conditional char after token %d: \"%s\"",
                       data->num_tokens-1,
                       get_token_desc(&data->tokens[data->num_tokens-1], buf, sizeof(buf), data)
                      );
                break;

            case PARSE_FAIL_COND_SYNTAX_ERROR:
                DEBUGF("ERR: Conditional syntax error after token %d: \"%s\"",
                       data->num_tokens-1,
                       get_token_desc(&data->tokens[data->num_tokens-1], buf, sizeof(buf), data)
                      );
                break;

            case PARSE_FAIL_COND_INVALID_PARAM:
                DEBUGF("ERR: Invalid parameter list for token %d: \"%s\"",
                       data->num_tokens,
                       get_token_desc(&data->tokens[data->num_tokens], buf, sizeof(buf), data)
                      );
                break;

            case PARSE_FAIL_LIMITS_EXCEEDED:
                DEBUGF("ERR: Limits exceeded");
                break;
        }
        DEBUGF("\n");
    }
}

void debug_skin_usage(void)
{
#if defined(SIMULATOR) || defined(__PCTOOL__)
    if (wps_verbose_level > 1)
#endif
        DEBUGF("Skin buffer usage: %lu/%lu\n", (unsigned long)skin_buffer_usage(),
                                (unsigned long)(skin_buffer_usage() + skin_buffer_freespace()));
}


#endif /* DEBUG || SIMULATOR */
