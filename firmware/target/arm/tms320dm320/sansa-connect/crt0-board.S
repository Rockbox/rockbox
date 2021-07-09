/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#include "config.h"
#include "cpu.h"

/* Macro for reading a register */
.macro mrh  register
    ldr     r1, =\register
    ldrh    r0, [r1]
.endm

/* Macro for writing a register */
.macro mwh  register, value
    ldr     r0, =\value
    ldr     r1, =\register
    strh    r0, [r1]
.endm

/* This version uses a mov to save on the literal pool size. Otherwise it is
 *  functionally equivalent.
 */
.macro mwhm register, value
    mov     r0, #\value
    ldr     r1, =\register
    strh    r0, [r1]
.endm

 /*
  * _init_board:
  *    This function initializes the specific board this SoC is on.
  */
.section    .init, "ax"
.code       32
.align      0x04
.global     _init_board
.type       _init_board, %function

_init_board:

    /* Setup the EMIF interface timings */

    /* FLASH interface:
     * These are based on the OF setup
     */
    /* IO_EMIF_CS0CTRL1 and
     * IO_EMIF_CS0CTRL2
     */
    mwh     0x30A00, 0x889A
    mwh     0x30A02, 0x1110

    mwhm    0x30A04, 0
    mwh     0x30A06, 0x1415
    mwh     0x30A08, 0x1109

    mwh     0x30A0A, 0x1220
    mwh     0x30A0C, 0x1104
    mwh     0x30A0E, 0x0222

    /* IO_EMIF_CS3CTRL1 and
     * IO_EMIF_CS3CTRL2
     */
    mwh     0x30A10, 0x8899
    mwh     0x30A12, 0x5110

   /* USB interface */
    /* IO_EMIF_CS4CTRL1 and
     * IO_EMIF_CS4CTRL2
     */
    mwh     0x30A14, 0x77DF
    mwh     0x30A16, 0x7740

    /* IO_EMIF_BUSCTRL */
    mwhm    0x30A18, 0
    mwhm    0x30A1A, 0
    mwhm    0x30A1C, 0
    mwhm    0x30A1E, 0

_clock_setup:
    /* Clock initialization */

    /* Disable peripheral clocks */
    mwhm 0x3089A, 0
    mwhm 0x3089C, 0

    /* IO_CLK_BYP: Bypass the PLLs for the following changes */
    mwh 0x30894, 0x1111

    /*
     *  IO_CLK_PLLA: 27 MHz * 11 / 1 = 297 MHz
     *  IO_CLK_PLLB: 27 MHz
     */
    mwh 0x30880, 0x10A0
    mwhm 0x30882, 0x1000

    /* IO_CLK_SEL0: Timer 0 and 1, UART 0 and 1 from PLLIN (27 MHz) */
    mwh 0x30884, 0x0066

    /* IO_CLK_SEL1: VENC from PLLA, OSD clock = VENC clock / 2 */
    mwhm 0x30886, 0x0003

    /* IO_CLK_SEL2: ARM, AXL, SDRAM and DSP are from PLLA */
    mwh 0x30888, 0

    /* IO_CLK_DIV0: Set the fast clock speed for the ARM/AHB
     * ARM = PLLA / 2 = 148.5 MHz
     * AHB = ARM / 2 = 74.25 MHz
     */
    mwh 0x3088A, 0x0101

    /* IO_CLK_DIV1: Accelerator, SDRAM
     * AXL = PLLA / 2 = 148.5 MHz
     * SDRAM = PLLA / 3 = 99 MHz
     */
    mwh 0x3088C, 0x0102

    /* IO_CLK_DIV2: DSP, MS Clock
     * OF must be booted with this value
     * DSP = PLLA / 3 = 99 MHz
     * MS = PLLA / 1 = 297 MHz
     */
    mwhm 0x3088E, 0x0200

    /* PLLA &= ~0x1000 (BIC #0x1000) */
    mrh     0x30880
    bic     r0, r0, #0x1000
    strh    r0, [r1]

    /* Wait for PLLs to lock before feeding them to the downstream devices */
_plla_wait:
    mrh     0x30880
    bic     r0, r0, #0x7F
    tst     r0, r0
    beq     _plla_wait

    /* IO_CLK_BYP: Enable PLL feeds  */
    mwhm 0x30894, 0x0

    /* IO_CLK_MOD0
     * Enable clocks:
     *   ARM, Bus Controller, AHB, ARM internal memory, EMIF, SDRAM
     * Disable clocks:
     *   ETM, E2ICE, INTC, EXTHOST, DSP, HPIB
     */
    mwh 0x30898, 0x01A7

    /* IO_CLK_MOD2: Enable GIO and SIF1 clocks */
    mwhm 0x3089C, 0x0420

    /* Setup the SDRAM range on the AHB bus */
    /* SDRAMSA */
    mov    r0, #0x60000
    mov    r1, #0x1000000
    str    r1, [r0, #0xF00]

    /* SDRAMEA: 64MB */
    mov    r1, #0x5000000
    str    r1, [r0, #0xF04]

    /* SDRC_REFCTL */
    mwh    0x309A8, 0

    ldr    r0, =0x309A6
    mov    r2, #0x1380
    orr    r1, r2, #2
    strh   r1, [r0]
    orr    r1, r2, #4
    strh   r1, [r0]
    strh   r1, [r0]
    strh   r1, [r0]
    strh   r1, [r0]
    strh   r1, [r0]
    strh   r1, [r0]
    strh   r1, [r0]
    strh   r1, [r0]
    orr    r1, r2, #1
    strh   r1, [r0]
    strh   r2, [r0]
    strh   r2, [r0]

    mwhm   0x309A8, 0x0140

    /* IMGBUF SDRAM priority bit 2 set */
    mwhm   0x309BE, 0x4
    /* SDRAM refresh priority bit 1 set */
    mwhm   0x309BC, 0x2
    /* Use defined priority bits */
    ldr    r0, =0x309C4
    ldr    r1, [r0]
    orr    r1, r1, #1
    strh   r1, [r0]

    ldr    r0, =0x309A6
    mov    r1, #0x1380
    strh   r1, [r0]
    bic    r1, r1, #0x80
    strh   r1, [r0]
    orr    r1, r1, #0x40
    strh   r1, [r0]

    /* Enable auto refresh with interval (64 + 1) * 8 SDRAM clocks */
    mwhm   0x309A8, 0x0140

    /* Go through the GPIO initialization */
    /* Warning: setting some of the functions wrong will make OF unable
       to boot (freeze during startup) */
    /* IO_GIO_FSEL0: Set up the GPIO pin functions 0-16 */
    mwhm   0x305A4, 0xC000

    /* IO_GIO_FSEL1: 17-24 */
    mwh    0x305A6, 0xAAAA

    /* IO_GIO_FSEL2: 18-32 */
    mwh    0x305A8, 0xA80A

    /* IO_GIO_FSEL3: 33-40 */
    mwh    0x305AA, 0x1007

    /* IO_GIO_DIR0 */
    mwh    0x30580, 0xFF77

    /* IO_GIO_DIR1 */
    mwh     0x30582, 0xEFFE

    /* IO_GIO_DIR2 */
    mwh     0x30584, 0x01FD

    /* IO_GIO_INV0 */
    mwh     0x30586, 0x0000

    /* IO_GIO_INV1 */
    mwh     0x30588, 0x0000

    /* IO_GIO_INV2 */
    mwh     0x3058A, 0x0000

    bx     lr

.ltorg
.size       _init_board, .-_init_board

