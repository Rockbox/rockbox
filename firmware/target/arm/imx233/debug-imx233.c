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
#include "ocotp-imx233.h"
#include "string.h"
#include "stdio.h"

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

static struct
{
    const char *name;
    unsigned src;
} dbg_irqs[] =
{
    { "ssp2_err", INT_SRC_SSP2_ERROR },
    { "vdd5v", INT_SRC_VDD5V },
    { "dac_dma", INT_SRC_DAC_DMA },
    { "dac_err", INT_SRC_DAC_ERROR },
    { "adc_dma", INT_SRC_ADC_DMA },
    { "adc_err", INT_SRC_ADC_ERROR },
    { "usbctrl", INT_SRC_USB_CTRL },
    { "ssp1_dma", INT_SRC_SSP1_DMA },
    { "ssp1_err", INT_SRC_SSP1_ERROR },
    { "gpio0", INT_SRC_GPIO0 },
    { "gpio1", INT_SRC_GPIO1 },
    { "gpio2", INT_SRC_GPIO2 },
    { "ssp2_dma", INT_SRC_SSP2_DMA },
    { "i2c_dma", INT_SRC_I2C_DMA },
    { "i2c_err", INT_SRC_I2C_ERROR },
    { "timer0", INT_SRC_TIMER(0) },
    { "timer1", INT_SRC_TIMER(1) },
    { "timer2", INT_SRC_TIMER(2) },
    { "timer3", INT_SRC_TIMER(3) },
    { "touch_det", INT_SRC_TOUCH_DETECT },
    { "lradc_ch0", INT_SRC_LRADC_CHx(0) },
    { "lradc_ch1", INT_SRC_LRADC_CHx(1) },
    { "lradc_ch2", INT_SRC_LRADC_CHx(2) },
    { "lradc_ch3", INT_SRC_LRADC_CHx(3) },
    { "lradc_ch4", INT_SRC_LRADC_CHx(4) },
    { "lradc_ch5", INT_SRC_LRADC_CHx(5) },
    { "lradc_ch6", INT_SRC_LRADC_CHx(6) },
    { "lradc_ch7", INT_SRC_LRADC_CHx(7) },
    { "lcdif_dma", INT_SRC_LCDIF_DMA },
    { "lcdif_err", INT_SRC_LCDIF_ERROR },
    { "rtc_1msec", INT_SRC_RTC_1MSEC },
    { "dcp", INT_SRC_DCP },
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

        lcd_putsf(0, 0, "S C name bar      apb ahb una");
        for(unsigned i = 0; i < ARRAYLEN(dbg_channels); i++)
        {
            struct imx233_dma_info_t info = imx233_dma_get_info(dbg_channels[i].chan, DMA_INFO_ALL);
            lcd_putsf(0, i + 1, "%c %c %4s %8x %3x %3x %3x",
                info.gated ? 'g' : info.freezed ? 'f' : ' ',
                !info.int_enabled ? '-' : info.int_error ? 'e' : info.int_cmdcomplt ? 'c' : ' ',
                dbg_channels[i].name, info.bar, info.apb_bytes, info.ahb_bytes,
                info.nr_unaligned);
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
        int line = 0;
        unsigned trg, bo;
        bool en;
        int linreg;
        char buf[16];
        
        lcd_putsf(0, line++, "name  value bo linreg");
#define DISP_REGULATOR(name) \
        imx233_power_get_regulator(REGULATOR_##name, &trg, &bo); \
        imx233_power_get_regulator_linreg(REGULATOR_##name, &en, &linreg); \
        if(en) snprintf(buf, sizeof(buf), "%d", linreg); \
        else snprintf(buf, sizeof(buf), " "); \
        lcd_putsf(0, line++, "%6s %4d %4d %s", #name, trg, bo, buf); \

        DISP_REGULATOR(VDDD);
        DISP_REGULATOR(VDDA);
        DISP_REGULATOR(VDDIO);
        DISP_REGULATOR(VDDMEM);
        lcd_putsf(0, line++, "DC-DC: pll: %d   freq: %d", info.dcdc_sel_pllclk, info.dcdc_freqsel);
        lcd_putsf(0, line++, "charge: %d mA  stop: %d mA", info.charge_current, info.stop_current);
        lcd_putsf(0, line++, "charging: %d  bat_adj: %d", info.charging, info.batt_adj);
        lcd_putsf(0, line++, "4.2: en: %d  dcdc: %d", info._4p2_enable, info._4p2_dcdc);
        lcd_putsf(0, line++, "4.2: cmptrip: %d dropout: %d", info._4p2_cmptrip, info._4p2_dropout);
        lcd_putsf(0, line++, "5V: pwd_4.2_charge: %d", info._5v_pwd_charge_4p2);
        lcd_putsf(0, line++, "5V: chargelim: %d mA", info._5v_charge_4p2_limit);
        lcd_putsf(0, line++, "5V: dcdc: %d  xfer: %d", info._5v_enable_dcdc, info._5v_dcdc_xfer);
        lcd_putsf(0, line++, "5V: thr: %d mV use: %d cmps: %d", info._5v_vbusvalid_thr,
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
                lcd_putsf(5, i + 1, "%2d", imx233_clkctrl_is_clock_enabled(c.clk));
            if(c.has_bypass)
                lcd_putsf(8, i + 1, "%2d", imx233_clkctrl_get_bypass_pll(c.clk));
            if(c.has_idiv && imx233_clkctrl_get_clock_divisor(c.clk) != 0)
                lcd_putsf(10, i + 1, "%4d", imx233_clkctrl_get_clock_divisor(c.clk));
            if(c.has_fdiv && imx233_clkctrl_get_fractional_divisor(c.clk) != 0)
                lcd_putsf(16, i + 1, "%4d", imx233_clkctrl_get_fractional_divisor(c.clk));
            if(c.has_freq)
                lcd_putsf(21, i + 1, "%9d", imx233_clkctrl_get_clock_freq(c.clk));
            #undef c
        }
        int line = ARRAYLEN(dbg_clk) + 1;
        lcd_putsf(0, line, "as: %d/%d  emi sync: %d", imx233_clkctrl_is_auto_slow_enabled(),
            1 << imx233_clkctrl_get_auto_slow_divisor(), imx233_clkctrl_is_emi_sync_enabled());
        line++;
        lcd_putsf(0, line, "as monitor: ");
        int x_off = 12;
        bool first = true;
        unsigned line_w = lcd_getwidth() / font_get_width(font_get(lcd_getfont()), ' ');
        for(unsigned i = 0; i < ARRAYLEN(dbg_as_monitor); i++)
        {
            if(!imx233_clkctrl_is_auto_slow_monitor_enabled(dbg_as_monitor[i].monitor))
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

bool dbg_hw_info_icoll(void)
{
    lcd_setfont(FONT_SYSFIXED);

    int first_irq = 0;
    int dbg_irqs_count = sizeof(dbg_irqs) / sizeof(dbg_irqs[0]);
    int line_count = lcd_getheight() / font_get(lcd_getfont())->height;
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
                first_irq++;
                if(first_irq >= dbg_irqs_count)
                    first_irq = dbg_irqs_count - 1;
                break;
            case ACTION_STD_PREV:
                first_irq--;
                if(first_irq < 0)
                    first_irq = 0;
                break;
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        for(int i = first_irq, j = 0; i < dbg_irqs_count && j < line_count; i++, j++)
        {
            struct imx233_icoll_irq_info_t info = imx233_icoll_get_irq_info(dbg_irqs[i].src);
            lcd_putsf(0, j, "%s", dbg_irqs[i].name);
            if(info.enabled)
                lcd_putsf(10, j, "%d", info.freq);
        }
        lcd_update();
        yield();
    }
}

bool dbg_hw_info_pinctrl(void)
{
    lcd_setfont(FONT_SYSFIXED);

#ifdef IMX233_PINCTRL_DEBUG
    unsigned top_user = 0;
#endif
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
#ifdef IMX233_PINCTRL_DEBUG
                top_user++;
                break;
#endif
            case ACTION_STD_PREV:
#ifdef IMX233_PINCTRL_DEBUG
                if(top_user > 0)
                    top_user--;
                break;
#endif
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
#ifdef IMX233_PINCTRL_DEBUG
        unsigned cur_line = 6;
        unsigned last_line = lcd_getheight() / font_get(lcd_getfont())->height;
        unsigned cur_idx = 0;

        for(int bank = 0; bank < 4; bank++)
        for(int pin = 0; pin < 32; pin++)
        {
            const char *owner = imx233_pinctrl_get_pin_use(bank, pin);
            if(owner == NULL)
                continue;
            if(cur_idx++ >= top_user && cur_line < last_line)
                lcd_putsf(0, cur_line++, "B%dP%02d %s", bank, pin, owner);
        }
        if(cur_idx < top_user)
            top_user = cur_idx - 1;
#endif
        lcd_update();
        yield();
    }
}

struct
{
    const char *name;
    volatile uint32_t *addr;
} dbg_ocotp[] =
{
#define E(n,v) { .name = n, .addr = &v }
    E("CUST0", HW_OCOTP_CUSTx(0)), E("CUST1", HW_OCOTP_CUSTx(1)),
    E("CUST2", HW_OCOTP_CUSTx(2)), E("CUST0", HW_OCOTP_CUSTx(3)),
    E("HWCAP0", HW_OCOTP_HWCAPx(0)), E("HWCAP1", HW_OCOTP_HWCAPx(1)),
    E("HWCAP2", HW_OCOTP_HWCAPx(2)), E("HWCAP3", HW_OCOTP_HWCAPx(3)),
    E("HWCAP4", HW_OCOTP_HWCAPx(4)), E("HWCAP5", HW_OCOTP_HWCAPx(5)),
    E("SWCAP", HW_OCOTP_SWCAP), E("CUSTCAP", HW_OCOTP_CUSTCAP),
    E("OPS0", HW_OCOTP_OPSx(0)), E("OPS1", HW_OCOTP_OPSx(1)),
    E("OPS2", HW_OCOTP_OPSx(2)), E("OPS2", HW_OCOTP_OPSx(3)),
    E("UN0", HW_OCOTP_UNx(0)), E("UN1", HW_OCOTP_UNx(1)),
    E("UN2", HW_OCOTP_UNx(2)),
    E("ROM0", HW_OCOTP_ROMx(0)), E("ROM1", HW_OCOTP_ROMx(1)),
    E("ROM2", HW_OCOTP_ROMx(2)), E("ROM3", HW_OCOTP_ROMx(3)),
    E("ROM4", HW_OCOTP_ROMx(4)), E("ROM5", HW_OCOTP_ROMx(5)),
    E("ROM6", HW_OCOTP_ROMx(6)), E("ROM7", HW_OCOTP_ROMx(7)),
};

bool dbg_hw_info_ocotp(void)
{
    lcd_setfont(FONT_SYSFIXED);

    unsigned top_user = 0;

    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
                top_user++;
                break;
            case ACTION_STD_PREV:
                if(top_user > 0)
                    top_user--;
                break;
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        unsigned cur_line = 0;
        unsigned last_line = lcd_getheight() / font_get(lcd_getfont())->height;
        unsigned i = 0;

        for(i = 0; i < ARRAYLEN(dbg_ocotp); i++)
        {
            if(i >= top_user && cur_line < last_line)
            {
                lcd_putsf(0, cur_line, "%s", dbg_ocotp[i].name);
                lcd_putsf(8, cur_line++, "%x", imx233_ocotp_read(dbg_ocotp[i].addr));
            }
        }
        if(i < top_user)
            top_user = i - 1;
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info(void)
{
    return dbg_hw_info_clkctrl() && dbg_hw_info_dma() && dbg_hw_info_adc() &&
        dbg_hw_info_power() && dbg_hw_info_powermgmt() && dbg_hw_info_rtc() &&
        dbg_hw_info_dcp() && dbg_hw_info_pinctrl() && dbg_hw_info_icoll() &&
        dbg_hw_info_ocotp() && dbg_hw_target_info();
}

bool dbg_ports(void)
{
    return false;
}
