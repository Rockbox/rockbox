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

#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "playlist.h"
#include <file.h>
#include "sprintf.h"
#include "debug.h"
#include "mpeg.h"

playlist_info_t playlist;

int index_array[1000];

char now_playing[256];

char* playlist_next(int type)
{
    int seek = playlist.indices[playlist.index];
    int max;
    int fd;
    (void)type; /* prevent compiler warning until this is gets used */

    playlist.index = (playlist.index+1) % playlist.amount;

    fd = open(playlist.filename, O_RDONLY);
    if(-1 != fd) {
      lseek(fd, seek, SEEK_SET);
      max = read(fd, now_playing+1, sizeof(now_playing)-1);
      close(fd);

      /* Zero-terminate the file name */
      seek=0;
      while((now_playing[seek] != '\n') &&
            (now_playing[seek] != '\r') &&
            (seek < max))
        seek++;

      now_playing[seek]=0;

      if('/' == now_playing[1])
          return &now_playing[1];
      else
          return now_playing;
    }
    else
        return NULL;
}

void play_list(char *dir, char *file)
{
    char *sep="";
    empty_playlist(&playlist);

    /* If the dir does not end in trailing new line, we use a separator.
       Otherwise we don't. */
    if('/' != dir[strlen(dir)-1])
        sep="/";

    snprintf(playlist.filename, sizeof(playlist.filename),
             "%s%s%s", 
             dir, sep, file);

    /* add track indices to playlist data structure */
    add_indices_to_playlist(&playlist);

    /* if shuffle is wanted, this is where to do that */

    /* also make the first song get playing */
    mpeg_play(playlist_next(0));

}

/*
 * remove any filename and indices associated with the playlist
 */
void empty_playlist( playlist_info_t *playlist )
{
    playlist->filename[0] = '\0';
    playlist->index = 0;
    playlist->amount = 0;
}

/*
 * calculate track offsets within a playlist file
 */
void add_indices_to_playlist( playlist_info_t *playlist )
{
    int nread;
    int fd;
    int i = 0;
    int store_index = 0;
    int count = 0;

    unsigned char *p;
    unsigned char buf[255];


    fd = open(playlist->filename, O_RDONLY);
    if(-1 == fd)
        return; /* failure */

    store_index = 1;

    while((nread = read(fd, &buf, sizeof(buf))) != 0)
    {
        p = buf;

        for(count=0; count < nread; count++,p++) {

            /* Are we on a new line? */
            if(((*p == '\n') || (*p == '\r')) && (!store_index))
            {
                store_index = 1;
            } 
            else if(store_index) 
            {
                /* Store a new entry */
                DEBUGF("tune %d at position %d\n", playlist->amount, i+count);
                playlist->indices[ playlist->amount ] = i+count;
                playlist->amount++;

                store_index = 0;
            }
        }

        i+= count;
    }


    close(fd);
}

static unsigned int playlist_seed = 0xdeadcafe;
static void seedit(unsigned int seed)
{
    playlist_seed = seed;   
}

static int getrand(void)
{
    playlist_seed += 0x12345;

    /* the rand is from 0 to RAND_MAX */
    return playlist_seed;
}

/*
 * randomly rearrange the array of indices for the playlist
 */
void randomise_playlist( playlist_info_t *playlist, unsigned int seed )
{
    int count = 0;
    int candidate;
    int store;
    
    /* seed with the given seed */
    seedit( seed );

    /* randomise entire indices list */
    
    while( count < playlist->amount )
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        candidate = getrand() % ( playlist->amount );

        /* now swap the values at the 'count' and 'candidate' positions */
        store = playlist->indices[candidate];
        playlist->indices[candidate] = playlist->indices[count];
        playlist->indices[count] = store;
                
        /* move along */
        count++;
    }
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../firmware/rockbox-mode.el")
 * end:
 */
