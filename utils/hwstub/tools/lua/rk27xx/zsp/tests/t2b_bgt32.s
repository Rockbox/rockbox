; T2b: the disassembler treats 32-bit conditions 2-5 and 10-13 (agn0-3,
; bgt, ble, bov, bnov) as always-backward. Here bgt.l jumps FORWARD +4.
; Hardware agrees with the DLL  -> runaway, TIMEOUT, r11 never set.
; Hardware uses a signed offset -> r10 = 0, r11 = 1.
        .equ TEST_ID, 0x0021
        .include "common.inc"
        .include "prologue.inc"
        mov.l r10,0
        mov.l r11,0
        mov.l r5,5
        cmp r5,1                ; 5 > 1
        .word 0xF0A0, 4         ; bgt.l +4 (skips the next 2-halfword insn)
        mov.l r10,0xBAD
        mov.l r11,1
.include "epilogue.inc"
