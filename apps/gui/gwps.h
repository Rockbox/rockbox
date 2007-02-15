/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Jerome Kuptz
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _WPS_H
#define _WPS_H

#include "screen_access.h"
#include "statusbar.h"
#include "id3.h"
#include "playlist.h"

/* constants used in line_type and as refresh_mode for wps_refresh */
#define WPS_REFRESH_STATIC          1    /* line doesn't change over time */
#define WPS_REFRESH_DYNAMIC         2    /* line may change (e.g. time flag) */
#define WPS_REFRESH_SCROLL          4    /* line scrolls */
#define WPS_REFRESH_PLAYER_PROGRESS 8    /* line contains a progress bar */
#define WPS_REFRESH_PEAK_METER      16   /* line contains a peak meter */
#define WPS_REFRESH_ALL             0xff /* to refresh all line types */
/* to refresh only those lines that change over time */
#define WPS_REFRESH_NON_STATIC (WPS_REFRESH_ALL & ~WPS_REFRESH_STATIC & ~WPS_REFRESH_SCROLL)

/* alignments */
#define WPS_ALIGN_RIGHT 32
#define WPS_ALIGN_CENTER 64
#define WPS_ALIGN_LEFT 128

/* wps_data*/

#ifdef HAVE_LCD_BITMAP
struct gui_img{
    struct bitmap bm;
    int x;                  /* x-pos */
    int y;                  /* y-pos */
    bool loaded;            /* load state */
    bool display;           /* is to be displayed */
    bool always_display;    /* not using the preload/display mechanism */
};

struct prog_img{ /*progressbar image*/
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
#define IMG_BUFSIZE ((LCD_HEIGHT*LCD_WIDTH*LCD_DEPTH/8) \
                   + (2*LCD_HEIGHT*LCD_WIDTH/8))
#define WPS_MAX_LINES (LCD_HEIGHT/5+1)
#define FORMAT_BUFFER_SIZE 3072
#else
#define WPS_MAX_LINES 2
#define FORMAT_BUFFER_SIZE 400
#endif
#define WPS_MAX_SUBLINES 12
#define DEFAULT_SUBLINE_TIME_MULTIPLIER 20 /* (10ths of sec) */
#define BASE_SUBLINE_TIME 10 /* base time that multiplier is applied to
                                (1/HZ sec, or 100ths of sec) */
#define SUBLINE_RESET -1

/* wps_data
   this struct old all necessary data which describes the
   viewable content of a wps */
struct wps_data
{
#ifdef HAVE_LCD_BITMAP
    struct gui_img img[MAX_IMAGES];
    struct prog_img progressbar;
    unsigned char img_buf[IMG_BUFSIZE];
    unsigned char* img_buf_ptr;
    int img_buf_free;
    bool wps_sb_tag;
    bool show_sb_on_wps;
#endif
#ifdef HAVE_LCD_CHARCELLS
    unsigned char wps_progress_pat[8];
    bool full_line_progressbar;
#endif
    char format_buffer[FORMAT_BUFFER_SIZE];
    char* format_lines[WPS_MAX_LINES][WPS_MAX_SUBLINES];
    unsigned char line_type[WPS_MAX_LINES][WPS_MAX_SUBLINES];
    unsigned short time_mult[WPS_MAX_LINES][WPS_MAX_SUBLINES];
    long subline_expire_time[WPS_MAX_LINES];
    int curr_subline[WPS_MAX_LINES];
    int progress_top;
    int progress_height;
    int progress_start;
    int progress_end;
    bool wps_loaded;
    bool peak_meter_enabled;
};

/* initial setup of wps_data */
void wps_data_init(struct wps_data *wps_data);

/* to setup up the wps-data from a format-buffer (isfile = false)
   from a (wps-)file (isfile = true)*/
bool wps_data_load(struct wps_data *wps_data,
                   const char *buf,
                   bool isfile);

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
    char current_track_path[MAX_PATH+1];
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
   defines a wps with it's data, state,
   and the screen on which the wps-content should be drawn */
struct gui_wps
{
    struct screen * display;
    struct wps_data *data;
    struct wps_state *state;
    struct gui_statusbar *statusbar;
};

/* gui_wps end */

long gui_wps_show(void);

/* currently only on wps_state is needed */
extern struct wps_state wps_state;
extern struct gui_wps gui_wps[NB_SCREENS];

void gui_sync_wps_init(void);
void gui_sync_wps_screen_init(void);

#endif
