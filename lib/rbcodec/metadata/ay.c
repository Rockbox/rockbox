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

/* Taken from blargg's Game_Music_Emu library */

typedef unsigned char byte;

/* AY file header */
enum { header_size = 0x14 };
struct header_t
{
    byte tag[8];
    byte vers;
    byte player;
    byte unused[2];
    byte author[2];
    byte comment[2];
    byte max_track;
    byte first_track;
    byte track_info[2];
};

struct file_t {
    struct header_t const* header;
    byte const* tracks;
    byte const* end;    /* end of file data */
};

static int get_be16( const void *a )
{
    return get_short_be( (void*) a );
}

/* Given pointer to 2-byte offset of data, returns pointer to data, or NULL if
 * offset is 0 or there is less than min_size bytes of data available. */
static byte const* get_data( struct file_t const* file, byte const ptr [], int min_size )
{
    int offset = (int16_t) get_be16( ptr );
    int pos  = ptr - (byte const*) file->header;
    int size = file->end - (byte const*) file->header;
    int limit = size - min_size;
    if ( limit < 0 || !offset || (unsigned) (pos + offset) > (unsigned) limit )
        return NULL;
    return ptr + offset;
}

static const char *parse_header( byte const in [], int size, struct file_t* out )
{
    if ( size < header_size )
        return "wrong file type";

    out->header = (struct header_t const*) in;
    out->end    = in + size;
    struct header_t const* h = (struct header_t const*) in;
    if ( memcmp( h->tag, "ZXAYEMUL", 8 ) )
        return "wrong file type";

    out->tracks = get_data( out, h->track_info, (h->max_track + 1) * 4 );
    if ( !out->tracks )
        return "missing track data";

    return 0;
}

static void copy_ay_fields( struct file_t const* file, struct mp3entry* id3, int track )
{
    int track_count = file->header->max_track + 1;

    /* calculate track length based on number of subtracks */
    if (track_count > 1) {
        id3->length = file->header->max_track * 1000;
    } else {
        byte const* track_info = get_data( file, file->tracks + track * 4 + 2, 6 );
        if (track_info)
            id3->length = get_be16( track_info + 4 ) * (1000 / 50); /* frames to msec */
        else id3->length = 120 * 1000;
    }
    
    if ( id3->length <= 0 )
        id3->length = 120 * 1000;  /* 2 minutes */ 

    /* If meta info was found in the m3u skip next step */
    if (id3->title && id3->title[0]) return;

    /* If file has more than one track will
        use file name as title */
    char * tmp;
    if (track_count <= 1) {
        tmp = (char *) get_data( file, file->tracks + track * 4, 1 );
        if ( tmp ) id3->title = tmp;
    }

    /* Author */
    tmp = (char *) get_data( file, file->header->author, 1 );
    if (tmp) id3->artist = tmp;
    
    /* Comment */
    tmp = (char *) get_data( file, file->header->comment, 1 );
    if (tmp) id3->comment = tmp;
}

static bool parse_ay_header(int fd, struct mp3entry *id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->id3v2buf;
    struct file_t file;
    int read_bytes;

    lseek(fd, 0, SEEK_SET);
    if ((read_bytes = read(fd, buf, ID3V2_BUF_SIZE)) < header_size)
        return false;

    buf [ID3V2_BUF_SIZE] = '\0';
    if ( parse_header( buf, read_bytes, &file ) ) 
        return false;

    copy_ay_fields( &file, id3, 0 );
    return true;
}

bool get_ay_metadata(int fd, struct mp3entry* id3)
{
    char ay_type[8];
    if ((lseek(fd, 0, SEEK_SET) < 0) ||
         read(fd, ay_type, 8) < 8)
        return false;

    id3->vbr = false;
    id3->filesize = filesize(fd);

    id3->bitrate = 706;
    id3->frequency = 44100;
  
    /* Make sure this is a ZX Ay file */
    if (memcmp( ay_type, "ZXAYEMUL", 8 ) != 0)
        return false;

    return parse_ay_header(fd, id3);
}
