/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 by Magnus Holmgren
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
 */

/* Register usage:
 * i        - r0
 * i4       - r1
 * checksum - r2
 * addr     - r3
 * source   - r4
 * dest     - r5
 * len      - r6
 * len4     - r7
 * data     - r8
 * temp     - r9
 */

_descramble:
    mov.l   r8,@-r15
    mov.l   r9,@-r15
    mov     #0,r0           /* i = 0 */
    mov     #0,r1           /* i4 = i / 4 */
    mov     #0,r2           /* checksum = 0 */
    mov     r4,r3           /* addr = source */
    mov     r6,r7
    shlr2   r7              /* len4 = len / 4 */

.loop:
    mov.b   @r3,r8          /* data = source[addr] */
    add     r7,r3           /* addr += len4 */
    extu.b  r8,r8           /* we want the data extended unsigned */
    shlr    r8              /* start rotate right of low byte */
    movt    r9              /* get bit 0 that was shifted out */
    shll8   r9
    shlr    r9              /* move it to bit 7 */
    or      r9,r8           /* finish rotate right */
    not     r8,r8
    extu.b  r8,r8
    mov.b   r8,@(r0,r5)     /* dest[i] = data */
    add     r8,r2           /* checksum += data[i] */
    add     #1,r0           /* i++ */
    tst     #3,r0           /* reset addr? */
    bf      .loop

    add     #1,r1           /* i4++ */
    mov     r4,r3
    add     r1,r3           /* addr = source + i4 */
    cmp/hs  r6,r0           /* all done? */
    bf      .loop
    
    /* 17 cycles if no "reset," 22 if reset => average 18.25 cycles per
     * byte, assuming no wait states from reads or writes. "Old" algorithm
     * needed 24-26 cycles per byte, under the same assumptions.
     */

    mov.l   @r15+,r9
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

    .global     _rolo_restart
    .type       _rolo_restart,@function

_rolo_restart:
    mov.w   .offset,r0
    mov     r5,r7
    add     r0,r7                       /* start_func() */
    mov     r6,r0
    shlr2   r0
    add     #1,r0
.copy:
    mov.l   @r4+,r1
    add     #-1,r0
    mov.l   r1,@r5
    add     #4,r5
    cmp/eq  #0,r0
    bf      .copy

    jmp     @r7
    
    nop

.offset:
    .word   0x200

.end:
    .size   _descramble,.end-_descramble
