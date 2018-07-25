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
#include "misc.h"
#include "screens.h"
#include "list.h"
#include "action.h"
#include "lang.h"
#include "debug.h"
#include "settings.h"
#include "plugin.h"
#include "playback.h"
#include "cuesheet.h"
#include "gui/wps.h"

#define CUE_DIR ROCKBOX_DIR "/cue"

static bool search_for_cuesheet(const char *path, struct cuesheet_file *cue_file)
{
    size_t len;
    char cuepath[MAX_PATH];
    char *dot, *slash, *slash_cuepath;

    cue_file->pos = 0;
    cue_file->size = 0;
    cue_file->path[0] = '\0';
    slash = strrchr(path, '/');
    if (!slash)
        return false;
    len = strlcpy(cuepath, path, MAX_PATH);
    slash_cuepath = &cuepath[slash - path];
    dot = strrchr(slash_cuepath, '.');
    if (dot)
        strlcpy(dot, ".cue", MAX_PATH - (dot-cuepath));

    if (!dot || !file_exists(cuepath))
    {
        strcpy(cuepath, CUE_DIR);
        if (strlcat(cuepath, slash, MAX_PATH) >= MAX_PATH)
            goto skip; /* overflow */   
        char *dot = strrchr(cuepath, '.');
        strcpy(dot, ".cue");
        if (!file_exists(cuepath))
        {
skip:
            if ((len+4) >= MAX_PATH)
                return false;
            strlcpy(cuepath, path, MAX_PATH);
            strlcat(cuepath, ".cue", MAX_PATH);
            if (!file_exists(cuepath))
                return false;
        }
    }

    strlcpy(cue_file->path, cuepath, MAX_PATH);
    return true;
}

bool look_for_cuesheet_file(struct mp3entry *track_id3, struct cuesheet_file *cue_file)
{
    /* DEBUGF("look for cue file\n"); */
    if (track_id3->has_embedded_cuesheet)
    {
        cue_file->pos = track_id3->embedded_cuesheet.pos;
        cue_file->size = track_id3->embedded_cuesheet.size;
        cue_file->encoding = track_id3->embedded_cuesheet.encoding;
        strlcpy(cue_file->path, track_id3->path, MAX_PATH);
        return true;
    }

    return search_for_cuesheet(track_id3->path, cue_file);
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

/* parse cuesheet "cue_file" and store the information in "cue" */
bool parse_cuesheet(struct cuesheet_file *cue_file, struct cuesheet *cue)
{
    char line[MAX_PATH];
    char *s;
    unsigned char char_enc = CHAR_ENC_ISO_8859_1;
    bool is_embedded = false;
    int line_len;
    int bytes_left = 0;
    int read_bytes = MAX_PATH;
    unsigned char utf16_buf[MAX_PATH];

    int fd = open(cue_file->path, O_RDONLY, 0644);
    if(fd < 0)
        return false;
    if (cue_file->pos > 0)
    {
        is_embedded = true;
        lseek(fd, cue_file->pos, SEEK_SET);
        bytes_left = cue_file->size;
        char_enc = cue_file->encoding;
    }

    /* Look for a Unicode BOM */
    unsigned char bom_read = 0;
    read(fd, line, BOM_UTF_8_SIZE);
    if(!memcmp(line, BOM_UTF_8, BOM_UTF_8_SIZE))
    {
        char_enc = CHAR_ENC_UTF_8;
        bom_read = BOM_UTF_8_SIZE;
    }
    else if(!memcmp(line, BOM_UTF_16_LE, BOM_UTF_16_SIZE))
    {
        char_enc = CHAR_ENC_UTF_16_LE;
        bom_read = BOM_UTF_16_SIZE;
    }
    else if(!memcmp(line, BOM_UTF_16_BE, BOM_UTF_16_SIZE))
    {
        char_enc = CHAR_ENC_UTF_16_BE;
        bom_read = BOM_UTF_16_SIZE;
    }
    if (bom_read < BOM_UTF_8_SIZE)
        lseek(fd, cue_file->pos + bom_read, SEEK_SET);
    if (is_embedded)
    {
        if (bom_read  > 0)
            bytes_left -= bom_read;
        if (read_bytes > bytes_left)
            read_bytes = bytes_left;
    }

    /* Initialization */
    memset(cue, 0, sizeof(struct cuesheet));
    strcpy(cue->path, cue_file->path);
    cue->curr_track = cue->tracks;

    if (is_embedded)
        strcpy(cue->file, cue->path);

    while ((line_len = read_line(fd, line, read_bytes)) > 0
        && cue->track_count < MAX_TRACKS )
    {
        if (char_enc == CHAR_ENC_UTF_16_LE)
        {
            s = utf16LEdecode(line, utf16_buf, line_len);
            /* terminate the string at the newline */
            *s = '\0';
            strcpy(line, utf16_buf);
            /* chomp the trailing 0 after the newline */
            lseek(fd, 1, SEEK_CUR);
            line_len++;
        }
        else if (char_enc == CHAR_ENC_UTF_16_BE)
        {
            s = utf16BEdecode(line, utf16_buf, line_len);
            *s = '\0';
            strcpy(line, utf16_buf);
        }
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
                 || !strncmp(s, "SONGWRITER", 10)
                 || !strncmp(s, "FILE", 4))
        {
            char *dest = NULL;
            char *string = get_string(s);
            if (!string)
                break;

            size_t count = MAX_NAME*3 + 1;
            size_t count8859 = MAX_NAME;

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

                case 'F': /* FILE */
                    if (is_embedded || cue->track_count > 0)
                        break;

                    dest = cue->file;
                    count = MAX_PATH;
                    count8859 = MAX_PATH/3;
                    break;
            }

            if (dest) 
            {
                if (char_enc == CHAR_ENC_ISO_8859_1)
                {
                    dest = iso_decode(string, dest, -1,
                        MIN(strlen(string), count8859));
                    *dest = '\0';
                }
                else
                {
                    strlcpy(dest, string, count);
                }
            }    
        }
        if (is_embedded)
        {
            bytes_left -= line_len;
            if (bytes_left <= 0)
                break;
            if (bytes_left < read_bytes)
                read_bytes = bytes_left;
        }
    }
    close(fd);

    /* If just a filename, add path information from cuesheet path */
    if (*cue->file && !strrchr(cue->file, '/'))
    {
        strcpy(line, cue->file);
        strcpy(cue->file, cue->path);
        char *slash = strrchr(cue->file, '/');
        if (!slash++) slash = cue->file;
        strlcpy(slash, line, MAX_PATH - (slash - cue->file));
    }

    /* If some songs don't have performer info, we copy the cuesheet performer */
    int i;
    for (i = 0; i < cue->track_count; i++)
    {
        if (*(cue->tracks[i].performer) == '\0')
            strlcpy(cue->tracks[i].performer, cue->performer, MAX_NAME*3);

        if (*(cue->tracks[i].songwriter) == '\0')
            strlcpy(cue->tracks[i].songwriter, cue->songwriter, MAX_NAME*3);
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
static const char* list_get_name_cb(int selected_item,
                                    void *data,
                                    char *buffer,
                                    size_t buffer_len)
{
    struct cuesheet *cue = (struct cuesheet *)data;

    if (selected_item & 1)
        strlcpy(buffer, cue->tracks[selected_item/2].title, buffer_len);
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
    char title[MAX_PATH];
    struct cuesheet_file cue_file;
    struct mp3entry *id3 = audio_current_track();

    snprintf(title, MAX_PATH, "%s: %s", cue->performer, cue->title);
    gui_synclist_init(&lists, list_get_name_cb, cue, false, 2, NULL);
    gui_synclist_set_nb_items(&lists, 2*cue->track_count);
    gui_synclist_set_title(&lists, title, 0);


    if (id3)
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
            {
                bool startit = true;
                unsigned long elapsed =
                    cue->tracks[gui_synclist_get_sel_pos(&lists)/2].offset;

                id3 = audio_current_track();
                if (id3 && *id3->path)
                {
                    look_for_cuesheet_file(id3, &cue_file);
                    if (!strcmp(cue->path, cue_file.path))
                        startit = false;
                }

                if (!startit)
                    startit = !seek(elapsed);

                if (!startit || !*cue->file)
                    break;

                /* check that this cue is the same one that would be found by
                   a search from playback */
                char file[MAX_PATH];
                strlcpy(file, cue->file, MAX_PATH);

                if (!strcmp(cue->path, file) || /* if embedded */
                    (search_for_cuesheet(file, &cue_file) &&
                     !strcmp(cue->path, cue_file.path)))
                {
                    char *fname = strrsplt(file, '/');
                    char *dirname = fname <= file + 1 ? "/" : file;
                    bookmark_play(dirname, 0, elapsed, 0, current_tick, fname);
                }
                break;
                } /* ACTION_STD_OK */

            case ACTION_STD_CANCEL:
                done = true;
        }
    }
}

bool display_cuesheet_content(char* filename)
{
    size_t bufsize = 0;
    struct cuesheet_file cue_file;
    struct cuesheet *cue = (struct cuesheet *)plugin_get_buffer(&bufsize);
    if (!cue || bufsize < sizeof(struct cuesheet))
        return false;

    strlcpy(cue_file.path, filename, MAX_PATH);
    cue_file.pos = 0;
    cue_file.size = 0;

    if (!parse_cuesheet(&cue_file, cue))
        return false;

    browse_cuesheet(cue);
    return true;
}

/* skips backwards or forward in the current cuesheet
 * the return value indicates whether we're still in a cusheet after skipping
 * it also returns false if we weren't in a cuesheet.
 * direction should be 1 or -1.
 */
bool curr_cuesheet_skip(struct cuesheet *cue, int direction, unsigned long curr_pos)
{
    int track = cue_find_current_track(cue, curr_pos);
    
    if (direction >= 0 && track == cue->track_count - 1)
    {
        /* we want to get out of the cuesheet */
        return false;
    }
    else
    {
        if (!(direction <= 0 && track == 0))
        {
            /* If skipping forward, skip to next cuesheet segment. If skipping
            backward before DEFAULT_SKIP_TRESH milliseconds have elapsed, skip
            to previous cuesheet segment. If skipping backward after
            DEFAULT_SKIP_TRESH seconds have elapsed, skip to the start of the
            current cuesheet segment */
            if (direction == 1 || 
                  ((curr_pos - cue->tracks[track].offset) < DEFAULT_SKIP_TRESH))
            {
                track += direction;
            }
        }

        seek(cue->tracks[track].offset);
        return true;
    }

}

#ifdef HAVE_LCD_BITMAP
static inline void draw_veritcal_line_mark(struct screen * screen,
                                           int x, int y, int h)
{
    screen->set_drawmode(DRMODE_COMPLEMENT);
    screen->vline(x, y, y+h-1);
}

/* draw the cuesheet markers for a track of length "tracklen",
   between (x,y) and (x+w,y) */
void cue_draw_markers(struct screen *screen, struct cuesheet *cue,
                      unsigned long tracklen,
                      int x, int y, int w, int h)
{
    int i,xi;
    unsigned long tracklen_seconds = tracklen/1000; /* duration in seconds */
    
    for (i=1; i < cue->track_count; i++)
    {
        /* Convert seconds prior to multiplication to avoid overflow. */
        xi = x + (w * (cue->tracks[i].offset/1000)) / tracklen_seconds;
        draw_veritcal_line_mark(screen, xi, y, h);
    }
}
#endif

bool cuesheet_subtrack_changed(struct mp3entry *id3)
{
    struct cuesheet *cue = id3->cuesheet;
    if (cue && (id3->elapsed < cue->curr_track->offset
            || (cue->curr_track_idx < cue->track_count - 1
                && id3->elapsed >= (cue->curr_track+1)->offset)))
    {
        cue_find_current_track(cue, id3->elapsed);
        return true;
    }
    return false;
}
