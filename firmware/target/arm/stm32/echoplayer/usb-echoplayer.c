/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#include "usb.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "usb-designware.h"
#include "gpio-stm32h7.h"
#include "nvic-arm.h"
#include "power-echoplayer.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/pwr.h"
#include "regs/stm32h743/crs.h"
#include "regs/stm32h743/syscfg.h"
#include "regs/stm32h743/exti.h"

/*
 * Needed because USB core calls usb_enable(false) on boot,
 * so we need to keep track if ULPI was enabled or not.
 */
static bool echoplayer_ulpi_enabled = false;

static void echoplayer_enable_ulpi_phy(void)
{
    if (!echoplayer_ulpi_enabled)
    {
        /* Enable MCO1 output for PHY reference clock */
        reg_writef(RCC_CFGR, MCO1_V(HSE), MCO1PRE(1));

        /* Enable power supplies */
        echoplayer_enable_1v8_regulator(true);
        gpio_set_level(GPIO_USBPHY_POWER_EN, 0);

        /* Wait for stable clocks/power */
        sleep(1);

        /* Release PHY from reset */
        gpio_set_level(GPIO_USBPHY_RESET, 1);

        echoplayer_ulpi_enabled = true;
    }
}

static void echoplayer_disable_ulpi_phy(void)
{
    if (echoplayer_ulpi_enabled)
    {
        /* Put PHY into reset */
        gpio_set_level(GPIO_USBPHY_RESET, 0);

        /* Remove power */
        gpio_set_level(GPIO_USBPHY_POWER_EN, 1);
        echoplayer_enable_1v8_regulator(false);

        /* Disable PHY reference clock */
        reg_writef(RCC_CFGR, MCO1PRE(0));

        echoplayer_ulpi_enabled = false;
    }
}

static void echoplayer_usb_enable(void)
{
    if ((STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_HS) ||
        (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_FS))
    {
        echoplayer_enable_ulpi_phy();
    }

    usb_core_init();
}

static void echoplayer_usb_disable(void)
{
    usb_core_exit();

    if ((STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_HS) ||
        (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_FS))
    {
        echoplayer_disable_ulpi_phy();
    }
}

void usb_init_device(void)
{
    if (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_INT_FS)
    {
        /* Required if internal PHY is used */
        reg_writef(PWR_CR3, USB33DEN(1));
    }

    /*
     * This is the only place that needs a GPIO interrupt so
     * just setup EXTI directly.
     */
    const int gpio_port  = GPION_PORT(GPIO_USB_VBUS);
    const int gpio_pin   = GPION_PIN(GPIO_USB_VBUS);
    const int syscfg_idx = gpio_pin / 4;
    const int syscfg_pos = gpio_pin % 4;

    /* Set GPIO port in syscfg's exti config registers */
    reg_writef(RCC_APB4ENR, SYSCFGEN(1));
    reg_var(SYSCFG_EXTICR(syscfg_idx)) = gpio_port << syscfg_pos;
    reg_writef(RCC_APB4ENR, SYSCFGEN(0));

    /* Enable rising & falling edge trigger for pin */
    reg_var(EXTI_RTSR(0)) |= 1u << gpio_pin;
    reg_var(EXTI_FTSR(0)) |= 1u << gpio_pin;
    reg_var(EXTI_CPUIMR(0)) |= 1u << gpio_pin;

    nvic_enable_irq(NVIC_IRQN_EXTI9_5);
}

void usb_enable(bool on)
{
    if (on)
        echoplayer_usb_enable();
    else
        echoplayer_usb_disable();
}

int usb_detect(void)
{
    if (gpio_get_level(GPIO_USB_VBUS))
        return USB_INSERTED;

    return USB_EXTRACTED;
}

void exti9_5_irq_handler(void)
{
    reg_var(EXTI_CPUPR(0)) = 1u << GPION_PIN(GPIO_USB_VBUS);

    usb_status_event(usb_detect());
}
