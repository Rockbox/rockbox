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
    /* save registers for the ROM */
    stmfd   sp!, {r4-r6,lr}
    /* if the power source was 5v (ie usb), bypass the power button delay */
    ldr     r2, =0x800440c0 @ HW_POWER_STS
    ldr     r2, [r2]
    mov     r2, r2, lsl#2
    mov     r2, r2, lsr#26 @ extract PWRUP_SOURCE
    tst     r2, #0x20 @ bit 5: five volts
    bne     boot_rockbox
    /* otherwise monitor the power button for a short time */
    ldr     r2, =550000 @ loop count
    ldr     r4, =0 @ number of times PSWITCH was 1
pswitch_monitor_loop:
    ldr     r3, =0x800440c0 @ HW_POWER_STS
    ldr     r3, [r3]
    mov     r3, r3, lsl#10
    mov     r3, r3, lsr#30 @ extract PSWITCH
    cmp     r3, #1
    addeq   r4, r3 @ add one if PSWITCH=1 (means power hold)
    subs    r2, #1
    bne     pswitch_monitor_loop
    /* power down if power wasn't hold long enough */
    ldr     r2, =400000
    cmp     r4, r2
    bcc     power_down
    /* jump to section given as argument */
boot_rockbox:
    str     r0, [r1]
    mov     r0, #1
    ldmfd   sp!, {r4-r6,pc}
power_down:
    ldr     r0, =0x80044100 @ HW_POWER_RESET
    ldr     r1, =0x3E770001 @ unlock key + power down
    str     r1, [r0]
    b       power_down
#else
#error No target defined !
#endif
