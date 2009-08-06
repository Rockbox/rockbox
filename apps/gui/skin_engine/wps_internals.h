/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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

 /* This stuff is for the wps engine only.. anyone caught using this outside
  * of apps/gui/wps_engine will be shot on site! */
 
#ifndef _WPS_ENGINE_INTERNALS_
#define _WPS_ENGINE_INTERNALS_
/* Timeout unit expressed in HZ. In WPS, all timeouts are given in seconds
   (possibly with a decimal fraction) but stored as integer values.
   E.g. 2.5 is stored as 25. This means 25 tenth of a second, i.e. 25 units.
*/
#define TIMEOUT_UNIT (HZ/10) /* I.e. 0.1 sec */
#define DEFAULT_SUBLINE_TIME_MULTIPLIER 20 /* In TIMEOUT_UNIT's */



                           
/* TODO: sort this mess out */

#include "screen_access.h"
#include "statusbar.h"
#include "metadata.h"

/* constants used in line_type and as refresh_mode for wps_refresh */
#define WPS_REFRESH_STATIC          (1u<<0)  /* line doesn't change over time */
#define WPS_REFRESH_DYNAMIC         (1u<<1)  /* line may change (e.g. time flag) */
#define WPS_REFRESH_SCROLL          (1u<<2)  /* line scrolls */
#define WPS_REFRESH_PLAYER_PROGRESS (1u<<3)  /* line contains a progress bar */
#define WPS_REFRESH_PEAK_METER      (1u<<4)  /* line contains a peak meter */
#define WPS_REFRESH_STATUSBAR       (1u<<5)  /* refresh statusbar */
#define WPS_REFRESH_ALL       (0xffffffffu)   /* to refresh all line types */

/* to refresh only those lines that change over time */
#define WPS_REFRESH_NON_STATIC (WPS_REFRESH_DYNAMIC| \
                                WPS_REFRESH_PLAYER_PROGRESS| \
                                WPS_REFRESH_PEAK_METER)
/* alignments */
#define WPS_ALIGN_RIGHT 32
#define WPS_ALIGN_CENTER 64
#define WPS_ALIGN_LEFT 128

#ifdef HAVE_ALBUMART

/* albumart definitions */
#define WPS_ALBUMART_NONE           0      /* WPS does not contain AA tag */
#define WPS_ALBUMART_CHECK          1      /* WPS contains AA conditional tag */
#define WPS_ALBUMART_LOAD           2      /* WPS contains AA tag */

#define WPS_ALBUMART_ALIGN_RIGHT    1    /* x align:   right */
#define WPS_ALBUMART_ALIGN_CENTER   2    /* x/y align: center */
#define WPS_ALBUMART_ALIGN_LEFT     4    /* x align:   left */
#define WPS_ALBUMART_ALIGN_TOP      1    /* y align:   top */
#define WPS_ALBUMART_ALIGN_BOTTOM   4    /* y align:   bottom */

#endif /* HAVE_ALBUMART */

/* wps_data*/

#ifdef HAVE_LCD_BITMAP
struct gui_img {
    struct bitmap bm;
    struct viewport* vp;    /* The viewport to display this image in */
    short int x;                  /* x-pos */
    short int y;                  /* y-pos */
    short int num_subimages;      /* number of sub-images */
    short int subimage_height;    /* height of each sub-image */
    short int display;            /* -1 for no display, 0..n to display a subimage */
    bool loaded;            /* load state */
    bool always_display;    /* not using the preload/display mechanism */
};

struct progressbar {
    /* regular pb */
    short x;
    /* >=0: explicitly set in the tag -> y-coord within the viewport
       <0 : not set in the tag -> negated 1-based line number within
            the viewport. y-coord will be computed based on the font height */
    short y;
    short width;
    short height;
    /*progressbar image*/
    struct bitmap bm;
    bool have_bitmap_pb;
};
#endif



struct align_pos {
    char* left;
    char* center;
    char* right;
};

#ifdef HAVE_LCD_BITMAP

#define MAX_IMAGES (26*2) /* a-z and A-Z */
#define MAX_PROGRESSBARS 3

/* The image buffer is big enough to store one full-screen native bitmap,
   plus two full-screen mono bitmaps. */

#define IMG_BUFSIZE ((LCD_HEIGHT*LCD_WIDTH*LCD_DEPTH/8) \
                   + (2*LCD_HEIGHT*LCD_WIDTH/8))

#define WPS_MAX_VIEWPORTS   24
#define WPS_MAX_LINES       ((LCD_HEIGHT/5+1) * 2)
#define WPS_MAX_SUBLINES    (WPS_MAX_LINES*3)
#define WPS_MAX_TOKENS      1024
#define WPS_MAX_STRINGS     128
#define STRING_BUFFER_SIZE  1024
#define WPS_MAX_COND_LEVEL  10

#else

#define WPS_MAX_VIEWPORTS   2
#define WPS_MAX_LINES       2
#define WPS_MAX_SUBLINES    12
#define WPS_MAX_TOKENS      64
#define WPS_MAX_STRINGS     32
#define STRING_BUFFER_SIZE  64
#define WPS_MAX_COND_LEVEL  5

#endif

#define SUBLINE_RESET -1

enum wps_parse_error {
    PARSE_OK,
    PARSE_FAIL_UNCLOSED_COND,
    PARSE_FAIL_INVALID_CHAR,
    PARSE_FAIL_COND_SYNTAX_ERROR,
    PARSE_FAIL_COND_INVALID_PARAM,
    PARSE_FAIL_LIMITS_EXCEEDED,
};

enum wps_token_type {
    WPS_NO_TOKEN,   /* for WPS tags we don't want to save as tokens */
    WPS_TOKEN_UNKNOWN,

    /* Markers */
    WPS_TOKEN_CHARACTER,
    WPS_TOKEN_STRING,

    /* Alignment */
    WPS_TOKEN_ALIGN_LEFT,
    WPS_TOKEN_ALIGN_CENTER,
    WPS_TOKEN_ALIGN_RIGHT,

    /* Sublines */
    WPS_TOKEN_SUBLINE_TIMEOUT,

    /* Battery */
    WPS_TOKEN_BATTERY_PERCENT,
    WPS_TOKEN_BATTERY_VOLTS,
    WPS_TOKEN_BATTERY_TIME,
    WPS_TOKEN_BATTERY_CHARGER_CONNECTED,
    WPS_TOKEN_BATTERY_CHARGING,
    WPS_TOKEN_BATTERY_SLEEPTIME,

    /* Sound */
#if (CONFIG_CODEC != MAS3507D)
    WPS_TOKEN_SOUND_PITCH,
#endif
#if (CONFIG_CODEC == SWCODEC)
    WPS_TOKEN_REPLAYGAIN,
    WPS_TOKEN_CROSSFADE,
#endif

    /* Time */

    WPS_TOKEN_RTC_PRESENT,

    /* The begin/end values allow us to know if a token is an RTC one.
       New RTC tokens should be added between the markers. */

    WPS_TOKENS_RTC_BEGIN, /* just the start marker, not an actual token */

    WPS_TOKEN_RTC_DAY_OF_MONTH,
    WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED,
    WPS_TOKEN_RTC_12HOUR_CFG,
    WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED,
    WPS_TOKEN_RTC_HOUR_24,
    WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED,
    WPS_TOKEN_RTC_HOUR_12,
    WPS_TOKEN_RTC_MONTH,
    WPS_TOKEN_RTC_MINUTE,
    WPS_TOKEN_RTC_SECOND,
    WPS_TOKEN_RTC_YEAR_2_DIGITS,
    WPS_TOKEN_RTC_YEAR_4_DIGITS,
    WPS_TOKEN_RTC_AM_PM_UPPER,
    WPS_TOKEN_RTC_AM_PM_LOWER,
    WPS_TOKEN_RTC_WEEKDAY_NAME,
    WPS_TOKEN_RTC_MONTH_NAME,
    WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON,
    WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN,

    WPS_TOKENS_RTC_END,     /* just the end marker, not an actual token */

    /* Conditional */
    WPS_TOKEN_CONDITIONAL,
    WPS_TOKEN_CONDITIONAL_START,
    WPS_TOKEN_CONDITIONAL_OPTION,
    WPS_TOKEN_CONDITIONAL_END,

    /* Database */
#ifdef HAVE_TAGCACHE
    WPS_TOKEN_DATABASE_PLAYCOUNT,
    WPS_TOKEN_DATABASE_RATING,
    WPS_TOKEN_DATABASE_AUTOSCORE,
#endif

    /* File */
    WPS_TOKEN_FILE_BITRATE,
    WPS_TOKEN_FILE_CODEC,
    WPS_TOKEN_FILE_FREQUENCY,
    WPS_TOKEN_FILE_FREQUENCY_KHZ,
    WPS_TOKEN_FILE_NAME,
    WPS_TOKEN_FILE_NAME_WITH_EXTENSION,
    WPS_TOKEN_FILE_PATH,
    WPS_TOKEN_FILE_SIZE,
    WPS_TOKEN_FILE_VBR,
    WPS_TOKEN_FILE_DIRECTORY,

#ifdef HAVE_LCD_BITMAP
    /* Image */
    WPS_TOKEN_IMAGE_BACKDROP,
    WPS_TOKEN_IMAGE_PROGRESS_BAR,
    WPS_TOKEN_IMAGE_PRELOAD,
    WPS_TOKEN_IMAGE_PRELOAD_DISPLAY,
    WPS_TOKEN_IMAGE_DISPLAY,
#endif

#ifdef HAVE_ALBUMART
    /* Albumart */
    WPS_TOKEN_ALBUMART_DISPLAY,
    WPS_TOKEN_ALBUMART_FOUND,
#endif

    /* Metadata */
    WPS_TOKEN_METADATA_ARTIST,
    WPS_TOKEN_METADATA_COMPOSER,
    WPS_TOKEN_METADATA_ALBUM_ARTIST,
    WPS_TOKEN_METADATA_GROUPING,
    WPS_TOKEN_METADATA_ALBUM,
    WPS_TOKEN_METADATA_GENRE,
    WPS_TOKEN_METADATA_DISC_NUMBER,
    WPS_TOKEN_METADATA_TRACK_NUMBER,
    WPS_TOKEN_METADATA_TRACK_TITLE,
    WPS_TOKEN_METADATA_VERSION,
    WPS_TOKEN_METADATA_YEAR,
    WPS_TOKEN_METADATA_COMMENT,

    /* Mode */
    WPS_TOKEN_REPEAT_MODE,
    WPS_TOKEN_PLAYBACK_STATUS,

    WPS_TOKEN_MAIN_HOLD,

#ifdef HAS_REMOTE_BUTTON_HOLD
    WPS_TOKEN_REMOTE_HOLD,
#endif

    /* Progressbar */
    WPS_TOKEN_PROGRESSBAR,
#ifdef HAVE_LCD_CHARCELLS
    WPS_TOKEN_PLAYER_PROGRESSBAR,
#endif

#ifdef HAVE_LCD_BITMAP
    /* Peakmeter */
    WPS_TOKEN_PEAKMETER,
#endif

    /* Volume level */
    WPS_TOKEN_VOLUME,

    /* Current track */
    WPS_TOKEN_TRACK_ELAPSED_PERCENT,
    WPS_TOKEN_TRACK_TIME_ELAPSED,
    WPS_TOKEN_TRACK_TIME_REMAINING,
    WPS_TOKEN_TRACK_LENGTH,

    /* Playlist */
    WPS_TOKEN_PLAYLIST_ENTRIES,
    WPS_TOKEN_PLAYLIST_NAME,
    WPS_TOKEN_PLAYLIST_POSITION,
    WPS_TOKEN_PLAYLIST_SHUFFLE,

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    /* Virtual LED */
    WPS_TOKEN_VLED_HDD,
#endif

    /* Viewport display */
    WPS_VIEWPORT_ENABLE,

    /* buttons */
    WPS_TOKEN_BUTTON_VOLUME,
    WPS_TOKEN_LASTTOUCH,

    /* Setting option */
    WPS_TOKEN_SETTING,
};

struct wps_token {
    unsigned char type; /* enough to store the token type */

    /* Whether the tag (e.g. track name or the album) refers the
       current or the next song (false=current, true=next) */
    bool next;

    union {
        char c;
        unsigned short i;
    } value;
};

/* Description of a subline on the WPS */
struct wps_subline {

    /* Index of the first token for this subline in the token array.
       Tokens of this subline end where tokens for the next subline
       begin. */
    unsigned short first_token_idx;

    /* Bit or'ed WPS_REFRESH_xxx */
    unsigned char line_type;

    /* How long the subline should be displayed, in 10ths of sec */
    unsigned char time_mult;
};

/* Description of a line on the WPS. A line is a set of sublines.
   A subline is displayed for a certain amount of time. After that,
   the next subline of the line is displayed. And so on. */
struct wps_line {

    /* Number of sublines in this line */
    signed char num_sublines;

    /* Number (0-based) of the subline within this line currently being displayed */
    signed char curr_subline;

    /* Index of the first subline of this line in the subline array.
       Sublines for this line end where sublines for the next line begin. */
    unsigned short first_subline_idx;

    /* When the next subline of this line should be displayed
       (absolute time value in ticks) */
    long subline_expire_time;
};

#define VP_DRAW_HIDEABLE 0x1
#define VP_DRAW_HIDDEN   0x2
#define VP_DRAW_WASHIDDEN  0x4
struct wps_viewport {
    struct viewport vp;   /* The LCD viewport struct */
    struct progressbar *pb;
    /* Indexes of the first and last lines belonging to this viewport in the 
       lines[] array */
    int first_line, last_line;
    char hidden_flags;
    char label;
};

#ifdef HAVE_TOUCHSCREEN
struct touchregion {
    struct wps_viewport* wvp;/* The viewport this region is in */
    short int x;             /* x-pos */
    short int y;             /* y-pos */
    short int width;         /* width */
    short int height;        /* height */
    bool repeat;             /* requires the area be held for the action */
    int action;              /* action this button will return */
};
#define MAX_TOUCHREGIONS    15
#endif
/* wps_data
   this struct holds all necessary data which describes the
   viewable content of a wps */
struct wps_data
{
#ifdef HAVE_LCD_BITMAP
    struct gui_img img[MAX_IMAGES];
    unsigned char img_buf[IMG_BUFSIZE];
    unsigned char* img_buf_ptr;
    int img_buf_free;
    bool wps_sb_tag;
    bool show_sb_on_wps;

    struct progressbar progressbar[MAX_PROGRESSBARS];
    short progressbar_count;
    
    bool peak_meter_enabled;

#ifdef HAVE_ALBUMART
    /* Album art support */
    unsigned char wps_uses_albumart; /* WPS_ALBUMART_NONE, _CHECK, _LOAD */
    short albumart_x;
    short albumart_y;
    unsigned char albumart_xalign; /* WPS_ALBUMART_ALIGN_LEFT, _CENTER, _RIGHT */
    unsigned char albumart_yalign; /* WPS_ALBUMART_ALIGN_TOP, _CENTER, _BOTTOM */
    short albumart_max_width;
    short albumart_max_height;

    int albumart_cond_index;
#endif

#else /*HAVE_LCD_CHARCELLS */
    unsigned short wps_progress_pat[8];
    bool full_line_progressbar;
#endif

#ifdef HAVE_TOUCHSCREEN
    struct touchregion touchregion[MAX_TOUCHREGIONS];
    short touchregion_count;
#endif
    
#ifdef HAVE_REMOTE_LCD
    bool remote_wps;
#endif

    /* Number of lines in the WPS. During WPS parsing, this is
       the index of the line being parsed. */
    int num_lines;

    /* Number of viewports in the WPS */
    int num_viewports;
    struct wps_viewport viewports[WPS_MAX_VIEWPORTS];

    struct wps_line lines[WPS_MAX_LINES];

    /* Total number of sublines in the WPS. During WPS parsing, this is
       the index of the subline where the parsed tokens are added to. */
    int num_sublines;
    struct wps_subline sublines[WPS_MAX_SUBLINES];

    /* Total number of tokens in the WPS. During WPS parsing, this is
       the index of the token being parsed. */
    int num_tokens;
    struct wps_token tokens[WPS_MAX_TOKENS];

    char string_buffer[STRING_BUFFER_SIZE];
    char *strings[WPS_MAX_STRINGS];
    int num_strings;

    bool wps_loaded;
    
    /* tick the volume button was last pressed */
    unsigned int button_time_volume;
};


/* Redraw statusbars if necessary */
void gwps_draw_statusbars(void);

/* Returns the index of the last subline's token in the token array.
   line - 0-based line number
   subline - 0-based subline number within the line
 */
int skin_last_token_index(struct wps_data *data, int line, int subline);

/* wps_data end */

/* wps_state
   holds the data which belongs to the current played track,
   the track which will be played afterwards, current path to the track
   and some status infos */
struct wps_state
{
    bool ff_rewind;
    bool paused;
    int ff_rewind_count;
    bool wps_time_countup;
    struct mp3entry* id3;
    struct mp3entry* nid3;
    bool do_full_update;
};


/* change the ff/rew-status
   if ff_rew = true then we are in skipping mode
   else we are in normal mode */
/* void wps_state_update_ff_rew(bool ff_rew); Currently unused */

/* change the tag-information of the current played track
   and the following track */
/* void wps_state_update_id3_nid3(struct mp3entry *id3, struct mp3entry *nid3); Currently unused */
/* wps_state end*/

/* gui_wps
   defines a wps with its data, state,
   and the screen on which the wps-content should be drawn */
struct gui_wps
{
    struct screen *display;
    struct wps_data *data;
    struct wps_state *state;
};

/* gui_wps end */


/***** wps_tokens.c ******/

const char *get_token_value(struct gui_wps *gwps,
                           struct wps_token *token,
                           char *buf, int buf_size,
                           int *intval);

#endif
