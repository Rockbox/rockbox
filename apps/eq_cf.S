/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Thom Johansen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

    .text
    .global eq_filter
eq_filter:
    lea.l (-11*4, %sp), %sp 
    movem.l %d2-%d7/%a2-%a6, (%sp)    | save clobbered regs
    move.l (11*4+8, %sp), %a5         | fetch filter structure address
    movem.l (11*4+16, %sp), %d6-%d7   | load num. channels and shift count
    movem.l (%a5), %a0-%a4            | load coefs
    lea.l (5*4, %a5), %a5             | point to filter history
    moveq.l #2, %d6                   | number of channels (hardcode to stereo)

.filterloop:
    move.l (11*4+4, %sp), %a6         | load input channel pointer
    move.l (%a6), %a6
    move.l (11*4+12, %sp), %d5        | number of samples
    addq.l #4, (11*4+4, %sp)          | point x to next channel
    movem.l (%a5), %d0-%d3            | load filter history
.loop:
    move.l (%a6), %d4
    mac.l %a0, %d4, %acc0               | acc = b0*x[i]
    mac.l %a1, %d0, %acc0               | acc += b1*x[i - 1]
    mac.l %a2, %d1, %acc0               | acc += b2*x[i - 2]
    msac.l %a3, %d2, %acc0              | acc -= a1*y[i - 1]
    msac.l %a4, %d3, %acc0              | acc -= a2*y[i - 2]
    move.l %d0, %d1                     | fix history
    move.l %d4, %d0
    move.l %d2, %d3
    movclr.l %acc0, %d2                 | fetch and write result
    asl.l %d7, %d2                      | restore fixed point format
    move.l %d2, (%a6)+                  | save result
    subq.l #1, %d5                      | are we done with this channel?
    jne .loop
    
    movem.l %d0-%d3, (%a5)              | save history back to struct
    lea.l (4*4, %a5), %a5               | point to next channel's history
    subq.l #1, %d6                      | have we processed both channels?
    jne .filterloop

    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (11*4, %sp), %sp
    rts

