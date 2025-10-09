/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
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

#include <math.h>
#include <stdlib.h>         /* EXIT_SUCCESS */
#include <stdio.h>
#include "config.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "system.h"
#include "button-ctru.h"
#include "buttonmap.h"
#include "debug.h"
#include "powermgmt.h"
#include "storage.h"
#include "settings.h"
#include "sound.h"
#include "misc.h"

#include "touchscreen.h"

#include <3ds/types.h>
#include <3ds/services/apt.h>
#include <3ds/services/hid.h>
#include <3ds/services/mcuhwc.h>
#include <3ds/services/dsp.h>

static u8 old_slider_level = 0;
static int last_y, last_x;

static enum {
    STATE_UNKNOWN = -1,
    STATE_UP = 0,
    STATE_DOWN = 1,
} last_touch_state = STATE_UNKNOWN;

static double map_values(double n, double source_start, double source_end, double dest_start, double dest_end, int decimal_precision ) {
    double delta_start = source_end - source_start;
    double delta_end = dest_end - dest_start;
    if(delta_start == 0.0 || delta_end == 0.0) {
        return 1.0;
    }
    double scale  = delta_end / delta_start;
    double neg_start   = -1.0 * source_start;
    double offset = (neg_start * scale) + dest_start;
    double final_number = (n * scale) + offset;
    int calc_scale = (int) pow(10.0, decimal_precision);
    return (double) round(final_number * calc_scale) / calc_scale;
}

void update_sound_slider_level(void)
{
      /* update global volume based on sound slider level */
      u8 level;
      MCUHWC_GetSoundSliderLevel(&level);
      
      if (level != old_slider_level) {
          int volume = (int) map_values((double) level,
                                        0.0,             /* min slider voslume */
                                        (double) 0x3f,   /* max slider value */
                                        (double) sound_min(SOUND_VOLUME),
                                        (double) sound_max(SOUND_VOLUME),
                                        0);
         global_status.volume = volume;
         setvol();
         old_slider_level = level;
      }
}

int button_read_device(int* data)
{
    int key = BUTTON_NONE;

    /* TODO: implement Home Menu button support */
    /* if (!aptMainLoop()) {
        return true;
    } */

    hidScanInput();
    u32 kDown = hidKeysDown();
    
    if (kDown & KEY_SELECT) {
        touchscreen_set_mode(touchscreen_get_mode() == TOUCHSCREEN_POINT ? TOUCHSCREEN_BUTTON : TOUCHSCREEN_POINT);
        printf("Touchscreen mode: %s\n", touchscreen_get_mode() == TOUCHSCREEN_POINT ? "TOUCHSCREEN_POINT" : "TOUCHSCREEN_BUTTON");
    }
    
    u32 kHeld = hidKeysHeld();

    /* rockbox will handle button repeats */
    kDown |= kHeld;

    /* Check for all the keys */
    if (kDown & KEY_A) {
        key |= BUTTON_SELECT;
    }
    if (kDown & KEY_B) {
        key |= BUTTON_BACK;
    }
    if (kDown & KEY_X) {
        key |= BUTTON_MENU;
    }
    if (kDown & KEY_Y) {
        key |= BUTTON_USER;
    }
    if (kDown & KEY_START) {
        key |= BUTTON_POWER;
    }
    if (kDown & KEY_DRIGHT) {
        key |= BUTTON_RIGHT;
    }
    if (kDown & KEY_DLEFT) {
        key |= BUTTON_LEFT;
    }
    if (kDown & KEY_DUP) {
        key |= BUTTON_UP;
    }
    if (kDown & KEY_DDOWN) {
        key |= BUTTON_DOWN;
    }
    if (kDown & KEY_START) {
        key |= BUTTON_POWER;
    }

    touchPosition touch;
    hidTouchRead(&touch);

    /* Generate UP and DOWN events */
    if (kDown & KEY_TOUCH) {
        last_touch_state = STATE_DOWN;
    }
    else {
        last_touch_state = STATE_UP;
    }

    last_x = touch.px;
    last_y = touch.py;

    int tkey = touchscreen_to_pixels(last_x, last_y, data);
    if (last_touch_state == STATE_DOWN) {
        key |= tkey;
    }

    update_sound_slider_level();

    return key;
}

void button_init_device(void)
{
    hidInit();
}

#ifndef HAS_BUTTON_HOLD
void touchscreen_enable_device(bool en)
{
    (void)en;
}
#endif

bool headphones_inserted(void)
{
    bool is_inserted;
    DSP_GetHeadphoneStatus(&is_inserted);
    return is_inserted;
}
