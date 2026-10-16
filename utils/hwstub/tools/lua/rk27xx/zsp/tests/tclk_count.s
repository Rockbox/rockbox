; tclk_count: counts forever, the 32-bit count at DSP data address CNT
; (low halfword first). lua/zsp_clk.lua reads it twice, TMR1 timing the
; interval, to see how fast the DSP runs in each clock mode.
; Between stores the DSP spins ~1000 cycles without touching DMEM: storing
; back to back keeps the ARM's DMEM reads from ever completing.
CNT     = 0x200
        .org 0
        mov a0,lo(CNT)
        movhw a0,hi(CNT)
        mov.l r0,0
        mov.l r1,0
        mov.l r2,1
        mov.l r3,0
loop:   mov.l r4,999
        mov %loop0,r4
dly:    nop
        agn0 dly
        add.e r0,r2
        std r0,a0
        br loop
