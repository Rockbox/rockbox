/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef JZTOOL_PRIVATE_H
#define JZTOOL_PRIVATE_H

#include "jztool.h"
#include <libusb.h>

struct jz_context {
    void* user_data;
    jz_log_cb log_cb;
    jz_log_level log_level;
    libusb_context* usb_ctx;
    int usb_ctxref;
};

struct jz_usbdev {
    jz_context* jz;
    libusb_device_handle* handle;
};

int jz_context_ref_libusb(jz_context* jz);
void jz_context_unref_libusb(jz_context* jz);

#endif /* JZTOOL_PRIVATE_H */
