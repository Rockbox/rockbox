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
#ifndef __LIBRARY_FAT_FSI_SECTOR_H__
#define __LIBRARY_FAT_FSI_SECTOR_H__

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
// FSI SECTOR :
///////////////
// 
//

struct __fat_fsi_sector /* File System Info Sector */
  {
    unsigned long
      left_free_clusters;
    unsigned long
      next_free_cluster;
    short
      data[0];
    long /* 0x61415252 - aARR */
      fsi_signature0;
    char
      reserved0[480];
    long /* 0x41617272 - Aarr */
      fsi_signature1;
    short
      end0[0];
    char
      reserved1[12];
    long /* 0x000055AA */
      signature;
    short
      end1[0];
  };

int __fat_get_fsi_sector_callback (struct __fat_fsi_sector *fsi_sector);
int __fat_put_fsi_sector_callback (struct __fat_fsi_sector *fsi_sector);

static inline int __fat_get_fsi_sector (unsigned long partition_start,unsigned long lba,struct __fat_fsi_sector *fsi_sector)
  { return ata_read_sectors (partition_start + lba,1,fsi_sector,(int(*)(void *))get_fsi_sector_callback); } 

static inline int __fat_put_fsi_sector (unsigned long partition_start,unsigned long lba,struct __fat_fsi_sector *fsi_sector)
  { return ata_write_sectors (partition_start + lba,1,fsi_sector,(int(*)(void *))put_fsi_sector_callback); } 

// 
///////////////////////////////////////////////////////////////////////////////////

#endif