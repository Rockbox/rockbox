/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
 *
 * Coldfire processor threading support
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
static void __attribute__((used)) __start_thread(void)
{
    /* a0=macsr, a1=context */
    asm volatile (
    "start_thread:             \n" /* Start here - no naked attribute */
        "move.l  %a0, %macsr   \n" /* Set initial mac status reg */
        "lea.l   48(%a1), %a1  \n"
        "move.l  (%a1)+, %sp   \n" /* Set initial stack */
        "move.l  (%a1), %a2    \n" /* Fetch thread function pointer */
        "clr.l   (%a1)         \n" /* Mark thread running */
        "jsr     (%a2)         \n" /* Call thread function */
    );
    thread_exit();
}

/* Set EMAC unit to fractional mode with saturation for each new thread,
 * since that's what'll be the most useful for most things which the dsp
 * will do. Codecs should still initialize their preferred modes
 * explicitly. Context pointer is placed in d2 slot and start_thread
 * pointer in d3 slot. thread function pointer is placed in context.start.
 * See load_context for what happens when thread is initially going to
 * run.
 */
#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.macsr = EMAC_FRACTIONAL | EMAC_SATURATE, \
       (thread)->context.d[0] = (uint32_t)&(thread)->context, \
       (thread)->context.d[1] = (uint32_t)start_thread,       \
       (thread)->context.start = (uint32_t)(function); })

/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        "move.l  %%macsr,%%d0                  \n"
        "movem.l %%d0/%%d2-%%d7/%%a2-%%a7,(%0) \n"
        : : "a" (addr) : "d0" /* only! */
    );
}

/*---------------------------------------------------------------------------
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void load_context(const void* addr)
{
    asm volatile (
        "move.l  52(%0), %%d0                   \n"  /* Get start address */
        "beq.b   1f                             \n"  /* NULL -> already running */
        "movem.l (%0), %%a0-%%a2                \n"  /* a0=macsr, a1=context, a2=start_thread */
        "jmp     (%%a2)                         \n"  /* Start the thread */
    "1:                                         \n"
        "movem.l (%0), %%d0/%%d2-%%d7/%%a2-%%a7 \n"  /* Load context */
        "move.l  %%d0, %%macsr                  \n"
        : : "a" (addr) : "d0" /* only! */
    );
}

/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(void)
{
    /* Supervisor mode, interrupts enabled upon wakeup */
    asm volatile ("stop #0x2000");
};

/*---------------------------------------------------------------------------
 * Call this from asm to make sure the sp is pointing to the
 * correct place before the context is saved.
 *---------------------------------------------------------------------------
 */
static inline void _profile_thread_stopped(int current_thread)
{
    asm volatile ("move.l %[id], -(%%sp)\n\t"
                  "jsr profile_thread_stopped\n\t"
                  "addq.l #4, %%sp\n\t"
                  :: [id] "r" (current_thread)
                  : "cc", "memory");
}
