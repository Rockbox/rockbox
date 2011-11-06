/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

.text
.global start
@ int start(uint32_t arg, uint32_t *result_id)
start:
#if defined(SANSA_FUZEPLUS)
    /* If volume down key is hold, return so that the OF can boot */
    ldr     r2, =0x80018610 @ HW_PINCTRL_DIN1
    ldr     r2, [r2]
    tst     r2, #0x40000000 @ bit 30, active low
    moveq   r0, #0 @ return 0, continue boot
    bxeq    lr
    /* otherwise jump to section given as argument */
    str     r0, [r1]
    mov     r0, #1
    bx      lr
#else
#error No target defined !
#endif
