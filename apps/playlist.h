/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include <stdbool.h>
#include "file.h"
#include "kernel.h"

/* playlist data */

struct playlist_info
{
    char filename[MAX_PATH];  /* path name of m3u playlist on disk  */
    int  fd;             /* descriptor of the open playlist file    */
    int  control_fd;     /* descriptor of the open control file     */
    int  dirlen;         /* Length of the path to the playlist file */
    unsigned int *indices; /* array of indices                      */
    int  max_playlist_size; /* Max number of files in playlist. Mirror of
                              global_settings.max_files_in_playlist */
    bool in_ram;         /* playlist stored in ram (dirplay)        */
    char *buffer;        /* buffer for in-ram playlists             */
    int  buffer_size;    /* size of buffer                          */
    int  buffer_end_pos; /* last position where buffer was written  */
    int  index;          /* index of current playing track          */
    int  first_index;    /* index of first song in playlist         */
    int  amount;         /* number of tracks in the index           */
    int  last_insert_pos; /* last position we inserted a track      */
    struct mutex control_mutex; /* mutex for control file access    */
};

void playlist_init(void);
int playlist_create(char *dir, char *file);
int playlist_resume(void);
int playlist_add(char *filename);
int playlist_insert_track(char *filename, int position, bool queue);
int playlist_insert_directory(char *dirname, int position, bool queue);
int playlist_insert_playlist(char *filename, int position, bool queue);
int playlist_delete(int index);
int playlist_shuffle(int random_seed, int start_index);
int playlist_randomise(unsigned int seed, bool start_current);
int playlist_sort(bool start_current);
int playlist_start(int start_index, int offset);
bool playlist_check(int steps);
char *playlist_peek(int steps);
int playlist_next(int steps);
int playlist_get_resume_info(int *resume_index);
int playlist_get_display_index(void);
int playlist_amount(void);
char *playlist_name(char *buf, int buf_size);
int playlist_save(char *filename);

enum {
    PLAYLIST_PREPEND = -1,
    PLAYLIST_INSERT = -2,
    PLAYLIST_INSERT_LAST = -3,
    PLAYLIST_INSERT_FIRST = -4
};

#endif /* __PLAYLIST_H__ */
