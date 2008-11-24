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

/* This is the size of the Clip's RAM, but there is nothing to be gained
   (at the moment) by making use of the larger RAM of other targets */

.set DRAM_SIZE, 0x50000

.set GPIOA,     0xC80B0000
.set GPIOB,     0xC80C0000
.set GPIOC,     0xC80D0000
.set GPIOD,     0xC80E0000
.set CGU_PERI,  0xC80F0014


/* Vectors */
        ldr   pc, =start
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0

/* These values are filled in by mkamsboot - don't move them from offset 0x20 */
uclunpack_end:   .word   0 /* End of the ucl_unpack function */
uclunpack_size:  .word   0 /* Size in bytes of the ucl_unpack function */

ucl_of_end:      .word   0 /* End of the ucl-compressed OF image */
ucl_of_size:     .word   0 /* Size in bytes of the compressed OF image */

ucl_rb_end:      .word   0 /* End of the ucl-compressed RB image */
ucl_rb_size:     .word   0 /* Size in bytes of the compressed RB image */


start:
        /* First copy the UCL unpack function to the end of RAM */
        ldr   r0, uclunpack_end     /* Source */
        ldr   r1, uclunpack_size    /* Source length */
        sub   r2, r0, r1            /* Source start - 1*/

        ldr   r3, =(DRAM_SIZE-1)    /* Destination end */

uclcopy:
        ldrb  r4, [r0], #-1
        strb  r4, [r3], #-1
        cmp   r2, r0
        bne   uclcopy

        add   r5, r3, #2            /* r5 is entry point of copy of uclunpack */
                                    /* function, plus one (for thumb mode */

        /* enable gpio clock */
        ldr   r0, =CGU_PERI
        ldr   r1, [r0]
        orr   r1, r1, #(1<<16)
        str   r1, [r0]

        
#ifndef SANSA_M200V4  /* this doesnt work for m200 */         
        /* we check A3 unconditionally of the model because it seems to be */
        /* either hold, either usb on every model */

        ldr   r0, =GPIOA
        mov   r1, #0
        str   r1, [r0, #0x400]
        ldr   r1, [r0, #0x20]    /* read pin A3 */

        cmp   r1, #0
        bne   boot_of
#endif

        /* here are model specific tests, for dual boot without a computer */

#ifdef SANSA_CLIP
        /* HOME button */
.set row, (1<<6) /* enable output on C6 */
        ldr   r0, =GPIOC
        mov   r1, #row
        str   r1, [r0, #0x400]
        str   r1, [r0, #(4*row)]

.set col, (1<<2) /* read keyscan column B2 */
        ldr   r0, =GPIOB
        mov   r1, #0
        str   r1, [r0, #0x400]
        ldr   r1, [r0, #(4*col)]

        cmp   r1, #0
        bne   boot_of
#elif defined(SANSA_E200V2) || defined(SANSA_FUZE)
        /* LEFT button */
        ldr   r0, =GPIOC
        mov   r1, #0
        str   r1, [r0, #0x400]
        ldr   r1, [r0, #0x20]    /* read pin C3 */

        cmp   r1, #0             /* C3 = #0 means button pressed */
        beq   boot_of
#elif defined(SANSA_M200V4)
        /* SELECT button */
.set row, (1<<5) /* enable output on A5 */
        ldr   r0, =GPIOA
        mov   r1, #row
        str   r1, [r0, #0x400]
        str   r1, [r0, #(4*row)]

.set col, (1<<0) /* read keyscan column A0 */
        ldr   r2, [r0, #(4*col)]

        /* check value read (0 means button pressed) */
        cmp   r2, #0
        beq   boot_of
#else
        #error No target-specific key check defined!
#endif

        /* No button was held, so we boot rockbox */
        ldr   r0, ucl_rb_end   /* Address of compressed image */
        ldr   r1, ucl_rb_size    /* Compressed size */
        b     decompress

boot_of:
        ldr   r0, ucl_of_end     /* Address of compressed image */
        ldr   r1, ucl_of_size    /* Compressed size */


decompress:
        /* At this point:                                              */
        /* r5 = entry point (plus one for thumb) of uclunpack function */
        /* r3 = destination_end for copy of UCL image                  */
        /* r0 = source_end for UCL image to copy                       */
        /* r1 = size of UCL image to copy                              */

        sub   r4, r3, r1      /* r4 := destination_start - 1 */
        
fw_copy:
        ldrb  r2, [r0], #-1
        strb  r2, [r3], #-1
        cmp   r3, r4          /* Stop when we reached dest_start-1 */
        bne   fw_copy

        /* Call the ucl decompress function, which will branch to 0x0 */
        /* on completion */
        add   r0, r3, #1      /* r0 := Start of compressed image */
                              /* r1 already contains compressed size */
        mov   r2, #0          /* r2 := Destination for unpacking */
        bx    r5              /* Branch to uclunpack, switching to thumb */

        /* never reached */
