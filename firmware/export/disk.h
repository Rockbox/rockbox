/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _DISK_H_
#define _DISK_H_

#include "ata.h" /* for volume definitions */

struct partinfo {
    unsigned long start; /* first sector (LBA) */
    unsigned long size;  /* number of sectors */
    unsigned char type;
};

#define PARTITION_TYPE_FAT32     0x0b
#define PARTITION_TYPE_FAT32_LBA 0x0c
#define PARTITION_TYPE_FAT16     0x06

/* returns a pointer to an array of 8 partinfo structs */
struct partinfo* disk_init(IF_MV_NONVOID(int volume));
struct partinfo* disk_partinfo(int partition);
int disk_mount_all(void); /* returns the # of successful mounts */

#endif
