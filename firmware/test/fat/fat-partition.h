/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
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
#ifndef __LIBRARY_FAT_PARTITION_H__
#define __LIBRARY_FAT_PARTITION_H__
#include <ata/ata.h>

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
// PARTITION INFO :
///////////////////
// 
//

struct __fat_partition_info
  {
    // Absolute start sector in this partition :
	  // start = start_cylinder * heads * sectors + start_head * sectors + start_sector - 1
	  unsigned long
      start;

    // Number of sectors in this partition :
	  // sectors = end_cylinder * heads * sectors + end_head * sectors + end_sector - start_sector
	  unsigned long
      sectors;

    // File system type.
    // Must be a FAT32 file system type (0x0B or 0x0C)
    // for Rockbox.
	  char
      filesystem_type;

    // Is this partition bootable ?
    // Not used by Rockbox.
	  char
      bootable;

    // Not used by Rockbox.
	  unsigned char
      start_head;

    // Not used by Rockbox.
	  unsigned char
      start_cylinder;

    // Not used by Rockbox.
	  unsigned char
      start_sector;

    // Not used by Rockbox.
	  unsigned char
      end_head;
    
    // Not used by Rockbox.
	  unsigned char
      end_cylinder;

    // Not used by Rockbox.
	  unsigned char
      end_sector;
  };


// load partition info into memory
static inline void __fat_get_partition_info (struct partition_info *__fat_partition_info)
  {
    // 
    union { unsigned long si[4]; unsigned short hi[8]; unsigned char qi[16]; } words;
    short *data = words.hi,*end;
    for (end = data + 8; data < end; ++data)
      *data = HI(ATAR_DATA);
    partition_info->start           = swawSI(words.si[2]);
    partition_info->sectors         = swawSI(words.si[3]);
    partition_info->bootable        = words.qi[1];
    partition_info->filesystem_type = words.qi[5];
    partition_info->start_head      = words.qi[0];
    partition_info->start_cylinder  = words.qi[3];
    partition_info->start_sector    = words.qi[2];
    partition_info->end_head        = words.qi[4];
    partition_info->end_cylinder    = words.qi[7];
    partition_info->end_sector      = words.qi[6];
  }

// store partition info into harddisk
static inline void __fat_put_partition_info (struct partition_info *__fat_partition_info)
  {
    union { unsigned long si[4]; short hi[8]; unsigned char qi[16]; } words;
    short *data = words.hi,*end;
    words.si[2] = swawSI(partition_info->start);
    words.si[3] = swawSI(partition_info->sectors);
    words.qi[1] = partition_info->bootable;
    words.qi[5] = partition_info->filesystem_type;
    words.qi[0] = partition_info->start_head;
    words.qi[3] = partition_info->start_cylinder;
    words.qi[2] = partition_info->start_sector;
    words.qi[4] = partition_info->end_head;
    words.qi[7] = partition_info->end_cylinder;
    words.qi[6] = partition_info->end_sector;
    for (end = data + 8; data < end;)
      HI(ATAR_DATA) = *data++;
  }

// 
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// PARTITION TABLE :
////////////////////
// 
// 

// load the partition table from a mbr sector
static inline void __fat_get_partition_table (struct partition_info table[4])
  {
    struct partition_info *last;
    for (last = table + 4; table < last;)
      __fat_get_partition_info (table++);
  }

// store the partition table into a mbr sector
static inline void __fat_put_partition_table (struct partition_info const table[4])
  {
    struct partition_info const *last;
    for (last = table + 4; table < last;)
      __fat_put_partition_info (table++);
  }

// 
///////////////////////////////////////////////////////////////////////////////////

#endif