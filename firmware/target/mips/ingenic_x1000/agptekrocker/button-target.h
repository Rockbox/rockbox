/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Marcin Bukat
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

#ifndef __BUTTON_TARGET_H__
#define __BUTTON_TARGET_H__

#include <stdbool.h>

/* Same button set as the hosted Agptek Rocker port, so the
 * AGPTEK_ROCKER_PAD keymap and plugin keymaps can be shared. */
#define BUTTON_LEFT         0x00000001
#define BUTTON_RIGHT        0x00000002
#define BUTTON_UP           0x00000004
#define BUTTON_DOWN         0x00000008
#define BUTTON_SELECT       0x00000010
#define BUTTON_VOLDOWN      0x00000020
#define BUTTON_VOLUP        0x00000040
#define BUTTON_POWER        0x00000080

#define BUTTON_MAIN         0x000000ff

#define POWEROFF_BUTTON     BUTTON_POWER
#define POWEROFF_COUNT      25

#endif /* __BUTTON_TARGET_H__ */
