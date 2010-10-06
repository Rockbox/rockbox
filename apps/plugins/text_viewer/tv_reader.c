/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "tv_preferences.h"
#include "tv_reader.h"

#if PLUGIN_BUFFER_SIZE < 0x10000
#define TV_MIN_BLOCK_SIZE 0x800
#else
#define TV_MIN_BLOCK_SIZE 0x1000
#endif

/* UTF-8 BOM */
#define BOM "\xef\xbb\xbf"
#define BOM_SIZE 3

static int fd = -1;

static off_t file_pos;
static off_t start_file_pos;

static off_t file_size;

static unsigned char *reader_buffer;
static ssize_t buffer_size;
static ssize_t block_size;

static ssize_t buf_pos;
static ssize_t read_size;

off_t tv_get_file_size(void)
{
    return file_size;
}

bool tv_is_eof(void)
{
    return (file_pos + buf_pos >= file_size);
}

off_t tv_get_current_file_pos(void)
{
    return file_pos + buf_pos;
}

const unsigned char *tv_get_buffer(ssize_t *bufsize)
{
    *bufsize = read_size - buf_pos;
    return reader_buffer + buf_pos;
}

static ssize_t tv_read(unsigned char *buf, ssize_t reqsize)
{
    if (buf - reader_buffer + reqsize > buffer_size)
        reqsize = buffer_size - (buf - reader_buffer);

    return rb->read(fd, buf, reqsize);
}

void tv_seek(off_t offset, int whence)
{
    ssize_t size;

    switch (whence)
    {
        case SEEK_SET:
            if (offset >= file_pos && offset < file_pos + read_size)
            {
                buf_pos = offset - file_pos;
                return;
            }
            file_pos = offset;
            break;

        case SEEK_CUR:
            buf_pos += offset;
            if (buf_pos >= 0 && buf_pos < read_size)
            {
                if (buf_pos > block_size)
                {
                    buf_pos  -= block_size;
                    file_pos += block_size;
                    size = read_size - block_size;
                    rb->memcpy(reader_buffer, reader_buffer + block_size, size);
                    read_size = tv_read(reader_buffer + block_size, block_size);
                    if (read_size < 0)
                        read_size = 0;

                    read_size += size;
                }
                return;
            }
            file_pos += buf_pos;
            break;

        default:
            return;
            break;
    }

    if (whence == SEEK_SET)
    {
        if (file_pos < 0)
            file_pos = 0;
        else if (file_pos > file_size)
            file_pos = file_size;

        rb->lseek(fd, file_pos + start_file_pos, SEEK_SET);
        buf_pos = 0;
        read_size = tv_read(reader_buffer, buffer_size);
    }
}

static int tv_change_preferences(const struct tv_preferences *oldp)
{
    bool change_file = false;

    /* open the new file */
    if (oldp == NULL || rb->strcmp(oldp->file_name, preferences->file_name))
    {
        if (fd >= 0)
            rb->close(fd);

        fd = rb->open(preferences->file_name, O_RDONLY);
        if (fd < 0)
            return TV_CALLBACK_ERROR;

        file_size = rb->filesize(fd);
        change_file = true;
    }

    /*
     * When a file is UTF-8 file with BOM, if encoding is UTF-8,
     * then file size decreases only BOM_SIZE.
     */
    if (change_file || oldp->encoding != preferences->encoding)
    {
        int old_start_file_pos = start_file_pos;
        int delta_start_file_pos;
        off_t cur_file_pos = file_pos + buf_pos;

        file_pos  = 0;
        buf_pos   = 0;
        read_size = 0;
        start_file_pos = 0;

        if (preferences->encoding == UTF_8)
        {
            unsigned char bom[BOM_SIZE];

            rb->lseek(fd, 0, SEEK_SET);
            rb->read(fd, bom, BOM_SIZE);
            if (rb->memcmp(bom, BOM, BOM_SIZE) == 0)
                start_file_pos = BOM_SIZE;
        }

        delta_start_file_pos = old_start_file_pos - start_file_pos;
        file_size += delta_start_file_pos;
        tv_seek(cur_file_pos + delta_start_file_pos, SEEK_SET);
    }

    return TV_CALLBACK_OK;
}

bool tv_init_reader(unsigned char **buf, size_t *size)
{
    if (*size < 2 * TV_MIN_BLOCK_SIZE)
        return false;

    block_size    = *size / 2;
    buffer_size   = 2 * block_size;
    reader_buffer = *buf;

    *buf += buffer_size;
    *size -= buffer_size;

    tv_add_preferences_change_listner(tv_change_preferences);

    return true;
}

void tv_finalize_reader(void)
{
    if (fd >= 0)
        rb->close(fd);
}
