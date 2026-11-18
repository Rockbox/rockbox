/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Mauricio G.
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

/* Thread stacks for the native scheduler on the PSP.
 *
 * All Rockbox threads run inside the main PSP OS thread. The PSP firmware
 * stops with a breakpoint when an OS thread blocks while its stack pointer
 * is outside that thread's registered stack (reproduced on hardware without
 * Rockbox, see testing/stack-range-probe). Rockbox thread stacks are static
 * arrays elsewhere in memory, so every one is replaced here by a block from
 * the low end of the registered main OS stack. The main Rockbox thread keeps
 * using the top of that stack.
 *
 * The link wraps create_thread() (packaging/psp/psp.make), so the shared
 * scheduler and every caller stay unchanged. A block is tied to the caller's
 * buffer: a thread created again with the same static buffer gets the same
 * block back, because two live threads can never share a buffer. */

#include <pspkernel.h>
#include "config.h"
#include "system.h"
#include "thread.h"
#include "panic.h"

/* create_thread() fills new stacks with this (DEADBEEF, kernel/thread-internal.h) */
#define STACK_FILL    ((uintptr_t)0xdeadbeef)

#define GUARD_BYTES   1024          /* kept free above the registered bottom */
#define MAIN_RESERVE  (256 * 1024)  /* top of the OS stack: main thread */
#define MAX_BLOCKS    32

static struct stack_block
{
    void       *orig;     /* caller's buffer */
    uintptr_t  *mem;      /* block in the OS stack */
    size_t      size;
    const char *name;     /* last thread that used it */
} blocks[MAX_BLOCKS];
static int nblocks;

static uintptr_t pool_start, pool_next, pool_end, main_top;

unsigned int __real_create_thread(void (*function)(void),
                                  void* stack, size_t stack_size,
                                  unsigned flags, const char *name
                                  IF_PRIO(, int priority)
                                  IF_COP(, unsigned int core));

static void pool_init(void)
{
    SceKernelThreadInfo info;
    info.size = sizeof(info);
    if (sceKernelReferThreadStatus(sceKernelGetThreadId(), &info) < 0)
        panicf("thread stacks: no main stack info");

    uintptr_t base = (uintptr_t)info.stack;
    main_top   = base + info.stackSize;
    pool_start = pool_next = (base + GUARD_BYTES + 15) & ~15;
    pool_end   = main_top - MAIN_RESERVE;

    /* Mark the main thread's reserve below the current stack pointer, so
     * psp_stack_report() can tell how deep the main thread went. */
    uintptr_t sp;
    asm volatile ("move %0, $sp" : "=r" (sp));
    for (uintptr_t *p = (uintptr_t *)pool_end; (uintptr_t)p < sp - 4096; p++)
        *p = STACK_FILL;
}

unsigned int __wrap_create_thread(void (*function)(void),
                                  void* stack, size_t stack_size,
                                  unsigned flags, const char *name
                                  IF_PRIO(, int priority)
                                  IF_COP(, unsigned int core))
{
    if (!pool_start)
        pool_init();

    struct stack_block *b = NULL;
    for (int i = 0; i < nblocks; i++)
        if (blocks[i].orig == stack && blocks[i].size >= stack_size)
            b = &blocks[i];

    if (!b) {
        size_t size = (stack_size + 15) & ~15;
        if (nblocks == MAX_BLOCKS || pool_next + size > pool_end)
            panicf("thread stacks: no room for %s (%lu bytes)",
                   name ? name : "?", (unsigned long)stack_size);
        b = &blocks[nblocks++];
        b->orig = stack;
        b->mem  = (uintptr_t *)pool_next;
        b->size = size;
        pool_next += size;
    }
    b->name = name;

    return __real_create_thread(function, b->mem, stack_size, flags, name
                                IF_PRIO(, priority) IF_COP(, core));
}

static size_t used_bytes(const uintptr_t *low, const uintptr_t *high)
{
    const uintptr_t *p = low;
    while (p < high && *p == STACK_FILL)
        p++;
    return (high - p) * sizeof(*p);
}

/* High-water marks, for test builds. out() is printf-like. */
void psp_stack_report(void (*out)(const char *fmt, ...))
{
    for (int i = 0; i < nblocks; i++)
        out("ABSTACK name=%s size=%lu used=%lu\n",
            blocks[i].name ? blocks[i].name : "?",
            (unsigned long)blocks[i].size,
            (unsigned long)used_bytes(blocks[i].mem,
                                      blocks[i].mem + blocks[i].size / sizeof(uintptr_t)));
    out("ABSTACK name=main(reserve) size=%lu used=%lu\n",
        (unsigned long)(main_top - pool_end),
        (unsigned long)used_bytes((uintptr_t *)pool_end, (uintptr_t *)main_top));
    out("ABSTACK pool used=%lu of %lu blocks=%d\n",
        (unsigned long)(pool_next - pool_start),
        (unsigned long)(pool_end - pool_start), nblocks);
}
