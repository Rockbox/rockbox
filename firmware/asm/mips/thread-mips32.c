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
        "lw     $t9,    4(%0)  \n" /* Fetch thread function pointer ($25 = t9) */
        "lw     $sp,   40(%0)  \n" /* Set initial sp(=$29) */
        "jalr   $t9            \n" /* Start the thread */
        "sw     $zero, 48(%0)  \n" /* Clear start address */
        ".set at              \n"
        ".set reorder         \n"
        : : "r" (addr) : "t9"
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
        "sw    $s0,  0(%0)     \n" /* s0 */
        "sw    $s1,  4(%0)     \n" /* s1 */
        "sw    $s2,  8(%0)     \n" /* s2 */
        "sw    $s3, 12(%0)     \n" /* s3 */
        "sw    $s4, 16(%0)     \n" /* s4 */
        "sw    $s5, 20(%0)     \n" /* s5 */
        "sw    $s6, 24(%0)     \n" /* s6 */
        "sw    $s7, 28(%0)     \n" /* s7 */
        "sw    $gp, 32(%0)     \n" /* gp */
        "sw    $fp, 36(%0)     \n" /* fp */
        "sw    $sp, 40(%0)     \n" /* sp */
        "sw    $ra, 44(%0)     \n" /* ra */
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
        "lw    $t9, 48(%0)     \n" /* Get start address ($8 = t0) */
        "beqz  $t9, running    \n" /* NULL -> already running */
        "nop                   \n"
        "jr    $t9             \n"
        "move  $a0, %0          \n" /* a0 = context branch delay slot anyway */
    "running:                  \n"
        "lw    $s0,  0(%0)     \n" /* s0 */
        "lw    $s1,  4(%0)     \n" /* s1 */
        "lw    $s2,  8(%0)     \n" /* s2 */
        "lw    $s3, 12(%0)     \n" /* s3 */
        "lw    $s4, 16(%0)     \n" /* s4 */
        "lw    $s5, 20(%0)     \n" /* s5 */
        "lw    $s6, 24(%0)     \n" /* s6 */
        "lw    $s7, 28(%0)     \n" /* s7 */
        "lw    $gp, 32(%0)     \n" /* gp */
        "lw    $fp, 36(%0)     \n" /* fp */
        "lw    $sp, 40(%0)     \n" /* sp */
        "lw    $ra, 44(%0)     \n" /* ra */
        ".set at               \n"
        ".set reorder          \n"
        : : "r" (addr) : "t9"
    );
}

