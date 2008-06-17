/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ipodpatcher.c 12237 2007-02-08 21:31:38Z dave $
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

#ifndef _IPODPATCHER_H
#define _IPODPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ipodio.h"

/* Size of buffer for disk I/O - 8MB is large enough for any version
   of the Apple firmware, but not the Nano's RSRC image. */
#define BUFFER_SIZE 8*1024*1024
extern unsigned char* ipod_sectorbuf;
extern int ipod_verbose;

#define FILETYPE_DOT_IPOD 0
#define FILETYPE_DOT_BIN  1
#ifdef WITH_BOOTOBJS
  #define FILETYPE_INTERNAL 2
#endif

char* get_parttype(int pt);
int read_partinfo(struct ipod_t* ipod, int silent);
int read_partition(struct ipod_t* ipod, int outfile);
int write_partition(struct ipod_t* ipod, int infile);
int diskmove(struct ipod_t* ipod, int delta);
int add_bootloader(struct ipod_t* ipod, char* filename, int type);
int delete_bootloader(struct ipod_t* ipod);
int write_firmware(struct ipod_t* ipod, char* filename, int type);
int read_firmware(struct ipod_t* ipod, char* filename, int type);
int read_directory(struct ipod_t* ipod);
int list_images(struct ipod_t* ipod);
int getmodel(struct ipod_t* ipod, int ipod_version);
int ipod_scan(struct ipod_t* ipod);
int write_dos_partition_table(struct ipod_t* ipod);
int read_aupd(struct ipod_t* ipod, char* filename);
int write_aupd(struct ipod_t* ipod, char* filename);
off_t filesize(int fd);

#ifdef __cplusplus
}
#endif
#endif

