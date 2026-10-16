; T1: pc units, %rpc after 16- and 32-bit calls, %pc read.
; expect: r1 == r6 (ret1), r2 == r7 (ret2), r4 ~ r8 (here)
        .equ TEST_ID, 0x0001
        .include "common.inc"
        .include "prologue.inc"
        call sub1               ; 32-bit call
ret1:   mov.l r3,sub2
        call r3                 ; 16-bit call through register
ret2:   nop
here:   mov r4,%pc
        mov.l r6,ret1
        mov.l r7,ret2
        mov.l r8,here
        br dump
sub1:   mov r1,%rpc
        ret
sub2:   mov r2,%rpc
        ret
.include "epilogue.inc"
