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
#include "applimits.h"

/* playlist data */

struct playlist_info
{
    char filename[MAX_PATH];  /* path name of m3u playlist on disk  */
    int  fd;             /* file descriptor of the open playlist    */
    int  dirlen;         /* Length of the path to the playlist file */
    int  *indices;       /* array of indices            */
    int max_playlist_size; /* Max number of files in playlist. Mirror of
                              global_settings.max_files_in_playlist */
    int buffer_size;     /* Playlist buffer size */
    int  index;          /* index of current playing track          */
    int  first_index;    /* index of first song in playlist         */
    int  seed;           /* random seed                             */
    int  amount;         /* number of tracks in the index           */
    bool in_ram;         /* True if the playlist is RAM-based       */

    /* Queue function */
    int queue_indices[MAX_QUEUED_FILES]; /* array of queue indices */
    int last_queue_index; /* index of last queued track            */
    int queue_index;    /* index of current playing queued track   */
    int num_queued;     /* number of songs queued                  */
    int start_queue;    /* the first song was queued               */
};

extern struct playlist_info playlist;
extern bool playlist_shuffle;

void playlist_init(void);
int play_list(char *dir, char *file, int start_index, 
              bool shuffled_index, int start_offset,
              int random_seed, int first_index, int queue_resume,
              int queue_resume_index);
char* playlist_peek(int steps);
char* playlist_name(char *name, int name_size);
int playlist_next(int steps);
bool playlist_check(int steps);
void randomise_playlist( unsigned int seed );
void sort_playlist(bool start_current);
void add_indices_to_playlist(void);
void playlist_clear(void);
int playlist_add(char *filename);
int queue_add(char *filename);
int playlist_amount(void);
int playlist_first_index(void);
int playlist_get_resume_info(int *resume_index, int *queue_resume,
                             int *queue_resume_index);

enum { QUEUE_OFF, QUEUE_BEGIN_QUEUE, QUEUE_BEGIN_PLAYLIST, NUM_QUEUE_MODES };

#endif /* __PLAYLIST_H__ */

