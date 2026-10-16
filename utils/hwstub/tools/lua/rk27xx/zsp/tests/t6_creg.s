; T6: control register tables. Write %loop0 through every table that names
; it, reading back through creg_e7e4 (mov r,%loop0) after each write.
        .equ TEST_ID, 0x0006
        .include "common.inc"
        .include "prologue.inc"
        mov.l r1,0x0011
        mov %loop0,r1           ; 62xx creg_e724[5]
        mov r2,%loop0           ; expect 0x0011
        mov r3,%loop0           ; second read, expect 0x0011 (79xx cannot reach %loop0: bit 6 is fixed)
        mov n1,0x0033
        .word 0x7F00 | (0 << 3) | 1     ; mov %loop0,n1 via 7F0x creg_e674[0]
        mov r4,%loop0           ; expect 0x0033
        mov %loop0,0x0044       ; F96x creg_e684[5]
        mov r5,%loop0           ; expect 0x0044
        movl %loop0,0x55        ; F94x low byte
        mov r6,%loop0           ; expect 0x0055
        movh %loop0,0x66        ; F95x high byte
        mov r7,%loop0           ; expect 0x6655
; %loop1 written via 62xx, read through 6000 and via a different index table
        mov.l r1,0x0123
        mov %loop1,r1
        mov r8,%loop1           ; expect 0x0123
; circular buffer registers: write 62xx, read 7B0x (creg_e864)
        mov.l r1,0x0C00
        mov %cb0_beg,r1         ; 62xx creg_e724[19]
        mov r9,%cb0_beg         ; 7Bxx creg_e864[0]
        mov.l r1,0x0C10
        mov %cb0_end,r1
        mov r10,%cb0_end        ; 7Bxx creg_e864[1]
; return pc pair: mov.e r12,%rpc (7Cxx creg_e6c4[1]) after a call
        call c_sub
c_ret:  br c_done
c_sub:  mov.e r12,%rpc          ; r12/r13 = 24-bit rpc split
        mov r14,%rpc
        ret
c_done:
.include "epilogue.inc"
