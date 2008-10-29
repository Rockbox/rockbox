/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * based on  arm_nrv2e_d8.S -- ARM decompressor for NRV2E
 * Copyright (C) 1996-2008 Markus Franz Xaver Johannes Oberhumer
 * Copyright (C) 1996-2008 Laszlo Molnar
 * Copyright (C) 2000-2008 John F. Reiser
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

#define src  r4
#define dst  r5
#define len  r6  /* overlaps 'cnt' */
#define cnt  r6  /* overlaps 'len' while reading an offset */
#define tmp  r7

#define off  r0  /* must be r0 because of indexed addressing */
#define bits r1
#define bitmask r2
#define wrnk r3  /* -0x500  -M2_MAX_OFFSET before "wrinkle" */


#define GETBIT             \
    tst     bits, bitmask; \
    bf      1f;            \
    bsr     get1_n2e;      \
1:                         \
    shll    bits  /* using the delay slot on purpose */

#define getnextb(reg) GETBIT; rotcl   reg
#define   jnextb0     GETBIT; bf
#define   jnextb1     GETBIT; bt

    .section  .icode,"ax",@progbits
    .align  2
    .global _ucl_nrv2e_decompress_8
    .type   _ucl_nrv2e_decompress_8,@function

/* src_len = ucl_nrv2e_decompress_8(const unsigned char *src,
 *                                  unsigned char *dst,
 *                                  unsigned long *dst_len)
 */

_ucl_nrv2e_decompress_8:
    sts.l   pr, @-r15
    mov     #-1, off     ! off = -1 initial condition
    mov.l   r6, @-r15
    mov     #-5, wrnk
    mov.l   r5, @-r15
    shll8   wrnk         ! nrv2e -M2_MAX_OFFSET
    mov.l   r4, @-r15
    mov     #-1, bitmask
    shlr    bitmask      ! 0x7fffffff for testing before shifting
    bra     top_n2e
    not     bitmask, bits ! refill next time (MSB must be set)

eof_n2e:
    mov.l   @r15+, r0    ! r0 = orig_src
    mov.l   @r15+, r1    ! r1 = orig_dst
    sub     r0, src
    mov.l   @r15+, r2    ! r2 = plen_dst
    sub     r1, dst
    mov.l   dst, @r2
    lds.l   @r15+, pr
    rts
    mov     src, r0

lit_n2e:
    mov.b   @src, tmp
    add     #1, src     ! Need to fill the pipeline latency anyway
    mov.b   tmp, @dst
    add     #1, dst
top_n2e:
    jnextb1 lit_n2e
    bra     getoff_n2e
    mov     #1, cnt

off_n2e:
    add     #-1, cnt
    getnextb(cnt)
getoff_n2e:
    getnextb(cnt)
    jnextb0 off_n2e

    mov     cnt, tmp
    mov     #0, len     ! cnt and len share a reg!
    add     #-3, tmp
    cmp/pz  tmp
    bf      offprev_n2e ! cnt was 2
    mov.b   @src+, off  ! low 7+1 bits
    shll8   tmp
    extu.b  off, off
    or      tmp, off
    not     off, off    ! off = ~off
    tst     off, off
    bt      eof_n2e
    shar    off
    bt      lenlast_n2e
    bra     lenmore_n2e
    mov     #1, len

offprev_n2e:
    jnextb1 lenlast_n2e
    mov     #1, len
lenmore_n2e:
    jnextb1 lenlast_n2e
len_n2e:
    getnextb(len)
    jnextb0 len_n2e
    bra     gotlen_n2e
    add     #6-2, len

get1_n2e:               ! in: T bit set
    mov.b   @src+, bits ! SH1 sign-extends on load
    rotcl   bits        ! LSB = T, T = MSB
    shll16  bits
    rts
    shll8   bits

lenlast_n2e:
    getnextb(len)       ! 0,1,2,3
    add     #2, len
gotlen_n2e:
    cmp/gt  off, wrnk
    movt    tmp          ! too far away, so minimum match length is 3
    add     tmp, len
copy_n2e:
    mov.b   @(off,dst), tmp
    add     #-1, len
    mov.b   tmp, @dst
    add     #1, dst
    tst     len, len
    bf      copy_n2e
    bra     top_n2e
    nop

    .size ucl_nrv2e_decompress_8, .-ucl_nrv2e_decompress_8
