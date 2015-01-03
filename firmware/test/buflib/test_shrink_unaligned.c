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
 * Expected output:
-------------------
0x602520: val: 1285 (first)
0x604d48: val:  -32 (<unallocated>)
0x604e48: val: 1222 (second)
0x607470: val:  -31 (<unallocated>)
0x607570: val: 1285 (third)
-------------------
*/
struct buflib_callbacks ops;

int main(void)
{
    UT_core_allocator_init();

    int first  = core_alloc("first", 10<<10);
    int second = core_alloc("second", 10<<10);
    int third  = core_alloc("third", 10<<10);

    strcpy((char*)core_get_data(second)+0x102, "foobar");
    core_shrink(second, (char*)core_get_data(second)+0x102, (10<<10)-0x200);
    core_print_blocks(&print_simple);

    int ret = strcmp(core_get_data(second), "foobar");

    core_free(first);
    core_free(second);
    core_free(third);

    return ret;
}
