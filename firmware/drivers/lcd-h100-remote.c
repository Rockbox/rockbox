/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Richard S. La Charité III
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
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"

unsigned char lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH]
#ifndef SIMULATOR
              __attribute__ ((section(".idata")))
#endif	
		;

#define CS_LO       GPIO1_OUT &= ~0x00000004
#define CS_HI       GPIO1_OUT |= 0x00000004
#define CLK_LO      GPIO_OUT &= ~0x10000000
#define CLK_HI      GPIO_OUT |= 0x10000000
#define DATA_LO     GPIO1_OUT &= ~0x00040000
#define DATA_HI     GPIO1_OUT |= 0x00040000
#define RS_LO       GPIO_OUT &= ~0x00010000
#define RS_HI       GPIO_OUT |= 0x00010000

/* delay loop */
#define DELAY   do { int _x; for(_x=0;_x<3;_x++);} while (0)

void lcd_remote_backlight_on(void)
{
    GPIO_OUT &= ~0x00000800;
}

void lcd_remote_backlight_off(void)
{
    GPIO_OUT |= 0x00000800;
}

void lcd_remote_write_command(int cmd)
{
    int i;
    
    CS_LO;
    RS_LO;
    
    for (i = 0; i < 8; i++)
    {
        if (cmd & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        cmd <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    CS_HI;
}

void lcd_remote_write_data(const unsigned char* p_bytes, int count)
{
    int i, j;
    int data;
    
    CS_LO;
    RS_HI;
    
    for (i = 0; i < count; i++)
    {
        data = p_bytes[i];
        
        for (j = 0; j < 8; j++)
        {
            if (data & 0x80)
                DATA_HI;
            else
                DATA_LO;
            
            CLK_HI;
            data <<= 1;
            DELAY;
            
            CLK_LO;
        }
    }
    
    CS_HI;
}

void lcd_remote_write_command_ex(int cmd, int data)
{
    int i;
    
    CS_LO;
    RS_LO;
    
    for (i = 0; i < 8; i++)
    {
        if (cmd & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        cmd <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        data <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    CS_HI;
}

#define LCD_REMOTE_CNTL_ADC_NORMAL          0xa0
#define LCD_REMOTE_CNTL_ADC_REVERSE         0xa1
#define LCD_REMOTE_CNTL_SHL_NORMAL          0xc0
#define LCD_REMOTE_CNTL_SHL_REVERSE         0xc8
#define LCD_REMOTE_CNTL_DISPLAY_ON_OFF      0xae
#define LCD_REMOTE_CNTL_ENTIRE_ON_OFF       0xa4
#define LCD_REMOTE_CNTL_REVERSE_ON_OFF      0xa6
#define LCD_REMOTE_CTNL_NOP                 0xe3
#define LCD_REMOTE_CNTL_POWER_CONTROL       0x2b
#define LCD_REMOTE_CNTL_SELECT_REGULATOR    0x20
#define LCD_REMOTE_CNTL_SELECT_BIAS         0xa2
#define LCD_REMOTE_CNTL_SELECT_VOLTAGE      0x81
#define LCD_REMOTE_CNTL_INIT_LINE           0x40
#define LCD_REMOTE_CNTL_SET_PAGE_ADDRESS    0xB0

void lcd_remote_powersave(bool on)
{
    lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | (on ? 0 : 1));
    lcd_remote_write_command(LCD_REMOTE_CNTL_ENTIRE_ON_OFF | (on ? 1 : 0));
}

void lcd_remote_set_contrast(int val)
{
    lcd_remote_write_command_ex(LCD_REMOTE_CNTL_SELECT_VOLTAGE, val);
}

void lcd_remote_set_invert_display(bool yesno)
{
    lcd_remote_write_command(LCD_REMOTE_CNTL_REVERSE_ON_OFF | yesno);
}

int lcd_remote_default_contrast(void)
{
    return 32;
}

void lcd_remote_bitmap(const unsigned char *src, int x, int y, int nx, int ny, bool clear) __attribute__ ((section (".icode")));
void lcd_remote_bitmap(const unsigned char *src, int x, int y, int nx, int ny, bool clear)
{
    const unsigned char *src_col;
    unsigned char *dst, *dst_col;
    unsigned int data, mask1, mask2, mask3, mask4;
    int stride, shift;
    
    if (((unsigned) x >= LCD_REMOTE_WIDTH) || ((unsigned) y >= LCD_REMOTE_HEIGHT))
    {
        return;
    }
    
    stride = nx;    /* otherwise right-clipping will destroy the image */

    if (((unsigned) (x + nx)) >= LCD_REMOTE_WIDTH)
    {
        nx = LCD_REMOTE_WIDTH - x;
    }
    
    if (((unsigned) (y + ny)) >= LCD_REMOTE_HEIGHT)
    {
        ny = LCD_REMOTE_HEIGHT - y;
    }
        
    dst = &lcd_remote_framebuffer[y >> 3][x];
    shift = y & 7;
    
    if (!shift && clear)  /* shortcut for byte aligned match with clear */
    {
        while (ny >= 8)   /* all full rows */
        {
            memcpy(dst, src, nx);
            src += stride;
            dst += LCD_REMOTE_WIDTH;
            ny -= 8;
        }
        if (ny == 0)     /* nothing left to do? */
        {
            return;
        }
        /* last partial row to do by default routine */
    }

    ny += shift;

    /* Calculate bit masks */
    mask4 = ~(0xfe << ((ny-1) & 7));  /* data mask for last partial row */
    
    if (clear)
    {
        mask1 = ~(0xff << shift); /* clearing of first partial row */
        mask2 = 0;                /* clearing of intermediate (full) rows */
        mask3 = ~mask4;           /* clearing of last partial row */
        if (ny <= 8)
        {
            mask3 |= mask1;
        }
    }
    else
    {
        mask1 = mask2 = mask3 = 0xff;
    }

    /* Loop for each column */
    for (x = 0; x < nx; x++)
    {
        src_col = src++;
        dst_col = dst++;
        data = 0;
        y = 0;

        if (ny > 8)
        {
            /* First partial row */
            data = *src_col << shift;
            *dst_col = (*dst_col & mask1) | data;
            src_col += stride;
            dst_col += LCD_REMOTE_WIDTH;
            data >>= 8;

            /* Intermediate rows */
            for (y = 8; y < ny-8; y += 8)
            {
                data |= *src_col << shift;
                *dst_col = (*dst_col & mask2) | data;
                src_col += stride;
                dst_col += LCD_REMOTE_WIDTH;
                data >>= 8;
            }
        }

        /* Last partial row */
        if (y + shift < ny)
        {
            data |= *src_col << shift;
        }
        
        *dst_col = (*dst_col & mask3) | (data & mask4);
    }
}

void lcd_remote_drawrect(int x, int y, int nx, int ny)
{
    int i;

    if (x > LCD_REMOTE_WIDTH)
    {
        return;
    }
    
    if (y > LCD_REMOTE_HEIGHT)
    {
        return;
    }

    if (x + nx > LCD_REMOTE_WIDTH)
    {
        nx = LCD_REMOTE_WIDTH - x;
    }
    
    if (y + ny > LCD_REMOTE_HEIGHT)
    {
        ny = LCD_REMOTE_HEIGHT - y;
    }

    /* vertical lines */
    for (i = 0; i < ny; i++)
    {
        REMOTE_DRAW_PIXEL(x, (y + i));
        REMOTE_DRAW_PIXEL((x + nx - 1), (y + i));
    }

    /* horizontal lines */
    for (i = 0; i < nx; i++)
    {
        REMOTE_DRAW_PIXEL((x + i),y);
        REMOTE_DRAW_PIXEL((x + i),(y + ny - 1));
    }
}

void lcd_remote_clear_display(void)
{
    memset(lcd_remote_framebuffer, 0, sizeof lcd_remote_framebuffer);
}

void lcd_remote_update(void)
{
    int y;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_REMOTE_HEIGHT / 8; y++)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command_ex(0x10, 0x00);
        lcd_remote_write_data(lcd_remote_framebuffer[y], LCD_REMOTE_WIDTH);
    }
}

void lcd_remote_init(void)
{
    GPIO_FUNCTION   |= 0x10010800;  /* GPIO11: Backlight
                                       GPIO16: RS
                                       GPIO28: CLK */
    
    GPIO1_FUNCTION  |= 0x00040004;  /* GPIO34: CS
                                       GPIO50: Data */
    GPIO_ENABLE     |= 0x10010800;
    GPIO1_ENABLE    |= 0x00040004;
    
    CLK_LO;
    CS_HI;
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_REVERSE);
    lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_REVERSE);
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_BIAS | 0x0);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x5);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x6);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x7);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_REGULATOR | 0x4); // 0x4 Select regulator @ 5.0 (default);
    
    sleep(1);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_INIT_LINE | 0x0); // init line
    lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | 0x0); // page address
    lcd_remote_write_command_ex(0x10, 0x00); // Column MSB + LSB
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | 1);
    
    lcd_remote_clear_display();
    lcd_remote_update();
}
