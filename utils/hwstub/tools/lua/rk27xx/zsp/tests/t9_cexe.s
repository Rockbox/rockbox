; T9: conditional execution shadow (cexe ... !n covers the next n insns).
        .equ TEST_ID, 0x0009
        .include "common.inc"
        .include "prologue.inc"
        mov.l r5,0
        mov.l r6,0
        mov.l r7,0
        mov.l r8,0
        mov.l r9,0
        cmp r5,0                ; Z = 1
        cexe (z,nu) { } !2      ; true: both execute  -> r6 = 2
        add r6,1
        add r6,1
        add r7,1                ; outside shadow      -> r7 = 1
        cmp r5,0
        cexe (nz,nu) { } !2     ; false: both skipped -> r8 = 0
        add r8,1
        add r8,1
        cmp r5,0
        cexe (nz,nu) { } !1     ; false, length 1     -> r9 = 1
        add r9,1
        add r9,1
.include "epilogue.inc"
