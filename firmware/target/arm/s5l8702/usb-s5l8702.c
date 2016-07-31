/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2014 Michael Sparmann
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
#include <inttypes.h>

#include "config.h"
#include "usb.h"
#include "usb_drv.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif

#include "s5l8702.h"
#include "clocking-s5l8702.h"
#include "usb-designware.h"


const struct usb_dw_config usb_dw_config =
{
    .phytype = DWC_PHYTYPE_UTMI_16,

    /* Available FIFO memory: 0x820 words */
    .rx_fifosz   = 0x360,
    .nptx_fifosz = 0x40,   /* 1 dedicated FIFO for IN0 */
    .ptx_fifosz  = 0x180,  /* 3 dedicated FIFOs for IN1,IN3,IN5 */

#ifdef USB_DW_ARCH_SLAVE
    .disable_double_buffering = false,
#else
    .ahb_burst_len = HBSTLEN_INCR8,
    .ahb_threshold = 8,
#endif
};

void usb_dw_target_enable_clocks()
{
    clockgate_enable(CLOCKGATE_USBOTG, true);
    clockgate_enable(CLOCKGATE_USBPHY, true);

    OPHYPWR = 0;  /* PHY: Power up */
    udelay(10);
    OPHYUNK1 = 1;
    OPHYUNK2 = 0xe3f;
    ORSTCON = 1;  /* PHY: Assert Software Reset */
    udelay(10);
    ORSTCON = 0;  /* PHY: Deassert Software Reset */
    udelay(10);
    OPHYUNK3 = 0x600;
    OPHYCLK = USB_DW_CLOCK;
    udelay(400);
}

void usb_dw_target_disable_clocks()
{
    OPHYPWR = 0xf;  /* PHY: Power down */
    udelay(10);
    ORSTCON = 7;  /* PHY: Assert Software Reset */
    udelay(10);

    clockgate_enable(CLOCKGATE_USBOTG, false);
    clockgate_enable(CLOCKGATE_USBPHY, false);
}

void usb_dw_target_enable_irq()
{
    VICINTENABLE(IRQ_USB_FUNC >> 5) = 1 << (IRQ_USB_FUNC & 0x1f);
}

void usb_dw_target_disable_irq()
{
    VICINTENCLEAR(IRQ_USB_FUNC >> 5) = 1 << (IRQ_USB_FUNC & 0x1f);
}

void usb_dw_target_clear_irq()
{
}

/* RB API */
static int usb_status = USB_EXTRACTED;

void usb_enable(bool on)
{
#ifdef HAVE_USBSTACK
    if (on) usb_core_init();
    else usb_core_exit();
#else
    (void)on;
#endif
}

int usb_detect(void)
{
    return usb_status;
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

void usb_init_device(void)
{
    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    usb_dw_target_disable_irq();
    usb_dw_target_enable_clocks();

    usb_drv_exit();
}
