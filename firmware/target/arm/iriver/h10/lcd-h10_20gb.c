/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "system.h"

/** Initialized in lcd_init_device() **/
/* Is the power turned on? */
static bool power_on;
/* Is the display turned on? */
static bool display_on;
/* Amount of vertical offset. Used for flip offset correction/detection. */
static int y_offset;
/* Reverse flag. Must be remembered when display is turned off. */
static unsigned short disp_control_rev;
/* Contrast setting << 8 */
static int lcd_contrast;

/* Forward declarations */
static void lcd_display_off(void);

/* register defines for the Renesas HD66773R */
#define R_START_OSC             0x00
#define R_DEVICE_CODE_READ      0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_AC_CONTROL        0x02
#define R_POWER_CONTROL1        0x03
#define R_POWER_CONTROL2        0x04
#define R_ENTRY_MODE            0x05
#define R_COMPARE_REG           0x06
#define R_DISP_CONTROL          0x07
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_POWER_CONTROL3        0x0c
#define R_POWER_CONTROL4        0x0d
#define R_POWER_CONTROL5        0x0e
#define R_GATE_SCAN_START_POS   0x0f
#define R_VERT_SCROLL_CONTROL   0x11
#define R_1ST_SCR_DRV_POS       0x14
#define R_2ND_SCR_DRV_POS       0x15
#define R_HORIZ_RAM_ADDR_POS    0x16
#define R_VERT_RAM_ADDR_POS     0x17
#define R_RAM_WRITE_DATA_MASK   0x20
#define R_RAM_ADDR_SET          0x21
#define R_WRITE_DATA_2_GRAM     0x22
#define R_RAM_READ_DATA         0x22
#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33
#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37
#define R_GAMMA_AMP_ADJ_POS     0x3a
#define R_GAMMA_AMP_ADJ_NEG     0x3b

static inline void lcd_wait_write(void)
{
    while (LCD2_PORT & LCD2_BUSY_MASK);
}

/* Send command */
static inline void lcd_send_cmd(unsigned v)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_CMD_MASK;
    LCD2_PORT = LCD2_CMD_MASK | v;
}

/* Send 16-bit data */
static inline void lcd_send_data(unsigned v)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_DATA_MASK | (v >> 8);    /* Send MSB first */
    LCD2_PORT = LCD2_DATA_MASK | (v & 0xff);
}

/* Send 16-bit data byte-swapped. Only needed until we can use block transfer. */
static inline void lcd_send_data_swapped(unsigned v)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_DATA_MASK | (v & 0xff);  /* Send LSB first */
    LCD2_PORT = LCD2_DATA_MASK | (v >> 8);    
}


/* Write value to register */
static inline void lcd_write_reg(int reg, int val)
{
    lcd_send_cmd(reg);
    lcd_send_data(val);
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    /* Clamp val in range 0-14, 16-30 */
    if (val < 1)
        val = 0;
    else if (val <= 15)
        --val;
    else if (val > 30)
        val = 30;

    lcd_contrast = val << 8;

    if (!power_on)
  	         return;

    /* VCOMG=1, VDV4-0=xxxxx, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x2018 | lcd_contrast);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno == (disp_control_rev == 0x0000))
        return;

    disp_control_rev = yesno ? 0x0000 : 0x0004;

    if (!display_on)
        return;

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | disp_control_rev);
}


/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno == (y_offset != 0))
        return;

    /* The LCD controller is 132x160 while the LCD itself is 128x160, so we need
     * to shift the origin by 4 when we flip the LCD */
    y_offset = yesno ? 4 : 0;

    if (!power_on)
        return;

    /* SCN4-0=000x0 (G1/G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, yesno ? 0x0002 : 0x0000);
    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160) */
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, yesno ? 0x0213 : 0x0113);
    /* HEA7-0=0xxx, HSA7-0=0xxx */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, y_offset ? 0x8304 : 0x7f00);
}

/* LCD init */
void lcd_init_device(void)
{  
    CLCD_CLOCK_SRC |= 0xc0000000; /* Set LCD interface clock to PLL */
    power_on = true;
    display_on = true;
    y_offset = 0;
    disp_control_rev = 0x0004;
    lcd_contrast     = DEFAULT_CONTRAST_SETTING << 8;
}

static void lcd_power_on(void)
{
    /* Be sure standby bit is clear. */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0000);

    /** Power ON Sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 99 */

    lcd_write_reg(R_START_OSC, 0x0001); /* Start Oscillation */
    /* 10ms or more for oscillation circuit to stabilize */
    sleep(HZ/50);
    /* Instruction (1) for power setting; VC2-0, VRH3-0, CAD,
       VRL3-0, VCM4-0, VDV4-0 */
    /* VC2-0=001 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0001);
    /* VRL3-0=0100, PON=0, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0401);
    /* CAD=1 */
    lcd_write_reg(R_POWER_CONTROL2, 0x8000);
    /* VCOMG=0, VDV4-0=xxxxx (19), VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x0018 | lcd_contrast);
    /* Instruction (2) for power setting; BT2-0, DC2-0, AP2-0 */
    /* BT2-0=000, DC2-0=001, AP2-0=011, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x002c);
    /* Instruction (3) for power setting; VCOMG = "1" */
    /* VCOMG=1, VDV4-0=xxxxx (19), VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x2018 | lcd_contrast);

    /* 40ms or more; time for step-up circuits 1,2 to stabilize */
    sleep(HZ/25);

    /* Instruction (4) for power setting; PON = "1" */
    /* VRL3-0=0100, PON=1, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0411);

    /* 40ms or more; time for step-up circuit 4 to stabilize */
    sleep(HZ/25);

    /* Instructions for other mode settings (in register order). */
    /* SM=0, GS=x, SS=0, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, y_offset ? 0x0013 : 0x0113); /* different to X5 */
    /* FLD1-0=01 (1 field), B/C=1, EOR=1 (C-pat), NW5-0=000000 (1 row) */
    lcd_write_reg(R_DRV_AC_CONTROL, 0x0700);
    /* DIT=0, BGR=1, HWM=0, I/D1-0=10, AM=1, LG2-0=000 */
    lcd_write_reg(R_ENTRY_MODE, 0x1028); /* different to X5 */
    /* CP15-0=0000000000000000 */
    lcd_write_reg(R_COMPARE_REG, 0x0000);
    /* NO1-0=01, SDT1-0=00, EQ1-0=00, DIV1-0=00, RTN3-00000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4000);
    /* SCN4-0=000x0 (G1/G160) */
/*    lcd_write_reg(R_GATE_SCAN_START_POS, y_offset ? 0x0000 : 0x0002); */
    /* VL7-0=0x00 */
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);
    /* SE17-10(End)=0x9f (159), SS17-10(Start)=0x00 */
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);
    /* SE27-20(End)=0x5c (92), SS27-20(Start)=0x00 */
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x5c00);
    /* HEA7-0=0xxx, HSA7-0=0xxx */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, y_offset ? 0x8304 : 0x7f00);
    /* PKP12-10=0x0, PKP02-00=0x0 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0003);
    /* PKP32-30=0x4, PKP22-20=0x0 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0400);
    /* PKP52-50=0x4, PKP42-40=0x7 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0407);
    /* PRP12-10=0x3, PRP02-00=0x5 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0305);
    /* PKN12-10=0x0, PKN02-00=0x3 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0003);
    /* PKN32-30=0x7, PKN22-20=0x4 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0704);
    /* PKN52-50=0x4, PRN42-40=0x7 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0407);
    /* PRN12-10=0x5, PRN02-00=0x3 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0503);
    /* VRP14-10=0x14, VRP03-00=0x09 */
    lcd_write_reg(R_GAMMA_AMP_ADJ_POS, 0x1409);
    /* VRN14-00=0x06, VRN03-00=0x02 */
    lcd_write_reg(R_GAMMA_AMP_ADJ_NEG, 0x0602);

    /* 100ms or more; time for step-up circuits to stabilize */
    sleep(HZ/10);

    power_on = true;
}

static void lcd_power_off(void)
{
    /* Display must be off first */
    if (display_on)
        lcd_display_off();

    power_on = false;

    /** Power OFF sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 99 */

    /* Step-up1 halt setting bit */
    /* BT2-0=110, DC2-0=001, AP2-0=011, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x062c);
    /* Step-up3,4 halt setting bit */
    /* VRL3-0=0100, PON=0, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0401);
    /* VCOMG=0, VDV4-0=10011, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x0018 | lcd_contrast);

    /* Wait 100ms or more */
    sleep(HZ/10);

    /* Step-up2,amp halt setting bit */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0000);
}

static void lcd_display_on(void)
{
    /* Be sure power is on first */
    if (!power_on)
        lcd_power_on();

    /** Display ON Sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 97 */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=0, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0001);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0021 | disp_control_rev);
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0023 | disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | disp_control_rev);

    display_on = true;
}

static void lcd_display_off(void)
{
    display_on = false;

    /** Display OFF sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 97 */

    /* EQ1-0=00 already */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0032 | disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0022 | disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=0, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL, 0x0000);
}

void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
    {
        lcd_display_on();
        /* Probably out of sync and we don't wanna pepper the code with
           lcd_update() calls for this. */
        lcd_update();
    }
    else
    {
        lcd_display_off();
    }
}

bool lcd_enabled(void)
{
    return display_on;
}

void lcd_sleep(void)
{
    if (power_on)
        lcd_power_off();

    /* Set standby mode */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=1 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0001);
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 4-pixel units! */
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
}

#define CSUB_X 2
#define CSUB_Y 2

#define RYFAC (31*257)
#define GYFAC (31*257)
#define BYFAC (31*257)
#define RVFAC 11170     /* 31 * 257 *  1.402    */
#define GVFAC (-5690)  /* 31 * 257 * -0.714136 */
#define GUFAC (-2742)   /* 31 * 257 * -0.344136 */
#define BUFAC 14118     /* 31 * 257 *  1.772    */

#define ROUNDOFFS (127*257)
#define ROUNDOFFSG (63*257)

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    int ymax;

    if (!display_on)
        return;

    width = (width + 1) & ~1;

    /* calculate the drawing region */
    x0 = x;
    x1 = x + width - 1;
    y0 = y;
    y1 = y + height - 1;

    /* The 20GB LCD is actually 128x160 but rotated 90 degrees so the origin
     * is actually the bottom left and horizontal and vertical are swapped. 
     * Rockbox expects the origin to be the top left so we need to use 
     * 127 - y instead of just y */
     
    /* max horiz << 8 | start horiz */
    lcd_send_cmd(R_HORIZ_RAM_ADDR_POS);
    lcd_send_data( (((LCD_HEIGHT-1)-y0+y_offset) << 8) | ((LCD_HEIGHT-1)-y1+y_offset) );
    
    /* max vert << 8 | start vert */
    lcd_send_cmd(R_VERT_RAM_ADDR_POS);
    lcd_send_data((x1 << 8) | x0);

    /* position cursor (set AD0-AD15) */
    /* start vert << 8 | start horiz */
    lcd_send_cmd(R_RAM_ADDR_SET);
    lcd_send_data( (x0 << 8) | ((LCD_HEIGHT-1)-y0+y_offset) );
    
    /* start drawing */
    lcd_send_cmd(R_WRITE_DATA_2_GRAM);

    ymax = y + height - 1 ;

    const int stride_div_csub_x = stride/CSUB_X;

    for (; y <= ymax ; y++)
    {
        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;

        const int uvoffset = stride_div_csub_x * (src_y/CSUB_Y) +
                             (src_x/CSUB_X);

        const unsigned char *usrc = src[1] + uvoffset;
        const unsigned char *vsrc = src[2] + uvoffset;
        const unsigned char *row_end = ysrc + width;

        int y, u, v;
        int red1, green1, blue1;
        int red2, green2, blue2;
        unsigned rbits, gbits, bbits;

        int rc, gc, bc;

        do
        {
            u = *usrc++ - 128;
            v = *vsrc++ - 128;
            rc = RVFAC * v + ROUNDOFFS;
            gc = GVFAC * v + GUFAC * u + ROUNDOFFSG;
            bc = BUFAC * u + ROUNDOFFS;

            /* Pixel 1 */
            y = *ysrc++;

            red1   = RYFAC * y + rc;
            green1 = GYFAC * y + gc;
            blue1  = BYFAC * y + bc;

            /* Pixel 2 */
            y = *ysrc++;
            red2   = RYFAC * y + rc;
            green2 = GYFAC * y + gc;
            blue2  = BYFAC * y + bc;

            /* Since out of bounds errors are relatively rare, we check two
               pixels at once to see if any components are out of bounds, and
               then fix whichever is broken. This works due to high values and
               negative values both becoming larger than the cutoff when
               casted to unsigned.  And ORing them together checks all of them
               simultaneously.  */
            if (((unsigned)(red1 | green1 | blue1 |
                     red2 | green2 | blue2)) > (RYFAC*255+ROUNDOFFS)) {
                if (((unsigned)(red1 | green1 | blue1)) > 
                    (RYFAC*255+ROUNDOFFS)) {
                    if ((unsigned)red1 > (RYFAC*255+ROUNDOFFS))
                    {
                        if (red1 < 0)
                            red1 = 0;
                        else
                            red1 = (RYFAC*255+ROUNDOFFS);
                    }
                    if ((unsigned)green1 > (GYFAC*255+ROUNDOFFSG))
                    {
                        if (green1 < 0)
                            green1 = 0;
                        else
                            green1 = (GYFAC*255+ROUNDOFFSG);
                    }
                    if ((unsigned)blue1 > (BYFAC*255+ROUNDOFFS))
                    {
                        if (blue1 < 0)
                            blue1 = 0;
                        else
                            blue1 = (BYFAC*255+ROUNDOFFS);
                    }
                }

                if (((unsigned)(red2 | green2 | blue2)) > 
                    (RYFAC*255+ROUNDOFFS)) {
                    if ((unsigned)red2 > (RYFAC*255+ROUNDOFFS))
                    {
                        if (red2 < 0)
                            red2 = 0;
                        else
                            red2 = (RYFAC*255+ROUNDOFFS);
                    }
                    if ((unsigned)green2 > (GYFAC*255+ROUNDOFFSG))
                    {
                        if (green2 < 0)
                            green2 = 0;
                        else
                            green2 = (GYFAC*255+ROUNDOFFSG);
                    }
                    if ((unsigned)blue2 > (BYFAC*255+ROUNDOFFS))
                    {
                        if (blue2 < 0)
                            blue2 = 0;
                        else
                            blue2 = (BYFAC*255+ROUNDOFFS);
                    }
                }
            }
                
            rbits = red1 >> 16 ;
            gbits = green1 >> 15 ;
            bbits = blue1 >> 16 ;
            lcd_send_data((rbits << 11) | (gbits << 5) | bbits);

            rbits = red2 >> 16 ;
            gbits = green2 >> 15 ;
            bbits = blue2 >> 16 ;
            lcd_send_data((rbits << 11) | (gbits << 5) | bbits);
        }
        while (ysrc < row_end);

        src_y++;
    }
}


/* Update a fraction of the display. */
void lcd_update_rect(int x0, int y0, int width, int height)
{
    int x1, y1;

    unsigned short *addr = (unsigned short *)lcd_framebuffer;

    if (!display_on)
        return;

    /* calculate the drawing region */
    y1 = (y0 + height) - 1;     /* max vert */
    x1 = (x0 + width) - 1;      /* max horiz */

    if(x1 >= LCD_WIDTH)
        x1 = LCD_WIDTH - 1;
    if (x1 <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(y1 >= LCD_HEIGHT)
        y1 = LCD_HEIGHT-1;
        
    /* The 20GB LCD is actually 128x160 but rotated 90 degrees so the origin
     * is actually the bottom left and horizontal and vertical are swapped. 
     * Rockbox expects the origin to be the top left so we need to use 
     * 127 - y instead of just y */
    
    /* max horiz << 8 | start horiz */
    lcd_send_cmd(R_HORIZ_RAM_ADDR_POS);
    lcd_send_data( (((LCD_HEIGHT-1)-y0+y_offset) << 8) | ((LCD_HEIGHT-1)-y1+y_offset) );
    
    /* max vert << 8 | start vert */
    lcd_send_cmd(R_VERT_RAM_ADDR_POS);
    lcd_send_data((x1 << 8) | x0);

    /* position cursor (set AD0-AD15) */
    /* start vert << 8 | start horiz */
    lcd_send_cmd(R_RAM_ADDR_SET);
    lcd_send_data( (x0 << 8) | ((LCD_HEIGHT-1)-y0+y_offset) );

    /* start drawing */
    lcd_send_cmd(R_WRITE_DATA_2_GRAM);

    addr = (unsigned short*)&lcd_framebuffer[y0][x0];

    int c, r;

    /* for each row */
    for (r = 0; r < height; r++) {
        /* for each column */
        for (c = 0; c < width; c++) {
            /* output 1 pixel */
            lcd_send_data_swapped(*addr++);
        }

        addr += LCD_WIDTH - width;
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
