/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __LIBRARY_FAT_MBR_SECTOR_H__
#define __LIBRARY_FAT_MBR_SECTOR_H__
#include "fat-partition.h"

// [Alan]:
// I would like to draw your attention about the fact that SH1
// cannot use misaligned address access so you must be very cautious
// with structures stored in FAT32 partition because they come from
// PC world where misaligned address accesses are usual and not
// problematic. To avoid such a trouble, I decide to use special
// structures where fields are moved in such a way they can be
// accessed by SH1. It is possible thanks to the callback mechanism
// I use for reading or writing from/to an ATA device in ata.h/c.
// So don't be puzzled if those structures seem odd compared
// with the usual ones from PC world. I use this mechanism for structures
// 'partition_info', 'mbr_sector' and 'fsi_sector' for instance, but
// not for structure 'bpb_sector' which is too much complex to handle
// that way, I think.
// By the way, SH1 is big endian, not little endian as PC is.

///////////////////////////////////////////////////////////////////////////////////
// MBR SECTOR :
///////////////
//
// 

struct __fat_mbr_sector /* Master Boot Record Sector */
  {
    struct
      __fat_partition_info partition_table[4];
    short
      data[0x1BE/2];
    short
      end[0];
    short
      signature;
  };

int __fat_get_mbr_sector_callback (struct __fat_mbr_sector *mbr_sector);
int __fat_put_mbr_sector_callback (struct __fat_mbr_sector *mbr_sector);

static inline int __fat_get_mbr_sector (struct mbr_sector *__fat_mbr_sector)
  { return ata_read_sectors (0,1,mbr_sector,(int(*)(void *))__fat_get_mbr_sector_callback); }

static inline int __fat_put_mbr_sector (struct mbr_sector *__fat_mbr_sector)
  { return ata_write_sectors (0,1,mbr_sector,(int(*)(void *))__fat_put_mbr_sector_callback); }

//
///////////////////////////////////////////////////////////////////////////////////

#endif