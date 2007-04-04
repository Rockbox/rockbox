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

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atoi.h"
#include "gwps.h"
#include "settings.h"
#include "debug.h"
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#if LCD_DEPTH > 1
#include "backdrop.h"
#endif
#endif

#define WPS_DEFAULTCFG WPS_DIR "/rockbox_default.wps"
#define RWPS_DEFAULTCFG WPS_DIR "/rockbox_default.rwps"

/* level of current conditional.
   -1 means we're not in a conditional. */
int level = -1;

/* index of the last WPS_TOKEN_CONDITIONAL_OPTION
    or WPS_TOKEN_CONDITIONAL_START in current level */
int lastcond[WPS_MAX_COND_LEVEL];

/* index of the WPS_TOKEN_CONDITIONAL in current level */
int condindex[WPS_MAX_COND_LEVEL];

/* number of condtional options in current level */
int numoptions[WPS_MAX_COND_LEVEL];

#ifdef HAVE_LCD_BITMAP
/* pointers to the bitmap filenames in the WPS source */
const char *bmp_names[MAX_IMAGES];
const char *pb_bmp_name;
#if LCD_DEPTH > 1
const char *backdrop_bmp_name;
#endif
#endif

#ifdef DEBUG
/* debugging functions */
extern void dump_wps_tokens(struct wps_data *data);
extern void print_line_info(struct wps_data *data);
extern void print_img_cond_indexes(struct wps_data *data);
extern void print_wps_strings(struct wps_data *data);
#endif

typedef int (*wps_tag_parse_func)(const char *wps_token, struct wps_data *wps_data);

struct wps_tag {
    const char name[3];
    enum wps_token_type type;
    unsigned char refresh_type;
    wps_tag_parse_func parse_func;
};

/* prototypes of all special parse functions : */

static int parse_subline_timeout(const char *wps_token, struct wps_data *wps_data);
static int parse_progressbar(const char *wps_token, struct wps_data *wps_data);
static int parse_dir_level(const char *wps_token, struct wps_data *wps_data);
#ifdef HAVE_LCD_BITMAP
static int parse_image_special(const char *wps_token, struct wps_data *wps_data);
static int parse_statusbar(const char *wps_token, struct wps_data *wps_data);
static int parse_image_display(const char *wps_token, struct wps_data *wps_data);
static int parse_image_load(const char *wps_token, struct wps_data *wps_data);
#endif /*HAVE_LCD_BITMAP */
#if CONFIG_RTC
static int parse_rtc_format(const char *wps_token, struct wps_data *wps_data);

/* RTC tokens array */
static const struct wps_tag rtc_tags[] = {
    { "d", WPS_TOKEN_RTC_DAY_OF_MONTH,              0, NULL },
    { "e", WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED, 0, NULL },
    { "H", WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED,       0, NULL },
    { "k", WPS_TOKEN_RTC_HOUR_24,                   0, NULL },
    { "I", WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED,       0, NULL },
    { "l", WPS_TOKEN_RTC_HOUR_12,                   0, NULL },
    { "m", WPS_TOKEN_RTC_MONTH,                     0, NULL },
    { "M", WPS_TOKEN_RTC_MINUTE,                    0, NULL },
    { "S", WPS_TOKEN_RTC_SECOND,                    0, NULL },
    { "y", WPS_TOKEN_RTC_YEAR_2_DIGITS,             0, NULL },
    { "Y", WPS_TOKEN_RTC_YEAR_4_DIGITS,             0, NULL },
    { "p", WPS_TOKEN_RTC_AM_PM_UPPER,               0, NULL },
    { "P", WPS_TOKEN_RTC_AM_PM_LOWER,               0, NULL },
    { "a", WPS_TOKEN_RTC_WEEKDAY_NAME,              0, NULL },
    { "b", WPS_TOKEN_RTC_MONTH_NAME,                0, NULL },
    { "u", WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON,     0, NULL },
    { "w", WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN,     0, NULL },
    { "\0",WPS_TOKEN_CHARACTER,                     0, NULL }
    /* the array MUST end with a "\0" token */
};
#endif

/* array of available tags - those with more characters have to go first
   (e.g. "xl" and "xd" before "x"). It needs to end with the unknown token. */
static const struct wps_tag all_tags[] = {

    { "ac",  WPS_TOKEN_ALIGN_CENTER,             0,                   NULL },
    { "al",  WPS_TOKEN_ALIGN_LEFT,               0,                   NULL },
    { "ar",  WPS_TOKEN_ALIGN_RIGHT,              0,                   NULL },

    { "bl",  WPS_TOKEN_BATTERY_PERCENT,          WPS_REFRESH_DYNAMIC, NULL },
    { "bv",  WPS_TOKEN_BATTERY_VOLTS,            WPS_REFRESH_DYNAMIC, NULL },
    { "bt",  WPS_TOKEN_BATTERY_TIME,             WPS_REFRESH_DYNAMIC, NULL },
    { "bs",  WPS_TOKEN_BATTERY_SLEEPTIME,        WPS_REFRESH_DYNAMIC, NULL },
#if CONFIG_CHARGING >= CHARGING_MONITOR
    { "bc",  WPS_TOKEN_BATTERY_CHARGING,         WPS_REFRESH_DYNAMIC, NULL },
#endif
#if CONFIG_CHARGING
    { "bp",  WPS_TOKEN_BATTERY_CHARGER_CONNECTED,WPS_REFRESH_DYNAMIC, NULL },
#endif

#if CONFIG_RTC
    { "c",   WPS_TOKEN_RTC,          WPS_REFRESH_DYNAMIC, parse_rtc_format },
#endif

    /* current file */
    { "fb",  WPS_TOKEN_FILE_BITRATE,             WPS_REFRESH_STATIC,  NULL },
    { "fc",  WPS_TOKEN_FILE_CODEC,               WPS_REFRESH_STATIC,  NULL },
    { "ff",  WPS_TOKEN_FILE_FREQUENCY,           WPS_REFRESH_STATIC,  NULL },
    { "fm",  WPS_TOKEN_FILE_NAME_WITH_EXTENSION, WPS_REFRESH_STATIC,  NULL },
    { "fn",  WPS_TOKEN_FILE_NAME,                WPS_REFRESH_STATIC,  NULL },
    { "fp",  WPS_TOKEN_FILE_PATH,                WPS_REFRESH_STATIC,  NULL },
    { "fs",  WPS_TOKEN_FILE_SIZE,                WPS_REFRESH_STATIC,  NULL },
    { "fv",  WPS_TOKEN_FILE_VBR,                 WPS_REFRESH_STATIC,  NULL },
    { "d",   WPS_TOKEN_FILE_DIRECTORY, WPS_REFRESH_STATIC, parse_dir_level },

    /* next file */
    { "Fb",  WPS_TOKEN_FILE_BITRATE,             WPS_REFRESH_DYNAMIC, NULL },
    { "Fc",  WPS_TOKEN_FILE_CODEC,               WPS_REFRESH_DYNAMIC, NULL },
    { "Ff",  WPS_TOKEN_FILE_FREQUENCY,           WPS_REFRESH_DYNAMIC, NULL },
    { "Fm",  WPS_TOKEN_FILE_NAME_WITH_EXTENSION, WPS_REFRESH_DYNAMIC, NULL },
    { "Fn",  WPS_TOKEN_FILE_NAME,                WPS_REFRESH_DYNAMIC, NULL },
    { "Fp",  WPS_TOKEN_FILE_PATH,                WPS_REFRESH_DYNAMIC, NULL },
    { "Fs",  WPS_TOKEN_FILE_SIZE,                WPS_REFRESH_DYNAMIC, NULL },
    { "Fv",  WPS_TOKEN_FILE_VBR,                 WPS_REFRESH_DYNAMIC, NULL },
    { "D",   WPS_TOKEN_FILE_DIRECTORY, WPS_REFRESH_DYNAMIC,parse_dir_level },

    /* current metadata */
    { "ia",  WPS_TOKEN_METADATA_ARTIST,          WPS_REFRESH_STATIC,  NULL },
    { "ic",  WPS_TOKEN_METADATA_COMPOSER,        WPS_REFRESH_STATIC,  NULL },
    { "id",  WPS_TOKEN_METADATA_ALBUM,           WPS_REFRESH_STATIC,  NULL },
    { "iA",  WPS_TOKEN_METADATA_ALBUM_ARTIST,    WPS_REFRESH_STATIC,  NULL },
    { "ig",  WPS_TOKEN_METADATA_GENRE,           WPS_REFRESH_STATIC,  NULL },
    { "in",  WPS_TOKEN_METADATA_TRACK_NUMBER,    WPS_REFRESH_STATIC,  NULL },
    { "it",  WPS_TOKEN_METADATA_TRACK_TITLE,     WPS_REFRESH_STATIC,  NULL },
    { "iv",  WPS_TOKEN_METADATA_VERSION,         WPS_REFRESH_STATIC,  NULL },
    { "iy",  WPS_TOKEN_METADATA_YEAR,            WPS_REFRESH_STATIC,  NULL },
    { "iC",  WPS_TOKEN_METADATA_COMMENT,         WPS_REFRESH_DYNAMIC, NULL },

    /* next metadata */
    { "Ia",  WPS_TOKEN_METADATA_ARTIST,          WPS_REFRESH_DYNAMIC, NULL },
    { "Ic",  WPS_TOKEN_METADATA_COMPOSER,        WPS_REFRESH_DYNAMIC, NULL },
    { "Id",  WPS_TOKEN_METADATA_ALBUM,           WPS_REFRESH_DYNAMIC, NULL },
    { "IA",  WPS_TOKEN_METADATA_ALBUM_ARTIST,    WPS_REFRESH_STATIC,  NULL },
    { "Ig",  WPS_TOKEN_METADATA_GENRE,           WPS_REFRESH_DYNAMIC, NULL },
    { "In",  WPS_TOKEN_METADATA_TRACK_NUMBER,    WPS_REFRESH_DYNAMIC, NULL },
    { "It",  WPS_TOKEN_METADATA_TRACK_TITLE,     WPS_REFRESH_DYNAMIC, NULL },
    { "Iv",  WPS_TOKEN_METADATA_VERSION,         WPS_REFRESH_DYNAMIC, NULL },
    { "Iy",  WPS_TOKEN_METADATA_YEAR,            WPS_REFRESH_DYNAMIC, NULL },
    { "IC",  WPS_TOKEN_METADATA_COMMENT,         WPS_REFRESH_DYNAMIC, NULL },

#if (CONFIG_CODEC == SWCODEC)
    { "Sp",  WPS_TOKEN_SOUND_PITCH,              WPS_REFRESH_DYNAMIC, NULL },
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    { "lh",  WPS_TOKEN_VLED_HDD,                 WPS_REFRESH_DYNAMIC, NULL },
#endif

#ifdef HAS_BUTTON_HOLD
    { "mh",  WPS_TOKEN_MAIN_HOLD,                WPS_REFRESH_DYNAMIC, NULL },
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
    { "mr",  WPS_TOKEN_REMOTE_HOLD,              WPS_REFRESH_DYNAMIC, NULL },
#endif

    { "mm",  WPS_TOKEN_REPEAT_MODE,              WPS_REFRESH_DYNAMIC, NULL },
    { "mp",  WPS_TOKEN_PLAYBACK_STATUS,          WPS_REFRESH_DYNAMIC, NULL },

#ifdef HAVE_LCD_BITMAP
    { "pm",  WPS_TOKEN_PEAKMETER,
             WPS_REFRESH_PEAK_METER, NULL },
#else
    { "pf",  WPS_TOKEN_PLAYER_PROGRESSBAR,
             WPS_REFRESH_DYNAMIC | WPS_REFRESH_PLAYER_PROGRESS,
             parse_progressbar },
#endif
    { "pb",  WPS_TOKEN_PROGRESSBAR,
             WPS_REFRESH_PLAYER_PROGRESS, parse_progressbar },

    { "pv",  WPS_TOKEN_VOLUME,                   WPS_REFRESH_DYNAMIC, NULL },

    { "pc",  WPS_TOKEN_TRACK_TIME_ELAPSED,       WPS_REFRESH_DYNAMIC, NULL },
    { "pr",  WPS_TOKEN_TRACK_TIME_REMAINING,     WPS_REFRESH_DYNAMIC, NULL },
    { "pt",  WPS_TOKEN_TRACK_LENGTH,             WPS_REFRESH_STATIC,  NULL },

    { "pp",  WPS_TOKEN_PLAYLIST_POSITION,        WPS_REFRESH_STATIC,  NULL },
    { "pe",  WPS_TOKEN_PLAYLIST_ENTRIES,         WPS_REFRESH_STATIC,  NULL },
    { "pn",  WPS_TOKEN_PLAYLIST_NAME,            WPS_REFRESH_STATIC,  NULL },
    { "ps",  WPS_TOKEN_PLAYLIST_SHUFFLE,         WPS_REFRESH_DYNAMIC, NULL },

    { "rp",  WPS_TOKEN_DATABASE_PLAYCOUNT,       WPS_REFRESH_DYNAMIC, NULL },
    { "rr",  WPS_TOKEN_DATABASE_RATING,          WPS_REFRESH_DYNAMIC, NULL },
#if CONFIG_CODEC == SWCODEC
    { "rg",  WPS_TOKEN_REPLAYGAIN,               WPS_REFRESH_STATIC,  NULL },
#endif

    { "s",   WPS_TOKEN_SCROLL,                   WPS_REFRESH_SCROLL,  NULL },
    { "t",   WPS_TOKEN_SUBLINE_TIMEOUT,          0,  parse_subline_timeout },

#ifdef HAVE_LCD_BITMAP
    { "we",  WPS_TOKEN_STATUSBAR_ENABLED,        0,        parse_statusbar },
    { "wd",  WPS_TOKEN_STATUSBAR_DISABLED,       0,        parse_statusbar },

    { "xl",  WPS_NO_TOKEN,                       0,       parse_image_load },

    { "xd",  WPS_TOKEN_IMAGE_PRELOAD_DISPLAY,
             WPS_REFRESH_STATIC, parse_image_display },

    { "x",   WPS_TOKEN_IMAGE_DISPLAY,            0,       parse_image_load },
    { "P",   WPS_TOKEN_IMAGE_PROGRESS_BAR,       0,    parse_image_special },
#if LCD_DEPTH > 1
    { "X",   WPS_TOKEN_IMAGE_BACKDROP,           0,    parse_image_special },
#endif
#endif

    { "\0",  WPS_TOKEN_UNKNOWN, 0, NULL }
    /* the array MUST end with a "\0" token */
};


static int skip_end_of_line(const char *wps_token)
{
    int skip = 0;
    while(*(wps_token + skip) != '\n')
        skip++;
    return ++skip;
}

#if CONFIG_RTC
static int parse_rtc_format(const char *wps_token, struct wps_data *wps_data)
{
    int skip = 0, i;

    /* RTC tag format ends with a c or a newline */
    while (wps_token && *wps_token != 'c' && *wps_token != '\n')
    {
        /* find what format char we have */
        i = 0;
        while (*(rtc_tags[i].name) && *wps_token != *(rtc_tags[i].name))
            i++;

        wps_data->num_tokens++;
        wps_data->tokens[wps_data->num_tokens].type = rtc_tags[i].type;
        wps_data->tokens[wps_data->num_tokens].value.c = *wps_token;
        skip ++;
        wps_token++;
    }

    /* eat the unwanted c at the end of the format */
    if (*wps_token == 'c')
        skip++;

    return skip;
}
#endif

#ifdef HAVE_LCD_BITMAP

static int parse_statusbar(const char *wps_token, struct wps_data *wps_data)
{
    wps_data->wps_sb_tag = true;

    if (wps_data->tokens[wps_data->num_tokens].type == WPS_TOKEN_STATUSBAR_ENABLED)
        wps_data->show_sb_on_wps = true;
    else
        wps_data->show_sb_on_wps = false;

    /* Skip the rest of the line */
    return skip_end_of_line(wps_token);
}

static bool load_bitmap(struct wps_data *wps_data,
                        char* filename,
                        struct bitmap *bm)
{
    int ret = read_bmp_file(filename, bm,
                            wps_data->img_buf_free,
                            FORMAT_ANY|FORMAT_TRANSPARENT);

    if (ret > 0)
    {
#if LCD_DEPTH == 16
        if (ret % 2) ret++;
        /* Always consume an even number of bytes */
#endif
        wps_data->img_buf_ptr += ret;
        wps_data->img_buf_free -= ret;

        return true;
    }
    else
        return false;
}

static int get_image_id(int c)
{
    if(c >= 'a' && c <= 'z')
        return c - 'a';
    else if(c >= 'A' && c <= 'Z')
        return c - 'A' + 26;
    else
        return -1;
}

static char *get_image_filename(const char *start, const char* bmpdir,
                                char *buf, int buf_size)
{
    const char *end = strchr(start, '|');

    if ( !end || (end - start) >= (buf_size - ROCKBOX_DIR_LEN - 2) )
    {
        buf = "\0";
        return NULL;
    }

    int bmpdirlen = strlen(bmpdir);

    strcpy(buf, bmpdir);
    buf[bmpdirlen] = '/';
    memcpy( &buf[bmpdirlen + 1], start, end - start);
    buf[bmpdirlen + 1 + end - start] = 0;

    return buf;
}

static int parse_image_display(const char *wps_token, struct wps_data *wps_data)
{
    int n = get_image_id(*wps_token);
    wps_data->tokens[wps_data->num_tokens].value.i = n;

    /* if the image is in a conditional, remember it */
    if (level >= 0)
        wps_data->img[n].cond_index = condindex[level];

    return 1;
}

static int parse_image_load(const char *wps_token, struct wps_data *wps_data)
{
    int n;
    const char *ptr = wps_token;
    char *pos = NULL;

    /* format: %x|n|filename.bmp|x|y|
       or %xl|n|filename.bmp|x|y|  */

    ptr = strchr(ptr, '|') + 1;
    pos = strchr(ptr, '|');
    if (pos)
    {
        /* get the image ID */
        n = get_image_id(*ptr);

        /* check the image number and load state */
        if(n < 0 || n >= MAX_IMAGES || wps_data->img[n].loaded)
        {
            /* Skip the rest of the line */
            return skip_end_of_line(wps_token);
        }

        ptr = pos + 1;

        /* get image name */
        bmp_names[n] = ptr;

        pos = strchr(ptr, '|');
        ptr = pos + 1;

        /* get x-position */
        pos = strchr(ptr, '|');
        if (pos)
            wps_data->img[n].x = atoi(ptr);
        else
        {
            /* weird syntax, bail out */
            return skip_end_of_line(wps_token);
        }

        /* get y-position */
        ptr = pos + 1;
        pos = strchr(ptr, '|');
        if (pos)
            wps_data->img[n].y = atoi(ptr);
        else
        {
            /* weird syntax, bail out */
            return skip_end_of_line(wps_token);
        }

        if (wps_data->tokens[wps_data->num_tokens].type == WPS_TOKEN_IMAGE_DISPLAY)
            wps_data->img[n].always_display = true;
    }

    /* Skip the rest of the line */
    return skip_end_of_line(wps_token);
}

static int parse_image_special(const char *wps_token, struct wps_data *wps_data)
{
    if (wps_data->tokens[wps_data->num_tokens].type == WPS_TOKEN_IMAGE_PROGRESS_BAR)
    {
        /* format: %P|filename.bmp| */
        pb_bmp_name = wps_token + 1;
    }
#if LCD_DEPTH > 1
    else if (wps_data->tokens[wps_data->num_tokens].type == WPS_TOKEN_IMAGE_BACKDROP)
    {
        /* format: %X|filename.bmp| */
        backdrop_bmp_name = wps_token + 1;
    }
#endif

    (void)wps_data; /* to avoid a warning */

    /* Skip the rest of the line */
    return skip_end_of_line(wps_token);
}

#endif /* HAVE_LCD_BITMAP */

static int parse_dir_level(const char *wps_token, struct wps_data *wps_data)
{
    char val[] = { *wps_token, '\0' };
    wps_data->tokens[wps_data->num_tokens].value.i = atoi(val);
    return 1;
}

static int parse_subline_timeout(const char *wps_token, struct wps_data *wps_data)
{
    int skip = 0;
    int val = 0;
    bool have_point = false;
    bool have_tenth = false;

    while ( isdigit(*wps_token) || *wps_token == '.' )
    {
        if (*wps_token != '.')
        {
            val *= 10;
            val += *wps_token - '0';
            if (have_point)
            {
                have_tenth = true;
                wps_token++;
                skip++;
                break;
            }
        }
        else
            have_point = true;

        wps_token++;
        skip++;
    }

    if (have_tenth == false)
        val *= 10;

    if (val > 0)
    {
        int line = wps_data->num_lines;
        int subline = wps_data->num_sublines[line];
        wps_data->time_mult[line][subline] = val;
    }

    wps_data->tokens[wps_data->num_tokens].value.i = val;
    return skip;
}

static int parse_progressbar(const char *wps_token, struct wps_data *wps_data)
{
#ifdef HAVE_LCD_BITMAP

    short *vals[] = {
        &wps_data->progress_height,
        &wps_data->progress_start,
        &wps_data->progress_end,
        &wps_data->progress_top };

    /* default values : */
    wps_data->progress_height = 6;
    wps_data->progress_start = 0;
    wps_data->progress_end = 0;
    wps_data->progress_top = -1;

    int i = 0;
    char *newline = strchr(wps_token, '\n');
    char *prev = strchr(wps_token, '|');
    if (prev && prev < newline) {
        char *next = strchr(prev+1, '|');
        while (i < 4 && next && next < newline)
        {
            *(vals[i++]) = atoi(++prev);
            prev = strchr(prev, '|');
            next = strchr(++next, '|');
        }

        if (wps_data->progress_height < 3)
            wps_data->progress_height = 3;
        if (wps_data->progress_end < wps_data->progress_start + 3)
            wps_data->progress_end = 0;
    }

    return newline - wps_token;

#else

    if (*(wps_token-1) == 'f')
        wps_data->full_line_progressbar = true;
    else
        wps_data->full_line_progressbar = false;

    return 0;

#endif
}

/* Parse a generic token from the given string. Return the length read */
static int parse_token(const char *wps_token, struct wps_data *wps_data)
{
    int skip = 0, taglen = 0;
    int i = 0;
    int line = wps_data->num_lines;
    int subline = wps_data->num_sublines[line];

    switch(*wps_token)
    {

        case '%':
        case '<':
        case '|':
        case '>':
        case ';':
            /* escaped characters */
            wps_data->tokens[wps_data->num_tokens].type = WPS_TOKEN_CHARACTER;
            wps_data->tokens[wps_data->num_tokens].value.c = *wps_token;
            wps_data->num_tokens++;
            skip++;
            break;

        case '?':
            /* conditional tag */
            wps_data->tokens[wps_data->num_tokens].type = WPS_TOKEN_CONDITIONAL;
            level++;
            condindex[level] = wps_data->num_tokens;
            numoptions[level] = 1;
            wps_data->num_tokens++;
            wps_token++;
            skip++;
            /* no "break" because a '?' is followed by a regular tag */

        default:
            /* find what tag we have */
            while (all_tags[i].name &&
                   strncmp(wps_token, all_tags[i].name, strlen(all_tags[i].name)))
                i++;

            taglen  = strlen(all_tags[i].name);
            skip += taglen;
            wps_data->tokens[wps_data->num_tokens].type = all_tags[i].type;

            /* if the tag has a special parsing function, we call it */
            if (all_tags[i].parse_func)
                skip += all_tags[i].parse_func(wps_token + taglen, wps_data);

            /* Some tags we don't want to save as tokens */
            if (all_tags[i].type == WPS_NO_TOKEN)
                break;

            /* tags that start with 'F', 'I' or 'D' are for the next file */
            if ( *(all_tags[i].name) == 'I' || *(all_tags[i].name) == 'F'
                                            || *(all_tags[i].name) == 'D')
                wps_data->tokens[wps_data->num_tokens].next = true;

            wps_data->line_type[line][subline] |= all_tags[i].refresh_type;
            wps_data->num_tokens++;
            break;
    }

    return skip;
}

static bool wps_parse(struct wps_data *data, const char *wps_buffer)
{
    if (!data || !wps_buffer || !*wps_buffer)
        return false;

    int subline;
    data->num_tokens = 0;
    char *current_string = data->string_buffer;

    while(wps_buffer && *wps_buffer && data->num_tokens < WPS_MAX_TOKENS
          && data->num_lines < WPS_MAX_LINES)
    {
        switch(*wps_buffer++)
        {

            /* Regular tag */
            case '%':
                wps_buffer += parse_token(wps_buffer, data);
                break;

            /* Alternating sublines separator */
            case ';':
                if (data->num_sublines[data->num_lines]+1 < WPS_MAX_SUBLINES)
                {
                    data->tokens[data->num_tokens++].type = WPS_TOKEN_SUBLINE_SEPARATOR;
                    subline = ++(data->num_sublines[data->num_lines]);
                    data->format_lines[data->num_lines][subline] = data->num_tokens;
                }
                else
                    wps_buffer += skip_end_of_line(wps_buffer);

                break;

            /* Conditional list start */
            case '<':
                data->tokens[data->num_tokens].type = WPS_TOKEN_CONDITIONAL_START;
                lastcond[level] = data->num_tokens++;
                break;

            /* Conditional list end */
            case '>':
                if (level < 0) /* not in a conditional, ignore the char */
                    break;

condlistend:  /* close a conditional. sometimes we want to close them even when
                 we don't have a closing token, e.g. at the end of a line. */

                data->tokens[data->num_tokens].type = WPS_TOKEN_CONDITIONAL_END;
                if (lastcond[level])
                    data->tokens[lastcond[level]].value.i = data->num_tokens;

                lastcond[level] = 0;
                data->num_tokens++;
                data->tokens[condindex[level]].value.i = numoptions[level];
                level--;
                break;

            /* Conditional list option */
            case '|':
                if (level < 0) /* not in a conditional, ignore the char */
                    break;

                data->tokens[data->num_tokens].type = WPS_TOKEN_CONDITIONAL_OPTION;
                if (lastcond[level])
                    data->tokens[lastcond[level]].value.i = data->num_tokens;

                lastcond[level] = data->num_tokens;
                numoptions[level]++;
                data->num_tokens++;
                break;

            /* Comment */
            case '#':
                wps_buffer += skip_end_of_line(wps_buffer);
                break;

            /* End of this line */
            case '\n':
                if (level >= 0)
                {
                    /* We have unclosed conditionals, so we
                       close them before adding the EOL token */
                    wps_buffer--;
                    goto condlistend;
                    break;
                }
                data->tokens[data->num_tokens++].type = WPS_TOKEN_EOL;
                (data->num_sublines[data->num_lines])++;
                data->num_lines++;

                if (data->num_lines < WPS_MAX_LINES)
                {
                    data->format_lines[data->num_lines][0] = data->num_tokens;
                }

                break;

            /* String */
            default:
                if (data->num_strings < WPS_MAX_STRINGS)
                {
                    data->tokens[data->num_tokens].type = WPS_TOKEN_STRING;
                    data->strings[data->num_strings] = current_string;
                    data->tokens[data->num_tokens].value.i = data->num_strings++;
                    data->num_tokens++;

                    /* Copy the first byte */
                    *current_string++ = *(wps_buffer - 1);

                    /* continue until we hit something that ends the string */
                    while(wps_buffer &&
                          *wps_buffer != '%' && //*wps_buffer != '#' &&
                          *wps_buffer != '<' && *wps_buffer != '>' &&
                          *wps_buffer != '|' && *wps_buffer != '\n')
                    {
                        *current_string++ = *wps_buffer++;
                    }

                    /* null terminate the string */
                    *current_string++ = '\0';
                }

                break;
        }
    }

#ifdef DEBUG
    /* debugging code */
    if (false)
    {
        dump_wps_tokens(data);
        print_line_info(data);
        print_wps_strings(data);
#ifdef HAVE_LCD_BITMAP
        print_img_cond_indexes(data);
#endif
    }
#endif

    return true;
}

#ifdef HAVE_LCD_BITMAP
/* Clear the WPS image cache */
static void wps_images_clear(struct wps_data *data)
{
    int i;
    /* set images to unloaded and not displayed */
    for (i = 0; i < MAX_IMAGES; i++)
    {
       data->img[i].loaded = false;
       data->img[i].display = false;
       data->img[i].always_display = false;
    }
    data->progressbar.have_bitmap_pb = false;
}
#endif

/* initial setup of wps_data */
void wps_data_init(struct wps_data *wps_data)
{
#ifdef HAVE_LCD_BITMAP
    wps_images_clear(wps_data);
    wps_data->wps_sb_tag = false;
    wps_data->show_sb_on_wps = false;
    wps_data->img_buf_ptr = wps_data->img_buf; /* where in image buffer */
    wps_data->img_buf_free = IMG_BUFSIZE; /* free space in image buffer */
    wps_data->peak_meter_enabled = false;
#else /* HAVE_LCD_CHARCELLS */
    int i;
    for(i = 0; i < 8; i++)
    {
        wps_data->wps_progress_pat[i] = 0;
    }
    wps_data->full_line_progressbar = false;
#endif
    wps_data->wps_loaded = false;
}

static void wps_reset(struct wps_data *data)
{
    memset(data, 0, sizeof(*data));
    data->wps_loaded = false;
    wps_data_init(data);
}

#ifdef HAVE_LCD_BITMAP


static void clear_bmp_names(void)
{
    int n;
    for (n = 0; n < MAX_IMAGES; n++)
    {
        bmp_names[n] = NULL;
    }
    pb_bmp_name = NULL;
#if LCD_DEPTH > 1
    backdrop_bmp_name = NULL;
#endif
}

static void load_wps_bitmaps(struct wps_data *wps_data, char *bmpdir)
{
    char img_path[MAX_PATH];

    int n;
    for (n = 0; n < MAX_IMAGES; n++)
    {
        if (bmp_names[n])
        {
            get_image_filename(bmp_names[n], bmpdir,
                               img_path, sizeof(img_path));

            /* load the image */
            wps_data->img[n].bm.data = wps_data->img_buf_ptr;
            if (load_bitmap(wps_data, img_path, &wps_data->img[n].bm))
            {
                wps_data->img[n].loaded = true;
            }
        }
    }

    if (pb_bmp_name)
    {
        get_image_filename(pb_bmp_name, bmpdir, img_path, sizeof(img_path));

        /* load the image */
        wps_data->progressbar.bm.data = wps_data->img_buf_ptr;
        if (load_bitmap(wps_data, img_path, &wps_data->progressbar.bm)
            && wps_data->progressbar.bm.width <= LCD_WIDTH)
        {
            wps_data->progressbar.have_bitmap_pb = true;
        }
    }

#if LCD_DEPTH > 1
    if (backdrop_bmp_name)
    {
        get_image_filename(backdrop_bmp_name, bmpdir,
                           img_path, sizeof(img_path));
        load_wps_backdrop(img_path);
    }
#endif
}

#endif /* HAVE_LCD_BITMAP */

/* to setup up the wps-data from a format-buffer (isfile = false)
   from a (wps-)file (isfile = true)*/
bool wps_data_load(struct wps_data *wps_data,
                   const char *buf,
                   bool isfile)
{
    if (!wps_data || !buf)
        return false;

    wps_reset(wps_data);

    if (!isfile)
    {
        return wps_parse(wps_data, buf);
    }
    else
    {
        /*
         * Hardcode loading WPS_DEFAULTCFG to cause a reset ideally this
         * wants to be a virtual file.  Feel free to modify dirbrowse()
         * if you're feeling brave.
         */
        if (! strcmp(buf, WPS_DEFAULTCFG) )
        {
            global_settings.wps_file[0] = 0;
            return false;
        }

#ifdef HAVE_REMOTE_LCD
        if (! strcmp(buf, RWPS_DEFAULTCFG) )
        {
            global_settings.rwps_file[0] = 0;
            return false;
        }
#endif

        int fd = open(buf, O_RDONLY);

        if (fd < 0)
            return false;

        /* get buffer space from the plugin buffer */
        unsigned int buffersize = 0;
        char *wps_buffer = (char *)plugin_get_buffer(&buffersize);

        if (!wps_buffer)
            return false;

        /* copy the file's content to the buffer for parsing */
        unsigned int start = 0;
        while(read_line(fd, wps_buffer + start, buffersize - start) > 0)
        {
            start += strlen(wps_buffer + start);
            if (start < buffersize - 1)
            {
                wps_buffer[start++] = '\n';
                wps_buffer[start] = 0;
            }
        }

        close(fd);

        if (start <= 0)
            return false;

#ifdef HAVE_LCD_BITMAP
        clear_bmp_names();
#endif

        /* parse the WPS source */
        if (!wps_parse(wps_data, wps_buffer))
            return false;

        wps_data->wps_loaded = true;

#ifdef HAVE_LCD_BITMAP
        /* get the bitmap dir */
        char bmpdir[MAX_PATH];
        size_t bmpdirlen;
        char *dot = strrchr(buf, '.');
        bmpdirlen = dot - buf;
        strncpy(bmpdir, buf, dot - buf);
        bmpdir[bmpdirlen] = 0;

        /* load the bitmaps that were found by the parsing */
        load_wps_bitmaps(wps_data, bmpdir);
#endif
        return true;
    }
}
