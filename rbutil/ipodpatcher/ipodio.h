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

#ifndef __IPODIO_H
#define __IPODIO_H

#include <stdint.h>
#include <unistd.h>

#ifdef __WIN32__
#include <windows.h>
#else
#define HANDLE int
#define O_BINARY 0
#endif

/* The maximum number of images in a firmware partition - a guess... */
#define MAX_IMAGES 10

enum firmwaretype_t {
   FTYPE_OSOS = 0,
   FTYPE_RSRC,
   FTYPE_AUPD,
   FTYPE_HIBE
};

struct ipod_directory_t {
  enum firmwaretype_t ftype;
  int id;
  uint32_t devOffset; /* Offset of image relative to one sector into bootpart*/
  uint32_t len;
  uint32_t addr;
  uint32_t entryOffset;
  uint32_t chksum;
  uint32_t vers;
  uint32_t loadAddr;
};

/* A fake partition type - DOS partition tables can't include HFS partitions */
#define PARTTYPE_HFS 0xffff

struct partinfo_t {
  uint32_t start; /* first sector (LBA) */
  uint32_t size;  /* number of sectors */
  uint32_t type;
};

struct ipod_t {
    HANDLE dh;
    char diskname[4096];
    int sector_size;
    int sectors_per_track;
    int num_heads;
    struct ipod_directory_t ipod_directory[MAX_IMAGES];
    int nimages;
    off_t diroffset;
    off_t start;  /* Offset in bytes of firmware partition from start of disk */
    off_t fwoffset; /* Offset in bytes of start of firmware images from start of disk */
    struct partinfo_t pinfo[4];
    int modelnum;
    char* modelname;
    char* modelstr;
    char* targetname;
    int macpod;
#ifdef WITH_BOOTOBJS
    unsigned char* bootloader;
    int bootloader_len;
#endif
};

void print_error(char* msg);
int ipod_open(struct ipod_t* ipod, int silent);
int ipod_reopen_rw(struct ipod_t* ipod);
int ipod_close(struct ipod_t* ipod);
int ipod_seek(struct ipod_t* ipod, unsigned long pos);
ssize_t ipod_read(struct ipod_t* ipod, unsigned char* buf, int nbytes);
ssize_t ipod_write(struct ipod_t* ipod, unsigned char* buf, int nbytes);
int ipod_alloc_buffer(unsigned char** sectorbuf, int bufsize);

/* In fat32format.c */
int format_partition(struct ipod_t* ipod, int partition);

#endif
