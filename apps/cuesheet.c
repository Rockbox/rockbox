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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "system.h"
#include "audio.h"
#include "kernel.h"
#include "logf.h"
#include "sprintf.h"
#include "misc.h"
#include "screens.h"
#include "splash.h"
#include "list.h"
#include "action.h"
#include "lang.h"
#include "debug.h"
#include "settings.h"
#include "buffer.h"
#include "plugin.h"
#include "playback.h"
#include "cuesheet.h"

#define CUE_DIR ROCKBOX_DIR "/cue"

struct cuesheet *curr_cue;
struct cuesheet *temp_cue;

#if CONFIG_CODEC != SWCODEC
/* special trickery because the hwcodec playback engine is in firmware/ */
static bool cuesheet_handler(const char *filename)
{
    return cuesheet_is_enabled() && look_for_cuesheet_file(filename, NULL);
}
#endif

void cuesheet_init(void)
{
    if (global_settings.cuesheet) {
        curr_cue = (struct cuesheet *)buffer_alloc(sizeof(struct cuesheet));
        temp_cue = (struct cuesheet *)buffer_alloc(sizeof(struct cuesheet));
#if CONFIG_CODEC != SWCODEC
        audio_set_cuesheet_callback(cuesheet_handler);
#endif
    } else {
        curr_cue = NULL;
        temp_cue = NULL;
    }
}

bool cuesheet_is_enabled(void)
{
    return (curr_cue != NULL);
}

bool look_for_cuesheet_file(const char *trackpath, char *found_cue_path)
{
    /* DEBUGF("look for cue file\n"); */

    char cuepath[MAX_PATH];
    char *dot, *slash;

    slash = strrchr(trackpath, '/');
    if (!slash)
    {
        found_cue_path = NULL;
        return false;
    }

    strncpy(cuepath, trackpath, MAX_PATH);
    dot = strrchr(cuepath, '.');
    strcpy(dot, ".cue");

    if (!file_exists(cuepath))
    {
        strcpy(cuepath, CUE_DIR);
        strcat(cuepath, slash);
        char *dot = strrchr(cuepath, '.');
        strcpy(dot, ".cue");
        if (!file_exists(cuepath))
        {
            if (found_cue_path)
                found_cue_path = NULL;
            return false;
        }
    }

    if (found_cue_path)
        strncpy(found_cue_path, cuepath, MAX_PATH);
    return true;
}

static char *skip_whitespace(char* buf)
{
    char *r = buf;
    while (*r && isspace(*r))
        r++;
    return r;
}


static char *get_string(const char *line)
{
    char *start, *end;

    start = strchr(line, '"');
    if (!start)
    {
        start = strchr(line, ' ');

        if (!start)
            return NULL;
    }

    end = strchr(++start, '"');
    if (end)
        *end = '\0';

    return start;
}

/* parse cuesheet "file" and store the information in "cue" */
bool parse_cuesheet(char *file, struct cuesheet *cue)
{
    char line[MAX_PATH];
    char *s;

    DEBUGF("cue parse\n");
    int fd = open_utf8(file,O_RDONLY);
    if (fd < 0)
    {
        /* couln't open the file */
        return false;
    }

    /* Initialization */
    memset(cue, 0, sizeof(struct cuesheet));
    strcpy(cue->path, file);
    cue->curr_track = cue->tracks;

    while ( read_line(fd,line,MAX_PATH) && cue->track_count < MAX_TRACKS )
    {
        s = skip_whitespace(line);

        if (!strncmp(s, "TRACK", 5))
        {
            cue->track_count++;
        }
        else if (!strncmp(s, "INDEX 01", 8))
        {
            s = strchr(s,' ');
            s = skip_whitespace(s);
            s = strchr(s,' ');
            s = skip_whitespace(s);
            cue->tracks[cue->track_count-1].offset = 60*1000 * atoi(s);
            s = strchr(s,':') + 1;
            cue->tracks[cue->track_count-1].offset += 1000 * atoi(s);
            s = strchr(s,':') + 1;
            cue->tracks[cue->track_count-1].offset += 13 * atoi(s);
        }
        else if (!strncmp(s, "TITLE", 5)
                 || !strncmp(s, "PERFORMER", 9)
                 || !strncmp(s, "SONGWRITER", 10))
        {
            char *dest = NULL;
            char *string = get_string(s);
            if (!string)
                break;

            switch (*s)
            {
                case 'T': /* TITLE */
                    dest = (cue->track_count <= 0) ? cue->title :
                            cue->tracks[cue->track_count-1].title;
                    break;

                case 'P': /* PERFORMER */
                    dest = (cue->track_count <= 0) ? cue->performer :
                        cue->tracks[cue->track_count-1].performer;
                    break;

                case 'S': /* SONGWRITER */
                    dest = (cue->track_count <= 0) ? cue->songwriter :
                            cue->tracks[cue->track_count-1].songwriter;
                    break;
            }

            if (dest) {
                dest = iso_decode(string, dest, -1, MIN(strlen(string), MAX_NAME));
                *dest = '\0';
            }    
        }
    }
    close(fd);

    /* If some songs don't have performer info, we copy the cuesheet performer */
    int i;
    for (i = 0; i < cue->track_count; i++)
    {
        if (*(cue->tracks[i].performer) == '\0')
            strncpy(cue->tracks[i].performer, cue->performer, MAX_NAME);

        if (*(cue->tracks[i].songwriter) == '\0')
            strncpy(cue->tracks[i].songwriter, cue->songwriter, MAX_NAME);
    }

    return true;
}

/* takes care of seeking to a track in a playlist
 * returns false if audio  isn't playing */
static bool seek(unsigned long pos)
{
    if (!(audio_status() & AUDIO_STATUS_PLAY))
    {
        return false;
    }
    else
    {
#if (CONFIG_CODEC == SWCODEC)
        audio_pre_ff_rewind();
        audio_ff_rewind(pos);
#else
        audio_pause();
        audio_ff_rewind(pos);
        audio_resume();
#endif
        return true;
    }
}

/* returns the index of the track currently being played
   and updates the information about the current track. */
int cue_find_current_track(struct cuesheet *cue, unsigned long curpos)
{
    int i=0;
    while (i < cue->track_count-1 && cue->tracks[i+1].offset < curpos)
        i++;

    cue->curr_track_idx = i;
    cue->curr_track = cue->tracks + i;
    return i;
}

/* callback that gives list item titles for the cuesheet browser */
static char *list_get_name_cb(int selected_item,
                              void *data,
                              char *buffer,
                              size_t buffer_len)
{
    struct cuesheet *cue = (struct cuesheet *)data;

    if (selected_item & 1)
        snprintf(buffer, buffer_len, "%s",
                 cue->tracks[selected_item/2].title);
    else
        snprintf(buffer, buffer_len, "%02d. %s", selected_item/2+1,
                 cue->tracks[selected_item/2].performer);

    return buffer;
}

void browse_cuesheet(struct cuesheet *cue)
{
    struct gui_synclist lists;
    int action;
    bool done = false;
    int sel;
    char title[MAX_PATH];
    char cuepath[MAX_PATH];
    struct mp3entry *id3 = audio_current_track();

    snprintf(title, MAX_PATH, "%s: %s", cue->performer, cue->title);
    gui_synclist_init(&lists, list_get_name_cb, cue, false, 2, NULL);
    gui_synclist_set_nb_items(&lists, 2*cue->track_count);
    gui_synclist_set_title(&lists, title, 0);

    if (id3 && *id3->path && strcmp(id3->path, "No file!"))
    {
        look_for_cuesheet_file(id3->path, cuepath);
    }

    if (id3 && id3->cuesheet_type && !strcmp(cue->path, cuepath))
    {
        gui_synclist_select_item(&lists,
                                 2*cue_find_current_track(cue, id3->elapsed));
    }

    while (!done)
    {
        gui_synclist_draw(&lists);
        action = get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (gui_synclist_do_button(&lists, &action, LIST_WRAP_UNLESS_HELD))
            continue;
        switch (action)
        {
            case ACTION_STD_OK:
                id3 = audio_current_track();
                if (id3 && *id3->path && strcmp(id3->path, "No file!"))
                {
                    look_for_cuesheet_file(id3->path, cuepath);
                    if (id3->cuesheet_type && !strcmp(cue->path, cuepath))
                    {
                        sel = gui_synclist_get_sel_pos(&lists);
                        seek(cue->tracks[sel/2].offset);
                    }
                }
                break;
            case ACTION_STD_CANCEL:
                done = true;
        }
    }
}

bool display_cuesheet_content(char* filename)
{
    size_t bufsize = 0;
    struct cuesheet *cue = (struct cuesheet *)plugin_get_buffer(&bufsize);
    if (!cue || bufsize < sizeof(struct cuesheet))
        return false;

    if (!parse_cuesheet(filename, cue))
        return false;

    browse_cuesheet(cue);
    return true;
}

/* skips backwards or forward in the current cuesheet
 * the return value indicates whether we're still in a cusheet after skipping
 * it also returns false if we weren't in a cuesheet.
 * direction should be 1 or -1.
 */
bool curr_cuesheet_skip(int direction, unsigned long curr_pos)
{
    int track = cue_find_current_track(curr_cue, curr_pos);

    if (direction >= 0 && track == curr_cue->track_count - 1)
    {
        /* we want to get out of the cuesheet */
        return false;
    }
    else
    {
        if (!(direction <= 0 && track == 0))
            track += direction;

        seek(curr_cue->tracks[track].offset);
        return true;
    }

}

void cue_spoof_id3(struct cuesheet *cue, struct mp3entry *id3)
{
    if (!cue || !cue->curr_track)
        return;

    struct cue_track_info *track = cue->curr_track;

    id3->title = *track->title ? track->title : NULL;
    id3->artist = *track->performer ? track->performer : NULL;
    id3->composer = *track->songwriter ? track->songwriter : NULL;
    id3->album = *cue->title ? cue->title : NULL;

    /* if the album artist is the same as the track artist, we hide it. */
    if (strcmp(cue->performer, track->performer))
        id3->albumartist = *cue->performer ? cue->performer : NULL;
    else
        id3->albumartist = NULL;

    int i = cue->curr_track_idx;
    id3->tracknum = i+1;
    if (id3->track_string)
        snprintf(id3->track_string, 10, "%d/%d", i+1, cue->track_count);
}

#ifdef HAVE_LCD_BITMAP
static inline void draw_veritcal_line_mark(struct screen * screen,
                                           int x, int y, int h)
{
    screen->set_drawmode(DRMODE_COMPLEMENT);
    screen->vline(x, y, y+h-1);
}

/* draw the cuesheet markers for a track of length "tracklen",
   between (x1,y) and (x2,y) */
void cue_draw_markers(struct screen *screen, unsigned long tracklen,
                      int x1, int x2, int y, int h)
{
    int i,xi;
    int w = x2 - x1;
    for (i=1; i < curr_cue->track_count; i++)
    {
        xi = x1 + (w * curr_cue->tracks[i].offset)/tracklen;
        draw_veritcal_line_mark(screen, xi, y, h);
    }
}
#endif
