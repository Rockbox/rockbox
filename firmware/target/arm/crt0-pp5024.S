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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"

#define THUMB_MODE(_r_, _l_)                     \
        ldr     _r_,=_l_ ## f+1                 ;\
        bx      _r_                             ;\
        .pool                                   ;\
        .code   16                              ;\
        .thumb_func                             ;\
 _l_:

// Switch to ARM mode
#define ARM_MODE(_r_, _l_)                       \
        ldr     _r_,=_l_ ## f                   ;\
        bx      _r_                             ;\
        .pool                                   ;\
        .code   32                              ;\
 _l_:


.equ    MODE_MASK, 0x1f 
.equ    T_BIT, 0x20 
.equ    F_BIT, 0x40 
.equ    I_BIT, 0x80 
.equ    MODE_IRQ, 0x12 
.equ    MODE_SVC, 0x13 
.equ    MODE_SYS, 0x1f 



    .section .init.text,"ax",%progbits

    .global    start
start:

    .equ    PROC_ID,  0x60000000
    .equ    COP_CTRL, 0x60007004
    .equ    COP_STATUS, 0x60007004
    .equ    IIS_CONFIG, 0x70002800
    .equ    SLEEP,    0x80000000
    .equ    WAKE,     0x0
    .equ    SLEEPING, 0x80000000


      /* Find out which processor we are */
    mov    r0, #PROC_ID
    ldr    r0, [r0]
    and    r0, r0, #0xff
    cmp    r0, #0x55
    beq    1f

    /* put us (co-processor) to sleep */
    ldr    r4, =COP_CTRL
    mov    r3, #SLEEP
    str    r3, [r4]
    ldr    pc, =cop_wake_start

cop_wake_start:
    /* jump the COP to startup */
    ldr    r0, =startup_loc
    ldr    pc, [r0]

1:
    msr    cpsr_c, #0xd3
            
    ldr     sp, =0x40017f00          /* set stack */

    
    /* execute the loader - this will load an image to 0x10000000 */
    bl     main

startup_loc:
    .word    0x0

