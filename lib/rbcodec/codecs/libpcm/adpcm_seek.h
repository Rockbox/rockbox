/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#ifndef CODEC_LIBPCM_ADPCM_SEEK_H
#define CODEC_LIBPCM_ADPCM_SEEK_H

#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

struct adpcm_data {
    int16_t  pcmdata[2];
    uint16_t step[2];
    bool     is_valid;
};

void init_seek_table(uint32_t max_count);
void add_adpcm_data(struct adpcm_data *data);
uint32_t seek(uint32_t seek_time, struct adpcm_data *seek_data,
              uint8_t *(*read_buffer)(size_t *realsize),
              int (*decode)(const uint8_t *inbuf, size_t inbufsize));
#endif
