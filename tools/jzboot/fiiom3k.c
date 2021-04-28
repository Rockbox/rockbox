/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "mkjzboot.h"

static FILE* outf = NULL;
static const char* outfname = NULL;

static uint8_t* spl_code = NULL;
static size_t spl_codesz;

static uint8_t* spl_buf = NULL;
static size_t spl_bufsz;

static uint8_t* boot_buf = NULL;
static size_t boot_bufsz;

static int cleanup(int arg)
{
    if(outf) {
        fclose(outf);
        remove(outfname);
    }

    if(spl_code)
        free(spl_code);

    if(spl_buf)
        free(spl_buf);

    if(boot_buf)
        free(boot_buf);

    return arg;
}

int mkboot_fiiom3k(int argc, char** argv)
{
    if(argc != 5) {
        fprintf(stderr, "Usage: mkjzboot fiiom3k <version> <spl.bin> <bootloader.bin> <bootloader.m3k>\n");
        return cleanup(1);
    }

    const char* rbversion = argv[1];
    const char* spl_bin = argv[2];
    const char* boot_bin = argv[3];
    const char* boot_m3k = argv[4];

    spl_code = loadfile(spl_bin, &spl_codesz);
    if(!spl_code)
        return cleanup(1);

    boot_buf = loadfile(boot_bin, &boot_bufsz);
    if(!boot_buf)
        return cleanup(1);

    spl_buf = mkspl_flash(spl_code, spl_codesz,
                          FLASHTYPE_NAND, 2, 2, &spl_bufsz);
    if(!spl_buf)
        return cleanup(1);

    outfname = boot_m3k;
    outf = fopen(boot_m3k, "wb");
    if(!outf) {
        fprintf(stderr, "Error opening output file: %s", boot_m3k);
        return cleanup(1);
    }

    int err = 0;
    if(write_blobheader(outf, FIIOM3K_MAGIC, rbversion, 2))
        err = 1;
    else if(write_blob(outf, BLOBTYPE_SPL, BLOBFLAG_NONE, spl_buf, spl_bufsz))
        err = 2;
    else if(write_blob(outf, BLOBTYPE_BOOT, BLOBFLAG_UCL, boot_buf, boot_bufsz))
        err = 3;

    if(err) {
        fprintf(stderr, "Error writing output (stage %d)\n", err);
    } else {
        /* Success */
        fclose(outf);
        outf = NULL;
    }

    return cleanup(err);
}
