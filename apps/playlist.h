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

/* playlist data */

#define MAX_PLAYLIST_SIZE 1000
typedef struct 
{
    char filename[256];  /* path name of m3u playlist on disk       */
    int  indices[MAX_PLAYLIST_SIZE]; /* array of indices            */
    int  index;          /* index of *NEXT* track to play           */
    int  seed;           /* random seed                             */
    int  amount;         /* number of tracks in the index           */
} playlist_info_t;

void play_list(char *dir, char *file);

void read_entire_file( char *buf, const char *filename );
void load_playlist( playlist_info_t *playlist, const char *filename );
void extract_playlist_indices( char *buf, playlist_info_t *playlist );
void display_current_playlist( playlist_info_t *playlist );
void get_indices_as_string( char *string, playlist_info_t *playlist );
void empty_playlist( playlist_info_t *playlist );
void add_indices_to_playlist( playlist_info_t *playlist );
void extend_indices( playlist_info_t *playlist, int new_index );
void randomise_playlist( playlist_info_t *playlist, unsigned int seed );
int is_unused_random_in_list( int number, int *original_list, int count );

/**********/






int create_playlist( void );
/*int add_to_playlist( track_t *track );*/
int remove_from_playlist( int index );
int set_playlist_position( void );
/*track_t * get_previous_entry_in_playlist( void );*/

#endif /* __PLAYLIST_H__ */
