/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Rockbox driver for Sansa e200 LCDs
 *
 * Based on reverse engineering done my MrH
 *
 * Copyright (c) 2006 Daniel Ankers
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "system.h"
#include <string.h>
#include "backlight-target.h"
#include "cpu.h"

/* Power and display status */
static bool power_on   = false; /* Is the power turned on?   */
static bool display_on SHAREDBSS_ATTR = false; /* Is the display turned on? */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;

/* Reverse Flag */
#define R_DISP_CONTROL_NORMAL 0x0004
#define R_DISP_CONTROL_REV    0x0000
static unsigned short r_disp_control_rev = R_DISP_CONTROL_NORMAL;

/* Flipping */
#define R_DRV_OUTPUT_CONTROL_NORMAL  0x101b
#define R_DRV_OUTPUT_CONTROL_FLIPPED 0x131b
static unsigned short r_drv_output_control  = R_DRV_OUTPUT_CONTROL_NORMAL;

#define LCD_DATA_IN_GPIO GPIOB_INPUT_VAL
#define LCD_DATA_IN_PIN 6

#define LCD_DATA_OUT_GPIO GPIOB_OUTPUT_VAL
#define LCD_DATA_OUT_PIN 7

#define LCD_CLOCK_GPIO GPIOB_OUTPUT_VAL
#define LCD_CLOCK_PIN 5

#define LCD_CS_GPIO GPIOD_OUTPUT_VAL
#define LCD_CS_PIN 6

#define LCD_REG_0 (*(volatile unsigned long *)(0xc2000000))
#define LCD_REG_1 (*(volatile unsigned long *)(0xc2000004))
#define LCD_REG_2 (*(volatile unsigned long *)(0xc2000008))
#define LCD_REG_3 (*(volatile unsigned long *)(0xc200000c))
#define LCD_REG_4 (*(volatile unsigned long *)(0xc2000010))
#define LCD_REG_5 (*(volatile unsigned long *)(0xc2000014))
#define LCD_REG_6 (*(volatile unsigned long *)(0xc2000018))
#define LCD_REG_7 (*(volatile unsigned long *)(0xc200001c))
#define LCD_REG_8 (*(volatile unsigned long *)(0xc2000020))
#define LCD_REG_9 (*(volatile unsigned long *)(0xc2000024))
#define LCD_FB_BASE_REG (*(volatile unsigned long *)(0xc2000028))

/* Taken from HD66789 datasheet and seems similar enough.
   Definitely a Renesas chip though with a perfect register index
   match but at least one bit seems to be set that that datasheet
   doesn't show. It says T.B.D. on the regmap anyway. */
#define R_START_OSC             0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_WAVEFORM_CONTROL  0x02
#define R_ENTRY_MODE            0x03
#define R_COMPARE_REG1          0x04
#define R_COMPARE_REG2          0x05
#define R_DISP_CONTROL1         0x07
#define R_DISP_CONTROL2         0x08
#define R_DISP_CONTROL3         0x09
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_EXT_DISP_INTF_CONTROL 0x0c
#define R_POWER_CONTROL1        0x10
#define R_POWER_CONTROL2        0x11
#define R_POWER_CONTROL3        0x12
#define R_POWER_CONTROL4        0x13
#define R_RAM_ADDR_SET          0x21
#define R_RAM_READ_DATA         0x21
#define R_RAM_WRITE_DATA        0x22
#define R_RAM_WRITE_DATA_MASK1  0x23
#define R_RAM_WRITE_DATA_MASK2  0x24
#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33
#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37
#define R_GAMMA_AMP_ADJ_POS     0x38
#define R_GAMMA_AMP_ADJ_NEG     0x39
#define R_GATE_SCAN_START_POS   0x40
#define R_VERT_SCROLL_CONTROL   0x41
#define R_1ST_SCR_DRIVE_POS     0x42
#define R_2ND_SCR_DRIVE_POS     0x43
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45

/* We don't know how to receive a DMA finished signal from the LCD controller.
 * To avoid problems with flickering, we double-buffer the framebuffer.
 * Align as in lcd-16bit.c and not cached. */
static fb_data lcd_driver_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH]
    __attribute__((aligned(16))) NOCACHEBSS_ATTR;

#ifdef BOOTLOADER
static void lcd_init_gpio(void)
{
    GPIOB_ENABLE |= (1<<7);
    GPIOB_ENABLE |= (1<<5);
    GPIOB_OUTPUT_EN |= (1<<7);
    GPIOB_OUTPUT_EN |= (1<<5);
    GPIOD_ENABLE |= (1<<6);
    GPIOD_OUTPUT_EN |= (1<<6);
}
#endif

static void lcd_bus_idle(void)
{
    LCD_CLOCK_GPIO |= (1 << LCD_CLOCK_PIN);
    LCD_DATA_OUT_GPIO |= (1 << LCD_DATA_OUT_PIN);
}

static void lcd_send_byte(unsigned char byte)
{

    int i;

    for (i = 7; i >=0 ; i--)
    {
        LCD_CLOCK_GPIO &= ~(1 << LCD_CLOCK_PIN);
        if ((byte >> i) & 1)
        {
            LCD_DATA_OUT_GPIO |= (1 << LCD_DATA_OUT_PIN);
        } else {
            LCD_DATA_OUT_GPIO &= ~(1 << LCD_DATA_OUT_PIN);
        }
        udelay(1);
        LCD_CLOCK_GPIO |= (1 << LCD_CLOCK_PIN);
        udelay(1);
        lcd_bus_idle();
        udelay(3);
    }
}

static void lcd_send_msg(unsigned char cmd, unsigned int data)
{
    lcd_bus_idle();
    udelay(1);
    LCD_CS_GPIO &= ~(1 << LCD_CS_PIN);
    udelay(10);
    lcd_send_byte(cmd);
    lcd_send_byte((unsigned char)(data >> 8));
    lcd_send_byte((unsigned char)(data & 0xff));
    LCD_CS_GPIO |= (1 << LCD_CS_PIN);
    udelay(1);
    lcd_bus_idle();
}

static void lcd_write_reg(unsigned int reg, unsigned int data)
{
    lcd_send_msg(0x70, reg);
    lcd_send_msg(0x72, data);
}

/* Run the powerup sequence for the driver IC */
static void lcd_power_on(void)
{
    /* Clear standby bit */
    lcd_write_reg(R_POWER_CONTROL1, 0x0000);

    /** Power ON Sequence **/
    lcd_write_reg(R_START_OSC, 0x0001);
    /* 10ms or more for oscillation circuit to stabilize */
    sleep(HZ/50);

    /* SAP2-0=100, BT2-0=100, AP2-0=100, DK=1, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x4444);
    /* DC12-10=000, DC2-0=000, VC2-0=001 */
    lcd_write_reg(R_POWER_CONTROL2, 0x0001);
    /* PON=0, VRH3-0=0011 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0003);
    /* VCOMG=0, VDV4-0=10001, VCM3-0=11001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x1119);
    /* PON=1, VRH3-0=0011 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0013);
    sleep(HZ/25);

    /* SAP2-0=100, BT2-0=100, AP2-0=100, DK=0, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x4440);
    /* VCOMG=1, VDV4-0=10001, VCM3-0=11001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x3119);
    sleep(HZ/6);

    /* VSPL=0, HSPL=0, DPL=1, EPL=0, SM=0, GS=x, SS=x, NL4-0=11011 */
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, r_drv_output_control);
    /* FLD=0, FLD0=1, B/C=1, EOR=1, NW5-0=000000 */
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, 0x0700);
    /* TRI=0, DFM1-0=11, BGR=0, HWM=1, ID1-0=10, AM=0, LG2-0=000
     * AM: horizontal update direction
     * ID1-0: H decrement, V increment
     */
    lcd_write_reg(R_ENTRY_MODE, 0x6020);
    lcd_write_reg(R_COMPARE_REG1, 0x0000);
    lcd_write_reg(R_COMPARE_REG2, 0x0000);
    /* FP3-0=0010, BP3-0=0010 */
    lcd_write_reg(R_DISP_CONTROL2, 0x0202);
    /* PTG1-0=00 (normal scan), ISC3-0=0000 (ignored) */
    lcd_write_reg(R_DISP_CONTROL3, 0x0000);
    /* NO2-0=01, SDT1-0=00, EQ1-0=01, DIV1-0=00, RTN3-0=0000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4400);
    /* RM=1, DM1-0=01, RIM1-0=00 */
    lcd_write_reg(R_EXT_DISP_INTF_CONTROL, 0x0110);
    /* SCN4-0=00000 - G1 if GS=0, G240 if GS=1 */
    lcd_write_reg(R_GATE_SCAN_START_POS, 0x0000);
    /* VL7-0=00000000 (0 lines) */
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);
    /* SE17-10=219, SS17-10=0 - 220 gates */
    lcd_write_reg(R_1ST_SCR_DRIVE_POS, (219 << 8));
    /* SE27-10=0, SS27-10=0 - no second screen */
    lcd_write_reg(R_2ND_SCR_DRIVE_POS, 0x0000);
    /* HEA=175, HSA=0 = H window from 0-175 */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (175 << 8));
    /* VEA=219, VSA=0 = V window from 0-219 */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (219 << 8));
    /* PKP12-10=000, PKP02-00=000 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0000);
    /* PKP32-30=111, PKP22-20=100 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0704);
    /* PKP52-50=001, PKP42-40=111 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0107);
    /* PRP12-10=111, PRP02-00=100 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0704);
    /* PKN12-10=001, PKN02-00=111 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0107);
    /* PKN32-30=000, PKN22-20=010 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0002);
    /* PKN52-50=111, PKN42-40=111 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0707);
    /* PRN12-10=101, PRN02-00=011 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0503);
    /* VRP14-10=00000, VRP03-00=0000 */
    lcd_write_reg(R_GAMMA_AMP_ADJ_POS, 0x0000);
    /* WRN14-10=00000, VRN03-00=0000 */
    lcd_write_reg(R_GAMMA_AMP_ADJ_NEG, 0x0000);
    /* AD15-0=175 (upper right corner) */
    lcd_write_reg(R_RAM_ADDR_SET, 175);
    /* RM=1, DM1-0=01, RIM1-0=00 */
    lcd_write_reg(R_EXT_DISP_INTF_CONTROL, 0x0110);

    power_on = true;
}

/* Run the display on sequence for the driver IC */
static void lcd_display_on(void)
{
    if (!power_on)
    {
        /* Power has been turned off so full reinit is needed */
        lcd_power_on();
    }
    else
    {
        /* Restore what we fiddled with when turning display off */
        /* PON=1, VRH3-0=0011 */
        lcd_write_reg(R_POWER_CONTROL3, 0x0013);
        /* NO2-0=01, SDT1-0=00, EQ1-0=01, DIV1-0=00, RTN3-0=0000 */
        lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4400);
        /* VCOMG=1, VDV4-0=10001, VCM3-0=11001 */
        lcd_write_reg(R_POWER_CONTROL4, 0x3119);
    }

    /* SAP2-0=100, BT2-0=111, AP2-0=100, DK=1, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x4740);

    sleep(HZ/25);

    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=0, DTE=0, CL=0,
       REV=x, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0041 | r_disp_control_rev);

    sleep(HZ/30);

    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=1, DTE=0, CL=0,
       REV=x, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0061 | r_disp_control_rev);
    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=1, DTE=0, CL=0,
       REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0063 | r_disp_control_rev);

    sleep(HZ/30);

    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=1, DTE=1, CL=0,
       REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0073 | r_disp_control_rev);

    /* Go into write data mode */
    lcd_send_msg(0x70, R_RAM_WRITE_DATA);

    /* tell that we're on now */
    display_on = true;
}

/* Turn off visible display operations */
static void lcd_display_off(void)
{
    /* block drawing operations and changing of first */
    display_on = false;

    /* NO2-0=01, SDT1-0=00, EQ1-0=00, DIV1-0=00, RTN3-0=0000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4000);

    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=1, DTE=1, CL=0,
       REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0072 | r_disp_control_rev);

    sleep(HZ/25);

    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=1, DTE=0, CL=0,
       REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0062 | r_disp_control_rev);

    sleep(HZ/25);

    /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=0, GON=0, DTE=0, CL=0,
       REV=0, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL1, 0x0000);
    /* SAP2-0=000, BT2-0=000, AP2-0=000, DK=0, SLP=0, STBY=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0000);
    /* PON=0, VRH3-0=0011 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0003);
    /* VCOMG=0, VDV4-0=10001, VCM4-0=11001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x1119);
}

void lcd_init_device(void)
{
/* All this is magic worked out by MrH */

/* Stop any DMA which is in progress */
    LCD_REG_6 &= ~1;
    udelay(100000);

#ifdef BOOTLOADER /* Bother at all to do this again? */
/* Init GPIO ports */
    lcd_init_gpio();
/* Controller init */
    GPO32_ENABLE |= (1 << 28);
    GPO32_VAL &= ~(1 << 28);
    DEV_INIT1 = ( (DEV_INIT1 & 0x03ffffff) | (0x15 << 26) );
    outl(((inl(0x70000014) & (0x0fffffff)) | (0x5 << 28)), 0x70000014);
    outl((inl(0x70000020) & ~(0x3 << 10)), 0x70000020);
    DEV_EN |= DEV_LCD; /* Enable controller */
    outl(0x6, 0x600060d0);
    DEV_RS |= DEV_LCD; /* Reset controller */
    outl((inl(0x70000020) & ~(1 << 14)), 0x70000020);
    lcd_bus_idle();
    DEV_RS &=~DEV_LCD; /* Clear reset */
    udelay(1000);

    LCD_REG_0 = (LCD_REG_0 & (0x00ffffff)) | (0x22 << 24);
    LCD_REG_0 = (LCD_REG_0 & (0xff00ffff)) | (0x14 << 16);
    LCD_REG_0 = (LCD_REG_0 & (0xffffc0ff)) | (0x3 << 8);
    LCD_REG_0 = (LCD_REG_0 & (0xffffffc0)) | (0xa);

    LCD_REG_1 &= 0x00ffffff;
    LCD_REG_1 &= 0xff00ffff;
    LCD_REG_1 = (LCD_REG_1 & 0xffff03ff) | (0x2 << 10);
    LCD_REG_1 = (LCD_REG_1 & 0xfffffc00) | (0xdd);

    LCD_REG_2 |= (1 << 5);
    LCD_REG_2 |= (1 << 6);
    LCD_REG_2 = (LCD_REG_2 & 0xfffffcff) | (0x2 << 8);

    LCD_REG_7 &= (0xf800ffff);
    LCD_REG_7 &= (0xfffff800);

    LCD_REG_8 = (LCD_REG_8 & (0xf800ffff)) | (0xb0 << 16);
    LCD_REG_8 = (LCD_REG_8 & (0xfffff800)) | (0xdc); /* X-Y Geometry? */

    LCD_REG_5 |= 0xc;
    LCD_REG_5 = (LCD_REG_5 & ~(0x70)) | (0x3 << 4);
    LCD_REG_5 |= 2;

    LCD_REG_6 &= ~(1 << 15);
    LCD_REG_6 |= (0xe00);
    LCD_REG_6 = (LCD_REG_6 & (0xffffff1f)) | (0x4 << 5);
    LCD_REG_6 |= (1 << 4);

    LCD_REG_5 &= ~(1 << 7);
    /* lcd_driver_framebuffer is uncached therefore at the physical address */
    LCD_FB_BASE_REG = (long)lcd_driver_framebuffer;

    udelay(100000);

/* LCD init */
    /* Pull RESET low, then high to reset driver IC */
    GPO32_VAL &= ~(1 << 28);
    udelay(10000);
    GPO32_VAL |= (1 << 28);
    udelay(10000);

    lcd_display_on();
#else
    /* Power and display already ON - switch framebuffer address and reset
       settings */
    /* lcd_driver_framebuffer is uncached therefore at the physical address */
    LCD_FB_BASE_REG = (long)lcd_driver_framebuffer;

    power_on = true;
    display_on = true;

    lcd_set_invert_display(false);
    lcd_set_flip(false);
#endif

    LCD_REG_6 |= 1; /* Start DMA */
}

void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
    {
        DEV_EN |= DEV_LCD; /* Enable LCD controller */
        lcd_display_on();  /* Turn on display */
        lcd_update();      /* Resync display */
        lcd_call_enable_hook();
        LCD_REG_6 |= 1;    /* Restart DMA */
        sleep(HZ/50);      /* Wait for a frame to be written */
    }
    else
    {
        LCD_REG_6 &= ~1;    /* Disable DMA */
        sleep(HZ/50);       /* Wait for dma end (assuming 50Hz) */
        lcd_display_off();  /* Turn off display */
        DEV_EN &= ~DEV_LCD; /* Disable LCD controller */
    }
}

bool lcd_enabled(void)
{
    return display_on;
}

void lcd_sleep(void)
{
    LCD_REG_6 &= ~1;
    sleep(HZ/50);

    if (power_on)
    {
        /* Turn off display */
        if (display_on)
            lcd_display_off();

        power_on = false;
    }

    /* Set standby mode */
    /* SAP2-0=000, BT2-0=000, AP2-0=000, DK=0, SLP=0, STB=1 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0001);
}

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (!display_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    dst = &lcd_driver_framebuffer[y][x];
    src = &lcd_framebuffer[y][x];

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
}

void lcd_update(void)
{
    if (!display_on)
        return;

    /* Copy the Rockbox framebuffer to the second framebuffer */
    lcd_copy_buffer_rect(&lcd_driver_framebuffer[0][0],
                         &lcd_framebuffer[0][0], LCD_WIDTH*LCD_HEIGHT, 1);
}


/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
    /* TODO: Implement lcd_set_contrast() */
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    bool dma_on = LCD_REG_6 & 1;

    if (dma_on)
    {
        LCD_REG_6 &= ~1;    /* Disable DMA */
        sleep(HZ/50);       /* Wait for dma end (assuming 50Hz) */
        DEV_EN &= ~DEV_LCD; /* Disable LCD controller */
    }

    r_disp_control_rev = yesno ? R_DISP_CONTROL_REV :
                                 R_DISP_CONTROL_NORMAL;

    if (display_on)
    {
        /* PT1-0=00, VLE2-1=00, SPT=0, IB6(??)=1, GON=1, CL=0,
           DTE=1, REV=x, D1-0=11 */
        lcd_write_reg(R_DISP_CONTROL1, 0x0073 | r_disp_control_rev);
    }

    if (dma_on)
    {
        DEV_EN |= DEV_LCD; /* Enable LCD controller */
        lcd_send_msg(0x70, R_RAM_WRITE_DATA); /* Set to RAM write mode */
        LCD_REG_6 |= 1;    /* Restart DMA */
    }
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    bool dma_on = LCD_REG_6 & 1;

    if (dma_on)
    {
        LCD_REG_6 &= ~1;    /* Disable DMA */
        sleep(HZ/50);       /* Wait for dma end (assuming 50Hz) */
        DEV_EN &= ~DEV_LCD; /* Disable LCD controller */
    }

    r_drv_output_control = yesno ? R_DRV_OUTPUT_CONTROL_FLIPPED :
                                   R_DRV_OUTPUT_CONTROL_NORMAL;

    if (power_on)
    {
        /* VSPL=0, HSPL=0, DPL=1, EPL=0, SM=0, GS=x, SS=x,
           NL4-0=11011 (G1-G224) */
        lcd_write_reg(R_DRV_OUTPUT_CONTROL, r_drv_output_control);
    }

    if (dma_on)
    {
        DEV_EN |= DEV_LCD; /* Enable LCD controller */
        lcd_send_msg(0x70, R_RAM_WRITE_DATA); /* Set to RAM write mode */
        LCD_REG_6 |= 1;    /* Restart DMA */
    }
}

/* Blitting functions */

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the e200 - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    if (!display_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    y = LCD_WIDTH - 1 - y;
    fb_data *dst = &lcd_driver_framebuffer[x][y];

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither(dst, yuv_src, width, stride, y, x);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
            y -= 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_write_yuv420_lines(dst, yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
        }
        while (--height > 0);
    }
}
