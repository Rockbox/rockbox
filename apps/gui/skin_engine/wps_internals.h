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
#include "tag_table.h"
#include "skin_parser.h"


/* TODO: sort this mess out */

#include "screen_access.h"
#include "statusbar.h"
#include "metadata.h"

/* alignments */
#define WPS_ALIGN_RIGHT 32
#define WPS_ALIGN_CENTER 64
#define WPS_ALIGN_LEFT 128


#define TOKEN_VALUE_ONLY 0x0DEADC0D

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
    struct bitmap bm;
    const char *label;
    bool loaded;            /* load state */
    bool always_display;    /* not using the preload/display mechanism */
    int display;
    bool using_preloaded_icons; /* using the icon system instead of a bmp */
};

struct image_display {
    const char *label;
    int subimage;
    struct wps_token *token; /* the token to get the subimage number from */
    int offset; /* offset into the bitmap strip to start */
};

struct progressbar {
    enum skin_token_type type;
    struct viewport *vp;
    /* regular pb */
    short x;
    /* >=0: explicitly set in the tag -> y-coord within the viewport
       <0 : not set in the tag -> negated 1-based line number within
            the viewport. y-coord will be computed based on the font height */
    short y;
    short width;
    short height;
    bool  follow_lang_direction;
    
    struct gui_img *image;
    
    bool invert_fill_direction;
    bool nofill;
    bool nobar;
    struct gui_img *slider;
    bool horizontal;
    struct gui_img *backdrop;
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

#define VP_DRAW_HIDEABLE    0x1
#define VP_DRAW_HIDDEN      0x2
#define VP_DRAW_WASHIDDEN   0x4
/* these are never drawn, nor cleared, i.e. just ignored */
#define VP_NEVER_VISIBLE    0x8
#define VP_DEFAULT_LABEL    "|"
struct skin_viewport {
    struct viewport vp;   /* The LCD viewport struct */
    char hidden_flags;
    bool is_infovp;
    char* label;
    unsigned start_fgcolour;
    unsigned start_bgcolour;
};
struct viewport_colour {
    struct viewport *vp;
    unsigned colour;
};
#ifdef HAVE_TOUCHSCREEN
struct touchregion {
    char* label;            /* label to identify this region */
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
    bool reverse_bar;        /* if true 0% is the left or top */
    bool repeat;             /* requires the area be held for the action */
    int action;              /* action this button will return */
    bool armed;              /* A region is armed on press. Only armed regions are triggered
                                on repeat or release. */
    union {                  /* Extra data, action dependant */
        void* data;
        int   value;
    };
    long last_press;        /* last tick this was pressed */
};

struct touchregion_lastpress {
    struct touchregion *region;
    long timeout;
};
#endif

struct playlistviewer {
    struct viewport *vp;
    bool show_icons;
    int start_offset;
    struct skin_element *line;
};


#ifdef HAVE_ALBUMART
struct skin_albumart {
    /* Album art support */
    int x;
    int y;
    int width;
    int height;

    unsigned char xalign; /* WPS_ALBUMART_ALIGN_LEFT, _CENTER, _RIGHT */
    unsigned char yalign; /* WPS_ALBUMART_ALIGN_TOP, _CENTER, _BOTTOM */
    unsigned char state; /* WPS_ALBUMART_NONE, _CHECK, _LOAD */
    
    struct viewport *vp;
    int draw_handle;
};
#endif
    

struct line {
    unsigned update_mode;
};

struct line_alternator {
    int current_line;
    unsigned long next_change_tick;
};

struct conditional {
    int last_value;
    struct wps_token *token;
};

struct logical_if {
    struct wps_token *token;
    enum {
        IF_EQUALS, /* == */
        IF_NOTEQUALS, /* != */
        IF_LESSTHAN, /* < */
        IF_LESSTHAN_EQ, /* <= */
        IF_GREATERTHAN, /* > */
        IF_GREATERTHAN_EQ /* >= */
    } op;
    struct skin_tag_parameter operand;
    int num_options;
};

/* wps_data
   this struct holds all necessary data which describes the
   viewable content of a wps */
struct wps_data
{
    struct skin_element *tree;
#ifdef HAVE_LCD_BITMAP
    struct skin_token_list *images;
#endif
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    struct {
        char *backdrop;
        int backdrop_id;
    };
#endif

#ifdef HAVE_TOUCHSCREEN
    struct skin_token_list *touchregions;
#endif
#ifdef HAVE_ALBUMART
    struct skin_albumart *albumart;
    int    playback_aa_slot;
#endif

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
    bool is_fading;
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
};

/* gui_wps end */

char *get_image_filename(const char *start, const char* bmpdir,
                                char *buf, int buf_size);
/***** wps_tokens.c ******/

const char *get_token_value(struct gui_wps *gwps,
                           struct wps_token *token, int offset,
                           char *buf, int buf_size,
                           int *intval);

/* Get the id3 fields from the cuesheet */
const char *get_cuesheetid3_token(struct wps_token *token, struct mp3entry *id3,
                                  int offset_tracks, char *buf, int buf_size);
const char *get_id3_token(struct wps_token *token, struct mp3entry *id3,
                          char *filename, char *buf, int buf_size, int limit, int *intval);
#if CONFIG_TUNER
const char *get_radio_token(struct wps_token *token, int preset_offset,
                            char *buf, int buf_size, int limit, int *intval);
#endif

struct gui_img* find_image(const char *label, struct wps_data *data);
struct skin_viewport* find_viewport(const char *label, bool uivp, struct wps_data *data);


#ifdef SIMULATOR
#define DEBUG_SKIN_ENGINE
extern bool debug_wps;
#endif

#endif
