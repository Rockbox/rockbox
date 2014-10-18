#include "mips.h"
/* s0-s7 not saved as this are callee saved registers
 * CO_STATUS is not saved as nested interrupts are not supported
 *
 * Separate irqstack is used for context save and irq processing
 * k0 holds the address of the top of this stack and k1 is used
 * to hold original sp value. Since we do not support nesting 
 * there is nothing to warry about
 */
    .extern irqvector

    .global irq_handler
    .set mips32r2
    .set noreorder
    .set noat
    .section .irq_vector,"ax",%progbits

irq_handler:
    move k1, sp
    move sp, k0
    addiu sp, sp, -84

    /* context save */
    sw AT,  0(sp)
    sw v0,  4(sp)
    sw v1,  8(sp)
    sw a0, 12(sp)
    sw a1, 16(sp)
    sw a2, 20(sp)
    sw a3, 24(sp)
    sw t0, 28(sp)
    sw t1, 32(sp)
    sw t2, 36(sp)
    sw t3, 40(sp)
    sw t4, 44(sp)
    sw t5, 48(sp)
    sw t6, 52(sp)
    sw t7, 56(sp)
    sw t8, 60(sp)
    sw t9, 64(sp)
    sw fp, 68(sp)
    sw ra, 72(sp)

    mfhi t0
    mflo t1
    sw t0, 76(sp)
    sw t1, 80(sp)

    /* handle interrupt */
    lui t0, 0xb002   /* INTC base */
    lw  t1, 0(t0)    /* INTC_PD */
    lw  t2 , 4(t0)   /* INTC_MSK */
    and t1, t1, t2   /* mask */
    clz t1, t1
    sll  t0, t1, 2   /* offset */
    la  t1, irqvector
    addu t0, t1, t0  /* irq handler pointer address */
    lw t0, 0(t0)
    jalr t0          /* call handler function */
    nop

    /* context restore */
    lw t0, 76(sp)
    lw t1, 80(sp)
    mthi t0
    mtlo t1

    lw AT,  0(sp)
    lw v0,  4(sp)
    lw v1,  8(sp)
    lw a0, 12(sp)
    lw a1, 16(sp)
    lw a2, 20(sp)
    lw a3, 24(sp)
    lw t0, 28(sp)
    lw t1, 32(sp)
    lw t2, 36(sp)
    lw t3, 40(sp)
    lw t4, 44(sp)
    lw t5, 48(sp)
    lw t6, 52(sp)
    lw t7, 56(sp)
    lw t8, 60(sp)
    lw t9, 64(sp)
    lw fp, 68(sp)
    lw ra, 72(sp)

    addiu sp, sp, 84
    move sp, k1
    eret

    .set reorder
    .set at
