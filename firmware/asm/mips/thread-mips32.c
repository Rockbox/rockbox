/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * 32-bit MIPS threading support
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

static void USED_ATTR start_thread(void *addr)
{
    asm volatile (
        ".set noreorder       \n"
        ".set noat            \n"
        "lw     $25,    4(%0) \n" /* Fetch thread function pointer ($25 = t9) */
        "lw     $25,   40(%0) \n" /* Set initial sp(=$29) */
        "jalr   $25           \n" /* Start the thread */
        "sw     $0,    48(%0) \n" /* Clear start address */
        ".set at              \n"
        ".set reorder         \n"
        : : "r" (addr) : "$25"
    );
    thread_exit();
}


/* Place context pointer in s0 slot, function pointer in s1 slot, and
 * start_thread pointer in context_start */
#define THREAD_STARTUP_INIT(core, thread, function)            \
    ({ (thread)->context.r[0]  = (uint32_t)&(thread)->context, \
       (thread)->context.r[1]  = (uint32_t)(function),         \
       (thread)->context.start = (uint32_t)start_thread; })

/*---------------------------------------------------------------------------
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void store_context(void* addr)
{
    asm volatile (
        ".set noreorder        \n"
        ".set noat             \n"
        "sw    $16,  0(%0)     \n" /* s0 */
        "sw    $17,  4(%0)     \n" /* s1 */
        "sw    $18,  8(%0)     \n" /* s2 */
        "sw    $19, 12(%0)     \n" /* s3 */
        "sw    $20, 16(%0)     \n" /* s4 */
        "sw    $21, 20(%0)     \n" /* s5 */
        "sw    $22, 24(%0)     \n" /* s6 */
        "sw    $23, 28(%0)     \n" /* s7 */
        "sw    $28, 32(%0)     \n" /* gp */
        "sw    $30, 36(%0)     \n" /* fp */
        "sw    $29, 40(%0)     \n" /* sp */
        "sw    $31, 44(%0)     \n" /* ra */
        ".set at               \n"
        ".set reorder          \n"
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
        ".set noat             \n"
        ".set noreorder        \n"
        "lw    $25, 48(%0)     \n" /* Get start address (t9 = $25) */
        "beqz  $25, running    \n" /* NULL -> already running */
        "nop                   \n"
        "jr    $25             \n"
        "move  $4, %0          \n" /* a0 = context branch delay slot anyway */
    "running:                  \n"
        "lw    $16,  0(%0)     \n" /* s0 */
        "lw    $17,  4(%0)     \n" /* s1 */
        "lw    $18,  8(%0)     \n" /* s2 */
        "lw    $19, 12(%0)     \n" /* s3 */
        "lw    $20, 16(%0)     \n" /* s4 */
        "lw    $21, 20(%0)     \n" /* s5 */
        "lw    $22, 24(%0)     \n" /* s6 */
        "lw    $23, 28(%0)     \n" /* s7 */
        "lw    $28, 32(%0)     \n" /* gp */
        "lw    $30, 36(%0)     \n" /* fp */
        "lw    $29, 40(%0)     \n" /* sp */
        "lw    $31, 44(%0)     \n" /* ra */
        ".set at               \n"
        ".set reorder          \n"
        : : "r" (addr) : "$25"
    );
}

