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
#include <fat.h>
#include "fat-fsi_sector.h"

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

int __fat_get_fsi_sector_callback (struct __fat_fsi_sector *fsi_sector)
  {
    short *data = fsi_sector->data,*end;
    union { unsigned long si[2]; unsigned short hi[4]; unsigned char qi[8]; } words;
    for (end = fsi_sector->end0; data < end; ++data)
      *data = ata_get_word (0);
#ifdef __little__
    words.hi[0] = ata_get_word (0);
    words.hi[1] = ata_get_word (0);
    words.hi[2] = ata_get_word (0);
    words.hi[3] = ata_get_word (0);
#else
    words.hi[1] = ata_get_word (0);
    words.hi[0] = ata_get_word (0);
    words.hi[3] = ata_get_word (0);
    words.hi[2] = ata_get_word (0);
#endif
    for (end = fsi_sector->end1; data < end; ++data)
      *data = ata_get_word (0);
#ifdef __little__
    fsi_sector->left_free_clusters = words.si[0];
    fsi_sector->next_free_cluster  = words.si[1];
#else
    fsi_sector->left_free_clusters = swawSI (words.si[0]);
    fsi_sector->next_free_cluster  = swawSI (words.si[1]);
#endif
    return ATA_RETURN_SUCCESS;
  }

int __fat_put_fsi_sector_callback (struct __fat_fsi_sector *fsi_sector)
  {
    short *data = fsi_sector->data,*end;
    union { unsigned long si[2]; unsigned short hi[4]; unsigned char qi[8]; } words;
#ifdef __little__
    words.si[0] = swawSI (fsi_sector->left_free_clusters);
    words.si[1] = swawSI (fsi_sector->next_free_cluster);
#else
    words.si[0] = swawSI (fsi_sector->left_free_clusters);
    words.si[1] = swawSI (fsi_sector->next_free_cluster);
#endif
    for (end = fsi_sector->end0; data < end;)
      ata_put_word (*data++);
#ifdef __little__
    ata_put_word (words.hi[0],0);
    ata_put_word (words.hi[1],0);
    ata_put_word (words.hi[2],0);
    ata_put_word (words.hi[3],0);
#else
    ata_put_word (words.hi[1],0);
    ata_put_word (words.hi[0],0);
    ata_put_word (words.hi[3],0);
    ata_put_word (words.hi[2],0);
#endif
    for (end = fsi_sector->end1; data < end;)
      ata_put_word (*data++);
    return ATA_RETURN_SUCCESS;
  }

// 
///////////////////////////////////////////////////////////////////////////////////

#endif