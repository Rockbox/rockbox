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

#include "sansaio.h"

/* Size of buffer for disk I/O - 8MB is large enough for any version
   of the Apple firmware, but not the Nano's RSRC image. */
#define BUFFER_SIZE 8*1024*1024
extern unsigned char* sectorbuf;

#define FILETYPE_MI4 0
#ifdef WITH_BOOTOBJS
  #define FILETYPE_INTERNAL 1
#endif

char* get_parttype(int pt);
int read_partinfo(struct sansa_t* sansa, int silent);
off_t filesize(int fd);
int is_e200(struct sansa_t* sansa);
int sansa_scan(struct sansa_t* sansa);
int read_firmware(struct sansa_t* sansa, char* filename);
int add_bootloader(struct sansa_t* sansa, char* filename, int type);
int delete_bootloader(struct sansa_t* sansa);
void list_images(struct sansa_t* sansa);

#endif
