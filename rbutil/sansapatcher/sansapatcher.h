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

#ifndef _SANSAPATCHER_H
#define _SANSAPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sansaio.h"

/* exit codes */
#define SANSA_OK 0
#define SANSA_WRONG_ARGUMENTS 1
#define SANSA_OPEN_INFILE_FAILED 2
#define SANSA_PARTITION_ERROR 3
#define SANSA_CANNOT_REOPEN 5
#define SANSA_INSTALL_FAILED 6
#define SANSA_UNINSTALL_FAILED 7
#define SANSA_ACCESS_DENIED 10
#define SANSA_NOT_FOUND 11
#define SANSA_WRONG_DEVICE_COUNT 12
#define SANSA_MULTIPLE_DEVICES 15
#define SANSA_WRONG_TYPE 16
#define SANSA_OLD_INSTALL 17
#define SANSA_INTERNAL_ERROR 20

extern int sansa_verbose;
/* Size of buffer for disk I/O - 8MB is large enough for any version
   of the Apple firmware, but not the Nano's RSRC image. */
#define BUFFER_SIZE 8*1024*1024

int sansa_read_partinfo(struct sansa_t* sansa, int silent);
int is_sansa(struct sansa_t* sansa);
int sansa_scan(struct sansa_t* sansa);
int sansa_read_firmware(struct sansa_t* sansa, const char* filename);
unsigned int sansa_read_bootloader(struct sansa_t* sansa, const char* filename, unsigned char** bl_buffer);
int sansa_add_bootloader(struct sansa_t* sansa, const unsigned char* buf, unsigned int len);
int sansa_delete_bootloader(struct sansa_t* sansa);
int sansa_update_of(struct sansa_t* sansa,const char* filename);
int sansa_update_ppbl(struct sansa_t* sansa,const char* filename);
int sansa_list_images(struct sansa_t* sansa);

#ifdef __cplusplus
}
#endif
#endif

