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
#define __LIBRARY_FAT_VOLUME_C__

#include <fat.h>
#include "fat-mbr_sector.h"
#include "fat-bpb_sector.h"
#include "fat-fsi_sector.h"

///////////////////////////////////////////////////////////////////////////////////
// FAT VOLUME :
///////////////
//
//

// check fsi sector integrity
static int __fat_check_fsi_sector (struct fat_volume *volume,struct __fat_fsi_sector *fsi_sector,unsigned long lba)
  {
    int error;
    if (!lba)
      // no FSI sector
      {
        volume->next_free_cluster  = 2;
        return FAT_RETURN_SUCCESS;
      }
    if ((error = __fat_get_fsi_sector (volume->partition_start,lba,fsi_sector)) > 0)
      {
        if ((fsi_sector->signature      != 0x0000AA55) ||
            (fsi_sector->fsi_signature0 != 0x52524161) ||
            (fsi_sector->fsi_signature1 != 0x72726141))
          {
            return FAT_RETURN_BAD_FSI;
          }
        if (fsi_sector->left_free_clusters == -1)
          fsi_sector->next_free_cluster = 2;
        else if (fsi_sector->next_free_cluster >= volume->sectors_per_fat)
          return FAT_RETURN_BAD_FSI;
        volume->next_free_cluster  = fsi_sector->next_free_cluster;  
        fsi_sector->left_free_clusters = -1;
        fsi_sector->next_free_cluster  = 2;
        error = __fat_put_fsi_sector (volume->partition_start,lba,fsi_sector)));
      }
    return error;
  }

static inline int bit_in_range (int value,int min,int max)
  {
    for (;min < max; min <<= 1)
      if (value == min)
        return 1;
    return 0;
  }

// check bpb sector integrity
static int __fat_check_bpb_sector (struct fat_volume *volume,struct __fat_bpb_sector *bpb_sector,struct __fat_fsi_sector *fsi_sector)
  {
    long unsigned bpb_lba = 0,fsi_lba;
    long unsigned sectors_per_cluster,sectors_per_fat,sectors,reserved_sectors,total_sectors;
    long unsigned first_cluster_of_root,first_sector_of_fat,first_sector_of_data;
    long unsigned clusters_per_fat,bytes_per_sector;
    int error,backup;
    for (backup = 0; !backup ; backup = 1)
      {
        if ((error = __fat_get_bpb_sector (volume->partition_start,bpb_lba,bpb_sector)) > 0)
          {
            bytes_per_sector      = peekHI (bpb_sector->bytes_per_sector   );
            sectors_per_cluster   = peekQI (bpb_sector->sectors_per_cluster);
            sectors_per_fat       = peekSI (bpb_sector->sectors_per_fat    );
            sectors               = peekQI (bpb_sector->number_of_fats     ) * sectors_per_fat;
            reserved_sectors      = peekHI (bpb_sector->reserved_sectors   );
            total_sectors         = peekSI (bpb_sector->total_sectors      );
            first_cluster_of_root = peekSI (bpb_sector->root_cluster       );
            first_sector_of_fat   = reserved_sectors  + volume->partition_start;
            first_sector_of_data  = first_sector_of_fat + sectors;
            clusters_per_fat      = (total_sectors - first_sector_of_data) / sectors_per_cluster;

            if (!bpb_lba)
              {
                bpb_lba = peekHI(bpb_sector->backup_bpb);
                if (bpb_lba == -1)
                  bpb_lba = 0; 
              }

            if ((bpb_lba >= reserved_sectors)                                   ||
                (bpb_sector->signature != 0x000055AA)                           ||
                (clusters_per_fat < 65525)                                      ||
                (bytes_per_sector != 512)                                       ||
                (!bit_in_range (sectors_per_cluster,1,128))                     ||
                (bytes_per_sector * sectors_per_cluster >= 32 KB)               ||
                (peekHI (bpb_sector->total_sectors_16))                         ||
                (peekHI (bpb_sector->sectors_per_fat_16))                       ||
                (peekHI (bpb_sector->number_of_root_entries))                   ||
                ((bpb_sector->media[0] != 0xF0) && (bpb_sector->media[0] < 0xF8)))
              {
                error = FAT_RETURN_BAD_BPB; 
                if (bpb_lba) // try with backup BPB sector ?
                  continue;
                return error;
              }
            if ((signed char)bpb_sector->flags[0] >= 0)
              {
                bpb_sector->flags[0] = 0x80;
                if (!backup && (error = __fat_put_bpb_sector (volume->partition_start,0,bpb_sector)) <= 0)
                  return error;
                if ((error = __fat_put_bpb_sector (volume->partition_start,bpb_lba,bpb_sector)) <= 0)
                  return error;
              }

            volume->sectors_per_cluster   = sectors_per_cluster;
            volume->sectors_per_fat       = sectors_per_fat;
            volume->first_cluster_of_root = first_cluster_of_root;
            volume->first_sector_of_fat   = first_sector_of_fat;
            volume->first_sector_of_data  = first_sector_of_data;
            volume->clusters_per_fat      = clusters_per_fat;

            fsi_lba = ((long)peekHI(bpb_sector->filesystem_info));
            if (fsi_lba == -1)
              fsi_lba = 0;
            else if (fsi_lba >= reserved_sectors)
              {
                error = FAT_RETURN_BAD_FSI; 
                if (bpb_lba) // try with backup BPB sector ?
                  continue;
                return error;
              }
            
            if (((error = __fat_check_fsi_sector (volume,fsi_sector,fsi_lba + (backup ? 0 : bpb_lba))) <= 0) && bpb_lba)
              continue;

            if (backup)
              {
                error = __fat_put_bpb_sector (volume,0,bpb_sector)) <= 0);
                if (!error)
                  error = __fat_put_fsi_sector (volume,fsi_lba,fsi_sector)) <= 0);
              }

            break;
          }
      }
    return error;
  }

static inline int __fat_compare_volume_name (char const *name,struct fat_volume *volume)
  {
    return !name ? -1 : strncpy (name,volume->name,11);
  }

static struct fat_volume *__fat_splay_volume (struct fat_volume *root,char const *name)
  {
    struct fat_volume *down;
    struct fat_volume *less;
    struct fat_volume *more;
    struct fat_volume *head[2];
    ((struct fat_volume *)head)->less =
    ((struct fat_volume *)head)->more = 0;
    less =
    more = head;
    while (1)
      {
        int sign = __fat_compare_volume_name (name,root);
        if (sign < 0)
          {
            if ((down = root->less))
              {
                sign = __fat_compare_volume_name (name,down);
                if (sign < 0)
                  {
                    root->less = down->more;
                    down->more = root;
                    root = down;
                    if (!root->less)
                      break;
                  }
                more->less = root;
                more       = root;
                root       = root->less;
                continue;
              }
            break;
          }
        if (0 < sign)
          {
            if ((down = root->more))
              {
                sign = __fat_compare_volume_name (name,down);
                if (0 < sign)
                  {
                    root->more = down->less;
                    down->less = root;
                    root = down;
                    if (!root->more)
                      break;
                  }
                less->more = root;
                less       = root;
                root       = root->more;
                continue;
              }
          }
        break;
      }
    less->more = root->less;
    more->less = root->more;
    root->less = ((struct fat_volume *)head)->more;
    root->more = ((struct fat_volume *)head)->less;
    return root;
  }

static inline struct fat_volume *__fat_insert_volume (struct fat_volume *root,struct fat_volume *node)
  {
    if (!root)
      {
        node->less = 
        node->more = 0;
      }
    else if (node < (root = __fat_splay_volume (root,node->name)))
      {
        node->less = root->less;
        node->more = root;
        root->less = 0;
      }
    else if
      {
        node->less = root;
        node->more = root->more;
        node->more = 0;
      }
    return node;
  }

#if 0
static inline struct fat_volume *__fat_remove_volume (struct fat_volume *root,struct memory_free_page *node)
  {
    root = __fat_splay_volume (root,node->name);
    if (root->less)
      {        
        node = __fat_splay_volume (root->less,node->name);
        node->more = root->more;
      }
    else
      node = root->more;
    return node;
  }
#endif

static inline struct fat_volume *__fat_lookup_volume (struct fat_volume *root,char const *name)
  {
    return __fat_splay_volume (root,0);
  }

static struct fat_volume *__fat_first_volume (struct fat_volume *root)
  {
    struct fat_volume *down;
    struct fat_volume *less;
    struct fat_volume *more;
    struct fat_volume *head[2];
    ((struct fat_volume *)head)->less =
    ((struct fat_volume *)head)->more = 0;
    less =
    more = &head;
    if (root)
      while (1)
        {
          if ((down = root->less))
            {
              root->less = down->more;
              down->more = root;
              root = down;
              if (!root->less)
                break;
              more->less = root;
              more       = root;
              root       = root->less;
              continue;
            }
          break;
        }
    less->more = root->less;
    more->less = root->more;
    root->less = ((struct fat_volume *)head)->more;
    root->more = ((struct fat_volume *)head)->less;
    return root;
  }

static inline struct fat_volume *__fat_scan_volume (struct fat_volume *root,int next)
  {
    return __fat_first_volume (next ? root->more : root,0);
  }

static int __fat_build_volume_tree (struct fat_volume *root)
  {
    struct fat_volume *volume;
    int number = 4;
    struct __fat_partition_info *partition_info;
    struct __fat_mbr_sector mbr_sector;
    struct __fat_bpb_sector bpb_sector;
    struct __fat_fsi_sector fsi_sector;
    if (__fat_get_mbr_sector (&mbr_sector) <= 0)
      return 0;
    partition_info = mbr_sector.partition_table;
    for (;number-- > 0; ++partition_info) 
      {
        switch (partition_info->filesystem_type)
          {
          case 0x05: // extended partition - handle it as well
            {
              if (!__fat_build_volume_list (list))
                return 0;
              break;
            }
          case 0x0B: // FAT32 partitions
          case 0x0C:
            {
              if (!(volume = memory_allocate_page (0)))
                return 0;
              volume->next = 0;
              volume->partition_start   = partition_info->start; 
              volume->partition_sectors = partition_info->sectors; 
              if (__fat_check_bpb_sector (volume,&mbr_sector,&fsi_sector) > 0)
                {
                  dump_volume (volume);
                  *root = volume;
                  list = &volume->next;
                  break;
                }
              else
                memory_release_page (volume,0);
            }
          }
      }
    return 1;
  }

static struct fat_volume *__fat_volume_root;

void fat_setup (void)
  {
    //build_volume_list (&root);
  }
