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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __IPODIO_H
#define __IPODIO_H

#include <stdint.h>

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

struct partinfo_t {
  unsigned long start; /* first sector (LBA) */
  unsigned long size;  /* number of sectors */
  int type;
};

struct ipod_t {
    HANDLE dh;
    char diskname[4096];
    int sector_size;
    struct ipod_directory_t ipod_directory[MAX_IMAGES];
    int nimages;
    off_t diroffset;
    off_t start;  /* Offset in bytes of firmware partition from start of disk */
    off_t fwoffset; /* Offset in bytes of start of firmware images from start of disk */
    struct partinfo_t pinfo[4];
    int modelnum;
    char* modelname;
    char* modelstr;
    int macpod;
};

void print_error(char* msg);
int ipod_open(struct ipod_t* ipod, int silent);
int ipod_reopen_rw(struct ipod_t* ipod);
int ipod_close(struct ipod_t* ipod);
int ipod_seek(struct ipod_t* ipod, unsigned long pos);
int ipod_read(struct ipod_t* ipod, unsigned char* buf, int nbytes);
int ipod_write(struct ipod_t* ipod, unsigned char* buf, int nbytes);
int ipod_alloc_buffer(unsigned char** sectorbuf, int bufsize);

#endif
