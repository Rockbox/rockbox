/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * Based on merge0.cpp by James Espinoza, but completely rewritten.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#include "pstdint.h"
#else
#include <unistd.h>
#include <inttypes.h>
#endif

#include "mknkboot.h"

/* New entry point for nk.bin - where our dualboot code is inserted */
#define NK_ENTRY_POINT 0x88200000

/* Entry point (and load address) for the main Rockbox bootloader */
#define BL_ENTRY_POINT 0x8a000000

/*

Description of nk.bin from 

http://www.xs4all.nl/~itsme/projects/xda/wince-flashfile-formats.html

these files contain most information, several non-contigouos blocks
may be present and an entrypoint in the code.

   1. a 7 character signature "B000FF\n" ( that is with 3 zeroes, and
      ending in a linefeed )
   2. DWORD for image start
   3. DWORD for image length
   4. followd by several records of this format:
         1. DWORD with address where this block is to be flashed to
         2. DWORD with the length of this block
         3. DWORD with the 32 bit checksum of this block, in perl:
            unpack("%32C*", $data);
         4. followed by <length> bytes of data 
   5. the last record has address ZERO, in the length the entrypoint
      into the rom, and ZERO as checksum.


NOTE: The Gigabeat-S nk.bin contains 171 records, plus the EOF record.

mknkboot.c appends two images:

1) A "Disable" image which overwrites a word in the EBoot image
2) Our bootloader image, which has the same load address as nk.exe

*/

/* win32 compatibility */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define DISABLE_ADDR    0x88065A10 /* in EBoot */
#define DISABLE_INSN    0xe3a00001
#define DISABLE_SUM     (0xe3+0xa0+0x00+0x01)  

/* Code to dual-boot - this is inserted at NK_ENTRY_POINT */
static uint32_t dualboot[] = 
{
    0xe59f900c,      /* ldr     r9, [pc, #12] -> 0x53fa4000 */
    0xe5999000,      /* ldr     r9, [r9] */
    0xe3190010,      /* tst     r9, #16 ; 0x10 */
#if 0
    /* Branch to Rockbox if hold is on */
    0x159ff004,      /* ldrne   pc, [pc, #4]  -> 0x89000000 */
#else
    /* Branch to Rockbox if hold is off */
    0x059ff004,      /* ldreq   pc, [pc, #4]  -> 0x89000000 */
#endif
    /* Branch to original firmware */
    0xea0003fa,      /* b 0x1000 */

    0x53fa4000,      /* GPIO3_DR */
    BL_ENTRY_POINT   /* RB bootloader load address/entry point */
};


static void put_uint32le(uint32_t x, unsigned char* p)
{
    p[0] = (unsigned char)(x & 0xff);
    p[1] = (unsigned char)((x >> 8) & 0xff);
    p[2] = (unsigned char)((x >> 16) & 0xff);
    p[3] = (unsigned char)((x >> 24) & 0xff);
}

#if !defined(BEASTPATCHER)
static off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}
#endif


int mknkboot(const struct filebuf *indata, const struct filebuf *bootdata,
             struct filebuf *outdata)
{
    int i;
    unsigned char* boot;
    unsigned char* boot2;
    unsigned char* disable;
    uint32_t sum;

    /* Create buffer for original nk.bin, plus our bootloader (with 12
       byte header), plus the 16-byte "disable record", plus our dual-boot code */
    outdata->len = indata->len + (bootdata->len + 12) + 16 + (12 + 28);
    outdata->buf = malloc(outdata->len);

    if (outdata->buf==NULL)
    {
        printf("[ERR]  Could not allocate memory, aborting\n");
        return -1;
    }

    /****** STEP 1 - Read original nk.bin into buffer */
    memcpy(outdata->buf, indata->buf, indata->len);

    /****** STEP 2 - Move EOF record to the new EOF */
    memcpy(outdata->buf + outdata->len - 12, outdata->buf + indata->len - 12, 12);

    /* Overwrite default entry point with NK_ENTRY_POINT */
    put_uint32le(NK_ENTRY_POINT, outdata->buf + outdata->len - 8);

    /****** STEP 3 - Create a record to disable the firmware signature
                check in EBoot */
    disable = outdata->buf + indata->len - 12;

    put_uint32le(DISABLE_ADDR, disable);
    put_uint32le(4,            disable + 4);
    put_uint32le(DISABLE_SUM,  disable + 8);
    put_uint32le(DISABLE_INSN, disable + 12);

    /****** STEP 4 - Append the bootloader binary */
    boot = disable + 16;
    memcpy(boot + 12, bootdata->buf, bootdata->len);

    /****** STEP 5 - Create header for bootloader record */

    /* Calculate checksum */
    sum = 0;
    for (i = 0; i < bootdata->len; i++) {
        sum += boot[12 + i];
    }
    
    put_uint32le(BL_ENTRY_POINT, boot); /* Our entry point */
    put_uint32le(bootdata->len, boot + 4);
    put_uint32le(sum, boot + 8);

    /****** STEP 6 -  Insert our dual-boot code */
    boot2 = boot + bootdata->len + 12;

    /* Copy dual-boot code in an endian-safe way */
    for (i = 0; i < (signed int)sizeof(dualboot) / 4; i++) {
        put_uint32le(dualboot[i], boot2 + 12 + i*4);
    }

    /* Calculate checksum */
    sum = 0;
    for (i = 0; i < (signed int)sizeof(dualboot); i++) {
        sum += boot2[i+12];
    }
    
    put_uint32le(NK_ENTRY_POINT, boot2); /* New entry point for our nk.bin */
    put_uint32le(sizeof(dualboot), boot2 + 4);
    put_uint32le(sum, boot2 + 8);

    return 0;
}

#if !defined(BEASTPATCHER)
static void usage(void)
{
    printf("Usage: mknkboot <firmware file> <boot file> <output file>\n");

    exit(1);
}


int main(int argc, char* argv[])
{
    char *infile, *bootfile, *outfile;
    int fdin = -1, fdboot = -1, fdout = -1;
    int n;
    struct filebuf indata = {0, NULL}, bootdata = {0, NULL}, outdata = {0, NULL};
    int result = 0;

    if(argc < 4) {
        usage();
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    fdin = open(infile, O_RDONLY|O_BINARY);
    if (fdin < 0)
    {
        perror(infile);
        result = 2;
        goto quit;
    }

    fdboot = open(bootfile, O_RDONLY|O_BINARY);
    if (fdboot < 0)
    {
        perror(bootfile);
        close(fdin);
        result = 3;
        goto quit;
    }

    indata.len = filesize(fdin);
    bootdata.len = filesize(fdboot);
    indata.buf = (unsigned char*)malloc(indata.len);
    bootdata.buf = (unsigned char*)malloc(bootdata.len);
    if(indata.buf == NULL || bootdata.buf == NULL)
    {
        printf("[ERR]  Could not allocate memory, aborting\n");
        result = 4;
        goto quit;
    }
    n = read(fdin, indata.buf, indata.len);
    if (n != indata.len)
    {
        printf("[ERR]  Could not read from %s\n",infile);
        result = 5;
        goto quit;
    }
    n = read(fdboot, bootdata.buf, bootdata.len);
    if (n != bootdata.len)
    {
        printf("[ERR]  Could not read from %s\n",bootfile);
        result = 6;
        goto quit;
    }

    result = mknkboot(&indata, &bootdata, &outdata);
    if(result != 0)
    {
        goto quit;
    }
    fdout = open(outfile, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
    if (fdout < 0)
    {
        perror(outfile);
        result = 7;
        goto quit;
    }

    n = write(fdout, outdata.buf, outdata.len);
    if (n != outdata.len)
    {
        printf("[ERR] Could not write output file %s\n",outfile);
        result = 8;
        goto quit;
    }

quit:
    free(bootdata.buf);
    free(indata.buf);
    free(outdata.buf);
    close(fdin);
    close(fdboot);
    close(fdout);

    return result;

}
#endif

