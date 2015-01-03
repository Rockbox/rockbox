/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2015 Thomas Jarosch
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
#include "util.h"
#include "stdio.h"
#include "buflib.h"
#include "system.h"

void print_simple(const char *str)
{
    printf("%s\n", str);
}

void print_handle(int handle_num, const char *str)
{
    (void)handle_num;
    printf("%s\n", str);
}

/* fake core_allocator_init() with a fixed 50kb buffer size */
void UT_core_allocator_init()
{
    extern struct buflib_context core_ctx;
    static char buf[50<<10];
    unsigned char *raw_start = buf;
    unsigned char *aligned_start = ALIGN_UP(raw_start, sizeof(intptr_t));

    buflib_init(&core_ctx, aligned_start, sizeof(buf) - (aligned_start - raw_start));
}

/* TODO: those should be part of core_alloc */
void core_print_blocks(void (*print)(const char*))
{
    (void)print;
    extern struct buflib_context core_ctx;
    buflib_print_blocks(&core_ctx, &print_handle);
}

void core_print_allocs(void (*print)(const char*))
{
    (void)print;
    extern struct buflib_context core_ctx;
    buflib_print_allocs(&core_ctx, &print_handle);
}
