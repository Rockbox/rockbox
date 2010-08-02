/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Thomas Martitz
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


#include <jni.h>
#include <stdbool.h>
#include "config.h"
#include "kernel.h"
#include "system.h"
#include "touchscreen.h"

static long last_touch;
static int last_y, last_x;

static enum {
    STATE_UNKNOWN,
    STATE_UP,
    STATE_DOWN,
} last_state = STATE_UNKNOWN;


/*
 * this writes in an interrupt-like fashion the last pixel coordinates
 * that the user pressed on the screen */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxFramebuffer_pixelHandler(JNIEnv*env, jobject this,
                                                        int x, int y)
{
    (void)env;
    (void)this;
    last_x = x;
    last_y = y;
    last_touch = current_tick;
}

/*
 * this notifies us in an interrupt-like fashion whether the user just
 * began or stopped the touch action */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxFramebuffer_touchHandler(JNIEnv*env, jobject this,
                                                        int down)
{
    (void)env;
    (void)this;
    if (down)
        last_state = STATE_DOWN;
    else
        last_state = STATE_UP;
}

void button_init_device(void)
{
    last_touch = current_tick;
}

int button_read_device(int *data)
{
    /* get grid button/coordinates based on the current touchscreen mode */
    int btn = touchscreen_to_pixels(last_x, last_y, data);
    if (last_state == STATE_DOWN)
    {
        return btn;
    }
    else
    {
        *data = last_x = last_y = 0;
        return 0;
    }
}
