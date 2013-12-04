/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Daniel Ankers
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

/* Core locks using Peterson's mutual exclusion algorithm.
 * ASM optimized version of C code, see firmware/asm/corelock.c */

#include "cpu.h"

/*---------------------------------------------------------------------------
 * Wait for the corelock to become free and acquire it when it does.
 *---------------------------------------------------------------------------
 */
void __attribute__((naked)) corelock_lock(struct corelock *cl)
{
    /* Relies on the fact that core IDs are complementary bitmasks (0x55,0xaa) */
    asm volatile (
        "mov    r1, %0               \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "strb   r1, [r0, r1, lsr #7] \n" /* cl->myl[core] = core */
        "eor    r2, r1, #0xff        \n" /* r2 = othercore */
        "strb   r2, [r0, #2]         \n" /* cl->turn = othercore */
    "1:                              \n"
        "ldrb   r3, [r0, r2, lsr #7] \n" /* cl->myl[othercore] == 0 ? */
        "cmp    r3, #0               \n" /* yes? lock acquired */
        "bxeq   lr                   \n"
        "ldrb   r3, [r0, #2]         \n" /* || cl->turn == core ? */
        "cmp    r3, r1               \n"
        "bxeq   lr                   \n" /* yes? lock acquired */
        "b      1b                   \n" /* keep trying */
        : : "i"(&PROCESSOR_ID)
    );
    (void)cl;
}

/*---------------------------------------------------------------------------
 * Try to aquire the corelock. If free, caller gets it, otherwise return 0.
 *---------------------------------------------------------------------------
 */
int __attribute__((naked)) corelock_try_lock(struct corelock *cl)
{
    /* Relies on the fact that core IDs are complementary bitmasks (0x55,0xaa) */
    asm volatile (
        "mov    r1, %0               \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "mov    r3, r0               \n"
        "strb   r1, [r0, r1, lsr #7] \n" /* cl->myl[core] = core */
        "eor    r2, r1, #0xff        \n" /* r2 = othercore */
        "strb   r2, [r0, #2]         \n" /* cl->turn = othercore */
        "ldrb   r0, [r3, r2, lsr #7] \n" /* cl->myl[othercore] == 0 ? */
        "eors   r0, r0, r2           \n" /* yes? lock acquired */
        "bxne   lr                   \n"
        "ldrb   r0, [r3, #2]         \n" /* || cl->turn == core? */
        "ands   r0, r0, r1           \n"
        "streqb r0, [r3, r1, lsr #7] \n" /* if not, cl->myl[core] = 0 */
        "bx     lr                   \n" /* return result */
        : : "i"(&PROCESSOR_ID)
    );

    return 0;
    (void)cl;
}

/*---------------------------------------------------------------------------
 * Release ownership of the corelock
 *---------------------------------------------------------------------------
 */
void __attribute__((naked)) corelock_unlock(struct corelock *cl)
{
    asm volatile (
        "mov    r1, %0               \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "mov    r2, #0               \n" /* cl->myl[core] = 0 */
        "strb   r2, [r0, r1, lsr #7] \n"
        "bx     lr                   \n"
        : : "i"(&PROCESSOR_ID)
    );
    (void)cl;
}
