!***************************************************************************
!             __________               __   ___.
!   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
!   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
!   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
!   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
!                     \/            \/     \/    \/            \/
! $Id$
!
! Copyright (C) 2002 by Linus Nielsen Feltzing
!
! All files in this archive are subject to the GNU General Public License.
! See the file COPYING in the source tree root for full license agreement.
!
! This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
! KIND, either express or implied.
!
!***************************************************************************
! note: sh-1 has a "delay cycle" after every branch where you can
! execute another instruction "for free".
	
	.file	"start.s"
	.section	.text
	.extern	_INIT
	.extern _vectable
	.extern _stack
	.global _start
	.align  2

_start:
	mov.l	1f, r1
	mov.l	3f, r3
	mov.l	2f, r15
	jmp	@r3
	ldc	r1, vbr
	nop

1:	.long	_vectable
2:	.long	_stack
3:	.long	_INIT
	.type		_start,@function					
