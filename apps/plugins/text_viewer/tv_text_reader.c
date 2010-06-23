/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "tv_pager.h"
#include "tv_reader.h"
#include "tv_text_processor.h"
#include "tv_text_reader.h"

static int get_block;
static bool get_double_blocks;

bool tv_init_text_reader(unsigned char **buf, size_t *size)
{
    return tv_init_text_processor(buf, size) && tv_init_pager(buf, size);
}

void tv_finalize_text_reader(void)
{
    tv_finalize_pager();
}

void tv_set_read_conditions(int blocks, int width)
{
    tv_set_creation_conditions(blocks, width);
}

off_t tv_get_total_text_size(void)
{
    return tv_get_file_size();
}

bool tv_get_next_line(const unsigned char **buf)
{
    ssize_t bufsize;
    const unsigned char *buffer = tv_get_buffer(&bufsize);

    if (bufsize > 0)
    {
        tv_move_next_line(
             tv_create_formed_text(buffer, bufsize, get_block, get_double_blocks, buf));
        return true;
    }
    return false;
}

void tv_read_start(int block, bool is_multi)
{
    get_block = block;
    get_double_blocks = is_multi;
    tv_reset_line_positions();
}

int tv_read_end(void)
{
    return tv_get_line_positions(0);
}

void tv_seek_top(void)
{
   tv_move_screen(0, 0, SEEK_SET);
}
