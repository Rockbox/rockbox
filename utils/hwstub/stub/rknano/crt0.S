.section    .text.start,"ax",%progbits
.code       16
.syntax     unified
.thumb
.thumb_func
.align      0x02
.global     start
start:
vector_table:
    /* stack address */
    .word   0x20010000
    /* vectors */
    .word   reset
    .word   hang /* NMI */
    .word   hang /* Hard Fault */
    .word   hang /* MPU Fault */
    .word   hang /* Bus Fault */
    .word   hang /* Usage Fault */
    .word   hang /* Reserved */
    .word   hang /* Reserved */
    .word   hang /* Reserved */
    .word   hang /* Reserved */
    .word   hang /* SVC */
    .word   hang /* Debug Monitor */
    .word   hang /* Reserved */
    .word   hang /* PSV */
    .word   hang /* SysTick */
    .irp    i,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18
    .word   EXTINT\i
    .endr
/* actual start */
.thumb_func
reset:
    /* make sure interrupts are disabled */
    cpsid   i
    /* change vector table */
    ldr     r0, =start
    ldr     r1, =0xe000ed08 /* VTOR */
    str     r0, [r1]
    /* clear bss */
    ldr     r2, =bss_start
    ldr     r3, =bss_end
    mov     r4, #0
1:
    cmp     r3, r2
    itt     hi
    strhi   r4, [r2], #4
    bhi     1b
    /* jump to main */
    b       main
/* hang */
.thumb_func
hang:
    b       hang
