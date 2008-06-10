/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2008 Antoine Cellerier <dionoea -at- videolan -dot- org>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "lib/md5.h"

PLUGIN_HEADER

static const struct plugin_api *rb;

int hash( char *string, const char *path )
{
    char *buffer[512];
    ssize_t len;
    struct md5_s md5;
    int in = rb->open( path, O_RDONLY );
    if( in < 0 ) return -1;

    InitMD5( &md5 );
    while( ( len = rb->read( in, buffer, 512 ) ) > 0 )
        AddMD5( &md5, buffer, len );
    EndMD5( &md5 );

    psz_md5_hash( string, &md5 );

    rb->close( in );
    return 0;
}

void hash_file( int out, const char *path )
{
    char string[MD5_STRING_LENGTH+1];
    if( hash( string, path ) )
        rb->write( out, "error", 5 );
    else
        rb->write( out, string, MD5_STRING_LENGTH );
    rb->write( out, " ", 1 );
    rb->write( out, path, rb->strlen( path ) );
    rb->write( out, "\n", 1 );
}

void hash_dir( int out, const char *path );
void hash_dir( int out, const char *path )
{
    DIR *dir;
    struct dirent *entry;

    dir = rb->opendir( path );
    if( dir )
    {
        while( ( entry = rb->readdir( dir ) ) )
        {
            char childpath[MAX_PATH];
            rb->snprintf( childpath, MAX_PATH, "%s/%s",
                          path, entry->d_name );
            if( entry->attribute & ATTR_DIRECTORY )
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

void hash_list( int out, const char *path )
{
    int list = rb->open( path, O_RDONLY );
    char newpath[MAX_PATH];
    if( list < 0 ) return;

    while( rb->read_line( list, newpath, MAX_PATH ) > 0 )
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

void hash_check( int out, const char *path )
{
    int list = rb->open( path, O_RDONLY );
    char line[MD5_STRING_LENGTH+1+MAX_PATH+1];
    int len;
    if( list < 0 ) return;

    while( ( len = rb->read_line( list, line, MD5_STRING_LENGTH+1+MAX_PATH+1 ) ) > 0 )
    {
        const char *filename = rb->strchr( line, ' ' );
        if( !filename || len < MD5_STRING_LENGTH + 2 )
        {
            const char error[] = "Malformed input line ... skipping";
            rb->write( out, error, rb->strlen( error ) );
        }
        else
        {
            char string[MD5_STRING_LENGTH+1];
            filename++;
            rb->write( out, filename, rb->strlen( filename ) );
            rb->write( out, ": ", 2 );
            if( hash( string, filename ) )
                rb->write( out, "FAILED open or read", 19 );
            else if( rb->memcmp( line, string, MD5_STRING_LENGTH ) )
                rb->write( out, "FAILED", 6 );
            else
                rb->write( out, "OK", 2 );
        }
        rb->write( out, "\n", 1 );
    }
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    const char *arg = (const char *)parameter; /* input file name, if any */
    int out = -1; /* output file descriptor */
    char filename[MAX_PATH]; /* output file name */

    md5_init( api );
    rb = api;

    if( arg && *arg )
    {
        const char *ext = rb->strrchr( arg, '.' );
        DIR *dir;
        rb->snprintf( filename, MAX_PATH, "%s.md5sum", arg );
        out = rb->open( filename, O_WRONLY|O_CREAT );
        if( out < 0 ) return PLUGIN_ERROR;

        if( ext )
        {
            if( !rb->strcmp( ext, ".md5" ) || !rb->strcmp( ext, ".md5sum" ) )
            {
                /* Lets check the sums */
                hash_check( out, arg );
                goto exit;
            }
            else if( !rb->strcmp( ext, ".md5list" ) ) /* ugly */
            {
                /* Hash listed files */
                hash_list( out, arg );
                goto exit;
            }
        }

        dir = rb->opendir( arg );
        if( dir )
        {
            api->closedir( dir );

            /* Hash the directory's content recursively */
            hash_dir( out, arg );
        }
        else
        {
            /* Hash the file */
            hash_file( out, arg );
        }
    }
    else
    {
        rb->snprintf( filename, MAX_PATH, "/everything.md5sum" );
        out = rb->open( filename, O_WRONLY|O_CREAT );
        if( out < 0 ) return PLUGIN_ERROR;

        /* Hash the whole filesystem */
        hash_dir( out, "/" );
    }

    exit:
        rb->close( out );
        return PLUGIN_OK;
}
