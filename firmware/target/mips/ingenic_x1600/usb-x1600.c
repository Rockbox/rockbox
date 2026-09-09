/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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
#include "usb-x1600.h"
#include "panic.h"
#include "irq-x1600.h"
#include "gpio-x1600.h"
#include "x1600/cpm.h"

/* USB-Designware driver API */

const struct usb_dw_config usb_dw_config = {
    .phytype = DWC_PHYTYPE_UTMI_16,

    /* GHWCFG2/3 report 9 endpoints and 3576 words of FIFO, same as the X1000,
     * so the X1000's split applies unchanged */
    .rx_fifosz   = 816, /* shared RxFIFO */
    .nptx_fifosz = 32,  /* only used for EP0 IN */
    .ptx_fifosz  = 384, /* room for 7 IN EPs */

    /* USB uses slave mode; DMA mode has not been validated on this target. */
    .disable_double_buffering = false,
};

/* Initialise the PHY with the register values observed during BootROM USB
 * operation. X1600 has no USB clock-divider register. */
void usb_dw_target_enable_clocks(void)
{
    jz_writef(CPM_CLKGR, OTG(0));

    /* PHY soft reset -- a level, not a pulse (PM 11.2.2.9) */
    jz_writef(CPM_SRBC, OTG_SR(1));
    udelay(10);
    jz_writef(CPM_SRBC, OTG_SR(0));

    jz_writef(CPM_OPCR, GATE_USBPHY_CLK(0));

    /* Exit suspend */
    jz_writef(CPM_OPCR, SPENDN0(1));
    udelay(45);

    /* Restore the complete BootROM PHY values, including reserved bits. */
    REG_CPM_USBPCR  = 0x00000000;
    REG_CPM_USBPCR1 = 0x80000000;

    /* Power on reset */
    jz_writef(CPM_USBPCR, POR(1));
    mdelay(1);
    jz_writef(CPM_USBPCR, POR(0));
    mdelay(1);
}

void usb_dw_target_disable_clocks(void)
{
    jz_writef(CPM_OPCR, SPENDN0(0));
    udelay(5);
    jz_writef(CPM_OPCR, GATE_USBPHY_CLK(1));
    jz_writef(CPM_CLKGR, OTG(1));
}

void usb_dw_target_enable_irq(void)
{
    system_enable_irq(IRQ_USB_OTG);
}

void usb_dw_target_disable_irq(void)
{
    system_disable_irq(IRQ_USB_OTG);
}

void usb_dw_target_clear_irq(void)
{
}

/* Rockbox API */

static volatile int usb_status = USB_EXTRACTED;

void x1600_usb_update_detect(bool present)
{
    int status = present ? USB_INSERTED : USB_EXTRACTED;
    if(status != usb_status) {
        usb_status = status;
        usb_status_event(status);
    }
}

int usb_detect(void)
{
    return usb_status;
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_init_device(void)
{
    /* Drive VBUS is only needed in USB host mode. */
    gpio_set_function(GPIO_USB_DRVVBUS, GPIOF_OUTPUT(0));

    usb_dw_target_disable_irq();
    usb_dw_target_enable_clocks();

    usb_drv_exit();
}
