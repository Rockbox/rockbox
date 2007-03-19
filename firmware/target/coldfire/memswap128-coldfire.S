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
    .section    .icode, "ax", @progbits
    .align      2
    .global     memswap128
    .type       memswap128, @function
memswap128:
    lea.l       -28(%sp), %sp           | save registers
    movem.l     %d2-%d7/%a2, (%sp)      |
    movem.l 	32(%sp), %a0-%a2        | %a0 = buf1
                                        | %a1 = buf2
                                        | %a2 = len
    lea.l       -15(%a0, %a2.l), %a2    | %a2 = end address - 15
    cmp.l       %a0, %a2                | end address <= buf1?
    bls.b       .no_lines               | not at least a line? leave
.loop:                                  |
    movem.l	    (%a0), %d0-%d3          | read four longwords from buf1
    movem.l	    (%a1), %d4-%d7          | read four longwords from buf2
    movem.l	    %d4-%d7, (%a0)          | write buf2 data to buf1
    movem.l	    %d0-%d3, (%a1)          | write buf1 data to buf2
    lea.l       16(%a0), %a0            | buf1 += 16
    lea.l       16(%a1), %a1            | buf2 += 16
    cmp.l       %a0, %a2                | %a0 < %d0?
    bhi.b       .loop                   | yes? keep exchanging
.no_lines:                              |
    movem.l  	(%sp), %d2-%d7/%a2      | restore registers
    lea.l       28(%sp), %sp            |
    rts                                 |
.end:
	.size       memswap128, .end-memswap128
