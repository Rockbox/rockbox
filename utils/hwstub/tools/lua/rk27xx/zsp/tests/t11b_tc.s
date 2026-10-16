; T11b: try to start the DSP timers. Assemble with -D TC=value.
; Writes %tc = TC, then reads %timer0/%timer1 before and after a delay loop
; of 1000 x (10 nop + agn0).
;   r1/r2 = %timer0/%timer1 after writing %tc, r3 = %tc read back,
;   r4/r5 = %timer0/%timer1 after the delay, r6 = %imask
        .equ TEST_ID, 0x0B00
        .include "common.inc"
        .include "prologue.inc"
        mov.l r0,TC
        mov %tc,r0
        nop
        nop
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
        mov r6,%imask
.include "epilogue.inc"
