/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#ifndef __USB_RK27XX_H__
#define __USB_RK27XX_H__

#include <stdint.h>
#include "cpu.h"  /* USB_NUM_ENDPOINTS */

/* rk27xx UDC endpoint allocation: which endpoints may be allocated depends
 * on what else is - interrupt and bulk endpoints never share a group of
 * three - so the driver tracks the allocation itself (usb_drv.h, option 2;
 * usb-drv-rk27xx.c). Include before usb_drv.h. */
struct usb_drv_ep_alloc_ctx_rk27xx
{
    int8_t type[USB_NUM_ENDPOINTS][2];
    int max_packet_size[USB_NUM_ENDPOINTS][2];
};
#define usb_drv_ep_alloc_ctx usb_drv_ep_alloc_ctx_rk27xx

#endif /* __USB_RK27XX_H__ */
