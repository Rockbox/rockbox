/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * FAT32 formatting functions.  Based on:
 *
 * Fat32 formatter version 1.03
 * (c) Tom Thornhill 2005
 * This software is covered by the GPL. 
 * By using this tool, you agree to absolve Ridgecrop of an liabilities for 
 * lost data.
 * Please backup any data you value before using this tool.
 *
 *
 * Modified June 2007 by Dave Chapman for use in ipodpatcher
 *
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "ipodio.h"

static inline uint16_t swap16(uint16_t value)
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32(uint32_t value)
{
    uint32_t hi = swap16(value >> 16);
    uint32_t lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

/* The following functions are not the most efficient, but are
   self-contained and don't require needing to know endianness of CPU
   at compile-time.
*/

uint16_t htole16(uint16_t x)
{
    uint16_t test = 0x1234;
    unsigned char* p = (unsigned char*)&test;
    
    if (p[0]==0x12) {
        /* Big-endian */
        return swap16(x);
    } else {
        return x;
    }
}

uint32_t htole32(uint32_t x)
{
    uint32_t test = 0x12345678;
    unsigned char* p = (unsigned char*)&test;
    
    if (p[0]==0x12) {
        /* Big-endian */
        return swap32(x);
    } else {
        return x;
    }
}


/* A large aligned buffer for disk I/O */
extern unsigned char* sectorbuf;

/* TODO: Pass these as parameters to the various create_ functions */

/* can be zero for default or 1,2,4,8,16,32 or 64 */
static int sectors_per_cluster = 0;

/* Recommended values */
static uint32_t ReservedSectCount = 32;
static uint32_t NumFATs = 2;
static uint32_t BackupBootSect = 6;
static uint32_t VolumeId=0; /* calculated before format */
    
/* Calculated later */
static uint32_t FatSize=0; 
static uint32_t BytesPerSect=0;
static uint32_t SectorsPerCluster=0;
static uint32_t TotalSectors=0;
static uint32_t SystemAreaSize=0;
static uint32_t UserAreaSize=0;
static uint8_t VolId[12] = "NO NAME    ";


struct FAT_BOOTSECTOR32
{
    /* Common fields. */
    uint8_t sJmpBoot[3];
    char sOEMName[8];
    uint16_t wBytsPerSec;
    uint8_t bSecPerClus;
    uint16_t wRsvdSecCnt;
    uint8_t bNumFATs;
    uint16_t wRootEntCnt;
    uint16_t wTotSec16;  /* if zero, use dTotSec32 instead */
    uint8_t bMedia;
    uint16_t wFATSz16;
    uint16_t wSecPerTrk;
    uint16_t wNumHeads;
    uint32_t dHiddSec;
    uint32_t dTotSec32;

    /* Fat 32/16 only */
    uint32_t dFATSz32;
    uint16_t wExtFlags;
    uint16_t wFSVer;
    uint32_t dRootClus;
    uint16_t wFSInfo;
    uint16_t wBkBootSec;
    uint8_t Reserved[12];
    uint8_t bDrvNum;
    uint8_t Reserved1;
    uint8_t bBootSig;  /* == 0x29 if next three fields are ok */
    uint32_t dBS_VolID;
    uint8_t sVolLab[11];
    uint8_t sBS_FilSysType[8];
} __attribute__((packed));

struct FAT_FSINFO {
    uint32_t dLeadSig;         // 0x41615252
    uint8_t sReserved1[480];   // zeros
    uint32_t dStrucSig;        // 0x61417272
    uint32_t dFree_Count;      // 0xFFFFFFFF
    uint32_t dNxt_Free;        // 0xFFFFFFFF
    uint8_t sReserved2[12];    // zeros
    uint32_t dTrailSig;     // 0xAA550000
} __attribute__((packed));


/* Write "count" zero sectors, starting at sector "sector" */
static int zero_sectors(struct ipod_t* ipod, uint64_t sector, int count)
{
    int n;

    if (ipod_seek(ipod, sector * ipod->sector_size) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    memset(sectorbuf, 0, 128 * ipod->sector_size);

    /* Write 128 sectors at a time */
    while (count) {
        if (count >= 128)
            n = 128;
        else
            n = count;

        if (ipod_write(ipod,sectorbuf,n * ipod->sector_size) < 0) {
            perror("[ERR]  Write failed in zero_sectors\n");
            return -1;
        }

        count -= n;
    }

    return 0;
}


/*
28.2  CALCULATING THE VOLUME SERIAL NUMBER

For example, say a disk was formatted on 26 Dec 95 at 9:55 PM and 41.94
seconds.  DOS takes the date and time just before it writes it to the
disk.

Low order word is calculated:               Volume Serial Number is:
    Month & Day         12/26   0c1ah
    Sec & Hundrenths    41:94   295eh               3578:1d02
                                -----
                                3578h

High order word is calculated:
    Hours & Minutes     21:55   1537h
    Year                1995    07cbh
                                -----
                                1d02h
*/
static uint32_t get_volume_id ( )
{
    /* TODO */
#if 0
    SYSTEMTIME s;
    uint32_t d;
    uint16_t lo,hi,tmp;

    GetLocalTime( &s );

    lo = s.wDay + ( s.wMonth << 8 );
    tmp = (s.wMilliseconds/10) + (s.wSecond << 8 );
    lo += tmp;

    hi = s.wMinute + ( s.wHour << 8 );
    hi += s.wYear;
   
    d = lo + (hi << 16);
    return(d);
#endif
    return(0);
}

/*
This is the Microsoft calculation from FATGEN
    
    uint32_t RootDirSectors = 0;
    uint32_t TmpVal1, TmpVal2, FATSz;

    TmpVal1 = DskSize - ( ReservedSecCnt + RootDirSectors);
    TmpVal2 = (256 * SecPerClus) + NumFATs;
    TmpVal2 = TmpVal2 / 2;
    FATSz = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;

    return( FatSz );
*/


static uint32_t get_fat_size_sectors(uint32_t DskSize, uint32_t ReservedSecCnt,
                                     uint32_t SecPerClus, uint32_t NumFATs, 
                                     uint32_t BytesPerSect)
{
    uint64_t Numerator, Denominator;
    uint64_t FatElementSize = 4;
    uint64_t FatSz;

    /* This is based on 
       http://hjem.get2net.dk/rune_moeller_barnkob/filesystems/fat.html
       I've made the obvious changes for FAT32
     */

    Numerator = FatElementSize * ( DskSize - ReservedSecCnt );
    Denominator = ( SecPerClus * BytesPerSect ) + ( FatElementSize * NumFATs );
    FatSz = Numerator / Denominator;

    /* round up */
    FatSz += 1;

    return((uint32_t)FatSz);
}

static uint8_t get_spc(uint32_t ClusterSizeKB, uint32_t BytesPerSect)
{
    uint32_t spc = ( ClusterSizeKB * 1024 ) / BytesPerSect;
    return( (uint8_t) spc );
}

static uint8_t get_sectors_per_cluster(uint32_t DiskSizeSectors, 
                                       uint32_t BytesPerSect)
{
    uint8_t ret = 0x01; /* 1 sector per cluster */
    uint64_t DiskSizeBytes = (uint64_t)DiskSizeSectors * (uint64_t)BytesPerSect;
    int64_t DiskSizeMB = DiskSizeBytes / ( 1024*1024 );

    /* 512 MB to 8,191 MB 4 KB */
    if ( DiskSizeMB > 512 )
        ret = get_spc( 4, BytesPerSect );   /* ret = 0x8; */
        
    /* 8,192 MB to 16,383 MB 8 KB */
    if ( DiskSizeMB > 8192 )
        ret = get_spc( 8, BytesPerSect );   /* ret = 0x10; */

    /* 16,384 MB to 32,767 MB 16 KB */
    if ( DiskSizeMB > 16384 )
        ret = get_spc( 16, BytesPerSect );  /* ret = 0x20; */

    /* Larger than 32,768 MB 32 KB */
    if ( DiskSizeMB > 32768 )
        ret = get_spc( 32, BytesPerSect );  /* ret = 0x40; */
    
    return( ret );

}

static void create_boot_sector(unsigned char* buf, 
                               struct ipod_t* ipod, int partition)
{
    struct FAT_BOOTSECTOR32* pFAT32BootSect = (struct FAT_BOOTSECTOR32*)buf;

    /* fill out the boot sector and fs info */
    pFAT32BootSect->sJmpBoot[0]=0xEB;
    pFAT32BootSect->sJmpBoot[1]=0x5A;
    pFAT32BootSect->sJmpBoot[2]=0x90;
    strcpy( pFAT32BootSect->sOEMName, "MSWIN4.1" );
    pFAT32BootSect->wBytsPerSec = htole16(BytesPerSect);
    pFAT32BootSect->bSecPerClus = SectorsPerCluster ;
    pFAT32BootSect->wRsvdSecCnt = htole16(ReservedSectCount);
    pFAT32BootSect->bNumFATs = NumFATs;
    pFAT32BootSect->wRootEntCnt = htole16(0);
    pFAT32BootSect->wTotSec16 = htole16(0);
    pFAT32BootSect->bMedia = 0xF8;
    pFAT32BootSect->wFATSz16 = htole16(0);
    pFAT32BootSect->wSecPerTrk = htole16(ipod->sectors_per_track);
    pFAT32BootSect->wNumHeads = htole16(ipod->num_heads);
    pFAT32BootSect->dHiddSec = htole16(ipod->pinfo[partition].start);
    pFAT32BootSect->dTotSec32 = htole32(TotalSectors);
    pFAT32BootSect->dFATSz32 = htole32(FatSize);
    pFAT32BootSect->wExtFlags = htole16(0);
    pFAT32BootSect->wFSVer = htole16(0);
    pFAT32BootSect->dRootClus = htole32(2);
    pFAT32BootSect->wFSInfo = htole16(1);
    pFAT32BootSect->wBkBootSec = htole16(BackupBootSect);
    pFAT32BootSect->bDrvNum = 0x80;
    pFAT32BootSect->Reserved1 = 0;
    pFAT32BootSect->bBootSig = 0x29;
    pFAT32BootSect->dBS_VolID = htole32(VolumeId);
    memcpy(pFAT32BootSect->sVolLab, VolId, 11);
    memcpy(pFAT32BootSect->sBS_FilSysType, "FAT32   ", 8 );

    buf[510] = 0x55;
    buf[511] = 0xaa;
}

static void create_fsinfo(unsigned char* buf)
{
    struct FAT_FSINFO* pFAT32FsInfo = (struct FAT_FSINFO*)buf;

    /* FSInfo sect */
    pFAT32FsInfo->dLeadSig = htole32(0x41615252);
    pFAT32FsInfo->dStrucSig = htole32(0x61417272);
    pFAT32FsInfo->dFree_Count = htole32((uint32_t) -1);
    pFAT32FsInfo->dNxt_Free = htole32((uint32_t) -1);
    pFAT32FsInfo->dTrailSig = htole32(0xaa550000);
    pFAT32FsInfo->dFree_Count = htole32((UserAreaSize/SectorsPerCluster)-1);

    /* clusters 0-1 reserved, we used cluster 2 for the root dir */
    pFAT32FsInfo->dNxt_Free = htole32(3); 
}

static void create_firstfatsector(unsigned char* buf)
{
    uint32_t* p = (uint32_t*)buf;  /* We know the buffer is aligned */

    /* First FAT Sector */
    p[0] = htole32(0x0ffffff8);  /* Reserved cluster 1 media id in low byte */
    p[1] = htole32(0x0fffffff);  /* Reserved cluster 2 EOC */
    p[2] = htole32(0x0fffffff);  /* end of cluster chain for root dir */
}

int format_partition(struct ipod_t* ipod, int partition)
{
    uint32_t i;
    uint64_t qTotalSectors=0;
    uint64_t FatNeeded;

    VolumeId = get_volume_id( );

    /* Only support hard disks at the moment */
    if ( ipod->sector_size != 512 )
    {
        fprintf(stderr,"[ERR]  Only disks with 512 bytes per sector are supported.\n");
        return -1;
    }
    BytesPerSect = ipod->sector_size;

    /* Checks on Disk Size */
    qTotalSectors = ipod->pinfo[partition].size;

    /* low end limit - 65536 sectors */
    if ( qTotalSectors < 65536 )
    {
        /* I suspect that most FAT32 implementations would mount this
           volume just fine, but the spec says that we shouldn't do
           this, so we won't */

        fprintf(stderr,"[ERR]  This drive is too small for FAT32 - there must be at least 64K clusters\n" );
        return -1;
    }

    if ( qTotalSectors >= 0xffffffff )
    {
        /* This is a more fundamental limitation on FAT32 - the total
           sector count in the root dir ­s 32bit. With a bit of
           creativity, FAT32 could be extended to handle at least 2^28
           clusters There would need to be an extra field in the
           FSInfo sector, and the old sector count could be set to
           0xffffffff. This is non standard though, the Windows FAT
           driver FASTFAT.SYS won't understand this. Perhaps a future
           version of FAT32 and FASTFAT will handle this. */

        fprintf(stderr,"[ERR]  This drive is too big for FAT32 - max 2TB supported\n");
    }

    if ( sectors_per_cluster ) {
        SectorsPerCluster = sectors_per_cluster;
    } else {
        SectorsPerCluster = get_sectors_per_cluster(ipod->pinfo[partition].size,
                                                    BytesPerSect );
    }

    TotalSectors = (uint32_t)  qTotalSectors;

    FatSize = get_fat_size_sectors(TotalSectors, ReservedSectCount, 
                                   SectorsPerCluster, NumFATs, BytesPerSect );

    UserAreaSize = TotalSectors - ReservedSectCount - (NumFATs*FatSize);

    /* First zero out ReservedSect + FatSize * NumFats + SectorsPerCluster */
    SystemAreaSize = (ReservedSectCount+(NumFATs*FatSize) + SectorsPerCluster);

    /* Work out the Cluster count */
    FatNeeded = UserAreaSize/SectorsPerCluster;

    /* check for a cluster count of >2^28, since the upper 4 bits of
       the cluster values in the FAT are reserved. */
    if (FatNeeded > 0x0FFFFFFF) {
        fprintf(stderr,"[ERR]  This drive has more than 2^28 clusters, try to specify a larger cluster size\n" );
        return -1;
    }

    /* Sanity check, make sure the fat is big enough.
       Convert the cluster count into a Fat sector count, and check
       the fat size value we calculated earlier is OK.  */ 

    FatNeeded *=4;
    FatNeeded += (BytesPerSect-1);
    FatNeeded /= BytesPerSect;

    if ( FatNeeded > FatSize ) {
        fprintf(stderr,"[ERR]  Drive too big to format\n");
        return -1;
    }

    /*
       Write boot sector, fats
       Sector 0 Boot Sector
       Sector 1 FSInfo 
       Sector 2 More boot code - we write zeros here
       Sector 3 unused
       Sector 4 unused
       Sector 5 unused
       Sector 6 Backup boot sector
       Sector 7 Backup FSInfo sector
       Sector 8 Backup 'more boot code'
       zero'd sectors upto ReservedSectCount
       FAT1  ReservedSectCount to ReservedSectCount + FatSize
       ...
       FATn  ReservedSectCount to ReservedSectCount + FatSize
       RootDir - allocated to cluster2
     */
    
    fprintf(stderr,"[INFO] Heads - %d, sectors/track = %d\n",ipod->num_heads,ipod->sectors_per_track);
    fprintf(stderr,"[INFO] Size : %lluGB %u sectors\n", ((uint64_t)ipod->pinfo[partition].size * (uint64_t)ipod->sector_size) / (1000*1000*1000), TotalSectors );
    fprintf(stderr,"[INFO] %d Bytes Per Sector, Cluster size %d bytes\n", BytesPerSect, SectorsPerCluster*BytesPerSect );
    fprintf(stderr,"[INFO] Volume ID is %x:%x\n", VolumeId>>16, VolumeId&0xffff );
    fprintf(stderr,"[INFO] %d Reserved Sectors, %d Sectors per FAT, %d fats\n", ReservedSectCount, FatSize, NumFATs );
    fprintf (stderr,"[INFO] %d Total clusters\n", UserAreaSize/SectorsPerCluster );

    fprintf(stderr,"[INFO] Formatting partition %d:...\n",partition);

    /* Once zero_sectors has run, any data on the drive is basically lost... */
    fprintf(stderr,"[INFO] Clearing out %d sectors for Reserved sectors, fats and root cluster...\n", SystemAreaSize );

    zero_sectors(ipod, ipod->pinfo[partition].start, SystemAreaSize);

    fprintf(stderr,"[INFO] Initialising reserved sectors and FATs...\n" );

    /* Create the boot sector structure */
    create_boot_sector(sectorbuf, ipod, partition);
    create_fsinfo(sectorbuf + 512);

    /* Write boot sector and fsinfo at start of partition */
    if (ipod_seek(ipod, ipod->pinfo[partition].start * ipod->sector_size) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }
    if (ipod_write(ipod,sectorbuf,512 * 2) < 0) {
        perror("[ERR]  Write failed (first copy of bootsect/fsinfo)\n");
        return -1;
    }

    /* Write backup copy of boot sector and fsinfo */
    if (ipod_seek(ipod, (ipod->pinfo[partition].start + BackupBootSect) * ipod->sector_size) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }
    if (ipod_write(ipod,sectorbuf,512 * 2) < 0) {
        perror("[ERR]  Write failed (first copy of bootsect/fsinfo)\n");
        return -1;
    }

    /* Create the first FAT sector */
    create_firstfatsector(sectorbuf);
    
    /* Write the first fat sector in the right places */
    for ( i=0; i<NumFATs; i++ ) {
        int SectorStart = ReservedSectCount + (i * FatSize );

        if (ipod_seek(ipod, (ipod->pinfo[partition].start + SectorStart) * ipod->sector_size) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if (ipod_write(ipod,sectorbuf,512) < 0) {
            perror("[ERR]  Write failed (first copy of bootsect/fsinfo)\n");
            return -1;
        }
    }

    fprintf(stderr,"[INFO] Format successful\n");

    return 0;
}
