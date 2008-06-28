/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
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

    .section    .icode,"ax",@progbits
    .align      2
    .global     _bitswap
    .type       _bitswap,@function

/* Flips the bits of all bytes in a memory area (required for mp3 data on
 * the Archos). This version is optimized for speed and size.
 *
 * arguments:
 *  r4 - start address
 *  r5 - length
 *
 * return value: void
 *
 * register usage:
 *  r0 - temporary
 *  r1 - bit mask for rounding to long bound / low byte (after swap)
 *  r2 - high byte (after swap) / combined result
 *  r4 - data address - 4
 *  r5 - end address - 4
 *  r7 - flip table (addressing with signed offset)
 *
 * The instruction order below is devised in a way to utilize the pipelining
 * of the SH1 to the max.
 */

_bitswap:
    mova    _fliptable,r0
    mov     r0,r7
    add     #-4,r4          /* address is shifted by 4 */
    add     r4,r5           /* r5 = end_address - 4 */
    cmp/hi  r4,r5           /* at least something to do? */
    bf      .exit           /* no, get out of here! */

    add     #-3,r5          /* end offset for flipping 4 bytes per pass */
    mov     r4,r0
    tst     #1,r0           /* even address? */
    bt      .start2_w       /* yes, jump into main loop */

    /* no, flip first byte */
    mov.b   @(4,r4),r0      /* load byte, sign extension! */
    add     #1,r4           /* early increment */
    mov.b   @(r0,r7),r0     /* fliptable offset is signed */
    bra     .start2_w       /* jump into main loop */
    mov.b   r0,@(3,r4)      /* store byte */

    /* main loop: flips 2 words per pass */
    .align  2
.loop2_w:
    mov.w   @(6,r4),r0      /* load second word */
    add     #4,r4           /* early increment */
    swap.b  r0,r2           /* get high byte (2nd word) */
    exts.b  r0,r0           /* prepare low byte (2nd word) */
    mov.b   @(r0,r7),r1     /* swap low byte (2nd word) */
    exts.b  r2,r0           /* prepare high byte (2nd word) */
    mov.b   @(r0,r7),r2     /* swap high byte (2nd word) */
    extu.b  r1,r0           /* zero extend low byte (2nd word) */
    mov.w   @r4,r1          /* load first word */
    shll8   r2              /* shift high byte (2nd word), low byte zeroed */
    or      r2,r0           /* put low byte (2nd word) in result */
    swap.b  r1,r2           /* get high byte (1st word) */
    mov.w   r0,@(2,r4)      /* store result (2nd word) */
    exts.b  r1,r0           /* prepare low byte (1st word) */
    mov.b   @(r0,r7),r1     /* swap low byte (1st word) */
    exts.b  r2,r0           /* prepare high byte (1st word) */
    mov.b   @(r0,r7),r2     /* swap high byte (1st word) */
    extu.b  r1,r0           /* zero extend low byte (1st word) */
    shll8   r2              /* shift high byte (1st word), low byte zeroed */
    or      r2,r0           /* put low byte (1st word) in result */
    mov.w   r0,@r4          /* store result (1st word) */
.start2_w:
    cmp/hi  r4,r5           /* runs r4 up to last long bound */
    bt      .loop2_w

    bra     .start_b2       /* jump into trailing byte loop */
    add     #3,r5           /* reset end offset */

    /* trailing byte loop: flips 0..3 bytes */
.loop_b2:
    mov.b   @(4,r4),r0      /* loand byte, sign extension! */
    add     #1,r4           /* early increment */
    mov.b   @(r0,r7),r0     /* fliptable offset is signed */
    mov.b   r0,@(3,r4)      /* store byte */
.start_b2:
    cmp/hi  r4,r5           /* runs r4 up to end address */
    bt      .loop_b2

.exit:
    rts
    nop

    .align  2
    .global _fliptable

    .byte   0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1
    .byte   0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1
    .byte   0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9
    .byte   0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9
    .byte   0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5
    .byte   0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5
    .byte   0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed
    .byte   0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd
    .byte   0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3
    .byte   0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3
    .byte   0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb
    .byte   0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb
    .byte   0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7
    .byte   0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7
    .byte   0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef
    .byte   0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
_fliptable:
    .byte   0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0
    .byte   0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0
    .byte   0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8
    .byte   0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8
    .byte   0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4
    .byte   0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4
    .byte   0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec
    .byte   0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc
    .byte   0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2
    .byte   0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2
    .byte   0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea
    .byte   0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa
    .byte   0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6
    .byte   0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6
    .byte   0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee
    .byte   0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe


.end:
    .size   _bitswap,.end-_bitswap
