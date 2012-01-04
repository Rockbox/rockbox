/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
 *
 * Generic ARM threading support
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


#include <windows.h>
#include "system.h"

#define INIT_MAIN_THREAD

#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.stack_size = (thread)->stack_size, \
       (thread)->context.stack = (uintptr_t)(thread)->stack; \
       (thread)->context.start = function; })

static void init_main_thread(void *addr)
{
    struct regs *context = (struct regs*)addr;
    /* we must convert the current main thread to a fiber to be able to
     * schedule other fibers */
    context->uc = ConvertThreadToFiber(NULL);
    context->stack_size = 0;
}

static inline void store_context(void* addr)
{
    (void)addr;
    /* nothing to do here, Fibers continue after the SwitchToFiber call */
}

static void start_thread(void)
{
    void (*func)(void) = GetFiberData();
    func();
    /* go out if thread function returns */
    thread_exit();
}

/*
 * Load context and run it
 *
 * Resume execution from the last load_context call for the thread
 */

static inline void load_context(const void* addr)
{
    struct regs *context = (struct regs*)addr;
    if (UNLIKELY(context->start))
    {   /* need setup before switching to it */
        context->uc = CreateFiber(context->stack_size,
                        (LPFIBER_START_ROUTINE)start_thread, context->start);
        /* can't assign stack pointer, only stack size */
        context->stack_size = 0;
        context->start = NULL;
    }
    SwitchToFiber(context->uc);
}
