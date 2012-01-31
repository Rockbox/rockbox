/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

#include "system.h"
#include "dma-imx233.h"
#include "action.h"
#include "lcd.h"
#include "font.h"
#include "adc.h"
#include "adc-imx233.h"
#include "power-imx233.h"
#include "clkctrl-imx233.h"
#include "powermgmt-imx233.h"
#include "rtc-imx233.h"
#include "dcp-imx233.h"
#include "pinctrl-imx233.h"
#include "string.h"

#define DEBUG_CANCEL  BUTTON_BACK

static struct
{
    const char *name;
    unsigned chan;
} dbg_channels[] =
{
    { "i2c", APB_I2C },
    { "dac", APB_AUDIO_DAC },
    { "ssp1", APB_SSP(1) },
    { "ssp2", APB_SSP(2) },
};

bool dbg_hw_info_dma(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 25);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();

        lcd_putsf(0, 0, "I G F C E name bar apb ahb");
        for(unsigned i = 0; i < ARRAYLEN(dbg_channels); i++)
        {
            struct imx233_dma_info_t info = imx233_dma_get_info(dbg_channels[i].chan, DMA_INFO_ALL);
            lcd_putsf(0, i + 1, "%c %c %c %c %c %4s %x %x %x",
                info.int_enabled ? 'i' : ' ',
                info.gated ? 'g' : ' ',
                info.freezed ? 'f' : ' ',
                info.int_cmdcomplt ? 'c' : ' ',
                info.int_error ? 'e' : ' ',
                dbg_channels[i].name, info.bar, info.apb_bytes, info.ahb_bytes);
        }
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_power(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();

        struct imx233_power_info_t info = imx233_power_get_info(POWER_INFO_ALL);
        lcd_putsf(0, 0, "VDDD: %d mV  linreg: %d  offset: %d", info.vddd, info.vddd_linreg,
            info.vddd_linreg_offset);
        lcd_putsf(0, 1, "VDDA: %d mV  linreg: %d  offset: %d", info.vdda, info.vdda_linreg,
            info.vdda_linreg_offset);
        lcd_putsf(0, 2, "VDDIO: %d mV offset: %d", info.vddio, info.vddio_linreg_offset);
        lcd_putsf(0, 3, "VDDMEM: %d mV  linreg: %d", info.vddmem, info.vddmem_linreg);
        lcd_putsf(0, 4, "DC-DC: pll: %d   freq: %d", info.dcdc_sel_pllclk, info.dcdc_freqsel);
        lcd_putsf(0, 5, "charge: %d mA  stop: %d mA", info.charge_current, info.stop_current);
        lcd_putsf(0, 6, "charging: %d  bat_adj: %d", info.charging, info.batt_adj);
        lcd_putsf(0, 7, "4.2: en: %d  dcdc: %d", info._4p2_enable, info._4p2_dcdc);
        lcd_putsf(0, 8, "4.2: cmptrip: %d dropout: %d", info._4p2_cmptrip, info._4p2_dropout);
        lcd_putsf(0, 9, "5V: pwd_4.2_charge: %d", info._5v_pwd_charge_4p2);
        lcd_putsf(0, 10, "5V: chargelim: %d mA", info._5v_charge_4p2_limit);
        lcd_putsf(0, 11, "5V: dcdc: %d  xfer: %d", info._5v_enable_dcdc, info._5v_dcdc_xfer);
        lcd_putsf(0, 12, "5V: thr: %d mV use: %d cmps: %d", info._5v_vbusvalid_thr,
            info._5v_vbusvalid_detect, info._5v_vbus_cmps);
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_adc(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 25);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();

        /* add battery readout in mV, this it is not the direct output of a channel */
        lcd_putsf(0, 0, "Battery(mV) %d", _battery_voltage());
        for(unsigned i = 0; i < NUM_ADC_CHANNELS; i++)
        {
            lcd_putsf(0, i + 1, "%s %d", imx233_adc_channel_name[i],
                adc_read(i));
        }
        
        lcd_update();
        yield();
    }
}

static struct
{
    enum imx233_clock_t clk;
    const char *name;
    bool has_enable;
    bool has_bypass;
    bool has_idiv;
    bool has_fdiv;
    bool has_freq;
} dbg_clk[] =
{
    { CLK_PLL, "pll", true, false, false, false, true},
    { CLK_XTAL, "xtal", false, false, false, false, true},
    { CLK_PIX, "pix", true, true, true, true, true },
    { CLK_SSP, "ssp", true, true, true, false, true },
    { CLK_IO,  "io",  false, false, false, true, true },
    { CLK_CPU, "cpu", false, true, true, true, true },
    { CLK_HBUS, "hbus", false, false, true, true, true },
    { CLK_EMI, "emi", false, true, true, true, true },
    { CLK_XBUS, "xbus", false, false, true, false, true }
};

static struct
{
    enum imx233_as_monitor_t monitor;
    const char *name;
} dbg_as_monitor[] =
{
    { AS_CPU_INSTR, "cpu inst" },
    { AS_CPU_DATA, "cpu data" },
    { AS_TRAFFIC, "traffic" },
    { AS_TRAFFIC_JAM, "traffic jam" },
    { AS_APBXDMA, "apbx" },
    { AS_APBHDMA, "apbh" },
    { AS_PXP, "pxp" },
    { AS_DCP, "dcp" }
};

bool dbg_hw_info_clkctrl(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();

        /*               012345678901234567890123456789 */
        lcd_putsf(0, 0, "name en by idiv fdiv frequency");
        for(unsigned i = 0; i < ARRAYLEN(dbg_clk); i++)
        {
            #define c dbg_clk[i]
            lcd_putsf(0, i + 1, "%4s", c.name);
            if(c.has_enable)
                lcd_putsf(5, i + 1, "%2d", imx233_is_clock_enable(c.clk));
            if(c.has_bypass)
                lcd_putsf(8, i + 1, "%2d", imx233_get_bypass_pll(c.clk));
            if(c.has_idiv && imx233_get_clock_divisor(c.clk) != 0)
                lcd_putsf(10, i + 1, "%4d", imx233_get_clock_divisor(c.clk));
            if(c.has_fdiv && imx233_get_fractional_divisor(c.clk) != 0)
                lcd_putsf(16, i + 1, "%4d", imx233_get_fractional_divisor(c.clk));
            if(c.has_freq)
                lcd_putsf(21, i + 1, "%9d", imx233_get_clock_freq(c.clk));
            #undef c
        }
        int line = ARRAYLEN(dbg_clk) + 1;
        lcd_putsf(0, line, "auto slow: %d", imx233_is_auto_slow_enable());
        line++;
        lcd_putsf(0, line, "as monitor: ");
        int x_off = 12;
        bool first = true;
        unsigned line_w = lcd_getwidth() / font_get_width(font_get(lcd_getfont()), ' ');
        for(unsigned i = 0; i < ARRAYLEN(dbg_as_monitor); i++)
        {
            if(!imx233_is_auto_slow_monitor_enable(dbg_as_monitor[i].monitor))
                continue;
            if(!first)
            {
                lcd_putsf(x_off, line, ", ");
                x_off += 2;
            }
            first = false;
            if((x_off + strlen(dbg_as_monitor[i].name)) > line_w)
            {
                x_off = 1;
                line++;
            }
            lcd_putsf(x_off, line, "%s", dbg_as_monitor[i].name);
            x_off += strlen(dbg_as_monitor[i].name);
        }
        line++;
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_powermgmt(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();
        struct imx233_powermgmt_info_t info = imx233_powermgmt_get_info();
        
        lcd_putsf(0, 0, "state: %s",
            info.state == CHARGE_STATE_DISABLED ? "disabled" :
            info.state == CHARGE_STATE_ERROR ? "error" :
            info.state == DISCHARGING ? "discharging" :
            info.state == TRICKLE ? "trickle" :
            info.state == TOPOFF ? "topoff" :
            info.state == CHARGING ? "charging" : "<unknown>");
        lcd_putsf(0, 1, "charging tmo: %d", info.charging_timeout);
        lcd_putsf(0, 2, "topoff tmo: %d", info.topoff_timeout);
        lcd_putsf(0, 3, "4p2ilimit tmo: %d", info.incr_4p2_ilimit_timeout);
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_rtc(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();
        struct imx233_rtc_info_t info = imx233_rtc_get_info();
        
        lcd_putsf(0, 0, "seconds: %lu", info.seconds);
        for(int i = 0; i < 6; i++)
            lcd_putsf(0, i + 1, "persistent%d: 0x%lx", i, info.persistent[i]);
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_dcp(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();
        struct imx233_dcp_info_t info = imx233_dcp_get_info(DCP_INFO_ALL);
        
        lcd_putsf(0, 0, "crypto: %d  csc: %d", info.has_crypto, info.has_csc);
        lcd_putsf(0, 1, "keys: %d  channels: %d", info.num_keys, info.num_channels);
        lcd_putsf(0, 2, "ciphers: 0x%lx  hash: 0x%lx", info.ciphers, info.hashs);
        lcd_putsf(0, 3, "gather wr: %d  otp rdy: %d ch0merged: %d",
            info.gather_writes, info.otp_key_ready, info.ch0_merged);
        lcd_putsf(0, 4, "ctx switching: %d caching: %d", info.context_switching,
            info.context_caching);
        lcd_putsf(0, 5, "ch  irq ien en rdy pri sem cmdptr     a");
        int nr = HW_DCP_NUM_CHANNELS;
        for(int i = 0; i < nr; i++)
        {
            lcd_putsf(0, 6 + i, "%d    %d   %d   %d  %d   %d   %d  0x%08lx %d",
                i, info.channel[i].irq, info.channel[i].irq_en, info.channel[i].enable,
                info.channel[i].ready, info.channel[i].high_priority,
                info.channel[i].sema, info.channel[i].cmdptr, info.channel[i].acquired);
        }
        lcd_putsf(0, 6 + nr, "csc  %d   %d   %d      %d",
                  info.csc.irq, info.csc.irq_en, info.csc.enable, info.csc.priority);
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_pinctrl(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        for(int i = 0; i < 4; i++)
            lcd_putsf(0, i, "DIN%d = 0x%08x", i, imx233_get_gpio_input_mask(i, 0xffffffff));
        lcd_update();
        yield();
    }
}

bool dbg_hw_info(void)
{
    return dbg_hw_info_clkctrl() && dbg_hw_info_dma() && dbg_hw_info_adc() &&
        dbg_hw_info_power() && dbg_hw_info_powermgmt() && dbg_hw_info_rtc() &&
        dbg_hw_info_dcp() && dbg_hw_info_pinctrl() && dbg_hw_target_info();
}

bool dbg_ports(void)
{
    return false;
}
