/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef __HWEMUL__
#define __HWEMUL__

#include <libusb.h>
#include "hwemul_protocol.h"
#include "hwemul_soc.h"

/**
 *
 * Low-Level interface
 *
 */

struct hwemul_device_t
{
    libusb_device_handle *handle;
    int intf;
    int bulk_in;
    int bulk_out;
    int int_in;
};

/* Requires then ->handle field only. Returns 0 on success */
int hwemul_probe(struct hwemul_device_t *dev);
/* Returns 0 on success */
int hwemul_release(struct hwemul_device_t *dev);

/* Returns number of bytes filled */
int hwemul_get_info(struct hwemul_device_t *dev, uint16_t idx, void *info, size_t sz);
/* Returns number of bytes filled */
int hwemul_get_log(struct hwemul_device_t *dev, void *buf, size_t sz);
/* Returns number of bytes written/read or <0 on error */
int hwemul_rw_mem(struct hwemul_device_t *dev, int read, uint32_t addr, void *buf, size_t sz);
/* Returns <0 on error */
int hwemul_call(struct hwemul_device_t *dev, uint32_t addr);
int hwemul_jump(struct hwemul_device_t *dev, uint32_t addr);
/* Returns <0 on error. The size must be a multiple of 16. */
int hwemul_aes_otp(struct hwemul_device_t *dev, void *buf, size_t sz, uint16_t param);

const char *hwemul_get_product_string(struct usb_resp_info_stmp_t *stmp);
const char *hwemul_get_rev_string(struct usb_resp_info_stmp_t *stmp);

#endif /* __HWEMUL__ */