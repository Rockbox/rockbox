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

#include "config.h"
#include <inttypes.h>
#include "s5l8702.h"
#include "usb-designware.h"

const struct usb_dw_config usb_dw_config = 
{
    .phy_16bit = true, \
    .phy_ulpi = false, \
    .use_dma = true, \
    .shared_txfifo = true, \
    .disable_double_buffering = false, \
    .fifosize = 1024, \
    .txfifosize = { 0x200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, \
};

void usb_dw_target_enable_clocks()
{
    PWRCON(0) &= ~0x4;
    PWRCON(1) &= ~0x8;
    DESIGNWARE_PCGCCTL = 0; //usb_dw_config.core->pcgcctl.d32 = 0;
    *((volatile uint32_t*)(PHYBASE + 0x00)) = 0;  /* PHY: Power up */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x1c)) = 1;
    *((volatile uint32_t*)(PHYBASE + 0x44)) = 0xe3f;
    *((volatile uint32_t*)(PHYBASE + 0x08)) = 1;  /* PHY: Assert Software Reset */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x08)) = 0;  /* PHY: Deassert Software Reset */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x18)) = 0x600;
    *((volatile uint32_t*)(PHYBASE + 0x04)) = USB_DW_CLOCK;
    udelay(400);
}

void usb_dw_target_disable_clocks()
{
    *((volatile uint32_t*)(PHYBASE + 0x00)) = 0xf;  /* PHY: Power down */
    udelay(10);
    *((volatile uint32_t*)(PHYBASE + 0x08)) = 7;  /* PHY: Assert Software Reset */
    udelay(10);
    DESIGNWARE_PCGCCTL = 0; //usb_dw_config.core->pcgcctl.d32 = 0;
    PWRCON(0) |= 0x4;
    PWRCON(1) |= 0x8;
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

