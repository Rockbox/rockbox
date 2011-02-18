/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Thom Johansen
 * Copyright (C) 2010 by Thomas Martitz (Android-suitable core_sleep())
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

#include <system.h>
/*---------------------------------------------------------------------------
 * Start the thread running and terminate it if it returns
 *---------------------------------------------------------------------------
 */
static void __attribute__((naked,used)) start_thread(void)
{
    /* r0 = context */
    asm volatile (
        "ldr    sp, [r0, #32]            \n" /* Load initial sp */
        "ldr    r4, [r0, #40]            \n" /* start in r4 since it's non-volatile */
        "mov    r1, #0                   \n" /* Mark thread as running */
        "str    r1, [r0, #40]            \n"
        "mov    lr, pc                   \n" /* Call thread function */
        "bx     r4                       \n"
    ); /* No clobber list - new thread doesn't care */
    thread_exit();
}

/* For startup, place context pointer in r4 slot, start_thread pointer in r5
 * slot, and thread function pointer in context.start. See load_context for
 * what happens when thread is initially going to run. */
#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.r[0] = (uint32_t)&(thread)->context,  \
       (thread)->context.r[1] = (uint32_t)start_thread, \
       (thread)->context.start = (uint32_t)function; })


/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile(
        "stmia  %0, { r4-r11, sp, lr } \n"
        : : "r" (addr)
    );
}

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(const void* addr)
{
    asm volatile(
        "ldr     r0, [%0, #40]          \n" /* Load start pointer */
        "cmp     r0, #0                 \n" /* Check for NULL */

        /* If not already running, jump to start */
        "ldmneia %0, { r0, pc }         \n"
        "ldmia   %0, { r4-r11, sp, lr } \n" /* Load regs r4 to r14 from context */
        : : "r" (addr) : "r0" /* only! */
    );
}

/*
 * this core sleep suspends the OS thread rockbox runs under, which greatly
 * reduces cpu usage (~100% to <10%)
 *
 * it returns when when the tick timer is called, other interrupt-like
 * events occur
 *
 * wait_for_interrupt is implemented in kernel-<platform>.c
 **/

static inline void core_sleep(void)
{
    enable_irq();
    wait_for_interrupt();
}


