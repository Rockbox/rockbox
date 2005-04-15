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
#include "kernel.h"
#include "thread.h"

#if CONFIG_CPU == MCF5249

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
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_NORMAL);
    lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_NORMAL);
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_BIAS | 0x0);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x4);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x6);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x7);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_REGULATOR | 0x4); // Select regulator @ 5.0 (default);
    lcd_remote_set_contrast(32);
    
    sleep(1);
    
    lcd_remote_write_command(0x40); // init line
    lcd_remote_write_command(0xB0); // page address
    lcd_remote_write_command(0x10); // column
    lcd_remote_write_command(0x00); // column
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | 1);
}

#endif
