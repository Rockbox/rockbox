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
	.section .text
	.global	start
start:
	/* We begin with some tricks. If we have built our code to be loaded
	 * via the standalone GDB stub, we will have out VBR at some other
	 * location than 0x9000000. We must copy the trap vectors for the
	 * GDB stub to our vector table.
	 * If, on the other hand, we are running standalone we will have
	 * the VBR at 0x9000000, and the copy will not do any harm.
	 */
	mov.l	vbr_k,r1	
	mov.l	orig_vbr_k,r2
	
	/* Move the serial Rx interrupt vector */
	mov	#0x69,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)

	/* Move the breakpoint trap vector as well (32) */
	mov	#0x20,r0
	shll2	r0
	mov.l	@(r0,r2),r3
	mov.l	r3,@(r0,r1)
	
	/* Move the single step trap vector as well (127) */
	mov	#0x7f,r0
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
	cmp/ge	r0,r1
	bt	start_l
	nop

	! call the mainline	
	mov.l	main_k,r0
	jsr	@r0
	nop
.hoo:
    bra .hoo

	.align 2
stack_k:
	.long	_stack	
edata_k:
	.long	_edata
end_k:
	.long	_end
main_k:
	.long	_main
vbr_k:
	.long	vectors
orig_vbr_k:
	.long 0x9000000

#ifdef __ELF__
	.section .stack,"aw"
#else
	.section .stack
#endif

	.section .vectors
vectors:
	.long start
	.long _stack
	.long start
	.long _stack
