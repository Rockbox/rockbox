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
#include "id3.h"

/* playlist data */

struct playlist_info
{
    bool current;        /* current playing playlist                */
    char filename[MAX_PATH];  /* path name of m3u playlist on disk  */
    char control_filename[MAX_PATH]; /* full path of control file   */
    int  fd;             /* descriptor of the open playlist file    */
    int  control_fd;     /* descriptor of the open control file     */
    bool control_created; /* has control file been created?         */
    int  dirlen;         /* Length of the path to the playlist file */
    unsigned long *indices; /* array of indices                     */
    const struct dircache_entry **filenames; /* Entries from dircache */
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
    int  seed;           /* shuffle seed                            */
    bool shuffle_modified; /* has playlist been shuffled with
                              inserted tracks?                      */
    bool deleted;        /* have any tracks been deleted?           */
    int num_inserted_tracks; /* number of tracks inserted           */
    bool shuffle_flush;  /* does shuffle value need to be flushed?  */
    struct mutex control_mutex; /* mutex for control file access    */
};

#define PLAYLIST_ATTR_QUEUED    0x01
#define PLAYLIST_ATTR_INSERTED  0x02
#define PLAYLIST_ATTR_SKIPPED   0x04

#define DEFAULT_DYNAMIC_PLAYLIST_NAME "/dynamic.m3u"

struct playlist_track_info
{
    char filename[MAX_PATH]; /* path name of mp3 file               */
    int  attr;               /* playlist attributes for track       */
    int  index;              /* index of track in playlist          */
    int  display_index;      /* index of track for display          */
};

/* Exported functions only for current playlist. */
void playlist_init(void);
int playlist_create(const char *dir, const char *file);
int playlist_resume(void);
int playlist_add(const char *filename);
int playlist_shuffle(int random_seed, int start_index);
int playlist_start(int start_index, int offset);
bool playlist_check(int steps);
char *playlist_peek(int steps);
int playlist_next(int steps);
bool playlist_next_dir(int direction);
int playlist_get_resume_info(int *resume_index);
int playlist_update_resume_info(const struct mp3entry* id3);
int playlist_get_display_index(void);
int playlist_amount(void);

/* Exported functions for all playlists.  Pass NULL for playlist_info
   structure to work with current playlist. */
int playlist_create_ex(struct playlist_info* playlist,
                       const char* dir, const char* file,
                       void* index_buffer, int index_buffer_size,
                       void* temp_buffer, int temp_buffer_size);
int playlist_set_current(struct playlist_info* playlist);
void playlist_close(struct playlist_info* playlist);
int playlist_insert_track(struct playlist_info* playlist, const char *filename,
                          int position, bool queue);
int playlist_insert_directory(struct playlist_info* playlist,
                              const char *dirname, int position, bool queue,
                              bool recurse);
int playlist_insert_playlist(struct playlist_info* playlist, char *filename,
                             int position, bool queue);
void playlist_skip_entry(struct playlist_info *playlist, int steps);
int playlist_delete(struct playlist_info* playlist, int index);
int playlist_move(struct playlist_info* playlist, int index, int new_index);
int playlist_randomise(struct playlist_info* playlist, unsigned int seed,
                       bool start_current);
int playlist_sort(struct playlist_info* playlist, bool start_current);
bool playlist_modified(const struct playlist_info* playlist);
int playlist_get_first_index(const struct playlist_info* playlist);
int playlist_get_seed(const struct playlist_info* playlist);
int playlist_amount_ex(const struct playlist_info* playlist);
char *playlist_name(const struct playlist_info* playlist, char *buf,
                    int buf_size);
char *playlist_get_name(const struct playlist_info* playlist, char *buf,
                        int buf_size);
int playlist_get_track_info(struct playlist_info* playlist, int index,
                            struct playlist_track_info* info);
int playlist_save(struct playlist_info* playlist, char *filename);

enum {
    PLAYLIST_PREPEND = -1,
    PLAYLIST_INSERT = -2,
    PLAYLIST_INSERT_LAST = -3,
    PLAYLIST_INSERT_FIRST = -4,
    PLAYLIST_INSERT_SHUFFLED = -5
};

enum {
    PLAYLIST_DELETE_CURRENT = -1
};

#endif /* __PLAYLIST_H__ */
