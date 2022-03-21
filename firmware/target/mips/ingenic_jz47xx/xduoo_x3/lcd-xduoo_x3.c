/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
 * Copyright (C) 2020 by William Wilgus
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

#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "string.h"
#include "kernel.h"

/* LCD pins */
#define PIN_BL_EN   (32*4+0)

#define PIN_LCD_D0  (32*2+2)
#define PIN_LCD_D1  (32*2+3)
#define PIN_LCD_D2  (32*2+4)
#define PIN_LCD_D3  (32*2+5)
#define PIN_LCD_D4  (32*2+6)
#define PIN_LCD_D5  (32*2+7)
#define PIN_LCD_D6  (32*2+12)
#define PIN_LCD_D7  (32*2+13)

#define PIN_LCD_RD  (32*2+8)
#define PIN_LCD_DC  (32*2+9)
#define PIN_LCD_CS  (32*2+14)
#define PIN_LCD_RES (32*2+18)
#define PIN_LCD_WR  (32*2+19)

 /* LCD_PINS_MASK D0-D7 RD DC CS RES WR */
#define LCD_PINS_MASK 0x000C73FC
 /* LCD_DATA_MASK D0-D7 */
#define LCD_DATA_MASK 0x000030FC
/* FRAMEBUF_TO_LCD_DATA -- translate data to match LCD data pins */
#define FRAMEBUF_TO_LCD_DATA(b) (((b & 0xC0) << 6) | ((b & 0x3F) << 2))

/* LCD setup codes */
#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_CHARGE_PUMP                       ((char)0x8D)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_SEGMENT_REMAP_INV                 ((char)0xA1)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_MULTIPLEX_RATIO                   ((char)0xA8)
#define LCD_SET_DC_DC                             ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV     ((char)0xC8)
#define LCD_SET_DISPLAY_OFFSET                    ((char)0xD3)
#define LCD_SET_DISPLAY_CLOCK_AND_OSC_FREQ        ((char)0xD5)
#define LCD_SET_VCOM_HW_CONFIGURATION             ((char)0xDA)
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

#define LCD_COL_OFFSET 2 /* column offset */

#define BITDELAY()                    \
    do {                              \
        volatile unsigned int i = 12; \
        __asm__ __volatile__ (        \
          ".set noreorder    \n"      \
          "1:                \n"      \
          "bne  %0, $0, 1b   \n"      \
          "addi %0, %0, -1   \n"      \
          ".set reorder      \n"      \
          : "=r" (i)                  \
          : "0" (i)                   \
        );                            \
    } while (0)

void lcd_hw_init(void)
{
    REG_GPIO_PXFUNC(2) = LCD_PINS_MASK; /* GPIO/INTERRUPT */
    REG_GPIO_PXSELC(2) = LCD_PINS_MASK; /* GPIO */

    REG_GPIO_PXPEC(2) =  LCD_PINS_MASK; /* ENABLE PULLUP*/
    REG_GPIO_PXDIRS(2) = LCD_PINS_MASK; /* OUTPUT */

    REG_GPIO_PXDATS(2) = LCD_PINS_MASK; /* SET BIT */
    REG_GPIO_PXSLS(2)  = LCD_PINS_MASK; /* slew -- fast rate */

    REG_GPIO_PXDS0C(2) = LCD_PINS_MASK; /* Low pin drive strength */
    REG_GPIO_PXDS1C(2) = LCD_PINS_MASK;
    REG_GPIO_PXDS2C(2) = LCD_PINS_MASK;

    __gpio_clear_pin(PIN_LCD_RD);       /* UNUSED */
    __gpio_as_input(PIN_LCD_RD);        /* UNUSED */

    __gpio_clear_pin(PIN_BL_EN);
    __gpio_as_output(PIN_BL_EN);
    __gpio_clear_pin(PIN_LCD_RES);
    udelay(1);
    __gpio_set_pin(PIN_LCD_RES);
    __gpio_clear_pin(PIN_LCD_CS);

    __cpm_stop_lcd(); /* We don't use the LCD controller */
}

void lcd_write_command(int byte)
{
    __gpio_clear_pin(PIN_LCD_DC);
    REG_GPIO_PXDATC(2) = LCD_DATA_MASK;
    REG_GPIO_PXDATS(2) = FRAMEBUF_TO_LCD_DATA(byte);
    __gpio_clear_pin(PIN_LCD_WR);
    BITDELAY();
    __gpio_set_pin(PIN_LCD_WR);
    BITDELAY();
}

static void lcd_write_cmd_triplet(int cmd1, int cmd2, int cmd3)
{
#if 0
    lcd_write_command(cmd1);
    lcd_write_command(cmd2);
    lcd_write_command(cmd3);
#else
    int command = LCD_DATA_MASK;
    __gpio_clear_pin(PIN_LCD_DC);

    REG_GPIO_PXDATC(2) = command;
    command = FRAMEBUF_TO_LCD_DATA(cmd1);
    REG_GPIO_PXDATS(2) = command;
    __gpio_clear_pin(PIN_LCD_WR);
    BITDELAY();
    __gpio_set_pin(PIN_LCD_WR);
    BITDELAY();

    REG_GPIO_PXDATC(2) = command;
    command = FRAMEBUF_TO_LCD_DATA(cmd2);
    REG_GPIO_PXDATS(2) = command;
    __gpio_clear_pin(PIN_LCD_WR);
    BITDELAY();
    __gpio_set_pin(PIN_LCD_WR);
    BITDELAY();

    REG_GPIO_PXDATC(2) = command;
    command = FRAMEBUF_TO_LCD_DATA(cmd3);
    REG_GPIO_PXDATS(2) = command;
    __gpio_clear_pin(PIN_LCD_WR);
    BITDELAY();
    __gpio_set_pin(PIN_LCD_WR);
    BITDELAY();
#endif
}

void lcd_write_data(const fb_data* p_bytes, int count)
{
    int data = LCD_DATA_MASK;
    __gpio_set_pin(PIN_LCD_DC);
    while (count--)
    {
        REG_GPIO_PXDATC(2) = data;
        data = FRAMEBUF_TO_LCD_DATA(*p_bytes);
        p_bytes++;
        REG_GPIO_PXDATS(2) = data;
        __gpio_clear_pin(PIN_LCD_WR);
        BITDELAY();
        __gpio_set_pin(PIN_LCD_WR);
        BITDELAY();
    }
}

void lcd_enable_power(bool onoff)
{
    if (onoff)
        __gpio_set_pin(PIN_BL_EN);
    else
        __gpio_clear_pin(PIN_BL_EN);
}

/** globals **/

static bool display_on = false; /* used by lcd_enable */

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    static int last_val = 0xFFFFFF;

    if (val >= 0) /* brightness menu */
    {
        lcd_write_command(LCD_CNTL_CONTRAST);
        lcd_write_command(val);
    }
    else if (val != last_val)
    {
        /* here we change the voltage level and drive times
         * longer precharge = dimmer display
         * higher voltage = shorter precharge required
         */
        int precharge;
        int vcomdsel;
        switch (val)
        {
            case -9:
                precharge = 0xFF;
                vcomdsel  = 0x10;
                break;
            case -8:
                precharge = 0xF9;
                vcomdsel  = 0x10;
                break;
            case -7:
                precharge = 0xF6;
                vcomdsel  = 0x20;
                break;
            default:
            case -6:
                precharge = 0xF1;
                vcomdsel  = 0x30;
                break;
            case -5:
                precharge = 0xF1;
                vcomdsel  = 0x40;
                break;
            case -4:
                precharge = 0x91;
                vcomdsel  = 0x50;
                break;
            case -3:
                precharge = 0x61;
                vcomdsel  = 0x60;
                break;
            case -2:
                precharge = 0x31;
                vcomdsel  = 0x65;
                break;
            case -1:
                precharge = 0x11;
                vcomdsel  = 0x70;
                break;
        }
        last_val = val;
        lcd_enable(false);
        /* Set pre-charge period */
        lcd_write_command(LCD_SET_PRECHARGE_PERIOD);
        lcd_write_command(precharge); /* VCC Generated by Internal DC/DC Circuit */

        /* Set VCOM deselect level */
        lcd_write_command(LCD_SET_VCOM_DESELECT_LEVEL);
        lcd_write_command(vcomdsel);
        lcd_enable(true);
    }
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
        lcd_enable_power(enable);
        lcd_write_command(LCD_SET_DISPLAY_ON);
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_write_command(LCD_SET_DISPLAY_OFF);
        lcd_enable_power(enable);
        REG_GPIO_PXDATC(2) = LCD_DATA_MASK;
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

    lcd_hw_init();

    /* Set display off */
    lcd_write_command(LCD_SET_DISPLAY_OFF);

    /* Set display clock and oscillator frequency */
    lcd_write_command(LCD_SET_DISPLAY_CLOCK_AND_OSC_FREQ);
    lcd_write_command(0x00); /* External clock Bits [0-3] for divider */

    /* Set multiplex ratio*/
    lcd_write_command(LCD_SET_MULTIPLEX_RATIO);
    lcd_write_command(0x3F);

    /* Set display offset */
    lcd_write_command(LCD_SET_DISPLAY_OFFSET);
    lcd_write_command(0x00);

    /* Set starting line as 0 */
    lcd_write_command(LCD_SET_DISPLAY_START_LINE);

    /* Set charge pump */
    lcd_write_command(LCD_SET_CHARGE_PUMP);
    lcd_write_command(0x14); /* VCC Generated by Internal DC/DC Circuit */

    /* Column 131 is remapped to SEG0 */
    lcd_write_command(LCD_SET_SEGMENT_REMAP_INV);

    /* Invert COM scan direction (N-1 to 0) */
    lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV);

    /* Set COM hardware configuration */
    lcd_write_command(LCD_SET_VCOM_HW_CONFIGURATION);
    lcd_write_command(0x12);

    /* Set contrast control */
    lcd_write_command(LCD_SET_CONTRAST_CONTROL_REGISTER);
    lcd_write_command(0xCF); /* VCC Generated by Internal DC/DC Circuit */

    /* Set pre-charge period */
    lcd_write_command(LCD_SET_PRECHARGE_PERIOD);
    lcd_write_command(0xF1); /* VCC Generated by Internal DC/DC Circuit */

    /* Set VCOM deselect level */
    lcd_write_command(LCD_SET_VCOM_DESELECT_LEVEL);
    lcd_write_command(0x20);

    /* Set normal display mode (not every pixel ON) */
    lcd_write_command(LCD_SET_ENTIRE_DISPLAY_OFF);

    /* Set normal display mode (not inverted) */
    lcd_write_command(LCD_SET_NORMAL_DISPLAY);

    fb_data p_bytes[LCD_WIDTH + 2 * LCD_COL_OFFSET];
    memset(p_bytes, 0, sizeof(p_bytes)); /* fills with 0 : pixel off */
    for(i = 0; i < 8; i++)
    {
        lcd_write_command (LCD_SET_PAGE_ADDRESS | (i /*& 0xf*/));
        lcd_write_data(p_bytes, LCD_WIDTH + 2 * LCD_COL_OFFSET);
    }

    lcd_enable(true);

    lcd_update();
}

/*** Update functions ***/

/* returns LCD_CNTL_HIGHCOL or'd with higher 4 bits of
   the 8-bit column address for the display data RAM.
*/
static inline int get_column_high_byte(const int x)
{
    return (LCD_CNTL_HIGHCOL | (((x+LCD_COL_OFFSET) >> 4) & 0xf));
}

/* returns LCD_CNTL_LOWCOL or'd with lower 4 bits of
   the 8-bit column address for the display data RAM.
*/
static inline int get_column_low_byte(const int x)
{
     return (LCD_CNTL_LOWCOL | ((x+LCD_COL_OFFSET) & 0xf));
}

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    if(!display_on)
        return;

    const int column_high = get_column_high_byte(x);
    const int column_low = get_column_low_byte(x);

    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (by++ & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_write_data(data, width);
        data += stride;
    }
}


#ifndef BOOTLOADER
/* Helper function for lcd_grey_phase_blit(). */
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count) ICODE_ATTR;
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count)
{
    unsigned long ltmp;
    unsigned long *lval = (unsigned long *)values;
    unsigned long *lpha = (unsigned long *)phases;
    const unsigned long mask = 0x80808080;
    int data = LCD_DATA_MASK;

    __gpio_set_pin(PIN_LCD_DC);
    while(count--)
    {
        /* calculate disp data from phase we only use the last byte (8bits) */
        ltmp = mask & lpha[0];         // ltmp= 3.......2.......1.......0.......
        ltmp |= (mask & lpha[1]) >> 4; // ltmp= 7.......6.......5.......4.......
        /* phase0 | phase1 >> 4 */     // ltmp= 3...7...2...6...1...5...0...4...
        ltmp |= ltmp >> 9;             // ltmp= 3...7...23..67..12..56..01..45..
        ltmp |= ltmp >> 9;             // ltmp= 3...7...23..67..123.567.012.456.
        ltmp |= ltmp >> 9;             // ltmp= 3...7...23..67..123.567.01234567

        /* update the phases */
        lpha[0] = lval[0] + (lpha[0] & ~mask);
        lpha[1] = lval[1] + (lpha[1] & ~mask);

        REG_GPIO_PXDATC(2) = data;
        data = FRAMEBUF_TO_LCD_DATA(ltmp);
        REG_GPIO_PXDATS(2) = data;
        __gpio_clear_pin(PIN_LCD_WR);
        BITDELAY();
        __gpio_set_pin(PIN_LCD_WR);
        /*BITDELAY(); //enough instructions above to satisfy data hold time */
        lpha+=2;
        lval+=2;
    }
}

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    static long last_tick = 0;
    if(!display_on || TIME_BEFORE(current_tick, last_tick + 2))
        return;

    last_tick = current_tick;
    const int column_high = get_column_high_byte(x);
    const int column_low = get_column_low_byte(x);

    stride <<= 3; /* 8 pixels per block */
    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (by++ & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_grey_data(values, phases, width);

        values += stride;
        phases += stride;
    }
}
#endif

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int y;

    if(!display_on)
        return;


    const int column_high = get_column_high_byte(0);
    const int column_low = get_column_low_byte(0);

    void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_FBHEIGHT; y++)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (y & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_write_data (fbaddr(0,y), LCD_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax;

    if(!display_on)
        return;

    const int column_high = get_column_high_byte(x);
    const int column_low = get_column_low_byte(x);

    /* The Y coordinates have to work on even 8 pixel rows */
    if (x < 0)
    {
        width += x;
        x = 0;
    }

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;

    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */

    if (y < 0)
    {
        height += y;
        y = 0;
    }

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    if (height <= 0)
        return; /* nothing left to do */

    ymax = (y + height-1) >> 3;
    y >>= 3;

    void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (y & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_write_data (fbaddr(x,y), width);
    }
}
