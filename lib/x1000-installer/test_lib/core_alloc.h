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

/* fake core_alloc implementation for testing */

#ifndef CORE_ALLOC_H
#define CORE_ALLOC_H

#include <stddef.h>
#include <stdbool.h>

struct buflib_callbacks {
    int (*move_callback)(int handle, void* current, void* new);
    int (*shrink_callback)(int handle, unsigned hints, void* start, size_t old_size);
    void (*sync_callback)(int handle, bool sync_on);
};

extern struct buflib_callbacks buflib_ops_locked;

int core_alloc(const char* name, size_t size);
int core_alloc_ex(const char* name, size_t size, struct buflib_callbacks* cb);
int core_free(int handle);
void* core_get_data(int handle);

#endif
