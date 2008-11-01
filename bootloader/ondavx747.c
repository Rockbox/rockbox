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
#include "config.h"
#include "jz4740.h"
#include "backlight.h"
#include "font.h"
#include "lcd.h"
#include "system.h"
#include "button.h"
#include "timefuncs.h"
#include "rtc.h"
#include "common.h"
#include "mipsregs.h"

static void audiotest(void)
{
    __i2s_internal_codec();
    __aic_enable();
    __aic_reset();
    __aic_select_i2s();
    __aic_enable_loopback();
}

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
    
    storage_init();

    int touch, btn;
    char datetime[30];
    reset_screen();
    printf("Rockbox bootloader v0.000001");
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
    
#if 0
    unsigned char testdata[4096];
    char msg[30];
    int j = 1;
    while(1)
    {
        memset(testdata, 0, 4096);
        reset_screen();
        jz_nand_read(0, j, &testdata);
        printf("Page %d", j);
        int i;
        for(i=0; i<768; i+=16)
        {
            snprintf(msg, 30, "%02x%02x%02x%02x%02x%02x%02x%02x %02x%02x%02x%02x%02x%02x%02x%02x",
            testdata[i], testdata[i+1], testdata[i+2], testdata[i+3], testdata[i+4], testdata[i+5], testdata[i+6], testdata[i+7],
            testdata[i+8], testdata[i+9], testdata[i+10], testdata[i+11], testdata[i+12], testdata[i+13], testdata[i+14], testdata[i+15]
            );
            printf(msg);
        }
        while(!((btn = button_read_device(&touch)) & (BUTTON_VOL_UP|BUTTON_VOL_DOWN)));
        if(btn & BUTTON_VOL_UP)
            j++;
        if(btn & BUTTON_VOL_DOWN)
            j--;
        if(j<0)
            j = 0;
    }
#endif
    while(1)
    {
#ifdef ONDA_VX747
#if 1
        btn = button_get(false);
        touch = button_get_data();
#else
        btn = button_read_device(&touch);
#endif
#else
        btn = button_read_device();
#endif
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
