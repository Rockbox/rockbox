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
; Erase exactly one block, using the large-page NAND 60h/D0h protocol.
; P_CEMASKS = selected CE bit, P_ROWS = first page in block,
; P_TIMING = bus timing. Result r6: 0 success, 1 NAND FAIL, bit 27 fault.
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
    wr   [FMC_CMD], #0x60
    wait 0
    wr   [FMC_STATUS], #2
    ld   r0, [P_ROWS]
    st   [FMC_ADDR0], r0
    wr   [FMC_ANUM], #2             ; three row address cycles
    wr   [FMCTRL1], #1
    wait 1
    wr   [FMC_STATUS], #4
    wr   [FMC_CMD], #0xd0
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
