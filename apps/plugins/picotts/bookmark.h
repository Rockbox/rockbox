/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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
#include "plugin.h"

#ifndef PLUGIN_PICOTTS_BOOKMARK_H
#define PLUGIN_PICOTTS_BOOKMARK_H

#define TXTCHUNK_NUM 4096
#define PICOTTS_BOOKMARK_MAGIC 0x42545453
#define BMK_EXTENSION ".bmk"

struct bookmark_t {
    uint32_t magic;
    uint32_t filecrc;
    uint32_t bookmark;
    uint32_t txtchunkmap[TXTCHUNK_NUM];
};

void txtchunkmap_add(uint32_t txtchunk);
uint32_t txtchunk_prev(uint32_t txtchunk);
uint32_t txtchunk_next(uint32_t txtchunk);
void dump_txtchunkmap(void);

void bookmark_init(uint32_t crc);
void save_bookmark(char *filename, uint32_t txtchunk);
uint32_t load_bookmark(char *filename);
#endif
