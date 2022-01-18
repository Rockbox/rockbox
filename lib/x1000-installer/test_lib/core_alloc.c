/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "core_alloc.h"
#include <stdlib.h>

#define N_POINTERS 100

static void* pointers[N_POINTERS];
struct buflib_callbacks buflib_ops_locked = {NULL, NULL, NULL};

int core_alloc(const char* name, size_t size)
{
    (void)name;

    void* mem = malloc(size);
    if(!mem)
        return -1;

    for(int i = 0; i < N_POINTERS; ++i) {
        if(pointers[i])
            continue;

        pointers[i] = mem;
        return i + 1;
    }

    free(mem);
    return -1;
}

int core_alloc_ex(const char* name, size_t size, struct buflib_callbacks* cb)
{
    (void)cb;
    return core_alloc(name, size);
}

int core_free(int handle)
{
    if(handle > 0) {
        free(pointers[handle-1]);
        pointers[handle-1] = NULL;
    }

    return 0;
}

void* core_get_data(int handle)
{
    if(handle > 0)
        return pointers[handle-1];

    return NULL;
}
