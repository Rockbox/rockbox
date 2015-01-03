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
 *  Expected output (64bit):
-------------------
get_all(1):	0x6027a0
   	0x6027c8
   	6424
get_all(1):	0x6027a0
   	0x6027c8
   	3232
get_all(1):	0x6027a0
   	0x6027c8
   	3232
dont freeze(2):	0x603440
   	0x603470
   	152
0x6027a0: val:  404 (get_all)
0x603440: val:   19 (dont freeze)
-------------------
*/
struct buflib_callbacks ops;
int main(void)
{
    UT_core_allocator_init();
    size_t size;
    int handle = core_alloc_maximum("get_all", &size, &ops);

    if (handle <= 0)
        printf("core_alloc_maximum error\n");
    int handle2;

    core_print_allocs(&print_simple);

    /* this should freeze */
    // core_alloc("freeze", 100);
    core_shrink(handle, core_get_data(handle), size/2);

    core_print_allocs(&print_simple);

    /* this should not freeze anymore */
    handle2 = core_alloc("dont freeze", 100);
    if (handle2 <= 0)
        printf("handle 2 failed!\n");

    core_print_allocs(&print_simple);
    core_print_blocks(&print_simple);

    core_free(handle);
    core_free(handle2);
    return 0;
}
