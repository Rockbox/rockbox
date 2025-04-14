/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Sebastian Henriksen, Hardeep Sidhu
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
#ifndef _PLAYLIST_CATALOG_H_
#define _PLAYLIST_CATALOG_H_

/* Gets the configured playlist catalog dir */
void catalog_get_directory(char* dirbuf, size_t dirbuf_sz);

/* Set the playlist catalog dir */
void catalog_set_directory(const char* directory);

/*
 * View list of playlists in catalog.
 *  ret : true if item was selected
 */
bool catalog_view_playlists(void);

bool catalog_pick_new_playlist_name(char *pl_name, size_t buf_size,
                                    const char* curr_pl_name);

int catalog_insert_into(const char* playlist, bool new_playlist,
                        const char* sel, int sel_attr);

/*
 * Add something to a playlist (new or select from list of playlists in
 * catalog).
 *  sel          : the path of the music file, playlist or directory to add
 *  sel_attr     : the attributes that tell what type of file we're adding
 *  new_playlist : whether we want to create a new playlist or add to an
 *                 existing one.
 *  m3u8name     : NULL, or filename to show in keyboard picker (include the extension!)
 *  add_to_pl_cb : can be NULL, or a function responsible for handling the
 *                 insert operations itself, in case the caller wants full
 *                 control over how and what files are actually added.
 *  ret          : true if the file was successfully added
 */
bool catalog_add_to_a_playlist(const char* sel, int sel_attr,
                               bool new_playlist, char* m3u8name,
                               void (*add_to_pl_cb));

#endif
