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
#include "disk.h"
#include "debug.h"
#include "panic.h"

void read_file_into_buffer( char **buf, const char *filename )
{
    int i;
    FILE *fp;
    int count = 0;
   
    /*DEBUG( "read_file_into_buffer( %s, %s )\n", *buf, filename ); */
    
    fp = fopen( filename, "r" );

    if( fp == NULL )
    {
        panicf( "failed to open file: %s\n", filename );
    }
    
    while( ( i = getc( fp ) ) != EOF )
    {
        /*printf( "%d-'%c'\n", count, i ); */
        count++;
        *buf = (char *)realloc( *buf, count * sizeof( char ) );
        /*printf( "%d='%s'\n", *buf, *buf ); */
        (*(buf))[count - 1] = (char)i;
        /*printf( "%d='%s'\n", *buf, *buf );*/
    }

    /* add null terminator */
    
    *buf = (char *)realloc( *buf, count * sizeof( char ) );
    (*(buf))[ count ] = '\0';

    /* count -2 because of 0 start and \0 terminator 
       printf( "read %d bytes: '%s'\n", count - 2, *buf ); */
}

/*
 * stub versions of pre-fat sector storage functions
 */

int persist_volume_setting( void )
{
    return 1;
}

int persist_balance_setting( void )
{
    return 1;
}

int persist_bass_setting( void )
{
    return 1;
}

int persist_treble_setting( void )
{
    return 1;
}

int persist_loudness_setting( void )
{
    return 1;
}

int persist_bass_boost_setting( void )
{
    return 1;
}

int persist_contrast_setting( void )
{
    return 1;
}

int persist_poweroff_setting( void )
{
    return 1;
}

int persist_backlight_setting( void )
{
    return 1;
}

int persist_resume_setting( void )
{
    return 1;
}

int persist_playlist_filename( void )
{
    return 1;
}

int persist_playlist_indices( void )
{
    return 1;
}

int persist_playlist_index( void )
{
    return 1;
}

int persist_resume_track_time( void )
{
    return 1;
}
