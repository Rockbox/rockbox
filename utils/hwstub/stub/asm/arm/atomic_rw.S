/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
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

    .section   .text, "ax", %progbits
    .global    target_read8
    .type      target_read8, %function
    .global    target_read16
    .type      target_read16, %function
    .global    target_read32
    .type      target_read32, %function
    .global    target_write8
    .type      target_write8, %function
    .global    target_write16
    .type      target_write16, %function
    .global    target_write32
    .type      target_write32, %function

target_read8:
    ldrb r0, [r0]
    bx lr

target_read16:
    ldrh r0, [r0]
    bx lr

target_read32:
    ldr r0, [r0]
    bx lr

target_write8:
    strb r1, [r0]
    bx lr

target_write16:
    strh r1, [r0]
    bx lr

target_write32:
    str r1, [r0]
    bx lr

