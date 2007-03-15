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
#include "sansaio.h"
#include "sansapatcher.h"
#include "bootimg.h"

/* The offset of the MI4 image header in the firmware partition */
#define PPMI_OFFSET 0x80000

extern int verbose;

/* Windows requires the buffer for disk I/O to be aligned in memory on a 
   multiple of the disk volume size - so we use a single global variable
   and initialise it with sansa_alloc_buf() in main().
*/

unsigned char* sectorbuf;

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

int read_partinfo(struct sansa_t* sansa, int silent)
{
    int i;
    unsigned long count;

    count = sansa_read(sansa,sectorbuf, sansa->sector_size);

    if (count <= 0) {
        print_error(" Error reading from disk: ");
        return -1;
    }

    if ((sectorbuf[510] == 0x55) && (sectorbuf[511] == 0xaa)) {
        /* parse partitions */
        for ( i = 0; i < 4; i++ ) {
            unsigned char* ptr = sectorbuf + 0x1be + 16*i;
            sansa->pinfo[i].type  = ptr[4];
            sansa->pinfo[i].start = BYTES2INT32(ptr, 8);
            sansa->pinfo[i].size  = BYTES2INT32(ptr, 12);

            /* extended? */
            if ( sansa->pinfo[i].type == 5 ) {
                /* not handled yet */
            }
        }
    } else if ((sectorbuf[0] == 'E') && (sectorbuf[1] == 'R')) {
        if (!silent) fprintf(stderr,"[ERR]  Bad boot sector signature\n");
        return -1;
    }

    /* Calculate the starting position of the firmware partition */
    sansa->start = (loff_t)sansa->pinfo[1].start*(loff_t)sansa->sector_size;
    return 0;
}


/*
 * CRC32 implementation taken from:
 *
 * efone - Distributed internet phone system.
 *
 * (c) 1999,2000 Krzysztof Dabrowski
 * (c) 1999,2000 ElysiuM deeZine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *		so make sure, you call it before using the other
 *		functions!
 */
static unsigned int crc_tab[256];

/* chksum_crc() -- to a given block, this one calculates the
 *				crc32-checksum until the length is
 *				reached. the crc32-checksum will be
 *				the result.
 */
static unsigned int chksum_crc32 (unsigned char *block, unsigned int length)
{
   register unsigned long crc;
   unsigned long i;

   crc = 0;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc);
}

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *				calculate the crcTable for crc32-checksums.
 *				it is generated to the polynom [..]
 */

static void chksum_crc32gentab (void)
{
   unsigned long crc, poly;
   int i, j;

   poly = 0xEDB88320L;
   for (i = 0; i < 256; i++)
   {
      crc = i;
      for (j = 8; j > 0; j--)
      {
	 if (crc & 1)
	 {
	    crc = (crc >> 1) ^ poly;
	 }
	 else
	 {
	    crc >>= 1;
	 }
      }
      crc_tab[i] = crc;
   }
}

/* Known keys for Sansa E200 firmwares: */
#define NUM_KEYS 2
static uint32_t keys[][4] = {
    { 0xe494e96e, 0x3ee32966, 0x6f48512b, 0xa93fbb42 }, /* "sansa" */
    { 0xd7b10538, 0xc662945b, 0x1b3fce68, 0xf389c0e6 }, /* "sansa_gh" */
};

/*

tea_decrypt() from http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm

"Following is an adaptation of the reference encryption and decryption
routines in C, released into the public domain by David Wheeler and
Roger Needham:"

*/

/* NOTE: The mi4 version of TEA uses a different initial value to sum compared
         to the reference implementation and the main loop is 8 iterations, not
         32.
*/

void tea_decrypt(uint32_t* v0, uint32_t* v1, uint32_t* k) {
    uint32_t sum=0xF1BBCDC8, i;                    /* set up */
    uint32_t delta=0x9E3779B9;                     /* a key schedule constant */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for(i=0; i<8; i++) {                               /* basic cycle start */
        *v1 -= ((*v0<<4) + k2) ^ (*v0 + sum) ^ ((*v0>>5) + k3);
        *v0 -= ((*v1<<4) + k0) ^ (*v1 + sum) ^ ((*v1>>5) + k1);
        sum -= delta;                                   /* end cycle */
    }
}

/* mi4 files are encrypted in 64-bit blocks (two little-endian 32-bit
   integers) and the key is incremented after each block
 */

void tea_decrypt_buf(unsigned char* src, unsigned char* dest, size_t n, uint32_t * key)
{
    uint32_t v0, v1;
    int i;

    for (i = 0; i < (n / 8); i++) {
        v0 = le2int(src);
        v1 = le2int(src+4);

        tea_decrypt(&v0, &v1, key);

        int2le(v0, dest);
        int2le(v1, dest+4);

        src += 8;
        dest += 8;

        /* Now increment the key */
        key[0]++;
        if (key[0]==0) {
            key[1]++;
            if (key[1]==0) {
                key[2]++;
                if (key[2]==0) {
                    key[3]++;
                }
            }
        }
    }
}

static int get_mi4header(unsigned char* buf,struct mi4header_t* mi4header)
{
    if (memcmp(buf,"PPOS",4)!=0)
        return -1;

    mi4header->version   = le2int(buf+0x04);
    mi4header->length    = le2int(buf+0x08);
    mi4header->crc32     = le2int(buf+0x0c);
    mi4header->enctype   = le2int(buf+0x10);
    mi4header->mi4size   = le2int(buf+0x14);
    mi4header->plaintext = le2int(buf+0x18);

    return 0;
}

static int set_mi4header(unsigned char* buf,struct mi4header_t* mi4header)
{
    if (memcmp(buf,"PPOS",4)!=0)
        return -1;

    int2le(mi4header->version   ,buf+0x04);
    int2le(mi4header->length    ,buf+0x08);
    int2le(mi4header->crc32     ,buf+0x0c);
    int2le(mi4header->enctype   ,buf+0x10);
    int2le(mi4header->mi4size   ,buf+0x14);
    int2le(mi4header->plaintext ,buf+0x18);

    /* Add a dummy DSA signature */
    memset(buf+0x1c,0,40);
    buf[0x2f] = 1;

    return 0;
}

int sansa_seek_and_read(struct sansa_t* sansa, loff_t pos, unsigned char* buf, int nbytes)
{
    int n;

    if (sansa_seek(sansa, pos) < 0) {
        return -1;
    }

    if ((n = sansa_read(sansa,buf,nbytes)) < 0) {
        return -1;
    }

    if (n < nbytes) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                nbytes,n);
        return -1;
    }

    return 0;
}


/* We identify an E200 based on the following criteria:

   1) Exactly two partitions;
   2) First partition is type "W95 FAT32" (0x0b);
   3) Second partition is type "OS/2 hidden C: drive" (0x84);
   4) The "PPBL" string appears at offset 0 in the 2nd partition;
   5) The "PPMI" string appears at offset PPMI_OFFSET in the 2nd partition.
*/

int is_e200(struct sansa_t* sansa)
{
    struct mi4header_t mi4header;

    /* Check partition layout */

    if ((sansa->pinfo[0].type != 0x0b) || (sansa->pinfo[1].type != 0x84) ||
        (sansa->pinfo[2].type != 0x00) || (sansa->pinfo[3].type != 0x00)) {
        /* Bad partition layout, abort */
        return -1;
    }

    /* Check Bootloader header */
    if (sansa_seek_and_read(sansa, sansa->start, sectorbuf, 0x200) < 0) {
        return -2;
    }
    if (memcmp(sectorbuf,"PPBL",4)!=0) {
        /* No bootloader header, abort */
        return -4;
    }

    /* Check Main firmware header */
    if (sansa_seek_and_read(sansa, sansa->start+PPMI_OFFSET, sectorbuf, 0x200) < 0) {
        fprintf(stderr,"[ERR]  Seek to 0x%08x in is_e200 failed.\n",
                       (unsigned int)(sansa->start+PPMI_OFFSET));
        return -5;
    }
    if (memcmp(sectorbuf,"PPMI",4)!=0) {
        /* No bootloader header, abort */
        return -7;
    }

    /* Check main mi4 file header */
    if (sansa_seek_and_read(sansa, sansa->start+PPMI_OFFSET+0x200, sectorbuf, 0x200) < 0) {
        fprintf(stderr,"[ERR]  Seek to 0x%08x in is_e200 failed.\n",
                       (unsigned int)(sansa->start+PPMI_OFFSET+0x200));
        return -5;
    }

    if (get_mi4header(sectorbuf,&mi4header) < 0) {
        fprintf(stderr,"[ERR]  Invalid mi4header\n");
        return -6;
    }

    if ((mi4header.mi4size < 100000) &&
        (memcmp(sectorbuf+0x1f8,"RBBL",4)!=0)) {
        sansa->hasoldbootloader = 1;
    } else {
        sansa->hasoldbootloader = 0;
    }

    return 0;
}

int sansa_scan(struct sansa_t* sansa)
{
    int i;
    int n = 0;
    char last_disk[4096];

    printf("[INFO] Scanning disk devices...\n");

    for (i = 0; i <= 25 ; i++) {
#ifdef __WIN32__
         sprintf(sansa->diskname,"\\\\.\\PhysicalDrive%d",i);
#elif defined(linux) || defined (__linux)
         sprintf(sansa->diskname,"/dev/sd%c",'a'+i);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
         sprintf(sansa->diskname,"/dev/da%d",i);
#elif defined(__APPLE__) && defined(__MACH__)
         sprintf(sansa->diskname,"/dev/disk%d",i);
#else
    #error No disk paths defined for this platform
#endif
         if (sansa_open(sansa, 1) < 0) {
             continue;
         }

         if (read_partinfo(sansa,1) < 0) {
             continue;
         }

         if (is_e200(sansa) < 0) {
             continue;
         }

#ifdef __WIN32__
         printf("[INFO] E200 found - disk device %d\n",i);
#else
         printf("[INFO] E200 found - %s\n",sansa->diskname);
#endif
         n++;
         strcpy(last_disk,sansa->diskname);
         sansa_close(sansa);
    }

    if (n==1) {
        /* Remember the disk name */
        strcpy(sansa->diskname,last_disk);
    }
    return n;
}

int load_original_firmware(struct sansa_t* sansa, unsigned char* buf, struct mi4header_t* mi4header)
{
    int ppmi_length;
    int n;
    unsigned char* tmpbuf;
    int i;
    int key_found;

    /* Read 512 bytes from PPMI_OFFSET - the PPMI header plus the mi4 header */
    if (sansa_seek_and_read(sansa, sansa->start + PPMI_OFFSET, buf, 512) < 0) {
        return -1;
    }

    /* No need to check PPMI magic - it's done during init to confirm
       this is an E200 */
    ppmi_length = le2int(buf+4);

    /* Firstly look for an original firmware after the first image */
    if (sansa_seek_and_read(sansa, sansa->start + PPMI_OFFSET + 0x200 + ppmi_length, buf, 512) < 0) {
        return -1;
    }

    if (get_mi4header(buf,mi4header)==0) {
        /* We have a valid MI4 file after a bootloader, so we use this. */
        if ((n = sansa_seek_and_read(sansa, 
                                     sansa->start + PPMI_OFFSET + 0x200 + ppmi_length,
                                     buf, mi4header->mi4size)) < 0) {
            return -1;
        }
    } else {
        /* No valid MI4 file, so read the first image. */
        if ((n = sansa_seek_and_read(sansa, 
                                     sansa->start + PPMI_OFFSET + 0x200,
                                     buf, ppmi_length)) < 0) {
            return -1;
        }
    }

    get_mi4header(buf,mi4header);

#if 0
    printf("mi4header->version   =0x%08x\n",mi4header->version);
    printf("mi4header->length    =0x%08x\n",mi4header->length);
    printf("mi4header->crc32     =0x%08x\n",mi4header->crc32);
    printf("mi4header->enctype   =0x%08x\n",mi4header->enctype);
    printf("mi4header->mi4size   =0x%08x\n",mi4header->mi4size);
    printf("mi4header->plaintext =0x%08x\n",mi4header->plaintext);
#endif

    /* Decrypt anything that needs decrypting. */
    if (mi4header->plaintext < mi4header->mi4size - 0x200) {
        /* TODO: Check different keys */
        tmpbuf=malloc(mi4header->mi4size-(mi4header->plaintext+0x200));
        if (tmpbuf==NULL) {
            fprintf(stderr,"[ERR]  Can not allocate memory\n");
            return -1;
        }

        key_found=0;
        for (i=0; i < NUM_KEYS && !key_found ; i++) {
            tea_decrypt_buf(buf+(mi4header->plaintext+0x200),
                            tmpbuf,
                            mi4header->mi4size-(mi4header->plaintext+0x200),
                            keys[i]);
            key_found = (le2int(tmpbuf+mi4header->length-mi4header->plaintext-4) == 0xaa55aa55);
        }

        if (key_found) {
            memcpy(buf+(mi4header->plaintext+0x200),tmpbuf,mi4header->mi4size-(mi4header->plaintext+0x200));
            free(tmpbuf);
        } else {
            fprintf(stderr,"[ERR]  Failed to decrypt image, aborting\n");
            free(tmpbuf);
            return -1;
        }
    }

    /* Increase plaintext value to full file */
    mi4header->plaintext = mi4header->mi4size - 0x200;

    /* Update CRC checksum */
    chksum_crc32gentab ();
    mi4header->crc32 = chksum_crc32(buf+0x200,mi4header->mi4size-0x200);

    set_mi4header(buf,mi4header);

    return 0;
}

int read_firmware(struct sansa_t* sansa, char* filename)
{
    int res;
    int outfile;
    struct mi4header_t mi4header;

    res = load_original_firmware(sansa,sectorbuf,&mi4header);
    if (res < 0)
        return res;

    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return -1;
    }

    write(outfile,sectorbuf,mi4header.mi4size);
    close(outfile);

    return 0;
}


int add_bootloader(struct sansa_t* sansa, char* filename, int type)
{
    int res;
    int infile;
    int bl_length;
    struct mi4header_t mi4header;
    int n;
    int length;

    if (type==FILETYPE_MI4) {
        /* Step 1 - read bootloader into RAM. */
        infile=open(filename,O_RDONLY|O_BINARY);
        if (infile < 0) {
            fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
            return -1;
        }

        bl_length = filesize(infile);
    } else {
        bl_length = LEN_bootimg;
    }

    /* Create PPMI header */
    memset(sectorbuf,0,0x200);
    memcpy(sectorbuf,"PPMI",4);
    int2le(bl_length,   sectorbuf+4);
    int2le(0x00020000,  sectorbuf+8);

    if (type==FILETYPE_MI4) {
        /* Read bootloader into sectorbuf+0x200 */
        n = read(infile,sectorbuf+0x200,bl_length);
        if (n < bl_length) {
            fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                          ,bl_length,n);
            return -1;
        }

        if (memcmp(sectorbuf+0x200+0x1f8,"RBBL",4)!=0) {
            fprintf(stderr,"[ERR]  Not a Rockbox bootloader, aborting.\n");
            return -1;
        }
    } else {
        memcpy(sectorbuf+0x200,bootimg,LEN_bootimg);
    }

    /* Load original firmware from Sansa to the space after the bootloader */
    res = load_original_firmware(sansa,sectorbuf+0x200+bl_length,&mi4header);
    if (res < 0)
        return res;

    /* Now write the whole thing back to the Sansa */

    if (sansa_seek(sansa, sansa->start+PPMI_OFFSET) < 0) {
        fprintf(stderr,"[ERR]  Seek to 0x%08x in add_bootloader failed.\n",
                       (unsigned int)(sansa->start+PPMI_OFFSET));
        return -5;
    }

    length = 0x200 + bl_length + mi4header.mi4size;

    n=sansa_write(sansa, sectorbuf, length);
    if (n < length) {
        fprintf(stderr,"[ERR]  Short write in add_bootloader\n");
        return -6;
    }

    return 0;
}

int delete_bootloader(struct sansa_t* sansa)
{
    int res;
    struct mi4header_t mi4header;
    int n;
    int length;

    /* Load original firmware from Sansa to sectorbuf+0x200 */
    res = load_original_firmware(sansa,sectorbuf+0x200,&mi4header);
    if (res < 0)
        return res;

    /* Create PPMI header */
    memset(sectorbuf,0,0x200);
    memcpy(sectorbuf,"PPMI",4);
    int2le(mi4header.mi4size, sectorbuf+4);
    int2le(0x00020000,        sectorbuf+8);

    /* Now write the whole thing back to the Sansa */

    if (sansa_seek(sansa, sansa->start+PPMI_OFFSET) < 0) {
        fprintf(stderr,"[ERR]  Seek to 0x%08x in add_bootloader failed.\n",
                       (unsigned int)(sansa->start+PPMI_OFFSET));
        return -5;
    }

    length = 0x200 + mi4header.mi4size;

    n=sansa_write(sansa, sectorbuf, length);
    if (n < length) {
        fprintf(stderr,"[ERR]  Short write in delete_bootloader\n");
        return -6;
    }

    return 0;
}

void list_images(struct sansa_t* sansa)
{
    struct mi4header_t mi4header;
    loff_t ppmi_length;

    /* Check Main firmware header */
    if (sansa_seek_and_read(sansa, sansa->start+PPMI_OFFSET, sectorbuf, 0x200) < 0) {
        return;
    }

    ppmi_length = le2int(sectorbuf+4);

    printf("[INFO] Image 1 - %llu bytes\n",ppmi_length);

    /* Look for an original firmware after the first image */
    if (sansa_seek_and_read(sansa, sansa->start + PPMI_OFFSET + 0x200 + ppmi_length, sectorbuf, 512) < 0) {
        return;
    }

    if (get_mi4header(sectorbuf,&mi4header)==0) {
        printf("[INFO] Image 2 - %d bytes\n",mi4header.mi4size);
    }
}
