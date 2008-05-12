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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
