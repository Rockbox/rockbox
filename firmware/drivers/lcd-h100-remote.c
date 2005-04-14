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

#define CS_LO       GPIO_OUT &= ~0x00000004
#define CS_HI       GPIO_OUT |= 0x00000004
#define CLK_LO      GPIO_OUT &= ~0x10000000
#define CLK_HI      GPIO_OUT |= 0x10000000
#define DATA_LO     GPIO_OUT &= ~0x00040000
#define DATA_HI     GPIO_OUT |= 0x00040000
#define RS_LO       GPIO_OUT &= ~0x00010000
#define RS_HI       GPIO_OUT |= 0x00010000

/* delay loop */
#define DELAY   do { int _x; for(_x=0;_x<3;_x++);} while (0)

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

void lcd_remote_write_command_ex(int cmd, int data1, int data2)
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
    
    RS_HI;
    
    for (i = 0; i < 8; i++)
    {
        if (data1 & 0x80)
            DATA_HI;
        else
            DATA_LO;
        
        CLK_HI;
        data1 <<= 1;
        DELAY;
        
        CLK_LO;
    }
    
    if (data2 != -1)
    {
        for (i = 0; i < 8; i++)
        {
            if (data2 & 0x80)
                DATA_HI;
            else
                DATA_LO;
            
            CLK_HI;
            data2 <<= 1;
            DELAY;
            
            CLK_LO;
        }
    }
    
    CS_HI;
}

#define LCD_REMOTE_ADC_NORMAL       0xa0
#define LCD_REMOTE_ADC_REVERSE      0xa1

#define LCD_REMOTE_SHL_NORMAL       0xc0
#define LCD_REMOTE_SHL_REVERSE      0xc8

#define LCD_REMOTE_DISPLAY_OFF      0xae
#define LCD_REMOTE_DISPLAY_ON       0xaf

#define LCD_REMOTE_REVERSE_OFF      0xa6
#define LCD_REMOTE_REVERSE_ON       0xa7

#define LCD_REMOTE_DISPLAY_NORMAL   0xa4
#define LCD_REMOTE_DISPLAY_ENTIRE   0xa5

#define LCD_REMOTE_NO_OPERATION     0xe3

#define LCD_REMOTE_POWER_CONTROL    0x2b

void lcd_remote_init(void)
{
    GPIO_FUNCTION   |= 0x10010000;  /* GPIO16: RS
                                       GPIO28: CLK */
    
    GPIO1_FUNCTION  |= 0x00040004;  /* GPIO34: CS
                                       GPIO50: Data */
    GPIO_ENABLE     |= 0x10010000;
    GPIO1_ENABLE    |= 0x00040004;

    CLK_LO;
    CS_HI;
}

/*
    lcd_remote_write_command(LCD_REMOTE_ADC_NORMAL);
    lcd_remote_write_command(LCD_REMOTE_SHL_NORMAL);
    lcd_remote_write_command(0xA2); // Bias 0
    lcd_remote_write_command(LCD_REMOTE_POWER_CONTROL | 0x4);	// Convertor
    // 1ms
    lcd_remote_write_command(LCD_REMOTE_POWER_CONTROL | 0x2);	// Regulator
    // 1ms
    lcd_remote_write_command(LCD_REMOTE_POWER_CONTROL | 0x1);	// Follower
    // 1ms
*/

#endif
