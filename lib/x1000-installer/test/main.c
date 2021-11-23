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

#include "xf_nandio.h"
#include "xf_error.h"
#include "xf_update.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct xf_nandio nio;

static struct xf_map testmap[] = {
    {
        .name = "src/xf_error.c",
        .offset = 0x00000000,
        .length = 0x00004000,
    },
    {
        .name = "src/xf_update.c",
        .offset = 0x00010000,
        .length = 0x00004000,
    },
    {
        .name = "src/xf_package.c",
        .offset = 0x00020000,
        .length = 0x00001800,
    },
};

void checkrc(int rc)
{
    if(rc == XF_E_SUCCESS)
        return;

    if(rc == XF_E_NAND) {
        printf("NAND error: %d\n", nio.nand_err);
    } else {
        printf("error: %s\n", xf_strerror(rc));
        printf("  CurBlock  = %lu\n", (unsigned long)nio.cur_block);
        printf("  CurOffset = %lu\n", (unsigned long)nio.offset_in_block);
    }

    exit(1);
}

int openstream_file(void* arg, const char* file, struct xf_stream* stream)
{
    (void)arg;
    return xf_open_file(file, O_RDONLY, stream);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    nand_trace_reset(65535);

    int rc = xf_nandio_init(&nio);
    checkrc(rc);

    rc = xf_updater_run(XF_UPDATE, &nio,
                        testmap, sizeof(testmap)/sizeof(struct xf_map),
                        openstream_file, NULL);
    checkrc(rc);

    for(size_t i = 0; i < nand_trace_length; ++i) {
        const char* types[] = {"READ", "PROGRAM", "ERASE"};
        const char* excep[] = {"NONE", "DOUBLE_PROGRAMMED", "CLEARED"};
        printf("%s %s %lu\n",
               types[nand_trace[i].type],
               excep[nand_trace[i].exception],
               (unsigned long)nand_trace[i].addr);
    }

    xf_nandio_destroy(&nio);
    return 0;
}
