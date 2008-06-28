/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: jackpot.c 14034 2007-07-28 05:42:55Z kevin $
 *
 * Copyright (C) 2007 Copyright Kévin Ferrare
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

#ifndef _PICTURE_
#define _PICTURE_
#include "plugin.h"

struct picture{
    const void* data;
    int width;
    int height;
};

void picture_draw(struct screen* display, const struct picture* picture,
                  int x, int y);

void vertical_picture_draw_part(struct screen* display, const struct picture* picture,
                       int yoffset,
                       int x, int y);

void vertical_picture_draw_sprite(struct screen* display, const struct picture* picture,
                         int sprite_no,
                         int x, int y);
#endif
