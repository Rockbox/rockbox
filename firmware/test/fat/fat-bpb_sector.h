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
#ifndef __LIBRARY_FAT_BPB_SECTOR_H__
#define __LIBRARY_FAT_BPB_SECTOR_H__

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
// BPB SECTOR :
///////////////
// 
//

struct __fat_bpb_sector /* Bios Parameters Block Sector */
  {
    // jmp_boot has two valid ways to look like in a FAT BPB.
    // Either EBXX90 or E9XXXX.
    // Not used by Rockbox.
    unsigned char
      jmp_boot[3];

    // Creator system of the fat-drive.
    // Usually looks like "MSWIN4.x".
    char
      oem_name[8];

    // It should be 512 if you don't want any trouble
    // with Rockbox firmware.
    unsigned char
      bytes_per_sector[2];
 
    // Must be a power of two.
    unsigned char
      sectors_per_cluster[1];

    // Number of reserved sectors in the reserved region of the volume
    // starting at the first sector of the volume.
    // Usually 32 for FAT32.
    unsigned char
      reserved_sectors[2];

    // Number of FAT structures.
    // This value should always be 2.
    unsigned char
      number_of_fats[1];

    // For FAT32, this field must be set to zero.
    // Not used by Rockbox.
    unsigned char
      number_of_root_entries[2];

    // Must be zero for FAT32, since the real value
    // can be found in total_sectors.
    // Not used by Rockbox. 
    unsigned char
      total_sectors_16[2];

    // Not used by Rockbox.
    unsigned char
      media[1];

    // In FAT32 this must be zero.
    // Not used by Rockbox.
    unsigned char
      sectors_per_fat_16[2];

    // Sectors per track used on this media.
    // Not used by Rockbox.
    unsigned char
      sectors_per_track[2];

    // Number of heads used on this media.
    // Not used by Rockbox.
    unsigned char
      number_of_heads[2];

    // Number of hidden sectors.
    // Not used by Rockbox.
    unsigned char
      hidden_sectors[4];

    // Number of total sectors.
    // For FAT32 volumes, this must be specified.
    unsigned char
      total_sectors[4];

    // Here follows FAT12/16 or FAT32 specific data. */

    // This is the number of sectors for one FAT.
    unsigned char
      sectors_per_fat[4];

    // Extended FAT32 flags follow.
    unsigned char
      flags[2];
    // bits 15-8: reserved
    // mirroring, bit 7:
    //   0 -> FAT is mirrored at runtime into all FATs.
    //   1 -> only the one specified in the following field
    //        is active.
    //   Rockbox always sets it. 
    // bits 7-4 : reserved
    // active_fat, bits 3-0:
    //   this specifies the "active" FAT mentioned previously.
  
    // This specifies the file system version.
    // High byte is major number, low byte is minor.
    // The current version is 0.0.
    unsigned char
      filesystem_version[2];
  
    // This is set to the cluster number of the first cluster
    // of the root directory. Usually 2, but not required.
    unsigned char
      root_cluster[4];

    // This specifies the sector number of the 'FSINFO' structure
    // in the reserved area.
    unsigned char
      filesystem_info[2];

    // If zero, this specifies where the backup of bpb
    // can be found.
    // Usually 6.
    // No value other than 6 is recommended by Microsoft.
    unsigned char
      backup_bpb[2];

    // The following area should always be set to zero
    // when the volume is initialised.
    unsigned char
      zeros[12];

    // Drive number for BIOS.
    // Not used by Rockbox.
    unsigned char
      drive_number[0];

    // Reserved for Windows NT.
    // Should always be set to 0.
    unsigned char
      reserved_for_nt[0];

    // Extended boot signature.
    // If this is 0x29, the following three fields are present.
    unsigned char
      boot_signature[0];

    // Volume serial number.
    unsigned char
      volume_id[4];

    // Volume label.
    // This field must be updated when the volume label
    // in the root directory is updated.
    char
      volume_label[11];

    // One of the strings "FAT12", "FAT16" or "FAT32".
    // This can not be used to determine the type of the FAT,
    // but it should be updated when creating file systems.
    char
      filesystem_type[8];

    char
      reserved[420];

    long
      signature;
  };

static inline int __fat_get_bpb_sector (unsigned long partition_start,unsigned long lba,struct __fat_bpb_sector *bpb_sector)
  { return ata_read_sectors (partition_start + lba,1,bpb_sector,0); } 

static inline int __fat_put_bpb_sector (unsigned long partition_start,unsigned long lba,struct __fat_bpb_sector *bpb_sector)
  { return FAT_RETURN_SUCCESS && ata_write_sectors (partition_start + lba,1,bpb_sector,0); } 

//
///////////////////////////////////////////////////////////////////////////////////

#endif