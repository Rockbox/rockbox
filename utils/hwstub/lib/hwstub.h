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
#ifndef __HWSTUB__
#define __HWSTUB__

#include <libusb.h>
#include "hwstub_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * Low-Level interface
 *
 */

struct hwstub_device_t
{
    libusb_device_handle *handle;
    int intf;
    int bulk_in;
    int bulk_out;
    int int_in;
};

/* Requires then ->handle field only. Returns 0 on success */
int hwstub_probe(struct hwstub_device_t *dev);
/* Returns 0 on success */
int hwstub_release(struct hwstub_device_t *dev);

/* Returns number of bytes filled */
int hwstub_get_info(struct hwstub_device_t *dev, uint16_t idx, void *info, size_t sz);
/* Returns number of bytes filled */
int hwstub_get_log(struct hwstub_device_t *dev, void *buf, size_t sz);
/* Returns number of bytes written/read or <0 on error */
int hwstub_rw_mem(struct hwstub_device_t *dev, int read, uint32_t addr, void *buf, size_t sz);
/* Returns <0 on error */
int hwstub_call(struct hwstub_device_t *dev, uint32_t addr);
int hwstub_jump(struct hwstub_device_t *dev, uint32_t addr);
/* Returns <0 on error. The size must be a multiple of 16. */
int hwstub_aes_otp(struct hwstub_device_t *dev, void *buf, size_t sz, uint16_t param);

const char *hwstub_get_product_string(struct usb_resp_info_stmp_t *stmp);
const char *hwstub_get_rev_string(struct usb_resp_info_stmp_t *stmp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __HWSTUB__ */