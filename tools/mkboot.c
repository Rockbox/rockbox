/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "mkboot.h"

#ifndef RBUTIL
static void usage(void)
{
    printf("usage: mkboot [-h300] <firmware file> <boot file> <output file>\n");

    exit(1);
}
#endif

static unsigned char image[0x400000 + 0x220 + 0x400000/0x200];

#ifndef RBUTIL
int main(int argc, char *argv[])
{
    char *infile, *bootfile, *outfile;
    int origin = 0x1f0000;   /* H1x0 bootloader address */

    if(argc < 3) {
        usage();
    }

    if(!strcmp(argv[1], "-h300")) {
        infile = argv[2];
        bootfile = argv[3];
        outfile = argv[4];

        origin = 0x3f0000;   /* H3x0 bootloader address */
    }
    else
    {
        infile = argv[1];
        bootfile = argv[2];
        outfile = argv[3];
    }
    return mkboot(infile, bootfile, outfile, origin);
}
#endif

int mkboot(const char* infile, const char* bootfile, const char* outfile, int origin)
{
    FILE *f;
    int i;
    int len;
    int actual_length, total_length, binary_length, num_chksums;

    memset(image, 0xff, sizeof(image));

    /* First, read the iriver original firmware into the image */
    f = fopen(infile, "rb");
    if(!f) {
        perror(infile);
        return -1;
    }

    i = fread(image, 1, 16, f);
    if(i < 16) {
        perror(infile);
        fclose(f);
        return -2;
    }

    /* This is the length of the binary image without the scrambling
       overhead (but including the ESTFBINR header) */
    binary_length = image[4] + (image[5] << 8) +
        (image[6] << 16) + (image[7] << 24);

    /* Read the rest of the binary data, but not the checksum block */
    len = binary_length+0x200-16;
    i = fread(image+16, 1, len, f);
    if(i < len) {
        perror(infile);
        fclose(f);
        return -3;
    }
    
    fclose(f);

    /* Now, read the boot loader into the image */
    f = fopen(bootfile, "rb");
    if(!f) {
        perror(bootfile);
        fclose(f);
        return -4;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);

    fseek(f, 0, SEEK_SET);

    i = fread(image+0x220 + origin, 1, len, f);
    if(i < len) {
        perror(bootfile);
        fclose(f);
        return -5;
    }

    fclose(f);

    f = fopen(outfile, "wb");
    if(!f) {
        perror(outfile);
        return -6;
    }

    /* Patch the reset vector to start the boot loader */
    image[0x220 + 4] = image[origin + 0x220 + 4];
    image[0x220 + 5] = image[origin + 0x220 + 5];
    image[0x220 + 6] = image[origin + 0x220 + 6];
    image[0x220 + 7] = image[origin + 0x220 + 7];

    /* This is the actual length of the binary, excluding all headers */
    actual_length = origin + len;

    /* Patch the ESTFBINR header */
    image[0x20c] = (actual_length >> 24) & 0xff;
    image[0x20d] = (actual_length >> 16) & 0xff;
    image[0x20e] = (actual_length >> 8) & 0xff;
    image[0x20f] = actual_length & 0xff;
    
    image[0x21c] = (actual_length >> 24) & 0xff;
    image[0x21d] = (actual_length >> 16) & 0xff;
    image[0x21e] = (actual_length >> 8) & 0xff;
    image[0x21f] = actual_length & 0xff;

    /* This is the length of the binary, including the ESTFBINR header and
       rounded up to the nearest 0x200 boundary */
    binary_length = (actual_length + 0x20 + 0x1ff) & 0xfffffe00;

    /* The number of checksums, i.e number of 0x200 byte blocks */
    num_chksums = binary_length / 0x200;

    /* The total file length, including all headers and checksums */
    total_length = binary_length + num_chksums + 0x200;

    /* Patch the scrambler header with the new length info */
    image[0] = total_length & 0xff;
    image[1] = (total_length >> 8) & 0xff;
    image[2] = (total_length >> 16) & 0xff;
    image[3] = (total_length >> 24) & 0xff;
    
    image[4] = binary_length & 0xff;
    image[5] = (binary_length >> 8) & 0xff;
    image[6] = (binary_length >> 16) & 0xff;
    image[7] = (binary_length >> 24) & 0xff;
    
    image[8] = num_chksums & 0xff;
    image[9] = (num_chksums >> 8) & 0xff;
    image[10] = (num_chksums >> 16) & 0xff;
    image[11] = (num_chksums >> 24) & 0xff;
    
    i = fwrite(image, 1, total_length, f);
    if(i < total_length) {
        perror(outfile);
        fclose(f);
        return -7;
    }

    printf("Wrote 0x%x bytes in %s\n", total_length, outfile);
    
    fclose(f);
    
    return 0;
}
