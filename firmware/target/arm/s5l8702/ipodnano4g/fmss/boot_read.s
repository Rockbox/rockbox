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
; Read three BCH16 codewords into a contiguous 1536-byte buffer.
; P_ROWS: chip row; P_CEMASKS: one CE bit; P_DMALIST: destination.
; d00/d04/d08 hold each sector's uncorrectable bit 0 and erased bit 6.
; r6 is zero on completion, or bit 27 on a controller fault.
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
    wr   [FMC_CMD], #0x0
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
    wr   [FMC_CMD], #0x30
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMC_DATA_SEL], #1
    wr   [FMC_CMD], #0x70
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMCTRL1], #0xca
    wait 12, arg=0x40, mask=0x40
    wr   [FMC_STATUS], #0x800000
    wr   [FMCTRL1], #0x20
    wr   [FMCTRL1], #0x100e0
    wr   [FMC_CMD], #0x0
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMC_DATA_SEL], #0x10
    wr   [FMC_DNUM], #25
    wr   [FMCTRL1], #0x32
    wait 3
    wr   [FMC_STATUS], #8
    wr   [FMC_DATA_SEL], #1
    wr   [FMC_DNUM], #511
    wr   [FMC_18], #1
    wr   [FMCTRL1], #0xe2
    wait 3
    wr   [FMC_STATUS], #8
    movi r7, #0
next_sector:
    and  r4, r7, #1
    movi r3, #1
    sub  r3, r4
    movi r0, #0x10
    shl  r0, r3
    st   [FMC_DATA_SEL], r0
    wr   [FMC_DNUM], #25
    wr   [FMCTRL1], #0x32
    wait 3
    wr   [FMC_STATUS], #8
    ld   r0, [FMC_CTRL0]
    or   r0, r0, #0x1000000
    st   [FMC_CTRL0], r0
    movi r0, #0x100
    shl  r0, r4
    movi r1, #1
    shl  r1, r3
    or   r0, r1
    st   [FMC_DATA_SEL], r0
    wr   [FMC_DNUM], #511
    wr   [FMCTRL1], #0xe2
    wr   [ECC_STATUS], #0x1ff
    st   [FMC_1C], r4
    wr   [0x808], #0x80
    wr   [ECC_CTRL], #5
    wait 4
    ld   r1, [ECC_RESULT], 1
    ld   r0, [ECC_STATUS], 0x40
    or   r1, r0
    wr   [ECC_STATUS], #1
    bz   r7, status0
    st   [0xd04], r1
    bz   r6, transfer
status0:
    st   [0xd00], r1
transfer:
    wr   [FMCTRL1], #0x100
    wait 2
    wr   [FMC_STATUS], #0x100000
    wait 3
    wr   [FMC_STATUS], #8
    add  r7, r7, #1
    sub  r0, r7, #2
    bnz  r0, next_sector
    wr   [ECC_STATUS], #0x1ff
    wr   [FMC_1C], #0
    wr   [0x808], #0x80
    wr   [ECC_CTRL], #5
    wait 4
    ld   r1, [ECC_RESULT], 1
    ld   r0, [ECC_STATUS], 0x40
    or   r1, r0
    st   [0xd08], r1
    wr   [ECC_STATUS], #1
    wr   [FMC_DATA_SEL], #0x100
    wr   [FMC_DNUM], #511
    wr   [FMCTRL1], #0x1e0
    wait 2
    wr   [FMC_STATUS], #0x100000
    wr   [FMC_CTRL0], #0
    end
fault:
    wr   [FMSS_C50], #1
    wr   [FMC_CTRL0], #0
    movi r6, #0x8000000
    end
