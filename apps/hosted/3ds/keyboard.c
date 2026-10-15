/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Mauricio G.
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
#include "string-extra.h"
#include "kernel.h"
#include "lang.h"

#include <3ds/applets/swkbd.h>

static SwkbdState swkbd;

int kbd_input(char* text, int buflen, ucschar_t *kbd)
{
    (void)kbd;

    SwkbdButton button = SWKBD_BUTTON_NONE;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, buflen);
    swkbdSetInitialText(&swkbd, text);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT,  str(LANG_KBD_CANCEL), false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, str(LANG_KBD_OK),     true);
    swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN);

    button = swkbdInputText(&swkbd, text, buflen);

    return button == SWKBD_BUTTON_RIGHT ? 0 : -1; /* return 0 on success */
}

int load_kbd(unsigned char* filename)
{
    (void)filename;
    return 1;
}

