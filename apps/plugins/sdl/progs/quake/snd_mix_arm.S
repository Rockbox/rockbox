/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

        /** Sound mixing code for ARM. **/
        /* Takes 8-bit mono audio and outputs stereo 16-bit samples.
         * stereo volumes are passed as arguments.
         *
         * Bear with me. This is my first ARM assembly, ever.
        */

        .text
        .align  2
        .global SND_PaintChannelFrom8
        .type   SND_PaintChannelFrom8, %function

#if defined(__ARM_ARCH_5TEJ__)
SND_PaintChannelFrom8:
        // r0: int true_lvol
        // r1: int true_rvol
        // r2: char *sfx
        // r3: int count

        stmfd sp!, {r4, r5, r6, r7, r8, sl}

        ldr ip, =paintbuffer
        ldr ip, [ip]

        mov r0, r0, asl #16 // pre-scale both volumes by 2^16
        mov r1, r1, asl #16

        sub r3, r3, #1 // we'll count backwards
        // sl = 0xffff0000
        ldrh sl, =0xffff

.loop:
        ldrsb r4, [r2, r3] // load *sfx[i] -> r4

        // keep endianness in mind here
        // buffer looks like [left_0, left_1, right_0, right_1] in memory
        // but it is loaded as [right1, right0, left1, left0] to registers
        ldr r8, [ip, r3, lsl #2] // load paintbuffer[0:1] = RIGHTCHANNEL:LEFTCHANNEL

        // handle high half (right channel) first
        mul r5, r4, r1 // SCALEDRIGHT = SFXI * (true_rvol << 16) -- bottom half is zero

        // r7 holds right channel in high half (dirty bottom half)
        qadd r7, r5, r8 // RIGHTCHANORIG = SCALEDRIGHT + RIGHTCHANORIG (high half)

        bic r7, r7, sl // zero bottom bits of r7

        // trash r5, r6 and handle left channel
        mul r5, r4, r0 // SCALEDLEFT = SFXI * (true_rvol << 16)

        mov r8, r8, lsl #16 // extract original left channel from paintbuffer

        // r8 holds left channel in high half with zero bottom half
        qadd r8, r5, r8

        // combine the two 16-bit samples in r7 as 32-bit [left:right]
        // (use lsr to not sign-extend the lower half)
        orr r7, r7, r8, lsr #16

        str r7, [ip, r3, lsl #2] // write 32-bit to paintbuffer
        subs r3, r3, #1
        bgt .loop // must use instead of bne because of the corner case count=1



        ldmfd sp!, {r4, r5, r6, r7, r8, sl}

        bx lr

#elif defined(__ARM_ARCH_6__) // ARMv6 with QADD16 (disabled)
SND_PaintChannelFrom8:
        // r0: int true_lvol
        // r1: int true_rvol
        // r2: char *sfx
        // r3: int count

        stmfd sp!, {r4, r5, r6, r7}

        ldr ip, =paintbuffer
        ldr ip, [ip]
        sub r3, r3, #1 // we'll count backwards
.loop:
        ldrsb r4, [r2, r3] // load *sfx[i] -> r4

        // keep endianness in mind here
        // buffer looks like [left_0, left_1, right_0, right_1] in memory
        // but it is loaded as [right1, right0, left1, left0] to registers
        ldr r7, [ip, r3, lsl #2] // load paintbuffer[0:1] = RIGHTCHANNEL:LEFTCHANNEL

        // handle high half (right channel) first
        mul r5, r4, r1 // SCALEDRIGHT = SFXI * true_rvol
        mul r6, r4, r0 // SCALEDLEFT = SFXI * true_rvol

        orr r6, r6, r5, lsl #16

        qadd16 r6, r6, r7

        str r6, [ip, r3, lsl #2] // write 32-bit to paintbuffer

        subs r3, r3, #1
        bne .loop

        ldmfd sp!, {r4, r5, r6, r7}

        bx lr
#else
#error ARMv5/6 only
#endif
