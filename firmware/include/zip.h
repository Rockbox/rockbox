/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by James Buren
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

#ifndef _ZIP_H_
#define _ZIP_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

enum {
    ZIP_PASS_SHALLOW,
    ZIP_PASS_START,
    ZIP_PASS_DATA,
    ZIP_PASS_END,
};

typedef struct zip zip;

typedef struct {
    uint16_t entry;
    uint16_t entries;
    char* name;
    uint32_t file_size;
    time_t modts;
    void* block;
    uint32_t block_size;
    uint32_t read_size;
} zip_args;

typedef int (*zip_cb) (const zip_args* args, int pass, void* ctx);

zip* zip_open(const char* name, bool try_mem);
int zip_read_shallow(zip* z, zip_cb cb, void* ctx);
int zip_read_deep(zip* z, zip_cb cb, void* ctx);
void zip_close(zip* z);

#endif
