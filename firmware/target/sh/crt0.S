/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "config.h"
#include "cpu.h"

    .section .init.text,"ax",@progbits

    .global    start
start:

    mov.l   .vbr_k,r1
#ifdef DEBUG
    /* If we have built our code to be loaded via the standalone GDB
     * stub, we will have out VBR at some other location than 0x9000000.
     * We must copy the trap vectors for the GDB stub to our vector table. */
    mov.l   .orig_vbr_k,r2

    /* Move the invalid instruction vector (4) */
    mov     #4,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the invalid slot vector (6) */
    mov     #6,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the bus error vector (9) */
    mov     #9,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the DMA bus error vector (10) */
    mov     #10,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the NMI vector as well (11) */
    mov     #11,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the UserBreak vector as well (12) */
    mov     #12,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the breakpoint trap vector (32) */
    mov     #32,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the IO trap vector (33) */
    mov     #33,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the serial Rx interrupt vector (105) */
    mov     #105,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the single step trap vector (127) */
    mov     #127,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)
#endif /* DEBUG */
    ldc     r1,vbr

    mov     #0,r0
    ldc     r0,gbr

    /* zero out .ibss */
    mov.l   .iedata_k,r0
    mov.l   .iend_k,r1
    bra     .iedatastart
    mov     #0,r2
.iedataloop: /* backwards is faster and shorter */
    mov.l   r2,@-r1
.iedatastart:
    cmp/hi  r0,r1
    bt      .iedataloop

    /* copy the .iram section */
    mov.l   .iramcopy_k,r0
    mov.l   .iram_k,r1
    mov.l   .iramend_k,r2
    /* Note: We cannot put a PC relative load into the delay slot of a 'bra'
       instruction (the offset would be wrong), but there is nothing else to
       do before the loop, so the delay slot would be 'nop'. The cmp / bf
       sequence is the same length, but more efficient. */
    cmp/hi  r1,r2
    bf      .noiramcopy
.iramloop:
    mov.l   @r0+,r3
    mov.l   r3,@r1
    add     #4,r1
    cmp/hi  r1,r2
    bt      .iramloop
.noiramcopy:

    /* zero out bss */
    mov.l   .edata_k,r0
    mov.l   .end_k,r1
    bra     .edatastart
    mov     #0,r2
.edataloop: /* backwards is faster and shorter */
    mov.l   r2,@-r1
.edatastart:
    cmp/hi  r0,r1
    bt      .edataloop

    /* copy the .data section, for rombased execution */
    mov.l   .datacopy_k,r0
    mov.l   .data_k,r1
    cmp/eq  r0,r1
    bt      .nodatacopy   /* Don't copy if src and dest are equal */
    mov.l   .dataend_k,r2
    cmp/hi  r1,r2
    bf      .nodatacopy
.dataloop:
    mov.l   @r0+,r3
    mov.l   r3,@r1
    add     #4,r1
    cmp/hi  r1,r2
    bt      .dataloop
.nodatacopy:

    /* Munge the main thread stack */
    mov.l   .stackbegin_k,r0
    mov.l   .stackend_k,r1
    mov     r1,r15
    mov.l   .deadbeef_k,r2
.mungeloop: /* backwards is faster and shorter */
    mov.l   r2,@-r1
    cmp/hi  r0,r1
    bt      .mungeloop

    /* call the mainline */
    mov.l   .main_k,r0
    jsr     @r0
    nop
.hoo:
    bra     .hoo
    nop

    .align  2
.vbr_k:
    .long   vectors
#ifdef DEBUG
.orig_vbr_k:
    .long   0x09000000
#endif
.iedata_k:
    .long   _iedata
.iend_k:
    .long   _iend
.iramcopy_k:
    .long   _iramcopy
.iram_k:
    .long   _iramstart
.iramend_k:
    .long   _iramend
.edata_k:
    .long   _edata
.end_k:
    .long   _end
.datacopy_k:
    .long   _datacopy
.data_k:
    .long   _datastart
.dataend_k:
    .long   _dataend
.stackbegin_k:
    .long   _stackbegin
.stackend_k:
    .long   _stackend
.deadbeef_k:
    .long   0xdeadbeef
.main_k:
    .long   _main

    .section .resetvectors
vectors:
    .long   start
    .long   _stackend
    .long   start
    .long   _stackend
