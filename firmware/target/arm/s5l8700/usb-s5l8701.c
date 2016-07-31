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
#include "power.h"
#endif

#include "s5l8700.h"
#include "usb-designware.h"


const struct usb_dw_config usb_dw_config =
{
    /* PHY must run at 60MHz, otherwise there are spurious -EPROTO
       errors, probably related with GHWCFG4_MIN_AHB_FREQ. */
    .phytype = DWC_PHYTYPE_UTMI_8,

    /*
     * Total available FIFO memory: 0x500 words.
     */
    .rx_fifosz   = 0x220,
    /* nptx_fifosz seems limited to 0x200 due to some internal counter
       misbehaviour (TBC). */
    .nptx_fifosz = 0x200,
    /* ptx_fifosz is not writeable (fixed to 0x300), anyway it seems
       internally limited to a small number, aroung 0x10..0x20 (TBC),
       we use_ptxfifo_as_plain_buffer to deal with this issue. */
    .ptx_fifosz  = 0x80,
    .use_ptxfifo_as_plain_buffer = true,

#ifdef USB_DW_ARCH_SLAVE
    .disable_double_buffering = false,
#else
    .ahb_burst_len = HBSTLEN_INCR4,
#endif
};

void usb_dw_target_enable_clocks()
{
    PWRCON &= ~0x4000;
    PWRCONEXT &= ~0x800;

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

    PWRCON |= 0x4000;
    PWRCONEXT |= 0x800;
}

void usb_dw_target_enable_irq()
{
    INTMSK |= INTMSK_USB_OTG;
}

void usb_dw_target_disable_irq()
{
    INTMSK &= ~INTMSK_USB_OTG;
}

void usb_dw_target_clear_irq()
{
    SRCPND |= INTMSK_USB_OTG;
}

/* RB API fuctions */
void usb_enable(bool on)
{
#ifdef HAVE_USBSTACK
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

int usb_detect(void)
{
#ifdef HAVE_USBSTACK
    if (power_input_status() & POWER_INPUT_USB)
        return USB_INSERTED;
#endif
    return USB_EXTRACTED;
}

void usb_init_device(void)
{
    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    usb_dw_target_disable_irq();
    usb_dw_target_enable_clocks();

    usb_drv_exit();
}
