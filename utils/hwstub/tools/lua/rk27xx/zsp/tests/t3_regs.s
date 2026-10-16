; T3: register field positions, pair order, multi-register loads.
;  r0-r15 = 0x1000+i first; later steps overwrite some (see expect.py).
        .equ TEST_ID, 0x0003
        .include "common.inc"
        .include "prologue.inc"
        mov.l r0,0x1000
        mov.l r1,0x1001
        mov.l r2,0x1002
        mov.l r3,0x1003
        mov.l r4,0x1004
        mov.l r5,0x1005
        mov.l r6,0x1006
        mov.l r7,0x1007
        mov.l r8,0x1008
        mov.l r9,0x1009
        mov.l r10,0x100A
        mov.l r11,0x100B
        mov.l r12,0x100C
        mov.l r13,0x100D
        mov.l r14,0x100E
        mov.l r15,0x100F
; a-register built from an r pair: which of r2/r3 is the high half?
        mov.e a1,r2             ; a1 = {r3,r2} or {r2,r3}
        movhw r4,a1             ; r4 = high half of a1
        movlw r5,a1             ; r5 = low half of a1
; a-register immediates
        mov a2,0x2222
        movhw a2,0x3333         ; expect a2 = 0x33332222
        mov a3,a2               ; a3 = a2
; index registers
        mov n1,0x0111
        mov n2,r1               ; n2 = 0x1001
        mov n3,n1               ; n3 = 0x0111
; multi-register loads: SCRATCH[0..3] = 0xA000..0xA003
        mov a0,lo(SCRATCH)
        movhw a0,hi(SCRATCH)
        mov.l r6,0xA000
        pst st r6,a0,0
        mov.l r6,0xA001
        pst st r6,a0,1
        mov.l r6,0xA002
        pst st r6,a0,2
        mov.l r6,0xA003
        pst st r6,a0,3
        ldd r8,a0               ; r8/r9 = SCRATCH[0..1] in some order
        ldq r12,a0              ; r12..r15 = SCRATCH[0..3]
        ldd a4,a0               ; a4 = 32-bit from SCRATCH[0..1]
; guard registers: g1 = 0x11 ... g6 = 0x66 (8-bit immediates)
        mov g0,0x10
        mov g1,0x11
        mov g2,0x22
        mov g3,0x33
        mov g4,0x44
        mov g5,0x55
        mov g6,0x66
        mov g7,0x77
.include "epilogue.inc"
