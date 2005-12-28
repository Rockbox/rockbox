/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "bidi.h"

static bool display_on=false; /* is the display turned on? */

/* register defines */
#define R_START_OSC             0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_WAVEFORM_CONTROL  0x02
#define R_ENTRY_MODE            0x03
#define R_COMPARE_REG1          0x04
#define R_COMPARE_REG2          0x05

#define R_DISP_CONTROL1     0x07
#define R_DISP_CONTROL2     0x08
#define R_DISP_CONTROL3     0x09

#define R_FRAME_CYCLE_CONTROL 0x0b
#define R_EXT_DISP_IF_CONTROL 0x0c

#define R_POWER_CONTROL1    0x10
#define R_POWER_CONTROL2    0x11
#define R_POWER_CONTROL3    0x12
#define R_POWER_CONTROL4    0x13

#define R_RAM_ADDR_SET  0x21
#define R_WRITE_DATA_2_GRAM 0x22

#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33

#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37

#define R_GAMMA_AMP_ADJ_RES_POS     0x38
#define R_GAMMA_AMP_AVG_ADJ_RES_NEG 0x39 

#define R_GATE_SCAN_POS         0x40
#define R_VERT_SCROLL_CONTROL   0x41
#define R_1ST_SCR_DRV_POS       0x42
#define R_2ND_SCR_DRV_POS       0x43
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45


/* called very frequently - inline! */
inline void lcd_write_reg(int reg, int val)
{
    *(volatile unsigned short *)0xf0000000 = reg;
    *(volatile unsigned short *)0xf0000002 = val;
}

/* called very frequently - inline! */
inline void lcd_begin_write_gram(void)
{
    *(volatile unsigned short *)0xf0000000 = R_WRITE_DATA_2_GRAM;
}

/* called very frequently - inline! */
inline void lcd_write_data(const unsigned short* p_bytes, int count) ICODE_ATTR;
inline void lcd_write_data(const unsigned short* p_bytes, int count)
{
    int precount = ((-(size_t)p_bytes) & 0xf) / 2;
    count -= precount;
    while(precount--)
        *(volatile unsigned short *)0xf0000002 = *p_bytes++;
    while((count -= 8) >= 0) asm (
            "\n\tmovem.l (%[p_bytes]),%%d1-%%d4\
             \n\tswap %%d1\
             \n\tmove.w %%d1,(%[dest])\
             \n\tswap %%d1\
             \n\tmove.w %%d1,(%[dest])\
             \n\tswap %%d2\
             \n\tmove.w %%d2,(%[dest])\
             \n\tswap %%d2\
             \n\tmove.w %%d2,(%[dest])\
             \n\tswap %%d3\
             \n\tmove.w %%d3,(%[dest])\
             \n\tswap %%d3\
             \n\tmove.w %%d3,(%[dest])\
             \n\tswap %%d4\
             \n\tmove.w %%d4,(%[dest])\
             \n\tswap %%d4\
             \n\tmove.w %%d4,(%[dest])\
             \n\tlea.l (16,%[p_bytes]),%[p_bytes]"
             : [p_bytes] "+a" (p_bytes)
             : [dest] "a" ((volatile unsigned short *)0xf0000002)
             : "d1", "d2", "d3", "d4", "memory");
    if (count != 0) {
        count += 8;
        while(count--)
            *(volatile unsigned short *)0xf0000002 = *p_bytes++;
    }
}

/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

/* Rolls up the lcd display by the specified amount of lines.
 * Lines that are rolled out over the top of the screen are
 * rolled in from the bottom again. This is a hardware 
 * remapping only and all operations on the lcd are affected.
 * -> 
 * @param int lines - The number of lines that are rolled. 
 *  The value must be 0 <= pixels < LCD_HEIGHT. */
void lcd_roll(int lines)
{
    (void)lines;
}


/* LCD init */
void lcd_init_device(void)
{
    /* GPO46 is LCD RESET */
    or_l(0x00004000, &GPIO1_OUT);
    or_l(0x00004000, &GPIO1_ENABLE);
    or_l(0x00004000, &GPIO1_FUNCTION);

    /* Reset LCD */
    sleep(1);
    and_l(~0x00004000, &GPIO1_OUT);
    sleep(1);
    or_l(0x00004000, &GPIO1_OUT);
    sleep(1);

    lcd_write_reg(R_START_OSC, 0x0001); /* Start Oscilation */
    sleep(1);
    lcd_write_reg(R_DISP_CONTROL1, 0x0040); /* zero all bits */ 
    lcd_write_reg(R_POWER_CONTROL3, 0x0000);
    lcd_write_reg(R_POWER_CONTROL4, 0x0000);
    sleep(1);
    lcd_write_reg(R_POWER_CONTROL2, 0x0003); /* VciOUT = 0.83*VciLVL */
    lcd_write_reg(R_POWER_CONTROL3, 0x0008); /* Vreg1OUT = REGP*1.90 */

    /* Vcom-level amplitude = 1.23*Vreg1OUT
     * VcomH-level amplitude = 0.84*Vreg1OUT */
    lcd_write_reg(R_POWER_CONTROL4, 0x3617); 
    lcd_write_reg(R_POWER_CONTROL3, 0x0008); /* Vreg1OUT = REGP*1.90 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0004); /* Step-up circuit 1 ON */
    lcd_write_reg(R_POWER_CONTROL1, 0x0004);
    
    lcd_write_reg(R_POWER_CONTROL2, 0x0002); /* VciOUT = 0.87*VciLVL */
    lcd_write_reg(R_POWER_CONTROL3, 0x0018); /* turn on VLOUT3  */
    lcd_write_reg(R_POWER_CONTROL1, 0x0044); /* LCD power supply Op.Amp 
                                                const curr = 1 */
    sleep(1);
    
    /* Step-up rate:
     * VLOUT1 (DDVDH) = Vci1*2
     * VLOUT4 (VCL)   = Vci1*(-1)
     * VLOUT2 (VGH)   = DDVDH*3 = Vci1*6
     * VLOUT3 (VGL)   = - DDVDH*2 = Vci1*(-4) */
    lcd_write_reg(R_POWER_CONTROL1, 0x0144); 

    /* Step-Up circuit 1 Off;
     * VLOUT2 (VGH)   = Vci1 + DDVDH*2 = Vci1*5
     * VLOUT3 (VGL)   = -(Vci1+DDVDH) = Vci1*(-3) */ 
    lcd_write_reg(R_POWER_CONTROL1, 0x0540);

    /* Vcom-level ampl = Vreg1OUT*1.11
     * VcomH-level ampl = Vreg1OUT*0.86 */
    lcd_write_reg(R_POWER_CONTROL4, 0x3218); 
    /* ??Number lines invalid? */
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x001b);

    /* B/C = 1 ; n-line inversion form; polarity inverses at completion of
     * driving n lines
     * Exclusive OR = 1; polarity inversion occurs by applying an EOR to
     * odd/even frame select signal and an n-line inversion signal.
     * FLD = 01b (1 field interlaced scan, external display iface) */
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, 0x0700);

    /* Address counter updated in vertical direction; left to right;
     * vertical increment horizontal increment.
     * data format for 8bit transfer or spi = 65k (5,6,5)
     * Reverse order of RGB to BGR for 18bit data written to GRAM
     * Replace data on writing to GRAM */
    lcd_write_reg(R_ENTRY_MODE, 0x7038);

    /* ???? compare val = (1)1100 0011 0000b; the MSB bit is out of spec.*/
    lcd_write_reg(R_COMPARE_REG1, 0x7030);
    lcd_write_reg(R_COMPARE_REG2, 0x0000);
    
    lcd_write_reg(R_GATE_SCAN_POS, 0x0000);
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);

    /* Gate Line = 0+1 = 1
     * gate "end" line = 0xdb + 1 = 0xdc */ 
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0xdb00);
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x0000);
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0xaf00);/* horiz ram addr 0 - 175 */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 0xdb00);/* vert ram addr 0 - 219 */

    /* 19 clocks,no equalization */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0002);

    /* Transfer mode for RGB interface disabled
     * internal clock operation;
     * System interface/VSYNC interface */ 
    lcd_write_reg(R_EXT_DISP_IF_CONTROL, 0x0003);
    sleep(1);
    lcd_write_reg(R_POWER_CONTROL1, 0x4540); /* Turn on the op-amp (1) */

    /* Enable internal display operations, not showed on the ext. display yet
     * Source: GND; Internal: ON; Gate-Driver control signals: ON*/
    lcd_write_reg(R_DISP_CONTROL1, 0x0041); 
    sleep(1);

    /* Front porch lines: 8; Back porch lines: 8; */
    lcd_write_reg(R_DISP_CONTROL2, 0x0808);

    /* Scan mode by the gate driver in the non-display area: disabled;
     * Cycle of scan by the gate driver - set to 31frames(518ms),disabled by 
     * above setting */
    lcd_write_reg(R_DISP_CONTROL3, 0x003f);
    sleep(1);

    /* Vertical scrolling disabled;
     * Gate Output: VGH/VGL;
     * Reversed grayscale image on;
     * Source output: Non-lit display; internal disp.operation: ON, gate-driver
     * control signals: ON
     * Note: bit 6 (zero based) isn't set to 1 (according to the datasheet
     * it should be) */
    lcd_write_reg(R_DISP_CONTROL1, 0x0636);
    sleep(1);
    lcd_write_reg(R_DISP_CONTROL1, 0x0626);/* Gate output:VGL; 6th bit not set*/
    sleep(1);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0003);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0707);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0007);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0705);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0007);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0000);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0407);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0507);
    lcd_write_reg(R_GAMMA_AMP_ADJ_RES_POS, 0x1d09);
    lcd_write_reg(R_GAMMA_AMP_AVG_ADJ_RES_NEG, 0x0303);
    
    /* VcomH = Vreg1OUT*0.70
     * Vcom amplitude = Vreg1OUT*0.78 */
    lcd_write_reg(R_POWER_CONTROL4, 0x2610);

    /* LCD ON 
     * Vertical scrolling: Originaly designated position/external display
     * Reverse grayscale off;
     * Internal display operation ON, Gate driver control signals: ON; Source
     * output GND */  
    lcd_write_reg(R_DISP_CONTROL1, 0x0061);
    sleep(1);

    /* init the GRAM, the framebuffer is already cleared in the 
     * device independent lcd_init() */
    lcd_write_reg(R_RAM_ADDR_SET, 0);
    lcd_begin_write_gram();
    lcd_write_data((unsigned short *)lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT);
 
    /* Reverse grayscale on;
     * Source output: display */ 
    lcd_write_reg(R_DISP_CONTROL1, 0x0067);
    sleep(1);

    /* Vertical Scrolling disabled
     * Gate output: VGH/VGL
     * 6th bit not set*/ 
    lcd_write_reg(R_DISP_CONTROL1, 0x0637);
}

void lcd_enable(bool on)
{
    if(display_on!=on)
    {    
        if(on)
        {
            lcd_init_device();
        }
        else
        {
            lcd_write_reg(R_FRAME_CYCLE_CONTROL,0x0002); /* No EQ, 19 clocks */
        
            /* Gate Output VGH/VGL; Non-lit display internal disp. ON, 
             * gate-driver control ON; */
            lcd_write_reg(R_DISP_CONTROL1,0x0072);
            sleep(1);
            lcd_write_reg(R_DISP_CONTROL1,0x0062); /* Gate Output: VGL */
            sleep(1);

            /* Gate Output VGH; Source Output: GND; 
             * internal display operation:halt
             * Gate-Driver control signals: OFF */
            lcd_write_reg(R_DISP_CONTROL1,0x0040);
        
            /* Now, turn off the power */

            /* Halt op. amp & step-up circuit */
            lcd_write_reg(R_POWER_CONTROL1,0x0000); 
            lcd_write_reg(R_POWER_CONTROL3,0x0000); /* Turn OFF VLOUT3 */

            /* halt negative volt ampl. */
            lcd_write_reg(R_POWER_CONTROL4,0x0000); 
        }
        display_on=on;
    }
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit(const fb_data* data, int x, int by, int width,
              int bheight, int stride)
{
    /* TODO: Implement lcd_blit() */
    (void)data;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
    /*if(display_on)*/ 
}


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    if(display_on){
        /* Copy display bitmap to hardware */
        lcd_write_reg(R_RAM_ADDR_SET, 0);
        lcd_begin_write_gram();
        lcd_write_data((unsigned short *)lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT);
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    if(display_on) {    
        int ymax = y + height;

        if(x + width > LCD_WIDTH)
            width = LCD_WIDTH - x;
        if (width <= 0)
            return; /* nothing left to do, 0 is harmful to lcd_write_data() */
        if(ymax >= LCD_HEIGHT)
            ymax = LCD_HEIGHT-1;

        /* set update window */ 

        /* horiz ram addr */ 
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (ymax<<8) | y); 
        
        /* vert ram addr */ 
        lcd_write_reg(R_VERT_RAM_ADDR_POS,((x+width-1)<<8) | x); 
        lcd_write_reg(R_RAM_ADDR_SET, (x<<8) | y); 
        lcd_begin_write_gram(); 

        /* Copy specified rectangle bitmap to hardware */ 
        for (; y <= ymax; y++) 
        { 
            lcd_write_data ((unsigned short *)&lcd_framebuffer[y][x], width); 
        } 

        /* reset update window */ 
        /* horiz ram addr: 0 - 175 */ 
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0xaf00); 

        /* vert ram addr: 0 - 219 */  
        lcd_write_reg(R_VERT_RAM_ADDR_POS, 0xdb00); 
    }
}
