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
#include "helper.h"

    .set noreorder
    /* parameter block lives at HELPER_PARAMS, filled in by host */
    .section .params,"aw",%progbits
    .space 0x40

    /* hwstub calls entry like a function, runs on hwstub stack */
    .section .init.text,"ax",%progbits
    .global start
start:
    addiu   $sp, $sp, -24
    sw      $ra, 20($sp)
    li      $a0, HELPER_PARAMS
    jal     helper_main
    nop
    lw      $ra, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24
