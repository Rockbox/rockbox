/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 by Benjamin Metzler
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

#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

#include <stdbool.h>

enum {
    BOOKMARK_FAIL = -1,
    BOOKMARK_SUCCESS = 0,
    BOOKMARK_USB_CONNECTED = 1
};

int  bookmark_load_menu(void);
bool bookmark_autobookmark(bool prompt_ok);
bool bookmark_create_menu(void);
bool bookmark_mrb_load(void);
bool bookmark_autoload(const char* file);
bool bookmark_load(const char* file, bool autoload);
bool bookmark_exists(void);
bool bookmark_is_bookmarkable_state(void);

#endif /* __BOOKMARK_H__ */

