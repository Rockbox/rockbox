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
	.section .init.text
	.global	start
start:
#ifdef IRIVER_H100
        /* Platform:     iRiver H1xx */
        move.w #0x2700,%sr

        move.l #MBAR+1,%d0
        movec.l %d0,%mbar
        
        move.l #MBAR2+1,%d0
        movec.l %d0,%mbar2

        /* Test code: blink the backlight */
        lea MBAR2,%a0
        move.l #0x00020000,%d0
        move.l %d0,(0xbc,%a0) /* Function = 1 */
        move.l %d0,(0xb8,%a0) /* Enable = 1 */
        
.blinkloop:
        eor.l %d0,(0xb4,%a0)
        
        move.l #2000000,%d2
.delay:  
        subq.l #1,%d2
        bne .delay
        jmp .blinkloop

	.section .resetvectors
vectors:
	.long _stackend
	.long start
#else
        /* Platform:     Archos Jukebox
	 * We begin with some tricks. If we have built our code to be loaded
	 * via the standalone GDB stub, we will have out VBR at some other
	 * location than 0x9000000. We must copy the trap vectors for the
	 * GDB stub to our vector table.
	 * If, on the other hand, we are running standalone we will have
	 * the VBR at 0x9000000, and the copy will not do any harm.
	 */
	mov.l	vbr_k,r1	
	mov.l	orig_vbr_k,r2
	
	/* Move the invalid instruction vector (4) */
	mov	#4,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the invalid slot vector (6) */
	mov	#6,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the bus error vector (9) */
	mov	#9,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the DMA bus error vector (10) */
	mov	#10,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the NMI vector as well (11) */
	mov	#11,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the breakpoint trap vector (32) */
	mov	#32,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the IO trap vector (33) */
	mov	#33,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the serial Rx interrupt vector (105) */
	mov	#105,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)

	/* Move the single step trap vector (127) */
	mov	#127,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	ldc	r1,vbr

	/* Now let's get on with the normal business */
	mov.l	stack_k,r15

	/* zero out bss */
	mov.l	edata_k,r0
	mov.l	end_k,r1
	mov	#0,r2
start_l:
	mov.l	r2,@r0
	add	#4,r0
	cmp/ge	r1,r0
	bf	start_l
	nop

	/* copy the .iram section */
	mov.l	iramcopy_k,r0
	mov.l	iram_k,r1
	mov.l	iramend_k,r2
copy_l:
	mov.l	@r0,r3
	mov.l	r3,@r1
	add	#4,r0
	add	#4,r1
	cmp/ge	r2,r1
	bf	copy_l
	nop

	/* copy the .data section, for rombased execution */
	mov.l	datacopy_k,r0
	mov.l	data_k,r1
	mov.l	dataend_k,r2
        /* Don't copy if src and dest are equal */
        cmp/eq  r0,r1
        bt      nodatacopy
copy_l2:
	mov.l	@r0,r3
	mov.l	r3,@r1
	add	#4,r0
	add	#4,r1
	cmp/ge	r2,r1
	bf	copy_l2
	nop
nodatacopy:
	/* Munge the main thread stack */
	mov.l	stack_k,r2
	mov.l	deadbeef_k,r0
	mov.l	stackbegin_k,r1
munge_loop:
	mov.l	r0,@r1
	add	#4,r1
	cmp/ge	r2,r1
	bf	munge_loop
	nop

	mov     #0,r0
	ldc     r0,gbr
	
	! call the mainline	
	mov.l	main_k,r0
	jsr	@r0
	nop
.hoo:
    bra .hoo

	.align 2
stack_k:
	.long	_stackend	
stackbegin_k:
	.long	_stackbegin
deadbeef_k:
	.long	0xdeadbeef
edata_k:
	.long	_edata
end_k:
	.long	_end
iramcopy_k:
	.long	_iramcopy
iram_k:
	.long	_iramstart
iramend_k:
	.long	_iramend
datacopy_k:
	.long	_datacopy
data_k:
	.long	_datastart
dataend_k:
	.long	_dataend
main_k:
	.long	_main
vbr_k:
	.long	vectors
orig_vbr_k:
	.long 0x9000000

	.section .resetvectors
vectors:
	.long start
	.long _stackend
	.long start
	.long _stackend
#endif
