/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 *****************************************************************************/


#include "button.h"
#include "android_keyevents.h"

int key_to_button(int keyboard_key)
{
    switch (keyboard_key)
    {
        default:
            return BUTTON_NONE;
        case KEYCODE_BACK:
            return BUTTON_BACK;
        case KEYCODE_DPAD_UP:
            return BUTTON_DPAD_UP;
        case KEYCODE_DPAD_DOWN:
            return BUTTON_DPAD_DOWN;
        case KEYCODE_DPAD_LEFT:
            return BUTTON_DPAD_LEFT;
        case KEYCODE_DPAD_RIGHT:
            return BUTTON_DPAD_RIGHT;
        case KEYCODE_DPAD_CENTER:
            return BUTTON_DPAD_CENTER;
        case KEYCODE_MENU:
            return BUTTON_MENU;
    }
}

unsigned multimedia_to_button(int keyboard_key)
{
    switch (keyboard_key)
    {
        case KEYCODE_MEDIA_PLAY_PAUSE:
            return BUTTON_MULTIMEDIA_PLAYPAUSE;
        case KEYCODE_MEDIA_STOP:
            return BUTTON_MULTIMEDIA_STOP;
        case KEYCODE_MEDIA_NEXT:
            return BUTTON_MULTIMEDIA_NEXT;
        case KEYCODE_MEDIA_PREVIOUS:
            return BUTTON_MULTIMEDIA_PREV;
        case KEYCODE_MEDIA_REWIND:
            return BUTTON_MULTIMEDIA_REW;
        case KEYCODE_MEDIA_FAST_FORWARD:
            return BUTTON_MULTIMEDIA_FFWD;
        default:
            return 0;
    }
}
