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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "jz4740.h"
#include "backlight.h"
#include "font.h"
#include "lcd.h"
#include "ata.h"
#include "usb.h"
#include "storage.h"
#include "system.h"
#include "button.h"
#include "timefuncs.h"
#include "rtc.h"
#include "common.h"
#include "mipsregs.h"

#ifdef ONDA_VX747P
 #define ONDA_VX747
#endif

int main(void)
{
    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    rtc_init();
    usb_init();
    
    backlight_init();
    
#if 0 /* Enable this when multi storage works */
    storage_init();
#else
    ata_init();
#endif

    int touch, btn;
    char datetime[30];
    reset_screen();
    printf("Rockbox bootloader v0.0001");
    printf("REG_EMC_SACR0: 0x%x", REG_EMC_SACR0);
    printf("REG_EMC_SACR1: 0x%x", REG_EMC_SACR1);
    printf("REG_EMC_SACR2: 0x%x", REG_EMC_SACR2);
    printf("REG_EMC_SACR3: 0x%x", REG_EMC_SACR3);
    printf("REG_EMC_SACR4: 0x%x", REG_EMC_SACR4);
    printf("REG_EMC_DMAR0: 0x%x", REG_EMC_DMAR0);
    unsigned int cpu_id = read_c0_prid();
    printf("CPU_ID: 0x%x", cpu_id);
    printf(" * Company ID: 0x%x", (cpu_id >> 16) & 7);
    printf(" * Processor ID: 0x%x", (cpu_id >> 8) & 7);
    printf(" * Revision ID: 0x%x", cpu_id & 7);
    unsigned int config_data = read_c0_config();
    printf("C0_CONFIG: 0x%x", config_data);
    printf(" * Architecture type: 0x%x", (config_data >> 13) & 3);
    printf(" * Architecture revision: 0x%x", (config_data >> 10) & 7);
    printf(" * MMU type: 0x%x", (config_data >> 7) & 7);
    printf("C0_CONFIG1: 0x%x", read_c0_config1());
    if(read_c0_config1() & (1 << 0)) printf(" * FP available");
    if(read_c0_config1() & (1 << 1)) printf(" * EJTAG available");
    if(read_c0_config1() & (1 << 2)) printf(" * MIPS-16 available");
    if(read_c0_config1() & (1 << 4)) printf(" * Performace counters available");
    if(read_c0_config1() & (1 << 5)) printf(" * MDMX available");
    if(read_c0_config1() & (1 << 6)) printf(" * CP2 available");
    printf("C0_STATUS: 0x%x", read_c0_status());
    
    lcd_puts_scroll(0, 25, "This is a very very long scrolling line.... VERY LONG VERY LONG VERY LONG VERY LONG VERY LONG VERY LONG!!!!!");
    
    while(1)
    {
#ifdef ONDA_VX747
#if 1
        btn = button_get(false);
        touch = button_get_data();
#else /* button_get() has performance issues */
        btn = button_read_device(&touch);
#endif
#else
        btn = button_read_device();
#endif /* ONDA_VX747 */
#define KNOP(x,y)     lcd_set_foreground(LCD_BLACK); \
                      if(btn & x) \
                        lcd_set_foreground(LCD_WHITE); \
                      lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(#x), SYSFONT_HEIGHT*y, #x);
        KNOP(BUTTON_VOL_UP, 0);
        KNOP(BUTTON_VOL_DOWN, 1);
        KNOP(BUTTON_MENU, 2);
        KNOP(BUTTON_POWER, 3);
        lcd_set_foreground(LCD_WHITE);
        if(button_hold())
        {
            printf("BUTTON_HOLD");
            asm("break 0x7");
        }
        if(btn & BUTTON_POWER)
        {
            power_off();
        }
#ifdef ONDA_VX747
        if(btn & BUTTON_TOUCH)
        {
            lcd_set_foreground(LCD_RGBPACK(touch & 0xFF, (touch >> 8)&0xFF, (touch >> 16)&0xFF));
            lcd_fillrect((touch>>16)-10, (touch&0xFFFF)-5, 10, 10);
            lcd_update();
            lcd_set_foreground(LCD_WHITE);
        }
#endif
        snprintf(datetime, 30, "%02d/%02d/%04d %02d:%02d:%02d", get_time()->tm_mday, get_time()->tm_mon, get_time()->tm_year,
                                     get_time()->tm_hour, get_time()->tm_min, get_time()->tm_sec);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT, datetime);
        snprintf(datetime, 30, "%d", current_tick);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*2, datetime);
        snprintf(datetime, 30, "X: %03d Y: %03d", touch>>16, touch & 0xFFFF);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*3, datetime);
        snprintf(datetime, 30, "PIN3: 0x%08x", REG_GPIO_PXPIN(3));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*4, datetime);
        snprintf(datetime, 30, "PIN2: 0x%08x", REG_GPIO_PXPIN(2));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*5, datetime);
        snprintf(datetime, 30, "PIN1: 0x%08x", REG_GPIO_PXPIN(1));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*6, datetime);
        snprintf(datetime, 30, "PIN0: 0x%08x", REG_GPIO_PXPIN(0));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*7, datetime);
        snprintf(datetime, 30, "BadVAddr: 0x%08x", read_c0_badvaddr());
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*8, datetime);
        snprintf(datetime, 30, "ICSR: 0x%08x", REG_INTC_ISR);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*9, datetime);
        lcd_update();
        yield();
    }
    
    return 0;
}
