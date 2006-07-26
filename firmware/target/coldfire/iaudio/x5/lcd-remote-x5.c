/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "kernel.h"

/* The LCD in the iAudio M3/M5/X5 remote control is a Tomato LSI 0350 */

#define	LCD_SET_DUTY_RATIO 0x48
#define	LCD_SELECT_ADC     0xa0
#define	LCD_SELECT_SHL     0xc0
#define	LCD_SET_COM0       0x44
#define	LCD_OSC_ON         0xab
#define	LCD_SELECT_DCDC    0x64
#define	LCD_SELECT_RES     0x20
#define	LCD_SET_VOLUME     0x81
#define	LCD_SET_BIAS       0x50
#define	LCD_CONTROL_POWER  0x28
#define	LCD_DISPLAY_ON     0xae
#define	LCD_SET_INITLINE   0x40
#define	LCD_SET_COLUMN     0x10
#define	LCD_SET_PAGE       0xb0
#define	LCD_SET_GRAY       0x88
#define	LCD_SET_PWM_FRC    0x90
#define	LCD_SET_POWER_SAVE 0xa8

#define CS_LO      and_l(~0x00000020, &GPIO1_OUT)
#define CS_HI      or_l(0x00000020, &GPIO1_OUT)
#define CLK_LO     and_l(~0x00004000, &GPIO_OUT)
#define CLK_HI     or_l(0x00004000, &GPIO_OUT)
#define DATA_LO    and_l(~0x00002000, &GPIO_OUT)
#define DATA_HI    or_l(0x00002000, &GPIO_OUT)
#define RS_LO      and_l(~0x00008000, &GPIO_OUT)
#define RS_HI      or_l(0x00008000, &GPIO_OUT)

#define LCD_REMOTE_DEFAULT_CONTRAST 0x18;

static int cached_contrast = LCD_REMOTE_DEFAULT_CONTRAST;
static bool remote_initialized = false;

static void remote_write(unsigned char byte, bool is_command)
{
    int i;
  
    CS_LO;
    if (is_command)
  	RS_LO;
    else
        RS_HI;
  
    for (i = 0x80; i; i >>= 1)
    {
        CLK_LO;
        if (i & byte) 
            DATA_HI;
        else
            DATA_LO;
        CLK_HI;
    }

    CS_HI;
}

void lcd_remote_write_command(int cmd)
{
    remote_write(cmd, true);
}

void lcd_remote_write_command_ex(int cmd, int data)
{
    remote_write(cmd, true);
    remote_write(data, true);
}

void lcd_remote_write_data(const unsigned char* p_bytes, int count)
{
    while(count--)
        remote_write(*p_bytes++, false);
}

void remote_set_row_and_col(int row, int col)
{
    lcd_remote_write_command(LCD_SET_PAGE | (row & 0xf));
    lcd_remote_write_command_ex(LCD_SET_COLUMN | ((col >> 4) & 0xf),
                                col & 0xf);
}

int lcd_remote_default_contrast(void)
{
    return LCD_REMOTE_DEFAULT_CONTRAST;
}

void lcd_remote_powersave(bool on)
{
    if (on)
        lcd_remote_write_command(LCD_SET_POWER_SAVE | 1);
    else
        lcd_remote_write_command(LCD_SET_POWER_SAVE | 1);
}

void lcd_remote_set_contrast(int val)
{
    cached_contrast = val;
    lcd_remote_write_command_ex(LCD_SET_VOLUME, val);
}

bool remote_detect(void)
{
    return (GPIO_READ & 0x01000000);
}

void remote_init(void)
{
    or_l(0x0000e000, &GPIO_OUT);
    or_l(0x0000e000, &GPIO_ENABLE);
    or_l(0x0000e000, &GPIO_FUNCTION);
    
    or_l(0x00000020, &GPIO1_OUT);
    or_l(0x00000020, &GPIO1_ENABLE);
    or_l(0x00000020, &GPIO1_FUNCTION);
	
    sleep(10);
	
    lcd_remote_write_command(LCD_SET_DUTY_RATIO);
    lcd_remote_write_command(0x70);  /* 1/128 */
    
    lcd_remote_write_command(LCD_SELECT_ADC | 1); /* Reverse direction */
    lcd_remote_write_command(LCD_SELECT_SHL | 8); /* Reverse direction */
    
    lcd_remote_write_command(LCD_SET_COM0);
    lcd_remote_write_command(0x00);
    
    lcd_remote_write_command(LCD_OSC_ON);
    
    lcd_remote_write_command(LCD_SELECT_DCDC | 2); /* DC/DC 5xboost */
    
    lcd_remote_write_command(LCD_SELECT_RES | 7); /* Regulator resistor: 7.2 */
    
    lcd_remote_write_command(LCD_SET_BIAS | 6); /* 1/11 */
    
    lcd_remote_write_command(LCD_CONTROL_POWER | 7); /* All circuits ON */
    
    sleep(30);
    
    lcd_remote_write_command_ex(LCD_SET_GRAY | 0, 0x00);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 1, 0x00);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 2, 0x0c);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 3, 0x00);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 4, 0xcc);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 5, 0x00);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 6, 0xcc);
    lcd_remote_write_command_ex(LCD_SET_GRAY | 7, 0x0c);
    
    lcd_remote_write_command(LCD_SET_PWM_FRC | 6); /* 4FRC + 12PWM */
    
    lcd_remote_write_command(LCD_DISPLAY_ON | 1); /* display on */

    remote_initialized = true;

    lcd_remote_set_contrast(cached_contrast);
}
