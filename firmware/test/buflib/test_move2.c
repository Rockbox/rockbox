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
/*
 * Expected output (64bit)
--------------------
after freeing first: available: 10040
after freeing forth: available: 10040
buflib_compact(): Compacting!
move_block(): moving "third"(id=3) by -3210(-25680)
Move! third, 0x608758, 0x602308, 25680
Cannot move now for handle 3
fifth failed. Retrying...
buflib_compact(): Compacting!
move_block(): moving "third"(id=3) by -3210(-25680)
Move! third, 0x608758, 0x602308, 25680
fifth handle: 1
fifth(1):	0x608730
   	0x608758
   	21544
second(2):	0x607308
   	0x607330
   	5160
third(3):	0x6022e0
   	0x602308
   	15400
sixth(4):	0x605f08
   	0x605f30
   	2088
seventh(5):	0x606730
   	0x606758
   	552
0x6022e0: val: 1925 (third)
0x605f08: val:  261 (sixth)
0x606730: val:   69 (seventh)
0x606958: val: -310 (<unallocated>)
0x607308: val:  645 (second)
0x608730: val: 2693 (fifth)
--------------------
*/

static int move_size, retry;
int move_callback(int handle, void* old, void* new)
{
    move_size = (char*)old-(char*)new;
    printf("Move! %s, %p, %p, %d\n", core_get_name(handle), old, new,
        move_size);

    if (!retry)
    {
        retry = 1;
        printf("Cannot move now for handle %d\n", handle);
        return BUFLIB_CB_CANNOT_MOVE;
    }
    return BUFLIB_CB_OK;
}

struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

static struct buflib_callbacks ops_no_move = {
    .move_callback = NULL,
    .shrink_callback = NULL,
};

int main(void)
{
    UT_core_allocator_init();

    int first = core_alloc("first", 20<<10);
    int second= core_alloc_ex("second", 5<<10, &ops_no_move);
    int third = core_alloc_ex("third", 15<<10, &ops);
    strcpy(core_get_data(second), "Here's data");

    core_free(first);
    printf("after freeing first: available: %zu\n", core_available());
    /* should not trigger compaction, but replace the just freed one */
    int fourth = core_alloc("forth", 20<<10);
    core_free(fourth);
    printf("after freeing forth: available: %zu\n", core_available());
    /* should trigger compaction since it's a bit bigger than the just freed one */
    int fifth = core_alloc("fifth", 21<<10);
    if (fifth <= 0)
    {
        printf("fifth failed. Retrying...\n");
        fifth = core_alloc("fifth", 21<<10);
    }
    if (fifth <= 0)
    {
        printf("fifth still failed\n");
    }

    printf("fifth handle: %d\n", fifth);
    int sixth = core_alloc("sixth", 2<<10);
    int seventh = core_alloc("seventh", 512);


    core_print_allocs(&print_simple);
    core_print_blocks(&print_simple);
    int ret = !(!strcmp(core_get_data(second), "Here's data") && move_size >= 20<<10);

    core_free(second);
    core_free(third);
    if (fifth > 0)
        core_free(fifth);
    core_free(sixth);
    core_free(seventh);

    return ret;
}
