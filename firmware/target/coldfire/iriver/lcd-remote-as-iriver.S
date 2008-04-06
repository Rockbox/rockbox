/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "config.h"

#define RS_MASK        0x00010000
#define CLOCK_MASK     0x10000000
#define GPIO_OUT_ADDR  0x80000004

#define CS_MASK        0x00000004  /* used in moveq.l */
#define DATA_MASK      0x00040000
#define GPIO1_OUT_ADDR 0x800000b4

#define CS_TIMEOUT     10 /* HZ/10 */

    .extern cpu_frequency       /* Global variable from system.c */
    .extern remote_byte_delay   /* Global variable from lcd-remote-iriver.c */
    .extern remote_cs_countdown /* Global variable from lcd-remote-iriver.c */

    .section    .icode,"ax",@progbits

    /* Output 8 bits to the LCD. Instruction order is devised to maximize the
     * delay between changing the data line and the CLK L->H transition, which
     * makes the LCD controller sample DATA.
     * Requires CLK = 0 on entry.
     *
     * Custom calling convention:
     *   %a0 - GPIO_OUT_ADDR
     *   %a1 - GPIO1_OUT_ADDR
     *   %d4 - data byte
     *   %d6 - DATA_MASK
      * Clobbers:
     *   %d0..%d4
     */
#ifdef HAVE_REMOTE_LCD_TICKING
.write_byte_delayed:
    move.l  remote_byte_delay, %d0
1:
    subq.l  #1, %d0
    bne.s   1b
#endif

.write_byte:
    move.w  %sr, %d3            /* Get current interrupt level */
    move.w  #0x2700, %sr        /* Disable interrupts */

    move.l  (%a1), %d0          /* Get current state of data port */
    move.l  %d0, %d1          
    and.l   %d6, %d1            /* Check current state of data line */
    beq.s   1f                  /*   and set it as previous-state bit */
    bset    #8, %d4
1:
    move.l  %d4, %d1            /* Compute the 'bit derivative', i.e. a value */
    lsr.l   #1, %d1             /*   with 1's where the data changes from the */
    eor.l   %d1, %d4            /*   previous state, and 0's where it doesn't */
    swap    %d4                 /* Shift data to upper byte */
    lsl.l   #8, %d4

    move.l  (%a0),%d1           /* Get current state of clock port */
    move.l  %d1, %d2            /* Precalculate opposite state of clock line */
    eor.l   #CLOCK_MASK, %d2
    
    lsl.l   #1, %d4             /* Invert data line for bit7 ? */
    bcc.s   1f                  /*   no: skip */
    eor.l   %d6, %d0            /* invert data bit */
    move.l  %d0, (%a1)          /*   output data bit7 */
    nop
1:

.macro bit_out
    lsl.l   #1, %d4             /* Invert data line for bit6 ? */
    bcc.s   1f                  /*   no: skip */
    eor.l   %d6, %d0            /* Invert data bit */
    move.l  %d2, (%a0)          /* Bit7: set CLK = 1 */
    move.l  %d1, (%a0)          /*       set CLK = 0 */
    move.l  %d0, (%a1)          /* Output data bit6 */
    bra.s   2f                  /* slower than trapf.l - required here */
1:                              /*   else */
    move.l  %d2, (%a0)          /* Bit7: set CLK = 1 */
    move.l  %d1, (%a0)          /*       set CLK = 0 */
2:
.endm
    bit_out
    bit_out
    bit_out
    bit_out
    bit_out
    bit_out
    bit_out

    nop                         /* Let data line settle */
    move.l  %d2, (%a0)          /* Bit0: Set CLK = 1 */
    move.l  %d1, (%a0)          /*       Set CLK = 0 */

    move.w  %d3, %sr            /* Restore interrupt level */
    rts

    /* Output 8 bits to the LCD as fast as possible. Use only at < 60MHz.
     *
     * Custom calling convention:
     *   %a0 - GPIO_OUT_ADDR
     *   %a1 - GPIO1_OUT_ADDR
     *   %d4 - data word
     *   %d6 - DATA_MASK
     * Clobbers:
     *   %d0..%d4
     */
#ifdef HAVE_REMOTE_LCD_TICKING
.write_byte_fast_delayed:
    move.l  remote_byte_delay, %d0
1:
    subq.l  #1, %d0
    bne.s   1b
#endif

.write_byte_fast:
    move.w  %sr, %d3            /* Get current interrupt level */
    move.w  #0x2700,%sr         /* Disable interrupts */

    move.l  (%a1), %d0          /* Get current state of data port */
    move.l  %d0, %d1          
    and.l   %d6, %d1            /* Check current state of data line */
    beq.s   1f                  /*   and set it as previous-state bit */
    bset    #8, %d4
1:
    move.l  %d4, %d1            /* Compute the 'bit derivative', i.e. a value */
    lsr.l   #1, %d1             /*   with 1's where the data changes from the */
    eor.l   %d1, %d4            /*   previous state, and 0's where it doesn't */
    swap    %d4                 /* Shift data to upper byte */
    lsl.l   #8, %d4

    move.l  (%a0), %d1          /* Get current state of clock port */
    move.l  %d1, %d2            /* Precalculate opposite state of clock line */
    eor.l   #CLOCK_MASK, %d2

.macro bit_out_fast
    lsl.l   #1,%d4              /* Shift out MSB */
    bcc.s   1f                  
    eor.l   %d6, %d0            /* 1: flip data bit */
    move.l  %d0, (%a1)          /*   and output new DATA state */
1:
    move.l  %d2, (%a0)          /* Set CLK */
    move.l  %d1, (%a0)          /* Reset CLK */
.endm
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast
    bit_out_fast

    move.w  %d3, %sr            /* Restore interrupt level */
    rts
    
    
    .global lcd_remote_write_command
    .type   lcd_remote_write_command, @function

lcd_remote_write_command:
    lea.l   (-4*4, %sp), %sp
    movem.l %d2-%d4/%d6, (%sp)

    move.l  (4*4+4, %sp), %d4   /* cmd */

    lea.l   GPIO_OUT_ADDR, %a0
    lea.l   GPIO1_OUT_ADDR, %a1
    move.l  #DATA_MASK, %d6
    
    clr.l   remote_cs_countdown

    move.l  #~RS_MASK, %d0
    and.l   %d0, (%a0)
    moveq.l #~CS_MASK, %d0
    and.l   %d0, (%a1)
    
#ifdef HAVE_REMOTE_LCD_TICKING
    tst.l   remote_byte_delay
    ble.s   1f
    bsr.w   .write_byte_delayed
    bra.s   2f
1:
#endif
    bsr.w   .write_byte
2:

    moveq.l #CS_TIMEOUT, %d0
    move.l  %d0, remote_cs_countdown

    movem.l (%sp), %d2-%d4/%d6
    lea.l   (4*4, %sp), %sp
    rts


    .global lcd_remote_write_command_ex
    .type   lcd_remote_write_command_ex, @function

lcd_remote_write_command_ex:
    lea.l   (-4*4, %sp), %sp
    movem.l %d2-%d4/%d6, (%sp)
    
    lea.l   GPIO_OUT_ADDR, %a0
    lea.l   GPIO1_OUT_ADDR, %a1
    move.l  #DATA_MASK, %d6

    clr.l   remote_cs_countdown

    move.l  #~RS_MASK, %d0
    and.l   %d0, (%a0)
    moveq.l #~CS_MASK, %d0
    and.l   %d0, (%a1)

    move.l  (4*4+4, %sp), %d4

#ifdef HAVE_REMOTE_LCD_TICKING
    tst.l   remote_byte_delay
    ble.s   1f
    bsr.w   .write_byte_delayed
    move.l  (4*4+8, %sp), %d4
    bsr.w   .write_byte_delayed
    bra.s   2f
1:
#endif
    bsr.w   .write_byte
    move.l  (4*4+8, %sp), %d4
    bsr.w   .write_byte
2:

    moveq.l #CS_TIMEOUT, %d0
    move.l  %d0, remote_cs_countdown

    movem.l (%sp), %d2-%d4/%d6
    lea.l   (4*4, %sp), %sp
    rts


    .global lcd_remote_write_data
    .type   lcd_remote_write_data, @function

lcd_remote_write_data:
    lea.l   (-7*4, %sp), %sp
    movem.l %d2-%d6/%a2-%a3, (%sp)

    move.l  (7*4+4, %sp), %a2   /* p_bytes */
    move.l  (7*4+8, %sp), %d5   /* count */

    lea.l   GPIO_OUT_ADDR, %a0
    lea.l   GPIO1_OUT_ADDR, %a1
    move.l  #DATA_MASK, %d6

    lea.l   .write_byte, %a3
    move.l  cpu_frequency, %d0
    cmp.l   #60000000, %d0
    bhi.b   1f
    lea.l   .write_byte_fast, %a3
1:

#ifdef HAVE_REMOTE_LCD_TICKING
    tst.l   remote_byte_delay
    ble.s   1f
    moveq.l #(.write_byte_delayed - .write_byte), %d0
    add.l   %d0, %a3
1:
#endif

    clr.l   remote_cs_countdown

    move.l  #RS_MASK, %d0
    or.l    %d0, (%a0)
    moveq.l #~CS_MASK, %d0
    and.l   %d0, (%a1)

.wd_loop:                
    clr.l   %d4
    move.b  (%a2)+, %d4
    jsr     (%a3)
    subq.l  #1, %d5
    bne.s   .wd_loop

    moveq.l #CS_TIMEOUT, %d0
    move.l  %d0, remote_cs_countdown

    movem.l (%sp), %d2-%d6/%a2-%a3
    lea.l   (7*4, %sp), %sp
    rts
