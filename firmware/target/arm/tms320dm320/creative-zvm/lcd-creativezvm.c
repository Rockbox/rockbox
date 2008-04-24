/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Maurus Cuelenaere
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
#include "memory.h"
#include "cpu.h"
#include "spi.h"
#include "spi-target.h"
#include "lcd-target.h"

/* Power and display status */
static bool display_on  = true; /* Is the display turned on? */
static bool direct_fb_access = false; /* Does the DM320 has direct access to the FB? */

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

void lcd_set_invert_display(bool yesno) {
  (void) yesno;
  // TODO:
}

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}


/* LTV250QV panel functions */

static void lcd_write_reg(unsigned char reg, unsigned short val)
{
    unsigned char block[3];
    block[0] = 0x74;
    block[1] = 0;
    block[2] = reg | 0xFF;
    spi_block_transfer(SPI_target_LTV250QV, block, sizeof(block), NULL, 0);
    block[0] = 0x76;
    block[1] = (val >> 8) & 0xFF;
    block[2] = val & 0xFF;
    spi_block_transfer(SPI_target_LTV250QV, block, sizeof(block), NULL, 0);
}

static void sleep_ms(unsigned int ms)
{
    sleep(ms*HZ/1000);
}

static void lcd_display_on(void)
{
    /* Enable main power */
    IO_GIO_BITSET2 |= (1 << 3);
    
    /* power on sequence as per the ZVM firmware */
    sleep_ms(250);
    IO_GIO_BITSET1 = (1 << 13);
    sleep_ms(5);
    IO_GIO_BITSET2 = (1 << 5);
    IO_GIO_BITSET2 = (1 << 8);
    sleep_ms(1);
    
    //Init SPI here...
    sleep_ms(32);
    
    IO_GIO_BITSET2 = (1 << 0);
    sleep_ms(5);
    IO_GIO_BITSET2 = (1 << 7);
    sleep_ms(5);
    IO_GIO_BITSET2 = (1 << 4);
    sleep_ms(5);
    IO_GIO_BITCLR2 = (1 << 8);
    //TODO: figure out what OF does after this...
    IO_GIO_BITSET2 = (1 << 8);
    sleep_ms(1);
    
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
    
    lcd_write_reg(1,  0x401D);
    lcd_write_reg(2,  0x204);
    lcd_write_reg(3,  0x100);
    lcd_write_reg(4,  0x1000);
    lcd_write_reg(5,  0x5033);
    lcd_write_reg(6,  0x5);
    lcd_write_reg(7,  0x1B);
    lcd_write_reg(8,  0x800);
    lcd_write_reg(16, 0x203);
    lcd_write_reg(17, 0x302);
    lcd_write_reg(18, 0xC08);
    lcd_write_reg(19, 0xC08);
    lcd_write_reg(20, 0x707);
    lcd_write_reg(21, 0x707);
    lcd_write_reg(22, 0x104);
    lcd_write_reg(23, 0x306);
    lcd_write_reg(24, 0x0);
    lcd_write_reg(25, 0x0);
    sleep_ms(60);
    
    lcd_write_reg(9,  0xA55);
    lcd_write_reg(10, 0x111A);
    sleep_ms(10);
    
    //TODO: other stuff!

    /* tell that we're on now */
    display_on = true;
}

static void lcd_display_off(void)
{
    display_on = false;

    /* LQV shutdown sequence */
    lcd_write_reg(9,  0x855);
    sleep_ms(20);
    
    lcd_write_reg(9,  0x55);
    lcd_write_reg(5,  0x4033);
    lcd_write_reg(10, 0x0);
    sleep_ms(20);
    
    lcd_write_reg(9, 0x0);
    sleep_ms(10);
    unsigned char temp[1];
    temp[0] = 0;
    spi_block_transfer(SPI_target_LTV250QV, temp, sizeof(temp), NULL, 0);
    
    IO_GIO_BITCLR2 = (1 << 4);
    sleep_ms(5);
    IO_GIO_BITCLR2 = (1 << 7);
    sleep_ms(5);
    IO_GIO_BITCLR2 = (1 << 0);
    sleep_ms(2);
    IO_GIO_BITCLR2 = (1 << 8);
    IO_GIO_BITCLR2 = (1 << 5);
    
    /* Disable main power */
    IO_GIO_BITCLR2 |= (1 << 3);
}



void lcd_enable(bool on)
{
    if (on == display_on)
    return;

    if (on)
    {
        display_on = true; //TODO: remove me!
        //lcd_display_on();  /* Turn on display */
        lcd_update();      /* Resync display */
    }
    else
    {
        display_on = false; //TODO: remove me!
        //lcd_display_off();  /* Turn off display */
    }
}

bool lcd_enabled(void)
{
    return display_on;
}

void lcd_set_direct_fb(bool yes)
{
    unsigned int addr;
    direct_fb_access = yes;
    if(yes)
        addr = ((unsigned int)&lcd_framebuffer-CONFIG_SDRAM_START) / 32;
    else
        addr = ((unsigned int)FRAME-CONFIG_SDRAM_START) / 32;
    IO_OSD_OSDWINADH = addr >> 16;
    IO_OSD_OSDWIN0ADL = addr & 0xFFFF;
}

bool lcd_get_direct_fb(void)
{
    return direct_fb_access;
}

void lcd_init_device(void)
{
    /* Based on lcd-mr500.c from Catalin Patulea */
    unsigned int addr;

    /* Clear the Frame */
    memset16(FRAME, 0x0000, LCD_WIDTH*LCD_HEIGHT);

    IO_OSD_MODE = 0x00ff;
    IO_OSD_VIDWINMD = 0x0002;
    IO_OSD_OSDWINMD0 = 0x2001;
    IO_OSD_OSDWINMD1 = 0x0002;
    IO_OSD_ATRMD = 0x0000;
    IO_OSD_RECTCUR = 0x0000;

    IO_OSD_OSDWIN0OFST = (LCD_WIDTH*16) / 256;
    addr = ((unsigned int)FRAME-CONFIG_SDRAM_START) / 32;
    IO_OSD_OSDWINADH = addr >> 16;
    IO_OSD_OSDWIN0ADL = addr & 0xFFFF;

#ifndef ZEN_VISION
    IO_OSD_BASEPX=26;
    IO_OSD_BASEPY=5;
#else
    IO_OSD_BASEPX=80;
    IO_OSD_BASEPY=0;
#endif

    IO_OSD_OSDWIN0XP = 0;
    IO_OSD_OSDWIN0YP = 0;
    IO_OSD_OSDWIN0XL = LCD_WIDTH;
    IO_OSD_OSDWIN0YL = LCD_HEIGHT;
#if 0
    //TODO: set LCD clock!
    IO_CLK_MOD1 &= ~0x18; // disable OSD clock and VENC clock
    IO_CLK_02DIV = 3;
    IO_CLK_OSEL = (IO_CLK_OSEL & ~0xF00) | 0x400; // reset 'General purpose clock output (GIO26, GIO34)' and set to 'PLLIN clock'
    IO_CLK_SEL1 = (IO_CLK_SEL1 | 7) | 0x1000; // set to 'GP clock output 2 (GIO26, GIO34)' and turn on 'VENC clock'
    IO_CLK_MOD1 |= 0x18; // enable OSD clock and VENC clock
    
    /* Set LCD values in OSD */
    IO_VID_ENC_VMOD = ( ( (IO_VID_ENC_VMOD & 0xFFFF8C00) | 0x14) | 0x2400 ); // disable NTSC/PAL encoder & set mode to RGB666 parallel 18 bit
    IO_VID_ENC_VDCTL = ( ( (IO_VID_ENC_VDCTL & 0xFFFFCFE8) | 0x20) | 0x4000 );
    //TODO: finish this...
#endif
}


/*** Update functions ***/



/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    register fb_data *dst, *src;

    if (!display_on || direct_fb_access)
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

#if CONFIG_ORIENTATION == SCREEN_PORTAIT
    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;
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
#else
    src = &lcd_framebuffer[y][x];
    
    register int xc, yc;
    register fb_data *start=FRAME + LCD_HEIGHT*(LCD_WIDTH-x-1) + y + 1;

    for(yc=0;yc<height;yc++)
    {
        dst=start+yc;
        for(xc=0; xc<width; xc++)
        {
            *dst=*src++;
            dst-=LCD_HEIGHT;
        }
        src+=x;
    }
#endif
}

/* Update the display.
This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!display_on || direct_fb_access)
    return;
#if CONFIG_ORIENTATION == SCREEN_PORTAIT
    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
    LCD_WIDTH*LCD_HEIGHT, 1);
#else
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
unsigned char chroma_buf[LCD_HEIGHT/2*3],
unsigned char const * const src[3],
int width,
int stride);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_blit_yuv(unsigned char * const src[3],
int src_x, int src_y, int stride,
int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recalculated every other
    line */
    unsigned char chroma_buf[LCD_HEIGHT/2*3]; /* 480 bytes */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!display_on || direct_fb_access)
    return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + (LCD_WIDTH - y) - 1;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    do
    {
        lcd_write_yuv420_lines(dst, chroma_buf, yuv_src, width,
        stride);

        yuv_src[0] += stride << 1; /* Skip down two luma lines */
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
        dst -= 2;
    }
    while (--height > 0);
}

