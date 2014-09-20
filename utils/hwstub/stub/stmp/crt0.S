.section .vectors,"ax",%progbits
.code 32
    /* most handlers are in DRAM which is too far away for a relative jump */
    b   start
    b   start
    b   start
    b   start
    b   data_abort_handler
    b   start
    b   start
    b   start

.section    .text,"ax",%progbits
.code       32
.align      0x04
.global     start
start:
    /* Save running address */
    sub     r7, pc, #8 /* Copy running address */
    /* adjust the offset between start and beginning of the binary */
    ldr     r0, =_copystart
    ldr     r1, =start
    add     r7, r0
    sub     r7, r1
    /* enter supervisor mode, disable IRQ/FIQ */
    msr     cpsr_c, #0xd3
    /* The stub could be located at a virtual address so killing the MMU at
     * this point would be mere suicide. We assume that the remap location
     * is identically mapped and kill the MMU after the copy */

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
    /* Disable MMU, disable caching and buffering;
     * use low exception range address */
    mrc     p15, 0, r0, c1, c0, 0
    ldr     r1, =0x3005
    bic     r0, r1
    mcr     p15, 0, r0, c1, c0, 0
    /* clear bss */
    ldr     r2, =bss_start
    ldr     r3, =bss_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b
    /* NOTE: we don't need an abort stack */
    /* Switch to sys mode */
    msr     cpsr_c, #0xdf
    /* jump to C code */
    ldr     sp, =oc_stackend
    b       main
