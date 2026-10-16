; T0c: data address units and store forms. Needs -D DMEM_DSP.
; Writes distinct-byte halfwords to SCRATCH+0..5 with several store forms;
; the host raw dump shows whether +1 is the next byte or the next halfword.
        .equ TEST_ID, 0x00C0
        .include "common.inc"
        .include "prologue.inc"
        mov a0,lo(SCRATCH)
        movhw a0,hi(SCRATCH)
        mov.l r0,0x1122
        pst st r0,a0                ; SCRATCH+0
        mov.l r0,0x3344
        pst st r0,a0,1              ; F400 16-bit displacement: SCRATCH+1
        mov.l r0,0x5566
        mov a1,a0
        add a1,2                ; 4080 add a,imm4
        pst st r0,a1                ; SCRATCH+2
        mov a2,a0
        add a2,3
        mov.l r0,0x7788
        pst stu r0,a2,1             ; SCRATCH+3, a2 -> +4
        mov.l r0,0x99AA
        pst st r0,a2                ; SCRATCH+4 if post-modify +1
        mov.l r0,0xBBCC
        pst std r0,a0,6             ; pair store r0:r1 at SCRATCH+6
        ld r1,a0                ; read back
        ld r2,a0,1
        ld r3,a0,2
        ld r4,a0,3
        ld r5,a0,4
.include "epilogue.inc"
