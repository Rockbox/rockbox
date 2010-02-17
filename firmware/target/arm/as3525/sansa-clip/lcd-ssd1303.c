/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2008 François Dinel
 * Copyright (C) 2008-2009 Rafaël Carré
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

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "string.h"

/*** AS3525 specifics ***/
#ifdef SANSA_CLIPV2
#include "as3525v2.h"
#else
#include "as3525.h"
#endif

#include "ascodec.h"

/*** definitions ***/

#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_SEGMENT_REMAP_INV                 ((char)0xA1)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_MULTIPLEX_RATIO                   ((char)0xA8)
#define LCD_SET_DC_DC                             ((char)0xAD)
#define LCD_SET_DC_DC_PART2                       ((char)0x8A)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV     ((char)0xC8)
#define LCD_SET_DISPLAY_CLOCK_AND_OSC_FREQ        ((char)0xD5)
#define LCD_SET_VCOM_DESELECT_LEVEL               ((char)0xDB)
#define LCD_SET_PRECHARGE_PERIOD                  ((char)0xD9)
#define LCD_NOP                                   ((char)0xE3)

/* LCD command codes */
#define LCD_CNTL_CONTRAST       0x81    /* Contrast */
#define LCD_CNTL_OUTSCAN        0xc8    /* Output scan direction */
#define LCD_CNTL_SEGREMAP       0xa1    /* Segment remap */
#define LCD_CNTL_DISPON         0xaf    /* Display on */

#define LCD_CNTL_PAGE           0xb0    /* Page address */
#define LCD_CNTL_HIGHCOL        0x10    /* Upper column address */
#define LCD_CNTL_LOWCOL         0x00    /* Lower column address */


static void lcd_hw_init(void)
{
#if defined(SANSA_CLIP)
/* DBOP initialisation, do what OF does */
    CGU_DBOP = (1<<3) | AS3525_DBOP_DIV;

    GPIOB_AFSEL = 0x08; /* DBOP on pin 3 */
    GPIOC_AFSEL = 0x0f; /* DBOP on pins 3:0 */

    DBOP_CTRL      = 0x51008;
    DBOP_TIMPOL_01 = 0x6E167;
    DBOP_TIMPOL_23 = 0xA167E06F;
#elif defined(SANSA_CLIPV2)
/* DBOP initialisation, do what OF does */
    CCU_IO |= (1<<12); /* ?? */
    CGU_DBOP |= /*(1<<3)*/ 0x18 | AS3525_DBOP_DIV;

    DBOP_CTRL      = 0x51004;
    DBOP_TIMPOL_01 = 0x36A12F;
    DBOP_TIMPOL_23 = 0xE037E037;
#elif defined(SANSA_CLIPPLUS)
    CGU_PERI |= CGU_SSP_CLOCK_ENABLE;

    SSP_CPSR = AS3525_SSP_PRESCALER;    /* OF = 0x10 */
    SSP_CR0 = (1<<7) | (1<<6) | 7;  /* Motorola SPI frame format, 8 bits */
    SSP_CR1 = (1<<3) | (1<<1);  /* SSP Operation enabled */
    SSP_IMSC = 0;       /* No interrupts */
#endif
}

#ifdef SANSA_CLIP
#define LCD_DELAY 1
#else /* SANSA_CLIPV2 */
#define LCD_DELAY 10
#endif

#if defined(SANSA_CLIP) || defined(SANSA_CLIPV2)
void lcd_write_command(int byte)
{
    volatile int i = 0;
    while(i<LCD_DELAY) i++;

    /* unset D/C# (data or command) */
#ifdef SANSA_CLIP
    GPIOA_PIN(5) = 0;
#else /* SANSA_CLIPV2 */
    GPIOB_PIN(2) = 0;
    DBOP_TIMPOL_23 = 0xE0370036;
#endif

    /* Write command */
    /* Only bits 15:12 and 3:0 of DBOP_DOUT are meaningful */
    DBOP_DOUT = (byte << 8) | byte;

    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0)
        ;

#ifdef SANSA_CLIPV2
    DBOP_TIMPOL_23 = 0xE037E037;
#endif
}
#elif defined(SANSA_CLIPPLUS)
void lcd_write_command(int byte)
{
    while(SSP_SR & (1<<4))  /* BSY flag */
        ;

    GPIOB_PIN(2) = 0;
    SSP_DATA = byte;

    while(SSP_SR & (1<<4))  /* BSY flag */
        ;
}
#endif

#if defined(SANSA_CLIP) || defined(SANSA_CLIPV2)
void lcd_write_data(const fb_data* p_bytes, int count)
{
    volatile int i = 0;
    while(i<LCD_DELAY) i++;
    /* set D/C# (data or command) */
#ifdef SANSA_CLIP
    GPIOA_PIN(5) = (1<<5);
#else /* SANSA_CLIPV2 */
    GPIOB_PIN(2) = (1<<2);
#endif

    while (count--)
    {
        /* Write pixels */
        /* Only bits 15:12 and 3:0 of DBOP_DOUT are meaningful */
        DBOP_DOUT = (*p_bytes << 8) | *p_bytes;

        p_bytes++; /* next packed pixels */

        /* Wait if push fifo is full */
        while ((DBOP_STAT & (1<<6)) != 0);
    }
    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0);
}
#elif defined(SANSA_CLIPPLUS)
void lcd_write_data(const fb_data* p_bytes, int count)
{
    GPIOB_PIN(2) = (1<<2);

    while (count--)
    {
        while(!(SSP_SR & (1<<1)))   /* wait until transmit FIFO is not full */
            ;

        SSP_DATA = *p_bytes++;
    }
}
#endif


/** globals **/

static bool display_on; /* used by lcd_enable */

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_write_command(LCD_CNTL_CONTRAST);
    lcd_write_command(val);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno)
        lcd_write_command(LCD_SET_REVERSE_DISPLAY);
    else
        lcd_write_command(LCD_SET_NORMAL_DISPLAY);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno)
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION);
    }
    else
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP_INV);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV);
    }
}

#ifdef HAVE_LCD_ENABLE
void lcd_enable(bool enable)
{
    if(display_on == enable)
        return;

    if( (display_on = enable) ) /* simple '=' is not a typo ! */
    {
#ifdef SANSA_CLIP
        /* Enable DC-DC AS3525 for some Clip v1 that need it */
        ascodec_write(AS3514_DCDC15, 1);
#endif

        lcd_write_command(LCD_SET_DISPLAY_ON);
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else {
        lcd_write_command(LCD_SET_DISPLAY_OFF);

#ifdef SANSA_CLIP
        /* Disable DC-DC AS3525 */
        ascodec_write(AS3514_DCDC15, 0);
#endif
    }
}

bool lcd_active(void)
{
    return display_on;
}

#endif

/* LCD init, largely based on what OF does */
void lcd_init_device(void)
{
    int i;
#define LCD_FULLSCREEN (128+4)
    fb_data p_bytes[LCD_FULLSCREEN]; /* framebuffer used to clear the screen */

    lcd_hw_init();

#if defined(SANSA_CLIP)
    GPIOA_DIR |= 0x33; /* pins 5:4 and 1:0 out */
    GPIOB_DIR |= 0x40; /* pin 6 out */

    GPIOA_PIN(1) = (1<<1);
    GPIOA_PIN(0) = (1<<0);
    GPIOA_PIN(4) = 0;
    GPIOB_PIN(6) = (1<<6);
#elif defined(SANSA_CLIPV2)
    GPIOB_DIR |= (1<<2)|(1<<5);
    GPIOB_PIN(5) = (1<<5);
#elif defined(SANSA_CLIPPLUS)
    GPIOA_DIR |= (1<<5);
    GPIOB_DIR |= (1<<2) | (1<<7);
    GPIOB_PIN(7) = 0;
    GPIOA_PIN(5) = (1<<5);
#endif

    /* Set display clock (divide ratio = 1) and oscillator frequency (1) */
    lcd_write_command(LCD_SET_DISPLAY_CLOCK_AND_OSC_FREQ);
    lcd_write_command(0x10);

    /* Set VCOM deselect level to 0.76V */
    lcd_write_command(LCD_SET_VCOM_DESELECT_LEVEL);
    lcd_write_command(0x34);

    /* Set pre-charge period (p1period is 2 dclk and p2period is 5 dclk) */
    lcd_write_command(LCD_SET_PRECHARGE_PERIOD);
    lcd_write_command(0x25);

    /* Set contrast register to 12% */
    lcd_set_contrast(lcd_default_contrast());

    /* Disable DC-DC */
    lcd_write_command(LCD_SET_DC_DC);
    lcd_write_command(LCD_SET_DC_DC_PART2/*|0*/);

    /* Set starting line as 0 */
    lcd_write_command(LCD_SET_DISPLAY_START_LINE /*|(0 & 0x3f)*/);

    /* Column 131 is remapped to SEG0 */
    lcd_write_command(LCD_SET_SEGMENT_REMAP_INV);

    /* Invert COM scan direction (N-1 to 0) */
    lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV);

    /* Set normal display mode (not every pixel ON) */
    lcd_write_command(LCD_SET_ENTIRE_DISPLAY_OFF);

    /* Set normal display mode (not inverted) */
    lcd_write_command(LCD_SET_NORMAL_DISPLAY);

    /* Clear whole framebuffer, including "overscan"
     * We don't need to handle that out of screen columns in lcd_clear_display()
     * since we will never write into it anymore
     */
    lcd_write_command (LCD_SET_HIGHER_COLUMN_ADDRESS /*| 0*/);
    lcd_write_command (LCD_SET_LOWER_COLUMN_ADDRESS /*| 0*/);

    memset(p_bytes, 0, sizeof(p_bytes)); /* fills with 0 : pixel off */

    for(i = 0; i < 8; i++)
    {
        lcd_write_command (LCD_SET_PAGE_ADDRESS | (i /*& 0xf*/));
        lcd_write_data(p_bytes, LCD_FULLSCREEN /* overscan */);
    }

    lcd_enable(true);

    lcd_update();
}

/*** Update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    if(!display_on)
        return;

    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_command (LCD_CNTL_PAGE | (by++ & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | (((x+2)>>4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | ((x+2) & 0xf));

        lcd_write_data(data, width);
        data += stride;
    }
}

/* Helper function for lcd_grey_phase_blit(). */
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count);

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    if(!display_on)
        return;

    stride <<= 3; /* 8 pixels per block */
    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_command (LCD_CNTL_PAGE | (by++ & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | (((x+2)>>4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | ((x+2) & 0xf));

        lcd_grey_data(values, phases, width);

        values += stride;
        phases += stride;
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int y;

    if(!display_on)
        return;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_FBHEIGHT; y++)
    {
        lcd_write_command (LCD_CNTL_PAGE | (y & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | ((2 >> 4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | (2 & 0xf));

        lcd_write_data (lcd_framebuffer[y], LCD_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax;

    if(!display_on)
        return;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 3;
    y >>= 3;

    if(x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_FBHEIGHT)
        ymax = LCD_FBHEIGHT-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_command (LCD_CNTL_PAGE | (y & 0xf));
        lcd_write_command (LCD_CNTL_HIGHCOL | (((x+2) >> 4) & 0xf));
        lcd_write_command (LCD_CNTL_LOWCOL | ((x+2) & 0xf));

        lcd_write_data (&lcd_framebuffer[y][x], width);
    }
}
