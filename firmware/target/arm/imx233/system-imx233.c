/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
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

#include "kernel.h"
#include "system.h"
#include "gcc_extensions.h"
#include "system-target.h"
#include "cpu.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"
#include "timrot-imx233.h"
#include "dma-imx233.h"
#include "ssp-imx233.h"
#include "i2c-imx233.h"
#include "dcp-imx233.h"
#include "pwm-imx233.h"
#include "icoll-imx233.h"
#include "lradc-imx233.h"
#include "rtc-imx233.h"
#include "power-imx233.h"
#include "lcd.h"
#include "backlight-target.h"
#include "button.h"
#include "fmradio_i2c.h"

void imx233_chip_reset(void)
{
    HW_CLKCTRL_RESET = HW_CLKCTRL_RESET_CHIP;
}

void system_reboot(void)
{
    _backlight_off();

    disable_irq();

    /* use watchdog to reset */
    imx233_chip_reset();
    while(1);
}

void system_exception_wait(void)
{
    /* make sure lcd and backlight are on */
    lcd_update();
    _backlight_on();
    _backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* wait until button release (if a button is pressed) */
#ifdef HAVE_BUTTON_DATA
    int data;
    while(button_read_device(&data));
    /* then wait until next button press */
    while(!button_read_device(&data));
#else
    while(button_read_device());
    /* then wait until next button press */
    while(!button_read_device());
#endif
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

static void set_page_tables(void)
{
    /* map every memory region to itself */
    map_section(0, 0, 0x1000, CACHE_NONE);

    /* map RAM and enable caching for it */
    map_section(DRAM_ORIG, CACHED_DRAM_ADDR, MEMORYSIZE, CACHE_ALL);
    map_section(DRAM_ORIG, BUFFERED_DRAM_ADDR, MEMORYSIZE, BUFFERED);
}

void memory_init(void)
{
    ttb_init();
    set_page_tables();
    enable_mmu();
}

void system_init(void)
{
    imx233_clkctrl_enable_clock(CLK_PLL, true);
    imx233_rtc_init();
    imx233_icoll_init();
    imx233_pinctrl_init();
    imx233_timrot_init();
    imx233_dma_init();
    imx233_ssp_init();
    imx233_dcp_init();
    imx233_pwm_init();
    imx233_lradc_init();
    imx233_i2c_init();
#if !defined(BOOTLOADER) &&(defined(SANSA_FUZEPLUS) || \
    defined(CREATIVE_ZENXFI3) || defined(CREATIVE_ZENXFI2))
    fmradio_i2c_init();
#endif
    imx233_clkctrl_enable_auto_slow_monitor(AS_CPU_INSTR, true);
    imx233_clkctrl_enable_auto_slow_monitor(AS_CPU_DATA, true);
    imx233_clkctrl_enable_auto_slow_monitor(AS_TRAFFIC, true);
    imx233_clkctrl_enable_auto_slow_monitor(AS_TRAFFIC_JAM, true);
    imx233_clkctrl_enable_auto_slow_monitor(AS_APBXDMA, true);
    imx233_clkctrl_enable_auto_slow_monitor(AS_APBHDMA, true);
    imx233_clkctrl_set_auto_slow_divisor(AS_DIV_8);
    imx233_clkctrl_enable_auto_slow(true);
}

bool imx233_us_elapsed(uint32_t ref, unsigned us_delay)
{
    uint32_t cur = HW_DIGCTL_MICROSECONDS;
    if(ref + us_delay <= ref)
        return !(cur > ref) && !(cur < (ref + us_delay));
    else
        return (cur < ref) || cur >= (ref + us_delay);
}

void imx233_reset_block(volatile uint32_t *block_reg)
{
    /* soft-reset */
    __REG_SET(*block_reg) = __BLOCK_SFTRST;
    /* make sure block is gated off */
    while(!(*block_reg & __BLOCK_CLKGATE));
    /* bring block out of reset */
    __REG_CLR(*block_reg) = __BLOCK_SFTRST;
    while(*block_reg & __BLOCK_SFTRST);
    /* make sure clock is running */
    __REG_CLR(*block_reg) = __BLOCK_CLKGATE;
    while(*block_reg & __BLOCK_CLKGATE);
}

void udelay(unsigned us)
{
    uint32_t ref = HW_DIGCTL_MICROSECONDS;
    while(!imx233_us_elapsed(ref, us));
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    (void) frequency;
    switch(frequency)
    {
        case IMX233_CPUFREQ_454_MHz:
            /* go back to a known state: everything at 24MHz ! */
            imx233_clkctrl_set_bypass_pll(CLK_CPU, true);
            imx233_clkctrl_set_clock_divisor(CLK_HBUS, 1);
            /* set VDDD to 1.550 mV (brownout at 1.450 mV) */
            imx233_power_set_regulator(REGULATOR_VDDD, 1550, 1450);
            /* clk_h@clk_p/2 */
            imx233_clkctrl_set_clock_divisor(CLK_HBUS, 3);
            /* clk_p@ref_cpu/1*18/19 */
            imx233_clkctrl_set_fractional_divisor(CLK_CPU, 19);
            imx233_clkctrl_set_clock_divisor(CLK_CPU, 1);
            imx233_clkctrl_set_bypass_pll(CLK_CPU, false);
            /* ref_cpu@480 MHz
             * ref_emi@480 MHz
             * clk_emi@130.91 MHz
             * clk_p@454.74 MHz
             * clk_h@130.91 MHz */
            break;
        case IMX233_CPUFREQ_261_MHz:
            /* go back to a known state: everything at 24MHz ! */
            imx233_clkctrl_set_bypass_pll(CLK_CPU, true);
            imx233_clkctrl_set_clock_divisor(CLK_HBUS, 1);
            /* set VDDD to 1.275 mV (brownout at 1.175 mV) */
            imx233_power_set_regulator(REGULATOR_VDDD, 1275, 1175);
            /* clk_h@clk_p/2 */
            imx233_clkctrl_set_clock_divisor(CLK_HBUS, 2);
            /* clk_p@ref_cpu/1*18/33 */
            imx233_clkctrl_set_fractional_divisor(CLK_CPU, 33);
            imx233_clkctrl_set_clock_divisor(CLK_CPU, 1);
            imx233_clkctrl_set_bypass_pll(CLK_CPU, false);
            /* ref_cpu@480 MHz
             * ref_emi@480 MHz
             * clk_emi@130.91 MHz
             * clk_p@261.82 MHz
             * clk_h@130.91 MHz */
            break;
        default:
            break;
    }
}
#endif

void imx233_enable_usb_controller(bool enable)
{
    if(enable)
        __REG_CLR(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__USB_CLKGATE;
    else
        __REG_SET(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__USB_CLKGATE;
}

void imx233_enable_usb_phy(bool enable)
{
    if(enable)
    {
        __REG_CLR(HW_USBPHY_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
        __REG_CLR(HW_USBPHY_PWD) = HW_USBPHY_PWD__ALL;
    }
    else
    {
        __REG_SET(HW_USBPHY_PWD) = HW_USBPHY_PWD__ALL;
        __REG_SET(HW_USBPHY_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
    }
}
