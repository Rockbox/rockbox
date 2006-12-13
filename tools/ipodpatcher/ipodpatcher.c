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

#define VERSION "0.5"

//#define DEBUG

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

/* Size of buffer for disk I/O */
#define BUFFER_SIZE 6*1024*1024
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

int static inline getint32le(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

int static inline getint16le(char* buf)
{
   int16_t res = (buf[1] << 8) | buf[0];

   return res;
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


int read_partinfo(HANDLE dh, int sector_size, struct partinfo_t* pinfo)
{
    int i;
    unsigned char sector[MAX_SECTOR_SIZE];
    unsigned long count;

    count = ipod_read(dh,sector,sector_size);

    if (count <= 0) {
        print_error(" Error reading from disk: ");
        return -1;
    }

    /* check that the boot sector is initialized */
    if ( (sector[510] != 0x55) ||
         (sector[511] != 0xaa)) {
        fprintf(stderr,"Bad boot sector signature\n");
        return 0;
    }

    if ((memcmp(&sector[71],"iPod",4) != 0) &&
        (memcmp(&sector[0x40],"This is your Apple iPod. You probably do not want to boot from it!",66) != 0) ) {
        fprintf(stderr,"Drive is not an iPod, aborting\n");
        return -1;
    }

    /* parse partitions */
    for ( i = 0; i < 4; i++ ) {
        unsigned char* ptr = sector + 0x1be + 16*i;
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

int disk_read(HANDLE dh, int outfile,unsigned long start, unsigned long count,
              int sector_size)
{
    int res;
    unsigned long n;
    int bytesleft;
    int chunksize;

    fprintf(stderr,"[INFO] Seeking to sector %ld\n",start);

    if (ipod_seek(dh,start) < 0) {
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
            fprintf(stderr,"[ERR]  Short read in disk_read() - requested %d, got %lu\n",chunksize,n);
            return -1;
        }

        bytesleft -= n;

        res = write(outfile,sectorbuf,n);

        if (res < 0) {
            perror("[ERR]  write in disk_read");
            return -1;
        }

        if (res != n) {
            fprintf(stderr,"Short write - requested %lu, received %d - aborting.\n",n,res);
            return -1;
        }
    }

    fprintf(stderr,"[INFO] Done.\n");
    return 0;
}

int disk_write(HANDLE dh, int infile,unsigned long start, int sector_size)
{
    unsigned long res;
    int n;
    int bytesread;
    int byteswritten = 0;
    int eof;
    int padding = 0;

    if (ipod_seek(dh, start*sector_size) < 0) {
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
            fprintf(stderr,"Short write - requested %d, received %lu - aborting.\n",n,res);
            return -1;
        }

        byteswritten += res;
    }

    fprintf(stderr,"[INFO] Wrote %d bytes plus %d bytes padding.\n",byteswritten-padding,padding);
    return 0;
}


void print_usage(void) {
#ifdef __WIN32__
    fprintf(stderr,"Usage: ipodpatcher DISKNO [action]\n");
#else
    fprintf(stderr,"Usage: ipodpatcher device [action]\n");
#endif
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of:\n");
#if 0
    fprintf(stderr,"     -e --extract-firmware filename.bin - extract firmware to a file\n");
    fprintf(stderr,"     -i --insert-firmware  filename.bin - replace the firmware with the file\n");
    fprintf(stderr,"     -a --add-bootloader   filename.bin - add a bootloader\n");
    fprintf(stderr,"     -r --remove-bootloader             - remove a bootloader\n");
#endif
    fprintf(stderr,"     -l --list                          - list images in firmware partition\n");
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
   LIST_IMAGES
};

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
  uint32_t devOffset;
  uint32_t len;
  uint32_t addr;
  uint32_t entryOffset;
  uint32_t chksum;
  uint32_t vers;
  uint32_t loadAddr;
};

int read_directory(HANDLE dh, int start, int sector_size, struct ipod_directory_t* ipod_directory)
{
    int n;
    int nimages;
    off_t diroffset;
    unsigned char* p;

    /* Read firmware partition header (first 512 bytes of disk - but let's read a whole sector) */

    if (ipod_seek(dh, start) < 0) { return -1; }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    if (memcmp(sectorbuf,apple_stop_sign,sizeof(apple_stop_sign))!=0) {
        fprintf(stderr,"[ERR]  Firmware partition doesn't contain Apple copyright, aborting.");
        return -1;
    }

    if (memcmp(sectorbuf+0x100,"]ih[",4)!=0) {
        fprintf(stderr,"[ERR] Bad firmware directory\n");
        return -1;
    }

    diroffset=getint32le(sectorbuf+0x104) + 0x200;

    /* Read directory */
    if (ipod_seek(dh,start + diroffset) < 0) { return -1; }

    n=ipod_read(dh, sectorbuf, sector_size);
    if (n < 0) { return -1; }

    nimages=0;
    p = sectorbuf;
    
    while ((nimages < MAX_IMAGES) && (p < (sectorbuf + 400)) && (memcmp(p,"!ATA",4)==0)) {
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
            fprintf(stderr,"[ERR]  Unknown image type %c%c%c%c\n",p[0],p[1],p[2],p[3]);
        }
        p+=4;
        ipod_directory[nimages].id=getint32le(p);
        p+=4;
        ipod_directory[nimages].devOffset=getint32le(p);
        p+=4;
        ipod_directory[nimages].len=getint32le(p);
        p+=4;
        ipod_directory[nimages].addr=getint32le(p);
        p+=4;
        ipod_directory[nimages].entryOffset=getint32le(p);
        p+=4;
        ipod_directory[nimages].chksum=getint32le(p);
        p+=4;
        ipod_directory[nimages].vers=getint32le(p);
        p+=4;
        ipod_directory[nimages].loadAddr=getint32le(p);
        p+=4;
        nimages++;
    }
    return nimages;
}

int list_images(int nimages, struct ipod_directory_t* ipod_directory)
{
    int i;

#ifdef DEBUG
    printf("    Type         id  devOffset        len       addr entryOffset    chksum       vers   loadAddr\n");
    for (i = 0 ; i < nimages; i++) {
        printf("%d - %s 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",i,
               ftypename[ipod_directory[i].ftype],
               ipod_directory[i].id,
               ipod_directory[i].devOffset,
               ipod_directory[i].len,
               ipod_directory[i].addr,
               ipod_directory[i].entryOffset,
               ipod_directory[i].chksum,
               ipod_directory[i].vers,
               ipod_directory[i].loadAddr);
    }
#endif

    printf("\n");
    printf("Listing firmware partition contents:\n");
    printf("\n");

    for (i = 0 ; i < nimages; i++) {
         printf("Image %d:\n",i+1);
         switch(ipod_directory[i].ftype) {
             case FTYPE_OSOS:
                 if (ipod_directory[i].entryOffset==0) {
                     printf("    Main firmware - %d bytes\n",ipod_directory[i].len);
                 } else {
                     printf("    Main firmware - %d bytes\n",ipod_directory[i].entryOffset);
                     printf("    Third-party bootloader - %d bytes\n",ipod_directory[i].len-ipod_directory[i].entryOffset);
                 }
                 break;
             default:
                     printf("    %s - %d bytes\n",ftypename[ipod_directory[i].ftype],ipod_directory[i].len);
         }
    }
    printf("\n");

    return 0;
}

int main(int argc, char* argv[])
{
    int i;
    int ipod_version;
    struct partinfo_t pinfo[4]; /* space for 4 partitions on 1 drive */
    int nimages;
    struct ipod_directory_t ipod_directory[MAX_IMAGES];
    int action = SHOW_INFO;
    int sector_size;
    char devicename[4096];
    HANDLE dh;

    fprintf(stderr,"ipodpatcher v" VERSION " - (C) Dave Chapman 2006\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    
    if (argc < 2) {
        print_usage();
        return 1;
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
        } else {
            print_usage(); return 1;
        }
    }

    if (devicename[0]==0) {
        print_usage();
        return 1;
    }

    if (ipod_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) {
        fprintf(stderr,"Failed to allocate memory buffer\n");
    }

    if (ipod_open(&dh, devicename, &sector_size) < 0) {
        return 1;
    }

    fprintf(stderr,"[INFO] Reading partition table from %s\n",devicename);
    fprintf(stderr,"[INFO] Sector size is %d bytes\n",sector_size);

    if (read_partinfo(dh,sector_size,pinfo) < 0) {
        return 2;
    }

    display_partinfo(pinfo, sector_size);

    if (pinfo[0].start==0) {
        fprintf(stderr,"[ERR]  No partition 0 on disk:\n");
        display_partinfo(pinfo, sector_size);
        return 3;
    }

    nimages=read_directory(dh, pinfo[0].start*sector_size, sector_size, ipod_directory);
    if (nimages <= 0) {
        fprintf(stderr,"[ERR]  Failed to read firmware directory\n");
        return 1;
    }

    ipod_version=(ipod_directory[0].vers>>12) & 0x0f;
    printf("[INFO] Ipod model: ");
    switch (ipod_version) {
        case 0x3: printf("3rd Generation\n"); break;
        case 0x4: printf("1st Generation Mini\n"); break;
        case 0x5: printf("4th Generation\n"); break;
        case 0x6: printf("Photo/Color\n"); break;
        case 0x7: printf("2nd Generation Mini\n"); break;
        case 0xc: printf("1st Generation Nano\n"); break;
        case 0xb: printf("Video (aka 5th Generation)\n"); break;
        default: printf("UNKNOWN (Firmware version is %08x)\n",ipod_directory[0].vers);
    }

    if (action==LIST_IMAGES) {
        list_images(nimages,ipod_directory);
#if 0
    } else if (mode==READ) {
        outfile = open(filename,O_CREAT|O_WRONLY|O_BINARY,S_IREAD|S_IWRITE);
        if (outfile < 0) {
           perror(filename);
           return 4;
        }

        res = disk_read(dh,outfile,pinfo[p].start,pinfo[p].size,sector_size);

        close(outfile);
    } else if (mode==WRITE) {
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
            if (inputsize <= (pinfo[p].size*sector_size)) {
                fprintf(stderr,"[INFO] Input file is %lu bytes\n",inputsize);
                res = disk_write(dh,infile,pinfo[p].start,sector_size);
            } else {
                fprintf(stderr,"[ERR]  File is too large for firmware partition, aborting.\n");
            }
        }

        close(infile);
#endif
    }

    ipod_close(dh);

    return 0;
}
