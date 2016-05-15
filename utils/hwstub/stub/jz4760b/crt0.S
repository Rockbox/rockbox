#include "mips.h"

.extern main
.global start

.set mips32
.set noreorder
.set noat

.section .init.text,"ax",%progbits
start:

clear_bss:
    la      t0, bssbegin
    la      t1, bssend
    beq     t0, t1, stack_setup
    nop

clear_bss_loop:
    sw      zero, 0(t0)
    bne     t0, t1, clear_bss_loop
    addiu   t0, 4

stack_setup:
    la      sp, oc_stackend

    /* jump to C code */
    la  t0, main
    jr  t0
    nop
