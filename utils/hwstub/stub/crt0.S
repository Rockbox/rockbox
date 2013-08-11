.section    .text,"ax",%progbits
.code       32
.align      0x04
.global     start
start:
    sub     r7, pc, #8 /* Copy running address */
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */
    /* Disable MMU, disable caching and buffering;
     * use low exception range address */
    mrc     p15, 0, r0, c1, c0, 0
    ldr     r1, =0x3005
    bic     r0, r1
    mcr     p15, 0, r0, c1, c0, 0
    ldr     sp, =oc_stackend
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
    mcr     p15, 0, r2, c7, c5, 0   @ Invalidate ICache
    /* Jump to real location */
    ldr     pc, =remap
remap:
    /* clear bss */
    ldr     r2, =bss_start
    ldr     r3, =bss_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b
    /* jump to C code */
    b       main
