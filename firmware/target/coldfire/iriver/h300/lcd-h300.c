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

static bool display_on = false; /* is the display turned on? */
static bool display_flipped = false;
static int xoffset = 0; /* needed for flip */

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

#define LCD_CMD  (*(volatile unsigned short *)0xf0000000)
#define LCD_DATA (*(volatile unsigned short *)0xf0000002)

#define R_ENTRY_MODE_HORZ 0x7030
#define R_ENTRY_MODE_VERT 0x7038

/* called very frequently - inline! */
static inline void lcd_write_reg(int reg, int val)
{
    LCD_CMD = reg;
    LCD_DATA = val;
}

/* called very frequently - inline! */
static inline void lcd_begin_write_gram(void)
{
    LCD_CMD = R_WRITE_DATA_2_GRAM;
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

static void flip_lcd(bool yesno)
{
    if (yesno)
    {
        lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x031b); /* 224 lines, GS=SS=1 */
        lcd_write_reg(R_GATE_SCAN_POS, 0x0002);      /* 16 lines offset */
        lcd_write_reg(R_1ST_SCR_DRV_POS, 0xdf04);    /* 4..223 */
    }
    else
    {
        lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x001b); /* 224 lines, GS=SS=0 */
        lcd_write_reg(R_GATE_SCAN_POS, 0x0000);
        lcd_write_reg(R_1ST_SCR_DRV_POS, 0xdb00);    /* 0..219 */
    }
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    display_flipped = yesno;
    xoffset = yesno ? 4 : 0;

    if (display_on)
        flip_lcd(yesno);
}

static void _display_on(void)
{
    /** Sequence according to datasheet, p. 132 **/
    
    lcd_write_reg(R_START_OSC, 0x0001); /* Start Oscilation */
    sleep(1);

    /* zero everything*/
    lcd_write_reg(R_POWER_CONTROL1, 0x0000); /* STB = 0, SLP = 0 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0000);  /* GON = 0, DTE = 0, D1-0 = 00b */
    lcd_write_reg(R_POWER_CONTROL3, 0x0000); /* PON = 0 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0000); /* VCOMG = 0 */
    sleep(1);

    /* initialise power supply */
    
    /* DC12-10 = 000b: Step-up1 = clock/8,
     * DC02-00 = 000b: Step-up2 = clock/16,
     * VC2-0   = 010b: VciOUT = 0.87 * VciLVL */
    lcd_write_reg(R_POWER_CONTROL2, 0x0002); 
    
    /* VRH3-0 = 1000b: Vreg1OUT = REGP * 1.90 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0008);
    
    /* VDV4-0 = 00110b: VcomA = Vreg1OUT * 0.76,
     * VCM4-0 = 10000b: VcomH = Vreg1OUT * 0.70*/
    lcd_write_reg(R_POWER_CONTROL4, 0x0610);     

    lcd_write_reg(R_POWER_CONTROL1, 0x0044); /* AP2-0 = 100b, DK = 1 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0018); /* PON = 1 */

    sleep(4); /* Step-up circuit stabilising time */
    
    /* start power supply */

    lcd_write_reg(R_POWER_CONTROL1, 0x0540); /* BT2-0 = 101b, DK = 0 */
    lcd_write_reg(R_POWER_CONTROL4, 0x2610); /* VCOMG = 1 */

    /* other settings */

    /* B/C = 1: n-line inversion form
     * EOR = 1: polarity inversion occurs by applying an EOR to odd/even
     *          frame select signal and an n-line inversion signal.
     * FLD = 01b: 1 field interlaced scan, external display iface */
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, 0x0700);

    /* Address counter updated in vertical direction; left to right;
     * vertical increment horizontal increment.
     * data format for 8bit transfer or spi = 65k (5,6,5)
     * Reverse order of RGB to BGR for 18bit data written to GRAM
     * Replace data on writing to GRAM */
    lcd_write_reg(R_ENTRY_MODE, 0x7038);

    flip_lcd(display_flipped);

    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x0000);
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);

    /* 19 clocks,no equalization */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0002);

    /* Transfer mode for RGB interface disabled
     * internal clock operation;
     * System interface/VSYNC interface */
    lcd_write_reg(R_EXT_DISP_IF_CONTROL, 0x0003);

    /* Front porch lines: 8; Back porch lines: 8; */
    lcd_write_reg(R_DISP_CONTROL2, 0x0808);

    /* Scan mode by the gate driver in the non-display area: disabled;
     * Cycle of scan by the gate driver - set to 31frames(518ms), 
     * disabled by above setting */
    lcd_write_reg(R_DISP_CONTROL3, 0x003f);

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

    display_on=true;  /* must be done before calling lcd_update() */
    lcd_update();
    
    sleep(4); /* op-amp stabilising time */
 
    /** Sequence according to datasheet, p. 130 **/

    lcd_write_reg(R_POWER_CONTROL1, 0x4540); /* SAP2-0=100, BT2-0=101, AP2-0=100 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0005);  /* GON=0, DTE=0, REV=1, D1-0=01 */
    sleep(2);
    
    lcd_write_reg(R_DISP_CONTROL1, 0x0025);  /* GON=1, DTE=0, REV=1, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0027);  /* GON=1, DTE=0, REV=1, D1-0=11 */
    sleep(2);

    lcd_write_reg(R_DISP_CONTROL1, 0x0037);  /* GON=1, DTE=1, REV=1, D1-0=11 */
}

/* LCD init */
void lcd_init_device(void)
{
    /* GPO46 is LCD RESET */
    or_l(0x00004000, &GPIO1_OUT);
    or_l(0x00004000, &GPIO1_ENABLE);
    or_l(0x00004000, &GPIO1_FUNCTION);

    /* Reset LCD */
    and_l(~0x00004000, &GPIO1_OUT);
    sleep(1);
    or_l(0x00004000, &GPIO1_OUT);
    sleep(1);

    _display_on();
}

void lcd_enable(bool on)
{
    if(display_on!=on)
    {
        if(on)
        {
            _display_on();
        }
        else
        {
            /** Off sequence according to datasheet, p. 130 **/

            lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x0002); /* EQ=0, 18 clks/line */
            lcd_write_reg(R_DISP_CONTROL1, 0x0036);  /* GON=1, DTE=1, REV=1, D1-0=10 */
            sleep(2);

            lcd_write_reg(R_DISP_CONTROL1, 0x0026);  /* GON=1, DTE=0, REV=1, D1-0=10 */
            sleep(2);

            lcd_write_reg(R_DISP_CONTROL1, 0x0000);  /* GON=0, DTE=0, D1-0=00 */

            lcd_write_reg(R_POWER_CONTROL1, 0x0000); /* SAP2-0=000, AP2-0=000 */
            lcd_write_reg(R_POWER_CONTROL3, 0x0000); /* PON=0 */
            lcd_write_reg(R_POWER_CONTROL4, 0x0000); /* VCOMG=0 */
            
            /* datasheet p. 131 */
            lcd_write_reg(R_POWER_CONTROL1, 0x0001); /* STB=1: standby mode */

            display_on=false;
        }
    }
}

bool lcd_enabled(void)
{
    return display_on;
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

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420.
 * y should have two lines of Y back to back, 2nd line first.
 * c should contain the Cb and Cr data for the two lines of Y back to back.
 * Needs EMAC set to saturated, signed integer mode.
 */
extern void lcd_write_yuv420_lines(const unsigned char *y,
                                   const unsigned char *c, int cwidth);

/* Performance function to blit a YUV bitmap directly to the LCD
 * src_x, src_y, width and height should be even
 * x, y, width and height have to be within LCD bounds
 */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* IRAM Y, Cb and Cb buffers. */
    unsigned char y_ibuf[LCD_WIDTH*2];
    unsigned char c_ibuf[LCD_WIDTH];
    const unsigned char *ysrc, *usrc, *vsrc;
    const unsigned char *ysrc_max;

    if (!display_on)
        return;

    width &= ~1;  /* stay on the safe side */
    height &= ~1;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);
    /* Set start position and window */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((xoffset + 219) << 8) | xoffset);

    ysrc = src[0] + src_y * stride + src_x;
    usrc = src[1] + (src_y * stride >> 2) + (src_x >> 1);
    vsrc = src[2] + (src_y * stride >> 2) + (src_x >> 1);
    ysrc_max = ysrc + height * stride;

    coldfire_set_macsr(EMAC_SATURATE);
    do
    {
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, ((y + 1) << 8) | y);
        lcd_write_reg(R_RAM_ADDR_SET, ((x+xoffset) << 8) | y);
        lcd_begin_write_gram();

        memcpy(y_ibuf + width, ysrc, width);
        memcpy(y_ibuf, ysrc + stride, width);
        memcpy(c_ibuf, usrc, width >> 1);
        memcpy(c_ibuf + (width >> 1), vsrc, width >> 1);
        lcd_write_yuv420_lines(y_ibuf, c_ibuf, width >> 1);

        y += 2;
        ysrc += 2 * stride;
        usrc += stride >> 1;
        vsrc += stride >> 1;
    }
    while (ysrc < ysrc_max);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    if(display_on){
        lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_VERT);
        /* set start position window */
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 175 << 8);
        lcd_write_reg(R_VERT_RAM_ADDR_POS,((xoffset+219)<<8) | xoffset);
        lcd_write_reg(R_RAM_ADDR_SET, xoffset << 8);

        lcd_begin_write_gram();

        DAR3 = 0xf0000002;
        SAR3 = (unsigned long)lcd_framebuffer;
        BCR3 = LCD_WIDTH*LCD_HEIGHT*2;
        DCR3 = DMA_AA | DMA_BWC(1)
             | DMA_SINC | DMA_SSIZE(DMA_SIZE_LINE) 
             | DMA_DSIZE(DMA_SIZE_WORD) | DMA_START;

        while (!(DSR3 & 1));
        DSR3 = 1;
    }
}


/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    unsigned long dma_addr;

    if(display_on) {

        if(x + width > LCD_WIDTH)
            width = LCD_WIDTH - x;
        if(width <= 0) /* nothing to do */
            return;
        if(y + height > LCD_HEIGHT)
            height = LCD_HEIGHT - y;

        lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_VERT);
        /* set update window */
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 175 << 8);
        lcd_write_reg(R_VERT_RAM_ADDR_POS,((x+xoffset+width-1) << 8) | (x+xoffset));
        lcd_write_reg(R_RAM_ADDR_SET, ((x+xoffset) << 8) | y);
        lcd_begin_write_gram();
        
        DAR3 = 0xf0000002;
        dma_addr = (unsigned long)&lcd_framebuffer[y][x];
        width *= 2;

        for (; height > 0; height--)
        {
            SAR3 = dma_addr;
            BCR3 = width;
            DCR3 = DMA_AA | DMA_BWC(1)
                 | DMA_SINC | DMA_SSIZE(DMA_SIZE_LINE)
                 | DMA_DSIZE(DMA_SIZE_WORD) | DMA_START;

            dma_addr += LCD_WIDTH*2;

            while (!(DSR3 & 1));
            DSR3 = 1;
        }
    }
}
