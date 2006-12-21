/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
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

#define VERSION "0.6cvs"

int verbose = 0;

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

/* The maximum number of images in a firmware partition - a guess... */
#define MAX_IMAGES 10

/* Windows requires the buffer for disk I/O to be aligned in memory on a 
   multiple of the disk volume size - so we use a single global variable
   and initialise it with ipod_alloc_buf() 
*/

/* Size of buffer for disk I/O - 8MB is large enough for any version
   of the Apple firmware, but not the Nano's RSRC image. */
#define BUFFER_SIZE 8*1024*1024
unsigned char* sectorbuf;

char* get_parttype(int pt)
{
    int i;
    static char unknown[]="Unknown";

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

struct partinfo_t {
  unsigned long start; /* first sector (LBA) */
  unsigned long size;  /* number of sectors */
  unsigned char type;
};

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

void display_partinfo(struct partinfo_t* pinfo, int sector_size)
{
    int i;
    double sectors_per_MB = (1024.0*1024.0)/sector_size;

    printf("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n");
    for ( i = 0; i < 4; i++ ) {
        if (pinfo[i].start != 0) {
            printf("[INFO]    %d      %10ld    %10ld  %10.1f   %s (0x%02x)\n",
                   i,
                   pinfo[i].start,
                   pinfo[i].start+pinfo[i].size-1,
                   pinfo[i].size/sectors_per_MB,
                   get_parttype(pinfo[i].type),
                   pinfo[i].type);
        }
    }
}


int read_partinfo(HANDLE dh, int sector_size, struct partinfo_t* pinfo,
                  int silent)
{
    int i;
    unsigned long count;

    count = ipod_read(dh,sectorbuf,sector_size);

    if (count <= 0) {
        print_error(" Error reading from disk: ");
        return -1;
    }

    /* check that the boot sector is initialized */
    if ( (sectorbuf[510] != 0x55) ||
         (sectorbuf[511] != 0xaa)) {
        if (!silent) fprintf(stderr,"[ERR]  Bad boot sector signature\n");
        return -1;
    }

    if ((memcmp(&sectorbuf[71],"iPod",4) != 0) &&
        (memcmp(&sectorbuf[0x40],"This is your Apple iPod. You probably do not want to boot from it!",66) != 0) ) {
        if (!silent) fprintf(stderr,"[ERR]  Drive is not an iPod, aborting\n");
        return -1;
    }

    /* parse partitions */
    for ( i = 0; i < 4; i++ ) {
        unsigned char* ptr = sectorbuf + 0x1be + 16*i;
        pinfo[i].type  = ptr[4];
        pinfo[i].start = BYTES2INT32(ptr, 8);
        pinfo[i].size  = BYTES2INT32(ptr, 12);

        /* extended? */
        if ( pinfo[i].type == 5 ) {
            /* not handled yet */
        }
    }

    return 0;
}

int read_partition(HANDLE dh, int outfile,unsigned long start_sector,
                   unsigned long count, int sector_size)
{
    int res;
    unsigned long n;
    int bytesleft;
    int chunksize;

    fprintf(stderr,"[INFO] Seeking to sector %ld\n",start_sector);

    if (ipod_seek(dh,start_sector*sector_size) < 0) {
        return -1;
    }

    fprintf(stderr,"[INFO] Writing %ld sectors to output file\n",count);

    bytesleft = count * sector_size;
    while (bytesleft > 0) {
        if (bytesleft > BUFFER_SIZE) {
           chunksize = BUFFER_SIZE;
        } else {
           chunksize = bytesleft;
        }

        n = ipod_read(dh, sectorbuf, chunksize);

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

int write_partition(HANDLE dh, int infile,unsigned long start_sector, 
                    int sector_size)
{
    unsigned long res;
    int n;
    int bytesread;
    int byteswritten = 0;
    int eof;
    int padding = 0;

    if (ipod_seek(dh, start_sector*sector_size) < 0) {
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
           if ((n % sector_size) != 0) {
               padding = (sector_size-(n % sector_size));
               n += padding; 
           }
        }

        bytesread += n;

        res = ipod_write(dh, sectorbuf,n);

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


void print_usage(void) {
    fprintf(stderr,"Usage: ipodpatcher --scan\n");
#ifdef __WIN32__
    fprintf(stderr,"    or ipodpatcher DISKNO [action]\n");
#else
    fprintf(stderr,"    or ipodpatcher device [action]\n");
#endif
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of the following options:\n");
    fprintf(stderr,"  -l,  --list\n");
    fprintf(stderr,"  -r,  --read-partition     bootpartition.bin\n");
    fprintf(stderr,"  -w,  --write-partition    bootpartition.bin\n");
    fprintf(stderr,"  -rf, --read-firmware      filename.ipod\n");
    fprintf(stderr,"  -wf, --write-firmware     filename.ipod\n");
    fprintf(stderr,"  -a,  --add-bootloader     filename.ipod\n");
    fprintf(stderr,"  -ab, --add-bootloader-bin filename.bin\n");
    fprintf(stderr,"  -d,  --delete-bootloader\n");
    fprintf(stderr,"\n");

#ifdef __WIN32__
    fprintf(stderr,"DISKNO is the number (e.g. 2) Windows has assigned to your ipod's hard disk.\n");
    fprintf(stderr,"The first hard disk in your computer (i.e. C:\\) will be disk0, the next disk\n");
    fprintf(stderr,"will be disk 1 etc.  ipodpatcher will refuse to access a disk unless it\n");
    fprintf(stderr,"can identify it as being an ipod.\n");
    fprintf(stderr,"\n");
#else
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/sda) assigned to your ipod.\n");
    fprintf(stderr,"ipodpatcher will refuse to access a disk unless it can identify it as being\n");
    fprintf(stderr,"an ipod.\n");
#endif
}

enum {
   NONE,
   SHOW_INFO,
   LIST_IMAGES,
   DELETE_BOOTLOADER,
   ADD_BOOTLOADER,
   READ_FIRMWARE,
   WRITE_FIRMWARE,
   READ_PARTITION,
   WRITE_PARTITION
};

#define DOT_IPOD 0
#define DOT_BIN  1

char* ftypename[] = { "OSOS", "RSRC", "AUPD", "HIBE" };

enum firmwaretype_t {
   FTYPE_OSOS = 0,
   FTYPE_RSRC,
   FTYPE_AUPD,
   FTYPE_HIBE
};

struct ipod_directory_t {
  enum firmwaretype_t ftype;
  int id;
  uint32_t devOffset; /* Offset of image relative to one sector into bootpart*/
  uint32_t len;
  uint32_t addr;
  uint32_t entryOffset;
  uint32_t chksum;
  uint32_t vers;
  uint32_t loadAddr;
};


int diskmove(HANDLE dh, int start, int nimages, struct ipod_directory_t* ipod_directory,
             int sector_size,int delta)
{
    int src_start;
    int src_end;
    int bytesleft;
    int chunksize;
    int i;
    int n;

    src_start = ipod_directory[1].devOffset + sector_size;
    src_end = (ipod_directory[nimages-1].devOffset + sector_size + ipod_directory[nimages-1].len + (sector_size-1)) & ~(sector_size-1);
    bytesleft = src_end - src_start;

    if (verbose) {
        fprintf(stderr,"[INFO] Need to move images 2-%d forward %08x bytes\n",nimages,delta);
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
                           start+src_end-chunksize,
                           start+src_end-chunksize+delta);
        }


        if (ipod_seek(dh,start+src_end-chunksize) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if ((n = ipod_read(dh,sectorbuf,chunksize)) < 0) {
            perror("[ERR]  Write failed\n");
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                           i,n);
            return -1;
        }

        if (ipod_seek(dh,start+src_end-chunksize+delta) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if ((n = ipod_write(dh,sectorbuf,chunksize)) < 0) {
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

int add_bootloader(HANDLE dh, int type, char* filename, int start, int sector_size, 
                   int nimages, struct ipod_directory_t* ipod_directory, 
                   off_t diroffset, int modelnum, char* modelname)
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

    /* First check that the input file is the correct type for this ipod. */
    infile=open(filename,O_RDONLY);
    if (infile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
        return -1;
    }

    if (type==DOT_IPOD) {
        n = read(infile,header,8);
        if (n < 8) {
            fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
            close(infile);
            return -1;
        }

        if (memcmp(header+4,modelname,4)!=0) {
            fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                    header[4],header[5],header[6],header[7],modelname);
            close(infile);
            return -1;
        }

        filechksum = be2int(header);

        length=filesize(infile)-8;
    } else {
        length=filesize(infile);
    }
    paddedlength=(length+sector_size-1)&~(sector_size-1);

    /* Now read our bootloader - we need to check it before modifying the partition*/
    n = read(infile,sectorbuf,length);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Couldn't read input file\n");
        close(infile);
        return -1;
    }

    if (type==DOT_IPOD) {
        /* Calculate and confirm bootloader checksum */
        chksum = modelnum;
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

    if (ipod_directory[0].entryOffset>0) {
        /* Keep the same entryOffset */
        entryOffset = ipod_directory[0].entryOffset;
    } else {
        entryOffset = (ipod_directory[0].len+sector_size-1)&~(sector_size-1);
    }

    if (entryOffset+paddedlength > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        close(infile);
        return -1;
    }

    if (verbose) {
        fprintf(stderr,"[VERB] Original firmware begins at 0x%08x\n",ipod_directory[0].devOffset + sector_size);
        fprintf(stderr,"[VERB] New entryOffset will be 0x%08x\n",entryOffset);
        fprintf(stderr,"[VERB] End of bootloader will be at 0x%08x\n",entryOffset+paddedlength);
    }

    /* Check if we have enough space */
    /* TODO: Check the size of the partition. */
    if (nimages > 1) {
        if ((entryOffset+paddedlength) >= ipod_directory[1].devOffset) {
            fprintf(stderr,"[INFO] Moving images to create room for new firmware...\n");
            delta = ipod_directory[0].devOffset + entryOffset+paddedlength
                    - ipod_directory[1].devOffset;

            if (diskmove(dh,start,nimages,ipod_directory,sector_size,delta) < 0) {
                close(infile);
                fprintf(stderr,"[ERR]  Image movement failed.\n");
                return -1;
            }
        }
    }


    /* We have moved the partitions, now we can write our bootloader */

    /* Firstly read the original firmware into sectorbuf */
    fprintf(stderr,"[INFO] Reading original firmware...\n");

    if (ipod_seek(dh,start+sector_size+ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_read(dh,sectorbuf,entryOffset)) < 0) {
        perror("[ERR]  Read failed\n");
        return -1;
    }

    if (n < entryOffset) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                      ,i,n);
        return -1;
    }

    /* Now read our bootloader - we need to seek back to the start */
    lseek(infile,(type == DOT_IPOD ? 8 : 0),SEEK_SET);
    n = read(infile,sectorbuf+entryOffset,length);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Couldn't read input file\n");
        close(infile);
        return -1;
    }
    close(infile);

    /* Calculate new checksum for combined image */
    chksum = 0;
    for (i=0;i<entryOffset + length; i++) {
         chksum += sectorbuf[i];
    }    

    /* Now write the combined firmware image to the disk */

    if (ipod_seek(dh,start+sector_size+ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(dh,sectorbuf,entryOffset+paddedlength)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < (entryOffset+paddedlength)) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                      ,i,n);
        return -1;
    }

    fprintf(stderr,"[INFO]  Wrote %d bytes to firmware partition\n",entryOffset+paddedlength);

    x = diroffset % sector_size;

    /* Read directory */
    if (ipod_seek(dh,start + diroffset - x) < 0) { return -1; }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image 0 */
    int2le(entryOffset+length,sectorbuf+x+16);
    int2le(entryOffset,sectorbuf+x+24);
    int2le(chksum,sectorbuf+x+28);

    /* Update devOffset entries for other images, if we have moved them */
    if (delta > 0) {
        for (i=1;i<nimages;i++) {
           int2le(le2int(sectorbuf+x+i*40+12)+delta,sectorbuf+x+i*40+12);
        }
    }

    /* Write directory */    
    if (ipod_seek(dh,start + diroffset - x) < 0) { return -1; }
    n=ipod_write(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int delete_bootloader(HANDLE dh, int start, int sector_size, off_t diroffset,
                      struct ipod_directory_t* ipod_directory)
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

    if (ipod_directory[0].entryOffset == 0) {
        fprintf(stderr,"[ERR]  No bootloader found.\n");
        return -1;
    }

    length = ipod_directory[0].entryOffset;

    /* Read the firmware so we can calculate the checksum */
    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (ipod_seek(dh,start+sector_size+ipod_directory[0].devOffset) < 0) {
        return -1;
    }

    i = (length+sector_size-1) & ~(sector_size-1);
    fprintf(stderr,"[INFO] Padding read from 0x%08x to 0x%08x bytes\n",
            length,i);

    if ((n = ipod_read(dh,sectorbuf,i)) < 0) {
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

    x = diroffset % sector_size;

    /* Read directory */
    if (ipod_seek(dh,start + diroffset - x) < 0) { return -1; }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image 0 */
    int2le(length,sectorbuf+x+16);
    int2le(0,sectorbuf+x+24);
    int2le(chksum,sectorbuf+x+28);

    /* Write directory */    
    if (ipod_seek(dh,start + diroffset - x) < 0) { return -1; }
    n=ipod_write(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int write_firmware(HANDLE dh, char* filename, int start, int sector_size, 
                   int nimages, struct ipod_directory_t* ipod_directory, 
                   off_t diroffset, int modelnum, char* modelname)
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

    n = read(infile,header,8);
    if (n < 8) {
        fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
        close(infile);
        return -1;
    }

    if (memcmp(header+4,modelname,4)!=0) {
        fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                header[4],header[5],header[6],header[7],modelname);
        close(infile);
        return -1;
    }

    filechksum = be2int(header);

    length=filesize(infile)-8;
    newsize=(length+sector_size-1)&~(sector_size-1);

    fprintf(stderr,"[INFO] Padding input file from 0x%08x to 0x%08x bytes\n",
            length,newsize);

    if (newsize > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        close(infile);
        return -1;
    }

    /* Check if we have enough space */
    /* TODO: Check the size of the partition. */
    if (nimages > 1) {
        bytesavailable=ipod_directory[1].devOffset-ipod_directory[0].devOffset;
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

    chksum = modelnum;
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

    if (ipod_seek(dh,start+sector_size+ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(dh,sectorbuf,newsize)) < 0) {
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

    x = diroffset % sector_size;

    /* Read directory */
    if (ipod_seek(dh,start + diroffset - x) < 0) { return -1; }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image 0 */
    int2le(length,sectorbuf+x+16);
    int2le(0,sectorbuf+x+24);
    int2le(chksum,sectorbuf+x+28);

    /* Write directory */    
    if (ipod_seek(dh,start + diroffset - x) < 0) { return -1; }
    n=ipod_write(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int read_firmware(HANDLE dh, char* filename, int start, int sector_size, 
                     struct ipod_directory_t* ipod_directory, 
                     int modelnum, char* modelname)
{
    int length;
    int i;
    int outfile;
    int n;
    unsigned long chksum=0;   /* 32 bit checksum - Rockbox .ipod style*/
    unsigned char header[8];  /* Header for .ipod file */

    if (ipod_directory[0].entryOffset != 0) {
        /* We have a bootloader... */
        length = ipod_directory[0].entryOffset;
    } else {
        length = ipod_directory[0].len;
    }

    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (ipod_seek(dh,start+sector_size+ipod_directory[0].devOffset) < 0) {
        return -1;
    }

    i = (length+sector_size-1) & ~(sector_size-1);
    fprintf(stderr,"[INFO] Padding read from 0x%08x to 0x%08x bytes\n",
            length,i);

    if ((n = ipod_read(dh,sectorbuf,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    chksum = modelnum;
    for (i = 0; i < length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += sectorbuf[i];
    }

    int2be(chksum,header);
    memcpy(header+4,modelname,4);

    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return -1;
    }

    write(outfile,header,8);
    write(outfile,sectorbuf,length);
    close(outfile);

    return 0;
}

int read_directory(HANDLE dh, int start, int sector_size, 
                   struct ipod_directory_t* ipod_directory, off_t* diroffset)
{
    int n;
    int x;
    int nimages;
    unsigned char* p;

    /* Read firmware partition header (first 512 bytes of disk - but 
       let's read a whole sector) */

    if (ipod_seek(dh, start) < 0) { 
        fprintf(stderr,"[ERR]  Seek to 0x%08x in read_directory() failed.\n",start);
        return -1;
    }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  ipod_read(dh,buf,0x%08x) failed in read_directory()\n",sector_size);
        return -1;
    }

    if (memcmp(sectorbuf,apple_stop_sign,sizeof(apple_stop_sign))!=0) {
        fprintf(stderr,"[ERR]  Firmware partition doesn't contain Apple copyright, aborting.");
        return -1;
    }

    if (memcmp(sectorbuf+0x100,"]ih[",4)!=0) {
        fprintf(stderr,"[ERR]  Bad firmware directory\n");
        return -1;
    }

    *diroffset=le2int(sectorbuf+0x104) + 0x200;

    /* diroffset may not be sector-aligned */
    x = *diroffset % sector_size;

    /* Read directory */
    if (ipod_seek(dh,start + *diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)*diroffset);
        return -1;
    }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Read of directory failed.\n");
        return -1;
    }

    nimages=0;
    p = sectorbuf + x;
    
    while ((nimages < MAX_IMAGES) && (p < (sectorbuf + x + 400)) && 
           (memcmp(p,"!ATA",4)==0)) {
        p+=4;
        if (memcmp(p,"soso",4)==0) {
            ipod_directory[nimages].ftype=FTYPE_OSOS;
        } else if (memcmp(p,"crsr",4)==0) {
            ipod_directory[nimages].ftype=FTYPE_RSRC;
        } else if (memcmp(p,"dpua",4)==0) {
            ipod_directory[nimages].ftype=FTYPE_AUPD;
        } else if (memcmp(p,"ebih",4)==0) {
            ipod_directory[nimages].ftype=FTYPE_HIBE;
        } else {
            fprintf(stderr,"[ERR]  Unknown image type %c%c%c%c\n",
                           p[0],p[1],p[2],p[3]);
        }
        p+=4;
        ipod_directory[nimages].id=le2int(p);
        p+=4;
        ipod_directory[nimages].devOffset=le2int(p);
        p+=4;
        ipod_directory[nimages].len=le2int(p);
        p+=4;
        ipod_directory[nimages].addr=le2int(p);
        p+=4;
        ipod_directory[nimages].entryOffset=le2int(p);
        p+=4;
        ipod_directory[nimages].chksum=le2int(p);
        p+=4;
        ipod_directory[nimages].vers=le2int(p);
        p+=4;
        ipod_directory[nimages].loadAddr=le2int(p);
        p+=4;
        nimages++;
    }
    return nimages;
}

int list_images(int nimages, struct ipod_directory_t* ipod_directory, 
                int sector_size)
{
    int i;

    if (verbose) {
        printf("    Type         id  devOffset        len       addr entryOffset    chksum       vers   loadAddr   devOffset+len\n");
        for (i = 0 ; i < nimages; i++) {
            printf("%d - %s 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",i,
                   ftypename[ipod_directory[i].ftype],
                   ipod_directory[i].id,
                   ipod_directory[i].devOffset,
                   ipod_directory[i].len,
                   ipod_directory[i].addr,
                   ipod_directory[i].entryOffset,
                   ipod_directory[i].chksum,
                   ipod_directory[i].vers,
                   ipod_directory[i].loadAddr,
                   ipod_directory[i].devOffset+sector_size+((ipod_directory[i].len+sector_size-1)&~(sector_size-1)));
        }
    }

    printf("\n");
    printf("Listing firmware partition contents:\n");
    printf("\n");

    for (i = 0 ; i < nimages; i++) {
         printf("Image %d:\n",i+1);
         switch(ipod_directory[i].ftype) {
             case FTYPE_OSOS:
                 if (ipod_directory[i].entryOffset==0) {
                     printf("    Main firmware - %d bytes\n",
                       ipod_directory[i].len);
                 } else {
                     printf("    Main firmware - %d bytes\n",
                       ipod_directory[i].entryOffset);
                     printf("    Third-party bootloader - %d bytes\n",
                       ipod_directory[i].len-ipod_directory[i].entryOffset);
                 }
                 break;
             default:
                     printf("    %s - %d bytes\n",
                            ftypename[ipod_directory[i].ftype],
                            ipod_directory[i].len);
         }
    }
    printf("\n");

    return 0;
}

int getmodel(int ipod_version, char** modelstr, char** modelname, int* modelnum)
{
    switch (ipod_version) {
        case 0x3:
            *modelstr="3rd Generation";
            *modelnum = 7;
            *modelname = "ip3g";
            break;
        case 0x4:
            *modelstr="1st Generation Mini";
            *modelnum = 9;
            *modelname = "mini";
            break;
        case 0x5:
            *modelstr="4th Generation";
            *modelnum = 8;
            *modelname = "ip4g";
            break;
        case 0x6:
            *modelstr="Photo/Color";
            *modelnum = 3;
            *modelname = "ipco";
            break;
        case 0x7:
            *modelstr="2nd Generation Mini";
            *modelnum = 11;
            *modelname = "mn2g";
            break;
        case 0xc:
            *modelstr="1st Generation Nano";
            *modelnum = 4;
            *modelname = "nano";
            break;
        case 0xb:
            *modelstr="Video (aka 5th Generation)";
            *modelnum = 5;
            *modelname = "ipvd";
            break;
        default:
            *modelname = NULL;
            *modelnum = 0;
            return -1;
    }
    return 0;
}

int ipod_scan(void)
{
    int i;
    int n = 0;
    char devicename[96];
    HANDLE dh;
    int nimages;
    struct partinfo_t pinfo[4]; /* space for 4 partitions on 1 drive */
    int ipod_version;
    off_t diroffset;
    char* modelname;
    char* modelstr;
    int modelnum;
    struct ipod_directory_t ipod_directory[MAX_IMAGES];
    int sector_size;

    printf("[INFO] Scanning disk devices...\n");

    for (i = 0; i < 25 ; i++) {
#ifdef __WIN32__
         sprintf(devicename,"\\\\.\\PhysicalDrive%d",i);
#elif defined(linux) || defined (__linux)
         sprintf(devicename,"/dev/sd%c",'a'+i);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
         sprintf(devicename,"/dev/da%d",i);
#elif defined(__APPLE__) && defined(__MACH__)
         sprintf(devicename,"/dev/disk%d",i);
#else
    #error No disk paths defined for this platform
#endif
         if (ipod_open(&dh, devicename, &sector_size, 1) < 0) {
             continue;
         }

         if (read_partinfo(dh,sector_size,pinfo,1) < 0) {
             continue;
         }

         if ((pinfo[0].start==0) || (pinfo[0].type != 0)) {
             continue;
         }

         nimages=read_directory(dh, pinfo[0].start*sector_size, sector_size, 
                                ipod_directory, &diroffset);

         if (nimages < 0) {
             continue;
         }

         ipod_version=(ipod_directory[0].vers>>12) & 0x0f;
         if (getmodel(ipod_version,&modelstr,&modelname,&modelnum) < 0) {
             continue;
         }

#ifdef __WIN32__
         printf("[INFO] Ipod found - %s - disk device %d\n",modelstr,i);
#else
         printf("[INFO] Ipod found - %s - %s\n",modelstr,devicename);
#endif
         n++;
    }

    if (n==0) {
        fprintf(stderr,"[ERR]  No ipods found.\n");
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int i;
    int infile, outfile;
    unsigned int inputsize;
    struct partinfo_t pinfo[4]; /* space for 4 partitions on 1 drive */
    int ipod_version;
    int nimages;
    off_t diroffset;
    char* modelname;
    char* modelstr;
    int modelnum;
    char* filename;
    struct ipod_directory_t ipod_directory[MAX_IMAGES];
    int action = SHOW_INFO;
    int sector_size;
    int type;
    char devicename[4096];
    HANDLE dh;

    fprintf(stderr,"ipodpatcher v" VERSION " - (C) Dave Chapman 2006\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if ((argc < 2) || (strcmp(argv[1],"-h")==0) || 
                      (strcmp(argv[1],"--help")==0)) {
        print_usage();
        return 1;
    }

    if (ipod_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) {
        fprintf(stderr,"Failed to allocate memory buffer\n");
    }

    if (strcmp(argv[1],"--scan")==0) {
        ipod_scan();
        return 0;
    }

    i = 1;
    devicename[0]=0;

#ifdef __WIN32__
     snprintf(devicename,sizeof(devicename),"\\\\.\\PhysicalDrive%s",argv[1]);
#else
     strncpy(devicename,argv[1],sizeof(devicename));
#endif

    i = 2;
    while (i < argc) {
        if ((strcmp(argv[i],"-l")==0) || (strcmp(argv[i],"--list")==0)) {
            action = LIST_IMAGES;
            i++;
        } else if ((strcmp(argv[i],"-d")==0) || 
                   (strcmp(argv[i],"--delete-bootloader")==0)) {
            action = DELETE_BOOTLOADER;
            i++;
        } else if ((strcmp(argv[i],"-a")==0) || 
                   (strcmp(argv[i],"--add-bootloader")==0)) {
            action = ADD_BOOTLOADER;
            type = DOT_IPOD;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-ab")==0) || 
                   (strcmp(argv[i],"--add-bootloader-bin")==0)) {
            action = ADD_BOOTLOADER;
            type = DOT_BIN;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-rf")==0) || 
                   (strcmp(argv[i],"--read-firmware")==0)) {
            action = READ_FIRMWARE;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-wf")==0) || 
                   (strcmp(argv[i],"--write-firmware")==0)) {
            action = WRITE_FIRMWARE;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-r")==0) || 
                   (strcmp(argv[i],"--read-partition")==0)) {
            action = READ_PARTITION;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-w")==0) || 
                   (strcmp(argv[i],"--write-partition")==0)) {
            action = WRITE_PARTITION;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-v")==0) || 
                   (strcmp(argv[i],"--verbose")==0)) {
            verbose++;
            i++;
        } else {
            print_usage(); return 1;
        }
    }

    if (devicename[0]==0) {
        print_usage();
        return 1;
    }

    if (ipod_open(&dh, devicename, &sector_size, 0) < 0) {
        return 1;
    }

    fprintf(stderr,"[INFO] Reading partition table from %s\n",devicename);
    fprintf(stderr,"[INFO] Sector size is %d bytes\n",sector_size);

    if (read_partinfo(dh,sector_size,pinfo,0) < 0) {
        return 2;
    }

    display_partinfo(pinfo, sector_size);

    if (pinfo[0].start==0) {
        fprintf(stderr,"[ERR]  No partition 0 on disk:\n");
        display_partinfo(pinfo, sector_size);
        return 3;
    }

    nimages=read_directory(dh, pinfo[0].start*sector_size, sector_size, 
                           ipod_directory, &diroffset);
    if (nimages <= 0) {
        fprintf(stderr,"[ERR]  Failed to read firmware directory - nimages=%d\n",nimages);
        return 1;
    }

    ipod_version=(ipod_directory[0].vers>>12) & 0x0f;
    if (getmodel(ipod_version,&modelstr,&modelname,&modelnum) < 0) {
        fprintf(stderr,"[ERR] Unknown version number in firmware (%08x)\n",
                       ipod_version);
        return -1;
    }

    printf("[INFO] Ipod model: %s\n",modelstr);

    if (action==LIST_IMAGES) {
        list_images(nimages,ipod_directory,sector_size);
    } else if (action==DELETE_BOOTLOADER) {
        if (ipod_reopen_rw(&dh, devicename) < 0) {
            return 5;
        }

        if (ipod_directory[0].entryOffset==0) {
            fprintf(stderr,"[ERR]  No bootloader detected.\n");
        } else {
            if (delete_bootloader(dh, pinfo[0].start*sector_size, sector_size, 
                               diroffset, ipod_directory)==0) {
                fprintf(stderr,"[INFO] Bootloader removed.\n");
            } else {
                fprintf(stderr,"[ERR]  --delete-bootloader failed.\n");
            }
        }
    } else if (action==ADD_BOOTLOADER) {
        if (ipod_reopen_rw(&dh, devicename) < 0) {
            return 5;
        }

        if (add_bootloader(dh, type, filename,pinfo[0].start*sector_size, 
                           sector_size, nimages, ipod_directory, diroffset, 
                           modelnum, modelname)==0) {
            fprintf(stderr,"[INFO] Bootloader %s written to device.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --add-bootloader failed.\n");
        }
    } else if (action==WRITE_FIRMWARE) {
        if (ipod_reopen_rw(&dh, devicename) < 0) {
            return 5;
        }

        if (write_firmware(dh, filename,pinfo[0].start*sector_size, 
                           sector_size, nimages, ipod_directory, diroffset, 
                           modelnum, modelname)==0) {
            fprintf(stderr,"[INFO] Firmware %s written to device.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --write-firmware failed.\n");
        }
    } else if (action==READ_FIRMWARE) {
        if (read_firmware(dh, filename,pinfo[0].start*sector_size, 
                          sector_size, ipod_directory, modelnum, modelname
                          )==0) {
            fprintf(stderr,"[INFO] Firmware read to file %s.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --read-firmware failed.\n");
        }
    } else if (action==READ_PARTITION) {
        outfile = open(filename,O_CREAT|O_WRONLY|O_BINARY,S_IREAD|S_IWRITE);
        if (outfile < 0) {
           perror(filename);
           return 4;
        }

        if (read_partition(dh, outfile, pinfo[0].start, pinfo[0].size, 
                           sector_size) < 0) {
            fprintf(stderr,"[ERR]  --read-partition failed.\n");
        } else {
            fprintf(stderr,"[INFO] Partition extracted to %s.\n",filename);
        }
        close(outfile);
    } else if (action==WRITE_PARTITION) {
        if (ipod_reopen_rw(&dh, devicename) < 0) {
            return 5;
        }

        infile = open(filename,O_RDONLY|O_BINARY);
        if (infile < 0) {
            perror(filename);
            return 2;
        }

        /* Check filesize is <= partition size */
        inputsize=filesize(infile);
        if (inputsize > 0) {
            if (inputsize <= (pinfo[0].size*sector_size)) {
                fprintf(stderr,"[INFO] Input file is %u bytes\n",inputsize);
                if (write_partition(dh,infile,pinfo[0].start,
                    sector_size) < 0) {
                    fprintf(stderr,"[ERR]  --write-partition failed.\n");
                } else {
                    fprintf(stderr,"[INFO] %s restored to partition\n",filename);
                }
            } else {
                fprintf(stderr,"[ERR]  File is too large for firmware partition, aborting.\n");
            }
        }

        close(infile);
    }

    ipod_close(dh);

    return 0;
}
