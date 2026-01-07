/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "nvic-arm.h"
#include "regs/stm32h743/pwr.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/crs.h"

#if STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1
# define IRQN_USB NVIC_IRQN_OTG_HS
#else
# define IRQN_USB NVIC_IRQN_OTG_FS
#endif

/* Total size of FIFO RAM */
#define FIFO_RAMSZ  1024

/*
 * Must be equal to N * (max packet size / 4) for IN endpoints.
 * Due to limited FIFO RAM set N=1; N > 1 increases performance.
 * NPTX is only for EP0 so its max packet size is 64 bytes.
 */
#define NPTX_FIFOSZ 16
#define PTX_FIFOSZ  128

#define RX_FIFOSZ \
    (FIFO_RAMSZ - NPTX_FIFOSZ - (PTX_FIFOSZ * (USB_NUM_ENDPOINTS - 1)))

/*
 * See RM0433 -- 57.11.3 FIFO RAM allocation
 * We require enough for 2x 512-byte packets / 1x 1024-byte packet
 */
_Static_assert(RX_FIFOSZ >= 14 + 2*(512/4 + 1) + 2*USB_NUM_ENDPOINTS,
               "RX FIFO size is too small");

const struct usb_dw_config usb_dw_config = {
    .nptx_fifosz = NPTX_FIFOSZ,
    .ptx_fifosz  = PTX_FIFOSZ,
    .rx_fifosz   = RX_FIFOSZ,

#if ((STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_HS) || \
     (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_FS))
    .phytype = DWC_PHYTYPE_ULPI_SDR,
#else
    .phytype = PHSEL,
#endif

#ifndef USB_DW_ARCH_SLAVE
    .ahb_burst_len = HBSTLEN_INCR8,
    .ahb_threshold = 0,
#else
    .disable_double_buffering = false,
#endif
};

_Static_assert(
    (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1) ||
    (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_INT_FS),
    "ULPI PHY not available on OTG_HS2");

_Static_assert(
    STM32H743_USBOTG_CLKSEL_PLL1Q     == BV_RCC_D2CCIP2R_USBSEL_PLL1Q &&
    STM32H743_USBOTG_CLKSEL_PLL3Q     == BV_RCC_D2CCIP2R_USBSEL_PLL3Q &&
    STM32H743_USBOTG_CLKSEL_HSI48_CRS == BV_RCC_D2CCIP2R_USBSEL_HSI48,
    "Mismatch between CLKSEL enum and USBSEL register");

/*
 * The internal PHY seems to need some modifications in usb-designware
 * to make it work but it's not clear what's wrong. At minimum it seems
 * necessary to set PWRDWN & NOVBUSSENS bits on the GCCFG register in
 * addition to setting DSPEED in DCFG, but that is not enough to get it
 * to enumerate. ULPI in FS mode works however.
 */
_Static_assert(STM32H743_USBOTG_PHY != STM32H743_USBOTG_PHY_INT_FS,
               "internal FS PHY not supported at the moment");

void usb_dw_target_enable_clocks(void)
{
    if (STM32H743_USBOTG_CLKSEL == STM32H743_USBOTG_CLKSEL_HSI48_CRS)
    {
        /* Enable HSI48 */
        reg_writef(RCC_CR, HSI48ON(1));
        while (reg_readf(RCC_CR, HSI48RDY) == 0);

        /* Enable CRS */
        reg_writef(RCC_APB1HENR, CRSEN(1));

        /* Select USBx SOF as input to CRS */
        if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1)
            reg_writef(CRS_CFGR, SYNCSRC_V(USB1_SOF));
        else if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB2)
            reg_writef(CRS_CFGR, SYNCSRC_V(USB2_SOF));

        /*
         * Enable CRS, the reset value of its registers contains
         * suitable settings for syncing with the USB FS SOF and
         * we assume the CRS configuration won't be modified.
         */
        reg_writef(CRS_CR, AUTOTRIMEN(1), CEN(1));
    }

    /* Set appropriate input clock */
    reg_writef(RCC_D2CCIP2R, USBSEL(STM32H743_USBOTG_CLKSEL));

    /* Enable peripheral clock */
    if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1)
        reg_writef(RCC_AHB1ENR, USB1OTGHSEN(1));
    else if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB2)
        reg_writef(RCC_AHB1ENR, USB2OTGHSEN(1));

    /* Enable ULPI clock for USB1 if high speed mode is enabled */
    if ((STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_HS) ||
        (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_FS))
    {
        reg_writef(RCC_AHB1ENR, USB1OTGHSULPIEN(1));
    }
}

void usb_dw_target_disable_clocks(void)
{
    /* Disable ULPI clock */
    if ((STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_HS) ||
        (STM32H743_USBOTG_PHY == STM32H743_USBOTG_PHY_ULPI_FS))
    {
        reg_writef(RCC_AHB1ENR, USB1OTGHSULPIEN(0));
    }

    /* Disable peripheral clock */
    if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB1)
        reg_writef(RCC_AHB1ENR, USB1OTGHSEN(0));
    else if (STM32H743_USBOTG_INSTANCE == STM32H743_USBOTG_INSTANCE_USB2)
        reg_writef(RCC_AHB1ENR, USB2OTGHSEN(0));

    if (STM32H743_USBOTG_CLKSEL == STM32H743_USBOTG_CLKSEL_HSI48_CRS)
    {
        /* Disable CRS */
        reg_writef(CRS_CR, CEN(0));
        reg_writef(RCC_APB1HENR, CRSEN(0));

        /* Disable HSI48 */
        reg_writef(RCC_CR, HSI48ON(0));
    }
}

void usb_dw_target_enable_irq(void)
{
    arm_dsb();
    nvic_enable_irq(IRQN_USB);
}

void usb_dw_target_disable_irq(void)
{
    nvic_disable_irq(IRQN_USB);
    arm_dsb();
}

void usb_dw_target_clear_irq(void)
{
}
