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

/* 
 * View list of playlists in catalog.
 *  ret : true if no error
 */
bool catalog_view_playlists(void);

/* 
 * Add something to a playlist (new or select from list of playlists in
 * catalog).
 *  sel          : the path of the music file, playlist or directory to add
 *  sel_attr     : the attributes that tell what type of file we're adding
 *  new_playlist : whether we want to create a new playlist or add to an
 *                 existing one.
 *  m3u8name     : filename to save the playlist to, NULL to show the keyboard
 *  ret          : true if the file was successfully added
 */
bool catalog_add_to_a_playlist(const char* sel, int sel_attr,
                               bool new_playlist, char* m3u8name);

#endif
