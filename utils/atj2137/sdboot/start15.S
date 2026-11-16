/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "sdboot.h"

    .set noreorder
    .set noat
    .section .init.text,"ax",%progbits
    .global start
start:
    b       1f
    nop

    /* header, filled in by mksdboot.py */
    .global hdr
hdr:
    .word   SDBOOT_MAGIC
    .word   0           /* load */
    .word   0           /* entry */
    .word   0           /* size */
    .word   0           /* lba */
    .word   0           /* flags */
1:
    di
    ehb
    li      $sp, STAGE1_STACK
    la      $t0, bssbegin
    la      $t1, bssend
2:
    beq     $t0, $t1, 3f
    nop
    sw      $zero, 0($t0)
    b       2b
    addiu   $t0, $t0, 4
3:
    j       sdboot_main
    nop

    .section .text
    /* int sd_read_sector(uint32_t lba, void *dst)
     * BROM driver takes function number in t0 */
    .global sd_read_sector
sd_read_sector:
    addiu   $sp, $sp, -24
    sw      $ra, 20($sp)
    move    $a3, $a1
    li      $a2, 1
    li      $t0, BROM_DRV_SD_READ
    li      $t1, BROM_DRV_ADDR
    jalr    $t1
    nop
    lw      $ra, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24

    /* void cache_init(void) - invalidate both caches, kseg0 cacheable */
    .global cache_init
cache_init:
    mtc0    $zero, $28           # C0_TAGLO
    mtc0    $zero, $29           # C0_TAGHI
    lui     $t0, 0x8000
    ori     $t1, $t0, 0x4000      # 16kB caches, 16B lines
1:
    cache   8, 0($t0)            # index store tag icache
    cache   9, 0($t0)            # index store tag dcache
    bne     $t0, $t1, 1b
    addiu   $t0, $t0, 0x10
    mfc0    $t0, $16             # C0_CONFIG
    li      $t1, ~7
    and     $t0, $t0, $t1
    ori     $t0, $t0, 3           # K0 = cacheable noncoherent
    mtc0    $t0, $16
    ehb
    jr      $ra
    nop

    /* void jump(uint32_t entry, void (*ret)(void)) - ra = ret */
    .global jump
jump:
    jr      $a0
    move    $ra, $a1
