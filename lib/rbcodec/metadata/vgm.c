#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "platform.h"

#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"

/* Ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */

typedef unsigned char byte;

enum { header_size = 0x40 };
enum { max_field = 64 };

struct header_t
{
    char tag [4]; 
    byte data_size [4];
    byte version [4];
    byte psg_rate [4];
    byte ym2413_rate [4];
    byte gd3_offset [4];
    byte track_duration [4];
    byte loop_offset [4];
    byte loop_duration [4];
    byte frame_rate [4];
    byte noise_feedback [2];
    byte noise_width;
    byte unused1;
    byte ym2612_rate [4];
    byte ym2151_rate [4];
    byte data_offset [4];
    byte unused2 [8];
};

static byte const* skip_gd3_str( byte const* in, byte const* end )
{
    while ( end - in >= 2 )
    {
        in += 2;
        if ( !(in [-2] | in [-1]) )
            break;
    }
    return in;
}

static byte const* get_gd3_str( byte const* in, byte const* end, char* field )
{
    byte const* mid = skip_gd3_str( in, end );
    int len = (mid - in) / 2 - 1;
    if ( field && len > 0 )
    {
        len = len < (int) max_field ? len : (int) max_field;

        field [len] = 0;
        /* Conver to utf8 */
        utf16LEdecode( in, field, len );
        
        /* Copy string back to id3v2buf */
        strcpy( (char*) in, field );
    }
    return mid;
}

static byte const* get_gd3_pair( byte const* in, byte const* end, char* field )
{
    return skip_gd3_str( get_gd3_str( in, end, field ), end );
}

static void parse_gd3( byte const* in, byte const* end, struct mp3entry* id3 )
{
    char* p = id3->path;
    id3->title = (char *) in;
    in = get_gd3_pair( in, end, p ); /* Song */

    id3->album = (char *) in;
    in = get_gd3_pair( in, end, p ); /* Game */

    in = get_gd3_pair( in, end, NULL ); /* System */

    id3->artist = (char *) in;
    in = get_gd3_pair( in, end, p ); /* Author */

#if MEMORYSIZE > 2
    in = get_gd3_str ( in, end, NULL  ); /* Copyright */
    in = get_gd3_pair( in, end, NULL ); /* Dumper */

    id3->comment = (char *) in;
    in = get_gd3_str ( in, end, p ); /* Comment */
#endif
}

int const gd3_header_size = 12;

static long check_gd3_header( byte* h, long remain )
{
    if ( remain < gd3_header_size ) return 0;
    if ( memcmp( h, "Gd3 ", 4 ) ) return 0;
    if ( get_long_le( h + 4 ) >= 0x200 ) return 0;

    long gd3_size = get_long_le( h + 8 );
    if ( gd3_size > remain - gd3_header_size )
        gd3_size = remain - gd3_header_size;

    return gd3_size;
}

static void get_vgm_length( struct header_t* h, struct mp3entry* id3 )
{
    long length = get_long_le( h->track_duration ) * 10 / 441;
    if ( length > 0 )
    {
        long loop_length = 0, intro_length = 0;
        long loop = get_long_le( h->loop_duration );
        if ( loop > 0 && get_long_le( h->loop_offset ) )
        {
            loop_length = loop * 10 / 441;
            intro_length = length - loop_length;
        }
        else
        {
            intro_length = length; /* make it clear that track is no longer than length */
            loop_length = 0;
        }
        
         id3->length = intro_length + 2 * loop_length; /* intro + 2 loops */
         return;
    }

    id3->length = 150 * 1000; /* 2.5 minutes */
}

bool get_vgm_metadata(int fd, struct mp3entry* id3)
{
    /* Use the id3v2 part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->id3v2buf;
    int read_bytes;

    memset(buf, 0, ID3V2_BUF_SIZE);
    if ((lseek(fd, 0, SEEK_SET) < 0) 
         || ((read_bytes = read(fd, buf, header_size)) < header_size))
    {
        return false;
    }

    id3->vbr = false;
    id3->filesize = filesize(fd);

    id3->bitrate = 706;
    id3->frequency = 44100;

    /* If file is gzipped, will get metadata later */
    if (memcmp(buf, "Vgm ", 4))
    {
        /* We must set a default song length here because
            the codec can't do it anymore */
        id3->length = 150 * 1000; /* 2.5 minutes */
        return true;
    }

    /* Get song length from header */
    struct header_t* header = (struct header_t*) buf;
    get_vgm_length( header, id3 );

    long gd3_offset = get_long_le( header->gd3_offset ) - 0x2C;
    
    /* No gd3 tag found */
    if ( gd3_offset < 0 )
        return true;

    /*  Seek to gd3 offset and read as 
         many bytes posible */
    gd3_offset = id3->filesize - (header_size + gd3_offset);
    if ((lseek(fd, -gd3_offset, SEEK_END) < 0) 
         || ((read_bytes = read(fd, buf, ID3V2_BUF_SIZE)) <= 0))
        return true;

    byte* gd3 = buf;
    long gd3_size = check_gd3_header( gd3, read_bytes );

    /* GD3 tag is zero */
    if ( gd3_size == 0 )
        return true;

    /* Finally, parse gd3 tag */
    if ( gd3 )
        parse_gd3( gd3 + gd3_header_size, gd3 + read_bytes, id3 );

    return true;
}
