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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifdef DEBUG

#include <stdio.h>
#include <string.h>
#include "gwps.h"
#ifdef __PCTOOL__
#define DEBUGF printf
#else
#include "debug.h"
#endif

#define PARSE_FAIL_UNCLOSED_COND        1
#define PARSE_FAIL_INVALID_CHAR         2
#define PARSE_FAIL_COND_SYNTAX_ERROR    3
#define PARSE_FAIL_COND_INVALID_PARAM   4

#if defined(SIMULATOR) || defined(__PCTOOL__)
extern bool debug_wps;
extern int wps_verbose_level;
#endif

static char *next_str(bool next) {
    return next ? "next " : "";
}

static char *get_token_desc(struct wps_token *token, struct wps_data *data,
                            char *buf, int bufsize)
{
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
            snprintf(buf, bufsize, "Character '%c'",
                    token->value.c);
            break;

        case WPS_TOKEN_STRING:
            snprintf(buf, bufsize, "String '%s'",
                     data->strings[token->value.i]);
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

        case WPS_TOKEN_LEFTMARGIN:
            snprintf(buf, bufsize, "left margin, value: %d",
                    token->value.i);
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
        case WPS_TOKEN_IMAGE_PRELOAD:
            snprintf(buf, bufsize, "preload image");
            break;

        case WPS_TOKEN_IMAGE_PRELOAD_DISPLAY:
            snprintf(buf, bufsize, "display preloaded image %d",
                    token->value.i);
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

        default:
            snprintf(buf, bufsize, "FIXME (code: %d)",
                    token->type);
            break;
    }

    return buf;
}

#if defined(SIMULATOR) || defined(__PCTOOL__)
static void dump_wps_tokens(struct wps_data *data)
{
    struct wps_token *token;
    int i, j;
    int indent = 0;
    char buf[64];
    int num_string_tokens = 0;

    /* Dump parsed WPS */
    for (i = 0, token = data->tokens; i < data->num_tokens; i++, token++)
    {
        get_token_desc(token, data, buf, sizeof(buf));

        switch(token->type)
        {
            case WPS_TOKEN_STRING:
                num_string_tokens++;
                break;

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

    if (wps_verbose_level > 0)
    {
        DEBUGF("\n");
        DEBUGF("Number of string tokens: %d\n", num_string_tokens);
        DEBUGF("\n");
    }
}

static void print_line_info(struct wps_data *data)
{
    int i, j, v;
    struct wps_line *line;
    struct wps_subline *subline;

    if (wps_verbose_level > 0)
    {
        DEBUGF("Number of viewports : %d\n", data->num_viewports);
        for (v = 0; v < data->num_viewports; v++)
        {
            DEBUGF("vp %d: Number of lines: %d\n", v, data->viewports[v].num_lines);
        }
        DEBUGF("Number of sublines  : %d\n", data->num_sublines);
        DEBUGF("Number of tokens    : %d\n", data->num_tokens);
        DEBUGF("\n");
    }

    if (wps_verbose_level > 1)
    {
        for (v = 0; v < data->num_viewports; v++)
        {
            DEBUGF("Viewport %d - +%d+%d (%dx%d)\n",v,data->viewports[v].vp.x, 
                                                      data->viewports[v].vp.y,
                                                      data->viewports[v].vp.width,
                                                      data->viewports[v].vp.height);
            for (i = 0, line = data->viewports[v].lines; i < data->viewports[v].num_lines; i++,line++)
            {
                DEBUGF("Line %2d (num_sublines=%d, first_subline=%d)\n",
                       i, line->num_sublines, line->first_subline_idx);

                for (j = 0, subline = data->sublines + line->first_subline_idx;
                     j < line->num_sublines; j++, subline++)
                {
                    DEBUGF("    Subline %d: first_token=%3d, last_token=%3d",
                           j, subline->first_token_idx,
                           wps_last_token_index(data, v, i, j));

                    if (subline->line_type & WPS_REFRESH_SCROLL)
                        DEBUGF(", scrolled");
                    else if (subline->line_type & WPS_REFRESH_PLAYER_PROGRESS)
                        DEBUGF(", progressbar");
                    else if (subline->line_type & WPS_REFRESH_PEAK_METER)
                        DEBUGF(", peakmeter");

                    DEBUGF("\n");
                }
            }
        }

        DEBUGF("\n");
    }
}

static void print_wps_strings(struct wps_data *data)
{
    int i, len, total_len = 0, buf_used = 0;

    if (wps_verbose_level > 1) DEBUGF("Strings:\n");
    for (i = 0; i < data->num_strings; i++)
    {
        len = strlen(data->strings[i]);
        total_len += len;
        buf_used += len + 1;
        if (wps_verbose_level > 1)
            DEBUGF("%2d: (%2d) '%s'\n", i, len, data->strings[i]);
    }
    if (wps_verbose_level > 1) DEBUGF("\n");

    if (wps_verbose_level > 0)
    {
        DEBUGF("Number of unique strings: %d (max: %d)\n",
               data->num_strings, WPS_MAX_STRINGS);
        DEBUGF("Total string length: %d\n", total_len);
        DEBUGF("String buffer used: %d out of %d bytes\n",
               buf_used, STRING_BUFFER_SIZE);
        DEBUGF("\n");
    }
}
#endif

void print_debug_info(struct wps_data *data, int fail, int line)
{
#if defined(SIMULATOR) || defined(__PCTOOL__)
    if (debug_wps && wps_verbose_level)
    {
        dump_wps_tokens(data);
        print_wps_strings(data);
        print_line_info(data);
    }
#endif /* SIMULATOR */

    if (data->num_tokens >= WPS_MAX_TOKENS - 1) {
        DEBUGF("Warning: Max number of tokens was reached (%d)\n",
               WPS_MAX_TOKENS - 1);
    }

    if (fail)
    {
        char buf[64];

        DEBUGF("Failed parsing on line %d : ", line);
        switch (fail)
        {
            case PARSE_FAIL_UNCLOSED_COND:
                DEBUGF("Unclosed conditional");
                break;

            case PARSE_FAIL_INVALID_CHAR:
                DEBUGF("unexpected conditional char after token %d: \"%s\"",
                       data->num_tokens-1,
                       get_token_desc(&data->tokens[data->num_tokens-1], data,
                                      buf, sizeof(buf))
                      );
                break;

            case PARSE_FAIL_COND_SYNTAX_ERROR:
                DEBUGF("Conditional syntax error after token %d: \"%s\"",
                       data->num_tokens-1,
                       get_token_desc(&data->tokens[data->num_tokens-1], data,
                                      buf, sizeof(buf))
                      );
                break;

            case PARSE_FAIL_COND_INVALID_PARAM:
                DEBUGF("Invalid parameter list for token %d: \"%s\"",
                       data->num_tokens,
                       get_token_desc(&data->tokens[data->num_tokens], data,
                                      buf, sizeof(buf))
                      );
                break;
        }
        DEBUGF("\n");
    }
}

#endif /* DEBUG */
