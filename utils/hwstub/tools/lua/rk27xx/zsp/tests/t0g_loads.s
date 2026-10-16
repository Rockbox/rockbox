; T0g: do loads lag like stores? Host pattern: DMEM[i] = 0xB000+i (i < 0x100).
; Run: ZSP.run_for("t0g_loads", 50, {pattern=true}); ZSP.dump(0xa0, 8)
; Results go out with post-increment stores at a7 = 0xA0. Two dummy stores go
; first: under the two-store address lag their data lands at addresses left
; pending by the previous run, and result i lands at 0xA0+i. Results:
;   r1 = ld [0x10]   r2 = ld [0x20]   r3 = ld [0x30]   r4 = ld [0x10]
;   r5 = ld a6+5 (a6=0x40, short form)  r6 = ld a1,4 (F2xx, a1=0x20)
;   r7 = C0D7 (marker)
; Normal loads: B010 B020 B030 B010 B045 B024 C0D7
; Loads lagging two loads: ???? ???? B010 B020 B030 B010 C0D7
        .org 0
        mov a0,0x0010
        mov a1,0x0020
        mov a2,0x0030
        mov a6,0x0040
        mov a7,0x00A0
        nop
        nop
        ld r1,a0
        ld r2,a1
        ld r3,a2
        ld r4,a0
        ld r5,a6,5
        ld r6,a1,4
        mov.l r7,0xC0D7
        stu r7,a7,1             ; dummy (absorbs a stale address)
        stu r7,a7,1             ; dummy
        stu r1,a7,1
        stu r2,a7,1
        stu r3,a7,1
        stu r4,a7,1
        stu r5,a7,1
        stu r6,a7,1
        stu r7,a7,1
        stu r7,a7,1             ; flush
        stu r7,a7,1             ; flush
spin:   br spin
