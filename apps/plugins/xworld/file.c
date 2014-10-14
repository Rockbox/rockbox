/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"
#include "file.h"

void file_create(struct File* f, bool gzipped) {
    f->gzipped = gzipped;
    f->fd = -1;
    f->ioErr = false;
}

bool file_open(struct File* f, const char *filename, const char *directory, const char *mode) {
    char buf[512];
    rb->snprintf(buf, 512, "%s/%s", directory, filename);
    char *p = buf + rb->strlen(directory) + 1;
    string_lower(p);

    int flags = 0;
    for(int i = 0; mode[i]; ++i)
    {
        switch(mode[i])
        {
        case 'w':
            flags |= O_WRONLY | O_CREAT | O_TRUNC;
            break;
        case 'r':
            flags |= O_RDONLY;
            break;
        default:
            break;
        }
    }
    f->fd = -1;
    debug(DBG_FILE, "trying %s first", buf);
    f->fd = rb->open(buf, flags, 0666);
    if (f->fd < 0) { // let's try uppercase
        string_upper(p);
        debug(DBG_FILE, "now trying %s uppercase", buf);
        f->fd = rb->open(buf, flags, 0666);
    }
    if(f->fd > 0)
        return true;
    else
        return false;
}

void file_close(struct File* f) {
    if(f->gzipped)
    {
    }
    else
    {
        rb->close(f->fd);
    }
}

bool file_ioErr(struct File* f) {
    return f->ioErr;
}

void file_seek(struct File* f, int32_t off) {
    if(f->gzipped)
    {
    }
    else
    {
        rb->lseek(f->fd, off, SEEK_SET);
    }
}
int file_read(struct File* f, void *ptr, uint32_t size) {
    if(f->gzipped)
    {
        return -1;
    }
    else
    {
        unsigned int rc = rb->read(f->fd, ptr, size);
        if(rc != size)
            f->ioErr = true;
        return rc;
    }
}
uint8_t file_readByte(struct File* f) {
    uint8_t b;
    if(f->gzipped)
    {
        b = 0xff;
    }
    else
    {
        if(rb->read(f->fd, &b, 1) != 1)
        {
            f->ioErr = true;
            debug(DBG_FILE, "file read failed");
        }
    }
    return b;
}

uint16_t file_readUint16BE(struct File* f) {
    uint8_t hi = file_readByte(f);
    uint8_t lo = file_readByte(f);
    return (hi << 8) | lo;
}

uint32_t file_readUint32BE(struct File* f) {
    uint16_t hi = file_readUint16BE(f);
    uint16_t lo = file_readUint16BE(f);
    return (hi << 16) | lo;
}

int file_write(struct File* f, void *ptr, uint32_t size) {
    if(f->gzipped)
    {
        return 0;
    }
    else
    {
        return rb->write(f->fd, ptr, size);
    }
}

void file_writeByte(struct File* f, uint8_t b) {
    file_write(f, &b, 1);
}

void file_writeUint16BE(struct File* f, uint16_t n) {
    file_writeByte(f, n >> 8);
    file_writeByte(f, n & 0xFF);
}

void file_writeUint32BE(struct File* f, uint32_t n) {
    file_writeUint16BE(f, n >> 16);
    file_writeUint16BE(f, n & 0xFFFF);
}

void file_remove(const char* filename, const char* directory)
{
    char buf[512];
    rb->snprintf(buf, 512, "%s/%s", directory, filename);
    char *p = buf + rb->strlen(directory) + 1;
    string_lower(p);
    if(rb->file_exists(buf))
    {
        rb->remove(buf);
    }
    else
    {
        string_upper(p);
        rb->remove(buf);
    }
}
