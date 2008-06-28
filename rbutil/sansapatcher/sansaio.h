/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Dave Chapman
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

#ifndef __SANSAIO_H
#define __SANSAIO_H

#include <stdint.h>
#include <unistd.h>

#ifdef __WIN32__
#include <windows.h>
#define loff_t int64_t
#else
#define HANDLE int
#define O_BINARY 0

/* Only Linux seems to need lseek64 and loff_t */
#if !defined(linux) && !defined (__linux)
#define loff_t off_t
#define lseek64 lseek
#endif

#endif

struct sansa_partinfo_t {
    unsigned long start; /* first sector (LBA) */
    unsigned long size;  /* number of sectors */
    int type;
};

struct mi4header_t {
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint32_t enctype;
    uint32_t mi4size;
    uint32_t plaintext;
};

struct sansa_t {
    HANDLE dh;
    char diskname[4096];
    int sector_size;
    struct sansa_partinfo_t pinfo[4];
    int hasoldbootloader;
    char* targetname; /* "e200" or "c200" */
    loff_t start;  /* Offset in bytes of firmware partition from start of disk */
};

void print_error(char* msg);
int sansa_open(struct sansa_t* sansa, int silent);
int sansa_reopen_rw(struct sansa_t* sansa);
int sansa_close(struct sansa_t* sansa);
int sansa_seek(struct sansa_t* sansa, loff_t pos);
int sansa_read(struct sansa_t* sansa, unsigned char* buf, int nbytes);
int sansa_write(struct sansa_t* sansa, unsigned char* buf, int nbytes);
int sansa_alloc_buffer(unsigned char** sectorbuf, int bufsize);

#endif
