/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Jens Arnold
 * based on crt0.S by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
    .section  .start,"ax",@progbits
    .global    _start
_start:
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

    /* call the mainline */
    mov.l   .main_k,r0
    mov.l   .stackend_k,r15
    jmp     @r0
    nop

    .align  2
.iramcopy_k:
    .long   _iramcopy
.iram_k:
    .long   _iramstart
.iramend_k:
    .long   _iramend
.stackend_k:
    .long   _stackend
.main_k:
    .long   _main
