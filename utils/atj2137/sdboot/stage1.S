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

/* SD card boot sector (LBA 1) for ATJ213x.
 *
 * When NAND boot fails BROM tries SD: it reads LBA 1, 9, 17, 25 into
 * 0xb4040000 and jumps there if the sector passes MBRC check
 * ("ActBrm" + 0x55aa at 0x1f4, sum of dwords + 0x1234 at 0x1fc).
 * BROM SD driver is left resident at 0xb4040200 with the card initialized.
 * Driver function 3: a0 = sector, a2 = count, a3 = dst, returns 1 on success.
 *
 * This sector only pulls stage1.5 (LBA 2..) into free IRAM and jumps to it.
 */

#include "sdboot.h"

    .set noreorder
    .set noat
    .section .text
    .global start
start:
    di
    ehb
    li      $sp, STAGE1_STACK
    li      $s4, STAGE15_LBA
    li      $s5, STAGE15_SECTORS
    li      $s6, STAGE15_ADDR
1:
    lui     $t0, 0xb002          # RTCWDT_WDCTL (0xb0018014) |= 1, feed wdt
    lw      $t1, -0x7fec($t0)
    ori     $t1, $t1, 1
    sw      $t1, -0x7fec($t0)

    move    $a0, $s4
    li      $a2, 1
    move    $a3, $s6
    li      $t0, BROM_DRV_SD_READ
    li      $t1, BROM_DRV_ADDR
    jalr    $t1
    nop
    li      $t1, 1
    bne     $v0, $t1, brom_adfu
    addiu   $s4, $s4, 1
    addiu   $s5, $s5, -1
    bnez    $s5, 1b
    addiu   $s6, $s6, 512

    li      $t0, STAGE15_ADDR
    jr      $t0
    nop

    /* give up: copy BROM ADFU launcher to IRAM and run it (as MBRC does) */
brom_adfu:
    li      $t2, BROM_ADFU_SRC
    li      $t3, BROM_ADFU_DST
    li      $t4, BROM_ADFU_SRC + BROM_ADFU_SIZE
2:
    lw      $t0, 0($t2)
    addiu   $t2, $t2, 4
    sw      $t0, 0($t3)
    bne     $t2, $t4, 2b
    addiu   $t3, $t3, 4
    li      $t0, BROM_ADFU_DST
    jr      $t0
    nop
