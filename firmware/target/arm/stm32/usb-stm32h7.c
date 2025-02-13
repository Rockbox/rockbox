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

const struct usb_dw_config usb_dw_config = {
    .phytype = DWC_PHYTYPE_ULPI_SDR,
    /*
     * Available FIFO memory: 1024 words (TODO: check this)
     * Number of endpoints: 9
     * Max packet size: 512 bytes
     */
    .rx_fifosz   = 224, /* shared RxFIFO */
    .nptx_fifosz = 32,  /* only used for EP0 IN */
    .ptx_fifosz  = 192, /* room for 4 IN EPs */

#ifndef USB_DW_ARCH_SLAVE
    .ahb_burst_len = HBSTLEN_INCR8,
    /* Disable Rx FIFO thresholding. It appears to cause problems,
     * apparently a known issue -- Synopsys recommends disabling it
     * because it can cause issues during certain error conditions.
     */
    .ahb_threshold = 0,
#else
    .disable_double_buffering = false,
#endif
};

void usb_enable(bool on)
{
    if (on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_init_device(void)
{
    /* TODO - this is highly target specific */
}

int usb_detect(void)
{
    return USB_EXTRACTED;
}

void usb_dw_target_enable_clocks(void)
{
}

void usb_dw_target_disable_clocks(void)
{
}

void usb_dw_target_enable_irq(void)
{
}

void usb_dw_target_disable_irq(void)
{
}

void usb_dw_target_clear_irq(void)
{
}
