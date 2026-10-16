; T7: bits marked x in the encoding tables. Assemble with -D XB=0 (reference)
; and -D XB=1 (x bits set); both dumps must match except TEST_ID.
        .equ TEST_ID, 0x0070 + XB
        .include "common.inc"
        .include "prologue.inc"
        mov a0,lo(SCRATCH)
        movhw a0,hi(SCRATCH)
        mov.l r1,0xB000
        pst st r1,a0,0
        mov.l r1,0xB001
        pst st r1,a0,1
        mov.l r1,0xB002
        pst st r1,a0,2
        mov.l r1,0xB003
        pst st r1,a0,3
        .word 0x2880 | (2 << 4) | (XB << 3) | 0         ; ldd r4,a0   (bit 3 = x)
        .word 0x2900 | (2 << 5) | (XB * 3 << 3) | 0     ; ldq r8,a0   (bits 4:3 = x)
        mov.l r2,0x0001
        mov.l r3,0x0002
        .word 0x4400 | (1 << 4) | (1 << 1) | XB         ; mov.e a1,r2 (bit 0 = x)
        mov.l r12,0x8001
        mov.l r13,0x0000
        .word 0x9C80 | (6 << 3) | (XB * 7)              ; sxtn.f r12  (bits 2:0 = x)
        mov.l r6,lo(SCRATCH)
        .word 0xF100 | (XB * 7), (1 << 8) | (14 << 4) | 6 | (XB * 0x1F << 11) ; ld r14,r6,1 (op 2:0, imm 15:11 = x)
.include "epilogue.inc"
