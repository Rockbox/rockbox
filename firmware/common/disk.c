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
#include <stdio.h>
#include "ata.h"
#include "debug.h"
#include "fat.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif
#include "disk.h"

/* Partition table entry layout:
   -----------------------
   0: 0x80 - active
   1: starting head
   2: starting sector
   3: starting cylinder
   4: partition type
   5: end head
   6: end sector
   7: end cylinder
   8-11: starting sector (LBA)
   12-15: nr of sectors in partition
*/

#define BYTES2INT32(array,pos)					\
    ((long)array[pos] | (long)(array[pos+1] << 8 ) |		\
     ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

static struct partinfo part[8]; /* space for 4 partitions on 2 drives */

struct partinfo* disk_init(IF_MV_NONVOID(int drive))
{
    int i;
    unsigned char sector[512];
#ifdef HAVE_MULTIVOLUME
    /* For each drive, start at a different position, in order not to destroy 
       the first entry of drive 0. 
       That one is needed to calculate config sector position. */
    struct partinfo* pinfo = &part[drive*4];
    if ((size_t)drive >= sizeof(part)/sizeof(*part)/4)
        return NULL; /* out of space in table */
#else
    struct partinfo* pinfo = part;
#endif

    ata_read_sectors(IF_MV2(drive,) 0,1,&sector);

    /* check that the boot sector is initialized */
    if ( (sector[510] != 0x55) ||
         (sector[511] != 0xaa)) {
        DEBUGF("Bad boot sector signature\n");
        return NULL;
    }

    /* parse partitions */
    for ( i=0; i<4; i++ ) {
        unsigned char* ptr = sector + 0x1be + 16*i;
        pinfo[i].type  = ptr[4];
        pinfo[i].start = BYTES2INT32(ptr, 8);
        pinfo[i].size  = BYTES2INT32(ptr, 12);

        DEBUGF("Part%d: Type %02x, start: %08lx size: %08lx\n",
               i,pinfo[i].type,pinfo[i].start,pinfo[i].size);

        /* extended? */
        if ( pinfo[i].type == 5 ) {
            /* not handled yet */
        }
    }

    return pinfo;
}

struct partinfo* disk_partinfo(int partition)
{
    return &part[partition];
}

int disk_mount_all(void)
{
    struct partinfo* pinfo;
    int i,j;
    int mounted = 0;
    bool found;
    int drives = 1;
#ifdef HAVE_MMC
    if (mmc_detect()) /* for Ondio, only if card detected */
    {
        drives = 2; /* in such case we have two drives to try */
    }
#endif

    fat_init(); /* reset all mounted partitions */
    for (j=0; j<drives; j++)
    {
        found = false; /* reset partition-on-drive flag */
        pinfo = disk_init(IF_MV(j));
        if (pinfo == NULL)
        {
            continue;
        }
        for (i=0; mounted<NUM_VOLUMES && i<4; i++)
        {
            if (!fat_mount(IF_MV2(mounted,) IF_MV2(j,) pinfo[i].start))
            {
                mounted++;
                found = true; /* at least one valid entry */
            }
        }

        if (!found && mounted<NUM_VOLUMES) /* none of the 4 entries worked? */
        {   /* try "superfloppy" mode */
            DEBUGF("No partition found, trying to mount sector 0.\n");
            if (!fat_mount(IF_MV2(mounted,) IF_MV2(j,) 0))
            {
                mounted++;
            }
        }
    }

    return mounted;
}

