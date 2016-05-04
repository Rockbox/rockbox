/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rafaël Carré
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

/* AS3525 hardware registers */
.set GPIOA,     0xC80B0000
.set GPIOB,     0xC80C0000
.set GPIOC,     0xC80D0000
.set GPIOD,     0xC80E0000
.set CGU_PROC,  0xC80F0010
.set CGU_PERI,  0xC80F0014
.set CGU_DBOP,  0xC80F0038
.set CCU_IO,    0xC810000C
.set DBOP,      0xC8120000
.set I2C_BASE,  0xC8070000
.set I2C_DATA,  0x00
.set I2C_SLAD0, 0x04
.set I2C_CNTRL, 0x0c
.set I2C_DACNT, 0x10
.set I2C_CPSR0, 0x1c
.set I2C_CPSR1, 0x20
.set I2C_IMR,   0x24
.set I2C_SR,    0x30
.set I2C_SADDR, 0x44
.set AS3514_I2C_ADDR, 0x46
.set AS3514_IRQ_ENRD0, 0x25
.set PCLK, 24000000
.set I2C_CLK, 400000
.set I2C_PRESCALER, ((PCLK + I2C_CLK -1) / I2C_CLK)
.set I2C_PRESCALER_LOW, (I2C_PRESCALER & 0xff)
.set I2C_PRESCALER_HIGH, (I2C_PRESCALER >> 8)
#if I2C_PRESCALER_HIGH > 3
#error i2c prescaler too big!
#endif

        b start     @ skip our data

/* These values are filled in by mkamsboot - don't move them from offset 0x4 */

uclunpack_end:   .word   0 /* End of the ucl_unpack function */
uclunpack_size:  .word   0 /* Size in bytes of the ucl_unpack function */

ucl_of_end:      .word   0 /* End of the ucl-compressed OF image */
ucl_of_size:     .word   0 /* Size in bytes of the compressed OF image */

ucl_rb_end:      .word   0 /* End of the ucl-compressed RB image */
ucl_rb_size:     .word   0 /* Size in bytes of the compressed RB image */

ucl_dest:        .word   0 /* End of our destination buffer (end of memory) */

start:
        /* First copy the UCL unpack function to the end of RAM */
        ldr     r0, uclunpack_end   /* Source */
        ldr     r1, uclunpack_size  /* Source length */
        sub     r2, r0, r1          /* Source start - 1*/

        ldr     r3, ucl_dest        /* Destination end */

uclcopy:
        ldrb    r4, [r0], #-1
        strb    r4, [r3], #-1
        cmp     r2, r0
        bne     uclcopy

        /* store the new destination buffer */
        str     r3, ucl_dest

        /* enable gpio clock */
        ldr     r0, =CGU_PERI
        ldr     r1, [r0]
        orr     r1, r1, #(1<<16)
        str     r1, [r0]

        /* Here are model specific tests for dual boot (test left button) */

#ifdef SANSA_CLIP
.set row, (1<<5) /* enable output on C5 */
.set col, (1<<0) /* read keyscan column B0 */

        ldr     r0, =GPIOC
        mov     r1, #row
        str     r1, [r0, #0x400]
        str     r1, [r0, #(4*row)]

        ldr     r0, =GPIOB
        mov     r1, #0
        str     r1, [r0, #0x400]
        ldr     r1, [r0, #(4*col)]

        cmp     r1, #0
        bne     boot_of
#elif defined(SANSA_CLIPV2)
.set row, (1<<4) /* enable output on D4 */
.set col, (1<<0) /* read keyscan column D0 */

        ldr     r0, =CCU_IO
        ldr     r1, [r0]
        bic     r1, r1, #(3<<2) @ XPD works as general purpose IO
        str     r1, [r0]

        ldr     r0, =GPIOD
        mov     r1, #((1<<5)|(1<<4)|(1<<3)) /* all rows as output */
        str     r1, [r0, #0x400]

        /* all rows high */
        mov     r1, #(1<<3)
        str     r1, [r0, #(4*(1<<3))]
        mov     r1, #(1<<4)
        str     r1, [r0, #(4*(1<<4))]
        mov     r1, #(1<<5)
        str     r1, [r0, #(4*(1<<5))]

        mov     r1, #0                      /* button row low */
        str     r1, [r0, #(4*row)]

        mov     r1, #5                      /* small delay */
1:      subs    r1, r1, #1
        bne     1b

        ldr     r1, [r0, #(4*col)]

        cmp     r1, #0
        beq     boot_of
#elif defined(SANSA_E200V2) || defined(SANSA_FUZE)
        ldr     r0, =GPIOC
        mov     r1, #0
        str     r1, [r0, #0x400]
        ldr     r1, [r0, #0x20]    /* read pin C3 */

        cmp     r1, #0             /* C3 = #0 means button pressed */
        beq     boot_of

#elif defined(SANSA_FUZEV2)
        ldr     r0, =GPIOC
        mov     r1, #0
        str     r1, [r0, #0x400]
        ldr     r1, [r0, #0x20]    /* read pin C3 */

        cmp     r1, #0             /* C3 != #0 means button pressed */
        bne     boot_of

#elif defined(SANSA_CLIPPLUS)
        @ read pins
        ldr     r0, =GPIOC
        ldr     r1, [r0, #4*(1<<3)] @ read pin C3 "|<<"

        ldr     r0, =GPIOA
        ldr     r2, [r0, #4*(1<<1)] @ read pin A1 "Home"

        orr     r2, r2, r1          @ c3 || A1

        cmp     r2, #0              @ test input from pins
        bne     boot_of             @ branch directly to OF if either pin high

#elif defined(SANSA_CLIPZIP)
        @ read pins
        ldr     r0, =GPIOA
        ldr     r1, [r0, #4*(1<<6)] @ read GPIO A6 "vol-"

        cmp     r1, #0              @ test input from pins
        bne     boot_of             @ branch directly to OF if either pin high

        ldr     r0, =GPIOC

        ldr     r1, [r0, #0x400]
        orr     r1, r1, #((1<<1)|(1<<2))    @ output
        bic     r1, r1, #(1<<5)             @ input
        str     r1, [r0, #0x400]

        mov     r1, #0
        str     r1, [r0, #4*(1<<1)] @ zero C1
        mov     r1, #(1<<2)
        str     r1, [r0, #4*(1<<2)] @ set C2

        mov     r1, #50                     /* small delay */
1:      subs    r1, r1, #1
        bne     1b

        ldr     r1, [r0, #4*(1<<5)] @ read C5 = left

        cmp     r1, #0
        bne     boot_of

#elif defined(SANSA_C200V2)
.set BUTTON_LEFT,      (1<< 2)
.set BUTTON_DOWN,      (1<< 3)
.set BUTTON_SELECT,    (1<< 4)
.set BUTTON_UP,        (1<< 5)
.set BUTTON_RIGHT,     (1<< 6)
.set BUTTON_HOLD,      (1<<12)

        ldr     r0, =CGU_DBOP
        mov     r1, #(1<<3)         @ DBOP freq = PCLK, clock enabled
        str     r1, [r0]

        @ AFSEL needs to be set for this to work
        ldr     r2, =GPIOB
        mov     r1, #0xc
        str     r1, [r2, #0x420]    @ GPIOB_AFSEL
        ldr     r2, =GPIOC
        mov     r1, #0xff
        str     r1, [r2, #0x420]    @ GPIOC_AFSEL

        ldr     r0, =DBOP
        @ TIMPOL doesn't matter here since we don't need
        @ the control signals.

        @ 16 bit data width
        @ enable write
        @ tri-state output
        ldr     r1, =0x00091000
        str     r1, [r0, #8]        @ DBOP_CTRL

        ldr     r1, =0xf0ff         @ precharge
        str     r1, [r0, #0x10]     @ DBOP_DOUT

2:      ldr     r1, [r0, #0xc]      @ DOBP_STAT
        ands    r1, r1, #(1<<10)
        beq     2b                  @ make sure fifo is empty

        @ 16 bit data width
        @ start read
        @ tri-state output
        @ strobe time 31
        ldr     r1, =0x0008901f
        str     r1, [r0, #8]        @ DBOP_CTRL

3:      ldr     r1, [r0, #0xc]      @ DOBP_STAT
        ands    r1, r1, #(1<<16)
        beq     3b                  @ wait for valid data

        ldrh    r1, [r0, #0x14]     @ DBOP_DIN

        tst     r1, #BUTTON_LEFT    @ boot of?
        beq     boot_of
#elif defined(SANSA_M200V4)
.set row, (1<<5) /* enable output on A5 */
.set col, (1<<0) /* read keyscan column A0 */

        ldr     r0, =GPIOA
        mov     r1, #row
        str     r1, [r0, #0x400]
        str     r1, [r0, #(4*row)]

        ldr     r2, [r0, #(4*col)]

        /* check value read (1 means button pressed) */
        cmp     r2, #0
        bne     boot_of
#else
        #error No target-specific key check defined!
#endif

        /* The dualboot button was not held, so we boot rockbox */
        ldr     r0, ucl_rb_end      /* Address of compressed image */
        ldr     r1, ucl_rb_size     /* Compressed size */
        b       decompress

boot_of:
        ldr     r0, ucl_of_end      /* Address of compressed image */
        ldr     r1, ucl_of_size     /* Compressed size */


decompress:
        /* At this point:                                              */
        /* r0 = source_end for UCL image to copy                       */
        /* r1 = size of UCL image to copy                              */

        ldr     r3, ucl_dest
        add     r5, r3, #2      /* r5 is entry point of copy of uclunpack */
                                /* function, plus one (for thumb mode */

        sub     r4, r3, r1      /* r4 := destination_start - 1 */

fw_copy:
        ldrb    r2, [r0], #-1
        strb    r2, [r3], #-1
        cmp     r3, r4          /* Stop when we reached dest_start-1 */
        bne     fw_copy

        /* Call the ucl decompress function, which will branch to 0x0 */
        /* on completion */
        add     r0, r3, #1      /* r0 := Start of compressed image */
                                /* r1 already contains compressed size */
        mov     r2, #0          /* r2 := Destination for unpacking */
        bx      r5              /* Branch to uclunpack, switching to thumb */

        /* never reached : uclunpack will branch to the reset vector (0x0) */
