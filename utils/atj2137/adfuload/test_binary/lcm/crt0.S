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
    lui t3, 0xa000             # use KSEG1 uncached unmapped
    la t0, relocstart          #   addresses as we don't know
    or t0, t0, t3              #   the state of caches
    la t1, relocend
    or t1, t1, t3
    beq t0, v0, cache_init     # no relocation needed
    nop

reloc_loop:
    lw t2, 0(v0)               # src
    addiu v0, 4                # inc src addr
    addiu t0, 4                # inc dst addr
    bne t0, t1, reloc_loop
    sw t2, -4(t0)              # dst

cache_init:
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
    addiu   t0, t0, 0x10
    bne     t0, t1, cache_init_loop
    nop

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
    addiu t0, 4
    bne t0, t1, clear_bss_loop
    sw zero, -4(t0)

    # setup stack
    la k0, irqstackend
    la sp, stackend
    la t0, stackbegin
    li t1, 0xdeadbeef

stack_munge_loop:
    addiu t0, 4
    bne t0, sp, stack_munge_loop
    sw t1, -4(t0)

    # jump to C code with enabled interrupts
    la t0, main
    jr t0
    ei

    .set at
    .set reorder
