 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#include "xrick/system/system.h"

#include "xrick/debug.h"

#include "plugin.h"

/*
 * Local variables
 */
enum
{
    ALIGNMENT = sizeof(void*)  /* this is more of an educated guess; might want to adjust for your specific architecture */
};
static U8 * stackBuffer;
static U8 * stackTop;
static size_t stackSize;
static size_t stackMaxSize;
static bool isMemoryInitialised = false;
IFDEBUG_MEMORY( static size_t maxUsedMemory = 0; );

/*
 * Initialise memory stack
 */
bool sysmem_init(void)
{
    if (isMemoryInitialised)
    {
        return true;
    }

    if (rb->audio_status())
    {
        /* Playback must be stopped the entire time the sound buffer is used.*/
        rb->audio_stop();
    }

    stackBuffer = rb->plugin_get_audio_buffer(&stackMaxSize);
    stackTop = stackBuffer;
    stackSize = 0;
    isMemoryInitialised = true;
    return true;
}

/*
 * Cleanup memory stack
 */
void sysmem_shutdown(void)
{
    if (!isMemoryInitialised)
    {
        return;
    }

    if (stackTop != stackBuffer || stackSize != 0)
    {
        sys_error("(memory) improper deallocation detected");
    }

    IFDEBUG_MEMORY(
        sys_printf("xrick/memory: max memory usage was %u bytes\n", maxUsedMemory);
    );

    isMemoryInitialised = false;
}

/*
 * Allocate a memory-aligned block on top of the memory stack
 */
void *sysmem_push(size_t size)
{
    uintptr_t alignedPtr;
    size_t * allocatedSizePtr;

    size_t neededSize = sizeof(size_t) + size + (ALIGNMENT - 1);
    if (stackSize + neededSize > stackMaxSize)
    {
        sys_error("(memory) tried to allocate a block when memory full");
        return NULL;
    }

    alignedPtr = (((uintptr_t)stackTop) + sizeof(size_t) + ALIGNMENT) & ~((uintptr_t)(ALIGNMENT - 1));

    allocatedSizePtr = (size_t *)(alignedPtr);
    allocatedSizePtr[-1] = neededSize;

    stackTop += neededSize;
    stackSize += neededSize;

    IFDEBUG_MEMORY(
        sys_printf("xrick/memory: allocated %u bytes\n", neededSize);
        if (stackSize > maxUsedMemory) maxUsedMemory = stackSize;
    );

    return (void *)alignedPtr;
}

/*
 * Release block from the top of the memory stack
 */
void sysmem_pop(void * alignedPtr)
{
    size_t allocatedSize;

    if (!alignedPtr)
    {
        return;
    }

    if (stackSize == 0)
    {
        sys_error("(memory) tried to release a block when memory empty");
        return;
    }

    allocatedSize = ((size_t *)(alignedPtr))[-1];
    stackTop -= allocatedSize;
    stackSize -= allocatedSize;

    IFDEBUG_MEMORY(
        if ((uintptr_t)alignedPtr != ((((uintptr_t)stackTop) + sizeof(size_t) + ALIGNMENT) & ~((uintptr_t)(ALIGNMENT - 1))))
        {
            sys_error("(memory) tried to release a wrong block");
            return;
        }
    );

    IFDEBUG_MEMORY(
        sys_printf("xrick/memory: released %u bytes\n", allocatedSize);
    );
}

/* eof */
