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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include <stdbool.h>
#include "config.h"
#include "file.h"
#include "kernel.h"
#include "metadata.h"
#include "rbpaths.h"
#include "chunk_alloc.h"

#define PLAYLIST_ATTR_QUEUED    0x01
#define PLAYLIST_ATTR_INSERTED  0x02
#define PLAYLIST_ATTR_SKIPPED   0x04
#define PLAYLIST_ATTR_RETRIEVE_ID3_ATTEMPTED   0x08
#define PLAYLIST_ATTR_RETRIEVE_ID3_SUCCEEDED   0x10

#define PLAYLIST_DISPLAY_COUNT  10

#define PLAYLIST_UNTITLED_PREFIX "Playlist "

#define PLAYLIST_FLAG_MODIFIED (1u << 0) /* playlist was manually modified */
#define PLAYLIST_FLAG_DIRPLAY  (1u << 1) /* enable directory skipping */

enum playlist_command {
    PLAYLIST_COMMAND_PLAYLIST,
    PLAYLIST_COMMAND_ADD,
    PLAYLIST_COMMAND_QUEUE,
    PLAYLIST_COMMAND_DELETE,
    PLAYLIST_COMMAND_SHUFFLE,
    PLAYLIST_COMMAND_UNSHUFFLE,
    PLAYLIST_COMMAND_RESET,
    PLAYLIST_COMMAND_CLEAR,
    PLAYLIST_COMMAND_FLAGS,
    PLAYLIST_COMMAND_COMMENT,
    PLAYLIST_COMMAND_ERROR = PLAYLIST_COMMAND_COMMENT + 1 /* Internal */
};

enum {
    PLAYLIST_PREPEND = -1,
    PLAYLIST_INSERT = -2,
    PLAYLIST_INSERT_LAST = -3,
    PLAYLIST_INSERT_FIRST = -4,
    PLAYLIST_INSERT_SHUFFLED = -5,
    PLAYLIST_REPLACE = -6,
    PLAYLIST_INSERT_LAST_SHUFFLED = -7,
    PLAYLIST_INSERT_LAST_ROTATED = -8
};

struct playlist_info
{
    bool utf8;           /* playlist is in .m3u8 format             */
    bool control_created; /* has control file been created?         */
    unsigned int flags;  /* flags for misc. state */
    int  fd;             /* descriptor of the open playlist file    */
    int  control_fd;     /* descriptor of the open control file     */
    int  max_playlist_size; /* Max number of files in playlist. Mirror of
                              global_settings.max_files_in_playlist */
    unsigned long *indices; /* array of indices            */

    int  index;          /* index of current playing track          */
    int  first_index;    /* index of first song in playlist         */
    int  amount;         /* number of tracks in the index           */
    int  last_insert_pos; /* last position we inserted a track      */
    bool started;       /* has playlist been started?               */
    int last_shuffled_start; /* number of tracks when insert last
                                    shuffled command start */
    int  seed;           /* shuffle seed                            */
    struct mutex mutex; /* mutex for control file access    */
#ifdef HAVE_DIRCACHE
    int dcfrefs_handle;
#endif
    int  dirlen;         /* Length of the path to the playlist file */
    char filename[MAX_PATH];  /* path name of m3u playlist on disk  */
    /* full path of control file (with extra room for extensions) */
    char control_filename[sizeof(PLAYLIST_CONTROL_FILE) + 8];
};

struct playlist_track_info
{
    char filename[MAX_PATH]; /* path name of mp3 file               */
    int  attr;               /* playlist attributes for track       */
    int  index;              /* index of track in playlist          */
    int  display_index;      /* index of track for display          */
};

struct playlist_insert_context {
    struct playlist_info* playlist;
    int position;
    bool queue;
    bool progress;
    bool initialized;
    int count;
    int32_t count_langid;
};

/* Exported functions only for current playlist. */
void playlist_init(void) INIT_ATTR;
void playlist_shutdown(void);
int playlist_create(const char *dir, const char *file);
int playlist_resume(void);
int playlist_shuffle(int random_seed, int start_index);
unsigned int playlist_get_filename_crc32(struct playlist_info *playlist,
                                         int index);
void playlist_resume_track(int start_index, unsigned int crc,
                           unsigned long elapsed, unsigned long offset);
void playlist_start(int start_index, unsigned long elapsed,
                    unsigned long offset);
bool playlist_check(int steps);
const char *playlist_peek(int steps, char* buf, size_t buf_size);
int playlist_next(int steps);
bool playlist_next_dir(int direction);
int playlist_get_resume_info(int *resume_index);
int playlist_update_resume_info(const struct mp3entry* id3);
int playlist_get_display_index(void);
int playlist_amount(void);
void playlist_set_last_shuffled_start(void);
struct playlist_info *playlist_get_current(void);
bool playlist_dynamic_only(void);

/* Exported functions for all playlists.  Pass NULL for playlist_info
   structure to work with current playlist. */
size_t playlist_get_index_bufsz(size_t max_sz);
struct playlist_info* playlist_load(const char* dir, const char* file,
                                    void* index_buffer, int index_buffer_size,
                                    void* temp_buffer, int temp_buffer_size);
int playlist_set_current(struct playlist_info* playlist);
void playlist_close(struct playlist_info* playlist);
void playlist_sync(struct playlist_info* playlist);
int playlist_insert_track(struct playlist_info* playlist, const char *filename,
                          int position, bool queue, bool sync);
int playlist_insert_context_create(struct playlist_info* playlist,
                                   struct playlist_insert_context *context,
                                   int position, bool queue, bool progress);
int playlist_insert_context_add(struct playlist_insert_context *context,
                                const char *filename);
void playlist_insert_context_release(struct playlist_insert_context *context);
int playlist_insert_directory(struct playlist_info* playlist,
                              const char *dirname, int position, bool queue,
                              bool recurse);
int playlist_insert_playlist(struct playlist_info* playlist, const char *filename,
                             int position, bool queue);
bool playlist_entries_iterate(const char *filename,
                              struct playlist_insert_context *pl_context,
                              bool (*action_cb)(const char *file_name));
void playlist_skip_entry(struct playlist_info *playlist, int steps);
int playlist_delete(struct playlist_info* playlist, int index);
int playlist_move(struct playlist_info* playlist, int index, int new_index);
int playlist_randomise(struct playlist_info* playlist, unsigned int seed,
                       bool start_current);
int playlist_sort(struct playlist_info* playlist, bool start_current);
bool playlist_modified(const struct playlist_info* playlist);
void playlist_set_modified(struct playlist_info* playlist, bool modified);
bool playlist_allow_dirplay(const struct playlist_info* playlist);
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
int playlist_directory_tracksearch(const char* dirname, bool recurse,
                                   int (*callback)(char*, void*),
                                   void* context);
int playlist_remove_all_tracks(struct playlist_info *playlist);

#endif /* __PLAYLIST_H__ */
