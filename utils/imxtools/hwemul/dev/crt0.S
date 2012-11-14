.section    .text,"ax",%progbits
.code       32
.align      0x04
.global     start
start:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */
    ldr     sp, =oc_stackend
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
