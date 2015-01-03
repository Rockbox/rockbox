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
#include <stdlib.h>
#include "core_alloc.h"
#include "util.h"

/*
 * Expected output (64-bit):
-------------------
MOVED!
0x6032e0: val: 1285 (first)
0x605b08: val: 1285 (third)
0x608330: val: 1413 (fourth)
0x60af58: val: 2181 (fifth)
SHRINK! 517
MOVED!
0x6032e0: val: 1285 (first)
0x605b08: val:  645 (third)
0x606f30: val: 1413 (fourth)
0x609b58: val: 2181 (fifth)
0x60df80: val:  517 (sixth)
-------------------
*/

#define error(...) do { printf(__VA_ARGS__); exit(1); } while(0)
static int move_callback(int handle, void* old, void* new)
{
    printf("MOVED!\n");
    return BUFLIB_CB_OK;
}

static int shrink_callback(int handle, unsigned hints, void* start, size_t size)
{
    char* buf = start;
    size /= 2;

    printf("SHRINK! %u\n", hints);

    memmove(buf + size/2, buf, size);
    if (core_shrink(handle, buf + size/2, size))
    {
        return BUFLIB_CB_OK;
    }
    return BUFLIB_CB_CANNOT_SHRINK;
}

struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = shrink_callback,
};

int main(void)
{
    UT_core_allocator_init();

    int first  = core_alloc("first", 10<<10);
    int second = core_alloc("second", 10<<10);
    int third  = core_alloc_ex("third", 10<<10, &ops);

    strcpy(core_get_data(third), "third");

    core_free(second);
    int fourth = core_alloc("fourth", 11<<10);
    strcpy(core_get_data(fourth), "fourth");

    /* this should cause MOVED! twice */
    int fifth = core_alloc("fifth", 17<<10);
    if (fifth <= 0) error("fifth failed\n");
    strcpy(core_get_data(fifth), "fifth");

    core_print_blocks(&print_simple);
    int sixth = core_alloc("sixth", 4<<10);
    if (sixth <= 0) error("sixth failed\n");
    strcpy(core_get_data(sixth), "sixth");

    core_print_blocks(&print_simple);

    int ret = strcmp(core_get_data(third), "third")
           || strcmp(core_get_data(fourth), "fourth")
           || strcmp(core_get_data(fifth), "fifth")
           || strcmp(core_get_data(sixth), "sixth");
    core_free(first);
    core_free(third);
    core_free(fourth);
    core_free(fifth);
    core_free(sixth);

    return ret;
}
