; T5: small-immediate tables and signedness.
        .equ TEST_ID, 0x0005
        .include "common.inc"
        .include "prologue.inc"
        mov a0,0x0100
        add a0,-3               ; 4080 imm4 signed      -> 0x00FD
        mov a1,0x0001
        shll a1,3               ; shift_imm[0]          -> 8
        mov a2,0x0001
        shll a2,4               ; shift_imm[3]          -> 0x10
        mov a3,0x0100
        shrl a3,1               ; shift_imm[2]          -> 0x80
        mov n1,0x0100
        add n1,7                ; 4100 imm4             -> 0x107
        mov r1,0xFFF8           ; ADxx imm4 sign-ext    -> 0xFFF8
        mov.l r2,0
        add r2,-2               ; AExx                  -> 0xFFFE
        mov.l r3,0x8000
        and r3,0x00FF           ; F810                  -> 0
        mov.l r4,0x0010
        add r4,-32              ; F850 signed imm16     -> 0xFFF0
        mov.l r5,0x0F00
        or r5,0x000F            ; F820                  -> 0x0F0F
        mov.l r6,0x1234
        shll r6,4               ; A8xx shll imm         -> 0x2340
        mov.l r7,0x8000
        shra r7,3               ; A8xx shra             -> 0xF000
; cmp_imm table: r8 bit i set when 'cmp r9,imm_i' sets Z for r9 = imm_i
        mov.l r8,0
        mov.l r9,0
        cmp r9,0
        bnz c1
        add r8,1
c1:     mov.l r9,1
        cmp r9,1
        bnz c2
        add r8,2
c2:     mov.l r9,0xFFFE
        cmp r9,-2
        bnz c3
        add r8,4
c3:     mov.l r9,0xFFFF
        cmp r9,-1
        bnz c4
        add r8,8
c4:
.include "epilogue.inc"
