/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin, Jonathan Gordon
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

#ifndef _CUESHEET_H_
#define _CUESHEET_H_

#include <stdbool.h>
#include "screens.h"
#include "file.h"
#include "metadata.h"

#define MAX_NAME 80    /* Max length of information strings */
#define MAX_TRACKS 99  /* Max number of tracks in a cuesheet */

struct cue_track_info {
    char title[MAX_NAME*3+1];
    char performer[MAX_NAME*3+1];
    char songwriter[MAX_NAME*3+1];
    unsigned long offset; /* ms from start of track */
};

struct cuesheet {
    char path[MAX_PATH];
    char audio_filename[MAX_PATH];

    char title[MAX_NAME*3+1];
    char performer[MAX_NAME*3+1];
    char songwriter[MAX_NAME*3+1];

    int track_count;
    struct cue_track_info tracks[MAX_TRACKS];

    int curr_track_idx;
    struct cue_track_info *curr_track;
};

extern struct cuesheet *curr_cue;
extern struct cuesheet *temp_cue;

/* returns true if cuesheet support is initialised */
bool cuesheet_is_enabled(void);

/* allocates the cuesheet buffer */
void cuesheet_init(void);

/* looks if there is a cuesheet file that has a name matching "trackpath" */
bool look_for_cuesheet_file(const char *trackpath, char *found_cue_path);

/* parse cuesheet "file" and store the information in "cue" */
bool parse_cuesheet(char *file, struct cuesheet *cue);

/* reads a cuesheet to find the audio track associated to it */
bool get_trackname_from_cuesheet(char *filename, char *buf);

/* display a cuesheet struct */
void browse_cuesheet(struct cuesheet *cue);

/* display a cuesheet file after parsing and loading it to the plugin buffer */
bool display_cuesheet_content(char* filename);

/* finds the index of the current track played within a cuesheet */
int cue_find_current_track(struct cuesheet *cue, unsigned long curpos);

/* update the id3 info to that of the currently playing track in the cuesheet */
void cue_spoof_id3(struct cuesheet *cue, struct mp3entry *id3);

/* skip to next track in the cuesheet towards "direction" (which is 1 or -1) */
bool curr_cuesheet_skip(int direction, unsigned long curr_pos);

#ifdef HAVE_LCD_BITMAP
/* draw track markers on the progressbar */
void cue_draw_markers(struct screen *screen, unsigned long tracklen,
                      int x1, int x2, int y, int h);
#endif

#endif
