/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2010 by Karl Kurbjun
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

#define  SDRAM_MODE     0x9B00

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

/******************************************************************************
 * _init_board:                                                               *
 *    This function initializes the specific baord this SoC is on.            *
 ******************************************************************************/
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
    mwh     0x30A00, 0x4488
    mwh     0x30A02, 0x1220

    /* ATA interface:
     * These are based on the OF setup 
     */
    /* IO_EMIF_CS3CTRL1 and
     * IO_EMIF_CS3CTRL2 
     */
    mwh     0x30A10, 0x77EF
    mwh     0x30A12, 0x5220

    /* USB interface:
    * The following EMIF timing values are from the OF:
    *      IO_EMIF_CS4CTRL1 = 0x66AB;
    *      IO_EMIF_CS4CTRL2 = 0x4220;
    *
    * More agressive numbers may be possible, but it depends on the clocking
    *  setup. 
    */
    /* IO_EMIF_CS4CTRL1 and
     * IO_EMIF_CS4CTRL2
     */
    mwh     0x30A14, 0x66AB
    mwh     0x30A16, 0x4220

    /* IO_EMIF_BUSCTRL */
    mwh     0x30A18, 0x0001

_clock_setup:
    /* Clock initialization */
    /* Used for ES10 and unknown, slower clocks, but also uses more power
    *  due to PLL setup and slow ARM clock speed.
    */

    /* IO_CLK_BYP: Bypass the PLLs for the following changes */
    mwh 0x30894, 0x1111

    /* 27 MHz input clock:
    *  IO_CLK_PLLA = 27 * 11 / 1 = 297 MHz
    *  IO_CLK_PLLB = 27 * 13 / 1 = 351 MHz
    */
    mwh 0x30880, 0x10A0
    mwh 0x30882, 0x10C0

    /* IO_CLK_SEL0: VLNQ is fed by PLLB, I2C from M48XI, MS from PLLA */
    mwh 0x30884, 0x17E

    /* IO_CLK_SEL1: VENC = MXI CLK, PLLA, PCLK, nodiv2 */
    mwhm 0x30886, 0x1000

    # IO_CLK_SEL2: ARM and AXL are from PLLB, SDRAM and DSP are PLLA */
    mwh 0x30888, 0x1001

    /* IO_CLK_DIV0: Set the slow clock speed for the ARM/AHB
     * Slow Setup:
     *  ARM div  = 4    ( 87.5 MHz )
     *  AHB div  = 1    ( 87.5 MHz )
     */
    mwh 0x3088A, 0x0003 /* OF sets this to 0x0103 */

    /* IO_CLK_DIV1:
     * SDRAM div= 3     ( 99   MHz )
     * AXL div  = 2     ( 175  MHz )
     */
    mwh 0x3088C, 0x0102 /* OF sets this to 0x0103 */

    /* IO_CLK_DIV2:
     * MS div   = 32    ( ~9  MHz )
     * DSP div  = 3     ( 99  MHz )
     */ 
    mwh 0x3088E, 0x020E

    /* IO_CLK_DIV3: (this comment is incorrect, (31 << 8) | 255;)
     * MMC div  = 256   ( slow )
     * VENC div = 32    ( 843.75 KHz )
     */
    mwhm 0x30890, 0x0003

    /* IO_CLK_DIV4: (this comment is incorrect, (31 << 8) | 0;)
     * I2C div  = 1     (48 MHz if M48XI is running)
     * VLNQ div = 32
     */
    mwhm 0x30892, 0x0200

    # PLLA &= ~0x1000 (BIC #0x1000)
    mrh     0x30880
    bic     r0, r0, #0x1000
    strh    r0, [r1]

    # PLLB &= ~0x1000
    mrh     0x30882
    bic     r0, r0, #0x1000
    strh    r0, [r1]

    /* Wait for PLLs to lock before feeding them to the downstream devices */
_plla_wait:
    mrh     0x30880
    bic     r0, r0, #0x7F
    tst     r0, r0
    beq     _plla_wait

_pllb_wait:
    mrh     0x30882
    bic     r0, r0, #0x7F
    tst     r0, r0
    beq     _plla_wait

    /* IO_CLK_BYP: Enable PLL feeds */
    mwhm 0x30894, 0x0

    /* IO_CLK_MOD0: Most off */
    mwh 0x30898, 0x0167

    /* IO_CLK_MOD1: All off */
    mwhm 0x3089A, 0x0

    /* IO_CLK_MOD2: Turn on the GPIO clock */
    mwhm 0x3089C, 0x20

    /* Setup the SDRAM range on the AHB bus */
    /* SDRAMSA */
    mov    r0, #0x60000
    mov    r1, #0x900000
    str    r1, [r0, #0xF00]
    
    /* SDRAMEA: 64MB */
    mov    r1, #0x4900000
    str    r1, [r0, #0xF04]
    
    /* BUSCTRL */
    mov    r1, #0x00000
    str    r1, [r0, #0xF08]
    
    /* USBCTRL */
    str    r1, [r0, #0xF20]

    /* Setup IO_SDRAM_SDMODE based on OF */
    mov    r1, #0x00030000
    orr    r1, r1, #0x900

    /* Pre-charge all banks */
    ldr    r0, =(SDRAM_MODE | 0x82)
    strh   r0, [r1, #0xA6]

    /* Setup auto refresh, OF uses 0x5F */
    ldr    r0, =0x140
    strh   r0, [r1, #0xA8]

    /* Issue 8 auto refresh cycles */
    ldr    r0, =(SDRAM_MODE | 0x84)
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    strh   r0, [r1, #0xA6]
    
    /* Set the mode register */
    ldr    r0, =(SDRAM_MODE | 0x81)
    strh   r0, [r1, #0xA6]
    
    /* Go back to the NOP state */
    ldr    r0, =(SDRAM_MODE | 0x80)
    strh   r0, [r1, #0xA6]
    
    /* Turn on auto power down */
    ldr    r0, =(SDRAM_MODE | 0x40)
    strh   r0, [r1, #0xA6]

    /* Go through the GPIO initialization */
    /* IO_GIO_FSEL0: Set up the GPIO pin functions 0-16 */
    mwh     0x305A4, 0x0000

    /* IO_GIO_FSEL1: 17-24 */
    mwh     0x305A6, 0x0000

    /* IO_GIO_FSEL2: 18-32 */
    mwh     0x305A8, 0x1450

    /* IO_GIO_FSEL3: 33-40 */
    mwh     0x305AA, 0x0404

    /* IO_GIO_DIR0 */
    mwh     0x30580, 0x6A0B

    /* IO_GIO_DIR1: Important note - pin 26 has control over the system power.
     *  If this pin is not initialized the device will shut off immediately.
     */
    mwh     0x30582, 0x8B00

    /* IO_GIO_DIR2 */
    mwh     0x30584, 0x0000
    
    /* IO_GIO_INV0 */
    mwh     0x30586, 0x0000

    /* IO_GIO_INV1 */
    mwh     0x30588, 0x0000

    /* IO_GIO_INV2 */
    mwh     0x3058A, 0x0000

    /* IO_GIO_BITCLR0 */
    mwh     0x30592, 0xFFFF

    /* IO_GIO_BITCLR1 */
    mwh     0x30594, 0xFFFF

    /* IO_GIO_BITCLR2 */
    mwh     0x30596, 0xFFFF

    /* IO_GIO_BITSET0 */
    mwh     0x3058C, 0x0280
   
    /* IO_GIO_BITSET1 */
    mwh     0x3058E, 0x2066
   
    /* IO_GIO_BITSET2 */
    mwh     0x30590, 0x0025

    /* IO_GIO_IRQPORT */
    mwh     0x30598, 0xD60B

    /* IO_GIO_IRQEDGE */
    mwh     0x3059A, 0x0801

    /* IO_GIO_CHAT0 */
    mwh     0x3059C, 0x0000

    /* IO_GIO_CHAT1 */
    mwh     0x3059E, 0x0000

    /* IO_GIO_CHAT2 */
    mwh     0x305A0, 0x0000

    /* IO_GIO_NCHAT */
    mwh     0x305A2, 0x0000

    bx     lr

.ltorg  
.size       _init_board, .-_init_board

