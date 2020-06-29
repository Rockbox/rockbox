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
void corelock_lock(struct corelock *cl)
{
    /* Relies on the fact that core IDs are complementary bitmasks (0x55,0xaa) */
    asm volatile (
        "mov    r1, %[id]            \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "strb   r1, [%[cl], r1, lsr #7] \n" /* cl->myl[core] = core */
        "eor    r2, r1, #0xff        \n" /* r2 = othercore */
        "strb   r2, [%[cl], #2]      \n" /* cl->turn = othercore */
    "1:                              \n"
        "ldrb   r2, [%[cl], r2, lsr #7] \n" /* cl->myl[othercore] == 0 ? */
        "cmp    r2, #0               \n" /* yes? lock acquired */
        "beq    2f                   \n"
        "ldrb   r2, [%[cl], #2]      \n" /* || cl->turn == core ? */
        "cmp    r2, r1               \n"
        "bne    1b                   \n" /* no? try again */
    "2:                              \n" /* Done */
        :
        : [id] "i"(&PROCESSOR_ID), [cl] "r" (cl)
        : "r1","r2","cc"
    );
}

/*---------------------------------------------------------------------------
 * Try to aquire the corelock. If free, caller gets it, otherwise return 0.
 *---------------------------------------------------------------------------
 */
int corelock_try_lock(struct corelock *cl)
{
    int rval = 0;

    /* Relies on the fact that core IDs are complementary bitmasks (0x55,0xaa) */
    asm volatile (
        "mov    r1, %[id]            \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "strb   r1, [%[cl], r1, lsr #7] \n" /* cl->myl[core] = core */
        "eor    r2, r1, #0xff        \n" /* r2 = othercore */
        "strb   r2, [%[cl], #2]      \n" /* cl->turn = othercore */
        "ldrb   %[rv], [%[cl], r2, lsr #7] \n" /* cl->myl[othercore] == 0 ? */
        "eors   %[rv], %[rv], r2     \n"
        "bne    1f                   \n" /* yes? lock acquired */
        "ldrb   %[rv], [%[cl], #2]   \n" /* || cl->turn == core? */
        "ands   %[rv], %[rv], r1     \n"
        "streqb %[rv], [%[cl], r1, lsr #7] \n" /* if not, cl->myl[core] = 0 */
    "1:                              \n" /* Done */
        : [rv] "=r"(rval)
        : [id] "i" (&PROCESSOR_ID), [cl] "r" (cl)
        : "r1","r2","cc"
    );

    return rval;
}

/*---------------------------------------------------------------------------
 * Release ownership of the corelock
 *---------------------------------------------------------------------------
 */
void corelock_unlock(struct corelock *cl)
{
    asm volatile (
        "mov    r1, %[id]            \n" /* r1 = PROCESSOR_ID */
        "ldrb   r1, [r1]             \n"
        "mov    r2, #0               \n" /* cl->myl[core] = 0 */
        "strb   r2, [%[cl], r1, lsr #7] \n"
        :
        : [id] "i" (&PROCESSOR_ID), [cl] "r" (cl)
        : "r1","r2"
    );
}
