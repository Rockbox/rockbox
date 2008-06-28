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

    .align      2  /* this aligns to 2^2=4 byte bounday */
    .global     _descramble
    .type       _descramble,@function

/* Descramble a block of byte data, from source to dest, processing len
 * bytes. Size only limited by the len argument. Note that len must
 * be an even multiple of 4 (something rolo_load() already assumes.
 * (Does the Archos firmware loader also require that?).
 *
 * Returns the 16-bit "sum" checksum of the descrambled data.
 *
 * Arguments:
 *   r4 - source (unsigned char*)
 *   r5 - dest   (unsigned char*)
 *   r6 - len    (unsigned int)
 *
 * Register usage:
 *   r0 - data
 *   r1 - temp
 *   r2 - checksum
 *   r3 - current src address
 *   r4 - source
 *   r5 - dest
 *   r6 - len -> source_end
 *   r7 - dest_end
 *   r8 - len / 4
 */

_descramble:
    mov.l   r8,@-r15
    mov     r6,r8
    shlr2   r8              /* r8 = len / 4 */
    mov     r5,r7
    add     r6,r7           /* dest_end = dest + len */
    add     r4,r6           /* source_end = source + len */
    mov     r4,r3           /* addr = source */
    mov     #0,r2           /* checksum = 0 */

.loop:
    mov.b   @r3,r0          /* data = *addr */
    add     r8,r3           /* addr += len / 4 */
    extu.b  r0,r0           /* zero extend data byte */
    swap.b  r0,r1           /* byte swap low word to temp */
    or      r1,r0           /* r0's two lower bytes now identical */
    shlr    r0              /* -> this equals "rotr.b r0" now */
    not     r0,r0           /* negate */
    extu.b  r0,r0           /* zero extend low byte (only needed for sum) */
    mov.b   r0,@r5          /* *dest = data */
    add     r0,r2           /* checksum += data */
    add     #1,r5           /* dest++ */
    cmp/hi  r3,r6           /* addr < source_end ? */
    bt      .loop

    add     #1,r4           /* source++ */
    mov     r4,r3           /* addr = source */
    cmp/hi  r5,r7           /* dest < dest_end */
    bt      .loop

/* 15 clock cycles if no reset of source address, 19 if reset,
 * avg. 16 cycles per byte. Magnus' Version needed 17-22 cycles per byte
 */

    mov.l   @r15+,r8
    rts
    extu.w  r2,r0


/* Move len bytes from source to dest (which must be suitably aligned for
 * long moves) and jump to dest + 0x200.
 *
 * Arguments:
 *   r4 - source
 *   r5 - dest
 *   r6 - len
 */

    .align      2
    .global     _rolo_restart
    .type       _rolo_restart,@function

_rolo_restart:
    mov     r5,r0
    sub     r4,r0           /* r0 = dest - source */
    add     #-4,r0          /* adjust for early increment */
    add     r4,r6           /* r6 = source + len */

.copy:                      /* loop takes 6 cycles per longword */
    mov.l   @r4+,r1
    cmp/hi  r4,r6
    mov.l   r1,@(r0,r4)
    bt      .copy

    mov.l   @r5+,r0         /* start address from image */
    jmp     @r0
    mov.l   @r5+,r15        /* stack pointer from image */

.end:
    .size   _descramble,.end-_descramble
