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
static int level = -1;

/* index of the last WPS_TOKEN_CONDITIONAL_OPTION
    or WPS_TOKEN_CONDITIONAL_START in current level */
static int lastcond[WPS_MAX_COND_LEVEL];

/* index of the WPS_TOKEN_CONDITIONAL in current level */
static int condindex[WPS_MAX_COND_LEVEL];

/* number of condtional options in current level */
static int numoptions[WPS_MAX_COND_LEVEL];

#ifdef HAVE_LCD_BITMAP
/* pointers to the bitmap filenames in the WPS source */
static const char *bmp_names[MAX_IMAGES];
static const char *pb_bmp_name;
#if LCD_DEPTH > 1
static const char *backdrop_bmp_name;
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
    enum wps_token_type type;
    const char name[3];
    unsigned char refresh_type;
    const wps_tag_parse_func parse_func;
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
    { WPS_TOKEN_RTC_DAY_OF_MONTH,              "d", 0, NULL },
    { WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED, "e", 0, NULL },
    { WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED,       "H", 0, NULL },
    { WPS_TOKEN_RTC_HOUR_24,                   "k", 0, NULL },
    { WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED,       "I", 0, NULL },
    { WPS_TOKEN_RTC_HOUR_12,                   "l", 0, NULL },
    { WPS_TOKEN_RTC_MONTH,                     "m", 0, NULL },
    { WPS_TOKEN_RTC_MINUTE,                    "M", 0, NULL },
    { WPS_TOKEN_RTC_SECOND,                    "S", 0, NULL },
    { WPS_TOKEN_RTC_YEAR_2_DIGITS,             "y", 0, NULL },
    { WPS_TOKEN_RTC_YEAR_4_DIGITS,             "Y", 0, NULL },
    { WPS_TOKEN_RTC_AM_PM_UPPER,               "p", 0, NULL },
    { WPS_TOKEN_RTC_AM_PM_LOWER,               "P", 0, NULL },
    { WPS_TOKEN_RTC_WEEKDAY_NAME,              "a", 0, NULL },
    { WPS_TOKEN_RTC_MONTH_NAME,                "b", 0, NULL },
    { WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON,     "u", 0, NULL },
    { WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN,     "w", 0, NULL },
    { WPS_TOKEN_CHARACTER,                     "\0",0, NULL }
    /* the array MUST end with a "\0" token */
};
#endif

/* array of available tags - those with more characters have to go first
   (e.g. "xl" and "xd" before "x"). It needs to end with the unknown token. */
static const struct wps_tag all_tags[] = {

    { WPS_TOKEN_ALIGN_CENTER,             "ac",  0,                   NULL },
    { WPS_TOKEN_ALIGN_LEFT,               "al",  0,                   NULL },
    { WPS_TOKEN_ALIGN_RIGHT,              "ar",  0,                   NULL },

    { WPS_TOKEN_BATTERY_PERCENT,          "bl",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_BATTERY_VOLTS,            "bv",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_BATTERY_TIME,             "bt",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_BATTERY_SLEEPTIME,        "bs",  WPS_REFRESH_DYNAMIC, NULL },
#if CONFIG_CHARGING >= CHARGING_MONITOR
    { WPS_TOKEN_BATTERY_CHARGING,         "bc",  WPS_REFRESH_DYNAMIC, NULL },
#endif
#if CONFIG_CHARGING
    { WPS_TOKEN_BATTERY_CHARGER_CONNECTED,"bp",  WPS_REFRESH_DYNAMIC, NULL },
#endif

#if CONFIG_RTC
    {  WPS_TOKEN_RTC,                     "c",   WPS_REFRESH_DYNAMIC, parse_rtc_format },
#endif

    /* current file */
    { WPS_TOKEN_FILE_BITRATE,             "fb",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_CODEC,               "fc",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_FREQUENCY,           "ff",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_NAME_WITH_EXTENSION, "fm",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_NAME,                "fn",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_PATH,                "fp",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_SIZE,                "fs",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_VBR,                 "fv",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_DIRECTORY,           "d",   WPS_REFRESH_STATIC, parse_dir_level },

    /* next file */
    { WPS_TOKEN_FILE_BITRATE,             "Fb",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_CODEC,               "Fc",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_FREQUENCY,           "Ff",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_NAME_WITH_EXTENSION, "Fm",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_NAME,                "Fn",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_PATH,                "Fp",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_SIZE,                "Fs",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_VBR,                 "Fv",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_DIRECTORY,           "D",   WPS_REFRESH_DYNAMIC, parse_dir_level },

    /* current metadata */
    { WPS_TOKEN_METADATA_ARTIST,          "ia",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_COMPOSER,        "ic",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_ALBUM,           "id",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_ALBUM_ARTIST,    "iA",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_GENRE,           "ig",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_TRACK_NUMBER,    "in",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_TRACK_TITLE,     "it",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_VERSION,         "iv",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_YEAR,            "iy",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_COMMENT,         "iC",  WPS_REFRESH_DYNAMIC, NULL },

    /* next metadata */
    { WPS_TOKEN_METADATA_ARTIST,          "Ia",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_COMPOSER,        "Ic",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_ALBUM,           "Id",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_ALBUM_ARTIST,    "IA",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_GENRE,           "Ig",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_TRACK_NUMBER,    "In",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_TRACK_TITLE,     "It",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_VERSION,         "Iv",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_YEAR,            "Iy",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_COMMENT,         "IC",  WPS_REFRESH_DYNAMIC, NULL },

#if (CONFIG_CODEC != MAS3507D)
    { WPS_TOKEN_SOUND_PITCH,              "Sp",  WPS_REFRESH_DYNAMIC, NULL },
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    { WPS_TOKEN_VLED_HDD,                 "lh",  WPS_REFRESH_DYNAMIC, NULL },
#endif

#ifdef HAS_BUTTON_HOLD
    { WPS_TOKEN_MAIN_HOLD,                "mh",  WPS_REFRESH_DYNAMIC, NULL },
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
    { WPS_TOKEN_REMOTE_HOLD,              "mr",  WPS_REFRESH_DYNAMIC, NULL },
#endif

    { WPS_TOKEN_REPEAT_MODE,              "mm",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_PLAYBACK_STATUS,          "mp",  WPS_REFRESH_DYNAMIC, NULL },

#ifdef HAVE_LCD_BITMAP
    { WPS_TOKEN_PEAKMETER,                "pm",  WPS_REFRESH_PEAK_METER, NULL },
#else
    { WPS_TOKEN_PLAYER_PROGRESSBAR,       "pf",  WPS_REFRESH_DYNAMIC | WPS_REFRESH_PLAYER_PROGRESS, parse_progressbar },
#endif
    { WPS_TOKEN_PROGRESSBAR,              "pb",  WPS_REFRESH_PLAYER_PROGRESS, parse_progressbar },

    { WPS_TOKEN_VOLUME,                   "pv",  WPS_REFRESH_DYNAMIC, NULL },

    { WPS_TOKEN_TRACK_TIME_ELAPSED,       "pc",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_TRACK_TIME_REMAINING,     "pr",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_TRACK_LENGTH,             "pt",  WPS_REFRESH_STATIC,  NULL },

    { WPS_TOKEN_PLAYLIST_POSITION,        "pp",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_PLAYLIST_ENTRIES,         "pe",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_PLAYLIST_NAME,            "pn",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_PLAYLIST_SHUFFLE,         "ps",  WPS_REFRESH_DYNAMIC, NULL },

    { WPS_TOKEN_DATABASE_PLAYCOUNT,       "rp",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_DATABASE_RATING,          "rr",  WPS_REFRESH_DYNAMIC, NULL },
#if CONFIG_CODEC == SWCODEC
    { WPS_TOKEN_REPLAYGAIN,               "rg",  WPS_REFRESH_STATIC,  NULL },
#endif

    { WPS_TOKEN_SCROLL,                   "s",   WPS_REFRESH_SCROLL,  NULL },
    { WPS_TOKEN_SUBLINE_TIMEOUT,          "t",   0,  parse_subline_timeout },

#ifdef HAVE_LCD_BITMAP
    { WPS_TOKEN_STATUSBAR_ENABLED,        "we",  0,        parse_statusbar },
    { WPS_TOKEN_STATUSBAR_DISABLED,       "wd",  0,        parse_statusbar },

    { WPS_NO_TOKEN,                       "xl",  0,       parse_image_load },

    { WPS_TOKEN_IMAGE_PRELOAD_DISPLAY,    "xd",  WPS_REFRESH_STATIC, parse_image_display },

    {  WPS_TOKEN_IMAGE_DISPLAY,           "x",   0,       parse_image_load },
    {  WPS_TOKEN_IMAGE_PROGRESS_BAR,      "P",   0,    parse_image_special },
#if LCD_DEPTH > 1
    {  WPS_TOKEN_IMAGE_BACKDROP,          "X",   0,    parse_image_special },
#endif
#endif

    { WPS_TOKEN_UNKNOWN,                  "\0",  0, NULL }
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
