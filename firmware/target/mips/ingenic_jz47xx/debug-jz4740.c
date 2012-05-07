/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include "jz4740.h"
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
    unsigned int cppcr = REG_CPM_CPPCR;  /* PLL Control Register */
    unsigned int cpccr = REG_CPM_CPCCR;  /* Clock Control Register */
    unsigned int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
    unsigned int od[4] = {1, 2, 2, 4};

    lcd_putsf(0, line++, "CPPCR   :    0x%08x", cppcr);
    lcd_putsf(0, line++, "CPCCR   :    0x%08x", cpccr);
    lcd_putsf(0, line++, "PLL     :    %s", (cppcr & CPM_CPPCR_PLLEN) ? "ON" : "OFF");
    lcd_putsf(0, line++, "m:n:o   :    %d:%d:%d",
            __cpm_get_pllm() + 2,
            __cpm_get_plln() + 2,
            od[__cpm_get_pllod()]
        );
    lcd_putsf(0, line++, "C:H:M:P :   %d:%d:%d:%d", 
            div[__cpm_get_cdiv()],
            div[__cpm_get_hdiv()],
            div[__cpm_get_mdiv()],
            div[__cpm_get_pdiv()]
        );
    lcd_putsf(0, line++, "U:L:I:P:M : %d:%d:%d:%d:%d",
            __cpm_get_udiv() + 1,
            __cpm_get_ldiv() + 1,
            __cpm_get_i2sdiv()+1,
            __cpm_get_pixdiv()+1,
            __cpm_get_mscdiv()+1
        );
    lcd_putsf(0, line++, "PLL Freq:    %3d.%02d MHz", TO_MHZ(__cpm_get_pllout()));
    lcd_putsf(0, line++, "CCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_cclk()));
    lcd_putsf(0, line++, "HCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_hclk()));
    lcd_putsf(0, line++, "MCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_mclk()));
    lcd_putsf(0, line++, "PCLK    :    %3d.%02d MHz", TO_MHZ(__cpm_get_pclk()));
    lcd_putsf(0, line++, "LCDCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_lcdclk()));
    lcd_putsf(0, line++, "PIXCLK  : %6d.%02d KHz",    TO_KHZ(__cpm_get_pixclk()));
    lcd_putsf(0, line++, "I2SCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_i2sclk()));
    lcd_putsf(0, line++, "USBCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_usbclk()));
    lcd_putsf(0, line++, "MSCCLK  :    %3d.%02d MHz", TO_MHZ(__cpm_get_mscclk()));
    lcd_putsf(0, line++, "EXTALCLK:    %3d.%02d MHz", TO_MHZ(__cpm_get_extalclk()));
    lcd_putsf(0, line++, "RTCCLK  :    %3d.%02d KHz", TO_KHZ(__cpm_get_rtcclk()));
}

static void display_enabled_clocks(void)
{
    unsigned long lcr = REG_CPM_LCR;
    unsigned long clkgr = REG_CPM_CLKGR;

    lcd_putsf(0, line++, "Low Power Mode : %s", 
            ((lcr & CPM_LCR_LPM_MASK) == CPM_LCR_LPM_IDLE) ?
            "IDLE" : (((lcr & CPM_LCR_LPM_MASK) == CPM_LCR_LPM_SLEEP) ? "SLEEP" : "HIBERNATE")
          );

    lcd_putsf(0, line++, "Doze Mode      : %s", 
            (lcr & CPM_LCR_DOZE_ON) ? "ON" : "OFF");
    if (lcr & CPM_LCR_DOZE_ON)
        lcd_putsf(0, line++, "     duty      : %d", (int)((lcr & CPM_LCR_DOZE_DUTY_MASK) >> CPM_LCR_DOZE_DUTY_BIT));

    lcd_putsf(0, line++, "IPU            : %s",
            (clkgr & CPM_CLKGR_IPU) ? "stopped" : "running");
    lcd_putsf(0, line++, "DMAC           : %s",
            (clkgr & CPM_CLKGR_DMAC) ? "stopped" : "running");
    lcd_putsf(0, line++, "UHC            : %s",
            (clkgr & CPM_CLKGR_UHC) ? "stopped" : "running");
    lcd_putsf(0, line++, "UDC            : %s",
            (clkgr & CPM_CLKGR_UDC) ? "stopped" : "running");
    lcd_putsf(0, line++, "LCD            : %s",
            (clkgr & CPM_CLKGR_LCD) ? "stopped" : "running");
    lcd_putsf(0, line++, "CIM            : %s",
            (clkgr & CPM_CLKGR_CIM) ? "stopped" : "running");
    lcd_putsf(0, line++, "SADC           : %s",
            (clkgr & CPM_CLKGR_SADC) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC            : %s",
            (clkgr & CPM_CLKGR_MSC) ? "stopped" : "running");
    lcd_putsf(0, line++, "AIC1           : %s",
            (clkgr & CPM_CLKGR_AIC1) ? "stopped" : "running");
    lcd_putsf(0, line++, "AIC2           : %s",
            (clkgr & CPM_CLKGR_AIC2) ? "stopped" : "running");
    lcd_putsf(0, line++, "SSI            : %s",
            (clkgr & CPM_CLKGR_SSI) ? "stopped" : "running");
    lcd_putsf(0, line++, "I2C            : %s",
            (clkgr & CPM_CLKGR_I2C) ? "stopped" : "running");
    lcd_putsf(0, line++, "RTC            : %s",
            (clkgr & CPM_CLKGR_RTC) ? "stopped" : "running");
    lcd_putsf(0, line++, "TCU            : %s",
            (clkgr & CPM_CLKGR_TCU) ? "stopped" : "running");
    lcd_putsf(0, line++, "UART1          : %s",
            (clkgr & CPM_CLKGR_UART1) ? "stopped" : "running");
    lcd_putsf(0, line++, "UART0          : %s",
            (clkgr & CPM_CLKGR_UART0) ? "stopped" : "running");
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

    lcd_setfont(FONT_SYSFIXED);
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

