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
#include "adpcm_seek.h"
#include "codeclib.h"

/*
 * The helper functions in order to seek for the adpcm codec
 * which does not include the header each the data block.
 */

#define MAX_STORE_COUNT 1000

static struct adpcm_data seek_table[MAX_STORE_COUNT];
static int seek_count;
static int cur_count;
static int max_ratio;
static int cur_ratio;

void init_seek_table(uint32_t max_count)
{
    int i = 0;

    for ( ; i < MAX_STORE_COUNT; i++)
    {
        seek_table[i].is_valid = false;
    }
    seek_count = max_count / MAX_STORE_COUNT + 1;
    max_ratio  = max_count / seek_count + 1;
    cur_count  = 0;
    cur_ratio  = -1;
}

void add_adpcm_data(struct adpcm_data *data)
{
    if (--cur_count <= 0)
    {
        cur_count = seek_count;
        if (++cur_ratio >= max_ratio)
            cur_ratio = max_ratio - 1;

        if (!seek_table[cur_ratio].is_valid)
        {
            seek_table[cur_ratio].pcmdata[0] = data->pcmdata[0];
            seek_table[cur_ratio].pcmdata[1] = data->pcmdata[1];
            seek_table[cur_ratio].step[0]    = data->step[0];
            seek_table[cur_ratio].step[1]    = data->step[1];
            seek_table[cur_ratio].is_valid   = true;
        }
    }
}

uint32_t seek(uint32_t count, struct adpcm_data *seek_data,
              uint8_t *(*read_buffer)(size_t *realsize),
              int (*decode)(const uint8_t *inbuf, size_t inbufsize))
{
    int new_ratio = count / seek_count;

    if (new_ratio >= max_ratio)
        new_ratio = max_ratio - 1;

    if (!seek_table[new_ratio].is_valid)
    {
        uint8_t *buffer;
        size_t n;

        do
        {
            buffer = read_buffer(&n);
            if (n == 0)
                break;
            decode(buffer, n);
        } while (cur_ratio < new_ratio);
    }

    seek_data->pcmdata[0] = seek_table[new_ratio].pcmdata[0];
    seek_data->pcmdata[1] = seek_table[new_ratio].pcmdata[1];
    seek_data->step[0]    = seek_table[new_ratio].step[0];
    seek_data->step[1]    = seek_table[new_ratio].step[1];

    cur_ratio = new_ratio;
    cur_count = seek_count;
    return cur_ratio * seek_count;
}
