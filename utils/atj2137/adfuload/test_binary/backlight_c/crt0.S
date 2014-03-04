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
    addiu t0, 4                # inc dst addr
    bne t0, t1, reloc_loop
    sw t2, -4(t0)              # dst

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
    addiu   t0, t0, 0x10
    bne     t0, t1, cache_init_loop
    nop

    # clear bss
    la t0, bssbegin
    la t1, bssend

clear_bss_loop:
    addiu t0, 4
    bne t0, t1, clear_bss_loop
    sw zero, -4(t0)

    # setup stack
    la sp, stackend
    la t0, stackbegin
    li t1, 0xdeadbeef

stack_munge_loop:
    addiu t0, 4
    bne t0, sp, stack_munge_loop
    sw t1, -4(t0)

    # jump to C code
    j main
    nop

    .set at
    .set reorder
