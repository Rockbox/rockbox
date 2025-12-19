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
#pragma once
#include <usb_drv.h>

#include "debug.h"

#if DEBUG_ENABLE_INFO == 1
#define IAP_LOGF(fmt, ...) logf("%lu " fmt, iap_debug_timestamp() __VA_OPT__(, __VA_ARGS__))
#endif

#if DEBUG_ENABLE_ERROR == 1
#define IAP_ERRORF(fmt, ...) logf("%lu " fmt, iap_debug_timestamp() __VA_OPT__(, __VA_ARGS__))
#endif

#define IAP_ARTWORK_WIDTH  128
#define IAP_ARTWORK_HEIGHT 128
#define IAP_COLOR_ARTWORK  iap_true
