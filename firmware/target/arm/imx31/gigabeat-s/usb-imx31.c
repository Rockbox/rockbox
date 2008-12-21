/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "ata.h"
#include "usb.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "usb-target.h"
#include "clkctl-imx31.h"
#include "power-imx31.h"
#include "mc13783.h"

static int usb_status = USB_EXTRACTED;

static void enable_transceiver(bool enable)
{
    if (enable)
    {
        if (GPIO1_DR & (1 << 30))
        {
            imx31_regclr32(&GPIO3_DR, (1 << 16)); /* Reset ISP1504 */
            sleep(HZ/100);
            imx31_regset32(&GPIO3_DR, (1 << 16));
            sleep(HZ/10);
            imx31_regclr32(&GPIO1_DR, (1 << 30)); /* Select ISP1504 */
        }
    }
    else
    {
        imx31_regset32(&GPIO1_DR, (1 << 30)); /* Deselect ISP1504 */
    }
}

void usb_connect_event(void)
{
    uint32_t status = mc13783_read(MC13783_INTERRUPT_SENSE0);
    usb_status = (status & MC13783_USB4V4S) ?
        USB_INSERTED : USB_EXTRACTED;
    /* Notify power that USB charging is potentially available */
    charger_usb_detect_event(usb_status);
}

int usb_detect(void)
{
    return usb_status;
}

/* Read the immediate state of the cable from the PMIC */
bool usb_plugged(void)
{
    return mc13783_read(MC13783_INTERRUPT_SENSE0) & MC13783_USB4V4S;
}

void usb_init_device(void)
{
    imx31_clkctl_module_clock_gating(CG_USBOTG, CGM_ON_ALL);

    enable_transceiver(true);

    /* Module will be turned off later after firmware init */
    usb_drv_startup();

    /* Initially poll */
    usb_connect_event();

    /* Enable PMIC event */
    mc13783_enable_event(MC13783_USB_EVENT);
}

void usb_enable(bool on)
{
    if (on)
    {
        imx31_clkctl_module_clock_gating(CG_USBOTG, CGM_ON_ALL);
        enable_transceiver(true);
        usb_core_init();
    }
    else
    {
        /* Module clock should be on since this could be called first */
        imx31_clkctl_module_clock_gating(CG_USBOTG, CGM_ON_ALL);
        enable_transceiver(true);
        usb_core_exit();
        enable_transceiver(false);
        imx31_clkctl_module_clock_gating(CG_USBOTG, CGM_OFF);
    }
}
