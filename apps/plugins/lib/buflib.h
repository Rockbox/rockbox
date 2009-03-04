/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* This is a memory allocator designed to provide reasonable management of free
* space and fast access to allocated data. More than one allocator can be used
* at a time by initializing multiple contexts.
*
* Copyright (C) 2009 Andrew Mahone
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

#ifndef _BUFLIB_H_
#include <plugin.h>

union buflib_data
{
    intptr_t val;
    union buflib_data *ptr;
};

struct buflib_context
{
    union buflib_data *handle_table;
    union buflib_data *first_free_handle;
    union buflib_data *last_handle;
    union buflib_data *first_free_block;
    union buflib_data *alloc_end;
    bool compact;
};

void buflib_init(struct buflib_context *context, void *buf, size_t size);
int buflib_alloc(struct buflib_context *context, size_t size);
void buflib_free(struct buflib_context *context, int handle);

/* always_inline is due to this not getting inlined when not optimizing, which
 * leads to an unresolved reference since it doesn't exist as a non-inline
 * function
 */
extern inline __attribute__((always_inline))
void* buflib_get_data(struct buflib_context *context, int handle)
{
    return (void*)(context->handle_table[-handle].ptr);
}
#endif
