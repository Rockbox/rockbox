/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * 
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/****************************************************************************
 * void memswap128(void *buf1, void *buf2, size_t len)
 */
    .section    .icode, "ax", %progbits
    .align      2
    .global     memswap128
    .type       memswap128, %function
memswap128:
                                        @ r0 = buf1
                                        @ r1 = buf2
                                        @ r2 = len
    movs        r2, r2, lsr #4          @ bytes => lines, len == 0?
    moveq       pc, lr                  @ not at least a line? leave
    stmdb       sp!, { r4-r10, lr }     @ save registers and return address
.loop:                                  @
    ldmia       r0, { r3-r6 }           @ read four longwords from buf1
    ldmia       r1, { r7-r10 }          @ read four longwords from buf2
    stmia       r0!, { r7-r10 }         @ write buf2 data to buf1, buf1 += 16
    stmia       r1!, { r3-r6 }          @ write buf1 data to buf2, buf2 += 16
    subs        r2, r2, #1              @ len -= 1, len > 0 ?
    bhi         .loop                   @ yes? keep exchanging
    ldmia       sp!, { r4-r10, pc }     @ restore registers and return
.end:
    .size       memswap128, .end-memswap128

