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

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
                     "nop; nop; nop; nop; nop; nop;\n\t" \
                     ".set reorder\n\t")
static void show_tlb(void)
{
#define ASID_MASK 0xFF

    unsigned int old_ctx;
    unsigned int entry;
    unsigned int entrylo0, entrylo1, entryhi;
    unsigned int pagemask;

    cli();

    /* Save old context */
    old_ctx = (read_c0_entryhi() & 0xff);

    printf("TLB content:");
    for(entry = 0; entry < 32; entry++)
    {
        write_c0_index(entry);
        BARRIER;
        tlb_read();
        BARRIER;
        entryhi = read_c0_entryhi();
        entrylo0 = read_c0_entrylo0();
        entrylo1 = read_c0_entrylo1();
        pagemask = read_c0_pagemask();
        printf("%02d: ASID=%02d%s VA=0x%08x", entry, entryhi & ASID_MASK, (entrylo0 & entrylo1 & 1) ? "(G)" : "   ", entryhi & ~ASID_MASK);
        printf("PA0=0x%08x C0=%x %s%s%s", (entrylo0>>6)<<12, (entrylo0>>3) & 7, (entrylo0 & 4) ? "Dirty " : "", (entrylo0 & 2) ? "Valid " : "Invalid ", (entrylo0 & 1) ? "Global" : "");
        printf("PA1=0x%08x C1=%x %s%s%s", (entrylo1>>6)<<12, (entrylo1>>3) & 7, (entrylo1 & 4) ? "Dirty " : "", (entrylo1 & 2) ? "Valid " : "Invalid ", (entrylo1 & 1) ? "Global" : "");

        printf("pagemask=0x%08x entryhi=0x%08x", pagemask, entryhi);
        printf("entrylo0=0x%08x entrylo1=0x%08x", entrylo0, entrylo1);
    }
    BARRIER;
    write_c0_entryhi(old_ctx);

    sti();
}

int main(void)
{
    cli();
    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    rtc_init();
    usb_init();
    
    backlight_init();
    
    ata_init();
    
    sti();
    
    /* To make Windows say "ding-dong".. */
    //REG8(USB_REG_POWER) &= ~USB_POWER_SOFTCONN;

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
    int j = 0;
    while(1)
    {
        memset(testdata, 0, 4096);
        jz_nand_read_page(j, &testdata);
        reset_screen();
        printf("Page %d", j);
        int i;
        for(i=0; i<16; i+=8)
        {
            snprintf(msg, 30, "%x%x%x%x%x%x%x%x", testdata[i], testdata[i+1], testdata[i+2], testdata[i+3], testdata[i+4], testdata[i+5], testdata[i+6], testdata[i+7]);
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
        btn = button_read_device(&touch);
#define KNOP(x,y)     lcd_set_foreground(LCD_BLACK); \
                      if(btn & x) \
                        lcd_set_foreground(LCD_WHITE); \
                      lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(#x), LCD_HEIGHT-SYSFONT_HEIGHT*y, #x);
        KNOP(BUTTON_VOL_UP, 5);
        KNOP(BUTTON_VOL_DOWN, 6);
        KNOP(BUTTON_MENU, 7);
        KNOP(BUTTON_POWER, 8);
        lcd_set_foreground(LCD_WHITE);
        if(button_hold())
        {
            printf("BUTTON_HOLD");
            asm("break 0x7");
        }
        if(btn & BUTTON_VOL_DOWN)
        {
            reset_screen();
            show_tlb();
        }
        if(btn & BUTTON_POWER)
        {
            power_off();
        }
        if(btn & BUTTON_TOUCH)
        {
            lcd_set_foreground(LCD_RGBPACK(touch & 0xFF, (touch >> 8)&0xFF, (touch >> 16)&0xFF));
            lcd_fillrect((touch>>16)-5, (touch&0xFFFF)-5, 10, 10);
            lcd_update();
            lcd_set_foreground(LCD_WHITE);
        }
        snprintf(datetime, 30, "%02d/%02d/%04d %02d:%02d:%02d", get_time()->tm_mday, get_time()->tm_mon, get_time()->tm_year,
                                     get_time()->tm_hour, get_time()->tm_min, get_time()->tm_sec);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT, datetime);
        snprintf(datetime, 30, "%d", current_tick);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*2, datetime);
        snprintf(datetime, 30, "X: %d Y: %d", touch>>16, touch & 0xFFFF);
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*3, datetime);
        snprintf(datetime, 30, "PIN3: 0x%x", REG_GPIO_PXPIN(3));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*4, datetime);
        snprintf(datetime, 30, "PIN2: 0x%x", REG_GPIO_PXPIN(2));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*5, datetime);
        snprintf(datetime, 30, "PIN1: 0x%x", REG_GPIO_PXPIN(1));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*6, datetime);
        snprintf(datetime, 30, "PIN0: 0x%x", REG_GPIO_PXPIN(0));
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*7, datetime);
        snprintf(datetime, 30, "ICSR: 0x%x", read_c0_badvaddr());
        lcd_putsxy(LCD_WIDTH-SYSFONT_WIDTH*strlen(datetime), LCD_HEIGHT-SYSFONT_HEIGHT*8, datetime);
        lcd_update();
    }
    
    return 0;
}
