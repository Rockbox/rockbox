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

static int write_le32(FILE* outf, uint32_t x)
{
    uint8_t p[4];
    to_le32(p, x);
    return fwrite(p, 4, 1, outf);
}

int write_blob(FILE* outf, const char* type, uint32_t flags,
               const uint8_t* data, size_t length)
{
    size_t wsize = length;
    const uint8_t* wdata = data;
    uint8_t* ucl_data = NULL;

    /* handle UCL compressed blobs */
    if(flags & BLOBFLAG_UCL) {
        ucl_data = uclpack(data, length, &wsize);
        if(!ucl_data)
            return 1;

        wdata = ucl_data;
    }

    /* calculate data CRC */
    uint32_t crc32 = crc_32(wdata, wsize, 0xffffffff);

    /* calculate padding amount */
    static const char padding[4] = {0,0,0,0};
    size_t paddingsize = 4*((wsize+3)/4) - wsize;

    /* write the blob contents */
    int err = 0;
    if(fwrite(type, 4, 1, outf) != 1)
        err = 1;
    else if(write_le32(outf, flags) != 1)
        err = 1;
    else if(write_le32(outf, wsize) != 1)
        err = 1;
    else if(write_le32(outf, crc32) != 1)
        err = 1;
    else if(fwrite(wdata, wsize, 1, outf) != 1)
        err = 1;
    else if(paddingsize != 0 && fwrite(padding, paddingsize, 1, outf) != 1)
        err = 1;

    if(ucl_data)
        free(ucl_data);

    return err;
}

int write_blobfile(FILE* outf, const char* type, uint32_t flags,
                   const char* filename)
{
    size_t size;
    uint8_t* data = loadfile(filename, &size);
    if(!data)
        return 1;

    int err = write_blob(outf, type, flags, data, size);
    free(data);
    return err;
}

int write_blobheader(FILE* outf, const char* model,
                     const char* rbversion, int num_blobs)
{
    /* format the version string */
    char verstr[VERSTR_LENGTH + 1];
    memset(verstr, 0, sizeof(verstr));
    int n = snprintf(verstr, sizeof(verstr), "%s", rbversion);

    /* write the header */
    int err = 0;
    if(n < 0 || n > VERSTR_LENGTH) {
        fprintf(stderr, "Version string too long (%d chars, max %d)\n", n, VERSTR_LENGTH);
        err = 1;
    } else if(fwrite(JZBOOT_MAGIC, 4, 1, outf) != 1)
        err = 1;
    else if(fwrite(model, 4, 1, outf) != 1)
        err = 1;
    else if(fwrite(verstr, VERSTR_LENGTH, 1, outf) != 1)
        err = 1;
    else if(write_le32(outf, num_blobs) != 1)
        err = 1;

    return err;
}
