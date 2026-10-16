; T0e: is the "address of two stores earlier" effect tied to the 38xx
; 'st r,a' encoding, or to every store? Low addresses only.
; Run: ZSP.run_for("t0e_storelag"); ZSP.scan()
;  E1 38xx  st r0,a0       C0D0-C0D2, a0 = 0x60 (unchanged)
;  E2 F4xx  st r0,a1,n     C0D3-C0D5, a1 = 0x70, n = 0,1,2
;  E3 30xx  st r0,a6,n     C0D6-C0D8, a6 = 0x80, n = 0,1,2
;  flush    38xx  st r0,a2 C0DD-C0DF, a2 = 0x90
; Normal stores: 60 61 62 = D0 D0 D0 (same address) -> only C0D2 at 0x60;
;                70 71 72 = D3 D4 D5; 80 81 82 = D6 D7 D8; 90 = DF.
        .org 0
        mov a0,0x0060
        mov a1,0x0070
        mov a6,0x0080
        mov a2,0x0090
        nop
        nop
        nop
        nop
        mov.l r0,0xC0D0
        st r0,a0
        mov.l r0,0xC0D1
        st r0,a0
        mov.l r0,0xC0D2
        st r0,a0
        mov.l r0,0xC0D3
        st r0,a1,0
        mov.l r0,0xC0D4
        st r0,a1,1
        mov.l r0,0xC0D5
        st r0,a1,2
        mov.l r0,0xC0D6
        st r0,a6,0
        mov.l r0,0xC0D7
        st r0,a6,1
        mov.l r0,0xC0D8
        st r0,a6,2
        mov.l r0,0xC0DD
        st r0,a2
        mov.l r0,0xC0DE
        st r0,a2
        mov.l r0,0xC0DF
        st r0,a2
spin:   br spin
