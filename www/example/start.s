! note: sh-1 has a "delay cycle" after every branch where you can
! execute another instruction "for free".
	
	.file	"start.s"
	.section	.text.start
	.extern	_main
	.extern _vectors
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

1:	.long	_vectors
2:	.long	_stack
3:	.long	_main
	.type		_start,@function					
