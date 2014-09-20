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
    /* setup abort stack */
    msr     cpsr_c, #0xd7
    ldr     sp, =oc_abortstackend
    /* Switch to sys mode */
    msr     cpsr_c, #0xdf
    /* jump to C code */
    ldr     sp, =oc_stackend
    b       main

/* Handling of data abort:
 * the code can register a "longjmp" buffer to restore the context in case of
 * fault, otherwise the stub will be restarted */
.data
data_abort_jmp_ctx_ptr:
/* buffer contains in order: cpsr,r4-r11,sp,lr,pc */
.skip   48 /* = 4 * (cpsr +  11 registers) */

.text
/* Prototype: int set_data_abort_jmp()
 * Return: 1 in case of data abort, 0 otherwise */
.global set_data_abort_jmp
set_data_abort_jmp:
    mrs     r2, cpsr
    ldr     r1, =data_abort_jmp_ctx_ptr
    mov     r0, #0
    stmia   r1, {r2,r4-r11,sp,lr,pc} /* see PC note below */
    bx      lr
    mov     r0, #1 /* <-- PC points here in stmia */
    bx      lr

data_abort_handler:
    /* to restore lr_sys, we need to switch to sys mode temporarily */
    msr     cpsr_c, #0xdf
    /* restore everything from context */
    ldr     r1, =data_abort_jmp_ctx_ptr
    ldmia   r1!, {r2,r4-r11,sp,lr}
    ldmia   r1, {r0}
    /* return to abort mode */
    msr     cpsr_c, #0xd7
    /* will restore cpsr using spsr */
    msr     spsr, r2
    /* return from exception to store data abort jump point */
    subs    pc, r0, #0

