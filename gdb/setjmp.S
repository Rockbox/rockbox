/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Red Hat Inc
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
        .type   _setjmp,@function
	.global	_setjmp
_setjmp: 
	add	#(9*4),r4

	sts.l	pr,@-r4

	mov.l	r15,@-r4
	mov.l	r14,@-r4
	mov.l	r13,@-r4
	mov.l	r12,@-r4

	mov.l	r11,@-r4
	mov.l	r10,@-r4
	mov.l	r9,@-r4
	mov.l	r8,@-r4

	rts
	mov    #0,r0

        .type   _longjmp,@function
        .global _longjmp
_longjmp:  
	mov.l	@r4+,r8
	mov.l	@r4+,r9
	mov.l	@r4+,r10
	mov.l	@r4+,r11

	mov.l	@r4+,r12
	mov.l	@r4+,r13
	mov.l	@r4+,r14
	mov.l	@r4+,r15

	lds.l	@r4+,pr

	mov	r5,r0
	tst	r0,r0
	bf	retr4
	movt	r0
retr4:	rts
	nop
