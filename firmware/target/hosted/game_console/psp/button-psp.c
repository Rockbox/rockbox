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
#include "button-console.h"
#include "buttonmap.h"
#include "debug.h"
#include "powermgmt.h"
#include "storage.h"
#include "settings.h"
#include "sound.h"
#include "misc.h"

#include <pspctrl.h>
#include <psphprm.h>
#include <psptypes.h>

#include "sys_kernel_hook.h"

static int old_slider_level = 0;

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
    int level = sys_impose_get_param(PSP_IMPOSE_MAIN_VOLUME);
    if (level < 0) {
        /* logf("sys_impose_get_param: error (0x%x)\n", level); */
    }

    if (level != old_slider_level) {
        int volume = (int) map_values((double) level,
                                    (double) 0,     /* min slider value */
                                    (double) 30,    /* max slider value */
                                    (double) sound_min(SOUND_VOLUME),
                                    (double) sound_max(SOUND_VOLUME),
                                    0);
        global_status.volume = volume;
        setvol();
        old_slider_level = level;
    }
}

int button_read_device(void)
{
    int key = BUTTON_NONE;

    /* TODO: implement Home Screen button support */
    /* if (!aptMainLoop()) {
        return true;
    } */

    /* rockbox will handle button repeats */

    SceCtrlData pad;
    int res = sceCtrlPeekBufferPositive(&pad, 1);

    /* Check for all the keys */
    if (pad.Buttons & PSP_CTRL_CROSS) {
        key |= BUTTON_SELECT;
    }
    if (pad.Buttons & PSP_CTRL_CIRCLE) {
        key |= BUTTON_BACK;
    }
    if (pad.Buttons & PSP_CTRL_TRIANGLE) {
        key |= BUTTON_MENU;
    }
    if (pad.Buttons & PSP_CTRL_SQUARE) {
        key |= BUTTON_USER;
    }
    if (pad.Buttons & PSP_CTRL_RIGHT) {
        key |= BUTTON_RIGHT;
    }
    if (pad.Buttons & PSP_CTRL_LEFT) {
        key |= BUTTON_LEFT;
    }
    if (pad.Buttons & PSP_CTRL_UP) {
        key |= BUTTON_UP;
    }
    if (pad.Buttons & PSP_CTRL_DOWN) {
        key |= BUTTON_DOWN;
    }
    if (pad.Buttons & PSP_CTRL_SELECT) {
    }
    if (pad.Buttons & PSP_CTRL_START) {
        key |= BUTTON_POWER;
    }
    if (pad.Buttons & PSP_CTRL_L1) {
        key |= BUTTON_PREV;
    }
    if (pad.Buttons & PSP_CTRL_R1) {
        key |= BUTTON_NEXT;
    }

    update_sound_slider_level();

    return key;
}

void button_init_device(void)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
}

bool headphones_inserted(void)
{
    return sceHprmIsHeadphoneExist() == 1 ? true : false;
}
