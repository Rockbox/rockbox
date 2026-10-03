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
; Read pages from P_ROWS/P_CEMASKS into the DMA list; advance parameter pointers.
; Pages run sequentially. Sector transfers alternate buffers A/B to overlap
; NAND transfers with ECC and DMA. Sector 0 includes 10 metadata bytes.
;
; r7: pages left; r6: FMC_CTRL0 until completion, then status summary;
; r5: four packed sector statuses; r0..r4: scratch.
; Result bits: 30 uncorrectable, 29 erased, 0..16 corrected-bit counts,
; 27 sequencer fault. Each page returns 12 metadata bytes, padded with 0xff.
; Optional sector status: 0xff uncorrectable, 0xfe erased, else corrected bits.
; Four statuses per u32, first sector in the most significant byte.
;
    wr   [CS_IRQ], #0xff               ; clear sequencer interrupts
    wr   [FMSS_C10], #0x1              ; sequencer enable
    wr   [FMSS_C58], #0x4              ; event timeout scale
    wr   [FMSS_C4C], #fault            ; fault vector: byte offset of 'fault' below
    wr   [FMCTRL1], #0x33338e0         ; transfer engine idle setup
    wr   [FMC_STATUS], #0xff00ffe      ; clear FMC status
; r6 = FMC_CTRL0 image for the whole run; only its chip-enable bits change per page
    ld   r6, [P_TIMING], 0xffffffff    ; timing fields 30:28 / 18:16 / 14:12 (computed by the CPU)
    or   r6, r6, #0x1000801            ; + DMA path, no chip selected
    st   [FMC_CTRL0], r6               ; (bit 24 = DMA path, bits 11 and 0 always set)
    wr   [V_D3C], #0x0                 ; V_SUM = 0 (summary of all pages)
    ld   r7, [P_COUNT], 0xffffffff     ; r7 = pages left
; ---- per page: fetch the next (CE mask, row) request; *P_x++ = ld pointer, ldi, add 4, st pointer
page:
    bz   r7, finish                    ; no pages left (also P_COUNT = 0)
    ld   r0, [P_CEMASKS], 0xffffffff   ; r4 = *P_CEMASKS++ (1 << chip)
    ldi  r4, [r0]
    add  r0, r0, #0x4
    st   [P_CEMASKS], r0
    ld   r0, [P_ROWS], 0xffffffff      ; r1 = *P_ROWS++ (page within the chip)
    ldi  r1, [r0]
    add  r0, r0, #0x4
    st   [P_ROWS], r0
    sub  r7, r7, #0x1                  ; count this page
; ---- 5 address cycles: 2 column bytes (0) + 3 row bytes (row = block * 128 + page), select the chip
    wr   [FMC_ANUM], #0x4              ; 5 address cycles
    shl  r0, r1, #0x10                 ; cycles 1..4 = column lo, column hi, row b7..0, row b15..8
    st   [FMC_ADDR0], r0               ; column 0, row bits 15..0
    shr  r0, r1, #0x10                 ; cycle 5 = row b23..16
    st   [FMC_ADDR1], r0               ; row bits 23..16
    shl  r0, r4, #0x1                  ; CE n select = FMC_CTRL0 bit (n + 1)
    and  r6, r6, #0xfffffe01           ; clear CE bits 8..1, keep timing / DMA path
    or   r6, r0
    st   [FMC_CTRL0], r6               ; select the chip
; ---- 00h - address - 30h: the chip loads the page into its page register (busy for tR);
;      each step: start it, wait for its event, clear the event's FMC_STATUS bit
    wr   [FMC_CMD], #0x0               ; READ (00h)
    wait 0                             ; command sent
    wr   [FMC_STATUS], #0x2
    wr   [FMCTRL1], #0x1               ; send the 5 address cycles
    wait 1                             ; address sent
    wr   [FMC_STATUS], #0x4
    wr   [FMC_CMD], #0x30              ; READ confirm (30h): the chip loads the page
    wait 0                             ; command sent
    wr   [FMC_STATUS], #0x2
; ---- wait for ready: 70h, then the controller polls the status byte (FMCTRL1 0xca)
    st   [FMC_CTRL0], r6               ; (chip still selected)
    wr   [FMC_CMD], #0x70              ; READ STATUS (70h)
    wait 0                             ; command sent
    wr   [FMC_STATUS], #0x2
    wr   [FMCTRL1], #0xca              ; poll the status byte ...
    wait 12, arg=0x40, mask=0x40       ; ... until bit 6 (ready) is set
    wr   [FMCTRL1], #0x10020           ; as the original program after the poll (meaning unknown)
    wr   [FMC_STATUS], #0x1800000      ; clear status bits 23 + 24 after the ready poll (w1c; as original)
    wr   [FMC_CMD], #0x0               ; back to data output (00h)
    wait 0                             ; command sent
    wr   [FMC_STATUS], #0x2
; ---- sector 0: 10 meta bytes + parity into parity buffer A, then 512 data bytes into data buffer A
    ld   r0, [P_SPARELEN], 0xffffffff  ; = P_HDRLEN (same word, +0xd34)
    st   [FMC_DNUM], r0                ; meta + parity bytes - 1
    wr   [FMC_58], #0x10               ; into parity buffer A
    wr   [FMCTRL1], #0x12              ; read meta + parity of sector 0
    movi r5, #0x0                      ; status bytes of this page start empty
    wait 3                             ; transferred
    wr   [FMC_STATUS], #0x8
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMC_58], #0x1                ; into data buffer A
    wr   [FMCTRL1], #0x42              ; read data of sector 0
; ---- step 1: sector 1 in (buffer B), sector 0 decoded and DMAed (buffer A)
    wait 3                             ; sector 0 data transferred
    wr   [FMC_STATUS], #0x8
    wr   [FMCTRL1], #0x20020           ; as the original program after sector 0 (meaning unknown)
; sector 1 parity -> parity buffer B
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x20               ; into parity buffer B
    wr   [FMCTRL1], #0x12              ; read parity of sector 1
    ld   r0, [P_DMALIST], 0xffffffff   ; next 2 KiB DMA destination: r1 = *P_DMALIST++
    ldi  r1, [r0]
    add  r0, r0, #0x4
    st   [P_DMALIST], r0
    st   [FMC_DMA_ADDR], r1            ; the controller advances it by itself within the chunk
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x102        ; data into buffer B
    wr   [FMC_18], #0x1                ; as the original program (meaning unknown)
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 1 (runs while sector 0 is decoded)
; decode sector 0 (buffer A)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #0                  ; decode the buffer of sector 0
    ld   r0, [P_ECCCFG], 0xffffffff    ; BCH mode incl. the 10 meta bytes
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 0: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 0
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s0_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s0_byte                   ; (r1 != 0 here: unconditional jump)
s0_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s0_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s0_byte                   ; (r1 != 0: jump)
s0_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s0_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s0_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s0_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s0_done
    and  r0, r2, #0x1
    bz   r0, s0_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s0_inc                    ; (r1 != 0: jump)
s0_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s0_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s0_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 0 out
; corrected meta bytes of sector 0 -> *P_META++ (12 bytes)
    wr   [FMC_40], #0xc2               ; latch the corrected meta bytes
meta_busy:
    ld   r0, [FMC_40], 0x2             ; bit 1: latch busy
    bnz  r0, meta_busy
    ld   r1, [P_META], 0xffffffff      ; *P_META++ = 3 words, the last | 0xffff0000
    ld   r0, [FMC_DATA0?], 0xffffffff  ; meta bytes 0..3
    sti  [r1], r0
    add  r1, r1, #0x4
    ld   r0, [FMC_64], 0xffffffff      ; meta bytes 4..7
    sti  [r1], r0
    add  r1, r1, #0x4
    ld   r0, [FMC_68], 0xffffffff      ; meta bytes 8..9
    or   r0, r0, #0xffff0000           ; bytes 10..11 = 0xff
    sti  [r1], r0
    add  r1, r1, #0x4
    st   [P_META], r1
; ---- step 2: sector 2 in (buffer A), sector 1 decoded and DMAed (buffer B)
    wait 2                             ; DMA of sector 0 done: buffer A is free again
    wr   [FMC_STATUS], #0x100000       ; clear the DMA-done bit
    wait 3                             ; sector 1 data transferred
    wr   [FMC_STATUS], #0x8
; sector 2 parity -> parity buffer A
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x10               ; into parity buffer A
    wr   [FMCTRL1], #0x12              ; read parity of sector 2
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x201        ; data into buffer A
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 2 (runs while sector 1 is decoded)
; decode sector 1 (buffer B)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #1                  ; decode the buffer of sector 1
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 1: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 1
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s1_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s1_byte                   ; (r1 != 0 here: unconditional jump)
s1_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s1_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s1_byte                   ; (r1 != 0: jump)
s1_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s1_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s1_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s1_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s1_done
    and  r0, r2, #0x1
    bz   r0, s1_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s1_inc                    ; (r1 != 0: jump)
s1_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s1_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s1_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 1 out
; ---- step 3: sector 3 in (buffer B), sector 2 decoded and DMAed (buffer A)
    wait 2                             ; DMA of sector 1 done: buffer B is free again
    wr   [FMC_STATUS], #0x100000       ; clear the DMA-done bit
    wait 3                             ; sector 2 data transferred
    wr   [FMC_STATUS], #0x8
; sector 3 parity -> parity buffer B
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x20               ; into parity buffer B
    wr   [FMCTRL1], #0x12              ; read parity of sector 3
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x102        ; data into buffer B
    wr   [FMC_18], #0x1                ; as the original program (meaning unknown)
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 3 (runs while sector 2 is decoded)
; decode sector 2 (buffer A)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #0                  ; decode the buffer of sector 2
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 2: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 2
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s2_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s2_byte                   ; (r1 != 0 here: unconditional jump)
s2_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s2_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s2_byte                   ; (r1 != 0: jump)
s2_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s2_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s2_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s2_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s2_done
    and  r0, r2, #0x1
    bz   r0, s2_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s2_inc                    ; (r1 != 0: jump)
s2_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s2_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s2_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 2 out
; ---- step 4: sector 4 in (buffer A), sector 3 decoded and DMAed (buffer B)
    wait 2                             ; DMA of sector 2 done: buffer A is free again
    wr   [FMC_STATUS], #0x100000       ; clear the DMA-done bit
    wait 3                             ; sector 3 data transferred
    wr   [FMC_STATUS], #0x8
; sector 4 parity -> parity buffer A
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x10               ; into parity buffer A
    wr   [FMCTRL1], #0x12              ; read parity of sector 4
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x201        ; data into buffer A
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 4 (runs while sector 3 is decoded)
; decode sector 3 (buffer B)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #1                  ; decode the buffer of sector 3
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 3: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 3
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s3_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s3_byte                   ; (r1 != 0 here: unconditional jump)
s3_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s3_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s3_byte                   ; (r1 != 0: jump)
s3_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s3_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s3_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s3_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s3_done
    and  r0, r2, #0x1
    bz   r0, s3_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s3_inc                    ; (r1 != 0: jump)
s3_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s3_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s3_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 3 out
; ---- step 5: sector 5 in (buffer B), sector 4 decoded and DMAed (buffer A)
    wait 2                             ; DMA of sector 3 done: buffer B is free again
    wr   [FMC_STATUS], #0x100000       ; clear the DMA-done bit
; sectors 0..3 done: store their status word, DMA sectors 4..7 to the next 2 KiB
    ld   r0, [P_ECCBUF], 0xffffffff    ; status bytes of 4 sectors -> *P_ECCBUF++ (optional)
    bz   r0, st465                     ; P_ECCBUF = 0: no status bytes wanted
    sti  [r0], r5
    add  r0, r0, #0x4
    st   [P_ECCBUF], r0
st465:
    ld   r0, [P_DMALIST], 0xffffffff   ; next 2 KiB DMA destination: r1 = *P_DMALIST++
    ldi  r1, [r0]
    add  r0, r0, #0x4
    st   [P_DMALIST], r0
    st   [FMC_DMA_ADDR], r1            ; the controller advances it by itself within the chunk
    wait 3                             ; sector 4 data transferred
    wr   [FMC_STATUS], #0x8
; sector 5 parity -> parity buffer B
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x20               ; into parity buffer B
    wr   [FMCTRL1], #0x12              ; read parity of sector 5
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x102        ; data into buffer B
    wr   [FMC_18], #0x1                ; as the original program (meaning unknown)
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 5 (runs while sector 4 is decoded)
; decode sector 4 (buffer A)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #0                  ; decode the buffer of sector 4
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 4: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 4
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s4_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s4_byte                   ; (r1 != 0 here: unconditional jump)
s4_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s4_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s4_byte                   ; (r1 != 0: jump)
s4_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s4_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s4_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s4_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s4_done
    and  r0, r2, #0x1
    bz   r0, s4_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s4_inc                    ; (r1 != 0: jump)
s4_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s4_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s4_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 4 out
; ---- step 6: sector 6 in (buffer A), sector 5 decoded and DMAed (buffer B)
    wait 2                             ; DMA of sector 4 done: buffer A is free again
    wr   [FMC_STATUS], #0x100000       ; clear the DMA-done bit
    wait 3                             ; sector 5 data transferred
    wr   [FMC_STATUS], #0x8
; sector 6 parity -> parity buffer A
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x10               ; into parity buffer A
    wr   [FMCTRL1], #0x12              ; read parity of sector 6
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x201        ; data into buffer A
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 6 (runs while sector 5 is decoded)
; decode sector 5 (buffer B)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #1                  ; decode the buffer of sector 5
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 5: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 5
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s5_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s5_byte                   ; (r1 != 0 here: unconditional jump)
s5_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s5_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s5_byte                   ; (r1 != 0: jump)
s5_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s5_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s5_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s5_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s5_done
    and  r0, r2, #0x1
    bz   r0, s5_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s5_inc                    ; (r1 != 0: jump)
s5_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s5_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s5_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 5 out
; ---- step 7: sector 7 in (buffer B), sector 6 decoded and DMAed (buffer A)
    wait 2                             ; DMA of sector 5 done: buffer B is free again
    wr   [FMC_STATUS], #0x100000       ; clear the DMA-done bit
    wait 3                             ; sector 6 data transferred
    wr   [FMC_STATUS], #0x8
; sector 7 parity -> parity buffer B
    ld   r0, [P_PARLEN], 0xffffffff
    st   [FMC_DNUM], r0                ; parity bytes - 1 (12 BCH8 / 25 BCH16)
    wr   [FMC_58], #0x20               ; into parity buffer B
    wr   [FMCTRL1], #0x12              ; read parity of sector 7
    wait 3                             ; parity transferred
    wr   [FMC_STATUS], #0x8            ; clear the transfer-done bit
    wr   [FMC_DATA_SEL], #0x102        ; data into buffer B
    wr   [FMC_18], #0x1                ; as the original program (meaning unknown)
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0xe2              ; read data of sector 7 (runs while sector 6 is decoded)
; decode sector 6 (buffer A)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #0                  ; decode the buffer of sector 6
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 6: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 6
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s6_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s6_byte                   ; (r1 != 0 here: unconditional jump)
s6_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s6_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s6_byte                   ; (r1 != 0: jump)
s6_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s6_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s6_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s6_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s6_done
    and  r0, r2, #0x1
    bz   r0, s6_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s6_inc                    ; (r1 != 0: jump)
s6_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s6_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s6_done:
    wr   [FMCTRL1], #0x100             ; DMA sector 6 out
; ---- end of page: sector 7 decoded and DMAed, status word of sectors 4..7 stored
    wait 2                             ; DMA of sector 6 done
    wr   [FMC_STATUS], #0x100000
    wait 3                             ; sector 7 data transferred
    wr   [FMC_STATUS], #0x8
; decode sector 7 (buffer B)
    wr   [ECC_STATUS], #0x1ff          ; clear the decoder
    wr   [FMC_1C], #1                  ; decode the buffer of sector 7
    ld   r0, [P_ECCMODE], 0xffffffff   ; BCH mode
    st   [ECC_CTRL], r0                ; start decoding
    wait 4                             ; decoder done
    wr   [ECC_STATUS], #0x1            ; acknowledge the decoder (write 1 to clear)
; classify sector 7: r2 = ECC_RESULT | (ECC_STATUS & 0x40): b0 uncorrectable, b6 erased, b20..16 count;
;   r0 = status byte 0xff / 0xfe / count, r3 = summary bit b30 / b29 / 1 << count
    ld   r2, [ECC_RESULT], 0xffffffff  ; b0 uncorrectable, b20..16 corrected bits
    ld   r1, [ECC_STATUS], 0x40        ; b6: the sector is all 0xff (erased)
    or   r2, r1                        ; r2 = decode result of sector 7
    and  r1, r2, #0x1                  ; b0: uncorrectable?
    bz   r1, s7_ok
    movi r0, #0xff                     ; status byte: uncorrectable
    movi r3, #0x40000000               ; summary: some sector uncorrectable
    bnz  r1, s7_byte                   ; (r1 != 0 here: unconditional jump)
s7_ok:
    and  r1, r2, #0x40                 ; b6: erased?
    bz   r1, s7_corr
    movi r3, #0x20000000               ; summary: some sector erased (r0 is set to 0xfe below)
    bnz  r1, s7_byte                   ; (r1 != 0: jump)
s7_corr:
    shr  r0, r2, #0x10                 ; ECC_RESULT b20..16
    and  r0, r0, #0x1f                 ; status byte: corrected bits
    movi r3, #0x1
    shl  r3, r0                        ; summary: bit n = some sector had n corrections
s7_byte:
    and  r1, r2, #0x40                 ; erased overrides the byte chosen above
    bz   r1, s7_add
    movi r0, #0xfe                     ; status byte: erased (also if uncorrectable)
s7_add:
    shl  r5, r5, #0x8                  ; r5 = (r5 << 8) | status byte:
    or   r5, r0                        ; append to this group's status word
    ld   r1, [V_D3C], 0xffffffff       ; V_SUM is kept in sequencer RAM, not in a register
    or   r1, r3
    st   [V_D3C], r1                   ; V_SUM |= summary bit
    ld   r1, [P_STATS], 0xffffffff     ; histogram (optional): r1 = &histogram[0], 0 = none
    bz   r1, s7_done
    and  r0, r2, #0x1
    bz   r0, s7_hcorr
    add  r1, r1, #0x44                 ; entry 17 (byte offset 0x44): uncorrectable sectors
    bnz  r1, s7_inc                    ; (r1 != 0: jump)
s7_hcorr:
    shr  r0, r2, #0xe                  ; count (b20..16) >> 14 = count * 4 in b6..2
    and  r0, r0, #0x7c                 ; entry n (offset 4n): sectors with n corrections
    add  r1, r0                        ; r1 = &histogram[n]
s7_inc:
    ldi  r0, [r1]                      ; histogram[entry] += 1 (system memory)
    add  r0, r0, #0x1
    sti  [r1], r0
s7_done:
    wr   [FMC_5C], #0x200              ; last DMA of the page
    wr   [FMC_DNUM], #0x1ff            ; 512 bytes
    wr   [FMCTRL1], #0x180             ; DMA sector 7 out
    ld   r0, [P_ECCBUF], 0xffffffff    ; status bytes of 4 sectors -> *P_ECCBUF++ (optional)
    bz   r0, st739                     ; P_ECCBUF = 0: no status bytes wanted
    sti  [r0], r5
    add  r0, r0, #0x4
    st   [P_ECCBUF], r0
st739:
    wait 2                             ; DMA done
    wr   [FMC_STATUS], #0x100000
    bnz  r7, page                      ; next page
; ---- all pages done: deselect, r6 = V_SUM (bits: see RESULT above)
finish:
    and  r6, r6, #0xfefffe01           ; clear CE bits 8..1 and bit 24
    st   [FMC_CTRL0], r6               ; deselect, DMA path off
    ld   r6, [V_D3C], 0xffffffff       ; result = V_SUM
    end                                ; raises interrupt bit 0 (done)
; ---- fault handler: entered through FMSS_C4C when a WAIT times out (the engine also raises interrupt
;      bit 2). Deselect, return the summary so far with bit 27 set; the CPU resets the chips.
fault:
    ld   r2, [FMSS_C3C], 0xffffffff    ; fault info (read as the original handlers do, unused)
    ld   r1, [FMC_CTRL0], 0xfefffe01   ; current FMC_CTRL0 without CE bits and DMA path
    st   [FMC_CTRL0], r1               ; deselect
    ld   r6, [V_D3C], 0xffffffff
    or   r6, r6, #0x8000000            ; result: fault flag
    end
