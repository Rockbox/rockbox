/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Johannes Schwarz
 * based on Will Robertson code in superdom
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
#ifndef _DISPLAY_TEXT_H
#define _DISPLAY_TEXT_H

#include "plugin.h"

/*
 * basic usage:
 * char *text[] = {"normal", "centering", "red,underline"};
 * struct style_text formation[]={
 *     { 1, TEXT_CENTER },
 *     { 2, C_RED|TEXT_UNDERLINE },
 *     LAST_STYLE_ITEM
 * };
 * if (display_text(ARRAYLEN(text), text, formation, NULL, true))
 *      return PLUGIN_USB_CONNECTED;
 */

enum ecolor { STANDARD, C_YELLOW, C_RED, C_BLUE, C_GREEN , C_ORANGE };
#define TEXT_COLOR_MASK 0x00ff
#define TEXT_CENTER     0x0100
#define TEXT_UNDERLINE  0x0200
#define LAST_STYLE_ITEM { -1, 0 }

struct style_text {
    unsigned short index;
    unsigned short flags;
};

/*
 * display text.
 *  - words : number of words in text.
 *  - text  : array of word to be displayed. use empty string for newline.
 *  - style : (optional) set style of each word. must be sorted by index.
 *  - vp_text  : (optional) viewport to display text.
 *  - wait_key : set true to wait button press after all words is displayed.
 * return true if usb is connected, false otherwise.
 */
bool display_text(unsigned short words, char** text, struct style_text* style,
                  struct viewport* vp_text, bool wait_key);

#endif /*  _DISPLAY_TEXT_H */
