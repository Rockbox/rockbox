/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Michael McAllister
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
#ifndef __USB_DAC_HIBY_H__
#define __USB_DAC_HIBY_H__

/**
 * Host-PCM pump for the HiBy USB DAC gadget mode: drains /dev/uac_sa
 * into the PCM mixer. Started/stopped by the USB gadget driver when the
 * uac_sa function is brought up/torn down.
 *
 * usb_dac_start() returns false if the uac_sa device never appears (the
 * pump is not started); the caller should fall back to a safe USB mode.
 */
bool usb_dac_start(void);
void usb_dac_stop(void);

#endif /* __USB_DAC_HIBY_H__ */
