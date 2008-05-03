/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Magnus Holmgren
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

#include "system.h"
#include "errno.h"
#include "id3.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "logf.h"
#include "debug.h"
#include "replaygain.h"

#define MP4_ID(a, b, c, d)  (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define MP4_3gp6 MP4_ID('3', 'g', 'p', '6')
#define MP4_aART MP4_ID('a', 'A', 'R', 'T')
#define MP4_alac MP4_ID('a', 'l', 'a', 'c')
#define MP4_calb MP4_ID(0xa9, 'a', 'l', 'b')
#define MP4_cART MP4_ID(0xa9, 'A', 'R', 'T')
#define MP4_cgrp MP4_ID(0xa9, 'g', 'r', 'p')
#define MP4_cgen MP4_ID(0xa9, 'g', 'e', 'n')
#define MP4_cnam MP4_ID(0xa9, 'n', 'a', 'm')
#define MP4_cwrt MP4_ID(0xa9, 'w', 'r', 't')
#define MP4_ccmt MP4_ID(0xa9, 'c', 'm', 't')
#define MP4_cday MP4_ID(0xa9, 'd', 'a', 'y')
#define MP4_disk MP4_ID('d', 'i', 's', 'k')
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
        *buffer_left > 0 ? *buffer_left - 1 : 0);
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
        unsigned int type;
        
        /* Read the (leading part of the) decoder config. */
        length = read_mp4_length(fd, size);
        length = MIN(length, *size);
        length = MIN(length, sizeof(buf));
        memset(buf, 0, sizeof(buf));
        read(fd, buf, length);
        *size -= length;
        
        /* Maybe time to write a simple read_bits function... */

        /* Decoder config format:
         * Object type           - 5 bits
         * Frequency index       - 4 bits
         * Channel configuration - 4 bits
         */
        bits = get_long_be(buf);
        type = bits >> 27;              /* Object type - 5 bits */
        index = (bits >> 23) & 0xf;     /* Frequency index - 4 bits */

        if (index < (sizeof(sample_rates) / sizeof(*sample_rates)))
        {
            id3->frequency = sample_rates[index];
        }
    
        if (type == 5)
        {
            DEBUGF("MP4: SBR\n");
            unsigned int old_index = index;

            sbr = true;
            index = (bits >> 15) & 0xf; /* Frequency index - 4 bits */
    
            if (index == 15)
            {
                /* 17 bits read so far... */
                bits = get_long_be(&buf[2]);
                id3->frequency = (bits >> 7) & 0x00ffffff;
            }
            else if (index < (sizeof(sample_rates) / sizeof(*sample_rates)))
            {
                id3->frequency = sample_rates[index];
            }
            
            if (old_index == index)
            {
                /* Downsampled SBR */
                id3->frequency *= 2;
            }
        }
        /* Skip 13 bits from above, plus 3 bits, then read 11 bits */
        else if ((length >= 4) && (((bits >> 5) & 0x7ff) == 0x2b7))
        {
            /* extensionAudioObjectType */
            DEBUGF("MP4: extensionAudioType\n");
            type = bits & 0x1f;         /* Object type - 5 bits*/
            bits = get_long_be(&buf[4]);
            
            if (type == 5)
            {
                sbr = bits >> 31;

                if (sbr)
                {
                    unsigned int old_index = index;
                    
                    /* 1 bit read so far */
                    index = (bits >> 27) & 0xf; /* Frequency index - 4 bits */

                    if (index == 15)
                    {
                        /* 5 bits read so far */
                        id3->frequency = (bits >> 3) & 0x00ffffff;
                    }
                    else if (index < (sizeof(sample_rates) / sizeof(*sample_rates)))
                    {
                        id3->frequency = sample_rates[index];
                    }

                    if (old_index == index)
                    {
                        /* Downsampled SBR */
                        id3->frequency *= 2;
                    }
                }
            }
        }
        
        if (!sbr && (id3->frequency <= 24000) && (length <= 2))
        {
            /* Double the frequency for low-frequency files without a "long" 
             * DecSpecificConfig header. The file may or may not contain SBR,
             * but here we guess it does if the header is short. This can
             * fail on some files, but it's the best we can do, short of 
             * decoding (parts of) the file.
             */
            id3->frequency *= 2;
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

        case MP4_aART:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                &id3->albumartist);
            break;

        case MP4_cgrp:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                &id3->grouping);
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

        case MP4_ccmt:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                &id3->comment);
            break;

        case MP4_cday:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                                &id3->year_string);
 
            /* Try to parse it as a year, for the benefit of the database.
             */
            if(id3->year_string)
            {
                id3->year = atoi(id3->year_string);
                if (id3->year < 1900)
                {
                    id3->year = 0;
                }
            }
            else
                id3->year = 0;

            break;

        case MP4_gnre:
            {
                unsigned short genre;
                
                read_mp4_tag(fd, size, (char*) &genre, sizeof(genre));
                id3->genre_string = id3_get_num_genre(betoh16(genre) - 1);
            }
            break;
        
        case MP4_cgen:
            read_mp4_tag_string(fd, size, &buffer, &buffer_left,
                &id3->genre_string);
            break;

        case MP4_disk:
            {
                unsigned short n[2];
                
                read_mp4_tag(fd, size, (char*) &n, sizeof(n));
                id3->discnum = betoh16(n[1]);
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
                    lseek(fd, sub_size - (sizeof(tag_name) - 1), SEEK_CUR);
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
                else if (strcasecmp(tag_name, "iTunSMPB") == 0)
                {
                    char value[TAG_VALUE_LENGTH];
                    char* value_p = value;
                    char* any;
                    unsigned int length = sizeof(value);

                    read_mp4_tag_string(fd, size, &value_p, &length, &any);
                    id3->lead_trim = get_itunes_int32(value, 1);
                    id3->tail_trim = get_itunes_int32(value, 2);
                    DEBUGF("AAC: lead_trim %d, tail_trim %d\n", 
                        id3->lead_trim, id3->tail_trim);
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

bool get_mp4_metadata(int fd, struct mp3entry* id3)
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
        DEBUGF("MP4 bitrate %d, frequency %ld Hz, length %ld ms\n",
            id3->bitrate, id3->frequency, id3->length);
    }
    else
    {
        logf("MP4 metadata error");
        DEBUGF("MP4 metadata error. errno %d, frequency %ld, filesize %ld\n",
            errno, id3->frequency, id3->filesize);
        return false;
    }

    return true;
}
