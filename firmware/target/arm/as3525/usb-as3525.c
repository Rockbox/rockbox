/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "usb.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif
#include "power.h"
#include "as3525.h"
#include "usb_drv.h"
#if CONFIG_USBOTG == USBOTG_DESIGNWARE
#include "usb-designware.h"

const struct usb_dw_config usb_dw_config =
{
    .phytype = DWC_PHYTYPE_UTMI_16,

    /* Available FIFO memory: 0x535 words */
    .rx_fifosz   = 0x215,
    .nptx_fifosz = 0x20,   /* 1 dedicated FIFO for IN0 */
    .ptx_fifosz  = 0x100,  /* 3 dedicated FIFOs for IN1,IN3,IN5 */

#ifdef USB_DW_ARCH_SLAVE
    .disable_double_buffering = false,
#else
    .ahb_burst_len = HBSTLEN_INCR8,
    .ahb_threshold = 8,
#endif
};
#endif /* USBOTG_DESIGNWARE */

static int usb_status = USB_EXTRACTED;

void usb_enable(bool on)
{
#if defined(HAVE_USBSTACK)
    if (on){
        cpu_boost(1);
        usb_core_init();
    } else {
        usb_core_exit();
        cpu_boost(0);
    }
#else
    (void)on;
#endif
}

void usb_insert_int(void)
{
    usb_status = USB_INSERTED;
#ifdef USB_STATUS_BY_EVENT
    usb_status_event(USB_INSERTED);
#endif
}

void usb_remove_int(void)
{
    usb_status = USB_EXTRACTED;
#ifdef USB_STATUS_BY_EVENT
    usb_status_event(USB_EXTRACTED);
#endif
}

int usb_detect(void)
{
    return usb_status;
}

void usb_init_device(void)
{
#if CONFIG_USBOTG == USBOTG_DESIGNWARE
    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    usb_dw_target_disable_irq();
    usb_dw_target_enable_clocks();

    usb_drv_exit();
}

void usb_dw_target_enable_clocks()
{
    bitset32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);
    CCU_USB = (CCU_USB & ~(3<<24)) | (1 << 24); /* ?? */
    /* PHY clock */
    CGU_USB = 1<<5  /* enable */
        | 0 << 2
        | 0; /* source = ? (24MHz crystal?) */

    /* Do something that is probably CCU related but undocumented*/
    CCU_USB |= 0x1000;
    CCU_USB &= ~0x300000;

    udelay(400);
}

void usb_dw_target_disable_clocks()
{
    CGU_USB = 0;
    bitclr32(&CGU_PERI, CGU_USB_CLOCK_ENABLE);

    /* reset USB_PHY to prevent power consumption */
    CCU_SRC = CCU_SRC_USB_PHY_EN;
    CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    CCU_SRL = 0;
}

void usb_dw_target_enable_irq()
{
    VIC_INT_ENABLE = INTERRUPT_USB;
}

void usb_dw_target_disable_irq()
{
    VIC_INT_EN_CLEAR = INTERRUPT_USB;
}

void usb_dw_target_clear_irq()
{
#endif /* USBOTG_DESIGNWARE */
}
