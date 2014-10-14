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

#ifndef __FILE_H__
#define __FILE_H__

#include "intern.h"

typedef struct File {
    int fd;
    bool gzipped;
    bool ioErr;
} File;

void file_create(struct File*, bool gzipped);

bool file_open(struct File*, const char *filename, const char *directory, const char *mode);
void file_close(struct File*);
bool file_ioErr(struct File*);
void file_seek(struct File*, int32_t off);
int file_read(struct File*, void *ptr, uint32_t size);
uint8_t file_readByte(struct File*);
uint16_t file_readUint16BE(struct File*);
uint32_t file_readUint32BE(struct File*);
int file_write(struct File*, void *ptr, uint32_t size);
void file_writeByte(struct File*, uint8_t b);
void file_writeUint16BE(struct File*, uint16_t n);
void file_writeUint32BE(struct File*, uint32_t n);

void file_remove(const char* filename, const char* directory);

#endif
