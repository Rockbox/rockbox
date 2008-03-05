/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "i2c.h"

/* GPIO A pins for LCD panel SDI interface */

#define LTV250QV_CS  (1<<24)
#define LTV250QV_SCL (1<<25)
#define LTV250QV_SDI (1<<26)

/* LCD Controller registers */

#define LCDC_CTRL    (*(volatile unsigned long *)0xF0000000)
#define LCDC_CLKDIV  (*(volatile unsigned long *)0xF0000008)
#define LCDC_HTIME1  (*(volatile unsigned long *)0xF000000C)
#define LCDC_HTIME2  (*(volatile unsigned long *)0xF0000010)
#define LCDC_VTIME1  (*(volatile unsigned long *)0xF0000014)
#define LCDC_VTIME2  (*(volatile unsigned long *)0xF0000018)
#define LCDC_VTIME3  (*(volatile unsigned long *)0xF000001C)
#define LCDC_VTIME4  (*(volatile unsigned long *)0xF0000020)
#define LCDC_DS      (*(volatile unsigned long *)0xF000005C)
#define LCDC_I1CTRL  (*(volatile unsigned long *)0xF000008C)
#define LCDC_I1POS   (*(volatile unsigned long *)0xF0000090)
#define LCDC_I1SIZE  (*(volatile unsigned long *)0xF0000094)
#define LCDC_I1BASE  (*(volatile unsigned long *)0xF0000098)
#define LCDC_I1OFF   (*(volatile unsigned long *)0xF00000A8)
#define LCDC_I1SCALE (*(volatile unsigned long *)0xF00000AC)

/* Power and display status */
static bool display_on  = false; /* Is the display turned on? */


int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    /* iirc there is an ltv250qv command to do this */
    #warning function not implemented
    (void)val;
}


/* LTV250QV panel functions */

static void ltv250qv_write(unsigned int command)
{
    int i;
    
    GPIOA_CLEAR = LTV250QV_CS;

    for (i = 23; i >= 0; i--)
    {
      GPIOA_CLEAR = LTV250QV_SCL;

      if ((command>>i) & 1)
        GPIOA_SET = LTV250QV_SDI;
      else
        GPIOA_CLEAR = LTV250QV_SDI;
      
      GPIOA_SET = LTV250QV_SCL;
    }

    GPIOA_SET = LTV250QV_CS;
}

static void lcd_write_reg(unsigned char reg, unsigned short val)
{
    ltv250qv_write(0x740000 | reg);
    ltv250qv_write(0x760000 | val);
}


/* TODO: The existing pcf50606 drivers are target-specific, so the following 
   lonely function exists until a D2 driver exists. */

void pcf50606_write_reg(unsigned char reg, unsigned char val)
{
    unsigned char data[] = { reg, val };
    i2c_write(0x10, data, 2);
}


/*
   TEMP: Rough millisecond delay routine used by the LCD panel init sequence.
   PCK_TCT must first have been initialised to 2Mhz by calling clock_init().
*/
static void sleep_ms(unsigned int ms)
{
    /* disable timer */
    TCFG1 = 0;

    /* set Timer1 reference value based on 125kHz tick */
    TREF1 = ms * 125;

    /* single count, zero the counter, divider = 16 [2^(3+1)], enable */
    TCFG1 = (1<<9) | (1<<8) | (3<<4) | 1;

    /* wait until Timer1 ref reached */
    while (!(TIREQ & TF1)) {};
}


static void lcd_display_on(void)
{
    /* power on sequence as per the D2 firmware */
    GPIOA_SET = (1<<16);
    
    sleep_ms(10);
    
    lcd_write_reg(1,  0x1D);
    lcd_write_reg(2,  0x0);
    lcd_write_reg(3,  0x0);
    lcd_write_reg(4,  0x0);
    lcd_write_reg(5,  0x40A3);
    lcd_write_reg(6,  0x0);
    lcd_write_reg(7,  0x0);
    lcd_write_reg(8,  0x0);
    lcd_write_reg(9,  0x0);
    lcd_write_reg(10, 0x0);
    lcd_write_reg(16, 0x0);
    lcd_write_reg(17, 0x0);
    lcd_write_reg(18, 0x0);
    lcd_write_reg(19, 0x0);
    lcd_write_reg(20, 0x0);
    lcd_write_reg(21, 0x0);
    lcd_write_reg(22, 0x0);
    lcd_write_reg(23, 0x0);
    lcd_write_reg(24, 0x0);
    lcd_write_reg(25, 0x0);
    sleep_ms(10);
    
    lcd_write_reg(9,  0x4055);
    lcd_write_reg(10, 0x0);
    sleep_ms(40);
    
    lcd_write_reg(10, 0x2000);
    sleep_ms(40);
    
    lcd_write_reg(1,  0xC01D);
    lcd_write_reg(2,  0x204);
    lcd_write_reg(3,  0xE100);
    lcd_write_reg(4,  0x1000);
    lcd_write_reg(5,  0x5033);
    lcd_write_reg(6,  0x4);
    lcd_write_reg(7,  0x30);
    lcd_write_reg(8,  0x41C);
    lcd_write_reg(16, 0x207);
    lcd_write_reg(17, 0x702);
    lcd_write_reg(18, 0xB05);
    lcd_write_reg(19, 0xB05);
    lcd_write_reg(20, 0x707);
    lcd_write_reg(21, 0x507);
    lcd_write_reg(22, 0x103);
    lcd_write_reg(23, 0x406);
    lcd_write_reg(24, 0x2);
    lcd_write_reg(25, 0x0);
    sleep_ms(60);
    
    lcd_write_reg(9,  0xA55);
    lcd_write_reg(10, 0x111F);
    sleep_ms(10);

    pcf50606_write_reg(0x35, 0xe9); /* PWMC1 - backlight power (intensity) */
    pcf50606_write_reg(0x38, 0x3);  /* GPOC1 - ? */

    /* tell that we're on now */
    display_on = true;
}

static void lcd_display_off(void)
{
    /* block drawing operations and changing of first */
    display_on = false;

    /* LQV shutdown sequence */
    lcd_write_reg(9,  0x55);
    lcd_write_reg(10, 0x1417);
    lcd_write_reg(5,  0x4003);
    sleep_ms(10);
    
    lcd_write_reg(9, 0x0);
    sleep_ms(10);
    
    /* kill power to LCD panel (unconfirmed) */
    GPIOA_CLEAR = (1<<16);
    
    /* also kill the backlight, otherwise LCD fade is visible on screen */
    GPIOA_CLEAR = (1<<6);
}


void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
    {
        LCDC_CTRL |= 1;     /* controller enable */
        GPIOA_SET = (1<<6); /* backlight enable - not visible otherwise */
        lcd_update();       /* Resync display */
    }
    else
    {
        LCDC_CTRL &= ~1;      /* controller disable */
        GPIOA_CLEAR = (1<<6); /* backlight off */
    }
}

bool lcd_enabled(void)
{
    return display_on;
}

/* TODO: implement lcd_sleep() and separate out the power on/off functions */

void lcd_init_device(void)
{
    BCLKCTR |= 4;   /* enable LCD bus clock */
    
    /* set PCK_LCD to 108Mhz */
    PCLK_LCD &= ~PCK_EN;
    PCLK_LCD = PCK_EN | (CKSEL_PLL1<<24) | 1;  /* source = PLL1, divided by 2 */
    
    /* reset the LCD controller */
    SWRESET |= 4;
    SWRESET &= ~4;
    
    /* set port configuration */
    PORTCFG1 &= ~0xC0000000;
    PORTCFG1 &= ~0x3FC0;
    PORTCFG2 &= ~0x100;
    
    /* set physical display size */
    LCDC_DS = (LCD_HEIGHT<<16) | LCD_WIDTH;

    LCDC_HTIME1 = (0x2d<<16) | 0x3bf;
    LCDC_HTIME2 = (1<<16) | 1;
    LCDC_VTIME1 = LCDC_VTIME3 = (0<<16) | 239;
    LCDC_VTIME2 = LCDC_VTIME4 = (1<<16) | 3;

    LCDC_I1BASE  = (unsigned int)lcd_framebuffer; /* dirty, dirty hack */
    LCDC_I1SIZE  = (LCD_HEIGHT<<16) | LCD_WIDTH;  /* image 1 size */
    LCDC_I1POS   = (0<<16) | 0;                   /* position */
    LCDC_I1OFF   = 0;                             /* address offset */
    LCDC_I1SCALE = 0;                             /* scaling */
    LCDC_I1CTRL  = 5;                             /* 565bpp (7 = 888bpp) */
    LCDC_CTRL &= ~(1<<28);

    LCDC_CLKDIV = (LCDC_CLKDIV &~ 0xFF00FF) | (1<<16) | 2; /* and this means? */
    
    /* set and clear various flags - not investigated yet */
    LCDC_CTRL &~ 0x090006AA;  /* clear bits 1,3,5,7,9,10,24,27 */
    LCDC_CTRL |= 0x02800144;  /*   set bits 2,6,8,25,23 */
    LCDC_CTRL = (LCDC_CTRL &~ 0xF0000) | 0x20000;
    LCDC_CTRL = (LCDC_CTRL &~ 0x700000) | 0x700000;

    /* enable LCD controller */
    LCDC_CTRL |= 1;
    
    /* enable LTV250QV panel */
    lcd_display_on();
    
    /* turn on the backlight, without it the LCD is not visible at all */
    GPIOA_SET = (1<<6);
}


/*** Update functions ***/


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    #warning function not implemented
    /* currently lcd_framebuffer is accessed directly by the hardware */
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    #warning function not implemented
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void lcd_set_flip(bool yesno)
{
    #warning function not implemented
    (void)yesno;
}

void lcd_set_invert_display(bool yesno)
{
    #warning function not implemented
    (void)yesno;
}

void lcd_blit(const fb_data* data, int bx, int y, int bwidth,
              int height, int stride)
{
    #warning function not implemented
    (void)data;
    (void)bx;
    (void)y;
    (void)bwidth;
    (void)height;
    (void)stride;
}

void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    #warning function not implemented
    (void)src;
    (void)src_x;
    (void)src_y;
    (void)stride;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
