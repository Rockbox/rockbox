/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2012 Marcin Bukat
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

struct gif_decoder {
    unsigned char *mem;
    size_t mem_size;
    int width;
    int height;
    int frames_count;
    int delay;
    size_t native_img_size;
    int error;
};

void gif_decoder_init(struct gif_decoder *decoder, void *mem, size_t size);
void gif_decoder_destroy_memory_pool(struct gif_decoder *d);
void gif_open(char *filename, struct gif_decoder *d);
void gif_decode(struct gif_decoder *d, void (*pf_progress)(int current, int total));

