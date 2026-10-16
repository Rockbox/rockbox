; T0b: find the DSP-side address of DMEM.
; Stores marker 0xC0D0+i to candidate_i + 0x20 + i, most likely candidate
; first (a store to a missing address may hang the DSP). Run with
; ZSP.run_for("t0b_probe") then ZSP.scan().
        .org 0
        mov a0,0x0020           ; candidate 0: DMEM at DSP address 0, a0 = 0x20
        mov.l r0,0xC0D0
        st r0,a0

        mov a0,0x0021
        movhw a0,0x0000         ; same, but through movhw (tests movhw keeps low half)
        mov.l r0,0xC0D1
        st r0,a0

        mov a0,0x8022           ; candidate 2: 0x8000
        movhw a0,0x0000
        mov.l r0,0xC0D2
        st r0,a0

        mov a0,0x0023           ; candidate 3: 0x10000
        movhw a0,0x0001
        mov.l r0,0xC0D3
        st r0,a0

        mov a0,0x0024           ; candidate 4: 0x30000000 (ARM view)
        movhw a0,0x3000
        mov.l r0,0xC0D4
        st r0,a0

        mov a0,0x4025           ; candidate 5: 0x4000
        movhw a0,0x0000
        mov.l r0,0xC0D5
        st r0,a0
spin:   br spin
