; T11: do %timer0 / %timer1 count? Reads them (and %tc) before and after a
; delay loop of 1000 x (10 nop + agn0), and twice back to back.
;   r1/r2/r3 = %timer0/%timer1/%tc before, r4/r5/r6 after,
;   r7/r8 = %timer0 read twice in a row, r9/r10 = %timer1 twice in a row.
        .equ TEST_ID, 0x0011
        .include "common.inc"
        .include "prologue.inc"
        mov r1,%timer0
        mov r2,%timer1
        mov r3,%tc
        mov.l r0,999
        mov %loop0,r0
dly:    nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        agn0 dly
        mov r4,%timer0
        mov r5,%timer1
        mov r6,%tc
        mov r7,%timer0
        mov r8,%timer0
        mov r9,%timer1
        mov r10,%timer1
.include "epilogue.inc"
