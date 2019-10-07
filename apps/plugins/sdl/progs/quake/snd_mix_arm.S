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

	/*
	 * Sound mixing code for ARM.
	 *
	 * Takes an array of 8-bit mono audio and outputs stereo 16-bit
	 * samples. Stereo volumes are passed as arguments r0 and r1.
	 *
	 * Bear with me. This is my first ARM assembly, ever.
	 */

        .text
        .align  2
        .global SND_PaintChannelFrom8
        .type   SND_PaintChannelFrom8, %function

#if defined(__ARM_ARCH_5TEJ__)
SND_PaintChannelFrom8:
        ;; r0: int true_lvol
        ;; r1: int true_rvol
        ;; r2: char *sfx
        ;; r3: int count

        stmfd sp!, {r4, r5, r6, r7, r8, sl}

        ldr ip, =paintbuffer
        ldr ip, [ip]

        mov r0, r0, asl #16					; prescale by 2^16
        mov r1, r1, asl #16

        sub r3, r3, #1						; count backwards

        ldrh sl, =0xffff 					; halfword mask

1:
        ldrsb r4, [r2, r3]					; load input sample
        ldr r8, [ip, r3, lsl #2]				; load output sample pair from paintbuffer
								; (left:right in memory -> right:left in register)
        ;; right channel (high half)
        mul r5, r4, r1						; scaledright = sfx[i] * (true_rvol << 16) -- bottom half is zero
        qadd r7, r5, r8						; right = scaledright + right (in high half of word)
        bic r7, r7, sl						; zero bottom half of r7

        ;; left channel (low half)
        mul r5, r4, r0						; scaledleft = sfx[i] * (true_rvol << 16)
        mov r8, r8, lsl #16					; extract original left channel from paintbuffer
        qadd r8, r5, r8						; left = scaledleft + left

        orr r7, r7, r8, lsr #16					; combine right:left in r7
        str r7, [ip, r3, lsl #2]				; write right:left to output buffer
        subs r3, r3, #1

        bgt 1b							; must use bgt instead of bne in case count=1

        ldmfd sp!, {r4, r5, r6, r7, r8, sl}

        bx lr

#elif defined(__ARM_ARCH_6__) ; ARMv6 with QADD16 (disabled)
SND_PaintChannelFrom8:
        ;; r0: int true_lvol
        ;; r1: int true_rvol
        ;; r2: char *sfx
        ;; r3: int count

        stmfd sp!, {r4, r5, r6, r7}

        ldr ip, =paintbuffer
        ldr ip, [ip]						; load paintbuffer address
        sub r3, r3, #1						; we'll count backwards
1:
        ldrsb r4, [r2, r3]					; load sfx[i] -> r4
	ldr r7, [ip, r3, lsl #2]				; load old sample pair

        mul r5, r4, r1 						; SCALEDRIGHT = SFXI * true_rvol
        mul r6, r4, r0						; SCALEDLEFT = SFXI * true_rvol

        orr r6, r6, r5, lsl #16					; combine samples as 32-bit

        qadd16 r6, r6, r7					; parallel 16-bit add

        str r6, [ip, r3, lsl #2]				; write 32-bit to paintbuffer

        subs r3, r3, #1
        bgt 1b

        ldmfd sp!, {r4, r5, r6, r7}

        bx lr
#else
#error ARMv5/6 only
#endif
