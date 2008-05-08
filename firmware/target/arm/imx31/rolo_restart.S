/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
 *
 * RoLo restart code for IMX31
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"

/****************************************************************************
 * void rolo_restart(const unsigned char* source, unsigned char* dest,
 *                   int length) __attribute__((noreturn));
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     rolo_restart
rolo_restart:
    adr     r4, restart_copy_start
    adr     r5, restart_copy_end
    ldr     r6, =IRAM_BASE_ADDR
    mov     r7, r6

    @ Copy stub to IRAM  
1:
    ldr     r8, [r4], #4
    str     r8, [r7], #4
    cmp     r5, r4
    bhi     1b

    @ Branch to stub
    bx      r6

restart_copy_start:
    @ Trivial copy of firmware to final location
    mov     r4, r1
1:
    subs    r2, r2, #1
    ldrb    r7, [r0], #1
    strb    r7, [r4], #1
    bge     1b

    @ Clean and invalidate all caches
    mov     r0, #0
    mcr     p15, 0, r0, c7, c14, 0
    mcr     p15, 0, r0, c7, c5, 0
    mcr     p15, 0, r0, c7, c10, 4
    mcr     p15, 0, r0, c7, c5, 4
    
    @ Branch to destination (should be address 0x00000000)
    bx      r1
restart_copy_end:
    .size   rolo_restart,.-rolo_restart
