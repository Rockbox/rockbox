; T4: addressing modes. SCRATCH[0..15] = 0x4000+i, B = SCRATCH+8.
        .equ TEST_ID, 0x0004
        .include "common.inc"
        .include "prologue.inc"
        mov a5,lo(SCRATCH)
        movhw a5,hi(SCRATCH)
        mov.l r0,0x4000
        pst st r0,a5,0
        mov.l r0,0x4001
        pst st r0,a5,1
        mov.l r0,0x4002
        pst st r0,a5,2
        mov.l r0,0x4003
        pst st r0,a5,3
        mov.l r0,0x4004
        pst st r0,a5,4
        mov.l r0,0x4005
        pst st r0,a5,5
        mov.l r0,0x4006
        pst st r0,a5,6
        mov.l r0,0x4007
        pst st r0,a5,7
        mov.l r0,0x4008
        pst st r0,a5,8
        mov.l r0,0x4009
        pst st r0,a5,9
        mov.l r0,0x400A
        pst st r0,a5,10
        mov.l r0,0x400B
        pst st r0,a5,11
        add a5,8                ; a5 = B
; post-modify with every mod_u value; a1..a4 snapshot a0 after each
        mov a0,a5
        ldu r1,a0,2             ; r1=[B]   a0=B+2
        mov a1,a0
        ldu r2,a0,1             ; r2=[B+2] a0=B+3
        mov a2,a0
        ldu r3,a0,-2            ; r3=[B+3] a0=B+1
        mov a3,a0
        ldu r4,a0,-1            ; r4=[B+1] a0=B
        mov a4,a0
; 16-bit signed displacement (F2xx)
        ld r5,a5,3              ; [B+3]
        ld r6,a5,-1             ; [B-1]
; r-register base, 3-bit signed displacement (F1xx)
        mov.l r7,lo(SCRATCH + 8)
        ld r8,r7,2              ; [B+2] if r7 is a full data address
; indexed with n (F640)
        mov n1,3
        ldx r9,a5,n1            ; [B+3]?
; push / pop through a6 (stack in SCRATCH+0x40)
        mov a6,lo(SCRATCH + 0x40)
        movhw a6,hi(SCRATCH + 0x40)
        mov.l r10,0x5A5A
        pst push r10,a6
        movlw r11,a6            ; a6 low after push
        pop r12,a6              ; expect 0x5A5A
        movlw r13,a6            ; a6 low after pop
; where did push write? (host fills DMEM with 0xA5A5 before the run)
        ld r14,a5,0x37          ; SCRATCH+0x3F
        ld r15,a5,0x38          ; SCRATCH+0x40
        ld r7,a5,0x39           ; SCRATCH+0x41
.include "epilogue.inc"
