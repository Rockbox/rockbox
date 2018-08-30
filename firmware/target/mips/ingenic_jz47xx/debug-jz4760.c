/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2018 by Solomon Peachy
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
#include "system.h"
#include "cpu.h"
#include <stdarg.h>
#include <stdio.h>
#include "lcd.h"
#include "kernel.h"
#include "font.h"
#include "button.h"
#include "timefuncs.h"

static int line = 0;

/*
 * Clock Generation Module
 */
#define TO_MHZ(x) ((x)/1000000), ((x)%1000000)/10000
#define TO_KHZ(x) ((x)/1000), ((x)%1000)/10

static void display_clocks(void)
{
    unsigned int cppcr0 = REG_CPM_CPPCR0;  /* PLL Control Register */
    unsigned int cppcr1 = REG_CPM_CPPCR1;  /* PLL Control Register */
    unsigned int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
    unsigned int od[4] = {1, 2, 2, 4};

    lcd_putsf(0, line++, "CPPCR0:0x%08x", cppcr0);
    lcd_putsf(0, line++, "PLL0  :%s", (cppcr0 & CPPCR0_PLLEN) ? "ON" : "OFF");
    lcd_putsf(0, line++, "PLL0  :%3d.%02d MHz", TO_MHZ(__cpm_get_pllout()));
    lcd_putsf(0, line++, "m:n:o :%d:%d:%d",
            __cpm_get_pllm() + 2,
            __cpm_get_plln() + 2,
            od[__cpm_get_pllod()]
        );

    lcd_putsf(0, line++, "CPPCR1:0x%08x", cppcr1);

    lcd_putsf(0, line++, "PLL1  :%s", (cppcr1 & CPPCR1_PLL1EN) ? "ON" : "OFF");
    lcd_putsf(0, line++, "PLL1  :%3d.%02d MHz", TO_MHZ(__cpm_get_pll1out()));
    lcd_putsf(0, line++, "m:n:o :%d:%d:%d",
            __cpm_get_pll1m() + 2,
            __cpm_get_pll1n() + 2,
            od[__cpm_get_pll1od()]
        );

    lcd_putsf(0, line++, "C:H:M:P:%d:%d:%d:%d",
            div[__cpm_get_cdiv()],
            div[__cpm_get_hdiv()],
            div[__cpm_get_mdiv()],
            div[__cpm_get_pdiv()]
        );
    lcd_putsf(0, line++, "I:P:M : %d:%d:%d",
            __cpm_get_i2sdiv()+1,
            __cpm_get_pixdiv()+1,
            __cpm_get_mscdiv()+1
        );
    lcd_putsf(0, line++, "CCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_cclk()));
    lcd_putsf(0, line++, "HCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_hclk()));
    lcd_putsf(0, line++, "MCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_mclk()));
    lcd_putsf(0, line++, "PCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_pclk()));
    lcd_putsf(0, line++, "PIXCLK:%6d.%02d KHz", TO_KHZ(__cpm_get_pixclk()));
    lcd_putsf(0, line++, "I2SCLK:%3d.%02d MHz", TO_MHZ(__cpm_get_i2sclk()));
    lcd_putsf(0, line++, "MSCCLK:%3d.%02d MHz", TO_MHZ(__cpm_get_mscclk()));
    lcd_putsf(0, line++, "EXTALCLK:%3d.%02d MHz", TO_MHZ(__cpm_get_extalclk()));
    lcd_putsf(0, line++, "RTCCLK:%3d.%02d KHz", TO_KHZ(__cpm_get_rtcclk()));
}

static void display_enabled_clocks(void)
{
    unsigned long lcr = REG_CPM_LCR;
    unsigned long clkgr0 = REG_CPM_CLKGR0;

    lcd_putsf(0, line++, "Low Power Mode : %s",
            ((lcr & LCR_LPM_MASK) == LCR_LPM_IDLE) ?
            "IDLE" : (((lcr & LCR_LPM_MASK) == LCR_LPM_SLEEP) ? "SLEEP" : "HIBERNATE")
          );

    lcd_putsf(0, line++, "Doze Mode      : %s",
            (lcr & LCR_DOZE) ? "ON" : "OFF");
    if (lcr & LCR_DOZE)
        lcd_putsf(0, line++, "     duty      : %d", (int)((lcr & LCR_DUTY_MASK) >> LCR_DUTY_LSB));

    lcd_putsf(0, line++, "IPU   : %s",
            (clkgr0 & CLKGR0_IPU) ? "stopped" : "running");
    lcd_putsf(0, line++, "DMAC  : %s",
            (clkgr0 & CLKGR0_DMAC) ? "stopped" : "running");
    lcd_putsf(0, line++, "UHC   : %s",
            (clkgr0 & CLKGR0_UHC) ? "stopped" : "running");
    lcd_putsf(0, line++, "LCD   : %s",
            (clkgr0 & CLKGR0_LCD) ? "stopped" : "running");
    lcd_putsf(0, line++, "CIM   : %s",
            (clkgr0 & CLKGR0_CIM) ? "stopped" : "running");
    lcd_putsf(0, line++, "SADC  : %s",
            (clkgr0 & CLKGR0_SADC) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC0  : %s",
            (clkgr0 & CLKGR0_MSC0) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC1  : %s",
            (clkgr0 & CLKGR0_MSC1) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC2  : %s",
            (clkgr0 & CLKGR0_MSC2) ? "stopped" : "running");
    lcd_putsf(0, line++, "AIC   : %s",
            (clkgr0 & CLKGR0_AIC) ? "stopped" : "running");
    lcd_putsf(0, line++, "SSI1  : %s",
            (clkgr0 & CLKGR0_SSI1) ? "stopped" : "running");
    lcd_putsf(0, line++, "SSI2  : %s",
            (clkgr0 & CLKGR0_SSI2) ? "stopped" : "running");
    lcd_putsf(0, line++, "I2C0  : %s",
            (clkgr0 & CLKGR0_I2C0) ? "stopped" : "running");
    lcd_putsf(0, line++, "I2C1  : %s",
            (clkgr0 & CLKGR0_I2C1) ? "stopped" : "running");
    lcd_putsf(0, line++, "UART1 : %s",
            (clkgr0 & CLKGR0_UART1) ? "stopped" : "running");
    lcd_putsf(0, line++, "UART0 : %s",
            (clkgr0 & CLKGR0_UART0) ? "stopped" : "running");
}

bool dbg_ports(void)
{
    return false;
}

bool dbg_hw_info(void)
{
    int btn = 0;
#ifdef HAVE_TOUCHSCREEN
    int touch;
#endif
    struct tm *cur_time;

    lcd_setfont(FONT_UI);
    while(btn ^ BUTTON_POWER)
    {
        lcd_clear_display();
        line = 0;
        display_clocks();
        display_enabled_clocks();
#ifdef HAVE_TOUCHSCREEN
        btn = button_read_device(&touch);
        lcd_putsf(0, line++, "X: %d Y: %d BTN: 0x%X", touch>>16, touch&0xFFFF, btn);
#else
        btn = button_read_device();
#endif
        cur_time = get_time();
        lcd_putsf(0, line++, "%02d/%02d/%04d %02d:%02d:%02d", cur_time->tm_mday,
               cur_time->tm_mon, cur_time->tm_year, cur_time->tm_hour,
               cur_time->tm_min, cur_time->tm_sec);
        lcd_update();
        sleep(HZ/16);
    }
    return true;
}
