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

#include "skin_tokens.h"

                           
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
    struct viewport* vp;    /* The viewport to display this image in */
    short int x;                  /* x-pos */
    short int y;                  /* y-pos */
    short int num_subimages;      /* number of sub-images */
    short int subimage_height;    /* height of each sub-image */
    short int display;            /* -1 for no display, 0..n to display a subimage */
    struct bitmap bm;
    char label;
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
    bool  follow_lang_direction;
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


/* Description of a subline on the WPS */
struct skin_subline {

    /* Index of the first token for this subline in the token array.
       Tokens of this subline end where tokens for the next subline
       begin. */
    unsigned short first_token_idx;
    unsigned short last_token_idx;

    /* Bit or'ed WPS_REFRESH_xxx */
    unsigned char line_type;

    /* How long the subline should be displayed, in 10ths of sec */
    unsigned char time_mult;
    
    /* pointer to the next subline in this line */
    struct skin_subline *next;
};

/* Description of a line on the WPS. A line is a set of sublines.
   A subline is displayed for a certain amount of time. After that,
   the next subline of the line is displayed. And so on. */
struct skin_line {

    /* Linked list of all the sublines on this line,
     * a line *must* have at least one subline so no need to add an extra pointer */
    struct skin_subline sublines;
    /* pointer to the current subline */
    struct skin_subline *curr_subline;

    /* When the next subline of this line should be displayed
       (absolute time value in ticks) */
    long subline_expire_time;
    
    /* pointer to the next line */
    struct skin_line *next;
};

#define VP_DRAW_HIDEABLE    0x1
#define VP_DRAW_HIDDEN      0x2
#define VP_DRAW_WASHIDDEN   0x4
/* these are never drawn, nor cleared, i.e. just ignored */
#define VP_NEVER_VISIBLE    0x8
#define VP_DEFAULT_LABEL    '|'
#define VP_NO_LABEL         '-'
#define VP_INFO_LABEL       '_'
struct skin_viewport {
    struct viewport vp;   /* The LCD viewport struct */
    struct progressbar *pb;
    struct skin_line *lines;
    char hidden_flags;
    char label;
};

#ifdef HAVE_TOUCHSCREEN
struct touchregion {
    struct skin_viewport* wvp;/* The viewport this region is in */
    short int x;             /* x-pos */
    short int y;             /* y-pos */
    short int width;         /* width */
    short int height;        /* height */
    enum {
        WPS_TOUCHREGION_ACTION,
        WPS_TOUCHREGION_SCROLLBAR,
        WPS_TOUCHREGION_VOLUME
    } type;                  /* type of touch region */
    bool repeat;             /* requires the area be held for the action */
    int action;              /* action this button will return */
    bool armed;              /* A region is armed on press. Only armed regions are triggered
                                on repeat or release. */
};
#endif

#define MAX_PLAYLISTLINE_TOKENS 16
#define MAX_PLAYLISTLINE_STRINGS    8
#define MAX_PLAYLISTLINE_STRLEN     8
enum info_line_type {
    TRACK_HAS_INFO = 0,
    TRACK_HAS_NO_INFO
};
struct playlistviewer {
    struct viewport *vp;
    bool show_icons;
    int start_offset;
    struct {
        enum wps_token_type tokens[MAX_PLAYLISTLINE_TOKENS];
        char strings[MAX_PLAYLISTLINE_STRINGS][MAX_PLAYLISTLINE_STRLEN];
        int count;
        bool scroll;
    } lines[2];
};
    


#ifdef HAVE_ALBUMART
struct skin_albumart {
    /* Album art support */
    struct viewport *vp;/* The viewport this is in */
    int x;
    int y;
    int width;
    int height;

    bool draw;
    unsigned char xalign; /* WPS_ALBUMART_ALIGN_LEFT, _CENTER, _RIGHT */
    unsigned char yalign; /* WPS_ALBUMART_ALIGN_TOP, _CENTER, _BOTTOM */
    unsigned char state; /* WPS_ALBUMART_NONE, _CHECK, _LOAD */
};
#endif

/* wps_data
   this struct holds all necessary data which describes the
   viewable content of a wps */
struct wps_data
{
#ifdef HAVE_LCD_BITMAP
    struct skin_token_list *images;
    struct skin_token_list *progressbars;
#endif
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    char *backdrop;
#endif

#ifdef HAVE_TOUCHSCREEN
    struct skin_token_list *touchregions;
#endif
    struct skin_token_list *viewports;
    struct skin_token_list *strings;
#ifdef HAVE_ALBUMART
    struct skin_albumart *albumart;
    int    playback_aa_slot;
#endif
    struct wps_token *tokens;
    /* Total number of tokens in the WPS. During WPS parsing, this is
       the index of the token being parsed. */
    int num_tokens;

#ifdef HAVE_LCD_BITMAP
    bool peak_meter_enabled;
    bool wps_sb_tag;
    bool show_sb_on_wps;
#else /*HAVE_LCD_CHARCELLS */
    unsigned short wps_progress_pat[8];
    bool full_line_progressbar;
#endif
    bool wps_loaded;
};

/* wps_data end */

/* wps_state
   holds the data which belongs to the current played track,
   the track which will be played afterwards, current path to the track
   and some status infos */
struct wps_state
{
    struct mp3entry* id3;
    struct mp3entry* nid3;
    int  ff_rewind_count;
    bool ff_rewind;
    bool paused;
    bool wps_time_countup;
    bool is_fading;
};

/* Holds data for all screens in a skin. */
struct wps_sync_data
{
    /* suitable for the viewportmanager, possibly only temporary here
     * needs to be same for all screens! can't be split up for screens
     * due to what viewportmanager_set_statusbar() accepts
     * (FIXME?) */
    int statusbars;
    /* indicates whether the skin needs a full update for all screens */
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
    /* must point to the same struct for all screens */
    struct wps_sync_data *sync_data;
};

/* gui_wps end */

char *get_image_filename(const char *start, const char* bmpdir,
                                char *buf, int buf_size);
/***** wps_tokens.c ******/

const char *get_token_value(struct gui_wps *gwps,
                           struct wps_token *token,
                           char *buf, int buf_size,
                           int *intval);

const char *get_id3_token(struct wps_token *token, struct mp3entry *id3,
                          char *buf, int buf_size, int limit, int *intval);


struct gui_img* find_image(char label, struct wps_data *data);
struct skin_viewport* find_viewport(char label, struct wps_data *data);

#endif
