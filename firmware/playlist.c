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
    debug( "reload_playlist_info()\n" );

    /* this is a TEMP stub version */

    /* return a dummy playlist entry */

    strncpy( playlist->filename, "\\playlists\\1.m3u", sizeof(playlist->filename) );

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
    char debug_message[128];
    
    snprintf( debug_message, sizeof(debug_message),
              "load_playlist( %s )\n", filename );
    debug( debug_message );
    
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

    debug( "empty_playlist()\n" );
    
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
    
    /*debug( "add_indices_to_playlist()\n" ); */
    
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
    /*sprintf( debug_message, "extend_indices(%d)\n", new_index ); 
      debug( debug_message );*/

    /* increase array size count */
    
    playlist->indices_count++;

    /* increase size of array to new count size */
    
    playlist->indices = (int *)realloc( playlist->indices, (playlist->indices_count) * sizeof( int ) );
    
    /* add new element to end of array */

    playlist->indices[ playlist->indices_count - 1 ] = new_index;
}

track_t next_playlist_track( playlist_info_t *playlist ) {

    track_t track;
    strncpy( track.filename, "boogie", sizeof(track.filename) );
    return track;
}

/*
 * randomly rearrange the array of indices for the playlist
 */
void randomise_playlist( playlist_info_t *playlist ) {

    unsigned seed;
    int count = 0;
    int candidate;
    int adjusted_candidate;
    int found_next_number;
    int *index_list = (int*) malloc(sizeof(int) * playlist->indices_count);
    int *randomised_list;
    int i;
    
    debug( "randomise_playlist()\n" );

    /* create dynamic storage for randomised list so it can be freed later */

    randomised_list = (int *)malloc( playlist->indices_count * sizeof( int ) );
    
    /* use date as random seed */

    seed = time(0);
    srand( seed );

    /* randomise entire indices list */
    
    while( count < playlist->indices_count )
    {
        found_next_number = 0;
        
        /* loop until we successfully get the next number */
        
        while( ! found_next_number )
        {
            /* get the next random number */
            
            candidate = rand();
            
            /* the rand is from 0 to RAND_MAX, so adjust to our value range */
            
            adjusted_candidate = candidate % ( playlist->indices_count + 1 );
            
            /* has this number already been used? */
            
            if( is_unused_random_in_list( adjusted_candidate, index_list, playlist->indices_count ) )
            {
                /* store value found at random location in original list */
                
                index_list[ count ] = adjusted_candidate;
                
                /* leave loop */
                
                found_next_number = 1;
            }
        }
        
        /* move along */
        
        count++;
    }

    /* populate actual replacement list with values
     * found at indexes specified in index_list */
    
    for( i = 0; i < playlist->indices_count; i++ )
    {
        randomised_list[i] = playlist->indices[ index_list[ i ] ];
    }

    /* release memory from old array */
    
    free( (void *)playlist->indices );

    /* use newly randomise list */
    
    playlist->indices = randomised_list;
}

/*
 * check if random number has been used previously
 */
int is_unused_random_in_list( int number, int *new_list, int count )
{
    int i = 0;
    int *p = new_list;

    /* examine all in list */
    
    while( i < count )
    {
        /* did we find the number in the list already? */
        
        if( p[i] == number )
        {
            /* yes - return false */

            return 0;
        }

        /* move along list */
        
        i++;
    }

    /* if we got here, we didn't find the number. return true */
    
    return 1;
}

/*
 * dump the details of a track to stdout
 */
void display_playlist_track( track_t *track )
{
    debugf( "track: %s\n", track->filename );
}

/*
 * dump the current playlist info
 */
void display_current_playlist( playlist_info_t *playlist )
{
    char indices[2048];
    indices[0]='\0';

    /*debug( "\ndisplay_current_playlist()\n" );  */
    
    if( playlist->indices_count != 0 )
    {
        get_indices_as_string( indices, playlist );
    }
    
    debugf( "\nfilename:\t%s\ntotal:\t\t%d\nindices:\t%s\ncurrent index:\t%d\n\n",
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
    
    /*debug( "get_indices_as_string()\n" );  */

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


