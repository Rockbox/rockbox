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

/*---------------------------------------------------------------------------
 * Start the thread running and terminate it if it returns
 *---------------------------------------------------------------------------
 */
static void __attribute__((naked)) USED_ATTR start_thread(void)
{
    /* r0 = context */
    asm volatile (
        "ldr    sp, [r0, #32]            \n" /* Load initial sp */
        "ldr    r4, [r0, #40]            \n" /* start in r4 since it's non-volatile */
        "mov    r1, #0                   \n" /* Mark thread as running */
        "str    r1, [r0, #40]            \n"
#if NUM_CORES > 1
        "bl     commit_discard_idcache   \n" /* Invalidate this core's cache. */
#endif
        "mov    lr, pc                   \n" /* Call thread function */
        "bx     r4                       \n"
    ); /* No clobber list - new thread doesn't care */
    thread_exit();
#if 0
    asm volatile (".ltorg"); /* Dump constant pool */
#endif
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
#if defined(CPU_ARM_MICRO) && ARCH_VERSION >= 7
        "stmia  %0, { r4-r11, lr }     \n"
        "str    sp, [%0, #36]          \n"
#else
        "stmia  %0, { r4-r11, sp, lr } \n"
#endif
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
        BEGIN_ARM_ASM_SYNTAX_UNIFIED
        "ldr     r0, [%0, #40]          \n" /* Load start pointer */
        "cmp     r0, #0                 \n" /* Check for NULL */

        /* If not already running, jump to start */
#if ARM_ARCH == 4 && defined(USE_THUMB)
        "ldmiane %0, { r0, r12 }        \n"
        "bxne    r12                    \n"
#else
        "ldmiane %0, { r0, pc }         \n"
#endif

#if defined(CPU_ARM_MICRO) && ARCH_VERSION >= 7
        "mov     r0, %0                 \n"
        "ldmia   r0, { r4-r11, lr }     \n"
        "ldr     sp, [r0, #36]          \n"
#else
        "ldmia   %0, { r4-r11, sp, lr } \n" /* Load regs r4 to r14 from context */
#endif
        END_ARM_ASM_SYNTAX_UNIFIED
        : : "r" (addr) : "r0" /* only! */
    );
}


