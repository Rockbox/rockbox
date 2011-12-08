/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Jonathan Gordon
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
#ifndef __SHORTCUTS_H__
#define __SHORTCUTS_H__
#include <stdbool.h>
#include <stdlib.h>
 
enum shortcut_type {
    SHORTCUT_UNDEFINED = -1,
    SHORTCUT_SETTING = 0,
    SHORTCUT_FILE,
    SHORTCUT_DEBUGITEM,
    SHORTCUT_BROWSER,
    SHORTCUT_PLAYLISTMENU,
    SHORTCUT_SEPARATOR,
    SHORTCUT_SHUTDOWN,
    SHORTCUT_TIME,

    SHORTCUT_TYPE_COUNT
};

void shortcuts_add(enum shortcut_type type, const char* value);
void shortcuts_init(void);
int do_shortcut_menu(void*ignored);

#endif
