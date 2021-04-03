/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2021 William Wilgus
*
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
#ifndef _BUTTON_HELPER_H_
#define _BUTTON_HELPER_H_
struct available_button
{
    const char* name;
    unsigned long value;
};

/* *available_buttons is holding a pointer to the first element of an array
 * of struct available_button it is set up in such a way due to the file being
 * generated at compile time you can still call it as such though
* eg available_buttons[0] or  available_buttons[available_button_count] (NULL SENTINEL, 0)*/

extern const size_t button_helper_maxbuffer;
extern const struct available_button * const available_buttons;
extern const int available_button_count;

int get_button_names(char *buf, size_t bufsz, unsigned long button);
#endif /* _BUTTON_HELPER_H_ */
