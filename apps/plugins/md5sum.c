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
    int in = rb->open( path, O_RDONLY );
    if( in < 0 ) return -1;

    InitMD5( &md5 );
    while( !quit && ( len = rb->read( in, buffer, sizeof(buffer) ) ) > 0 )
    {
        AddMD5( &md5, buffer, len );

        if( rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK) == ACTION_STD_CANCEL )
            quit = true;
    }

    EndMD5( &md5 );

    psz_md5_hash( string, &md5 );

    rb->close( in );
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
        rb->splashf( 0, "%d / %d : %s", done, count, path );
        status = hash( string, path );

        if( quit )
            return;

        if( status )
            rb->write( out, "error", 5 );
        else
            rb->write( out, string, MD5_STRING_LENGTH );
        rb->write( out, "  ", 2 );
        rb->write( out, path, rb->strlen( path ) );
        rb->write( out, "\n", 1 );

        rb->yield();
    }
}

static void hash_dir( int out, const char *path )
{
    DIR *dir;
    struct dirent *entry;

    dir = rb->opendir( path );
    if( dir )
    {
        while( !quit && ( entry = rb->readdir( dir ) ) )
        {
            char childpath[MAX_PATH];
            rb->snprintf( childpath, MAX_PATH, "%s/%s",
                          rb->strcmp( path, "/" ) ? path : "", entry->d_name );

            struct dirinfo info = rb->dir_get_info(dir, entry);
            if (info.attribute & ATTR_DIRECTORY)
            {
                if( rb->strcmp( entry->d_name, "." )
                    && rb->strcmp( entry->d_name, ".." ) )
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
        rb->closedir( dir );
    }
}

static void hash_list( int out, const char *path )
{
    int list = rb->open( path, O_RDONLY );
    char newpath[MAX_PATH];
    if( list < 0 ) return;

    while( !quit && rb->read_line( list, newpath, MAX_PATH ) > 0 )
    {
        DIR *dir = rb->opendir( newpath );
        if( dir )
        {
            rb->closedir( dir );
            hash_dir( out, newpath );
        }
        else
        {
            hash_file( out, newpath );
        }
    }

    rb->close( list );
}

static void hash_check( int out, const char *path )
{
    int list = rb->open( path, O_RDONLY );
    char line[MD5_STRING_LENGTH+1+MAX_PATH+1];
    int len;
    if( list < 0 ) return;

    while( !quit && ( len = rb->read_line( list, line, MD5_STRING_LENGTH+1+MAX_PATH+1 ) ) > 0 )
    {
        if( out < 0 )
            count++;
        else
        {
            const char *filename = rb->strchr( line, ' ' );
            done++;
            rb->splashf( 0, "%d / %d : %s", done, count, filename );
            if( !filename || len < MD5_STRING_LENGTH + 2 )
            {
                const char error[] = "Malformed input line ... skipping";
                rb->write( out, error, rb->strlen( error ) );
            }
            else
            {
                char string[MD5_STRING_LENGTH+1];
                while( *filename == ' ' )
                    filename++;
                rb->write( out, filename, rb->strlen( filename ) );
                rb->write( out, ": ", 2 );
                if( hash( string, filename ) )
                    rb->write( out, "FAILED open or read", 19 );
                else if( rb->strncasecmp( line, string, MD5_STRING_LENGTH ) )
                    rb->write( out, "FAILED", 6 );
                else
                    rb->write( out, "OK", 2 );
            }
            rb->write( out, "\n", 1 );
        }
    }

    rb->close( list );
}

/*
 * Return the last name from a pathname (ignoring a trailing slash if
 * it exists). The returned pointer points to a statically allocated
 * buffer.
 */
static char *get_basename(const char *path) {
    static char temp[MAX_PATH];
    char *p;
    int len, isdir = 0;

    rb->strcpy(temp, path);

    len = rb->strlen(temp);

    if (temp[len - 1] == '/')
    {
        /* strip trailing slash, and update length accordingly */
        temp[--len] = '\0';
        isdir = 1;
    }

    /* find the last slash, if there is one */
    p = rb->strrchr(temp, '/');

    /*
     * re-append trailing slash if we previously removed it (the
     * original NUL is still present)
     */
    if(isdir)
        temp[len++] = '/';

    return p ? (p + 1) : temp;
}

enum plugin_status plugin_start(const void* parameter)
{
    const char *arg = (const char *)parameter; /* input file path, if any */
    char *basename;
    int out = -1; /* output file descriptor */
    char filename[MAX_PATH]; /* output file name */

    void (*action)( int, const char * ) = NULL;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost( true );
#endif

    if( arg && *arg )
    {
        const char *ext = rb->strrchr( arg, '.' );
        DIR *dir;
        rb->snprintf( filename, MAX_PATH, "%s.md5sum", arg );

        if( ext )
        {
            if( !rb->strcmp( ext, ".md5" ) || !rb->strcmp( ext, ".md5sum" ) )
            {
                rb->snprintf( filename + ( ext - arg ),
                              MAX_PATH + rb->strlen( ext ) - rb->strlen( arg ),
                              ".md5check" );
                /* Lets check the sums */
                action = hash_check;
            }
            else if( !rb->strcmp( ext, ".md5list" ) ) /* ugly */
            {
                /* Hash listed files */
                action = hash_list;
            }
        }

        if( !action )
        {
            dir = rb->opendir( arg );
            if( dir )
            {
                rb->closedir( dir );

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
        rb->snprintf( filename, MAX_PATH, "/everything.md5sum" );
        /* Hash the whole filesystem */
        action = hash_dir;
        arg = "/";
    }

    basename = get_basename(arg);

    rb->lcd_putsf( 0, 1, "Hashing %s", basename );
    rb->lcd_puts( 0, 2, rb->str(LANG_ACTION_STD_CANCEL) );

    rb->lcd_puts( 0, 3, "Output file:" );
    rb->lcd_puts( 0, 4, filename );

    rb->lcd_update();
    count = 0;
    done = 0;
    action( out, arg );

    out = rb->open( filename, O_WRONLY|O_CREAT|O_TRUNC , 0666);
    if( out < 0 ) return PLUGIN_ERROR;
    action( out, arg );
    rb->close( out );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost( false );
#endif
    return PLUGIN_OK;
}
