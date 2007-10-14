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

#ifndef _SANSAPATCHER_H
#define _SANSAPATCHER_H

#include "sansaio.h"

/* Size of buffer for disk I/O - 8MB is large enough for any version
   of the Apple firmware, but not the Nano's RSRC image. */
#define BUFFER_SIZE 8*1024*1024
extern unsigned char* sectorbuf;

#define FILETYPE_MI4 0
#define FILETYPE_INTERNAL 1

int sansa_read_partinfo(struct sansa_t* sansa, int silent);
int is_sansa(struct sansa_t* sansa);
int sansa_scan(struct sansa_t* sansa);
int sansa_read_firmware(struct sansa_t* sansa, char* filename);
int sansa_add_bootloader(struct sansa_t* sansa, char* filename, int type);
int sansa_delete_bootloader(struct sansa_t* sansa);
int sansa_update_of(struct sansa_t* sansa,char* filename);
int sansa_update_ppbl(struct sansa_t* sansa,char* filename);
void sansa_list_images(struct sansa_t* sansa);

#endif
