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

    size_t wanted = hints & BUFLIB_SHRINK_SIZE_MASK;

    if (handle == 4)
    {
        buf+=1, size-=1;
        memmove(buf, buf-1, (size < 20) ? size : 20);
        core_shrink(handle, buf, size);
        return BUFLIB_CB_OK;
    }
    return BUFLIB_CB_CANNOT_SHRINK;
}

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = shrink_callback,
};

static struct buflib_callbacks ops2 = {
    .move_callback = NULL,
    .shrink_callback = shrink_callback,
};

int main(void)
{
    size_t size2, size4;
    UT_core_allocator_init();

    printf("available: %zu\n", core_available());
    int first = core_alloc("first, fixed", 4<<10);
    if (first <= 0) error("first failed\n");

    printf("available: %zu\n", core_available());
    int second = core_alloc_maximum("second, var", &size2, &ops);
    if (second <= 0) error("second failed\n");
    printf("second size: %zu\n", size2);

    strcpy(core_get_data(second), "begin");
    strcpy(core_get_data(second)+124, "end");
    printf("%s\n", core_get_name(second));
    if (!core_shrink(second, core_get_data(second), 128))
        error("shrink second failed\n");

    int third = core_alloc("third, fixed", 20<<10);
    if (third <= 0) error("third failed");
    strcpy(core_get_data(third), "third");

    printf("available: %zu\n", core_available());
    int fourth = core_alloc_maximum("fourth", &size4, &ops2);
    if (fourth <= 0) error("fourth failed\n");
    core_print_blocks(&print_simple);
    if (!core_shrink(fourth, core_get_data(fourth)+(5<<10), size4-(5<<10)))
    {
        error("shrink fourth failed\n");
    }
    sprintf(core_get_data(fourth), "fourth size: %zu", size4);
    core_print_blocks(&print_simple);

    int fifth = core_alloc("fifth, fixed", 6<<10);
    if (fifth <= 0) error("fifth failed\n");

    printf("%s\n", core_get_data(fourth));
    core_print_blocks(&print_simple);
    core_print_allocs(&print_simple);

    return 0;
}
