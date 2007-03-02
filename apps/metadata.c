/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "errno.h"
#include "metadata.h"
#include "mp3_playback.h"
#include "logf.h"
#include "rbunicode.h"
#include "atoi.h"
#include "replaygain.h"
#include "debug.h"
#include "system.h"
#include "cuesheet.h"
#include "structec.h"

enum tagtype { TAGTYPE_APE = 1, TAGTYPE_VORBIS };

#ifdef ROCKBOX_BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

#define APETAG_HEADER_LENGTH        32
#define APETAG_HEADER_FORMAT        "8llll8"
#define APETAG_ITEM_HEADER_FORMAT   "ll"
#define APETAG_ITEM_TYPE_MASK       3

#define TAG_NAME_LENGTH             32
#define TAG_VALUE_LENGTH            128

#define MP4_ID(a, b, c, d)  (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define MP4_3gp6 MP4_ID('3', 'g', 'p', '6')
#define MP4_alac MP4_ID('a', 'l', 'a', 'c')
#define MP4_calb MP4_ID(0xa9, 'a', 'l', 'b')
#define MP4_cART MP4_ID(0xa9, 'A', 'R', 'T')
#define MP4_cnam MP4_ID(0xa9, 'n', 'a', 'm')
#define MP4_cwrt MP4_ID(0xa9, 'w', 'r', 't')
#define MP4_esds MP4_ID('e', 's', 'd', 's')
#define MP4_ftyp MP4_ID('f', 't', 'y', 'p')
#define MP4_gnre MP4_ID('g', 'n', 'r', 'e')
#define MP4_hdlr MP4_ID('h', 'd', 'l', 'r')
#define MP4_ilst MP4_ID('i', 'l', 's', 't')
#define MP4_M4A  MP4_ID('M', '4', 'A', ' ')
#define MP4_M4B  MP4_ID('M', '4', 'B', ' ')
#define MP4_mdat MP4_ID('m', 'd', 'a', 't')
#define MP4_mdia MP4_ID('m', 'd', 'i', 'a')
#define MP4_mdir MP4_ID('m', 'd', 'i', 'r')
#define MP4_meta MP4_ID('m', 'e', 't', 'a')
#define MP4_minf MP4_ID('m', 'i', 'n', 'f')
#define MP4_moov MP4_ID('m', 'o', 'o', 'v')
#define MP4_mp4a MP4_ID('m', 'p', '4', 'a')
#define MP4_mp42 MP4_ID('m', 'p', '4', '2')
#define MP4_qt   MP4_ID('q', 't', ' ', ' ')
#define MP4_soun MP4_ID('s', 'o', 'u', 'n')
#define MP4_stbl MP4_ID('s', 't', 'b', 'l')
#define MP4_stsd MP4_ID('s', 't', 's', 'd')
#define MP4_stts MP4_ID('s', 't', 't', 's')
#define MP4_trak MP4_ID('t', 'r', 'a', 'k')
#define MP4_trkn MP4_ID('t', 'r', 'k', 'n')
#define MP4_udta MP4_ID('u', 'd', 't', 'a')
#define MP4_extra MP4_ID('-', '-', '-', '-')

struct apetag_header 
{
    char id[8];
    long version;
    long length;
    long item_count;
    long flags;
    char reserved[8];
};

struct apetag_item_header 
{
    long length;
    long flags;
};

#if CONFIG_CODEC == SWCODEC
static const unsigned short a52_bitrates[] =
{
     32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 
    192, 224, 256, 320, 384, 448, 512, 576, 640
};

/* Only store frame sizes for 44.1KHz - others are simply multiples 
   of the bitrate */
static const unsigned short a52_441framesizes[] =
{
      69 * 2,   70 * 2,   87 * 2,   88 * 2,  104 * 2,  105 * 2,  121 * 2, 
     122 * 2,  139 * 2,  140 * 2,  174 * 2,  175 * 2,  208 * 2,  209 * 2,
     243 * 2,  244 * 2,  278 * 2,  279 * 2,  348 * 2,  349 * 2,  417 * 2,
     418 * 2,  487 * 2,  488 * 2,  557 * 2,  558 * 2,  696 * 2,  697 * 2,  
     835 * 2,  836 * 2,  975 * 2,  976 * 2, 1114 * 2, 1115 * 2, 1253 * 2, 
    1254 * 2, 1393 * 2, 1394 * 2
};

static const long wavpack_sample_rates [] = 
{
     6000,  8000,  9600, 11025, 12000, 16000,  22050, 24000, 
    32000, 44100, 48000, 64000, 88200, 96000, 192000 
};

/* Read a string from the file. Read up to size bytes, or, if eos != -1, 
 * until the eos character is found (eos is not stored in buf, unless it is
 * nil). Writes up to buf_size chars to buf, always terminating with a nil.
 * Returns number of chars read or -1 on read error.
 */
static long read_string(int fd, char* buf, long buf_size, int eos, long size)
{
    long read_bytes = 0;
    char c;
    
    while (size != 0)
    {
        if (read(fd, &c, 1) != 1)
        {
            read_bytes = -1;
            break;
        }
        
        read_bytes++;
        size--;
        
        if ((eos != -1) && (eos == (unsigned char) c))
        {
            break;
        }
        
        if (buf_size > 1)
        {
            *buf++ = c;
            buf_size--;
        }
    }
    
    *buf = 0;
    return read_bytes;
}

/* Read an unsigned 32-bit integer from a big-endian file. */
#ifdef ROCKBOX_BIG_ENDIAN
#define read_uint32be(fd,buf) read((fd), (buf), 4)
#else
static int read_uint32be(int fd, unsigned int* buf)
{
  size_t n;

  n = read(fd, (char*) buf, 4);
  *buf = betoh32(*buf);
  return n;
}
#endif

/* Read an unaligned 32-bit little endian long from buffer. */
static unsigned long get_long_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Read an unaligned 32-bit big endian long from buffer. */
static unsigned long get_long_be(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* Read an unaligned 32-bit little endian long from buffer. */
static long get_slong(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}
 
/* Parse the tag (the name-value pair) and fill id3 and buffer accordingly.
 * String values to keep are written to buf. Returns number of bytes written
 * to buf (including end nil).
 */
static long parse_tag(const char* name, char* value, struct mp3entry* id3,
    char* buf, long buf_remaining, enum tagtype type)
{
    long len = 0;
    char** p;

    if ((((strcasecmp(name, "track") == 0) && (type == TAGTYPE_APE)))
        || ((strcasecmp(name, "tracknumber") == 0) && (type == TAGTYPE_VORBIS)))
    {
        id3->tracknum = atoi(value);
        p = &(id3->track_string);
    }
    else if (((strcasecmp(name, "year") == 0) && (type == TAGTYPE_APE))
        || ((strcasecmp(name, "date") == 0) && (type == TAGTYPE_VORBIS)))
    {
        /* Date can be in more any format in a Vorbis tag, so don't try to 
         * parse it. 
         */
        if (type != TAGTYPE_VORBIS)
        {
            id3->year = atoi(value);
        }
        
        p = &(id3->year_string);
    }
    else if (strcasecmp(name, "title") == 0)
    {
        p = &(id3->title);
    }
    else if (strcasecmp(name, "artist") == 0)
    {
        p = &(id3->artist);
    }
    else if (strcasecmp(name, "album") == 0)
    {
        p = &(id3->album);
    }
    else if (strcasecmp(name, "genre") == 0)
    {
        p = &(id3->genre_string);
    }
    else if (strcasecmp(name, "composer") == 0)
    {
        p = &(id3->composer);
    }
    else if (strcasecmp(name, "comment") == 0)
    {
        p = &(id3->comment);
    }
    else if (strcasecmp(name, "albumartist") == 0)
    {
        p = &(id3->albumartist);
    }
    else if (strcasecmp(name, "album artist") == 0)
    {
        p = &(id3->albumartist);
    }
    else if (strcasecmp(name, "ensemble") == 0)
    {
        p = &(id3->albumartist);
    }
    else
    {
        len = parse_replaygain(name, value, id3, buf, buf_remaining);
        p = NULL;
    }
    
    if (p)
    {
        len = strlen(value);
        len = MIN(len, buf_remaining - 1);

        if (len > 0)
        {
            strncpy(buf, value, len);
            buf[len] = 0;
            *p = buf;
            len++;
        }
        else
        {
            len = 0;
        }
    }
    
    return len;
}

/* Read the items in an APEV2 tag. Only looks for a tag at the end of a 
 * file. Returns true if a tag was found and fully read, false otherwise.
 */
static bool read_ape_tags(int fd, struct mp3entry* id3)
{
    struct apetag_header header;

    if ((lseek(fd, -APETAG_HEADER_LENGTH, SEEK_END) < 0)
        || (ecread(fd, &header, 1, APETAG_HEADER_FORMAT, IS_BIG_ENDIAN) != APETAG_HEADER_LENGTH)
        || (memcmp(header.id, "APETAGEX", sizeof(header.id))))
    {
        return false;
    }

    if ((header.version == 2000) && (header.item_count > 0)
        && (header.length > APETAG_HEADER_LENGTH)) 
    {
        char *buf = id3->id3v2buf;
        unsigned int buf_remaining = sizeof(id3->id3v2buf) 
            + sizeof(id3->id3v1buf);
        unsigned int tag_remaining = header.length - APETAG_HEADER_LENGTH;
        int i;
        
        if (lseek(fd, -header.length, SEEK_END) < 0)
        {
            return false;
        }

        for (i = 0; i < header.item_count; i++)
        {
            struct apetag_item_header item;
            char name[TAG_NAME_LENGTH];
            char value[TAG_VALUE_LENGTH];
            long r;

            if (tag_remaining < sizeof(item))
            {
                break;
            }
            
            if (ecread(fd, &item, 1, APETAG_ITEM_HEADER_FORMAT, IS_BIG_ENDIAN) < (long) sizeof(item))
            {
                return false;
            }
            
            tag_remaining -= sizeof(item);
            r = read_string(fd, name, sizeof(name), 0, tag_remaining);
            
            if (r == -1)
            {
                return false;
            }

            tag_remaining -= r + item.length;

            if ((item.flags & APETAG_ITEM_TYPE_MASK) == 0) 
            {
                long len;
                
                if (read_string(fd, value, sizeof(value), -1, item.length) 
                    != item.length)
                {
                    return false;
                }

                len = parse_tag(name, value, id3, buf, buf_remaining, 
                    TAGTYPE_APE);
                buf += len;
                buf_remaining -= len;
            }
            else
            {
                if (lseek(fd, item.length, SEEK_CUR) < 0)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

/* Read the items in a Vorbis comment packet. Returns true the items were
 * fully read, false otherwise.
 */
static bool read_vorbis_tags(int fd, struct mp3entry *id3, 
    long tag_remaining)
{
    char *buf = id3->id3v2buf;
    long comment_count;
    long len;
    int buf_remaining = sizeof(id3->id3v2buf) + sizeof(id3->id3v1buf);
    int i;

    if (ecread(fd, &len, 1, "l", IS_BIG_ENDIAN) < (long) sizeof(len)) 
    {
        return false;
    }
    
    if ((lseek(fd, len, SEEK_CUR) < 0)
        || (ecread(fd, &comment_count, 1, "l", IS_BIG_ENDIAN) 
            < (long) sizeof(comment_count)))
    {
        return false;
    }
    
    tag_remaining -= len + sizeof(len) + sizeof(comment_count);

    if (tag_remaining <= 0)
    {
        return true;
    }

    for (i = 0; i < comment_count; i++)
    {
        char name[TAG_NAME_LENGTH];
        char value[TAG_VALUE_LENGTH];
        long read_len;

        if (tag_remaining < 4) 
        {
            break;
        }

        if (ecread(fd, &len, 1, "l", IS_BIG_ENDIAN) < (long) sizeof(len))
        {
            return false;
        }

        tag_remaining -= 4;

        /* Quit if we've passed the end of the page */
        if (tag_remaining < len)
        {
            break;
        }

        tag_remaining -= len;
        read_len = read_string(fd, name, sizeof(name), '=', len);
        
        if (read_len < 0)
        {
            return false;
        }

        len -= read_len;

        if (read_string(fd, value, sizeof(value), -1, len) < 0)
        {
            return false;
        }

        len = parse_tag(name, value, id3, buf, buf_remaining, 
            TAGTYPE_VORBIS);
        buf += len;
        buf_remaining -= len;
    }

    /* Skip to the end of the block */
    if (tag_remaining) 
    {
        if (lseek(fd, tag_remaining, SEEK_CUR) < 0)
        {
            return false;
        }
    }

    return true;
}

/* Skip an ID3v2 tag if it can be found. We assume the tag is located at the
 * start of the file, which should be true in all cases where we need to skip it.
 * Returns true if successfully skipped or not skipped, and false if
 * something went wrong while skipping.
 */
static bool skip_id3v2(int fd, struct mp3entry *id3)
{
    char buf[4];

    read(fd, buf, 4);
    if (memcmp(buf, "ID3", 3) == 0)
    {
        /* We have found an ID3v2 tag at the start of the file - find its
           length and then skip it. */
        if ((id3->first_frame_offset = getid3v2len(fd)) == 0)
            return false;

        if ((lseek(fd, id3->first_frame_offset, SEEK_SET) < 0)) 
            return false;
        
        return true;
    } else {
        lseek(fd, 0, SEEK_SET);
        id3->first_frame_offset = 0;
        return true;
    }
}

/* A simple parser to read vital metadata from an Ogg Speex file. Returns
 * false if metadata needed by the Speex codec couldn't be read.
 */

static bool get_speex_metadata(int fd, struct mp3entry* id3)
{
    /* An Ogg File is split into pages, each starting with the string 
     * "OggS". Each page has a timestamp (in PCM samples) referred to as
     * the "granule position".
     *
     * An Ogg Speex has the following structure:
     * 1) Identification header (containing samplerate, numchannels, etc)
          Described in this page: (http://www.speex.org/manual2/node7.html)
     * 2) Comment header - containing the Vorbis Comments
     * 3) Many audio packets...
     */

    /* Use the path name of the id3 structure as a temporary buffer. */
    unsigned char* buf = (unsigned char*)id3->path;
    long comment_size;
    long remaining = 0;
    long last_serial = 0;
    long serial, r;
    int segments;
    int i;
    bool eof = false;

    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 58) < 33))
    {
        return false;
    }

    if ((memcmp(buf, "OggS", 4) != 0) || (memcmp(&buf[28], "Speex", 5) != 0))
    {
        return false;
    }

    /* We need to ensure the serial number from this page is the same as the
     * one from the last page (since we only support a single bitstream).
     */
    serial = get_long_le(&buf[14]);
    if ((lseek(fd, 33, SEEK_SET) < 0)||(read(fd, buf, 58) < 4))
    {
    return false;
    }

    id3->frequency = get_slong(&buf[31]);
    last_serial = get_long_le(&buf[27]);/*temporary, header size*/
    id3->bitrate = get_long_le(&buf[47]);
    id3->vbr = get_long_le(&buf[55]);
    id3->filesize = filesize(fd);
    /* Comments are in second Ogg page */
    if (lseek(fd, 28+last_serial/*(temporary for header size)*/, SEEK_SET) < 0)
    {
    return false;
    }

    /* Minimum header length for Ogg pages is 27. */
    if (read(fd, buf, 27) < 27) 
    {
    return false;
    }

    if (memcmp(buf, "OggS", 4) !=0 )
    {
    return false;
    }

    segments = buf[26];
    /* read in segment table */
    if (read(fd, buf, segments) < segments) 
    {
    return false;
    }

    /* The second packet in a vorbis stream is the comment packet. It *may*
     * extend beyond the second page, but usually does not. Here we find the
     * length of the comment packet (or the rest of the page if the comment
     * packet extends to the third page).
     */
    for (i = 0; i < segments; i++) 
    {
        remaining += buf[i];
        /* The last segment of a packet is always < 255 bytes */
        if (buf[i] < 255) 
        {
              break;
        }
    }

    comment_size = remaining;
    
    /* Failure to read the tags isn't fatal. */
    read_vorbis_tags(fd, id3, remaining);

    /* We now need to search for the last page in the file - identified by 
     * by ('O','g','g','S',0) and retrieve totalsamples.
     */

    /* A page is always < 64 kB */
    if (lseek(fd, -(MIN(64 * 1024, id3->filesize)), SEEK_END) < 0)
    {
    return false;
    }

    remaining = 0;

    while (!eof) 
    {
        r = read(fd, &buf[remaining], MAX_PATH - remaining);
        
        if (r <= 0) 
        {
            eof = true;
        } 
        else 
        {
            remaining += r;
        }
        
        /* Inefficient (but simple) search */
        i = 0;
        
        while (i < (remaining - 3)) 
        {
            if ((buf[i] == 'O') && (memcmp(&buf[i], "OggS", 4) == 0))
            {
                if (i < (remaining - 17)) 
                {
                    /* Note that this only reads the low 32 bits of a
                     * 64 bit value.
                     */
                     id3->samples = get_long_le(&buf[i + 6]);
                     last_serial = get_long_le(&buf[i + 14]);

                    /* If this page is very small the beginning of the next
                     * header could be in buffer. Jump near end of this header
                     * and continue */
                    i += 27;
                } 
                else 
                {
                    break;
                }
            } 
            else 
            {
                i++;
            }
        }

        if (i < remaining) 
        {
            /* Move the remaining bytes to start of buffer.
             * Reuse var 'segments' as it is no longer needed */
            segments = 0;
            while (i < remaining)
            {
                buf[segments++] = buf[i++];
            }
            remaining = segments;
        }
        else
        {
            /* Discard the rest of the buffer */
            remaining = 0;
        }
    }

    /* This file has mutiple vorbis bitstreams (or is corrupt). */
    /* FIXME we should display an error here. */
    if (serial != last_serial)
    {
        logf("serialno mismatch");
        logf("%ld", serial);
        logf("%ld", last_serial);
    return false;
    }

    id3->length = (id3->samples / id3->frequency) * 1000;
    id3->bitrate = (((int64_t) id3->filesize - comment_size) * 8) / id3->length;
    return true;
}


/* A simple parser to read vital metadata from an Ogg Vorbis file. 
 * Calls get_speex_metadata if a speex file is identified. Returns
 * false if metadata needed by the Vorbis codec couldn't be read.
 */
static bool get_vorbis_metadata(int fd, struct mp3entry* id3)
{
    /* An Ogg File is split into pages, each starting with the string 
     * "OggS". Each page has a timestamp (in PCM samples) referred to as
     * the "granule position".
     *
     * An Ogg Vorbis has the following structure:
     * 1) Identification header (containing samplerate, numchannels, etc)
     * 2) Comment header - containing the Vorbis Comments
     * 3) Setup header - containing codec setup information
     * 4) Many audio packets...
     */

    /* Use the path name of the id3 structure as a temporary buffer. */
    unsigned char* buf = (unsigned char *)id3->path;
    long comment_size;
    long remaining = 0;
    long last_serial = 0;
    long serial, r;
    int segments;
    int i;
    bool eof = false;

    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 58) < 4))
    {
        return false;
    }
    
    if ((memcmp(buf, "OggS", 4) != 0) || (memcmp(&buf[29], "vorbis", 6) != 0))
    {
        if ((memcmp(buf, "OggS", 4) != 0) || (memcmp(&buf[28], "Speex", 5) != 0))
        {        
            return false;
        }
        else
        {
            id3->codectype = AFMT_SPEEX;
            return get_speex_metadata(fd, id3);
        }
    }
    
    /* We need to ensure the serial number from this page is the same as the
     * one from the last page (since we only support a single bitstream).
     */
    serial = get_long_le(&buf[14]);
    id3->frequency = get_long_le(&buf[40]);
    id3->filesize = filesize(fd);

    /* Comments are in second Ogg page */
    if (lseek(fd, 58, SEEK_SET) < 0)
    {
        return false;
    }

    /* Minimum header length for Ogg pages is 27. */
    if (read(fd, buf, 27) < 27) 
    {
        return false;
    }

    if (memcmp(buf, "OggS", 4) !=0 )
    {
        return false;
    }

    segments = buf[26];

    /* read in segment table */
    if (read(fd, buf, segments) < segments) 
    {
        return false;
    }

    /* The second packet in a vorbis stream is the comment packet. It *may*
     * extend beyond the second page, but usually does not. Here we find the
     * length of the comment packet (or the rest of the page if the comment
     * packet extends to the third page).
     */
    for (i = 0; i < segments; i++) 
    {
        remaining += buf[i];
        
        /* The last segment of a packet is always < 255 bytes */
        if (buf[i] < 255) 
        {
              break;
        }
    }

    /* Now read in packet header (type and id string) */
    if (read(fd, buf, 7) < 7) 
    {
        return false;
    }

    comment_size = remaining;
    remaining -= 7;

    /* The first byte of a packet is the packet type; comment packets are
     * type 3.
     */
    if ((buf[0] != 3) || (memcmp(buf + 1, "vorbis", 6) !=0))
    {
        return false;
    }

    /* Failure to read the tags isn't fatal. */
    read_vorbis_tags(fd, id3, remaining);

    /* We now need to search for the last page in the file - identified by 
     * by ('O','g','g','S',0) and retrieve totalsamples.
     */

    /* A page is always < 64 kB */
    if (lseek(fd, -(MIN(64 * 1024, id3->filesize)), SEEK_END) < 0)
    {
        return false;
    }

    remaining = 0;

    while (!eof) 
    {
        r = read(fd, &buf[remaining], MAX_PATH - remaining);
        
        if (r <= 0) 
        {
            eof = true;
        } 
        else 
        {
            remaining += r;
        }
        
        /* Inefficient (but simple) search */
        i = 0;
        
        while (i < (remaining - 3)) 
        {
            if ((buf[i] == 'O') && (memcmp(&buf[i], "OggS", 4) == 0))
            {
                if (i < (remaining - 17)) 
                {
                    /* Note that this only reads the low 32 bits of a
                     * 64 bit value.
                     */
                     id3->samples = get_long_le(&buf[i + 6]);
                     last_serial = get_long_le(&buf[i + 14]);

                    /* If this page is very small the beginning of the next
                     * header could be in buffer. Jump near end of this header
                     * and continue */
                    i += 27;
                } 
                else 
                {
                    break;
                }
            } 
            else 
            {
                i++;
            }
        }

        if (i < remaining) 
        {
            /* Move the remaining bytes to start of buffer.
             * Reuse var 'segments' as it is no longer needed */
            segments = 0;
            while (i < remaining)
            {
                buf[segments++] = buf[i++];
            }
            remaining = segments;
        }
        else
        {
            /* Discard the rest of the buffer */
            remaining = 0;
        }
    }

    /* This file has mutiple vorbis bitstreams (or is corrupt). */
    /* FIXME we should display an error here. */
    if (serial != last_serial)
    {
        logf("serialno mismatch");
        logf("%ld", serial);
        logf("%ld", last_serial);
        return false;
    }

    id3->length = ((int64_t) id3->samples * 1000) / id3->frequency;

    if (id3->length <= 0)
    {
        logf("ogg length invalid!");
        return false;
    }
    
    id3->bitrate = (((int64_t) id3->filesize - comment_size) * 8) / id3->length;
    id3->vbr = true;
    
    return true;
}

static bool get_flac_metadata(int fd, struct mp3entry* id3)
{
    /* A simple parser to read vital metadata from a FLAC file - length,
     * frequency, bitrate etc. This code should either be moved to a 
     * seperate file, or discarded in favour of the libFLAC code.
     * The FLAC stream specification can be found at 
     * http://flac.sourceforge.net/format.html#stream
     */

    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    bool rc = false;

    if (!skip_id3v2(fd, id3) || (read(fd, buf, 4) < 4))
    {
        return rc;
    }
    
    if (memcmp(buf, "fLaC", 4) != 0) 
    {
        return rc;
    }

    while (true) 
    {
        long i;
        
        if (read(fd, buf, 4) < 0)
        {
            return rc;
        }
        
        /* The length of the block */
        i = (buf[1] << 16) | (buf[2] << 8) | buf[3];

        if ((buf[0] & 0x7f) == 0)       /* 0 is the STREAMINFO block */
        {
            unsigned long totalsamples;

            /* FIXME: Don't trust the value of i */
            if (read(fd, buf, i) < 0)
            {
                return rc;
            }
          
            id3->vbr = true;   /* All FLAC files are VBR */
            id3->filesize = filesize(fd);
            id3->frequency = (buf[10] << 12) | (buf[11] << 4) 
                | ((buf[12] & 0xf0) >> 4);
            rc = true;  /* Got vital metadata */

            /* totalsamples is a 36-bit field, but we assume <= 32 bits are used */
            totalsamples = get_long_be(&buf[14]);

            /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
            id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;
        
            if (id3->length <= 0)
            {
                logf("flac length invalid!");
                return false;
            }

            id3->bitrate = (id3->filesize * 8) / id3->length;
        } 
        else if ((buf[0] & 0x7f) == 4)  /* 4 is the VORBIS_COMMENT block */
        {
            /* The next i bytes of the file contain the VORBIS COMMENTS. */
            if (!read_vorbis_tags(fd, id3, i))
            {
                return rc;
            }
        } 
        else 
        {
            if (buf[0] & 0x80) 
            {
                /* If we have reached the last metadata block, abort. */
                break;
            }
            else
            {
                /* Skip to next metadata block */
                if (lseek(fd, i, SEEK_CUR) < 0)
                {
                    return rc;
                }
            }
        }
    }
    
    return true;
}

static bool get_wave_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long totalsamples = 0;
    unsigned long channels = 0;
    unsigned long bitspersample = 0;
    unsigned long numbytes = 0;
    int read_bytes;
    int i;

    /* get RIFF chunk header */
    if ((lseek(fd, 0, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 12)) < 12))
    {
        return false;
    }

    if ((memcmp(buf, "RIFF",4) != 0)
        || (memcmp(&buf[8], "WAVE", 4) !=0 ))
    {
        return false;
    }

    /* iterate over WAVE chunks until 'data' chunk */
    while (true)
    {
        /* get chunk header */
        if ((read_bytes = read(fd, buf, 8)) < 8)
            return false;

        /* chunkSize */
        i = get_long_le(&buf[4]);

        if (memcmp(buf, "fmt ", 4) == 0)
        {
            /* get rest of chunk */
            if ((read_bytes = read(fd, buf, 16)) < 16)
                return false;

            i -= 16;

            /* skipping wFormatTag */
            /* wChannels */
            channels = buf[2] | (buf[3] << 8);
            /* dwSamplesPerSec */
            id3->frequency = get_long_le(&buf[4]);
            /* dwAvgBytesPerSec */
            id3->bitrate = (get_long_le(&buf[8]) * 8) / 1000;
            /* skipping wBlockAlign */
            /* wBitsPerSample */
            bitspersample = buf[14] | (buf[15] << 8);
        }
        else if (memcmp(buf, "data", 4) == 0)
        {
            numbytes = i;
            break;
        }
        else if (memcmp(buf, "fact", 4) == 0)
        {
            /* dwSampleLength */
            if (i >= 4)
            {
                /* get rest of chunk */
                if ((read_bytes = read(fd, buf, 2)) < 2)
                    return false;

                i -= 2;
                totalsamples = get_long_le(buf);
            }
        }

        /* seek to next chunk (even chunk sizes must be padded) */
        if (i & 0x01)
            i++;

        if(lseek(fd, i, SEEK_CUR) < 0)
            return false;
    }

    if ((numbytes == 0) || (channels == 0))
    {
        return false;
    }

    if (totalsamples == 0)
    {
        /* for PCM only */
        totalsamples = numbytes
            / ((((bitspersample - 1) / 8) + 1) * channels);
    }

    id3->vbr = false;   /* All WAV files are CBR */
    id3->filesize = filesize(fd);

    /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
    id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;

    return true;
}

/* Read the tag data from an MP4 file, storing up to buffer_size bytes in
 * buffer.
 */
static unsigned long read_mp4_tag(int fd, unsigned int size_left, char* buffer,
                                  unsigned int buffer_left)
{
    unsigned int bytes_read = 0;
    
    if (buffer_left == 0)
    {
        lseek(fd, size_left, SEEK_CUR);     /* Skip everything */
    } 
    else 
    {
        /* Skip the data tag header - maybe we should parse it properly? */
        lseek(fd, 16, SEEK_CUR); 
        size_left -= 16;

        if (size_left > buffer_left)
        {
            read(fd, buffer, buffer_left);
            lseek(fd, size_left - buffer_left, SEEK_CUR);
            bytes_read = buffer_left;
        } 
        else
        {
            read(fd, buffer, size_left);
            bytes_read = size_left;
        }
    }
    
    return bytes_read;
}

/* Read a string tag from an MP4 file */
static unsigned int read_mp4_tag_string(int fd, int size_left, char** buffer,
                                        unsigned int* buffer_left, char** dest)
{
    unsigned int bytes_read = read_mp4_tag(fd, size_left, *buffer,
        *buffer_left - 1);
    unsigned int length = 0;

    if (bytes_read)
    {
        (*buffer)[bytes_read] = 0;
        *dest = *buffer;
        length = strlen(*buffer) + 1;
        *buffer_left -= length;
        *buffer += length;
    }
    else
    {
        *dest = NULL;
    }
    
    return length;
}

static unsigned int read_mp4_atom(int fd, unsigned int* size, 
                                  unsigned int* type, unsigned int size_left)
{
    read_uint32be(fd, size);
    read_uint32be(fd, type);

    if (*size == 1)
    {
        /* FAT32 doesn't support files this big, so something seems to 
         * be wrong. (64-bit sizes should only be used when required.)
         */
        errno = EFBIG;
        *type = 0;
        return 0;
    }

    if (*size > 0)
    {
        if (*size > size_left)
        {
            size_left = 0;
        }
        else
        {
            size_left -= *size;
        }
        
        *size -= 8;
    }
    else
    {
        *size = size_left;
        size_left = 0;
    }
    
    return size_left;
}

static unsigned int read_mp4_length(int fd, unsigned int* size)
{
    unsigned int length = 0;
    int bytes = 0;
    unsigned char c;

    do
    {
        read(fd, &c, 1);
        bytes++;
        (*size)--;
        length = (length << 7) | (c & 0x7F);
    }
    while ((c & 0x80) && (bytes < 4) && (*size > 0));

    return length;
}

static bool read_mp4_esds(int fd, struct mp3entry* id3, 
    unsigned int* size)
{
    unsigned char buf[8];
    bool sbr = false;

    lseek(fd, 4, SEEK_CUR);     /* Version and flags. */
    read(fd, buf, 1);           /* Verify ES_DescrTag. */
    *size -= 5;

    if (*buf == 3)
    {
        /* read length */
        if (read_mp4_length(fd, size) < 20)
        {
            return sbr;
        }

        lseek(fd, 3, SEEK_CUR);
        *size -= 3;
    } 
    else
    {
        lseek(fd, 2, SEEK_CUR);
        *size -= 2;
    }

    read(fd, buf, 1);           /* Verify DecoderConfigDescrTab. */
    *size -= 1;

    if (*buf != 4)
    {
        return sbr;
    }

    if (read_mp4_length(fd, size) < 13)
    {
        return sbr;
    }
    
    lseek(fd, 13, SEEK_CUR);    /* Skip audio type, bit rates, etc. */
    read(fd, buf, 1);
    *size -= 14;
    
    if (*buf != 5)              /* Verify DecSpecificInfoTag. */
    {
        return sbr;
    }

    {
        static const int sample_rates[] =
        {
            96000, 88200, 64000, 48000, 44100, 32000,
            24000, 22050, 16000, 12000, 11025, 8000
        };
        unsigned long bits;
        unsigned int length;
        unsigned int index;
        int type;
        
        /* Read the (leading part of the) decoder config. */
        length = read_mp4_length(fd, size);
        length = MIN(length, *size);
        length = MIN(length, sizeof(buf));
        read(fd, buf, length);
        *size -= length;

        /* Decoder config format:
         * Object type           - 5 bits
         * Frequency index       - 4 bits
         * Channel configuration - 4 bits
         */
        bits = get_long_be(buf);
        type = bits >> 27;
        index = (bits >> 23) & 0xf;
    
        if (index < (sizeof(sample_rates) / sizeof(*sample_rates)))
        {
            id3->frequency = sample_rates[index];
        }
    
        if (type == 5)
        {
            sbr = true;
            /* Extended frequency index - 4 bits */
            index = (bits >> 15) & 0xf;
    
            if (index == 15)
            {
                /* 17 bits read so far... */
                bits = get_long_be(&buf[2]);
                id3->frequency = (bits >> 7) & 0x00FFFFFF;
            }
            else if (index < (sizeof(sample_rates) / sizeof(*sample_rates)))
            {
                id3->frequency = sample_rates[index];
            }
        }
        else if (id3->frequency < 24000)
        {
            /* SBR not indicated, but the file might still contain SBR. 
             * MPEG specification says that one should assume SBR if
             * samplerate <= 24000 Hz.
             */
            id3->frequency *= 2;
            sbr = true;
        }
    }
    
    return sbr;
}

static bool read_mp4_tags(int fd, struct mp3entry* id3, 
                          unsigned int size_left)
{
    unsigned int size;
    unsigned int type;
    unsigned int buffer_left = sizeof(id3->id3v2buf) + sizeof(id3->id3v1buf);
    char* buffer = id3->id3v2buf;
    bool cwrt = false;

    do
    {
        size_left = read_mp4_atom(fd, &size, &type, size_left);

        /* DEBUGF("Tag atom: '%c%c%c%c' (%d bytes left)\n", type >> 24 & 0xff, 
            type >> 16 & 0xff, type >> 8 & 0xff, type & 0xff, size); */

        switch (type)
        {
        case MP4_cnam:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left, 
                &id3->title);
            break;

        case MP4_cART:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left, 
                &id3->artist);
            break;

        case MP4_calb:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                &id3->album);
            break;
        
        case MP4_cwrt:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                &id3->composer);
            cwrt = false;
            break;

        case MP4_gnre:
            {
                unsigned short genre;
                
                read_mp4_tag(fd, size, (char*) &genre, sizeof(genre));
                id3->genre_string = id3_get_num_genre(betoh16(genre));
            }
            break;

        case MP4_trkn:
            {
                unsigned short n[2];
                
                read_mp4_tag(fd, size, (char*) &n, sizeof(n));
                id3->tracknum = betoh16(n[1]);
            }
            break;

        case MP4_extra:
            {
                char tag_name[TAG_NAME_LENGTH];
                unsigned int sub_size;
                
                /* "mean" atom */
                read_uint32be(fd, &sub_size);
                size -= sub_size;
                lseek(fd, sub_size - 4, SEEK_CUR);
                /* "name" atom */
                read_uint32be(fd, &sub_size);
                size -= sub_size;
                lseek(fd, 8, SEEK_CUR);
                sub_size -= 12;
                
                if (sub_size > sizeof(tag_name) - 1)
                {
                    read(fd, tag_name, sizeof(tag_name) - 1);
                    lseek(fd, sub_size - sizeof(tag_name) - 1, SEEK_CUR);
                    tag_name[sizeof(tag_name) - 1] = 0;
                }
                else
                {
                    read(fd, tag_name, sub_size);
                    tag_name[sub_size] = 0;
                }
                
                if ((strcasecmp(tag_name, "composer") == 0) && !cwrt)
                {
                    read_mp4_tag_string(fd, size, &buffer, &buffer_left, 
                        &id3->composer);
                }
                else
                {
                    char* any;
                    unsigned int length = read_mp4_tag_string(fd, size,
                        &buffer, &buffer_left, &any);
                    
                    if (length > 0)
                    {
                        /* Re-use the read buffer as the dest buffer... */
                        buffer -= length;
                        buffer_left += length;
                        
                        if (parse_replaygain(tag_name, buffer, id3, 
                            buffer, buffer_left) > 0)
                        {
                            /* Data used, keep it. */
                            buffer += length;
                            buffer_left -= length;
                        }
                    }
                }
            }
            break;
        
        default:
            lseek(fd, size, SEEK_CUR);
            break;
        }
    }
    while ((size_left > 0) && (errno == 0));

    return true;
}

static bool read_mp4_container(int fd, struct mp3entry* id3, 
                               unsigned int size_left)
{
    unsigned int size;
    unsigned int type;
    unsigned int handler = 0;
    bool rc = true;
    
    do
    {
        size_left = read_mp4_atom(fd, &size, &type, size_left);
        
        /* DEBUGF("Atom: '%c%c%c%c' (0x%08x, %d bytes left)\n", 
            (type >> 24) & 0xff, (type >> 16) & 0xff, (type >> 8) & 0xff, 
            type & 0xff, type, size); */
        
        switch (type)
        {
        case MP4_ftyp:
            {
                unsigned int id;
                
                read_uint32be(fd, &id);
                size -= 4;
                
                if ((id != MP4_M4A) && (id != MP4_M4B) && (id != MP4_mp42) 
                    && (id != MP4_qt) && (id != MP4_3gp6))
                {
                    DEBUGF("Unknown MP4 file type: '%c%c%c%c'\n", 
                        id >> 24 & 0xff, id >> 16 & 0xff, id >> 8 & 0xff,
                        id & 0xff);
                    return false;
                }
            }
            break;

        case MP4_meta:
            lseek(fd, 4, SEEK_CUR);  /* Skip version */
            size -= 4;
            /* Fall through */

        case MP4_moov:
        case MP4_udta:
        case MP4_mdia:
        case MP4_stbl:
        case MP4_trak:
            rc = read_mp4_container(fd, id3, size);
            size = 0;
            break;
        
        case MP4_ilst:
            if (handler == MP4_mdir)
            {
                rc = read_mp4_tags(fd, id3, size);
                size = 0;
            }
            break;
        
        case MP4_minf:
            if (handler == MP4_soun)
            {
                rc = read_mp4_container(fd, id3, size);
                size = 0;
            }
            break;
        
        case MP4_stsd:
            lseek(fd, 8, SEEK_CUR);
            size -= 8;
            rc = read_mp4_container(fd, id3, size);
            size = 0;
            break;
        
        case MP4_hdlr:
            lseek(fd, 8, SEEK_CUR);
            read_uint32be(fd, &handler);
            size -= 12;
            /* DEBUGF("    Handler '%c%c%c%c'\n", handler >> 24 & 0xff, 
                handler >> 16 & 0xff, handler >> 8 & 0xff,handler & 0xff); */
            break;
        
        case MP4_stts:
            {
                unsigned int entries;
                unsigned int i;
                
                lseek(fd, 4, SEEK_CUR);
                read_uint32be(fd, &entries);
                id3->samples = 0;

                for (i = 0; i < entries; i++)
                {
                    unsigned int n;
                    unsigned int l;

                    read_uint32be(fd, &n);
                    read_uint32be(fd, &l);
                    id3->samples += n * l;
                }
                
                size = 0;
            }
            break;

        case MP4_mp4a:
        case MP4_alac:
            {
                unsigned int frequency;

                id3->codectype = (type == MP4_mp4a) ? AFMT_AAC : AFMT_ALAC;
                lseek(fd, 22, SEEK_CUR);
                read_uint32be(fd, &frequency);
                size -= 26;
                id3->frequency = frequency;
                
                if (type == MP4_mp4a)
                {
                    unsigned int subsize;
                    unsigned int subtype;

                    /* Get frequency from the decoder info tag, if possible. */
                    lseek(fd, 2, SEEK_CUR);
                    /* The esds atom is a part of the mp4a atom, so ignore 
                     * the returned size (it's already accounted for).
                     */
                    read_mp4_atom(fd, &subsize, &subtype, size);
                    size -= 10;
                    
                    if (subtype == MP4_esds)
                    {
                        read_mp4_esds(fd, id3, &size);
                    }
                }
            }
            break;

        case MP4_mdat:
            id3->filesize = size;
            break;

        default:
            break;
        }
        
        lseek(fd, size, SEEK_CUR);
    }
    while (rc && (size_left > 0) && (errno == 0) && (id3->filesize == 0));
    /* Break on non-zero filesize, since Rockbox currently doesn't support
     * metadata after the mdat atom (which sets the filesize field). 
     */
    
    return rc;
}

static bool get_mp4_metadata(int fd, struct mp3entry* id3)
{
    id3->codectype = AFMT_UNKNOWN;
    id3->filesize = 0;
    errno = 0;

    if (read_mp4_container(fd, id3, filesize(fd)) && (errno == 0) 
        && (id3->samples > 0) && (id3->frequency > 0) 
        && (id3->filesize > 0))
    {
        if (id3->codectype == AFMT_UNKNOWN)
        {
            logf("Not an ALAC or AAC file");
            return false;
        }

        id3->length = ((int64_t) id3->samples * 1000) / id3->frequency;
    
        if (id3->length <= 0)
        {
            logf("mp4 length invalid!");
            return false;
        }
        
        id3->bitrate = ((int64_t) id3->filesize * 8) / id3->length;
        DEBUGF("MP4 bitrate %d, frequency %d Hz, length %d ms\n",
            id3->bitrate, id3->frequency, id3->length);
    }
    else
    {
        logf("MP4 metadata error");
        DEBUGF("MP4 metadata error. errno %d, length %d, frequency %d, filesize %d\n",
            errno, id3->length, id3->frequency, id3->filesize);
        return false;
    }

    return true;
}

static bool get_musepack_metadata(int fd, struct mp3entry *id3)
{
    const int32_t sfreqs_sv7[4] = { 44100, 48000, 37800, 32000 };
    uint32_t header[8];
    uint64_t samples = 0;
    int i;
    
    if (!skip_id3v2(fd, id3))
        return false;
    if (read(fd, header, 4*8) != 4*8) return false;
    /* Musepack files are little endian, might need swapping */
    for (i = 1; i < 8; i++) 
       header[i] = letoh32(header[i]); 
    if (!memcmp(header, "MP+", 3)) { /* Compare to sig "MP+" */
        unsigned int streamversion;
        
        header[0] = letoh32(header[0]);
        streamversion = (header[0] >> 24) & 15;
        if (streamversion >= 8) {
            return false; /* SV8 or higher don't exist yet, so no support */
        } else if (streamversion == 7) {
            unsigned int gapless = (header[5] >> 31) & 0x0001;
            unsigned int last_frame_samples = (header[5] >> 20) & 0x07ff;
            int track_gain, album_gain;
            unsigned int bufused;
            
            id3->frequency = sfreqs_sv7[(header[2] >> 16) & 0x0003];
            samples = (uint64_t)header[1]*1152; /* 1152 is mpc frame size */
            if (gapless)
                samples -= 1152 - last_frame_samples;
            else
                samples -= 481; /* Musepack subband synth filter delay */
           
            /* Extract ReplayGain data from header */
            track_gain = (int16_t)((header[3] >> 16) & 0xffff);
            id3->track_gain = get_replaygain_int(track_gain);
            id3->track_peak = ((uint16_t)(header[3] & 0xffff)) << 9;
            
            album_gain = (int16_t)((header[4] >> 16) & 0xffff);
            id3->album_gain = get_replaygain_int(album_gain);
            id3->album_peak = ((uint16_t)(header[4] & 0xffff)) << 9;
            
            /* Write replaygain values to strings for use in id3 screen. We use
               the XING header as buffer space since Musepack files shouldn't
               need to use it in any other way */
            id3->track_gain_string = (char *)id3->toc;
            bufused = snprintf(id3->track_gain_string, 45,
                               "%d.%d dB", track_gain/100, abs(track_gain)%100);
            id3->album_gain_string = (char *)id3->toc + bufused + 1;
            bufused = snprintf(id3->album_gain_string, 45,
                               "%d.%d dB", album_gain/100, abs(album_gain)%100);
        }
    } else {
        header[0] = letoh32(header[0]);
        unsigned int streamversion = (header[0] >> 11) & 0x03FF;
        if (streamversion != 4 && streamversion != 5 && streamversion != 6)
            return false;
        id3->frequency = 44100;
        id3->track_gain = 0;
        id3->track_peak = 0;
        id3->album_gain = 0;
        id3->album_peak = 0;

        if (streamversion >= 5)
            samples = (uint64_t)header[1]*1152; // 32 bit
        else
            samples = (uint64_t)(header[1] >> 16)*1152; // 16 bit

        samples -= 576;
        if (streamversion < 6)
            samples -= 1152;
    }

    id3->vbr = true;
    /* Estimate bitrate, we should probably subtract the various header sizes
       here for super-accurate results */
    id3->length = ((int64_t) samples * 1000) / id3->frequency;

    if (id3->length <= 0)
    {
        logf("mpc length invalid!");
        return false;
    }

    id3->filesize = filesize(fd);
    id3->bitrate = id3->filesize * 8 / id3->length;
    return true;
}

/* PSID metadata info is available here: 
   http://www.unusedino.de/ec64/technical/formats/sidplay.html */
static bool get_sid_metadata(int fd, struct mp3entry* id3)
{    
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;    
    int read_bytes;
    char *p;
    

    if ((lseek(fd, 0, SEEK_SET) < 0) 
         || ((read_bytes = read(fd, buf, 0x80)) < 0x80))
    {
        return false;
    }
    
    if ((memcmp(buf, "PSID",4) != 0))
    {
        return false;
    }

    p = id3->id3v2buf;

    /* Copy Title (assumed max 0x1f letters + 1 zero byte) */
    id3->title = p;
    buf[0x16+0x1f] = 0;
    p = iso_decode(&buf[0x16], p, 0, strlen(&buf[0x16])+1);

    /* Copy Artist (assumed max 0x1f letters + 1 zero byte) */
    id3->artist = p;
    buf[0x36+0x1f] = 0;
    p = iso_decode(&buf[0x36], p, 0, strlen(&buf[0x36])+1);

    /* Copy Year (assumed max 4 letters + 1 zero byte) */
    buf[0x56+0x4] = 0;
    id3->year = atoi(&buf[0x56]);

    /* Copy Album (assumed max 0x1f-0x05 letters + 1 zero byte) */
    id3->album = p;
    buf[0x56+0x1f] = 0;
    p = iso_decode(&buf[0x5b], p, 0, strlen(&buf[0x5b])+1);

    id3->bitrate = 706;
    id3->frequency = 44100;
    /* New idea as posted by Marco Alanen (ravon):
     * Set the songlength in seconds to the number of subsongs
     * so every second represents a subsong.
     * Users can then skip the current subsong by seeking
     *
     * Note: the number of songs is a 16bit value at 0xE, so this code only
     * uses the lower 8 bits of the counter.
    */
    id3->length = (buf[0xf]-1)*1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);

    return true;
}

static bool get_adx_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char * buf = (unsigned char *)id3->path;
    int chanstart, channels, read_bytes;
    int looping = 0, start_adr = 0, end_adr = 0;
    
    /* try to get the basic header */
    if ((lseek(fd, 0, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 0x38)) < 0x38))
    {
        DEBUGF("lseek or read failed\n");
        return false;
    }
    
    /* ADX starts with 0x80 */
    if (buf[0] != 0x80) {
        DEBUGF("get_adx_metadata: wrong first byte %c\n",buf[0]);
        return false;
    }
    
    /* check for a reasonable offset */
    chanstart = ((buf[2] << 8) | buf[3]) + 4;
    if (chanstart > 4096) {
        DEBUGF("get_adx_metadata: bad chanstart %i\n", chanstart);
        return false;
    }

    /* check for a workable number of channels */
    channels = buf[7];
    if (channels != 1 && channels != 2) {
        DEBUGF("get_adx_metadata: bad channel count %i\n",channels);
        return false;
    }
    
    id3->frequency = get_long_be(&buf[8]);
    /* 32 samples per 18 bytes */
    id3->bitrate = id3->frequency * channels * 18 * 8 / 32 / 1000;
    id3->length = get_long_be(&buf[12]) / id3->frequency * 1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);
    
    /* get loop info */
    if (!memcmp(buf+0x10,"\x01\xF4\x03\x00",4)) {
        /* Soul Calibur 2 style (type 03) */
        DEBUGF("get_adx_metadata: type 03 found\n");
        /* check if header is too small for loop data */
        if (chanstart-6 < 0x2c) looping=0;
        else {
            looping = get_long_be(&buf[0x18]);
            end_adr = get_long_be(&buf[0x28]);
            start_adr = get_long_be(&buf[0x1c])/32*channels*18+chanstart;
        }
    } else if (!memcmp(buf+0x10,"\x01\xF4\x04\x00",4)) {
        /* Standard (type 04) */
        DEBUGF("get_adx_metadata: type 04 found\n");
        /* check if header is too small for loop data */
        if (chanstart-6 < 0x38) looping=0;
        else {
            looping = get_long_be(&buf[0x24]);
            end_adr = get_long_be(&buf[0x34]);
            start_adr = get_long_be(&buf[0x28])/32*channels*18+chanstart;
        }
    } else {
        DEBUGF("get_adx_metadata: error, couldn't determine ADX type\n");
        return false;
    }
    
    if (looping) {
        /* 2 loops, 10 second fade */
        id3->length = (start_adr-chanstart + 2*(end_adr-start_adr))
                      *8 / id3->bitrate + 10000;
    }
        
    /* try to get the channel header */
    if ((lseek(fd, chanstart-6, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 6)) < 6))
    {
        return false;
    }
    
    /* check channel header */
    if (memcmp(buf, "(c)CRI", 6) != 0) return false;
    
    return true;    
}

static bool get_aiff_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long numChannels = 0;
    unsigned long numSampleFrames = 0;
    unsigned long sampleSize = 0;
    unsigned long sampleRate = 0;
    unsigned long numbytes = 0;
    int read_bytes;
    int i;

    if ((lseek(fd, 0, SEEK_SET) < 0) 
        || ((read_bytes = read(fd, buf, sizeof(id3->path))) < 54))
    {
        return false;
    }
    
    if ((memcmp(buf, "FORM",4) != 0)
        || (memcmp(&buf[8], "AIFF", 4) !=0 ))
    {
        return false;
    }

    buf += 12;
    read_bytes -= 12;

    while ((numbytes == 0) && (read_bytes >= 8)) 
    {
        /* chunkSize */
        i = get_long_be(&buf[4]);
        
        if (memcmp(buf, "COMM", 4) == 0)
        {
            /* numChannels */
            numChannels = ((buf[8]<<8)|buf[9]);
            /* numSampleFrames */
            numSampleFrames = get_long_be(&buf[10]);
            /* sampleSize */
            sampleSize = ((buf[14]<<8)|buf[15]);
            /* sampleRate */
            sampleRate = get_long_be(&buf[18]);
            sampleRate = sampleRate >> (16+14-buf[17]);
            /* save format infos */
            id3->bitrate = (sampleSize * numChannels * sampleRate) / 1000;
            id3->frequency = sampleRate;
            id3->length = ((int64_t) numSampleFrames * 1000) / id3->frequency;

            id3->vbr = false;   /* AIFF files are CBR */
            id3->filesize = filesize(fd);
        }
        else if (memcmp(buf, "SSND", 4) == 0) 
        {
            numbytes = i - 8;
        }

        if (i & 0x01)
        {
            i++;  /* odd chunk sizes must be padded */
        }
        buf += i + 8;
        read_bytes -= i + 8;
    }

    if ((numbytes == 0) || (numChannels == 0)) 
    {
        return false;
    }
    return true;
}
#endif /* CONFIG_CODEC == SWCODEC */


/* Simple file type probing by looking at the filename extension. */
unsigned int probe_file_format(const char *filename)
{
    char *suffix;
    unsigned int i;
    
    suffix = strrchr(filename, '.');

    if (suffix == NULL)
    {
        return AFMT_UNKNOWN;
    }
    
    /* skip '.' */
    suffix++;
    
    for (i = 1; i < AFMT_NUM_CODECS; i++)
    {
        /* search extension list for type */
        const char *ext = audio_formats[i].ext_list;

        do
        {
            if (strcasecmp(suffix, ext) == 0)
            {
                return i;
            }

            ext += strlen(ext) + 1;
        }
        while (*ext != '\0');
    }
    
    return AFMT_UNKNOWN;
}

/* Get metadata for track - return false if parsing showed problems with the
 * file that would prevent playback.
 */
bool get_metadata(struct track_info* track, int fd, const char* trackname,
                  bool v1first) 
{
#if CONFIG_CODEC == SWCODEC
    unsigned char* buf;
    unsigned long totalsamples;
    int i;
#endif

    /* Take our best guess at the codec type based on file extension */
    track->id3.codectype = probe_file_format(trackname);

    /* Load codec specific track tag information and confirm the codec type. */
    switch (track->id3.codectype) 
    {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        if (!get_mp3_metadata(fd, &track->id3, trackname, v1first))
        {
            return false;
        }

        break;

#if CONFIG_CODEC == SWCODEC
    case AFMT_FLAC:
        if (!get_flac_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_MPC:
        if (!get_musepack_metadata(fd, &(track->id3)))
            return false;
        read_ape_tags(fd, &(track->id3));
        break;
    
    case AFMT_OGG_VORBIS:
        if (!get_vorbis_metadata(fd, &(track->id3)))/*detects and handles Ogg/Speex files*/
        {
            return false;
        }

        break;

    case AFMT_SPEEX:
        if (!get_speex_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_PCM_WAV:
        if (!get_wave_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_WAVPACK:
        /* A simple parser to read basic information from a WavPack file. This
         * now works with self-extrating WavPack files. This no longer fails on
         * WavPack files containing floating-point audio data because these are
         * now converted to standard Rockbox format in the decoder.
         */

        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = (unsigned char *)track->id3.path;
      
        for (i = 0; i < 256; ++i) {

            /* at every 256 bytes into file, try to read a WavPack header */

            if ((lseek(fd, i * 256, SEEK_SET) < 0) || (read(fd, buf, 32) < 32))
            {
                return false;
            }

            /* if valid WavPack 4 header version & not floating data, break */

            if (memcmp (buf, "wvpk", 4) == 0 && buf [9] == 4 &&
                (buf [8] >= 2 && buf [8] <= 0x10))
            {          
                break;
            }
        }

        if (i == 256) {
            logf ("%s is not a WavPack file\n", trackname);
            return false;
        }

        track->id3.vbr = true;   /* All WavPack files are VBR */
        track->id3.filesize = filesize (fd);

        if ((buf [20] | buf [21] | buf [22] | buf [23]) &&
            (buf [12] & buf [13] & buf [14] & buf [15]) != 0xff) 
        {
            int srindx = ((buf [26] >> 7) & 1) + ((buf [27] << 1) & 14);

            if (srindx == 15)
            {
                track->id3.frequency = 44100;
            }
            else
            {
                track->id3.frequency = wavpack_sample_rates[srindx];
            }

            totalsamples = get_long_le(&buf[12]);
            track->id3.length = totalsamples / (track->id3.frequency / 100) * 10;
            track->id3.bitrate = filesize (fd) / (track->id3.length / 8);
        }

        read_ape_tags(fd, &track->id3);     /* use any apetag info we find */
        break;

    case AFMT_A52:
        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = (unsigned char *)track->id3.path;
        
        if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 5) < 5))
        {
            return false;
        }

        if ((buf[0] != 0x0b) || (buf[1] != 0x77)) 
        { 
            logf("%s is not an A52/AC3 file\n",trackname);
            return false;
        }

        i = buf[4] & 0x3e;
      
        if (i > 36) 
        {
            logf("A52: Invalid frmsizecod: %d\n",i);
            return false;
        }
      
        track->id3.bitrate = a52_bitrates[i >> 1];
        track->id3.vbr = false;
        track->id3.filesize = filesize(fd);

        switch (buf[4] & 0xc0) 
        {
        case 0x00: 
            track->id3.frequency = 48000; 
            track->id3.bytesperframe=track->id3.bitrate * 2 * 2;
            break;
            
        case 0x40: 
            track->id3.frequency = 44100;
            track->id3.bytesperframe = a52_441framesizes[i];
            break;
        
        case 0x80: 
            track->id3.frequency = 32000; 
            track->id3.bytesperframe = track->id3.bitrate * 3 * 2;
            break;
        
        default: 
            logf("A52: Invalid samplerate code: 0x%02x\n", buf[4] & 0xc0);
            return false;
            break;
        }

        /* One A52 frame contains 6 blocks, each containing 256 samples */
        totalsamples = track->id3.filesize / track->id3.bytesperframe * 6 * 256;
        track->id3.length = totalsamples / track->id3.frequency * 1000;
        break;

    case AFMT_ALAC:
    case AFMT_AAC:
        if (!get_mp4_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_SHN:
        track->id3.vbr = true;
        track->id3.filesize = filesize(fd);
        if (!skip_id3v2(fd, &(track->id3)))
        {
            return false;
        }
        /* TODO: read the id3v2 header if it exists */
        break;

    case AFMT_SID:
        if (!get_sid_metadata(fd, &(track->id3)))
        {
            return false;
        }
        break;
    case AFMT_SPC:
        track->id3.filesize = filesize(fd);
        track->id3.genre_string = id3_get_num_genre(36);
        break;
    case AFMT_ADX:
        if (!get_adx_metadata(fd, &(track->id3)))
        {
            DEBUGF("get_adx_metadata error\n");
            return false;
        }

        break;
    case AFMT_NSF:
        buf = (unsigned char *)track->id3.path;
        if ((lseek(fd, 0, SEEK_SET) < 0) || ((read(fd, buf, 8)) < 8))
        {
            DEBUGF("lseek or read failed\n");
            return false;
        }
        track->id3.vbr = false;
        track->id3.filesize = filesize(fd);
        if (memcmp(buf,"NESM",4) && memcmp(buf,"NSFE",4)) return false;
    break;

    case AFMT_AIFF:
        if (!get_aiff_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

#endif /* CONFIG_CODEC == SWCODEC */
        
    default:
        /* If we don't know how to read the metadata, assume we can't play 
           the file */
        return false;
        break;
    }

    /* We have successfully read the metadata from the file */

    if (cuesheet_is_enabled() && look_for_cuesheet_file(trackname))
    {
        track->id3.cuesheet_type = 1;
    }

    lseek(fd, 0, SEEK_SET);
    strncpy(track->id3.path, trackname, sizeof(track->id3.path));
    track->taginfo_ready = true;

    return true;
}

