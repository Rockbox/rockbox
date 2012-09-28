/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Antoine Cellerier <dionoea -at- videolan -dot- org>
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

#include "plugin.h"
#include "lib/md5.h"



#define BUFFERSIZE 16384

static int count = 0;
static int done = 0;
static bool quit = false;

static int hash( char *string, const char *path )
{
    static char buffer[BUFFERSIZE];
    ssize_t len;
    struct md5_s md5;
    int in = open( path, O_RDONLY );
    if( in < 0 ) return -1;

    InitMD5( &md5 );
    while( !quit && ( len = read( in, buffer, sizeof(buffer) ) ) > 0 )
    {
        AddMD5( &md5, buffer, len );
        
        if( get_action(CONTEXT_STD, TIMEOUT_NOBLOCK) == ACTION_STD_CANCEL )
            quit = true;
    }
    
    EndMD5( &md5 );

    psz_md5_hash( string, &md5 );

    close( in );
    return 0;
}

static void hash_file( int out, const char *path )
{
    if( out < 0 )
        count++;
    else
    {
        char string[MD5_STRING_LENGTH+1];
        int status;
        done++;
        splashf( 0, "%d / %d : %s", done, count, path );
        status = hash( string, path );
        
        if( quit )
            return;
        
        if( status )
            write( out, "error", 5 );
        else
            write( out, string, MD5_STRING_LENGTH );
        write( out, "  ", 2 );
        write( out, path, strlen( path ) );
        write( out, "\n", 1 );
        
        yield();
    }
}

static void hash_dir( int out, const char *path )
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir( path );
    if( dir )
    {
        while( !quit && ( entry = readdir( dir ) ) )
        {
            char childpath[MAX_PATH];
            snprintf( childpath, MAX_PATH, "%s/%s",
                          strcmp( path, "/" ) ? path : "", entry->d_name );
            
            struct dirinfo info = dir_get_info(dir, entry);
            if (info.attribute & ATTR_DIRECTORY)
            {
                if( strcmp( entry->d_name, "." )
                    && strcmp( entry->d_name, ".." ) )
                {
                    /* Got a sub directory */
                    hash_dir( out, childpath );
                }
            }
            else
            {
                /* Got a file */
                hash_file( out, childpath );
            }
        }
        closedir( dir );
    }
}

static void hash_list( int out, const char *path )
{
    int list = open( path, O_RDONLY );
    char newpath[MAX_PATH];
    if( list < 0 ) return;

    while( !quit && read_line( list, newpath, MAX_PATH ) > 0 )
    {
        DIR *dir = opendir( newpath );
        if( dir )
        {
            closedir( dir );
            hash_dir( out, newpath );
        }
        else
        {
            hash_file( out, newpath );
        }
    }

    close( list );
}

static void hash_check( int out, const char *path )
{
    int list = open( path, O_RDONLY );
    char line[MD5_STRING_LENGTH+1+MAX_PATH+1];
    int len;
    if( list < 0 ) return;

    while( !quit && ( len = read_line( list, line, MD5_STRING_LENGTH+1+MAX_PATH+1 ) ) > 0 )
    {
        if( out < 0 )
            count++;
        else
        {
            const char *filename = strchr( line, ' ' );
            done++;
            splashf( 0, "%d / %d : %s", done, count, filename );
            if( !filename || len < MD5_STRING_LENGTH + 2 )
            {
                const char error[] = "Malformed input line ... skipping";
                write( out, error, strlen( error ) );
            }
            else
            {
                char string[MD5_STRING_LENGTH+1];
                while( *filename == ' ' )
                    filename++;
                write( out, filename, strlen( filename ) );
                write( out, ": ", 2 );
                if( hash( string, filename ) )
                    write( out, "FAILED open or read", 19 );
                else if( strncasecmp( line, string, MD5_STRING_LENGTH ) )
                    write( out, "FAILED", 6 );
                else
                    write( out, "OK", 2 );
            }
            write( out, "\n", 1 );
        }
    }

    close( list );
}

enum plugin_status plugin_start(const void* parameter)
{
    const char *arg = (const char *)parameter; /* input file name, if any */
    int out = -1; /* output file descriptor */
    char filename[MAX_PATH]; /* output file name */

    void (*action)( int, const char * ) = NULL;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost( true );
#endif

    if( arg && *arg )
    {
        const char *ext = strrchr( arg, '.' );
        DIR *dir;
        snprintf( filename, MAX_PATH, "%s.md5sum", arg );

        if( ext )
        {
            if( !strcmp( ext, ".md5" ) || !strcmp( ext, ".md5sum" ) )
            {
                snprintf( filename + ( ext - arg ),
                              MAX_PATH + strlen( ext ) - strlen( arg ),
                              ".md5check" );
                /* Lets check the sums */
                action = hash_check;
            }
            else if( !strcmp( ext, ".md5list" ) ) /* ugly */
            {
                /* Hash listed files */
                action = hash_list;
            }
        }

        if( !action )
        {
            dir = opendir( arg );
            if( dir )
            {
                closedir( dir );

                /* Hash the directory's content recursively */
                action = hash_dir;
            }
            else
            {
                /* Hash the file */
                action = hash_file;
            }
        }
    }
    else
    {
        snprintf( filename, MAX_PATH, "/everything.md5sum" );
        /* Hash the whole filesystem */
        action = hash_dir;
        arg = "/";
    }

    lcd_puts( 0, 1, "Output file:" );
    lcd_puts( 0, 2, filename );

    count = 0;
    done = 0;
    action( out, arg );

    out = open( filename, O_WRONLY|O_CREAT|O_TRUNC , 0666);
    if( out < 0 ) return PLUGIN_ERROR;
    action( out, arg );
    close( out );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost( false );
#endif
    return PLUGIN_OK;
}
