//
// startup code
//
//

#define PSR_MODE	0x0000001f
#define PSR_USR_MODE	0x00000010
#define PSR_IRQ_MODE	0x00000012
#define PSR_SVC_MODE	0x00000013

#define PSR_INT_MASK	0x000000c0
#define PSR_FIQ_DIS	0x00000040
#define PSR_IRQ_DIS	0x00000080

.section .init.text,"ax",%progbits
.global start
.extern _interrupt_disable

// -----------------------------------------------------
//  startup code (setup stacks, branch to main)
// -----------------------------------------------------
start:
	// setup IRQ stack
	mov		r0, #(PSR_IRQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
	msr		cpsr, r0
	ldr		sp,=irqstackend

	// setup SVC stack
	mov		r0, #(PSR_SVC_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
	msr		cpsr, r0
	ldr		sp,=stackend

	// disbale interrupts
        mrs             r0, cpsr
        orr             r0, r0, #0xc0
        msr             cpsr_c, r0

        // remap
        mov             r0, #0x18000000
        add             r0, r0, #0x1C000
        ldr             r1,=0xdeadbeef
        str             r1, [r0, #4]

        // relocate itself
        ldr		r0,=_relocstart
        ldr		r1,=_relocend
	ldr		r2,=0x0
1:
	cmp		r1,r0
        ldrhi		r3,[r0],#4
	strhi		r3,[r2],#4
	bhi		1b
	
	// continue running in SVC (supervisor mode)
	ldr		pc,=0x0
