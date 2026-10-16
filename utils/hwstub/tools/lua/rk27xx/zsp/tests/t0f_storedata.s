; T0f: stores complete two stores late (t0e). Which part is late?
; Run: ZSP.run_for("t0f_storedata"); ZSP.scan()
; Store order: P1 P2 (0x70, C0DA) S1 (0x60, r1=C0D1, then r1:=C0D9 without
; a store) S2 (0x61, r2=C0D2) S3 (0x62, r3=C0D3) F1 F2 F3 (0x63, C0DE)
;   address lag (data at issue):  60=D3 61=DE 62=DE 63=DE 70=D2
;   late data read (addr at issue): 60=D9 61=D2 62=D3 63=DE 70=DA
;   in-order FIFO, 2 stay pending:  60=D1 61=D2 62=D3 63=DE 70=DA
        .org 0
        mov a4,0x0070
        mov a0,0x0060
        mov a1,0x0061
        mov a2,0x0062
        mov a3,0x0063
        mov.l r5,0xC0DA
        mov.l r4,0xC0DE
        st r5,a4                ; P1
        st r5,a4                ; P2
        mov.l r1,0xC0D1
        st r1,a0                ; S1
        mov.l r1,0xC0D9         ; overwrite the source, no store
        mov.l r2,0xC0D2
        st r2,a1                ; S2
        mov.l r3,0xC0D3
        st r3,a2                ; S3
        st r4,a3                ; F1
        st r4,a3                ; F2
        st r4,a3                ; F3
spin:   br spin
