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
 * Based on mkboot, Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include "mktccboot.h"
#include "telechips.h"

static void usage(void)
{
    printf("Usage: mktccboot <firmware file> <boot file> <output file>\n");

    exit(1);
}

int main(int argc, char *argv[])
{
    char *infile, *bootfile, *outfile;
    int fdout = -1;
    int n, of_size, boot_size, patched_size;
    unsigned char *of_buf;
    unsigned char *boot_buf = NULL;
    unsigned char* image = NULL;
    int ret = 0;

    if(argc < 3) {
        usage();
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    /* Read OF and boot files */
    of_buf = file_read(infile, &of_size);
    if (!of_buf)
    {
        ret = 1;
        goto error_exit;
    }

    /* Validate input file */
    if (test_firmware_tcc(of_buf, of_size))
    {
        printf("[ERR] Unknown OF file used, aborting\n");
        ret = 2;
        goto error_exit;
    }

    boot_buf = file_read(bootfile, &boot_size);
    if (!boot_buf)
    {
        ret = 3;
        goto error_exit;
    }

    /* Allocate buffer for patched firmware */
    image = malloc(of_size + boot_size);
    if (image == NULL)
    {
        printf("[ERR] Could not allocate memory, aborting\n");
        ret = 4;
        goto error_exit;
    }

    /* Create the patched firmware */
    image = patch_firmware_tcc(of_buf, of_size, boot_buf, boot_size,
            &patched_size);
    if (!image)
    {
        printf("[ERR] Error creating patched firmware, aborting\n");
        ret = 5;
        goto error_exit;
    }

    fdout = open(outfile, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
    if (fdout < 0)
    {
        perror(outfile);
        ret = 6;
        goto error_exit;
    }

    n = write(fdout, image, patched_size);
    if (n != patched_size)
    {
        printf("[ERR] Could not write output file %s\n",outfile);
        ret = 7;
        goto error_exit;
    }

error_exit:

    if (fdout >= 0)
        close(fdout);

    if (of_buf)
        free(of_buf);

    if (boot_buf)
        free(boot_buf);

    if (image)
        free(image);

    return ret;
}

