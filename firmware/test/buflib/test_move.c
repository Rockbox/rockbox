/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2011 Thomas Martitz
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
#include "core_alloc.h"
#include "util.h"

static int move_size;
int move_callback(int handle, void* old, void* new)
{
    move_size = (char*)old-(char*)new;
    printf("Move! %s, %p, %p, %zu\n", core_get_name(handle), old, new,
        move_size);

    return BUFLIB_CB_OK;
}

struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

int main(void)
{
    UT_core_allocator_init();

    int first = core_alloc("first", 20<<10);
    int second= core_alloc_ex("second", 20<<10, &ops);
    strcpy(core_get_data(second), "Here's data");

    core_free(first);
    /* should not trigger compaction, but replace the just freed one */
    int third = core_alloc("third", 20<<10);
    core_free(third);
    /* should trigger compaction since it's a bit bigger than the just freed one */
    int fourth = core_alloc("fourth", 21<<10);

    int ret = !(!strcmp(core_get_data(second), "Here's data") && move_size >= 20<<10);

    core_print_blocks(&print_simple);

    core_free(second);
    core_free(third);

    return ret;
}
