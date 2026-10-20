#include "config.h"
#include "mips.h"
#include "system.h"

.extern main
.global start

.set mips32
.set noreorder
.set noat

.section .init.text,"ax",%progbits
/* jztool's "load" runs the Rockbox SPL, which sets up the clocks and the DDR,
 * then loads us at our link address and jumps here with VR_PROGRAM_START2,
 * after asking the boot ROM to flush the caches. */
start:
    b       realstart
    nop

    /* load address, read by jztool (see firmware/target/mips/ingenic_x1000/crt0.S) */
    .ascii  "BEGINHDR"
    .ascii  "LOAD"
    .word   start
    .ascii  "ENDH"

realstart:
    /* keep interrupts disabled, use normal exception vectors (to use EBASE) */
    mtc0    zero, C0_STATUS
    mtc0    zero, C0_CAUSE
    la      t0, irqbase
    mtc0    t0, C0_EBASE

    /* Clear the BSS segment */
    la      t0, bssbegin
    la      t1, bssend
    beq     t0, t1, cache_setup
    nop
clear_bss_loop:
    addiu   t0, 4
    bne     t0, t1, clear_bss_loop
    sw      zero, -4(t0)

cache_setup:
    /* Write back D-cache and invalidate I-cache */
    li      v0, 0x80000000
    ori     v1, v0, (DCACHE_SIZE - DCACHE_LINE_SIZE)
    mtc0    zero, C0_TAGLO
    mtc0    zero, C0_TAGHI
cache_loop:
    cache   DCIndexWBInv, 0(v0)
    cache   ICIndexStTag, 0(v0)
    bne     v0, v1, cache_loop
    addiu   v0, v0, DCACHE_LINE_SIZE

    /* Invalidate BTB */
    mfc0    v0, C0_CONFIG, 7
    nop
    ori     v0, v0, 2
    mtc0    v0, C0_CONFIG, 7
    nop

    la      sp, oc_stackend

    /* jump to C code */
    la      a0, start
    la      t0, main
    jr      t0
    nop

/* restore_exception_jmp restores the context and returns from exception, it takes
 * as argument the type of exception */
    .extern restore_exception_jmp

    .global tlb_refill_handler
    .section .exception.tlb_refill,"ax",%progbits
tlb_refill_handler:
    li      a0, EXCEPTION_ADDR
    la      k0, restore_exception_jmp
    jr      k0
    nop

    .global cache_error_handler
    .section .exception.cache_error,"ax",%progbits
cache_error_handler:
    li      a0, EXCEPTION_ADDR
    la      k0, restore_exception_jmp
    jr      k0
    nop

    .global general_exception_handler
    .section .exception.general_exception,"ax",%progbits
general_exception_handler:
    li      a0, EXCEPTION_UNSP
    la      k0, restore_exception_jmp
    jr      k0
    nop
