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
#include <stdint.h>

#include "libiap/datetime.h"
#include "libiap/platform.h"
#include "powermgmt.h"
#include "time.h"

#include "buffer.h"

/* usb_iap.c */
struct IAPContext* _iap_acquire_ctx(bool lock);
void               _iap_release_ctx(void);

struct Platform {
    struct IAPAllocResult            malloc_results[4]; /* allow up to 4 mallocs */
    struct IAPPlatformPendingControl pending_control;
    bool                             control_pending;

    int aa_slot;
};

/* helper functions */
uint8_t _iap_convert_play_status(int rb_audio_status);
uint8_t _iap_convert_volume(int rb_volume);
uint8_t _iap_convert_shuffle_state(bool rb_state);
uint8_t _iap_convert_repeat_state(int rb_state);
uint8_t _iap_convert_battery_level(int rb_battery_level);
uint8_t _iap_convert_charge_status(enum charge_state_type rb_charge_state);
void    _iap_convert_datetime(struct tm* rb_time, struct IAPDateTime* time);
