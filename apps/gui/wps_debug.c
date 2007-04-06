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
#include "debug.h"

void dump_wps_tokens(struct wps_data *data)
{
    int i, j;
    int indent = 0;
    char buf[64];
    bool next;

    /* Dump parsed WPS */
    for(i = 0; i < data->num_tokens && i < WPS_MAX_TOKENS; i++) {

        next = data->tokens[i].next;

        switch(data->tokens[i].type) {
            case WPS_TOKEN_UNKNOWN:
                snprintf(buf, sizeof(buf), "Unknown token");
                break;
            case WPS_TOKEN_CHARACTER:
                snprintf(buf, sizeof(buf), "Character '%c'",
                         data->tokens[i].value.c);
                break;

            case WPS_TOKEN_STRING:
                snprintf(buf, sizeof(buf), "String '%s'",
                         data->strings[data->tokens[i].value.i]);
                break;

            case WPS_TOKEN_EOL:
                snprintf(buf, sizeof(buf), "%s", "EOL");
                break;

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_ALIGN_LEFT:
                snprintf(buf, sizeof(buf), "%s", "align left");
                break;

            case WPS_TOKEN_ALIGN_CENTER:
                snprintf(buf, sizeof(buf), "%s", "align center");
                break;

            case WPS_TOKEN_ALIGN_RIGHT:
                snprintf(buf, sizeof(buf), "%s", "align right");
                break;
#endif

            case WPS_TOKEN_CONDITIONAL:
            snprintf(buf, sizeof(buf), "%s, %d options", "conditional",
                     data->tokens[i].value.i);
                break;

            case WPS_TOKEN_CONDITIONAL_START:
                snprintf(buf, sizeof(buf), "%s, next cond: %d",
                         "conditional start", data->tokens[i].value.i);
                indent++;
                break;

            case WPS_TOKEN_CONDITIONAL_OPTION:
                snprintf(buf, sizeof(buf), "%s, next cond: %d",
                         "conditional option", data->tokens[i].value.i);
                break;

            case WPS_TOKEN_CONDITIONAL_END:
                snprintf(buf, sizeof(buf), "%s", "conditional end");
                indent--;
                break;

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_IMAGE_PRELOAD:
                snprintf(buf, sizeof(buf), "%s", "preload image");
                break;

            case WPS_TOKEN_IMAGE_PRELOAD_DISPLAY:
                snprintf(buf, sizeof(buf), "%s %d", "display preloaded image",
                         data->tokens[i].value.i);
                break;

            case WPS_TOKEN_IMAGE_DISPLAY:
                snprintf(buf, sizeof(buf), "%s", "display image");
                break;
#endif

#ifdef HAS_BUTTON_HOLD
            case WPS_TOKEN_MAIN_HOLD:
                snprintf(buf, sizeof(buf), "%s", "mode hold");
                break;
#endif

#ifdef HAS_REMOTE_BUTTON_HOLD
            case WPS_TOKEN_REMOTE_HOLD:
                snprintf(buf, sizeof(buf), "%s", "mode remote hold");
                break;
#endif

            case WPS_TOKEN_REPEAT_MODE:
                snprintf(buf, sizeof(buf), "%s", "mode repeat");
                break;

            case WPS_TOKEN_PLAYBACK_STATUS:
                snprintf(buf, sizeof(buf), "%s", "mode playback");
                break;

#if CONFIG_RTC
            case WPS_TOKEN_RTC_DAY_OF_MONTH:
            case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
            case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
            case WPS_TOKEN_RTC_HOUR_24:
            case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
            case WPS_TOKEN_RTC_HOUR_12:
            case WPS_TOKEN_RTC_MONTH:
            case WPS_TOKEN_RTC_MINUTE:
            case WPS_TOKEN_RTC_SECOND:
            case WPS_TOKEN_RTC_YEAR_2_DIGITS:
            case WPS_TOKEN_RTC_YEAR_4_DIGITS:
            case WPS_TOKEN_RTC_AM_PM_UPPER:
            case WPS_TOKEN_RTC_AM_PM_LOWER:
            case WPS_TOKEN_RTC_WEEKDAY_NAME:
            case WPS_TOKEN_RTC_MONTH_NAME:
            case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
            case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
            case WPS_TOKEN_RTC:
            snprintf(buf, sizeof(buf), "%s %c", "real-time clock tag:",
                     data->tokens[i].value.c);
                break;
#endif

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_IMAGE_BACKDROP:
                snprintf(buf, sizeof(buf), "%s", "backdrop image");
                break;

            case WPS_TOKEN_IMAGE_PROGRESS_BAR:
                snprintf(buf, sizeof(buf), "%s", "progressbar bitmap");
                break;


            case WPS_TOKEN_STATUSBAR_ENABLED:
                snprintf(buf, sizeof(buf), "%s", "statusbar enable");
                break;

            case WPS_TOKEN_STATUSBAR_DISABLED:
                snprintf(buf, sizeof(buf), "%s", "statusbar disable");
                break;

            case WPS_TOKEN_PEAKMETER:
                snprintf(buf, sizeof(buf), "%s", "peakmeter");
                break;
#endif

            case WPS_TOKEN_PROGRESSBAR:
                snprintf(buf, sizeof(buf), "%s", "progressbar");
                break;

#ifdef HAVE_LCD_CHARCELLS
            case WPS_TOKEN_PLAYER_PROGRESSBAR:
                snprintf(buf, sizeof(buf), "%s", "full line progressbar");
                break;
#endif

            case WPS_TOKEN_TRACK_TIME_ELAPSED:
                snprintf(buf, sizeof(buf), "%s", "time elapsed in track");
                break;

            case WPS_TOKEN_PLAYLIST_ENTRIES:
                snprintf(buf, sizeof(buf), "%s", "number of entries in playlist");
                break;

            case WPS_TOKEN_PLAYLIST_NAME:
                snprintf(buf, sizeof(buf), "%s", "playlist name");
                break;

            case WPS_TOKEN_PLAYLIST_POSITION:
                snprintf(buf, sizeof(buf), "%s", "position in playlist");
                break;

            case WPS_TOKEN_TRACK_TIME_REMAINING:
                snprintf(buf, sizeof(buf), "%s", "time remaining in track");
                break;

            case WPS_TOKEN_PLAYLIST_SHUFFLE:
                snprintf(buf, sizeof(buf), "%s", "playlist shuffle mode");
                break;

            case WPS_TOKEN_TRACK_LENGTH:
                snprintf(buf, sizeof(buf), "%s", "track length");
                break;

            case WPS_TOKEN_VOLUME:
                snprintf(buf, sizeof(buf), "%s", "volume");
                break;

            case WPS_TOKEN_METADATA_ARTIST:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track artist");
                break;

            case WPS_TOKEN_METADATA_COMPOSER:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track composer");
                break;

            case WPS_TOKEN_METADATA_ALBUM:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track album");
                break;

            case WPS_TOKEN_METADATA_GENRE:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track genre");
                break;

            case WPS_TOKEN_METADATA_TRACK_NUMBER:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track number");
                break;

            case WPS_TOKEN_METADATA_TRACK_TITLE:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track title");
                break;

            case WPS_TOKEN_METADATA_VERSION:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track ID3 version");
                break;

            case WPS_TOKEN_METADATA_YEAR:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "track year");
                break;

            case WPS_TOKEN_BATTERY_PERCENT:
                snprintf(buf, sizeof(buf), "%s", "battery percentage");
                break;

            case WPS_TOKEN_BATTERY_VOLTS:
                snprintf(buf, sizeof(buf), "%s", "battery voltage");
                break;

            case WPS_TOKEN_BATTERY_TIME:
                snprintf(buf, sizeof(buf), "%s", "battery time left");
                break;

            case WPS_TOKEN_BATTERY_CHARGER_CONNECTED:
                snprintf(buf, sizeof(buf), "%s", "battery charger connected");
                break;

            case WPS_TOKEN_BATTERY_CHARGING:
                snprintf(buf, sizeof(buf), "%s", "battery charging");
                break;

            case WPS_TOKEN_FILE_BITRATE:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file bitrate");
                break;

            case WPS_TOKEN_FILE_CODEC:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file codec");
                break;

            case WPS_TOKEN_FILE_FREQUENCY:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file audio frequency");
                break;

            case WPS_TOKEN_FILE_NAME:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file name");
                break;

            case WPS_TOKEN_FILE_NAME_WITH_EXTENSION:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file name with extension");
                break;

            case WPS_TOKEN_FILE_PATH:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file path");
                break;

            case WPS_TOKEN_FILE_SIZE:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file size");
                break;

            case WPS_TOKEN_FILE_VBR:
                snprintf(buf, sizeof(buf), "%s%s", next ? "next " : "",
                         "file is vbr");
                break;

            case WPS_TOKEN_FILE_DIRECTORY:
                snprintf(buf, sizeof(buf), "%s%s: %d", next ? "next " : "",
                         "file directory, level",
                         data->tokens[i].value.i);
                break;

            case WPS_TOKEN_SCROLL:
                snprintf(buf, sizeof(buf), "%s", "scrolling line");
                break;

            case WPS_TOKEN_SUBLINE_TIMEOUT:
                snprintf(buf, sizeof(buf), "%s: %d", "subline timeout value",
                         data->tokens[i].value.i);
                break;

            case WPS_TOKEN_SUBLINE_SEPARATOR:
                snprintf(buf, sizeof(buf), "%s", "subline separator");
                break;

            default:
                snprintf(buf, sizeof(buf), "%s (code: %d)", "FIXME",
                         data->tokens[i].type);
                break;
        }

        for(j = 0; j < indent; j++) {
            DEBUGF("\t");
        }

        DEBUGF("[%03d] = %s\n", i, buf);
    }
    DEBUGF("\n");
}

void print_line_info(struct wps_data *data)
{
    int line, subline;

    DEBUGF("line/subline start indexes :\n");
    for (line = 0; line < data->num_lines; line++)
    {
        DEBUGF("%2d. ", line);
        for (subline = 0; subline < data->num_sublines[line]; subline++)
        {
            DEBUGF("%3d ", data->format_lines[line][subline]);
        }
        DEBUGF("\n");
    }

    DEBUGF("subline time multipliers :\n");
    for (line = 0; line < data->num_lines; line++)
    {
        DEBUGF("%2d. ", line);
        for (subline = 0; subline < data->num_sublines[line]; subline++)
        {
            DEBUGF("%3d ", data->time_mult[line][subline]);
        }
        DEBUGF("\n");
    }

    DEBUGF("\n");
}

void print_wps_strings(struct wps_data *data)
{
    DEBUGF("strings :\n");
    int i, len = 0;
    for (i=0; i < data->num_strings; i++)
    {
        len += strlen(data->strings[i]);
        DEBUGF("%2d: '%s'\n", i, data->strings[i]);
    }
    DEBUGF("total length : %d\n", len);
    DEBUGF("\n");
}

#ifdef HAVE_LCD_BITMAP
void print_img_cond_indexes(struct wps_data *data)
{
    DEBUGF("image conditional indexes :\n");
    int i;
    for (i=0; i < MAX_IMAGES; i++)
    {
        if (data->img[i].cond_index)
            DEBUGF("%2d: %d\n", i, data->img[i].cond_index);
    }
    DEBUGF("\n");
}
#endif /*HAVE_LCD_BITMAP */

#endif /* DEBUG */
