/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
 *
 * SH processor threading support
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
void start_thread(void); /* Provide C access to ASM label */
static void USED_ATTR __start_thread(void)
{
    /* r8 = context */
    asm volatile (
    "_start_thread:            \n" /* Start here - no naked attribute */
        "mov.l  @(4, r8), r0   \n" /* Fetch thread function pointer */
        "mov.l  @(28, r8), r15 \n" /* Set initial sp */
        "mov    #0, r1         \n" /* Start the thread */
        "jsr    @r0            \n"
        "mov.l  r1, @(36, r8)  \n" /* Clear start address */
    );
    thread_exit();
}

/* Place context pointer in r8 slot, function pointer in r9 slot, and
 * start_thread pointer in context_start */
#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.r[0] = (uint32_t)&(thread)->context, \
       (thread)->context.r[1] = (uint32_t)(function),         \
       (thread)->context.start = (uint32_t)start_thread; })

/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        "add     #36, %0   \n" /* Start at last reg. By the time routine */
        "sts.l   pr, @-%0  \n" /* is done, %0 will have the original value */
        "mov.l   r15,@-%0  \n"
        "mov.l   r14,@-%0  \n"
        "mov.l   r13,@-%0  \n"
        "mov.l   r12,@-%0  \n"
        "mov.l   r11,@-%0  \n"
        "mov.l   r10,@-%0  \n"
        "mov.l   r9, @-%0  \n"
        "mov.l   r8, @-%0  \n"
        : : "r" (addr)
    );
}

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(const void* addr)
{
    asm volatile (
        "mov.l  @(36, %0), r0 \n" /* Get start address */
        "tst    r0, r0        \n"
        "bt     .running      \n" /* NULL -> already running */
        "jmp    @r0           \n" /* r8 = context */
    ".running:                \n"
        "mov.l  @%0+, r8      \n" /* Executes in delay slot and outside it */
        "mov.l  @%0+, r9      \n"
        "mov.l  @%0+, r10     \n"
        "mov.l  @%0+, r11     \n"
        "mov.l  @%0+, r12     \n"
        "mov.l  @%0+, r13     \n"
        "mov.l  @%0+, r14     \n"
        "mov.l  @%0+, r15     \n"
        "lds.l  @%0+, pr      \n"
        : : "r" (addr) : "r0" /* only! */
    );
}


