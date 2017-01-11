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

#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "dma-imx233.h"
#include "lcd.h"
#include "font.h"
#include "adc.h"
#include "usb.h"
#include "power-imx233.h"
#include "clkctrl-imx233.h"
#include "powermgmt-imx233.h"
#include "rtc-imx233.h"
#include "dualboot-imx233.h"
#include "dcp-imx233.h"
#include "pinctrl-imx233.h"
#include "ocotp-imx233.h"
#include "pwm-imx233.h"
#include "emi-imx233.h"
#include "audioin-imx233.h"
#include "audioout-imx233.h"
#include "timrot-imx233.h"
#include "string.h"
#include "stdio.h"
#include "button.h"
#include "button-imx233.h"
#include "sdmmc-imx233.h"
#include "led-imx233.h"
#include "storage.h"

#include "regs/usbphy.h"
#include "regs/timrot.h"
#include "regs/power.h"

#define ACT_NONE    0
#define ACT_CANCEL  1
#define ACT_OK      2
#define ACT_PREV    3
#define ACT_NEXT    4
#define ACT_LEFT    5
#define ACT_RIGHT   6
#define ACT_REPEAT  0x1000

int xlate_button(int btn)
{
    switch(btn)
    {
        case BUTTON_POWER:
#if defined(BUTTON_BACK)
        case BUTTON_BACK:
#elif defined(BUTTON_LEFT)
        case BUTTON_LEFT:
#else
#error no key for ACT_CANCEL
#endif
            return ACT_CANCEL;
#if defined(BUTTON_SELECT)
        case BUTTON_SELECT:
#elif defined(BUTTON_PLAY)
        case BUTTON_PLAY:
#elif defined(BUTTON_CENTER)
        case BUTTON_CENTER:
#else
#error no key for ACT_OK
#endif
            return ACT_OK;
        case BUTTON_UP:
            return ACT_PREV;
        case BUTTON_DOWN:
            return ACT_NEXT;
        default:
            return ACT_NONE;
    }
}

int my_get_status(void)
{
    return xlate_button(button_status());
}

int my_get_action(int tmo)
{
    int btn = button_get_w_tmo(tmo);
    while(btn & BUTTON_REL)
        btn = button_get_w_tmo(tmo);
    bool repeat = btn & BUTTON_REPEAT;
    int act = xlate_button(btn & ~BUTTON_REPEAT);
    if(repeat)
        act |= ACT_REPEAT;
    return act;
}

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
#if IMX233_SUBTARGET >= 3780
    { "ssp2_dma", INT_SRC_SSP2_DMA },
    { "ssp2_err", INT_SRC_SSP2_ERROR },
    { "lcdif_dma", INT_SRC_LCDIF_DMA },
    { "lcdif_err", INT_SRC_LCDIF_ERROR },
    { "dcp", INT_SRC_DCP },
#endif
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
    { "rtc_1msec", INT_SRC_RTC_1MSEC },
};

bool dbg_hw_info_dma(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 25);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();

        lcd_putsf(0, 0, "S C name bar      apb ahb una");
        for(unsigned i = 0; i < ARRAYLEN(dbg_channels); i++)
        {
            struct imx233_dma_info_t info = imx233_dma_get_info(dbg_channels[i].chan, DMA_INFO_ALL);
            lcd_putsf(0, i + 1, "%c %c %4s %8x %3x %3x %3x",
                info.gated ? 'g' : info.frozen ? 'f' : ' ',
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
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
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
#if IMX233_SUBTARGET >= 3700
        DISP_REGULATOR(VDDA);
        DISP_REGULATOR(VDDIO);
#endif
#if IMX233_SUBTARGET >= 3780
        DISP_REGULATOR(VDDMEM);
#endif
        lcd_putsf(0, line++, "dcdc: pll: %d freq: %d", info.dcdc_sel_pllclk, info.dcdc_freqsel);
        lcd_putsf(0, line++, "chrg: %d mA / %d mA", info.charge_current, info.stop_current);
        lcd_putsf(0, line++, "chrging: %d  batadj: %d", info.charging, info.batt_adj);
        lcd_putsf(0, line++, "4.2: en: %d  dcdc: %d", info._4p2_enable, info._4p2_dcdc);
        lcd_putsf(0, line++, "4.2: cmptrip: %d", info._4p2_cmptrip);
        lcd_putsf(0, line++, "4.2: dropout: %d", info._4p2_dropout);
        lcd_putsf(0, line++, "5v: pwd_4.2_charge: %d", info._5v_pwd_charge_4p2);
        lcd_putsf(0, line++, "5v: chrglim: %d mA", info._5v_charge_4p2_limit);
        lcd_putsf(0, line++, "5v: dcdc: %d  xfer: %d", info._5v_enable_dcdc, info._5v_dcdc_xfer);
        lcd_putsf(0, line++, "5v: thr: %d mV", info._5v_vbusvalid_thr);
        lcd_putsf(0, line++, "5v: use: %d cmps: %d", info._5v_vbusvalid_detect, info._5v_vbus_cmps);
#if IMX233_SUBTARGET >= 3780
        lcd_putsf(0, line++, "pwrup: %x", BF_RD(POWER_STS, PWRUP_SOURCE));
#endif

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_lradc(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 25);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();

        /* add battery readout in mV, this it is not the direct output of a channel */
        lcd_putsf(0, 0, "Battery(mV) %d", _battery_voltage());
        for(unsigned i = 0; i < NUM_ADC_CHANNELS; i++)
        {
            lcd_putsf(0, i + 1, "%s %d", adc_name(i), adc_read(i));
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
#if IMX233_SUBTARGET >= 3700
    { CLK_PIX, "pix", true, true, true, true, true },
#endif
    { CLK_SSP, "ssp", true, true, true, false, true },
    { CLK_IO,  "io",  false, false, false, true, true },
    { CLK_CPU, "cpu", false, true, true, true, true },
    { CLK_HBUS, "hbus", false, false, true, true, true },
    { CLK_EMI, "emi", false, true, true, true, true },
    { CLK_XBUS, "xbus", false, false, true, false, true }
};

bool dbg_hw_info_clkctrl(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();

        /*               012345678901234567890123456789 */
#if LCD_WIDTH < 240
        lcd_putsf(0, 0, "name en frequency");
#else
        lcd_putsf(0, 0, "name en by idiv fdiv frequency");
#endif
        for(unsigned i = 0; i < ARRAYLEN(dbg_clk); i++)
        {
            #define c dbg_clk[i]
            lcd_putsf(0, i + 1, "%4s", c.name);
            if(c.has_enable)
                lcd_putsf(5, i + 1, "%2d", imx233_clkctrl_is_enabled(c.clk));
#if LCD_WIDTH >= 240
#if IMX233_SUBTARGET >= 3700
            if(c.has_bypass)
                lcd_putsf(8, i + 1, "%2d", imx233_clkctrl_get_bypass(c.clk));
#endif
            if(c.has_idiv && imx233_clkctrl_get_div(c.clk) != 0)
                lcd_putsf(10, i + 1, "%4d", imx233_clkctrl_get_div(c.clk));
#if IMX233_SUBTARGET >= 3700
            if(c.has_fdiv && imx233_clkctrl_get_frac_div(c.clk) != 0)
                lcd_putsf(16, i + 1, "%4d", imx233_clkctrl_get_frac_div(c.clk));
#endif
            if(c.has_freq)
                lcd_putsf(21, i + 1, "%9d", imx233_clkctrl_get_freq(c.clk));
#else /* LCD_WIDTH < 240 */
            if(c.has_freq)
                lcd_putsf(8, i + 1, "%9d", imx233_clkctrl_get_freq(c.clk));
#endif
            #undef c
        }
        int line = ARRAYLEN(dbg_clk) + 1;
        if(!imx233_clkctrl_is_auto_slow_enabled())
            lcd_putsf(0, line++, "auto-slow: disabled");
        else
            lcd_putsf(0, line++, "auto-slow: 1/%d", 1 << imx233_clkctrl_get_auto_slow_div());

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_powermgmt(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        struct imx233_powermgmt_info_t info = imx233_powermgmt_get_info();

        lcd_putsf(0, 0, "state: %s",
            info.state == DISCHARGING ? "discharging" :
#if CONFIG_CHARGING >= CHARGING_MONITOR
            info.state == CHARGE_STATE_DISABLED ? "disabled" :
            info.state == CHARGE_STATE_ERROR ? "error" :
#endif
#if CONFIG_CHARGING >= CHARGING_MONITOR
            info.state == TRICKLE ? "trickle" :
            info.state == TOPOFF ? "topoff" :
            info.state == CHARGING ? "charging" :
#endif
            "<unknown>");
        lcd_putsf(0, 1, "charging tmo: %d", info.charging_timeout);
        lcd_putsf(0, 2, "topoff tmo: %d", info.topoff_timeout);

        lcd_update();
        yield();
    }
}

#if IMX233_SUBTARGET >= 3780
/* stmp < 3780 does not have a 4.2V rail and thus cannot do this magic trick */
bool dbg_hw_info_power2(void)
{
    lcd_setfont(FONT_SYSFIXED);
    bool holding_select = false;
    int select_hold_time = 0;

    while(1)
    {
        int button = my_get_action(HZ / 10);
        if(button == ACT_NEXT || button == ACT_PREV)
        {
            lcd_setfont(FONT_UI);
            return true;
        }
        else if(button == ACT_CANCEL)
        {
            lcd_setfont(FONT_UI);
            return false;
        }

        button = my_get_status();
        if(button == ACT_OK && !holding_select)
        {
            holding_select = true;
            select_hold_time = current_tick;
        }
        else if(button != ACT_OK && holding_select)
        {
            holding_select = false;
        }

        /* disable feature if unsafe: we need 4.2 and dcdc fully operational */
        bool feat_safe = usb_detect() == USB_INSERTED && BF_RD(POWER_DCDC4P2, ENABLE_DCDC)
            && BF_RD(POWER_DCDC4P2, ENABLE_4P2) && BF_RD(POWER_5VCTRL, ENABLE_DCDC)
            && !BF_RD(POWER_5VCTRL, PWD_CHARGE_4P2);
        bool batt_disabled = (BF_RD(POWER_DCDC4P2, DROPOUT_CTRL) == 0xc);
        if(holding_select && TIME_AFTER(current_tick, select_hold_time + HZ))
        {
            if(batt_disabled)
            {
                BF_CLR(POWER_CHARGE, PWD_BATTCHRG); /* enable charger again */
                BF_WR(POWER_DCDC4P2, DROPOUT_CTRL(0xe)); /* select greater, 200 mV drop */
            }
            else if(feat_safe)
            {
                BF_WR(POWER_DCDC4P2, DROPOUT_CTRL(0xc)); /* always select 4.2, 200 mV drop */
                BF_SET(POWER_CHARGE, PWD_BATTCHRG); /* disable charger */
            }
            holding_select = false;
            /* return to the beginning of the loop to gather more information
             * about HW state before displaying it */
            continue;
        }

        lcd_clear_display();
        if(!batt_disabled)
        {
            lcd_putsf(0, 0, "Hold select for 1 sec");
            lcd_putsf(0, 1, "to disable battery");
            lcd_putsf(0, 1, "and battery charger.");
            lcd_putsf(0, 2, "The device will run");
            lcd_putsf(0, 3, "entirely from USB.");
            lcd_putsf(0, 5, "WARNING");
            lcd_putsf(0, 6, "This is a debug");
            lcd_putsf(0, 7, "feature !");
            if(!feat_safe)
            {
                lcd_putsf(0, 9, "NOTE: unavailable");
                lcd_putsf(0, 10, "Plug USB to enable.");
            }
        }
        else
        {
            lcd_putsf(0, 0, "Battery is DISABLED.");
            lcd_putsf(0, 1, "Hold select for 1 sec");
            lcd_putsf(0, 2, "to renable battery.");
            lcd_putsf(0, 4, "WARNING");
            lcd_putsf(0, 5, "Do not unplug USB !");
        }

        lcd_update();
        yield();
    }
}
#endif /* IMX233_SUBTARGET >= 3780 */

bool dbg_hw_info_rtc(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        struct imx233_rtc_info_t info = imx233_rtc_get_info();

        int line = 0;
        lcd_putsf(0, line++, "seconds: %lu", info.seconds);
        lcd_putsf(0, line++, "alarm: %lu", info.alarm);
        for(int i = 0; i < 6; i++)
            lcd_putsf(0, line++, "persist%d: 0x%lx", i, info.persistent[i]);

        lcd_update();
        yield();
    }
}

#if IMX233_SUBTARGET >= 3780
bool dbg_hw_info_dcp(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
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
#else
bool dbg_hw_info_dcp(void)
{
    return true;
}
#endif

bool dbg_hw_info_icoll(void)
{
    lcd_setfont(FONT_SYSFIXED);

    int first_irq = 0;
    int dbg_irqs_count = sizeof(dbg_irqs) / sizeof(dbg_irqs[0]);
    int line_count = lcd_getheight() / font_get(lcd_getfont())->height;

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
                first_irq++;
                if(first_irq >= dbg_irqs_count)
                    first_irq = dbg_irqs_count - 1;
                break;
            case ACT_PREV:
                first_irq--;
                if(first_irq < 0)
                    first_irq = 0;
                break;
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
        for(int i = first_irq; i < dbg_irqs_count && line < line_count; i++)
        {
            struct imx233_icoll_irq_info_t info = imx233_icoll_get_irq_info(dbg_irqs[i].src);
            static char prio[4] = {'-', '+', '^', '!'};
            if(info.enabled || info.freq > 0)
            {
                lcd_putsf(0, line, "%c%s", prio[info.priority & 3], dbg_irqs[i].name);
                lcd_putsf(11, line, "%d %d %d", info.freq, info.max_time, info.total_time);
                line++;
            }
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
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
#ifdef IMX233_PINCTRL_DEBUG
                top_user++;
                break;
#endif
            case ACT_PREV:
#ifdef IMX233_PINCTRL_DEBUG
                if(top_user > 0)
                    top_user--;
                break;
#endif
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        for(int i = 0; i < 4; i++)
            lcd_putsf(0, i, "DIN%d = 0x%08x", i, HW_PINCTRL_DINn(i));
#ifdef IMX233_PINCTRL_DEBUG
        unsigned cur_line = 6;
        unsigned last_line = lcd_getheight() / font_get(lcd_getfont())->height;
        unsigned cur_idx = 0;

        for(int bank = 0; bank < 4; bank++)
        for(int pin = 0; pin < 32; pin++)
        {
            const char *owner = imx233_pinctrl_blame(bank, pin);
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
#if IMX233_SUBTARGET >= 3700
#define E(n,v) { .name = n, .addr = &v }
    E("CUST0", HW_OCOTP_CUSTn(0)), E("CUST1", HW_OCOTP_CUSTn(1)),
    E("CUST2", HW_OCOTP_CUSTn(2)), E("CUST0", HW_OCOTP_CUSTn(3)),
    E("HWCAP0", HW_OCOTP_HWCAPn(0)), E("HWCAP1", HW_OCOTP_HWCAPn(1)),
    E("HWCAP2", HW_OCOTP_HWCAPn(2)), E("HWCAP3", HW_OCOTP_HWCAPn(3)),
    E("HWCAP4", HW_OCOTP_HWCAPn(4)), E("HWCAP5", HW_OCOTP_HWCAPn(5)),
    E("SWCAP", HW_OCOTP_SWCAP), E("CUSTCAP", HW_OCOTP_CUSTCAP),
    E("OPS0", HW_OCOTP_OPSn(0)), E("OPS1", HW_OCOTP_OPSn(1)),
    E("OPS2", HW_OCOTP_OPSn(2)), E("OPS2", HW_OCOTP_OPSn(3)),
    E("UN0", HW_OCOTP_UNn(0)), E("UN1", HW_OCOTP_UNn(1)),
    E("UN2", HW_OCOTP_UNn(2)),
    E("ROM0", HW_OCOTP_ROMn(0)), E("ROM1", HW_OCOTP_ROMn(1)),
    E("ROM2", HW_OCOTP_ROMn(2)), E("ROM3", HW_OCOTP_ROMn(3)),
    E("ROM4", HW_OCOTP_ROMn(4)), E("ROM5", HW_OCOTP_ROMn(5)),
    E("ROM6", HW_OCOTP_ROMn(6)), E("ROM7", HW_OCOTP_ROMn(7)),
#undef E
#else
#define E(n,v) { .name = n, .addr = &v }
    E("LASERFUSE0", HW_RTC_LASERFUSEn(0)),
    E("LASERFUSE1", HW_RTC_LASERFUSEn(1)),
    E("LASERFUSE2", HW_RTC_LASERFUSEn(2)),
    E("LASERFUSE3", HW_RTC_LASERFUSEn(3)),
    E("LASERFUSE4", HW_RTC_LASERFUSEn(4)),
    E("LASERFUSE5", HW_RTC_LASERFUSEn(5)),
    E("LASERFUSE6", HW_RTC_LASERFUSEn(6)),
    E("LASERFUSE7", HW_RTC_LASERFUSEn(7)),
    E("LASERFUSE8", HW_RTC_LASERFUSEn(8)),
    E("LASERFUSE9", HW_RTC_LASERFUSEn(9)),
    E("LASERFUSE10", HW_RTC_LASERFUSEn(10)),
    E("LASERFUSE11", HW_RTC_LASERFUSEn(11)),
#undef E
#endif
};

bool dbg_hw_info_ocotp(void)
{
    lcd_setfont(FONT_SYSFIXED);

    unsigned top_user = 0;

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
                top_user++;
                break;
            case ACT_PREV:
                if(top_user > 0)
                    top_user--;
                break;
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
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

static void get_pwm_freq_duty(int chan, int *freq, int *duty)
{
    struct imx233_pwm_info_t info = imx233_pwm_get_info(chan);
    *freq = imx233_clkctrl_get_freq(CLK_XTAL) * 1000 / info.cdiv / info.period;
    *duty = (info.inactive - info.active) * 100 / info.period;
}

bool dbg_hw_info_pwm(void)
{
    lcd_setfont(FONT_SYSFIXED);
    bool detailled_view = false;

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
                detailled_view = !detailled_view;
                break;
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
        if(detailled_view)
            lcd_putsf(0, line++, "c cdiv period active/x inactive");
        else
            lcd_putsf(0, line++, "pwm");
        for(int i = 0; i < IMX233_PWM_NR_CHANNELS; i++)
        {
            struct imx233_pwm_info_t info = imx233_pwm_get_info(i);
            if(!info.enabled)
                continue;
            if(detailled_view)
            {
                lcd_putsf(0, line++, "%d %4d %6d %6d/%c %6d/%c", i, info.cdiv,
                    info.period, info.active, info.active_state,
                    info.inactive, info.inactive_state);
            }
            else
            {
                int freq, duty;
                get_pwm_freq_duty(i, &freq, &duty);
                const char *prefix = "";
                if(freq > 1000)
                {
                    prefix = "K";
                    freq /= 1000;
                }
                lcd_putsf(0, line++, "%d @%d %sHz, %d%% %c/%c", i, freq, prefix,
                    duty, info.active_state, info.inactive_state);
            }
        }

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_usb(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
                BF_SET(USBPHY_CTRL, ENOTGIDDETECT);
                BF_SET(USBPHY_CTRL, ENDEVPLUGINDETECT);
                break;
            case ACT_PREV:
                BF_CLR(USBPHY_CTRL, ENOTGIDDETECT);
                BF_CLR(USBPHY_CTRL, ENDEVPLUGINDETECT);
                break;
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
#if IMX233_SUBTARGET >= 3700
        lcd_putsf(0, line++, "VBUS valid: %d/%d", BF_RD(POWER_STS, VBUSVALID), BF_RD(POWER_STS, VBUSVALID_STATUS));
        lcd_putsf(0, line++, "A valid: %d/%d", BF_RD(POWER_STS, AVALID), BF_RD(POWER_STS, AVALID_STATUS));
        lcd_putsf(0, line++, "B valid: %d/%d", BF_RD(POWER_STS, BVALID), BF_RD(POWER_STS, BVALID_STATUS));
#else
        lcd_putsf(0, line++, "VBUS valid: %d/%d", BF_RD(POWER_STS, VBUSVALID));
        lcd_putsf(0, line++, "A valid: %d/%d", BF_RD(POWER_STS, AVALID));
        lcd_putsf(0, line++, "B valid: %d/%d", BF_RD(POWER_STS, BVALID));
#endif
        lcd_putsf(0, line++, "session end: %d", BF_RD(POWER_STS, SESSEND));
        lcd_putsf(0, line++, "dev plugin: %d", BF_RD(USBPHY_STATUS, DEVPLUGIN_STATUS));
        lcd_putsf(0, line++, "OTG ID: %d", BF_RD(USBPHY_STATUS, OTGID_STATUS));

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_emi(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        struct imx233_emi_info_t info = imx233_emi_get_info();
        int line = 0;
        lcd_putsf(0, line++, "EMI");
        lcd_putsf(0, line++, "rows: %d", info.rows);
        lcd_putsf(0, line++, "columns: %d", info.columns);
        lcd_putsf(0, line++, "banks: %d", info.banks);
        lcd_putsf(0, line++, "chips: %d", info.chips);
        lcd_putsf(0, line++, "size: %d MiB", info.size / 1024 / 1024);
        lcd_putsf(0, line++, "cas: %d.%d", info.cas / 2, 5 * (info.cas % 2));

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_audio(void)
{
    static const char *hp_sel[2] = {"DAC", "Line1"};
    static const char *mux_sel[4] = {"Mic", "Line1", "HP", "Line2"};
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        struct imx233_audioout_info_t out = imx233_audioout_get_info();
        struct imx233_audioin_info_t in = imx233_audioin_get_info();
        int line = 0;
#define display_sys(st, sys, name) \
        if(st.sys) \
        { \
            char buffer[64]; \
            snprintf(buffer, 64, "%s: ", name); \
            for(int i = 0; i < 2; i++) \
            { \
                if(st.sys##mute[i]) \
                    strcat(buffer, "mute"); \
                else \
                    snprintf(buffer + strlen(buffer), 64,  "%d.%d", \
                        /* properly handle negative values ! */ \
                        st.sys##vol[i] / 10, (10 + (st.sys##vol[i]) % 10) % 10); \
                if(i == 0) \
                    strcat(buffer, " / "); \
                else \
                    strcat(buffer, " dB"); \
            } \
            lcd_putsf(0, line++, "%s", buffer); \
        } \
        else \
            lcd_putsf(0, line++, "%s: powered down", name);
        display_sys(out, dac, "DAC");
        display_sys(out, hp, "HP");
        display_sys(out, spkr, "SPKR");
        display_sys(in, adc, "ADC");
        display_sys(in, mux, "MUX");
        display_sys(in, mic, "MIC");
#undef display_sys
        lcd_putsf(0, line++, "capless: %d", out.capless);
        lcd_putsf(0, line++, "HP select: %s", hp_sel[out.hpselect]);
        lcd_putsf(0, line++, "MUX select: %s / %s", mux_sel[in.muxselect[0]], mux_sel[in.muxselect[1]]);

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_timrot(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
        for(int i = 0; i < 4; i++)
        {
            struct imx233_timrot_info_t info = imx233_timrot_get_info(i);
            const char *unit = NULL;
            const char *unit_prefix = "";
            int src_freq = 1;
            switch(info.src)
            {
                case BV_TIMROT_TIMCTRLn_SELECT__NEVER_TICK: src_freq = 0; break;
                case BV_TIMROT_TIMCTRLn_SELECT__PWM0: unit = "PWM0"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__PWM1: unit = "PWM1"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__PWM2: unit = "PWM2"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__PWM3: unit = "PWM3"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__PWM4: unit = "PWM4"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__ROTARYA: unit = "ROTA"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__ROTARYB: unit = "ROTB"; break;
                case BV_TIMROT_TIMCTRLn_SELECT__32KHZ_XTAL: src_freq = 32000; break;
                case BV_TIMROT_TIMCTRLn_SELECT__8KHZ_XTAL: src_freq = 8000; break;
                case BV_TIMROT_TIMCTRLn_SELECT__4KHZ_XTAL: src_freq = 4000; break;
                case BV_TIMROT_TIMCTRLn_SELECT__1KHZ_XTAL: src_freq = 1000; break;
                case BV_TIMROT_TIMCTRLn_SELECT__TICK_ALWAYS: 
                default: src_freq = 24000000 / (1 << info.prescale); break;
            }
            int count = info.reload ? info.fixed_count + 1 : info.run_count;
            if(src_freq == 0 || count == 0)
                continue;
            unsigned long long freq;
            if(info.reload)
                freq = (unsigned long long)src_freq * 1000 / count;
            else
                freq = count * 1000 / src_freq;

            if(freq >= 1000000000)
            {
                unit_prefix = "M";
                freq /= 1000000;
            }
            else if(freq >= 1000000)
            {
                unit_prefix = "k";
                freq /= 1000;
            }
            char str[32];
            if(freq % 1000)
                snprintf(str, sizeof(str), "%lu.%lu", (unsigned long)freq / 1000, (unsigned long)freq % 1000);
            else
                snprintf(str, sizeof(str), "%lu", (unsigned long)freq / 1000);
            char str2[32];
            if(unit)
                snprintf(str2, sizeof(str2), "%s%s(%s)", unit_prefix, info.reload ? "H" : "", unit);
            else
                snprintf(str2, sizeof(str2), "%s%s", unit_prefix, info.reload ? "Hz" : "s");
            lcd_putsf(0, line++, "%ctimer %d: %s %s", info.polarity ? '+' : '-', i, str, str2);
        }

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_button(void)
{
    lcd_setfont(FONT_SYSFIXED);
#if IMX233_SUBTARGET >= 3700
    int orig_vddio_val, orig_vddio_brownout;
    imx233_power_get_regulator(REGULATOR_VDDIO, &orig_vddio_val, &orig_vddio_brownout);
    int vddio_val = orig_vddio_val;
    int vddio_brownout = orig_vddio_brownout;
#endif

    while(1)
    {
        int btn = my_get_action(0);
        switch(btn)
        {
#if IMX233_SUBTARGET >= 3700
            case ACT_PREV:
                vddio_val -= 100; /* mV */
                vddio_brownout -= 100; /* mV */
                imx233_power_set_regulator(REGULATOR_VDDIO, vddio_val, vddio_brownout);
                break;
            case ACT_NEXT:
                vddio_val += 100; /* mV */
                vddio_brownout += 100; /* mV */
                imx233_power_set_regulator(REGULATOR_VDDIO, vddio_val, vddio_brownout);
                break;
#endif
            case ACT_OK:
#if IMX233_SUBTARGET >= 3700
                imx233_power_set_regulator(REGULATOR_VDDIO, orig_vddio_val, orig_vddio_brownout);
#endif
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
#if IMX233_SUBTARGET >= 3700
                imx233_power_set_regulator(REGULATOR_VDDIO, orig_vddio_val, orig_vddio_brownout);
#endif
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;

#ifdef HAVE_BUTTON_DATA
        int data;
        btn = button_read_device(&data);
#else
        btn = button_read_device();
#endif
        lcd_putsf(0, line++, "raw buttons: %x", btn);
#ifdef HAS_BUTTON_HOLD
        lcd_putsf(0, line++, "hold: %d", button_hold());
#endif
#ifdef HAVE_HEADPHONE_DETECTION
        lcd_putsf(0, line++, "headphones: %d", headphones_inserted());
#endif
#ifdef HAVE_BUTTON_DATA
#ifdef HAVE_TOUCHSCREEN
        lcd_putsf(0, line++, "touch: x=%d y=%d", data >> 16, data & 0xffff);
#else
        lcd_putsf(0, line++, "data: %d", data);
#endif
#endif

#define MAP imx233_button_map
        for(int i = 0; MAP[i].btn != IMX233_BUTTON_END; i++)
        {
            bool val = imx233_button_read_btn(i);
            int raw = imx233_button_read_raw(i);
            char type[20];
            char path[128];
            char flags[128];
            if(MAP[i].periph == IMX233_BUTTON_GPIO)
            {
                snprintf(type, sizeof(type), "gpio");
                snprintf(path, sizeof(path), "bank=%d pin=%d", MAP[i].u.gpio.bank, MAP[i].u.gpio.pin);
            }
            else if(MAP[i].periph == IMX233_BUTTON_LRADC)
            {
                static const char *op_name[] =
                {
                    [IMX233_BUTTON_EQ] = "eq",
                    [IMX233_BUTTON_GT] = "gt",
                    [IMX233_BUTTON_LT] = "lt"
                };
                char rel_name[20];
                snprintf(type, sizeof(type), "adc");
                if(MAP[i].u.lradc.relative != -1)
                    snprintf(rel_name, sizeof(rel_name), " %s", MAP[MAP[i].u.lradc.relative].name);
                else
                    rel_name[0] = 0;
                snprintf(path, sizeof(path), "%d %s %d%s %d", MAP[i].u.lradc.src,
                    op_name[MAP[i].u.lradc.op], MAP[i].u.lradc.value, rel_name,
                    MAP[i].u.lradc.margin);
            }
            else if(MAP[i].periph == IMX233_BUTTON_PSWITCH)
            {
                snprintf(type, sizeof(type), "psw");
                snprintf(path, sizeof(path), "level=%d", MAP[i].u.pswitch.level);
            }
            else
            {
                snprintf(type, sizeof(type), "unk");
                snprintf(path, sizeof(path), "unknown");
            }
            flags[0] = 0;
            if(MAP[i].flags & IMX233_BUTTON_INVERTED)
                strcat(flags, " inv");
            if(MAP[i].flags & IMX233_BUTTON_PULLUP)
                strcat(flags, " pull");
#if LCD_WIDTH <= LCD_HEIGHT
            lcd_putsf(0, line++, "%s %d %d/%d %d %s", MAP[i].name, val,
                MAP[i].rounds, MAP[i].threshold, raw, type);
            lcd_putsf(0, line++, "  %s%s", path, flags);
#else
            lcd_putsf(0, line++, "%s %d %d/%d %d %s  %s%s", MAP[i].name, val,
                MAP[i].rounds, MAP[i].threshold, raw, type, path, flags);
#endif
        }
#undef MAP

        lcd_update();
        yield();
    }
}

bool dbg_hw_info_sdmmc(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
#if CONFIG_STORAGE & STORAGE_MMC
        int mmc_idx = 0;
#endif
#if CONFIG_STORAGE & STORAGE_SD
        int sd_idx = 0;
#endif
        for(int drive = 0; drive < storage_num_drives(); drive++)
        {
            struct sdmmc_info_t info;
            if(false) {}
#if CONFIG_STORAGE & STORAGE_MMC
            else if(storage_driver_type(drive) == STORAGE_MMC_NUM)
                info = imx233_mmc_get_info(mmc_idx++);
#endif
#if CONFIG_STORAGE & STORAGE_SD
            else if(storage_driver_type(drive) == STORAGE_SD_NUM)
                info = imx233_sd_get_info(sd_idx++);
#endif
            else
                continue;
            lcd_putsf(0, line++, "%s", info.slot_name);
#ifdef HAVE_HOTSWAP
            bool removable = storage_removable(info.drive);
            bool present = storage_present(info.drive);
            if(removable)
                lcd_putsf(0, line++, "  removable %spresent", present ? "" : "not ");
            if(!present)
                continue;
#endif
            lcd_putsf(0, line++, "  bus: %d  sbc: %d", info.bus_width, info.has_sbc);
            lcd_putsf(0, line++, "  hs: %s", info.hs_enabled ? "enabled" :
                info.hs_capable ? "disabled" : "not capable");
        }
        lcd_update();
        yield();
    }
}

static const char *get_led_col(enum imx233_led_color_t col)
{
    switch(col)
    {
        case LED_RED: return "red";
        case LED_GREEN: return "green";
        case LED_BLUE: return "blue";
        default: return "unknown";
    }
}

bool dbg_hw_info_led(void)
{
    lcd_setfont(FONT_SYSFIXED);
    int cur_led = 0, cur_chan = 0;
    bool nr_leds = imx233_led_get_count();
    struct imx233_led_t *leds = imx233_led_get_info();
    bool prev_pending = false;
    bool next_pending = false;
    bool editing = false;

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
                if(nr_leds > 0 && !editing)
                {
                    cur_chan++;
                    if(cur_chan == leds[cur_led].nr_chan)
                    {
                        cur_chan = 0;
                        cur_led = (cur_led + 1) % nr_leds;
                    }
                }
                else
                    next_pending = true;
                break;
            case ACT_PREV:
                if(nr_leds > 0 && !editing)
                {
                    cur_chan--;
                    if(cur_chan == -1)
                    {
                        cur_led = (cur_led + nr_leds - 1) % nr_leds;
                        cur_chan = leds[cur_led].nr_chan - 1;
                    }
                }
                else
                    prev_pending = true;
                break;
            case ACT_OK:
                editing = !editing;
                break;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
        if(nr_leds == 0)
            lcd_putsf(0, line++, "This device has no LED!");
        for(int led = 0; led < imx233_led_get_count(); led++)
        {
            lcd_putsf(0, line++, "LED %d:", led);
            for(int chan = 0; chan < leds[led].nr_chan; chan++)
            {
                /* read current configuration */
                char buffer[64];
                bool use_pwm = false;
                int duty = 0, freq = 1;
                bool on = false;
                if(leds[led].chan[chan].has_pwm &&
                        imx233_pwm_is_enabled(leds[led].chan[chan].pwm_chan))
                {
                    get_pwm_freq_duty(leds[led].chan[chan].pwm_chan, &freq, &duty);
                    /* assume active is high and inactive is low */
                    snprintf(buffer, sizeof(buffer), "%d Hz, %d%%", freq, duty);
                    use_pwm = true;
                }
                else
                {
                    on = imx233_pinctrl_get_gpio(leds[led].chan[chan].gpio_bank,
                        leds[led].chan[chan].gpio_pin);
                    snprintf(buffer, sizeof(buffer), "%s", on ? "on" : "off");
                }
                if(cur_led == led && cur_chan == chan)
                    lcd_set_foreground(editing ? LCD_RGBPACK(255, 0, 0) : LCD_RGBPACK(0, 0, 255));
                lcd_putsf(0, line++, "  %s: %s",
                    get_led_col(leds[led].chan[chan].color), buffer);
                lcd_set_foreground(LCD_WHITE);
                /* do edit */
                if(cur_led != led || cur_chan != chan || !editing)
                    continue;
                if(!next_pending && !prev_pending)
                    continue;
                bool inc = next_pending;
                next_pending = false;
                prev_pending = false;
                if(use_pwm)
                {
                    duty += inc ? 10 : -10;
                    if(duty < 0)
                        duty = 0;
                    if(duty > 100)
                        duty = 100;
                    imx233_led_set_pwm(cur_led, cur_chan, freq, duty);
                }
                else
                    imx233_led_set(cur_led, cur_chan, !on);
            }
        }
        lcd_update();
        yield();
    }
}

#ifdef HAVE_DUALBOOT_STUB
bool dbg_hw_info_dualboot(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
            case ACT_PREV:
            {
                /* only if boot mode is supported... */
                if(!imx233_dualboot_get_field(DUALBOOT_CAP_BOOT))
                    break;
                /* change it */
                unsigned boot = imx233_dualboot_get_field(DUALBOOT_BOOT);
                if(boot == IMX233_BOOT_NORMAL)
                    boot = IMX233_BOOT_OF;
                else if(boot == IMX233_BOOT_OF)
                    boot = IMX233_BOOT_UPDATER;
                else
                    boot = IMX233_BOOT_NORMAL;
                imx233_dualboot_set_field(DUALBOOT_BOOT, boot);
                break;
            }
            case ACT_OK:
                lcd_setfont(FONT_UI);
                return true;
            case ACT_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        int line = 0;
        unsigned cap_boot = imx233_dualboot_get_field(DUALBOOT_CAP_BOOT);
        lcd_putsf(0, line++, "cap_boot: %s", cap_boot ? "yes" : "no");
        if(cap_boot)
        {
            unsigned boot = imx233_dualboot_get_field(DUALBOOT_BOOT);
            lcd_putsf(0, line++, "boot: %s",
                boot == IMX233_BOOT_NORMAL ? "normal"
                : boot == IMX233_BOOT_OF ? "of"
                : boot == IMX233_BOOT_UPDATER ? "updater" : "?");
        }

        lcd_update();
        yield();
    }
}
#endif

static struct
{
    const char *name;
    bool (*fn)(void);
} debug_screens[] =
{
    {"clock", dbg_hw_info_clkctrl},
    {"dma", dbg_hw_info_dma},
    {"lradc", dbg_hw_info_lradc},
    {"power", dbg_hw_info_power},
#if IMX233_SUBTARGET >= 3780
    {"power2", dbg_hw_info_power2},
#endif
    {"powermgmt", dbg_hw_info_powermgmt},
    {"rtc", dbg_hw_info_rtc},
    {"dcp", dbg_hw_info_dcp},
    {"pinctrl", dbg_hw_info_pinctrl},
    {"icoll", dbg_hw_info_icoll},
    {"ocotp", dbg_hw_info_ocotp},
    {"pwm", dbg_hw_info_pwm},
    {"usb", dbg_hw_info_usb},
    {"emi", dbg_hw_info_emi},
    {"audio", dbg_hw_info_audio},
    {"timrot", dbg_hw_info_timrot},
    {"button", dbg_hw_info_button},
    {"sdmmc", dbg_hw_info_sdmmc},
    {"led", dbg_hw_info_led},
#ifdef HAVE_DUALBOOT_STUB
    {"dualboot", dbg_hw_info_dualboot},
#endif
    {"target", dbg_hw_target_info},
};

bool dbg_hw_info(void)
{
    int nr_lines = lcd_getheight() / font_get(lcd_getfont())->height;
    int len = ARRAYLEN(debug_screens);
    int top_visible = 0;
    int highlight = 0;
    while(1)
    {
        int button = my_get_action(HZ / 10);
        switch(button)
        {
            case ACT_NEXT:
                highlight = (highlight + 1) % len;
                break;
            case ACT_PREV:
                highlight = (highlight + len - 1) % len;
                break;
            case ACT_OK:
                // if the screen returns true, advance to next screen
                while(debug_screens[highlight].fn())
                    highlight = (highlight + 1) % len;
                lcd_setfont(FONT_UI);
                break;
            case ACT_CANCEL:
                return false;
        }
        // adjust top visible if needed
        if(highlight < top_visible)
            top_visible = highlight;
        else if(highlight >= top_visible + nr_lines)
            top_visible = highlight - nr_lines + 1;

        lcd_clear_display();

        for(int i = top_visible; i < len && i < top_visible + nr_lines; i++)
        {
            if(i == highlight)
            {
                lcd_set_foreground(LCD_BLACK);
                lcd_set_background(LCD_RGBPACK(255, 255, 0));
            }
            else
            {
                lcd_set_foreground(LCD_WHITE);
                lcd_set_background(LCD_BLACK);
            }
            lcd_putsf(0, i - top_visible, "%s", debug_screens[i].name);
        }
        lcd_set_foreground(LCD_WHITE);
        lcd_set_background(LCD_BLACK);

        lcd_update();
        yield();
    }
    return false;
}

bool dbg_ports(void)
{
    return false;
}
