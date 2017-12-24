/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* Set of "extra" keys that SDL will map to the number keys on the
 * keypad. */

#ifndef _SDL_KEYMAPS_EXTRA_H
#define _SDL_KEYMAPS_EXTRA_H

#if (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define BTN_EXTRA1 BUTTON_SELECT
#define BTN_EXTRA2 BUTTON_BACK
#define BTN_EXTRA3 BUTTON_VOL_DOWN
#define BTN_EXTRA4 BUTTON_PREV
#define BTN_EXTRA5 BUTTON_PLAY
#define BTN_EXTRA6 BUTTON_NEXT
#define BTN_EXTRA7 BUTTON_POWER
#endif

/* _SDL_KEYMAPS_H */
#endif
