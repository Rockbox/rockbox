; T0h: why does dump slot 0 (st r0,a7,0 = 0x3001) end up holding r1?
; Run: ZSP.run("t0h_slot0"); ZSP.dump(0x100, 16)
; Expected with normal stores (slot: value):
;   0: C0D0  1: C0D1  2: C0D2  3: C0D0  4: C0D3
;   8: C0D3  9: C0D0  10: C0D1  12: C0D2
        .equ TEST_ID, 0x00C8
        .include "common.inc"
        .include "prologue.inc"
        mov.l r0,0xC0D0
        mov.l r1,0xC0D1
        mov.l r2,0xC0D2
        mov.l r3,0xC0D3
        mov a6,lo(RES + 8)
        movhw a6,hi(RES + 8)
        pst st r0,a7,0          ; 3001 short form, a7, offset 0  -> slot 0
        pst st r1,a7,1          ; 3023 short form, a7, offset 1  -> slot 1
        pst st r2,a7,2          ; 3045                           -> slot 2
        pst st.l r0,a7,3        ; F407 0003 32-bit form          -> slot 3
        pst st.l r3,a7,4        ; F41F 0004                      -> slot 4
        pst st r3,a6,0          ; 3006 short form, a6, offset 0  -> slot 8
        pst st r0,a6,1          ; 3020                           -> slot 9
        pst st r1,a7,10         ; short form, a7, offset 10      -> slot 10
        pst st r2,a7,12         ;                                -> slot 12
        mov.l r0,TEST_ID
        pst st r0,a7,62
        mov.l r0,MAGIC
        pst st r0,a7,63
spin:   br spin
