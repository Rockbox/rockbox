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

#define PARSE_FAIL_UNCLOSED_COND     1
#define PARSE_FAIL_INVALID_CHAR      2
#define PARSE_FAIL_COND_SYNTAX_ERROR 3

#if defined(SIMULATOR) || defined(__PCTOOL__)
extern bool debug_wps;
extern int wps_verbose_level;
#endif

static char *next_str(bool next) {
    return next ? "next " : "";
}

static void dump_wps_tokens(struct wps_data *data)
{
    struct wps_token *token;
    int i, j;
    int indent = 0;
    char buf[64];
    bool next;
    int num_string_tokens = 0;

    /* Dump parsed WPS */
    for (i = 0, token = data->tokens; i < data->num_tokens; i++, token++) {
        next = token->next;

        switch(token->type) {
            case WPS_TOKEN_UNKNOWN:
                snprintf(buf, sizeof(buf), "Unknown token");
                break;
            case WPS_TOKEN_CHARACTER:
                snprintf(buf, sizeof(buf), "Character '%c'",
                         token->value.c);
                break;

            case WPS_TOKEN_STRING:
                snprintf(buf, sizeof(buf), "String '%s'",
                         data->strings[token->value.i]);
                num_string_tokens++;
                break;

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_ALIGN_LEFT:
                snprintf(buf, sizeof(buf), "align left");
                break;

            case WPS_TOKEN_ALIGN_CENTER:
                snprintf(buf, sizeof(buf), "align center");
                break;

            case WPS_TOKEN_ALIGN_RIGHT:
                snprintf(buf, sizeof(buf), "align right");
                break;

            case WPS_TOKEN_LEFTMARGIN:
                snprintf(buf, sizeof(buf), "left margin, value: %d",
                         token->value.i);
                break;
#endif

            case WPS_TOKEN_SUBLINE_TIMEOUT:
                snprintf(buf, sizeof(buf), "subline timeout value: %d",
                         token->value.i);
                break;

            case WPS_TOKEN_CONDITIONAL:
                snprintf(buf, sizeof(buf), "conditional, %d options",
                        token->value.i);
                break;

            case WPS_TOKEN_CONDITIONAL_START:
                snprintf(buf, sizeof(buf), "conditional start, next cond: %d",
                         token->value.i);
                indent++;
                break;

            case WPS_TOKEN_CONDITIONAL_OPTION:
                snprintf(buf, sizeof(buf), "conditional option, next cond: %d",
                         token->value.i);
                break;

            case WPS_TOKEN_CONDITIONAL_END:
                snprintf(buf, sizeof(buf), "conditional end");
                indent--;
                break;

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_IMAGE_PRELOAD:
                snprintf(buf, sizeof(buf), "preload image");
                break;

            case WPS_TOKEN_IMAGE_PRELOAD_DISPLAY:
                snprintf(buf, sizeof(buf), "display preloaded image %d",
                         token->value.i);
                break;

            case WPS_TOKEN_IMAGE_DISPLAY:
                snprintf(buf, sizeof(buf), "display image");
                break;
#endif

#ifdef HAS_BUTTON_HOLD
            case WPS_TOKEN_MAIN_HOLD:
                snprintf(buf, sizeof(buf), "mode hold");
                break;
#endif

#ifdef HAS_REMOTE_BUTTON_HOLD
            case WPS_TOKEN_REMOTE_HOLD:
                snprintf(buf, sizeof(buf), "mode remote hold");
                break;
#endif

            case WPS_TOKEN_REPEAT_MODE:
                snprintf(buf, sizeof(buf), "mode repeat");
                break;

            case WPS_TOKEN_PLAYBACK_STATUS:
                snprintf(buf, sizeof(buf), "mode playback");
                break;

            case WPS_TOKEN_RTC_DAY_OF_MONTH:
                snprintf(buf, sizeof(buf), "rtc: day of month (01..31)");
                break;
            case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
                snprintf(buf, sizeof(buf),
                         "rtc: day of month, blank padded ( 1..31)");
                break;
            case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
                snprintf(buf, sizeof(buf), "rtc: hour (00..23)");
                break;
            case WPS_TOKEN_RTC_HOUR_24:
                snprintf(buf, sizeof(buf), "rtc: hour ( 0..23)");
                break;
            case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
                snprintf(buf, sizeof(buf), "rtc: hour (01..12)");
                break;
            case WPS_TOKEN_RTC_HOUR_12:
                snprintf(buf, sizeof(buf), "rtc: hour ( 1..12)");
                break;
            case WPS_TOKEN_RTC_MONTH:
                snprintf(buf, sizeof(buf), "rtc: month (01..12)");
                break;
            case WPS_TOKEN_RTC_MINUTE:
                snprintf(buf, sizeof(buf), "rtc: minute (00..59)");
                break;
            case WPS_TOKEN_RTC_SECOND:
                snprintf(buf, sizeof(buf), "rtc: second (00..59)");
                break;
            case WPS_TOKEN_RTC_YEAR_2_DIGITS:
                snprintf(buf, sizeof(buf),
                         "rtc: last two digits of year (00..99)");
                break;
            case WPS_TOKEN_RTC_YEAR_4_DIGITS:
                snprintf(buf, sizeof(buf), "rtc: year (1970...)");
                break;
            case WPS_TOKEN_RTC_AM_PM_UPPER:
                snprintf(buf, sizeof(buf),
                         "rtc: upper case AM or PM indicator");
                break;
            case WPS_TOKEN_RTC_AM_PM_LOWER:
                snprintf(buf, sizeof(buf),
                         "rtc: lower case am or pm indicator");
                break;
            case WPS_TOKEN_RTC_WEEKDAY_NAME:
                snprintf(buf, sizeof(buf),
                         "rtc: abbreviated weekday name (Sun..Sat)");
                break;
            case WPS_TOKEN_RTC_MONTH_NAME:
                snprintf(buf, sizeof(buf),
                         "rtc: abbreviated month name (Jan..Dec)");
                break;
            case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
                snprintf(buf, sizeof(buf),
                         "rtc: day of week (1..7); 1 is Monday");
                break;
            case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
                snprintf(buf, sizeof(buf),
                         "rtc: day of week (0..6); 0 is Sunday");
                break;

#if (CONFIG_CODEC == SWCODEC)
            case WPS_TOKEN_CROSSFADE:
                snprintf(buf, sizeof(buf), "crossfade");
                break;

            case WPS_TOKEN_REPLAYGAIN:
                snprintf(buf, sizeof(buf), "replaygain");
                break;
#endif

#ifdef HAVE_ALBUMART
            case WPS_TOKEN_ALBUMART_DISPLAY:
                snprintf(buf, sizeof(buf), "album art display at x=%d, y=%d, "
                         "maxwidth=%d, maxheight=%d", data->albumart_x,
                         data->albumart_y, data->albumart_max_width,
                         data->albumart_max_height);
                break;
#endif

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_IMAGE_BACKDROP:
                snprintf(buf, sizeof(buf), "backdrop image");
                break;

            case WPS_TOKEN_IMAGE_PROGRESS_BAR:
                snprintf(buf, sizeof(buf), "progressbar bitmap");
                break;

            case WPS_TOKEN_PEAKMETER:
                snprintf(buf, sizeof(buf), "peakmeter");
                break;
#endif

            case WPS_TOKEN_PROGRESSBAR:
                snprintf(buf, sizeof(buf), "progressbar");
                break;

#ifdef HAVE_LCD_CHARCELLS
            case WPS_TOKEN_PLAYER_PROGRESSBAR:
                snprintf(buf, sizeof(buf), "full line progressbar");
                break;
#endif

            case WPS_TOKEN_TRACK_TIME_ELAPSED:
                snprintf(buf, sizeof(buf), "time elapsed in track");
                break;

            case WPS_TOKEN_TRACK_ELAPSED_PERCENT:
                snprintf(buf, sizeof(buf), "played percentage of track");
                break;

            case WPS_TOKEN_PLAYLIST_ENTRIES:
                snprintf(buf, sizeof(buf), "number of entries in playlist");
                break;

            case WPS_TOKEN_PLAYLIST_NAME:
                snprintf(buf, sizeof(buf), "playlist name");
                break;

            case WPS_TOKEN_PLAYLIST_POSITION:
                snprintf(buf, sizeof(buf), "position in playlist");
                break;

            case WPS_TOKEN_TRACK_TIME_REMAINING:
                snprintf(buf, sizeof(buf), "time remaining in track");
                break;

            case WPS_TOKEN_PLAYLIST_SHUFFLE:
                snprintf(buf, sizeof(buf), "playlist shuffle mode");
                break;

            case WPS_TOKEN_TRACK_LENGTH:
                snprintf(buf, sizeof(buf), "track length");
                break;

            case WPS_TOKEN_VOLUME:
                snprintf(buf, sizeof(buf), "volume");
                break;

            case WPS_TOKEN_METADATA_ARTIST:
                snprintf(buf, sizeof(buf), "%strack artist",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_COMPOSER:
                snprintf(buf, sizeof(buf), "%strack composer",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_ALBUM:
                snprintf(buf, sizeof(buf), "%strack album",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_GROUPING:
                snprintf(buf, sizeof(buf), "%strack grouping",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_GENRE:
                snprintf(buf, sizeof(buf), "%strack genre",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_DISC_NUMBER:
                snprintf(buf, sizeof(buf), "%strack disc", next_str(next));
                break;

            case WPS_TOKEN_METADATA_TRACK_NUMBER:
                snprintf(buf, sizeof(buf), "%strack number",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_TRACK_TITLE:
                snprintf(buf, sizeof(buf), "%strack title",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_VERSION:
                snprintf(buf, sizeof(buf), "%strack ID3 version",
                         next_str(next));
                break;

            case WPS_TOKEN_METADATA_YEAR:
                snprintf(buf, sizeof(buf), "%strack year", next_str(next));
                break;

            case WPS_TOKEN_BATTERY_PERCENT:
                snprintf(buf, sizeof(buf), "battery percentage");
                break;

            case WPS_TOKEN_BATTERY_VOLTS:
                snprintf(buf, sizeof(buf), "battery voltage");
                break;

            case WPS_TOKEN_BATTERY_TIME:
                snprintf(buf, sizeof(buf), "battery time left");
                break;

            case WPS_TOKEN_BATTERY_CHARGER_CONNECTED:
                snprintf(buf, sizeof(buf), "battery charger connected");
                break;

            case WPS_TOKEN_BATTERY_CHARGING:
                snprintf(buf, sizeof(buf), "battery charging");
                break;

            case WPS_TOKEN_FILE_BITRATE:
                snprintf(buf, sizeof(buf), "%sfile bitrate", next_str(next));
                break;

            case WPS_TOKEN_FILE_CODEC:
                snprintf(buf, sizeof(buf), "%sfile codec", next_str(next));
                break;

            case WPS_TOKEN_FILE_FREQUENCY:
                snprintf(buf, sizeof(buf), "%sfile audio frequency in Hz",
                         next_str(next));
                break;

            case WPS_TOKEN_FILE_FREQUENCY_KHZ:
                snprintf(buf, sizeof(buf), "%sfile audio frequency in KHz",
                         next_str(next));
                break;

            case WPS_TOKEN_FILE_NAME:
                snprintf(buf, sizeof(buf), "%sfile name", next_str(next));
                break;

            case WPS_TOKEN_FILE_NAME_WITH_EXTENSION:
                snprintf(buf, sizeof(buf), "%sfile name with extension",
                         next_str(next));
                break;

            case WPS_TOKEN_FILE_PATH:
                snprintf(buf, sizeof(buf), "%sfile path", next_str(next));
                break;

            case WPS_TOKEN_FILE_SIZE:
                snprintf(buf, sizeof(buf), "%sfile size", next_str(next));
                break;

            case WPS_TOKEN_FILE_VBR:
                snprintf(buf, sizeof(buf), "%sfile is vbr", next_str(next));
                break;

            case WPS_TOKEN_FILE_DIRECTORY:
                snprintf(buf, sizeof(buf), "%sfile directory, level: %d",
                         next_str(next), token->value.i);
                break;

            default:
                snprintf(buf, sizeof(buf), "FIXME (code: %d)",
                         token->type);
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
    int i, j;
    struct wps_line *line;
    struct wps_subline *subline;

    if (wps_verbose_level > 0)
    {
        DEBUGF("Number of lines   : %d\n", data->num_lines);
        DEBUGF("Number of sublines: %d\n", data->num_sublines);
        DEBUGF("Number of tokens  : %d\n", data->num_tokens);
        DEBUGF("\n");
    }

    if (wps_verbose_level > 1)
    {
        for (i = 0, line = data->lines; i < data->num_lines; i++,line++)
        {
            DEBUGF("Line %2d (num_sublines=%d, first_subline=%d)\n",
                   i, line->num_sublines, line->first_subline_idx);

            for (j = 0, subline = data->sublines + line->first_subline_idx;
                 j < line->num_sublines; j++, subline++)
            {
                DEBUGF("    Subline %d: first_token=%3d, last_token=%3d",
                       j, subline->first_token_idx,
                       wps_last_token_index(data, i, j));

                if (subline->line_type & WPS_REFRESH_SCROLL)
                    DEBUGF(", scrolled");
                else if (subline->line_type & WPS_REFRESH_PLAYER_PROGRESS)
                    DEBUGF(", progressbar");
                else if (subline->line_type & WPS_REFRESH_PEAK_METER)
                    DEBUGF(", peakmeter");

                DEBUGF("\n");
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

#ifdef HAVE_LCD_BITMAP
static void print_img_cond_indexes(struct wps_data *data)
{
    DEBUGF("Image conditional indexes:\n");
    int i;
    for (i = 0; i < MAX_IMAGES; i++)
    {
        if (data->img[i].cond_index)
            DEBUGF("%2d: %d\n", i, data->img[i].cond_index);
    }
    DEBUGF("\n");
}
#endif /*HAVE_LCD_BITMAP */

void print_debug_info(struct wps_data *data, int fail, int line)
{
#if defined(SIMULATOR) || defined(__PCTOOL__)
    if (debug_wps && wps_verbose_level)
    {
        dump_wps_tokens(data);
        print_wps_strings(data);
        print_line_info(data);
#ifdef HAVE_LCD_BITMAP
        if (wps_verbose_level > 2) print_img_cond_indexes(data);
#endif
    }
#endif /* SIMULATOR */

    if (data->num_tokens >= WPS_MAX_TOKENS - 1) {
        DEBUGF("Warning: Max number of tokens was reached (%d)\n",
               WPS_MAX_TOKENS - 1);
    }

    if (fail)
    {
        DEBUGF("Failed parsing on line %d : ", line);
        switch (fail)
        {
            case PARSE_FAIL_UNCLOSED_COND:
                DEBUGF("Unclosed conditional");
                break;

            case PARSE_FAIL_INVALID_CHAR:
                DEBUGF("Invalid conditional char (not in an open conditional)");
                break;

            case PARSE_FAIL_COND_SYNTAX_ERROR:
                DEBUGF("Conditional syntax error");
                break;
        }
        DEBUGF("\n");
    }
}

#endif /* DEBUG */
