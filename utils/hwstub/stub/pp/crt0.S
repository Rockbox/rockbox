.section    .text,"ax",%progbits
.code       32
.align      0x04
.global     start
start:
    sub     r7, pc, #8 /* Copy running address */
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

    /* Get processor ID */
    ldr     r0, =0x60000000 /* PROC_ID */
    ldrb    r4, [r0]

    /* Halt the COP */
    ldr     r6, =0x60007004 /* COP_CTL */
    cmp     r4, #0x55   /* CPU_ID */
1:
    ldrne   r1, [r6]
    orrne   r1, #0x80000000 /* PROC_SLEEP */
    strne   r1, [r6]
    bne     1b

    /* Wait for the COP to be stopped */
1:
    ldr     r0, [r6]
    tst     r0, #0x80000000 /* PROC_SLEEP */
    beq     1b

    /* Relocate to right address */
    mov    r2, r7
    ldr    r3, =_copystart
    ldr    r4, =_copyend
1:
    cmp     r4, r3
    ldrhi   r5, [r2], #4
    strhi   r5, [r3], #4
    bhi     1b
    mov     r2, #0
    /* FIXME invalid Icache here ? */
    /* Jump to real location */
    ldr     pc, =remap
remap:
    /* NOTE on PP611x, we should make sure the MMU is disabled at this point */
    /* clear bss */
    ldr     r2, =bss_start
    ldr     r3, =bss_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b
    /* jump to C code */
    ldr     sp, =oc_stackend
    b       main
