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
#include "lcd.h"
#include "backlight-target.h"
#include "button.h"

#define default_interrupt(name) \
    extern __attribute__((weak, alias("UIRQ"))) void name(void)

static void UIRQ (void) __attribute__((interrupt ("IRQ")));
void irq_handler(void) __attribute__((interrupt("IRQ")));
void fiq_handler(void) __attribute__((interrupt("FIQ")));

default_interrupt(INT_USB_CTRL);
default_interrupt(INT_TIMER0);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_TIMER3);
default_interrupt(INT_LCDIF_DMA);
default_interrupt(INT_LCDIF_ERROR);
default_interrupt(INT_SSP1_DMA);
default_interrupt(INT_SSP1_ERROR);
default_interrupt(INT_SSP2_DMA);
default_interrupt(INT_SSP2_ERROR);
default_interrupt(INT_I2C_DMA);
default_interrupt(INT_I2C_ERROR);
default_interrupt(INT_GPIO0);
default_interrupt(INT_GPIO1);
default_interrupt(INT_GPIO2);
default_interrupt(INT_VDD5V);
default_interrupt(INT_LRADC_CH0);
default_interrupt(INT_LRADC_CH1);
default_interrupt(INT_LRADC_CH2);
default_interrupt(INT_LRADC_CH3);
default_interrupt(INT_LRADC_CH4);
default_interrupt(INT_LRADC_CH5);
default_interrupt(INT_LRADC_CH6);
default_interrupt(INT_LRADC_CH7);
default_interrupt(INT_DAC_DMA);
default_interrupt(INT_DAC_ERROR);
default_interrupt(INT_ADC_DMA);
default_interrupt(INT_ADC_ERROR);
default_interrupt(INT_DCP);

typedef void (*isr_t)(void);

static isr_t isr_table[INT_SRC_NR_SOURCES] =
{
    [INT_SRC_USB_CTRL] = INT_USB_CTRL,
    [INT_SRC_TIMER(0)] = INT_TIMER0,
    [INT_SRC_TIMER(1)] = INT_TIMER1,
    [INT_SRC_TIMER(2)] = INT_TIMER2,
    [INT_SRC_TIMER(3)] = INT_TIMER3,
    [INT_SRC_LCDIF_DMA] = INT_LCDIF_DMA,
    [INT_SRC_LCDIF_ERROR] = INT_LCDIF_ERROR,
    [INT_SRC_SSP1_DMA] = INT_SSP1_DMA,
    [INT_SRC_SSP1_ERROR] = INT_SSP1_ERROR,
    [INT_SRC_SSP2_DMA] = INT_SSP2_DMA,
    [INT_SRC_SSP2_ERROR] = INT_SSP2_ERROR,
    [INT_SRC_I2C_DMA] = INT_I2C_DMA,
    [INT_SRC_I2C_ERROR] = INT_I2C_ERROR,
    [INT_SRC_GPIO0] = INT_GPIO0,
    [INT_SRC_GPIO1] = INT_GPIO1,
    [INT_SRC_GPIO2] = INT_GPIO2,
    [INT_SRC_VDD5V] = INT_VDD5V,
    [INT_SRC_LRADC_CHx(0)] = INT_LRADC_CH0,
    [INT_SRC_LRADC_CHx(1)] = INT_LRADC_CH1,
    [INT_SRC_LRADC_CHx(2)] = INT_LRADC_CH2,
    [INT_SRC_LRADC_CHx(3)] = INT_LRADC_CH3,
    [INT_SRC_LRADC_CHx(4)] = INT_LRADC_CH4,
    [INT_SRC_LRADC_CHx(5)] = INT_LRADC_CH5,
    [INT_SRC_LRADC_CHx(6)] = INT_LRADC_CH6,
    [INT_SRC_LRADC_CHx(7)] = INT_LRADC_CH7,
    [INT_SRC_DAC_DMA] = INT_DAC_DMA,
    [INT_SRC_DAC_ERROR] = INT_DAC_ERROR,
    [INT_SRC_ADC_DMA] = INT_ADC_DMA,
    [INT_SRC_ADC_ERROR] = INT_ADC_ERROR,
    [INT_SRC_DCP] = INT_DCP,
};

static void UIRQ(void)
{
    panicf("Unhandled IRQ %02X",
        (unsigned int)(HW_ICOLL_VECTOR - (uint32_t)isr_table) / 4);
}

void irq_handler(void)
{
    HW_ICOLL_VECTOR = HW_ICOLL_VECTOR; /* notify icoll that we entered ISR */
    (*(isr_t *)HW_ICOLL_VECTOR)();
    /* acknowledge completion of IRQ (all use the same priority 0) */
    HW_ICOLL_LEVELACK = HW_ICOLL_LEVELACK__LEVEL0;
}

void fiq_handler(void)
{
}

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
    while(button_read_device());
    /* then wait until next button press */
    while(!button_read_device());
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

void imx233_enable_interrupt(int src, bool enable)
{
    if(enable)
        __REG_SET(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__ENABLE;
    else
        __REG_CLR(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__ENABLE;
}

void imx233_softirq(int src, bool enable)
{
    if(enable)
        __REG_SET(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__SOFTIRQ;
    else
        __REG_CLR(HW_ICOLL_INTERRUPT(src)) = HW_ICOLL_INTERRUPT__SOFTIRQ;
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
    imx233_reset_block(&HW_ICOLL_CTRL);
    /* disable all interrupts */
    for(int i = 0; i < INT_SRC_NR_SOURCES; i++)
    {
        /* priority = 0, disable, disable fiq */
        HW_ICOLL_INTERRUPT(i) = 0;
    }
    /* setup vbase as isr_table */
    HW_ICOLL_VBASE = (uint32_t)&isr_table;
    /* enable final irq bit */
    __REG_SET(HW_ICOLL_CTRL) = HW_ICOLL_CTRL__IRQ_FINAL_ENABLE;

    imx233_pinctrl_init();
    imx233_timrot_init();
    imx233_dma_init();
    imx233_ssp_init();
    imx233_dcp_init();
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
    switch(frequency)
    {
        #if 0
        case IMX233_CPUFREQ_454_MHz:
            /* clk_h@clk_p/3 */
            imx233_set_clock_divisor(CLK_AHB, 3);
            /* clk_p@ref_cpu/1*18/19 */
            imx233_set_fractional_divisor(CLK_CPU, 19);
            imx233_set_clock_divisor(CLK_CPU, 1);
            /* ref_cpu@480 MHz
             * clk_p@454.74 MHz
             * clk_h@151.58 MHz */
            break;
        #endif
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
