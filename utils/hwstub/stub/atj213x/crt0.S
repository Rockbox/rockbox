#include "mips.h"

    .extern main
    .global start

    .set mips32r2
    .set noreorder
    .set noat

    .section .init.text,"ax",%progbits

start:
    di                         # disable interrupts
    bltzal zero, load_addr     # ra = PC + 8, branch not taken
    nop

load_addr:
    addiu v0, ra, -12          # calc real load address
                               # account for branch delay slot
                               # and very first 'di' instruction
    la t0, relocstart
    la t1, relocend
    beq t0, v0, entry_point    # no relocation needed
    nop

reloc_loop:
    lw t2, 0(v0)               # src
    addiu v0, 4                # inc src addr
    sw t2, 0(t0)               # dst
    bne t0, t1, reloc_loop
    addiu t0, 4                # inc dst addr

entry_point_jump:
    la t0, entry_point
    jr t0
    nop

entry_point:
    # setup caches
    # 4-way, 256 sets, 16 bytes cacheline I/D
    li      t0, 3              # enable cache for kseg0 accesses
    mtc0    t0, C0_CONFIG

    la      t0, 0x80000000     # an idx op should use an unmappable address
    ori     t1, t0, 0x4000     # 16kB cache
    mtc0    zero, C0_TAGLO
    mtc0    zero, C0_TAGHI

cache_init_loop:
    cache   8, 0(t0)           # index store icache tag
    cache   9, 0(t0)           # index store dcache tag
    bne     t0, t1, cache_init_loop
    addiu   t0, t0, 0x10

intc_setup:
    li      t0, 0xb0020000     # INTC base
    lw      zero, 4(t0)        # INTC_MSK mask all interrupt sources

core_irq_setup:
    li      t0, 0x00404000     # BEV=1 for C0_EBASE setup, IM6=1, IE=0
    mtc0    t0, C0_STATUS

    la      t0, _irqbase       # vectors base address must be 4k aligned
    mtc0    t0, C0_EBASE

    li      t0, 0x00004000
    mtc0    t0, C0_STATUS      # BEV=0, IM6=1, IE=0

    li      t1, 0x08800000
    mtc0    t1, C0_CAUSE       # DC=1, IV=1
    mtc0    zero,C0_INTCTL     # VS = 0

    # clear bss
    la t0, bssbegin
    la t1, bssend

clear_bss_loop:
    sw zero, 0(t0)
    bne t0, t1, clear_bss_loop
    addiu t0, 4

    # setup stack
    la k0, irqstackend
    la sp, stackend
    la t0, stackbegin
    li t1, 0xdeadbeef

stack_munge_loop:
    sw t1, 0(t0)
    bne t0, sp, stack_munge_loop
    addiu t0, 4

    # jump to C code with enabled interrupts
    la t0, main
    jr t0
    ei

    .set at
    .set reorder

/* s0-s7 not saved as this are callee saved registers
 * CO_STATUS is not saved as nested interrupts are not supported
 *
 * Separate irqstack is used for context save and irq processing
 * k0 holds the address of the top of this stack and k1 is used
 * to hold original sp value. Since we do not support nesting 
 * there is nothing to worry about
 */
    .extern INT_UDC

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
    lui t0, 0xb002    /* INTC base */
    lw  a0, 0(t0)     /* INTC_PD */
    lw  a1, 4(t0)     /* INTC_MSK */
    and a0, a0, a1    /* mask */
    andi a0, a0, 0x10 /* UDC flag */
    beq  a0, zero, restore
    nop
    /* irq dispatch */
    la a0, INT_UDC
    jalr a0
    nop

restore:
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

