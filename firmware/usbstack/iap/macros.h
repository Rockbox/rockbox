/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
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
#include <usb_drv.h>

#include "debug.h"

#if DEBUG_ENABLE_INFO == 1
#define LOG(fmt, ...) logf("%lu %s:%d " fmt, iap_debug_timestamp(), __func__, __LINE__ __VA_OPT__(, __VA_ARGS__));
#else
#define LOG(...)
#endif

#if DEBUG_ENABLE_ERROR == 1
#define ERROR(fmt, ...) logf("%lu %s:%d " fmt, iap_debug_timestamp(), __func__, __LINE__ __VA_OPT__(, __VA_ARGS__));
#else
#define ERROR(...)
#endif

#define check_act(cond, act, ...)                              \
    if(!(cond)) {                                              \
        ERROR("assertion failed" __VA_OPT__(":" __VA_ARGS__)); \
        act;                                                   \
    }

#define AS_PACKET_SIZE 192
#define AS_EP_IN       usb_iap_ep_allocs[0].ep
#define HID_EP_IN      usb_iap_ep_allocs[1].ep
