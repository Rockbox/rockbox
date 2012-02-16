/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Hardeep Sidhu
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

#ifndef _PLAYLIST_VIEWER_H_
#define _PLAYLIST_VIEWER_H_

/* Maximum number of tracks we can have loaded at one time */
#define MAX_PLAYLIST_ENTRIES 200

/* The number of items between the selected one and the end/start of
 * the buffer under which the buffer must reload */
#define MIN_BUFFER_MARGIN (screens[0].getnblines()+1)

/* Information about a specific track */
struct playlist_entry {
    char *name;                 /* Formatted track name                     */
    int index;                  /* Playlist index                           */
    int display_index;          /* Display index                            */
    bool queued;                /* Is track queued?                         */
    bool skipped;               /* Is track marked as bad?                  */
};

enum direction
{
    FORWARD,
    BACKWARD
};

struct playlist_buffer
{
    char *name_buffer;        /* Buffer used to store track names */
    int buffer_size;          /* Size of name buffer */

    int first_index;          /* Real index of first track loaded inside
                                 the buffer */

    enum direction direction; /* Direction of the buffer (if the buffer
                                 was loaded BACKWARD, the last track in
                                 the buffer has a real index < to the
                                 real index of the the first track)*/

    struct playlist_entry tracks[MAX_PLAYLIST_ENTRIES];
    int num_loaded;           /* Number of track entries loaded in buffer */
};

/* Global playlist viewer settings */
struct playlist_viewer {
    struct playlist_info* playlist; /* playlist being viewed                 */
    int num_tracks;             /* Number of tracks in playlist              */
    int current_playing_track;  /* Index of current playing track            */
    int selected_track;         /* The selected track, relative (first is 0) */
    int moving_track;           /* The track to move, relative (first is 0)
                                   or -1 if nothing is currently being moved */
    int moving_playlist_index;  /* Playlist-relative index (as opposed to
                                   viewer-relative index) of moving track    */
    struct playlist_buffer buffer;
};

/* A type for playlist viewer callbacks */
typedef enum playlist_viewer_callback_value(*pl_viewer_callback)(struct playlist_viewer*);

/* Playlist callback return values */
enum playlist_viewer_callback_value {
    PLAYLIST_VIEWER_ACTION_CONTINUE,
    PLAYLIST_VIEWER_ACTION_DO_NOTHING,
    PLAYLIST_VIEWER_ACTION_EXIT
};

enum playlist_viewer_callback_value playlist_viewer_default_callback(struct playlist_viewer* viewer);

enum playlist_viewer_result playlist_viewer(void);
enum playlist_viewer_result playlist_viewer_ex(const char* filename);
enum playlist_viewer_result playlist_viewer_ex_ex(const char* filename,
                                      pl_viewer_callback cancel_callback,
                                      pl_viewer_callback ok_callback,
                                      pl_viewer_callback std_context_callback,
                                      pl_viewer_callback std_menu_callback);
bool search_playlist(void);

enum playlist_viewer_result {
    PLAYLIST_VIEWER_OK,
    PLAYLIST_VIEWER_CANCEL,
    PLAYLIST_VIEWER_USB,
    PLAYLIST_VIEWER_MAINMENU,
};

#endif
