/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mkamsboot - a tool for merging bootloader code into an Sansa V2
 *             (AMS) firmware file
 *
 * Copyright (C) 2008 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <ucl/ucl.h>

#include "mkamsboot.h"

/* Header for ARM code binaries */
#include "dualboot.h"

/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* standalone executable */
int main(int argc, char* argv[])
{
    char *infile, *bootfile, *outfile;
    int fdout;
    off_t len;
    uint32_t n;
    unsigned char* buf;
    int firmware_size;
    int bootloader_size;
    unsigned char* of_packed;
    int of_packedsize;
    unsigned char* rb_packed;
    int rb_packedsize;
    int patchable;
    int totalsize;
    char errstr[200];
    struct md5sums sum;
    char md5sum[33]; /* 32 digits + \0 */

    sum.md5 = md5sum;

/* VERSION comes frome the Makefile */
    fprintf(stderr,
"mkamsboot Version " VERSION "\n"
"This is free software; see the source for copying conditions.  There is NO\n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"\n");

    if(argc != 4) {
        printf("Usage: mkamsboot <firmware file> <boot file> <output file>\n");
        return 1;
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    /* Load original firmware file */
    buf = load_of_file(infile, &len, &sum,
            &firmware_size, &of_packed, &of_packedsize, errstr, sizeof(errstr));

    if (buf == NULL) {
        fprintf(stderr, "%s", errstr);
        fprintf(stderr, "[ERR]  Could not load %s\n", infile);
        return 1;
    }

    fprintf(stderr, "[INFO] Original firmware MD5 checksum match\n");
    fprintf(stderr, "[INFO] Model: Sansa %s v%d - Firmware version: %s\n",
            model_names[sum.model], hw_revisions[sum.model], sum.version);


    /* Load bootloader file */
    rb_packed = load_rockbox_file(bootfile, sum.model, &bootloader_size,
            &rb_packedsize, errstr, sizeof(errstr));
    if (rb_packed == NULL) {
        fprintf(stderr, "%s", errstr);
        fprintf(stderr, "[ERR]  Could not load %s\n", bootfile);
        free(buf);
        free(of_packed);
        return 1;
    }

    printf("[INFO] Firmware patching has begun !\n\n");

    fprintf(stderr, "[INFO] Original firmware size:   %d bytes\n",
            firmware_size);
    fprintf(stderr, "[INFO] Packed OF size:           %d bytes\n",
            of_packedsize);
    fprintf(stderr, "[INFO] Bootloader size:          %d bytes\n",
            (int)bootloader_size);
    fprintf(stderr, "[INFO] Packed bootloader size:   %d bytes\n",
            rb_packedsize);
    fprintf(stderr, "[INFO] Dual-boot function size:  %d bytes\n",
            bootloader_sizes[sum.model]);
    fprintf(stderr, "[INFO] UCL unpack function size: %u bytes\n",
            (unsigned int)sizeof(nrv2e_d8));

    patchable = check_sizes(sum.model, rb_packedsize, bootloader_size,
        of_packedsize, firmware_size, &totalsize, errstr, sizeof(errstr));

    fprintf(stderr, "[INFO] Total size of new image:  %d bytes\n", totalsize);

    if (!patchable) {
        fprintf(stderr, "%s", errstr);
        free(buf);
        free(of_packed);
        free(rb_packed);
        return 1;
    }

    patch_firmware(sum.model, fw_revisions[sum.model], firmware_size, buf, len,
            of_packed, of_packedsize, rb_packed, rb_packedsize);

    /* Write the new firmware */
    fdout = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0666);

    if (fdout < 0) {
        fprintf(stderr, "[ERR]  Could not open %s for writing\n", outfile);
        free(buf);
        free(of_packed);
        free(rb_packed);
        return 1;
    }

    n = write(fdout, buf, len);

    if (n != (unsigned)len) {
        fprintf(stderr, "[ERR]  Could not write firmware file\n");
        free(buf);
        free(of_packed);
        free(rb_packed);
        return 1;
    }

    close(fdout);
    free(buf);
    free(of_packed);
    free(rb_packed);
    fprintf(stderr, "\n[INFO] Patching succeeded!\n");

    return 0;
}

