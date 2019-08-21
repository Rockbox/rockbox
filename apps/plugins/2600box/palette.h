/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
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

#ifndef PALETTE_H
#define PALETTE_H


#if LCD_DEPTH>=8
extern fb_data vcs_palette[NUMCOLS];    /* colour pixel in framebuffer format */
#else
extern uint32_t vcs_palette[NUMCOLS];   /* dither pattern */
#endif

void setup_palette(const enum tv_system type);

void show_palette(void);
char *palette_chg_parameter(const int s, const int h, const int v);
enum tv_system get_palette_type(void);


#endif /* PALETTE_H */
