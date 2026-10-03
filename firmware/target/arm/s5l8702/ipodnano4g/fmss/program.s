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
; Program one 4096-byte page with eight BCH codewords. Each sector's
; DMA, encoding and transmission completes before the next sector starts.
;
; P_CEMASKS = CE bit; P_ROWS = page; P_DMALIST = contiguous data address;
; P_META = 12-byte metadata address (only 10 bytes stored); P_TIMING = timing.
; ECCMODE/ECCCFG are encode mode without/with metadata; HDRLEN/PARLEN are
; transmitted header/parity byte counts minus one. Result: as erase.s.
;
; Registers retained in sector loop: r7 index, r5 source address, r4 buffer
; number. The 2 KiB DMA windows are refreshed at sectors 0 and 4. A buffer
; is not reused until its transmission has finished.
    wr   [CS_IRQ], #0xff
    wr   [FMSS_C10], #1
    wr   [FMSS_C58], #4
    wr   [FMSS_C4C], #fault
    wr   [FMCTRL1], #0x33338e0
    wr   [FMC_STATUS], #0xff00ffe
    ld   r0, [P_CEMASKS]
    shl  r0, r0, #1
    ld   r1, [P_TIMING]
    or   r1, r0
    or   r1, r1, #0x801
    st   [FMC_CTRL0], r1
    movi r6, #0
    wr   [FMC_CMD], #0x80
    wait 0
    wr   [FMC_STATUS], #2
    ld   r0, [P_ROWS]
    shl  r1, r0, #16
    st   [FMC_ADDR0], r1
    shr  r1, r0, #16
    st   [FMC_ADDR1], r1
    wr   [FMC_ANUM], #4             ; column zero + three row bytes
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4

; Feed the metadata FIFO once; sector zero's BCH includes its first ten bytes.
    ld   r0, [P_META]
    ldi  r1, [r0]
    st   [FMC_DATA0?], r1
    add  r0, r0, #4
    ldi  r1, [r0]
    st   [FMC_64], r1
    add  r0, r0, #4
    ldi  r1, [r0]
    st   [FMC_68], r1
    wr   [FMC_40], #0xc1
metadata_busy:
    ld   r0, [FMC_40], 1
    bnz  r0, metadata_busy

    ld   r0, [FMC_CTRL0]
    or   r0, r0, #0x1000000
    st   [FMC_CTRL0], r0
    wr   [0x038], #7               ; memory-to-controller DMA
    wr   [0x028], #0               ; page transfer setup
    ld   r5, [P_DMALIST]
    movi r7, #0
sector:
    and  r0, r7, #3
    bnz  r0, dma_window_ready
    st   [FMC_DMA_ADDR], r5
 dma_window_ready:
    and  r4, r7, #1                ; alternate sector buffers A and B
    movi r0, #1
    shl  r0, r4
    st   [FMC_DATA_SEL], r0         ; DMA destination buffer
    wr   [FMC_18], #1
    wr   [FMC_DNUM], #0x1ff
    wr   [FMCTRL1], #0x2e0
    wait 2
    wr   [FMC_STATUS], #0x100000

    ld   r2, [P_PARLEN]
    ld   r3, [P_ECCMODE]
    bnz  r7, encode
    ld   r2, [P_HDRLEN]
    ld   r3, [P_ECCCFG]
encode:
    wr   [ECC_STATUS], #0x1ff
    st   [FMC_1C], r4
    st   [ECC_CTRL], r3
    wait 4
    wr   [ECC_STATUS], #1

; Each sector is represented on flash as parity then data. Sector zero's
; parity FIFO additionally starts with the ten metadata bytes.
    movi r0, #0x1000
    shl  r0, r4
    st   [FMC_DATA_SEL], r0
    st   [FMC_DNUM], r2
    wr   [FMCTRL1], #0x34
    wait 3
    wr   [FMC_STATUS], #8
    movi r0, #0x100
    shl  r0, r4
    st   [FMC_DATA_SEL], r0
    wr   [FMC_DNUM], #0x1ff
    wr   [FMCTRL1], #0xe4
    wait 3
    wr   [FMC_STATUS], #8
    add  r5, r5, #512
    add  r7, r7, #1
    sub  r0, r7, #8
    bnz  r0, sector

    wr   [FMC_CMD], #0x10
    wait 0
    wr   [FMC_STATUS], #2
; Poll the addressed chip until the operation has completed. Keep FAIL
; separate from a controller timeout; neither operation may be retried in place.
    wr   [FMC_CMD], #0x70
    wait 0
    wr   [FMC_STATUS], #2
    wr   [FMCTRL1], #0xca
    wait 12, arg=0x40, mask=0x40
    ld   r6, [0x04c], 1             ; NAND status FAIL
    wr   [FMCTRL1], #0x30020
    wr   [FMC_STATUS], #0x1800000
    ld   r0, [FMC_CTRL0], 0xfefffe01
    st   [FMC_CTRL0], r0
    end
fault:
    wr   [FMSS_C50], #1
    ld   r0, [FMC_CTRL0], 0xfefffe01
    st   [FMC_CTRL0], r0
    movi r6, #0x8000000             ; controller fault, never NAND FAIL
    end
