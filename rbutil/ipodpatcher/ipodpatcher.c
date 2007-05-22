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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "parttypes.h"
#include "ipodio.h"
#include "ipodpatcher.h"

#ifdef WITH_BOOTOBJS
#include "ipod3g.h"
#include "ipod4g.h"
#include "ipodmini.h"
#include "ipodmini2g.h"
#include "ipodcolor.h"
#include "ipodnano.h"
#include "ipodvideo.h"
#endif

extern int verbose;

unsigned char* sectorbuf;

/* The following string appears at the start of the firmware partition */
static const char *apple_stop_sign = "{{~~  /-----\\   "\
                                     "{{~~ /       \\  "\
                                     "{{~~|         | "\
                                     "{{~~| S T O P | "\
                                     "{{~~|         | "\
                                     "{{~~ \\       /  "\
                                     "{{~~  \\-----/   "\
                                     "Copyright(C) 200"\
                                     "1 Apple Computer"\
                                     ", Inc.----------"\
                                     "----------------"\
                                     "----------------"\
                                     "----------------"\
                                     "----------------"\
                                     "----------------"\
                                     "---------------";

/* Windows requires the buffer for disk I/O to be aligned in memory on a 
   multiple of the disk volume size - so we use a single global variable
   and initialise it with ipod_alloc_buf() 
*/

char* get_parttype(int pt)
{
    int i;
    static char unknown[]="Unknown";

    if (pt == -1) {
        return "HFS/HFS+";
    }

    i=0;
    while (parttypes[i].name != NULL) {
        if (parttypes[i].type == pt) {
            return (parttypes[i].name);
        }
        i++;
    }

    return unknown;
}

off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}

/* Partition table parsing code taken from Rockbox */

#define MAX_SECTOR_SIZE 2048
#define SECTOR_SIZE 512

unsigned short static inline le2ushort(unsigned char* buf)
{
   unsigned short res = (buf[1] << 8) | buf[0];

   return res;
}

int static inline le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

int static inline be2int(unsigned char* buf)
{
   int32_t res = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

   return res;
}

int static inline getint16le(char* buf)
{
   int16_t res = (buf[1] << 8) | buf[0];

   return res;
}

void static inline short2le(unsigned short val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
}

void static inline int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

void int2be(unsigned int val, unsigned char* addr)
{
    addr[0] = (val >> 24) & 0xff;
    addr[1] = (val >> 16) & 0xff;
    addr[2] = (val >> 8) & 0xff;
    addr[3] = val & 0xFF;
}


#define BYTES2INT32(array,pos)\
    ((long)array[pos] | ((long)array[pos+1] << 8 ) |\
    ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

int read_partinfo(struct ipod_t* ipod, int silent)
{
    int i;
    unsigned long count;

    count = ipod_read(ipod,sectorbuf, ipod->sector_size);

    if (count <= 0) {
        print_error(" Error reading from disk: ");
        return -1;
    }

    if ((sectorbuf[510] == 0x55) && (sectorbuf[511] == 0xaa)) {
        /* DOS partition table */
        ipod->macpod = 0;
        /* parse partitions */
        for ( i = 0; i < 4; i++ ) {
            unsigned char* ptr = sectorbuf + 0x1be + 16*i;
            ipod->pinfo[i].type  = ptr[4];
            ipod->pinfo[i].start = BYTES2INT32(ptr, 8);
            ipod->pinfo[i].size  = BYTES2INT32(ptr, 12);

            /* extended? */
            if ( ipod->pinfo[i].type == 5 ) {
                /* not handled yet */
            }
        }
    } else if ((sectorbuf[0] == 'E') && (sectorbuf[1] == 'R')) {
        /* Apple Partition Map */

        /* APM parsing code based on the check_mac_partitions() function in
           ipodloader2 - written by Thomas Tempelmann and released
           under the GPL. */

        int blkNo = 1;
        int partBlkCount = 1;
        int partBlkSizMul = sectorbuf[2] / 2;

        int pmMapBlkCnt;      /* # of blks in partition map */
        int pmPyPartStart;    /* physical start blk of partition */
        int pmPartBlkCnt;     /* # of blks in this partition */
        int i = 0;

        ipod->macpod = 1;

        memset(ipod->pinfo,0,sizeof(ipod->pinfo));

        while (blkNo <= partBlkCount) {
            if (ipod_seek(ipod, blkNo * partBlkSizMul * 512) < 0) {
                fprintf(stderr,"[ERR]  Seek failed whilst reading APM\n");
                return -1;
            }

            count = ipod_read(ipod, sectorbuf, ipod->sector_size);

            if (count <= 0) {
                print_error(" Error reading from disk: ");
                return -1;
            }

            /* see if it's a partition entry */
            if ((sectorbuf[0] != 'P') || (sectorbuf[1] != 'M')) {
                /* end of partition table -> leave the loop */
                break;  
            }

            /* Extract the interesting entries */
            pmMapBlkCnt = be2int(sectorbuf + 4);
            pmPyPartStart = be2int(sectorbuf + 8);
            pmPartBlkCnt = be2int(sectorbuf + 12);

            /* update the number of part map blocks */
            partBlkCount = pmMapBlkCnt;

            if (strncmp((char*)(sectorbuf + 48), "Apple_MDFW", 32)==0) {
                 /* A Firmware partition */
                 ipod->pinfo[i].start = pmPyPartStart;
                 ipod->pinfo[i].size = pmPartBlkCnt;
                 ipod->pinfo[i].type = 0;
                 i++;
            } else if (strncmp((char*)(sectorbuf + 48), "Apple_HFS", 32)==0) {
                 /* A HFS partition */
                 ipod->pinfo[i].start = pmPyPartStart;
                 ipod->pinfo[i].size = pmPartBlkCnt;
                 ipod->pinfo[i].type = -1;
                 i++;
            }

            blkNo++; /* read next partition map entry */
        }
    } else {
        if (!silent) fprintf(stderr,"[ERR]  Bad boot sector signature\n");
        return -1;
    }

    /* Check that the partition table looks like an ipod:
           1) Partition 1 is of type 0 (Empty) but isn't empty.
           2) Partition 2 is of type 0xb (winpod) or -1 (macpod)
    */
    if ((ipod->pinfo[0].type != 0) || (ipod->pinfo[0].size == 0) || 
        ((ipod->pinfo[1].type != 0xb) && (ipod->pinfo[1].type != -1))) {
        if (!silent) fprintf(stderr,"[ERR]  Partition layout is not an ipod\n");
        return -1;
    }

    ipod->start = ipod->pinfo[0].start*ipod->sector_size;
    return 0;
}

int read_partition(struct ipod_t* ipod, int outfile)
{
    int res;
    unsigned long n;
    int bytesleft;
    int chunksize;
    int count = ipod->pinfo[0].size;

    if (ipod_seek(ipod, ipod->start) < 0) {
        return -1;
    }

    fprintf(stderr,"[INFO] Writing %d sectors to output file\n",count);

    bytesleft = count * ipod->sector_size;
    while (bytesleft > 0) {
        if (bytesleft > BUFFER_SIZE) {
           chunksize = BUFFER_SIZE;
        } else {
           chunksize = bytesleft;
        }

        n = ipod_read(ipod, sectorbuf, chunksize);

        if (n < 0) {
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,
              "[ERR]  Short read in disk_read() - requested %d, got %lu\n",
              chunksize,n);
            return -1;
        }

        bytesleft -= n;

        res = write(outfile,sectorbuf,n);

        if (res < 0) {
            perror("[ERR]  write in disk_read");
            return -1;
        }

        if (res != n) {
            fprintf(stderr,
              "Short write - requested %lu, received %d - aborting.\n",n,res);
            return -1;
        }
    }

    fprintf(stderr,"[INFO] Done.\n");
    return 0;
}

int write_partition(struct ipod_t* ipod, int infile)
{
    unsigned long res;
    int n;
    int bytesread;
    int byteswritten = 0;
    int eof;
    int padding = 0;

    if (ipod_seek(ipod, ipod->start) < 0) {
        return -1;
    }

    fprintf(stderr,"[INFO] Writing input file to device\n");
    bytesread = 0;
    eof = 0;
    while (!eof) {
        n = read(infile,sectorbuf,BUFFER_SIZE);

        if (n < 0) {
            perror("[ERR]  read in disk_write");
            return -1;
        }

        if (n < BUFFER_SIZE) {
           eof = 1;
           /* We need to pad the last write to a multiple of SECTOR_SIZE */
           if ((n % ipod->sector_size) != 0) {
               padding = (ipod->sector_size-(n % ipod->sector_size));
               n += padding; 
           }
        }

        bytesread += n;

        res = ipod_write(ipod, sectorbuf, n);

        if (res < 0) {
            print_error(" Error writing to disk: ");
            fprintf(stderr,"Bytes written: %d\n",byteswritten);
            return -1;
        }

        if (res != n) {
            fprintf(stderr,"[ERR]  Short write - requested %d, received %lu - aborting.\n",n,res);
            return -1;
        }

        byteswritten += res;
    }

    fprintf(stderr,"[INFO] Wrote %d bytes plus %d bytes padding.\n",
            byteswritten-padding,padding);
    return 0;
}

char* ftypename[] = { "OSOS", "RSRC", "AUPD", "HIBE" };

int diskmove(struct ipod_t* ipod, int delta)
{
    int src_start;
    int src_end;
    int bytesleft;
    int chunksize;
    int i;
    int n;

    src_start = ipod->ipod_directory[1].devOffset + ipod->sector_size;
    src_end = (ipod->ipod_directory[ipod->nimages-1].devOffset + ipod->sector_size + 
              ipod->ipod_directory[ipod->nimages-1].len + 
              (ipod->sector_size-1)) & ~(ipod->sector_size-1);
    bytesleft = src_end - src_start;

    if (verbose) {
        fprintf(stderr,"[INFO] Need to move images 2-%d forward %08x bytes\n", ipod->nimages,delta);
        fprintf(stderr,"[VERB] src_start     = %08x\n",src_start);
        fprintf(stderr,"[VERB] src_end       = %08x\n",src_end);
        fprintf(stderr,"[VERB] dest_start    = %08x\n",src_start+delta);
        fprintf(stderr,"[VERB] dest_end      = %08x\n",src_end+delta);
        fprintf(stderr,"[VERB] bytes to copy = %08x\n",bytesleft);
    }

    while (bytesleft > 0) {
        if (bytesleft <= BUFFER_SIZE) {
            chunksize = bytesleft;
        } else {
            chunksize = BUFFER_SIZE;
        }

        if (verbose) {
            fprintf(stderr,"[VERB] Copying %08x bytes from %08x to %08x (absolute %08x to %08x)\n",
                           chunksize,
                           src_end-chunksize,
                           src_end-chunksize+delta,
                           (unsigned int)(ipod->start+src_end-chunksize),
                           (unsigned int)(ipod->start+src_end-chunksize+delta));
        }


        if (ipod_seek(ipod, ipod->start+src_end-chunksize) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if ((n = ipod_read(ipod,sectorbuf,chunksize)) < 0) {
            perror("[ERR]  Write failed\n");
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                           i,n);
            return -1;
        }

        if (ipod_seek(ipod, ipod->start+src_end-chunksize+delta) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if ((n = ipod_write(ipod,sectorbuf,chunksize)) < 0) {
            perror("[ERR]  Write failed\n");
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                          ,i,n);
            return -1;
        }

        src_end -= chunksize;
        bytesleft -= chunksize;
    }

    return 0;
}

int add_bootloader(struct ipod_t* ipod, char* filename, int type)
{
    int length;
    int i;
    int x;
    int n;
    int infile;
    int paddedlength;
    int entryOffset;
    int delta = 0;
    unsigned long chksum=0;
    unsigned long filechksum=0;
    unsigned char header[8];  /* Header for .ipod file */
    unsigned char* bootloader_buf;

    /* Calculate the position in the OSOS image where our bootloader will go. */
    if (ipod->ipod_directory[0].entryOffset>0) {
        /* Keep the same entryOffset */
        entryOffset = ipod->ipod_directory[0].entryOffset;
    } else {
        entryOffset = (ipod->ipod_directory[0].len+ipod->sector_size-1)&~(ipod->sector_size-1);
    }

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        fprintf(stderr,"[INFO] Using internal bootloader - %d bytes\n",ipod->bootloader_len);
        memcpy(sectorbuf+entryOffset,ipod->bootloader,ipod->bootloader_len);
        length = ipod->bootloader_len;
        paddedlength=(ipod->bootloader_len+ipod->sector_size-1)&~(ipod->sector_size-1);
    } 
    else 
#endif
    {
        infile=open(filename,O_RDONLY);
        if (infile < 0) {
            fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
            return -1;
        }

        if (type==FILETYPE_DOT_IPOD) {
            /* First check that the input file is the correct type for this ipod. */
            n = read(infile,header,8);
            if (n < 8) {
                fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
                close(infile);
                return -1;
            }
    
            if (memcmp(header+4, ipod->modelname,4)!=0) {
                fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                        header[4],header[5],header[6],header[7], ipod->modelname);
                close(infile);
                return -1;
            }
    
            filechksum = be2int(header);
    
            length=filesize(infile)-8;
        } else {
            length=filesize(infile);
        }
        paddedlength=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

        bootloader_buf = malloc(length);
        if (bootloader_buf == NULL) {
            fprintf(stderr,"[ERR]  Can not allocate memory for bootlaoder\n");
        }
        /* Now read our bootloader - we need to check it before modifying the partition*/
        n = read(infile,bootloader_buf,length);
        close(infile);

        if (n < 0) {
            fprintf(stderr,"[ERR]  Couldn't read input file\n");
            return -1;
        }
    
        if (type==FILETYPE_DOT_IPOD) {
            /* Calculate and confirm bootloader checksum */
            chksum = ipod->modelnum;
            for (i = 0; i < length; i++) {
                 /* add 8 unsigned bits but keep a 32 bit sum */
                 chksum += bootloader_buf[i];
            }
    
            if (chksum == filechksum) {
                fprintf(stderr,"[INFO] Checksum OK in %s\n",filename);
            } else {
                fprintf(stderr,"[ERR]  Checksum in %s failed check\n",filename);
                return -1;
            }
        }
    }

    if (entryOffset+paddedlength > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        return -1;
    }

    if (verbose) {
        fprintf(stderr,"[VERB] Original firmware begins at 0x%08x\n", ipod->ipod_directory[0].devOffset + ipod->sector_size);
        fprintf(stderr,"[VERB] New entryOffset will be 0x%08x\n",entryOffset);
        fprintf(stderr,"[VERB] End of bootloader will be at 0x%08x\n",entryOffset+paddedlength);
    }

    /* Check if we have enough space */
    /* TODO: Check the size of the partition. */
    if (ipod->nimages > 1) {
        if ((ipod->ipod_directory[0].devOffset+entryOffset+paddedlength) >
             ipod->ipod_directory[1].devOffset) {
            fprintf(stderr,"[INFO] Moving images to create room for new firmware...\n");
            delta = ipod->ipod_directory[0].devOffset + entryOffset+paddedlength
                    - ipod->ipod_directory[1].devOffset;

            if (diskmove(ipod, delta) < 0) {
                fprintf(stderr,"[ERR]  Image movement failed.\n");
                return -1;
            }
        }
    }


    /* We have moved the partitions, now we can write our bootloader */

    /* Firstly read the original firmware into sectorbuf */
    fprintf(stderr,"[INFO] Reading original firmware...\n");
    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_read(ipod,sectorbuf,entryOffset)) < 0) {
        perror("[ERR]  Read failed\n");
        return -1;
    }

    if (n < entryOffset) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                      ,i,n);
        return -1;
    }

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        memcpy(sectorbuf+entryOffset,ipod->bootloader,ipod->bootloader_len);
    }
    else
#endif
    {
        memcpy(sectorbuf+entryOffset,bootloader_buf,length);
        free(bootloader_buf);
    }

    /* Calculate new checksum for combined image */
    chksum = 0;
    for (i=0;i<entryOffset + length; i++) {
         chksum += sectorbuf[i];
    }    

    /* Now write the combined firmware image to the disk */

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,sectorbuf,entryOffset+paddedlength)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < (entryOffset+paddedlength)) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                      ,i,n);
        return -1;
    }

    fprintf(stderr,"[INFO]  Wrote %d bytes to firmware partition\n",entryOffset+paddedlength);

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    n=ipod_read(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Directory read failed\n");
        return -1;
    }

    /* Update entries for image 0 */
    int2le(entryOffset+length,sectorbuf+x+16);
    int2le(entryOffset,sectorbuf+x+24);
    int2le(chksum,sectorbuf+x+28);
    int2le(0xffffffff,sectorbuf+x+36);  /* loadAddr */

    /* Update devOffset entries for other images, if we have moved them */
    if (delta > 0) {
        for (i=1;i<ipod->nimages;i++) {
           int2le(le2int(sectorbuf+x+i*40+12)+delta,sectorbuf+x+i*40+12);
        }
    }

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to %d failed\n", (int)(ipod->start+ipod->diroffset-x));
        return -1;
    }
    n=ipod_write(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Directory write failed\n");
        return -1;
    }

    return 0;
}

int delete_bootloader(struct ipod_t* ipod)
{
    int length;
    int i;
    int x;
    int n;
    unsigned long chksum=0;   /* 32 bit checksum - Rockbox .ipod style*/

    /* Removing the bootloader involves adjusting the "length",
       "chksum" and "entryOffset" values in the osos image's directory
       entry. */

    /* Firstly check we have a bootloader... */

    if (ipod->ipod_directory[0].entryOffset == 0) {
        fprintf(stderr,"[ERR]  No bootloader found.\n");
        return -1;
    }

    length = ipod->ipod_directory[0].entryOffset;

    /* Read the firmware so we can calculate the checksum */
    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        return -1;
    }

    i = (length+ipod->sector_size-1) & ~(ipod->sector_size-1);
    fprintf(stderr,"[INFO] Padding read from 0x%08x to 0x%08x bytes\n",
            length,i);

    if ((n = ipod_read(ipod,sectorbuf,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    chksum = 0;
    for (i = 0; i < length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += sectorbuf[i];
    }

    /* Now write back the updated directory entry */

    fprintf(stderr,"[INFO] Updating firmware checksum\n");

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image 0 */
    int2le(length,sectorbuf+x+16);
    int2le(0,sectorbuf+x+24);
    int2le(chksum,sectorbuf+x+28);

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int write_firmware(struct ipod_t* ipod, char* filename, int type)
{
    int length;
    int i;
    int x;
    int n;
    int infile;
    int newsize;
    int bytesavailable;
    unsigned long chksum=0;
    unsigned long filechksum=0;
    unsigned char header[8];  /* Header for .ipod file */

    /* First check that the input file is the correct type for this ipod. */
    infile=open(filename,O_RDONLY);
    if (infile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
        return -1;
    }

    if (type==FILETYPE_DOT_IPOD) {
        n = read(infile,header,8);
        if (n < 8) {
            fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
            close(infile);
            return -1;
        }

        if (memcmp(header+4, ipod->modelname,4)!=0) {
            fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                    header[4],header[5],header[6],header[7], ipod->modelname);
            close(infile);
            return -1;
        }

        filechksum = be2int(header);

        length = filesize(infile)-8;
    } else {
        length = filesize(infile);
    }
    newsize=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

    fprintf(stderr,"[INFO] Padding input file from 0x%08x to 0x%08x bytes\n",
            length,newsize);

    if (newsize > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        close(infile);
        return -1;
    }

    /* Check if we have enough space */
    /* TODO: Check the size of the partition. */
    if (ipod->nimages > 1) {
        bytesavailable=ipod->ipod_directory[1].devOffset-ipod->ipod_directory[0].devOffset;
        if (bytesavailable < newsize) {
            fprintf(stderr,"[INFO] Moving images to create room for new firmware...\n");

            /* TODO: Implement image movement */
            fprintf(stderr,"[ERR]  Image movement not yet implemented.\n");
            close(infile);
            return -1;
        }
    }

    fprintf(stderr,"[INFO] Reading input file...\n");
    /* We now know we have enough space, so write it. */
    memset(sectorbuf+length,0,newsize-length);
    n = read(infile,sectorbuf,length);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Couldn't read input file\n");
        close(infile);
        return -1;
    }
    close(infile);

    if (type==FILETYPE_DOT_IPOD) {
        chksum = ipod->modelnum;
        for (i = 0; i < length; i++) {
            /* add 8 unsigned bits but keep a 32 bit sum */
            chksum += sectorbuf[i];
        }

        if (chksum == filechksum) {
            fprintf(stderr,"[INFO] Checksum OK in %s\n",filename);
        } else {
            fprintf(stderr,"[ERR]  Checksum in %s failed check\n",filename);
            return -1;
        }
    }

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,sectorbuf,newsize)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < newsize) {
        fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                      ,i,n);
        return -1;
    }
    fprintf(stderr,"[INFO]  Wrote %d bytes to firmware partition\n",n);

    /* Now we need to update the "len", "entryOffset" and "chksum" fields */
    chksum = 0;
    for (i = 0; i < length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += sectorbuf[i];
    }

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image 0 */
    int2le(length,sectorbuf+x+16);
    int2le(0,sectorbuf+x+24);
    int2le(chksum,sectorbuf+x+28);

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int read_firmware(struct ipod_t* ipod, char* filename, int type)
{
    int length;
    int i;
    int outfile;
    int n;
    unsigned long chksum=0;   /* 32 bit checksum - Rockbox .ipod style*/
    unsigned char header[8];  /* Header for .ipod file */

    if (ipod->ipod_directory[0].entryOffset != 0) {
        /* We have a bootloader... */
        length = ipod->ipod_directory[0].entryOffset;
    } else {
        length = ipod->ipod_directory[0].len;
    }

    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        return -1;
    }

    i = (length+ipod->sector_size-1) & ~(ipod->sector_size-1);
    fprintf(stderr,"[INFO] Padding read from 0x%08x to 0x%08x bytes\n",
            length,i);

    if ((n = ipod_read(ipod,sectorbuf,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return -1;
    }

    if (type == FILETYPE_DOT_IPOD) {
        chksum = ipod->modelnum;
        for (i = 0; i < length; i++) {
            /* add 8 unsigned bits but keep a 32 bit sum */
            chksum += sectorbuf[i];
        }

        int2be(chksum,header);
        memcpy(header+4, ipod->modelname,4);

        write(outfile,header,8);
    }

    write(outfile,sectorbuf,length);
    close(outfile);

    return 0;
}

int read_directory(struct ipod_t* ipod)
{
    int n;
    int x;
    unsigned char* p;
    unsigned short version;

    ipod->nimages=0;

    /* Read firmware partition header (first 512 bytes of disk - but 
       let's read a whole sector) */

    if (ipod_seek(ipod, ipod->start) < 0) { 
        fprintf(stderr,"[ERR]  Seek to 0x%08x in read_directory() failed.\n",
                       (unsigned int)(ipod->start));
        return -1;
    }

    n=ipod_read(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  ipod_read(ipod,buf,0x%08x) failed in read_directory()\n", ipod->sector_size);
        return -1;
    }

    if (memcmp(sectorbuf,apple_stop_sign,sizeof(apple_stop_sign))!=0) {
        fprintf(stderr,"[ERR]  Firmware partition doesn't contain Apple copyright, aborting.\n");
        return -1;
    }

    if (memcmp(sectorbuf+0x100,"]ih[",4)!=0) {
        fprintf(stderr,"[ERR]  Bad firmware directory\n");
        return -1;
    }

    version = le2ushort(sectorbuf+0x10a);
    if ((version != 2) && (version != 3)) {
        fprintf(stderr,"[ERR]  Unknown firmware format version %04x\n",
                version);
    }
    ipod->diroffset=le2int(sectorbuf+0x104) + 0x200;

    /* diroffset may not be sector-aligned */
    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)ipod->diroffset);
        return -1;
    }

    n=ipod_read(ipod, sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Read of directory failed.\n");
        return -1;
    }

    p = sectorbuf + x;
    
    while ((ipod->nimages < MAX_IMAGES) && (p < (sectorbuf + x + 400)) && 
           (memcmp(p,"!ATA",4)==0)) {
        p+=4;
        if (memcmp(p,"soso",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_OSOS;
        } else if (memcmp(p,"crsr",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_RSRC;
        } else if (memcmp(p,"dpua",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_AUPD;
        } else if (memcmp(p,"ebih",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_HIBE;
        } else {
            fprintf(stderr,"[ERR]  Unknown image type %c%c%c%c\n",
                           p[0],p[1],p[2],p[3]);
        }
        p+=4;
        ipod->ipod_directory[ipod->nimages].id=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].devOffset=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].len=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].addr=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].entryOffset=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].chksum=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].vers=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].loadAddr=le2int(p);
        p+=4;
        ipod->nimages++;
    }

    if ((ipod->nimages > 1) && (version==2)) {
        /* The 3g firmware image doesn't appear to have a version, so
           let's make one up... Note that this is never written back to the
           ipod, so it's OK to do. */

        if (ipod->ipod_directory[0].vers == 0) { ipod->ipod_directory[0].vers = 3; }

        ipod->fwoffset = ipod->start;
    } else {
        ipod->fwoffset = ipod->start + ipod->sector_size;
    }

    return 0;
}

int list_images(struct ipod_t* ipod)
{
    int i;

    if (verbose) {
        printf("    Type         id  devOffset        len       addr entryOffset    chksum       vers   loadAddr   devOffset+len\n");
        for (i = 0 ; i < ipod->nimages; i++) {
            printf("%d - %s 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",i,
                   ftypename[ipod->ipod_directory[i].ftype],
                   ipod->ipod_directory[i].id,
                   ipod->ipod_directory[i].devOffset,
                   ipod->ipod_directory[i].len,
                   ipod->ipod_directory[i].addr,
                   ipod->ipod_directory[i].entryOffset,
                   ipod->ipod_directory[i].chksum,
                   ipod->ipod_directory[i].vers,
                   ipod->ipod_directory[i].loadAddr,
                   ipod->ipod_directory[i].devOffset+((ipod->ipod_directory[i].len+ipod->sector_size-1)&~(ipod->sector_size-1)));
        }
    }

    printf("\n");
    printf("Listing firmware partition contents:\n");
    printf("\n");

    for (i = 0 ; i < ipod->nimages; i++) {
         printf("Image %d:\n",i+1);
         switch(ipod->ipod_directory[i].ftype) {
             case FTYPE_OSOS:
                 if (ipod->ipod_directory[i].entryOffset==0) {
                     printf("    Main firmware - %d bytes\n",
                       ipod->ipod_directory[i].len);
                 } else {
                     printf("    Main firmware - %d bytes\n",
                       ipod->ipod_directory[i].entryOffset);
                     printf("    Third-party bootloader - %d bytes\n",
                       ipod->ipod_directory[i].len-ipod->ipod_directory[i].entryOffset);
                 }
                 break;
             default:
                     printf("    %s - %d bytes\n",
                            ftypename[ipod->ipod_directory[i].ftype],
                            ipod->ipod_directory[i].len);
         }
    }
    printf("\n");

    return 0;
}

int getmodel(struct ipod_t* ipod, int ipod_version)
{
    switch (ipod_version) {
        case 0x02:
            ipod->modelstr="3rd Generation";
            ipod->modelnum = 7;
            ipod->modelname = "ip3g";
            ipod->targetname = "ipod3g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipod3g;
            ipod->bootloader_len = LEN_ipod3g;
#endif
            break;
        case 0x40:
            ipod->modelstr="1st Generation Mini";
            ipod->modelnum = 9;
            ipod->modelname = "mini";
            ipod->targetname = "ipodmini";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodmini;
            ipod->bootloader_len = LEN_ipodmini;
#endif
            break;
        case 0x50:
            ipod->modelstr="4th Generation";
            ipod->modelnum = 8;
            ipod->modelname = "ip4g";
            ipod->targetname = "ipod4g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipod4g;
            ipod->bootloader_len = LEN_ipod4g;
#endif
            break;
        case 0x60:
            ipod->modelstr="Photo/Color";
            ipod->modelnum = 3;
            ipod->modelname = "ipco";
            ipod->targetname = "ipodcolor";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodcolor;
            ipod->bootloader_len = LEN_ipodcolor;
#endif
            break;
        case 0x70:
            ipod->modelstr="2nd Generation Mini";
            ipod->modelnum = 11;
            ipod->modelname = "mn2g";
            ipod->targetname = "ipodmini2g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodmini2g;
            ipod->bootloader_len = LEN_ipodmini2g;
#endif
            break;
        case 0xc0:
            ipod->modelstr="1st Generation Nano";
            ipod->modelnum = 4;
            ipod->modelname = "nano";
            ipod->targetname = "ipodnano";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodnano;
            ipod->bootloader_len = LEN_ipodnano;
#endif
            break;
        case 0xb0:
            ipod->modelstr="Video (aka 5th Generation)";
            ipod->modelnum = 5;
            ipod->modelname = "ipvd";
            ipod->targetname = "ipodvideo";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodvideo;
            ipod->bootloader_len = LEN_ipodvideo;
#endif
            break;
        default:
            ipod->modelname = NULL;
            ipod->modelnum = 0;
            ipod->targetname = NULL;
#ifdef WITH_BOOTOBJS
            ipod->bootloader = NULL;
            ipod->bootloader_len = 0;
#endif
            return -1;
    }
    return 0;
}

int ipod_scan(struct ipod_t* ipod)
{
    int i;
    int n = 0;
    int ipod_version;
    char last_ipod[4096];

    printf("[INFO] Scanning disk devices...\n");

    for (i = 0; i <= 25 ; i++) {
#ifdef __WIN32__
         sprintf(ipod->diskname,"\\\\.\\PhysicalDrive%d",i);
#elif defined(linux) || defined (__linux)
         sprintf(ipod->diskname,"/dev/sd%c",'a'+i);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
         sprintf(ipod->diskname,"/dev/da%d",i);
#elif defined(__APPLE__) && defined(__MACH__)
         sprintf(ipod->diskname,"/dev/disk%d",i);
#else
    #error No disk paths defined for this platform
#endif
         if (ipod_open(ipod, 1) < 0) {
             continue;
         }

         if (read_partinfo(ipod,1) < 0) {
             continue;
         }

         if ((ipod->pinfo[0].start==0) || (ipod->pinfo[0].type != 0)) {
             continue;
         }

         if (read_directory(ipod) < 0) {
             continue;
         }

         ipod_version=(ipod->ipod_directory[0].vers>>8);
         if (getmodel(ipod,ipod_version) < 0) {
             continue;
         }

#ifdef __WIN32__
         printf("[INFO] Ipod found - %s (\"%s\") - disk device %d\n", 
             ipod->modelstr,ipod->macpod ? "macpod" : "winpod",i);
#else
         printf("[INFO] Ipod found - %s (\"%s\") - %s\n",
             ipod->modelstr,ipod->macpod ? "macpod" : "winpod",ipod->diskname);
#endif
         n++;
         strcpy(last_ipod,ipod->diskname);
         ipod_close(ipod);
    }

    if (n==1) {
        /* Remember the disk name */
        strcpy(ipod->diskname,last_ipod);
    }
    return n;
}
