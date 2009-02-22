/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Copyright (c) 2009, Dave Chapman
 * All rights reserved.
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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mtp_common.h"
#include "bootimg.h"

#define VERSION "1.0 with v1 bootloader"

void print_usage(void)
{
    fprintf(stderr,"Usage: beastpatcher [action]\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of the following options:\n");
    fprintf(stderr,"        --install (default)\n");
    fprintf(stderr,"  -?,   --help\n");
    fprintf(stderr,"\n");
}

/* Code to create a single-boot bootloader.
   Based on tools/gigabeats.c by Will Robertson.  
*/

/* Entry point (and load address) for the main Rockbox bootloader */
#define BL_ENTRY_POINT 0x8a000000

static void put_uint32le(uint32_t x, unsigned char* p)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

static uint32_t calc_csum(const unsigned char* pb, int cb)
{
    uint32_t l = 0;
    while (cb--)
        l += *pb++;
    return l;
}

static void create_single_boot(unsigned char* boot, int bootlen,
                               unsigned char** fwbuf, int* fwsize)
{
    unsigned char* buf;

    /* 15 bytes for header, 16 for signature bypass, 
     * 12 for record header, size of bootloader, 12 for footer */
    *fwsize = 15 + 16 + 12 + bootlen + 12;
    *fwbuf = malloc(*fwsize);

    if(buf == NULL) {
        fprintf(stderr, "[ERR]  Cannot allocate memory.\n" );
        *fwbuf = NULL;
        *fwsize = 0;
        return;
    }

    buf = *fwbuf;

    /* Copy bootloader image. */
    memcpy(buf + 43, boot, bootlen);

    /* Step 2: Create the file header */
    sprintf((char *)buf, "B000FF\n");
    put_uint32le(0x88200000, buf+7);

    /* If the value below is too small, the update will attempt to flash.
     * Be careful when changing this (leaving it as is won't cause issues) */
    put_uint32le(0xCC0CD8, buf+11); 

    /* Step 3: Add the signature bypass record */
    put_uint32le(0x88065A10, buf+15);
    put_uint32le(4, buf+19);
    put_uint32le(0xE3A00001, buf+27);
    put_uint32le(calc_csum(buf+27,4), buf+23);

    /* Step 4: Create a record for the actual code */
    put_uint32le(BL_ENTRY_POINT, buf+31);
    put_uint32le(bootlen, buf+35);
    put_uint32le(calc_csum(buf + 43, bootlen), buf+39);

    /* Step 5: Write the footer */
    put_uint32le(0, buf+*fwsize-12);
    put_uint32le(BL_ENTRY_POINT, buf+*fwsize-8);
    put_uint32le(0, buf+*fwsize-4);

    return;
}

int beastpatcher(int argc, char* argv[])
{
    char yesno[4];
    unsigned char* fwbuf;
    int fwsize;
    struct mtp_info_t mtp_info;

    (void)argv;

    fprintf(stderr,"beastpatcher v" VERSION " - (C) 2009 by the Rockbox developers\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    /* No options are currently implemented, so just display help if any are 
       provided. */

    if (argc > 1) {
        print_usage();
        return 1;
    }

    if (mtp_init(&mtp_info) < 0) {
        fprintf(stderr,"[ERR]  Can not init MTP\n");
        return 1;
    }

    /* Scan for attached MTP devices. */
    if (mtp_scan(&mtp_info) < 0)
    {
        fprintf(stderr,"[ERR]  No devices found\n");
        return 1;
    }

    printf("[INFO] Found device \"%s - %s\"\n", mtp_info.manufacturer, 
                                                mtp_info.modelname);
    printf("[INFO] Device version: \"%s\"\n",mtp_info.version);


    printf("\nEnter i to install the Rockbox bootloader or c to cancel and do nothing (i/c): ");

    if (fgets(yesno,4,stdin))
    {
        if (yesno[0]=='i')
        {
            /* Create a single-boot bootloader from the embedded bootloader */
            create_single_boot(bootimg, LEN_bootimg, &fwbuf, &fwsize);

            if (fwbuf == NULL)
                return 1;
            
            if (mtp_send_firmware(&mtp_info, fwbuf, fwsize) == 0)
            {
                fprintf(stderr,"[INFO] Bootloader installed successfully.\n");
            } 
            else
            {
                fprintf(stderr,"[ERR]  Bootloader install failed.\n");
            }

            /* We are now done with the firmware image */
            free(fwbuf);
        }
        else
        {
            fprintf(stderr,"[INFO] Installation cancelled.\n");
        }
    }

    mtp_finished(&mtp_info);

    return 0;
}


int main(int argc, char* argv[])
{
    int res;
    char yesno[4];

    res = beastpatcher(argc, argv);

    printf("\nPress ENTER to exit beastpatcher: ");
    fgets(yesno,4,stdin);

    return res;
}
