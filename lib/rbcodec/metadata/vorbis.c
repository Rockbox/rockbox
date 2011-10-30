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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "platform.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "structec.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

struct file
{
    int fd;
    bool packet_ended;
    long packet_remaining;
};


/* Read an Ogg page header. file->packet_remaining is set to the size of the
 * first packet on the page; file->packet_ended is set to true if the packet
 * ended on the current page. Returns true if the page header was
 * successfully read.
 */
static bool file_read_page_header(struct file* file)
{
    unsigned char buffer[64];
    ssize_t table_left;

    /* Size of page header without segment table */
    if (read(file->fd, buffer, 27) != 27)
    {
        return false;
    }
        
    if (memcmp("OggS", buffer, 4))
    {
        return false;
    }

    /* Skip pattern (4), version (1), flags (1), granule position (8),
     * serial (4), pageno (4), checksum (4)
     */
    table_left = buffer[26];
    file->packet_remaining = 0;

    /* Read segment table for the first packet */
    do
    {
        ssize_t count = MIN(sizeof(buffer), (size_t) table_left);
        int i;

        if (read(file->fd, buffer, count) < count)
        {
            return false;
        }
        
        table_left -= count;
        
        for (i = 0; i < count; i++)
        {
            file->packet_remaining += buffer[i];
            
            if (buffer[i] < 255)
            {
                file->packet_ended = true;
                
                /* Skip remainder of the table */
                if (lseek(file->fd, table_left, SEEK_CUR) < 0)
                {
                    return false;
                }
                
                table_left = 0;
                break;
            }
        }
    }
    while (table_left > 0);
    
    return true;
}


/* Read (up to) buffer_size of data from the file. If buffer is NULL, just
 * skip ahead buffer_size bytes (like lseek). Returns number of bytes read,
 * 0 if there is no more data to read (in the packet or the file), < 0 if a
 * read error occurred.
 */
static ssize_t file_read(struct file* file, void* buffer, size_t buffer_size)
{
    ssize_t done = 0;
    ssize_t count = -1;
    
    do
    {
        if (file->packet_remaining <= 0)
        {
            if (file->packet_ended)
            {
                break;
            }

            if (!file_read_page_header(file))
            {
                count = -1;
                break;
            }
        }

        count = MIN(buffer_size, (size_t) file->packet_remaining);

        if (buffer)
        {
            count = read(file->fd, buffer, count);
        }
        else
        {
            if (lseek(file->fd, count, SEEK_CUR) < 0)
            {
                count = -1;
            }
        }
        
        if (count <= 0)
        {
            break;
        }

        if (buffer)
        {
            buffer += count;
        }

        buffer_size -= count;
        done += count;
        file->packet_remaining -= count;
    }
    while (buffer_size > 0);

    return (count < 0 ? count : done);
}


/* Read an int32 from file. Returns false if a read error occurred.
 */
static bool file_read_int32(struct file* file, int32_t* value)
{
    char buf[sizeof(int32_t)];

    if (file_read(file, buf, sizeof(buf)) < (ssize_t) sizeof(buf))
    {
        return false;
    }
    
    *value = get_long_le(buf);
    return true;
}


/* Read a string from the file. Read up to buffer_size bytes, or, if eos
 * != -1, until the eos character is found (eos is not stored in buf,
 * unless it is nil). Writes up to buffer_size chars to buf, always
 * terminating with a nil. Returns number of chars read or < 0 if a read
 * error occurred.
 *
 * Unfortunately this is a slightly modified copy of read_string() in
 * metadata_common.c...
 */
static long file_read_string(struct file* file, char* buffer,
    long buffer_size, int eos, long size)
{
    long read_bytes = 0;
    
    while (size > 0)
    {
        char c;

        if (file_read(file, &c, 1) != 1)
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
        
        if (buffer_size > 1)
        {
            *buffer++ = c;
            buffer_size--;
        }
        else if (eos == -1)
        {
            /* No point in reading any more, skip remaining data */
            if (file_read(file, NULL, size) < 0)
            {
                read_bytes = -1;
            }
            else
            {
                read_bytes += size;
            }
            
            break;
        }
    }
    
    *buffer = 0;
    return read_bytes;
}


/* Init struct file for reading from fd. type is the AFMT_* codec type of
 * the file, and determines if Ogg pages are to be read. remaining is the
 * max amount to read if codec type is FLAC; it is ignored otherwise.
 * Returns true if the file was successfully initialized.
 */
static bool file_init(struct file* file, int fd, int type, int remaining)
{
    memset(file, 0, sizeof(*file));
    file->fd = fd;
    
    if (type == AFMT_OGG_VORBIS || type == AFMT_SPEEX)
    {
        if (!file_read_page_header(file))
        {
            return false;
        }
    }

    if (type == AFMT_OGG_VORBIS)
    {
        char buffer[7];

        /* Read packet header (type and id string) */
        if (file_read(file, buffer, sizeof(buffer)) < (ssize_t) sizeof(buffer))
        {
            return false;
        }

        /* The first byte of a packet is the packet type; comment packets
         * are type 3.
         */
        if (buffer[0] != 3)
        {
            return false;
        }
    }
    else if (type == AFMT_FLAC)
    {
        file->packet_remaining = remaining;
        file->packet_ended = true;
    }
    
    return true;
}


/* Read the items in a Vorbis comment packet. For Ogg files, the file must
 * be located on a page start, for other files, the beginning of the comment
 * data (i.e., the vendor string length). Returns total size of the
 * comments, or 0 if there was a read error.
 */
long read_vorbis_tags(int fd, struct mp3entry *id3, 
    long tag_remaining)
{
    struct file file;
    char *buf = id3->id3v2buf;
    int32_t comment_count;
    int32_t len;
    long comment_size = 0;
    int buf_remaining = sizeof(id3->id3v2buf) + sizeof(id3->id3v1buf);
    int i;
    
    if (!file_init(&file, fd, id3->codectype, tag_remaining))
    {
        return 0;
    }
    
    /* Skip vendor string */

    if (!file_read_int32(&file, &len) || (file_read(&file, NULL, len) < 0))
    {
        return 0;
    }

    if (!file_read_int32(&file, &comment_count))
    {
        return 0;
    }
    
    comment_size += 4 + len + 4;

    for (i = 0; i < comment_count && file.packet_remaining > 0; i++)
    {
        char name[TAG_NAME_LENGTH];
        int32_t read_len;

        if (!file_read_int32(&file, &len))
        {
            return 0;
        }
        
        comment_size += 4 + len;
        read_len = file_read_string(&file, name, sizeof(name), '=', len);
        
        if (read_len < 0)
        {
            return 0;
        }

        len -= read_len;
        read_len = file_read_string(&file, id3->path, sizeof(id3->path), -1, len);

        if (read_len < 0)
        {
            return 0;
        }

        logf("Vorbis comment %d: %s=%s", i, name, id3->path);

        /* Is it an embedded cuesheet? */
        if (!strcasecmp(name, "CUESHEET"))
        {
            id3->has_embedded_cuesheet = true;
            id3->embedded_cuesheet.pos = lseek(file.fd, 0, SEEK_CUR) - read_len;
            id3->embedded_cuesheet.size = len;
            id3->embedded_cuesheet.encoding = CHAR_ENC_UTF_8;
        }
        else
        {
            len = parse_tag(name, id3->path, id3, buf, buf_remaining,
                TAGTYPE_VORBIS);
        }

        buf += len;
        buf_remaining -= len;
    }

    /* Skip to the end of the block (needed by FLAC) */
    if (file.packet_remaining)
    {
        if (file_read(&file, NULL, file.packet_remaining) < 0)
        {
            return 0;
        }
    }

    return comment_size;
}
