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

playlist_info_t playlist;

int index_array[1000];

char now_playing[256];

char* playlist_next(int type)
{
    int seek = playlist.indices[playlist.index];
    int max;
    int fd;
    playlist.index = (playlist.index+1) % playlist.amount;

    fd = open(playlist.filename, O_RDONLY);
    if(-1 != fd) {
      lseek(fd, seek, SEEK_SET);
      max = read(fd, now_playing, sizeof(now_playing));
      close(fd);
      
      seek=0;
      while((now_playing[seek] != '\n') &&
            (now_playing[seek] != '\r') &&
            (seek < max))
        seek++;
      if(seek == max)
        seek = max-1;
      now_playing[seek]=0;

      return now_playing;
    }
    else
      return NULL;
}

void play_list(char *dir, char *file)
{
    empty_playlist(&playlist);

    snprintf(playlist.filename, sizeof(playlist.filename),
             "%s/%s", dir, file);

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
    char *p;
    int   i = 0;
    unsigned char byte;
    unsigned char lastbyte='\n';
    int nread;

    int fd;

    fd = open(playlist->filename, O_RDONLY);
    if(-1 == fd)
        return; /* failure */

    p = &byte;
    
    /* loop thru buffer, store index whenever we get a new line */
    
    while((nread = read(fd, &byte, 1)) == 1)
    {
        /* move thru (any) newlines */
        
        if(( byte != '\n' ) && ( byte != '\r' ) &&
           ((lastbyte == '\n') || (lastbyte == '\r'))) {
            /* we're now at the start of a new track filename. store index */

            DEBUGF("tune %d at position %d\n", playlist->amount, i);
            playlist->indices [ playlist->amount ] = i;
            playlist->amount++;
        }
        i++;
        lastbyte = byte;
    }

    close(fd);
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
    srand( seed );

    /* randomise entire indices list */
    
    while( count < playlist->amount )
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        candidate = rand() % ( playlist->amount );

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
