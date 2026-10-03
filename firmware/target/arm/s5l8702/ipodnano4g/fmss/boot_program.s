;             __________               __   ___.
;   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
;   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
;   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
;   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
;                     \/            \/     \/    \/            \/
;
; Copyright (C) 2026 Viktor Krapivin
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
; KIND, either express or implied.
;
;
; Program three BCH16 codewords, each parity before data, without metadata.
; P_ROWS: chip row; P_CEMASKS: one CE bit; P_DMALIST: 1536-byte source.
; r6: NAND FAIL in bit 0, controller fault in bit 27.
; The next sector DMA overlaps the preceding sector transmission.
    wr   [CS_IRQ], #0xff
    wr   [FMSS_C10], #1
    wr   [FMSS_C58], #4
    wr   [FMSS_C4C], #fault
    wr   [FMCTRL1], #0x33338e0
    wr   [FMC_STATUS], #0xff00ffe
    ld   r0, [P_CEMASKS]
    shl  r0, r0, #1
    or   r0, r0, #0x77801
    st   [FMC_CTRL0], r0
    movi r6, #0
    ld   r0, [P_DMALIST]
    st   [FMC_DMA_ADDR], r0
    wr   [0x038], #7
    wr   [FMC_CMD], #0x80
    wait 0
    wr   [FMC_STATUS], #2
    ld   r0, [P_ROWS]
    shl  r1, r0, #16
    st   [FMC_ADDR0], r1
    shr  r1, r0, #16
    st   [FMC_ADDR1], r1
    wr   [FMC_ANUM], #4
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
    ld   r0, [FMC_CTRL0]
    or   r0, r0, #0x1000000
    st   [FMC_CTRL0], r0
    movi r7, #0
    wr   [FMC_DATA_SEL], #1
    wr   [FMC_DNUM], #511
    wr   [FMC_18], #1
    wr   [FMCTRL1], #0x2e0
    wait 2
    wr   [FMC_STATUS], #0x100000
sector:
    and  r4, r7, #1
    wr   [ECC_STATUS], #0x1ff
    st   [FMC_1C], r4
    wr   [0x808], #0x80
    wr   [ECC_CTRL], #6
    wait 4
    wr   [ECC_STATUS], #1
    bz   r7, parity
    wait 3
    wr   [FMC_STATUS], #8
parity:
    movi r0, #0x1000
    shl  r0, r4
    st   [FMC_DATA_SEL], r0
    wr   [FMC_DNUM], #25
    wr   [FMCTRL1], #0x34
    wait 3
    wr   [FMC_STATUS], #8
    movi r0, #0x100
    shl  r0, r4
    wr   [FMC_DNUM], #511
    sub  r1, r7, #2
    bz   r1, last_data
    movi r1, #1
    sub  r1, r4
    movi r2, #1
    shl  r2, r1
    or   r0, r2
    st   [FMC_DATA_SEL], r0
    wr   [FMCTRL1], #0x2e4
    wait 2
    wr   [FMC_STATUS], #0x100000
    add  r7, r7, #1
    bnz  r7, sector
last_data:
    st   [FMC_DATA_SEL], r0
    wr   [FMCTRL1], #0xe4
    wait 3
    wr   [FMC_STATUS], #8
    wr   [FMC_CMD], #0x10
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMC_DATA_SEL], #1
    wr   [FMC_CMD], #0x70
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMCTRL1], #0xca
    wait 12, arg=0x40, mask=0x40
    wr   [FMC_STATUS], #0x800000
    ld   r6, [0x04c], 1
    wr   [FMCTRL1], #0x20
    wr   [FMC_CTRL0], #0
    end
fault:
    wr   [FMSS_C50], #1
    wr   [FMC_CTRL0], #0
    movi r6, #0x8000000
    end
