/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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
#ifndef PLUGIN_PICOTTS_MENU_H
#define PLUGIN_PICOTTS_MENU_H

enum {
    PICOTTS_CANCEL_MENU,
    PICOTTS_TTS_SETTING_CHANGE = 0x100,
    PICOTTS_SPEED_SETTING_CHANGE = 0x200,
    PICOTTS_PITCH_SETTING_CHANGE = 0x400,
    PICOTTS_EXIT_PLUGIN = 0x800,
    PICOTTS_USB_CONNECTED = 0x1000
};

int display_menu(void);

#endif
