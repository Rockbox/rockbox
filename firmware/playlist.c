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
#include "debug.h"
#include "disk.h"

/*
 * load playlist info from disk
 */
int reload_playlist_info( playlist_info_t *playlist )
{
    DEBUGF( "reload_playlist_info()\n" );

    /* this is a TEMP stub version */

    /* return a dummy playlist entry */

    strncpy( playlist->filename, "test.m3u", sizeof(playlist->filename) );

    playlist->indices_count = 4;

    playlist->indices = (void *)malloc( playlist->indices_count * sizeof( int ) );

    playlist->indices[0] = 3;
    playlist->indices[1] = 2;
    playlist->indices[2] = 1;
    playlist->indices[3] = 9;

    playlist->index = 3;
    
    return 1;
}

/*
 * read the contents of an m3u file from disk and store
 * the indices of each listed track in an array
 */
void load_playlist( playlist_info_t *playlist, const char *filename ) {

    char *m3u_buf = NULL;
    
    DEBUGF( "load_playlist( %s )\n", filename );
    
    /* read file */
    read_file_into_buffer( &m3u_buf, filename );

    /* store playlist filename */
    strncpy( playlist->filename, filename, sizeof(playlist->filename) );
    
    /* add track indices to playlist data structure */
    add_indices_to_playlist( m3u_buf, playlist );
}

/*
 * remove any filename and indices associated with the playlist
 */
void empty_playlist( playlist_info_t *playlist ) {

    DEBUGF( "empty_playlist()\n" );
    
    playlist->filename[0] = '\0';
    playlist->indices_count = 0;
    free( (void *)(playlist->indices) );
    playlist->indices = NULL;
    playlist->index = 0;
}

/*
 * calculate track offsets within a playlist buffer
 */
void add_indices_to_playlist( char *buf, playlist_info_t *playlist )
{
    char *p;
    int   i = 0;
    
    /*DEBUGF( "add_indices_to_playlist()\n" ); */
    
    p = buf;
    
    /* loop thru buffer, store index whenever we get a new line */
    
    while( p[i] != '\0' )
    {
        /* move thru (any) newlines */
        
        while( ( p[i] != '\0' ) && ( ( p[i] == '\n' ) || ( p[i] == '\r' ) ) )
        {
            i++;
        }

        /* did we get to the end of the buffer? */
        
        if( p[i] == '\0' )
        {
            break;
        }
        
        /* we're now at the start of a new track filename. store index */
        
        extend_indices( playlist, i );
        
        /* we're now inside a track name. move to next newline */
        
        while( ( p[i] != '\0' ) && ( ( p[i] != '\n' ) && ( p[i] != '\r' ) ) )
        {
            i++;
        }
    }
}

/*
 * extend the array of ints
 */
void extend_indices( playlist_info_t *playlist, int new_index )
{
    /*DEBUGF( "extend_indices(%d)\n", new_index ); */

    /* increase array size count */
    
    playlist->indices_count++;

    /* increase size of array to new count size */
    
    playlist->indices = (int *)realloc( playlist->indices, (playlist->indices_count) * sizeof( int ) );
    
    /* add new element to end of array */

    playlist->indices[ playlist->indices_count - 1 ] = new_index;
}

track_t next_playlist_track( playlist_info_t *playlist )
{
    track_t track;
    strncpy( track.filename, "boogie", sizeof(track.filename) );
    return track;
}

/*
 * randomly rearrange the array of indices for the playlist
 */
void randomise_playlist( playlist_info_t *playlist, unsigned int seed )
{
    int count = 0;
    int candidate;
    int store;
    
    DEBUGF( "randomise_playlist()\n" );

    /* seed with the given seed */
    srand( seed );

    /* randomise entire indices list */
    
    while( count < playlist->indices_count )
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        candidate = rand() % ( playlist->indices_count );

        /* now swap the values at the 'count' and 'candidate' positions */
        store = playlist->indices[candidate];
        playlist->indices[candidate] = playlist->indices[count];
        playlist->indices[count] = store;
                
        /* move along */
        count++;
    }
}

/*
 * dump the details of a track to stdout
 */
void display_playlist_track( track_t *track )
{
    DEBUGF( "track: %s\n", track->filename );
}

/*
 * dump the current playlist info
 */
void display_current_playlist( playlist_info_t *playlist )
{
    char indices[2048];
    indices[0]='\0';

    /*DEBUGF( "\ndisplay_current_playlist()\n" );  */
    
    if( playlist->indices_count != 0 )
    {
        get_indices_as_string( indices, playlist );
    }
    
    DEBUGF( "\nfilename:\t%s\ntotal:\t\t%d\nindices:\t%s\ncurrent index:\t%d\n\n",
            playlist->filename,
            playlist->indices_count,
            indices,
            playlist->index );
}

/*
 * produce a string of the playlist indices
 */
void get_indices_as_string( char *string, playlist_info_t *playlist )
{
    char tmp[8];
    int count = 0;
    int *p = playlist->indices;
    
    /*DEBUGF( "get_indices_as_string()\n" );  */

    while( count < playlist->indices_count ) {

        if( strlen( string ) == 0 )
        {
            /* first iteration - no comma */
            
            snprintf( tmp, sizeof(tmp), "%d", p[count] );
        }
        else
        {
            /* normal iteration - insert comma */
            
            snprintf( tmp, sizeof(tmp), ",%d", p[count] );
        }
        
        strcat( string, tmp );

        count++;
    }
}

/*
  Session Start (devlin.openprojects.net:alan): Thu Apr 25 15:13:27 2002
  <alan> for shuffle mode, it's really easy to use the array as a random stack
  <alan> just peek one randomly and exchange it with the last element in the stack
  <alan> when stack is void, just grow the stack with its initial size : elements are still there but in a random order 
  <alan> note :
  <alan> a stack is void when all songs were played
  <alan> it is an O(1) algo
  <alan> i don't see how you can do it with a list
*/

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
