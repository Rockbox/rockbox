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

#include "tag_table.h"
#include "skin_parser.h"
#ifndef __PCTOOL__
#include "core_alloc.h"
#endif

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

#define TOKEN_VALUE_ONLY 0x0DEADC0D

/* wps_data*/
struct wps_token {
    union {
        char c;
        unsigned short i;
        long l;
        OFFSETTYPE(void*) data;
    } value;

    enum skin_token_type type; /* enough to store the token type */
    /* Whether the tag (e.g. track name or the album) refers the
       current or the next song (false=current, true=next) */
    bool next;
};

char* get_dir(char* buf, int buf_size, const char* path, int level);


struct skin_token_list {
    OFFSETTYPE(struct wps_token *) token;
    OFFSETTYPE(struct skin_token_list *) next;
};

#ifdef HAVE_LCD_BITMAP
struct gui_img {
    OFFSETTYPE(struct viewport*) vp;    /* The viewport to display this image in */
    short int x;                  /* x-pos */
    short int y;                  /* y-pos */
    short int num_subimages;      /* number of sub-images */
    short int subimage_height;    /* height of each sub-image */
    struct bitmap bm;
    int buflib_handle;
    OFFSETTYPE(char*) label;
    bool loaded;            /* load state */
    int display;
    bool using_preloaded_icons; /* using the icon system instead of a bmp */
};

struct image_display {
    OFFSETTYPE(char*) label;
    int subimage;
    OFFSETTYPE(struct wps_token*) token; /* the token to get the subimage number from */
    int offset; /* offset into the bitmap strip to start */
};

struct progressbar {
    enum skin_token_type type;
    OFFSETTYPE(struct viewport *) vp;
    /* regular pb */
    short x;
    /* >=0: explicitly set in the tag -> y-coord within the viewport
       <0 : not set in the tag -> negated 1-based line number within
            the viewport. y-coord will be computed based on the font height */
    short y;
    short width;
    short height;
    bool  follow_lang_direction;
    
    OFFSETTYPE(struct gui_img *) image;
    
    bool invert_fill_direction;
    bool nofill;
    bool nobar;
    OFFSETTYPE(struct gui_img *) slider;
    bool horizontal;
    OFFSETTYPE(struct gui_img *) backdrop;
};

struct draw_rectangle {
    int x;
    int y;
    int width;
    int height;
    unsigned start_colour;
    unsigned end_colour;
};
#endif



struct align_pos {
    char* left;
    char* center;
    char* right;
};

#ifdef HAVE_LCD_BITMAP
#define WPS_MAX_TOKENS      1150
#else
#define WPS_MAX_TOKENS      64
#endif

enum wps_parse_error {
    PARSE_OK,
    PARSE_FAIL_UNCLOSED_COND,
    PARSE_FAIL_INVALID_CHAR,
    PARSE_FAIL_COND_SYNTAX_ERROR,
    PARSE_FAIL_COND_INVALID_PARAM,
    PARSE_FAIL_LIMITS_EXCEEDED,
};
#ifdef HAVE_LCD_COLOR
struct gradient_config {
    unsigned start;
    unsigned end;
    unsigned text;
    int lines_count;
};
#endif

#define VP_DRAW_HIDEABLE    0x1
#define VP_DRAW_HIDDEN      0x2
#define VP_DRAW_WASHIDDEN   0x4
/* these are never drawn, nor cleared, i.e. just ignored */
#define VP_NEVER_VISIBLE    0x8
#ifndef __PCTOOL__
#define VP_DEFAULT_LABEL    -200
#else
#define VP_DEFAULT_LABEL    NULL
#endif
#define VP_DEFAULT_LABEL_STRING "|"
struct skin_viewport {
    struct viewport vp;   /* The LCD viewport struct */
    char hidden_flags;
    bool is_infovp;
    OFFSETTYPE(char*) label;
    int   parsed_fontid;
#if LCD_DEPTH > 1
    bool output_to_backdrop_buffer;
    unsigned start_fgcolour;
    unsigned start_bgcolour;
#ifdef HAVE_LCD_COLOR
    struct gradient_config start_gradient;
#endif
#endif
};
struct viewport_colour {
    OFFSETTYPE(struct viewport *) vp;
    unsigned colour;
};

#ifdef HAVE_TOUCHSCREEN
struct touchregion {
    OFFSETTYPE(char*) label;            /* label to identify this region */
    OFFSETTYPE(struct skin_viewport*) wvp;/* The viewport this region is in */
    short int x;             /* x-pos */
    short int y;             /* y-pos */
    short int width;         /* width */
    short int height;        /* height */
    bool reverse_bar;        /* if true 0% is the left or top */
    bool allow_while_locked;
    enum {
        PRESS,               /* quick press only */
        LONG_PRESS,          /* Long press without repeat */
        REPEAT,              /* long press allowing repeats */
    } press_length;          
    int action;              /* action this button will return */
    bool armed;              /* A region is armed on press. Only armed regions are triggered
                                on repeat or release. */
    union {                  /* Extra data, action dependant */
        struct touchsetting {
            const struct settings_list *setting; /* setting being controlled */
            union {         /* Value to set the setting to for ACTION_SETTING_SET */
                int number;
                OFFSETTYPE(char*) text;
            } value;
        } setting_data;
        int   value;
    };
    long last_press;        /* last tick this was pressed */
};



struct touchregion_lastpress {
    OFFSETTYPE(struct touchregion *) region;
    long timeout;
};
#endif

struct playlistviewer {
    OFFSETTYPE(struct viewport *) vp;
    bool show_icons;
    int start_offset;
    OFFSETTYPE(struct skin_element *) line;
};


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

struct skin_albumart {
    /* Album art support */
    int x;
    int y;
    int width;
    int height;

    unsigned char xalign; /* WPS_ALBUMART_ALIGN_LEFT, _CENTER, _RIGHT */
    unsigned char yalign; /* WPS_ALBUMART_ALIGN_TOP, _CENTER, _BOTTOM */
    unsigned char state; /* WPS_ALBUMART_NONE, _CHECK, _LOAD */
    
    OFFSETTYPE(struct viewport *) vp;
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
    OFFSETTYPE(struct wps_token *) token;
};

struct logical_if {
    OFFSETTYPE(struct wps_token *) token;
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

struct substring {
    int start;
    int length;
    bool expect_number;
    OFFSETTYPE(struct wps_token *) token;
};

struct listitem {
    bool wrap;
    short offset;
};

#ifdef HAVE_SKIN_VARIABLES
struct skin_var {
    OFFSETTYPE(const char *) label;
    int value;
    long last_changed;
};
struct skin_var_lastchange {
    OFFSETTYPE(struct skin_var *) var;
    long timeout;
};
struct skin_var_changer {
    OFFSETTYPE(struct skin_var *) var;
    int newval;
    bool direct; /* true to make val=newval, false for val += newval */
    int max;
};
#endif

/* wps_data
   this struct holds all necessary data which describes the
   viewable content of a wps */
struct wps_data
{
    int buflib_handle;

    OFFSETTYPE(struct skin_element *) tree;
#ifdef HAVE_LCD_BITMAP
    OFFSETTYPE(struct skin_token_list *) images;
    OFFSETTYPE(int *) font_ids;
    int font_count;
#endif
#ifdef HAVE_BACKDROP_IMAGE
    int backdrop_id;
    bool use_extra_framebuffer;
#endif

#ifdef HAVE_TOUCHSCREEN
    OFFSETTYPE(struct skin_token_list *) touchregions;
    bool touchscreen_locked;
#endif
#ifdef HAVE_ALBUMART
    OFFSETTYPE(struct skin_albumart *) albumart;
    int    playback_aa_slot;
#endif

#ifdef HAVE_SKIN_VARIABLES
    OFFSETTYPE(struct skin_token_list *) skinvars;
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

#ifndef __PCTOOL__
static inline char* get_skin_buffer(struct wps_data* data)
{
    if (data->buflib_handle >= 0)
        return core_get_data(data->buflib_handle);
    return NULL;
}
#else
#define get_skin_buffer(d) skin_buffer
#endif

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

void get_image_filename(const char *start, const char* bmpdir,
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

enum skin_find_what {
    SKIN_FIND_VP = 0,
    SKIN_FIND_UIVP,
#ifdef HAVE_LCD_BITMAP
    SKIN_FIND_IMAGE,
#endif
#ifdef HAVE_TOUCHSCREEN
    SKIN_FIND_TOUCHREGION,
#endif
#ifdef HAVE_SKIN_VARIABLES
    SKIN_VARIABLE,
#endif
};
void *skin_find_item(const char *label, enum skin_find_what what,
                     struct wps_data *data);
#ifdef SIMULATOR
#define DEBUG_SKIN_ENGINE
extern bool debug_wps;
#endif

#endif
