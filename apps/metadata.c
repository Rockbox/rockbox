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

#include "metadata.h"
#include "mp3_playback.h"
#include "logf.h"
#include "atoi.h"
#include "replaygain.h"
#include "debug.h"
#include "system.h"

enum tagtype { TAGTYPE_APE = 1, TAGTYPE_VORBIS };

#define APETAG_HEADER_LENGTH        32
#define APETAG_HEADER_FORMAT        "8LLLL"
#define APETAG_ITEM_HEADER_FORMAT   "LL"
#define APETAG_ITEM_TYPE_MASK       3

#define TAG_NAME_LENGTH             32
#define TAG_VALUE_LENGTH            128

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

struct format_list
{
    char format;
    char extension[5];
};

static const struct format_list formats[] =
{
    { AFMT_MPA_L1,        "mp1"  },
    { AFMT_MPA_L2,        "mp2"  },
    { AFMT_MPA_L2,        "mpa"  },
    { AFMT_MPA_L3,        "mp3"  },
    { AFMT_OGG_VORBIS,    "ogg"  },
    { AFMT_PCM_WAV,       "wav"  },
    { AFMT_FLAC,          "flac" },
    { AFMT_MPC,           "mpc"  },
    { AFMT_A52,           "a52"  },
    { AFMT_A52,           "ac3"  },
    { AFMT_WAVPACK,       "wv"   },
    { AFMT_ALAC,          "m4a"  },
};

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

/* Convert a little-endian structure to native format using a format string.
 * Does nothing on a little-endian machine.
 */ 
static void convert_endian(void *data, const char *format)
{
    while (*format) 
    {
        switch (*format) 
        {
        case 'L':
            {
                long* d = (long*) data;
                
                *d = letoh32(*d);
                data = d + 1;
            }
            
            break;

        case 'S':
            {
                short* d = (short*) data;
                
                *d = letoh16(*d);
                data = d + 1;
            }
            
            break;

        default:
            if (isdigit(*format))
            {
                data = ((char*) data) + *format - '0';
            }
            
            break;
        }

        format++;
    }
}

/* read_uint32be() - read an unsigned integer from a big-endian
   (e.g. Quicktime) file.  This is used by the .m4a parser
*/
#ifdef ROCKBOX_BIG_ENDIAN
#define read_uint32be(fd,buf) read((fd),(buf),4)
#else
int read_uint32be(int fd, unsigned int* buf) {
  char tmp;
  char* p=(char*)buf;
  size_t n;

  n=read(fd,p,4);
  if (n==4) {
    tmp=p[0];
    p[0]=p[3];
    p[3]=tmp;
    tmp=p[2];
    p[2]=p[1];
    p[1]=tmp;
  }

  return(n);
}
#endif

/* Read an unaligned 32-bit little endian long from buffer. */
static unsigned long get_long(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Convert an UTF-8 string to Latin-1, overwriting the old string (the new
 * string is never longer than the original, so this is safe). Non-latin-1
 * chars are replaced with '?'.
 */
static void convert_utf8(char* utf8)
{
    char* dest = utf8;
    long code = 0;
    unsigned char c;
    int tail = 0;
    int size = 0;

    while ((c = *utf8++) != 0)
    {
        if ((tail <= 0) && ((c <= 0x7f) || (c >= 0xc2)))
        {
            /* Start of new character. */
            if (c < 0x80)
            {
                size = 1;
            }
            else if (c < 0xe0)
            {
                size = 2;
                c &= 0x1f;
            }
            else if (c < 0xf0)
            {
                size = 3;
                c &= 0x0f;
            }
            else if (c < 0xf5)
            {
                size = 4;
                c &= 0x07;
            }
            else
            {
                /* Invalid size. */
                size = 0;
            }

            code = c;
            tail = size - 1;
        }
        else if ((tail > 0) && ((c & 0xc0) == 0x80))
        {
            /* Valid continuation character. */
            code = (code << 6) | (c & 0x3f);
            tail--;

            if (tail == 0)
            {
                if (((size == 2) && (code < 0x80))
                    || ((size == 3) && (code < 0x800))
                    || ((size == 4) && (code < 0x10000)))
                {
                    /* Invalid encoding. */
                    code = 0;
                }
            }
        }
        else
        {
            tail = -1;
        }

        if ((tail == 0) && (code > 0))
        {
            *dest++ = (code <= 0xff) ? (char) (code & 0xff) : '?';
        }
    }

    *dest = 0;
}


/* Read a string tag from an M4A file */
void read_m4a_tag_string(int fd, int len,char** bufptr,size_t* bytes_remaining, char** dest)
{
  int data_length;

  if (bytes_remaining==0) {
    lseek(fd,len,SEEK_CUR); /* Skip everything */
  } else {
    /* Skip the data tag header - maybe we should parse it properly? */
    lseek(fd,16,SEEK_CUR); 
    len-=16;

    *dest=*bufptr;
    if ((size_t)len+1 > *bytes_remaining) {
      read(fd,*bufptr,*bytes_remaining-1);
      lseek(fd,len-(*bytes_remaining-1),SEEK_CUR);
      *bufptr+=(*bytes_remaining-1);
    } else {
      read(fd,*bufptr,len);
      *bufptr+=len;
    }
    **bufptr=(char)0;

    convert_utf8(*dest);
    data_length = strlen(*dest)+1;
    *bufptr=(*dest)+data_length;
    *bytes_remaining-=data_length;
  }
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
        || (read(fd, &header, APETAG_HEADER_LENGTH) != APETAG_HEADER_LENGTH)
        || (memcmp(header.id, "APETAGEX", sizeof(header.id))))
    {
        return false;
    }

    convert_endian(&header, APETAG_HEADER_FORMAT);
    id3->genre = 0xff;

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
            
            if (read(fd, &item, sizeof(item)) < (long) sizeof(item))
            {
                return false;
            }
            
            convert_endian(&item, APETAG_ITEM_HEADER_FORMAT);
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

                convert_utf8(value);
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

    id3->genre = 255;

    if (read(fd, &len, sizeof(len)) < (long) sizeof(len)) 
    {
        return false;
    }
    
    convert_endian(&len, "L");
    
    if ((lseek(fd, len, SEEK_CUR) < 0)
        || (read(fd, &comment_count, sizeof(comment_count)) 
            < (long) sizeof(comment_count)))
    {
        return false;
    }
    
    convert_endian(&comment_count, "L");
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

        if (read(fd, &len, sizeof(len)) < (long) sizeof(len))
        {
            return false;
        }

        convert_endian(&len, "L");
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

        convert_utf8(value);
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

/* A simple parser to read vital metadata from an Ogg Vorbis file. Returns
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
    unsigned char* buf = id3->path;
    long comment_size;
    long remaining = 0;
    long last_serial = 0;
    long serial;
    int segments;
    int i;
    bool eof = false;

    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 58) < 4))
    {
        return false;
    }
    
    if ((memcmp(buf, "OggS", 4) != 0) || (memcmp(&buf[29], "vorbis", 6) != 0))
    {
        return false;
    }
    
    /* We need to ensure the serial number from this page is the same as the
     * one from the last page (since we only support a single bitstream).
     */
    serial = get_long(&buf[14]);
    id3->frequency = get_long(&buf[40]);
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

    if (lseek(fd, -64 * 1024, SEEK_END) < 0)  /* A page is always < 64 kB */
    {
        return false;
    }

    remaining = 0;

    while (!eof) 
    {
        long r = read(fd, &buf[remaining], MAX_PATH - remaining);
        
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
        
        while (i < (remaining - 5)) 
        {
            if ((buf[i] == 'O') && (memcmp(&buf[i], "OggS", 4) == 0))
            {
                if (i < (remaining - 17)) 
                {
                    /* Note that this only reads the low 32 bits of a
                     * 64 bit value.
                     */
                     id3->samples = get_long(&buf[i + 6]);
                     last_serial = get_long(&buf[i + 14]);
                     /* We can discard the rest of the buffer */
                     remaining = 0;
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

        if (i < (remaining - 5)) 
        {
            /* Move OggS to start of buffer. */
            while (i >0)
            {
                buf[i--] = buf[remaining--];
            }
        }
        else
        {
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
    unsigned char* buf = id3->path;
    bool rc = false;

    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 4) < 4))
    {
        return rc;
    }

    if (memcmp(buf,"fLaC",4) != 0) 
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
            totalsamples = (buf[14] << 24) | (buf[15] << 16) 
                | (buf[16] << 8) | buf[17];

            /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
            id3->length = (totalsamples / id3->frequency) * 1000;
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
    unsigned char* buf = id3->path;
    unsigned long totalsamples = 0;
    unsigned long channels = 0;
    unsigned long bitspersample = 0;
    unsigned long numbytes = 0;
    int read_bytes;
    int i;

    if ((lseek(fd, 0, SEEK_SET) < 0) 
        || ((read_bytes = read(fd, buf, sizeof(id3->path))) < 44))
    {
        return false;
    }
    
    if ((memcmp(buf, "RIFF",4) != 0)
        || (memcmp(&buf[8], "WAVE", 4) !=0 ))
    {
        return false;
    }

    buf += 12;
    read_bytes -= 12;

    while ((numbytes == 0) && (read_bytes >= 8)) 
    {
        /* chunkSize */
        i = get_long(&buf[4]);
        
        if (memcmp(buf, "fmt ", 4) == 0)
        {
            /* skipping wFormatTag */
            /* wChannels */
            channels = buf[10] | (buf[11] << 8);
            /* dwSamplesPerSec */
            id3->frequency = get_long(&buf[12]);
            /* dwAvgBytesPerSec */
            id3->bitrate = (get_long(&buf[16]) * 8) / 1000;
            /* skipping wBlockAlign */
            /* wBitsPerSample */
            bitspersample = buf[22] | (buf[23] << 8);
        }
        else if (memcmp(buf, "data", 4) == 0) 
        {
            numbytes = i;
        }
        else if (memcmp(buf, "fact", 4) == 0) 
        {
            /* dwSampleLength */
            if (i >= 4) 
            {
                totalsamples = get_long(&buf[8]);
            }
        }
     
        /* go to next chunk (even chunk sizes must be padded) */
        if (i & 0x01)
        {
            i++;
        }
     
        buf += i + 8;
        read_bytes -= i + 8;
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
    id3->length = (totalsamples / id3->frequency) * 1000;

    return true;
}



static bool get_alac_metadata(int fd, struct mp3entry* id3)
{
  unsigned char* buf;
  unsigned long totalsamples;
  int i,j,k;
  size_t n;
  size_t bytes_remaining;
  char* id3buf;
  unsigned int compressedsize;
  unsigned int sample_count;
  unsigned int sample_duration;
  int numentries;
  int entry_size;
  int size_remaining;
  int chunk_len;
  unsigned char chunk_id[4];
  int sub_chunk_len;
  unsigned char sub_chunk_id[4];

  /* A simple parser to read vital metadata from an ALAC file.
     This parser also works for AAC files - they are both stored in 
     a Quicktime M4A container. */

  /* Use the trackname part of the id3 structure as a temporary buffer */
  buf=id3->path;

  lseek(fd, 0, SEEK_SET);

  totalsamples=0;
  compressedsize=0;
  /* read the chunks - we stop when we find the mdat chunk and set compressedsize */
  while (compressedsize==0) {
    n=read_uint32be(fd,&chunk_len);

    // This means it was a 64-bit file, so we have problems.
    if (chunk_len == 1) {
      logf("need 64bit support\n");
      return false;
    }

    n=read(fd,&chunk_id,4);
    if (memcmp(&chunk_id,"ftyp",4)==0) {
      /* Check for M4A type */
      n=read(fd,&chunk_id,4);
      if (memcmp(&chunk_id,"M4A ",4)!=0) {
        logf("Not an M4A file, aborting\n");
        return false;
      }
      /* Skip rest of chunk */
      lseek(fd, chunk_len - 8 - 4, SEEK_CUR); /* FIXME not 8 */
    } else if (memcmp(&chunk_id,"moov",4)==0) {
      size_remaining=chunk_len - 8; /* FIXME not 8 */

      while (size_remaining > 0) {
        n=read_uint32be(fd,&sub_chunk_len);
        if ((sub_chunk_len < 1) || (sub_chunk_len > size_remaining)) {
          logf("Strange sub_chunk_len value inside moov: %d (remaining: %d)\n",sub_chunk_len,size_remaining);
          return false;
        }
        n=read(fd,&sub_chunk_id,4);
        size_remaining-=8;

        if (memcmp(&sub_chunk_id,"mvhd",4)==0) {
          /* We don't need anything from here - skip */
          lseek(fd, sub_chunk_len - 8, SEEK_CUR); /* FIXME not 8 */
          size_remaining-=(sub_chunk_len-8);
        } else if (memcmp(&sub_chunk_id,"udta",4)==0) {
          /* The udta chunk contains the metadata - track, artist, album etc.
             The format appears to be:
               udta
                 meta
                  hdlr
                  ilst
                    .nam
                    [rest of tags]
                  free

              NOTE: This code was written by examination of some .m4a files
                    produced by iTunes v4.9 - it may not therefore be 100%
                    compliant with all streams.  But it should fail gracefully.
           */
          j=(sub_chunk_len-8);
          size_remaining-=j;
          n=read_uint32be(fd,&sub_chunk_len);
          n=read(fd,&sub_chunk_id,4);
          j-=8;
          if (memcmp(&sub_chunk_id,"meta",4)==0) {
            lseek(fd, 4, SEEK_CUR);
            j-=4;
            n=read_uint32be(fd,&sub_chunk_len);
            n=read(fd,&sub_chunk_id,4);
            j-=8;
            if (memcmp(&sub_chunk_id,"hdlr",4)==0) {
              lseek(fd, sub_chunk_len - 8, SEEK_CUR);
              j-=(sub_chunk_len - 8);
              n=read_uint32be(fd,&sub_chunk_len);
              n=read(fd,&sub_chunk_id,4);
              j-=8;
              if (memcmp(&sub_chunk_id,"ilst",4)==0) {
                /* Here are the actual tags.  We use the id3v2 300-byte buffer
                   to store the string data */
                bytes_remaining=sizeof(id3->id3v2buf);
                id3->genre=255; /* Not every track is the Blues */
                id3buf=id3->id3v2buf;
                k=sub_chunk_len-8;
                j-=k;
                while (k > 0) {
                  n=read_uint32be(fd,&sub_chunk_len);
                  n=read(fd,&sub_chunk_id,4);
                  k-=8;
                  if (memcmp(sub_chunk_id,"\251nam",4)==0) {
                    read_m4a_tag_string(fd,sub_chunk_len-8,&id3buf,&bytes_remaining,&id3->title);
                  } else if (memcmp(sub_chunk_id,"\251ART",4)==0) {
                    read_m4a_tag_string(fd,sub_chunk_len-8,&id3buf,&bytes_remaining,&id3->artist);
                  } else if (memcmp(sub_chunk_id,"\251alb",4)==0) {
                    read_m4a_tag_string(fd,sub_chunk_len-8,&id3buf,&bytes_remaining,&id3->album);
                  } else if (memcmp(sub_chunk_id,"\251gen",4)==0) {
                    read_m4a_tag_string(fd,sub_chunk_len-8,&id3buf,&bytes_remaining,&id3->genre_string);
                  } else if (memcmp(sub_chunk_id,"\251day",4)==0) {
                    read_m4a_tag_string(fd,sub_chunk_len-8,&id3buf,&bytes_remaining,&id3->year_string);
                  } else if (memcmp(sub_chunk_id,"trkn",4)==0) {
                    if (sub_chunk_len==0x20) {
                      read(fd,buf,sub_chunk_len-8);
                      id3->tracknum=buf[19];
                    } else {
                      lseek(fd, sub_chunk_len-8,SEEK_CUR);
                    }
                  } else {
                    lseek(fd, sub_chunk_len-8,SEEK_CUR);
                  }
                  k-=(sub_chunk_len-8);
                }
              }
            }
          }
          /* Skip any remaining data in udta chunk */
          lseek(fd, j, SEEK_CUR);
        } else if (memcmp(&sub_chunk_id,"trak",4)==0) {
          /* Format of trak chunk:
             tkhd
             mdia
               mdhd
               hdlr
               minf
                 smhd
                 dinf
                 stbl
                   stsd - Samplerate, Samplesize, Numchannels
                   stts - time_to_sample array - RLE'd table containing duration of each block
                   stsz - sample_byte_size array - ?Size in bytes of each compressed block
                   stsc - Seek table related?
                   stco - Seek table related?
           */

           /* Skip tkhd - not needed */
           n=read_uint32be(fd,&sub_chunk_len);
           n=read(fd,&sub_chunk_id,4);
           if (memcmp(&sub_chunk_id,"tkhd",4)!=0) {
             logf("Expecting tkhd\n");
             return false;
           }
           lseek(fd, sub_chunk_len - 8, SEEK_CUR); /* FIXME not 8 */
           size_remaining-=sub_chunk_len;

           /* Process mdia - skipping possible edts */
           n=read_uint32be(fd,&sub_chunk_len);
           n=read(fd,&sub_chunk_id,4);
           if (memcmp(&sub_chunk_id,"edts",4)==0) {
             lseek(fd, sub_chunk_len - 8, SEEK_CUR); /* FIXME not 8 */
             size_remaining-=sub_chunk_len;
             n=read_uint32be(fd,&sub_chunk_len);
             n=read(fd,&sub_chunk_id,4);
           }

           if (memcmp(&sub_chunk_id,"mdia",4)!=0) {
             logf("Expecting mdia\n");
             return false;
           }
           size_remaining-=sub_chunk_len;
           j=sub_chunk_len-8;

           while (j > 0) {
             n=read_uint32be(fd,&sub_chunk_len);
             n=read(fd,&sub_chunk_id,4);
             j-=4;
             if (memcmp(&sub_chunk_id,"minf",4)==0) {
               j=sub_chunk_len-8;
             } else if (memcmp(&sub_chunk_id,"stbl",4)==0) {
               j=sub_chunk_len-8;
             } else if (memcmp(&sub_chunk_id,"stsd",4)==0) {
               n=read(fd,buf,sub_chunk_len-8);
               j-=sub_chunk_len;
               i=0;
               /* Skip version and flags */
               i+=4;

               numentries=(buf[i]<<24)|(buf[i+1]<<16)|(buf[i+2]<<8)|buf[i+3];
               i+=4;
               if (numentries!=1) {
                 logf("ERROR: Expecting only one entry in stsd\n");
               }

               entry_size=(buf[i]<<24)|(buf[i+1]<<16)|(buf[i+2]<<8)|buf[i+3];
               i+=4;

               /* Check the codec type - 'alac' for ALAC, 'mp4a' for AAC */
               if (memcmp(&buf[i],"alac",4)!=0) {
		     logf("Not an ALAC file\n");
                 return false;
               }

               //numchannels=(buf[i+20]<<8)|buf[i+21];   /* Not used - assume Stereo */
               //samplesize=(buf[i+22]<<8)|buf[i+23];    /* Not used - assume 16-bit */

               /* Samplerate is 32-bit fixed point, but this works for < 65536 Hz */
               id3->frequency=(buf[i+28]<<8)|buf[i+29];
             } else if (memcmp(&sub_chunk_id,"stts",4)==0) {
               j-=sub_chunk_len;
               i=8;
               n=read(fd,buf,8);
               i+=8;
               numentries=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
               for (k=0;k<numentries;k++) {
                  n=read_uint32be(fd,&sample_count);
                  n=read_uint32be(fd,&sample_duration);
                  totalsamples+=sample_count*sample_duration;
                  i+=8;
               }
               if (i > 0) lseek(fd, sub_chunk_len - i, SEEK_CUR);
             } else if (memcmp(&sub_chunk_id,"stsz",4)==0) {
               j-=sub_chunk_len;
               i=8;
               n=read(fd,buf,8);
               i+=8;
               numentries=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
               for (k=0;k<numentries;k++) {
                  n=read_uint32be(fd,&sample_count);
                  n=read_uint32be(fd,&sample_duration);
                  totalsamples+=sample_count*sample_duration;
                  i+=8;
               }
               if (i > 0) lseek(fd, sub_chunk_len - i, SEEK_CUR);
             } else {
               lseek(fd, sub_chunk_len - 8, SEEK_CUR); /* FIXME not 8 */
               j-=sub_chunk_len;
             }
           }
        } else {
          logf("Unexpected sub_chunk_id inside moov: %c%c%c%c\n",
             sub_chunk_id[0],sub_chunk_id[1],sub_chunk_id[2],sub_chunk_id[3]);
          return false;
        }
      }
    } else if (memcmp(&chunk_id,"mdat",4)==0) {
       /* once we hit mdat we stop reading and return.
        * this is on the assumption that there is no furhter interesting
        * stuff in the stream. if there is stuff will fail (:()).
        * But we need the read pointer to be at the mdat stuff
        * for the decoder. And we don't want to rely on fseek/ftell,
        * as they may not always be avilable */
       lseek(fd, chunk_len - 8, SEEK_CUR); /* FIXME not 8 */
       compressedsize=chunk_len-8;
    } else if (memcmp(&chunk_id,"free",4)==0) {
      /* these following atoms can be skipped !!!! */
      lseek(fd, chunk_len - 8, SEEK_CUR); /* FIXME not 8 */
    } else {
      logf("(top) unknown chunk id: %c%c%c%c\n", chunk_id[0],chunk_id[1],chunk_id[2],chunk_id[3]);
      return false;
    }
  }

  id3->vbr=true; /* All ALAC files are VBR */
  id3->filesize=filesize(fd);
  id3->samples=totalsamples;
  id3->length=(10*totalsamples)/(id3->frequency/100);
  id3->bitrate=(compressedsize*8)/id3->length;;

  return true;
}    

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
    
    suffix += 1;

    for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
    {
        if (strcasecmp(suffix, formats[i].extension) == 0)
        {
            return formats[i].format;
        }
    }
    
    return AFMT_UNKNOWN;
}

/* Get metadata for track - return false if parsing showed problems with the
 * file that would prevent playback.
 */
bool get_metadata(struct track_info* track, int fd, const char* trackname,
    bool v1first) 
{
    unsigned char* buf;
    unsigned long totalsamples;
    int bytesperframe;
    int i;
      
    /* Load codec specific track tag information. */
    
    switch (track->id3.codectype) 
    {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        if (mp3info(&track->id3, trackname, v1first))
        {
            return false;
        }

        break;

    case AFMT_FLAC:
        if (!get_flac_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_MPC:
        read_ape_tags(fd, &(track->id3));
        break;
    case AFMT_OGG_VORBIS:
        if (!get_vorbis_metadata(fd, &(track->id3)))
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
        /* A simple parser to read basic information from a WavPack file.
         * This will fail on WavPack files that don't have the WavPack header
         * as the first thing (i.e. self-extracting WavPack files) or WavPack
         * files that have so much extra RIFF data stored in the first block
         * that they don't have samples (very rare, I would think).
         */

        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = track->id3.path;
      
        if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 32) < 32))
        {
            return false;
        }

        if (memcmp (buf, "wvpk", 4) != 0 || buf [9] != 4 || buf [8] < 2) 
        {          
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

            totalsamples = get_long(&buf[12]);
            track->id3.length = totalsamples / (track->id3.frequency / 100) * 10;
            track->id3.bitrate = filesize (fd) / (track->id3.length / 8);
        }

        read_ape_tags(fd, &track->id3);     /* use any apetag info we find */
        break;

    case AFMT_A52:
        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = track->id3.path;
        
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
            bytesperframe=track->id3.bitrate * 2 * 2;
            break;
            
        case 0x40: 
            track->id3.frequency = 44100;
            bytesperframe = a52_441framesizes[i];
            break;
        
        case 0x80: 
            track->id3.frequency = 32000; 
            bytesperframe = track->id3.bitrate * 3 * 2;
            break;
        
        default: 
            logf("A52: Invalid samplerate code: 0x%02x\n", buf[4] & 0xc0);
            return false;
            break;
        }

        /* One A52 frame contains 6 blocks, each containing 256 samples */
        totalsamples = (track->filesize / bytesperframe) * 6 * 256;
        track->id3.length = (totalsamples / track->id3.frequency) * 1000;
        break;

    case AFMT_ALAC:
        if (!get_alac_metadata(fd, &(track->id3)))
        {
//            return false;
        }

        break;

    /* If we don't know how to read the metadata, just store the filename */
    default:
        break;
    }

    lseek(fd, 0, SEEK_SET);
    strncpy(track->id3.path, trackname, sizeof(track->id3.path));
    track->taginfo_ready = true;

    return true;
}
