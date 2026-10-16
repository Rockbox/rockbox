; T0d: same stores as t0b, but with 8 nops after every a0 write and before
; every store, plus an end marker. Compare with t0b to see whether address
; register writes need delay slots. Run: ZSP.run_for("t0d_hazard"); ZSP.scan()
; Expected if delays fix it: C0D0..C0D5 at halfwords 0x40..0x45 (C0D2/C0D3/C0D4
; may alias because DMEM is 32K halfwords), C0DF at 0x50.
        .org 0
        mov a0,0x0040
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D0
        nop
        nop
        st r0,a0

        mov a0,0x0041
        movhw a0,0x0000
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D1
        nop
        nop
        st r0,a0

        mov a0,0x8042
        movhw a0,0x0000
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D2
        nop
        nop
        st r0,a0

        mov a0,0x0043
        movhw a0,0x0001
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D3
        nop
        nop
        st r0,a0

        mov a0,0x0044
        movhw a0,0x3000
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D4
        nop
        nop
        st r0,a0

        mov a0,0x4045
        movhw a0,0x0000
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D5
        nop
        nop
        st r0,a0

        mov a0,0x0050
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        mov.l r0,0xC0DF
        nop
        nop
        st r0,a0
spin:   br spin
