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
 * error(), lock_volume() and unlock_volume() functions and inspiration taken
 * from:
 *       RawDisk - Direct Disk Read/Write Access for NT/2000/XP
 *       Copyright (c) 2003 Jan Kiszka
 *       http://www.stud.uni-hannover.de/user/73174/RawDisk/
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
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <winioctl.h>

#include "parttypes.h"

/* Windows requires the buffer for disk I/O to be aligned in memory on a 
   multiple of the disk volume size - so we use a single global variable
   and initialise it in main() 
*/
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

void error(char* msg)
{
    char* pMsgBuf;

    printf(msg);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pMsgBuf,
                  0, NULL);
    printf(pMsgBuf);
    LocalFree(pMsgBuf);
}

int lock_volume(HANDLE hDisk) 
{ 
  DWORD dummy;

  return DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
			 &dummy, NULL); 
}

int unlock_volume(HANDLE hDisk) 
{ 
  DWORD dummy;

  return DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0,
			 &dummy, NULL); 
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


/* Size of buffer for disk I/O */
#define BUFFER_SIZE 32*1024

/* Partition table parsing code taken from Rockbox */

#define SECTOR_SIZE 512

struct partinfo {
  unsigned long start; /* first sector (LBA) */
  unsigned long size;  /* number of sectors */
  unsigned char type;
};

#define BYTES2INT32(array,pos)					\
    ((long)array[pos] | ((long)array[pos+1] << 8 ) |		\
    ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

void display_partinfo(struct partinfo* pinfo)
{
    int i;

    printf("Part    Start Sector    End Sector    Size (MB)  Type\n");
    for ( i = 0; i < 4; i++ ) {
        if (pinfo[i].start != 0) {
            printf("   %d      %10ld    %10ld  %10.1f   %s (0x%02x)\n",i,pinfo[i].start,pinfo[i].start+pinfo[i].size-1,pinfo[i].size/2048.0,get_parttype(pinfo[i].type),pinfo[i].type);
        }
    }
}


int read_partinfo(HANDLE dh, struct partinfo* pinfo)
{
    int i;
    unsigned char sector[SECTOR_SIZE];
    unsigned long count;

    if (!ReadFile(dh, sector, SECTOR_SIZE, &count, NULL)) {
        error(" Error reading from disk: ");
        return -1;
    }

    /* check that the boot sector is initialized */
    if ( (sector[510] != 0x55) ||
         (sector[511] != 0xaa)) {
        fprintf(stderr,"Bad boot sector signature\n");
        return 0;
    }

    if (memcmp(&sector[71],"iPod",4) != 0) {
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

int disk_read(HANDLE dh, int outfile,unsigned long start, unsigned long count)
{
    int res;
    unsigned long n;
    int bytesleft;
    int chunksize;

    fprintf(stderr,"[INFO] Seeking to sector %ld\n",start);

    if (SetFilePointer(dh, start*SECTOR_SIZE, NULL, FILE_BEGIN)==0xffffffff) {
        error(" Seek error ");
        return -1;
    }

    fprintf(stderr,"[INFO] Writing %ld sectors to output file\n",count);

    bytesleft = count * SECTOR_SIZE;
    while (bytesleft > 0) {
        if (bytesleft > BUFFER_SIZE) {
           chunksize = BUFFER_SIZE;
        } else {
           chunksize = bytesleft;
        }

        if (!ReadFile(dh, sectorbuf, chunksize, &n, NULL)) {
            error("[ERR] read in disk_read");
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
            fprintf(stderr,"Short write - requested %d, received %d - aborting.\n",SECTOR_SIZE,res);
            return -1;
        }
    }

    fprintf(stderr,"[INFO] Done.\n");
    return 0;
}

int disk_write(HANDLE dh, int infile,unsigned long start)
{
    unsigned long res;
    int n;
    int bytesread;
    int byteswritten = 0;
    int eof;
    int padding = 0;

    if (SetFilePointer(dh, start*SECTOR_SIZE, NULL, FILE_BEGIN)==0xffffffff) {
        error(" Seek error ");
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
           if ((n % SECTOR_SIZE) != 0) {
               padding = (SECTOR_SIZE-(n % SECTOR_SIZE));
               n += padding; 
           }
        }

        bytesread += n;

        if (!WriteFile(dh, sectorbuf, n, &res, NULL)) {
            error(" Error writing to disk: ");
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
    fprintf(stderr,"Usage: ipodpatcher  [-i|r|w] DISKNO [file]\n");
    fprintf(stderr,"         -i    Display iPod's partition information (default)\n");
    fprintf(stderr,"         -r    Read firmware partition to file\n");
    fprintf(stderr,"         -w    Write file to firmware partition\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"DISKNO is the number (e.g. 2) Windows has assigned to your ipod's hard disk.\n");
    fprintf(stderr,"The first hard disk in your computer (i.e. C:\\) will be disk0, the next disk\n");
    fprintf(stderr,"will be disk 1 etc.  ipodpatcher will refuse to access a disk unless it\n");
    fprintf(stderr,"can identify it as being an ipod.\n");
    fprintf(stderr,"\n");
}

enum {
   NONE,
   SHOW_INFO,
   READ,
   WRITE
};

int main(int argc, char* argv[])
{
    int i;
    struct partinfo pinfo[4]; /* space for 4 partitions on 1 drive */
    int res;
    int outfile;
    int infile;
    int mode = SHOW_INFO;
    int p = 0;
    int diskno = -1;
    char diskname[32];
    HANDLE dh;
    char* filename = NULL;
    off_t inputsize;

    fprintf(stderr,"ipodpatcher v0.2 - (C) Dave Chapman 2005\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    
    if (argc <= 1) {
        print_usage();
        return 1;
    }

    i = 1;
    while (i < argc) {
        if (strncmp(argv[i],"-i",2)==0) {
            mode=SHOW_INFO;
        } else if (strncmp(argv[i],"-r",2)==0) {
            mode = READ;
        } else if (strncmp(argv[i],"-w",2)==0) {
            mode = WRITE;
        } else {
            if (argv[i][0] == '-') {
                fprintf(stderr,"Unknown option %s\n",argv[i]);
                return 1;
            } else {
                if (diskno == -1) {
                   diskno = atoi(argv[i]);
                } else if (filename==NULL) {
                   filename = argv[i];
                } else {
                   fprintf(stderr,"Too many arguments: %s\n",argv[i]);
                   return 1;
                }
            }
        }
        i++;
    }

    if ((mode==NONE) || (diskno==-1) || ((mode!=SHOW_INFO) && (filename==NULL))) {
        print_usage();
        return 1;
    }

    snprintf(diskname,sizeof(diskname),"\\\\.\\PhysicalDrive%d",diskno);

    fprintf(stderr,"[INFO] Reading partition table from %s\n",diskname);

    /* The ReadFile function requires a memory buffer aligned to a multiple of
       the disk sector size. */
    sectorbuf = VirtualAlloc(NULL, BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (sectorbuf == NULL) {
        error(" Error allocating a buffer: ");
        return 2;
    }

    dh = CreateFile(diskname, GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

    if (dh == INVALID_HANDLE_VALUE) {
        error(" Error opening disk: ");
        return 2;
    }

    if (!lock_volume(dh)) {
        error(" Error locking disk: ");
        return 2;
    }

    if (read_partinfo(dh,pinfo) < 0) {
        return 2;
    }

    display_partinfo(pinfo);

    if (pinfo[p].start==0) {
        fprintf(stderr,"[ERR]  Specified partition (%d) does not exist:\n",p);
        display_partinfo(pinfo);
        return 3;
    }

    if (mode==READ) {
        outfile = open(filename,O_CREAT|O_WRONLY|O_BINARY,S_IREAD|S_IWRITE);
        if (outfile < 0) {
           perror(filename);
           return 4;
        }

        res = disk_read(dh,outfile,pinfo[p].start,pinfo[p].size);

        close(outfile);
    } else if (mode==WRITE) {
        /* Close existing file and re-open for writing */
        unlock_volume(dh);
        CloseHandle(dh);

        dh = CreateFile(diskname, GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

        if (dh == INVALID_HANDLE_VALUE) {
            error(" Error opening disk: ");
            return 2;
        }

        if (!lock_volume(dh)) {
            error(" Error locking disk: ");
            return 2;
        }

        infile = open(filename,O_RDONLY|O_BINARY);
        if (infile < 0) {
            perror(filename);
            return 2;
        }

        /* Check filesize is <= partition size */
        inputsize=filesize(infile);
        if (inputsize > 0) {
            if (inputsize <= (pinfo[p].size*SECTOR_SIZE)) {
                fprintf(stderr,"[INFO] Input file is %lu bytes\n",inputsize);
                res = disk_write(dh,infile,pinfo[p].start);
            } else {
                fprintf(stderr,"[ERR]  File is too large for firmware partition, aborting.\n");
            }
        }

        close(infile);
    }

    unlock_volume(dh);
    CloseHandle(dh);
    return 0;
}
