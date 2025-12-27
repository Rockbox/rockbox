/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __LCD_ECHOPLAYER_H__
#define __LCD_ECHOPLAYER_H__

#include "config.h"

/* Hsync pulse width in units of dot clocks */
#define LCD_HSW 10

/* Hsync back porch in units of dot clocks */
#define LCD_HBP 20

/* Horizontal active width in units of dot clocks */
#define LCD_HAW LCD_WIDTH

/* Hsync front porch in units of dot clocks */
#define LCD_HFP 10

/* Vsync pulse height in units of horizontal lines */
#define LCD_VSH 2

/* Vsync back porch in units of horizontal lines */
#define LCD_VBP 2

/* Vertical active height in units of horizontal lines */
#define LCD_VAH LCD_HEIGHT

/* Vsync front porch in units of horizontal lines */
#define LCD_VFP 2

/* Total horizontal width in dots */
#define LCD_HWIDTH   (LCD_HSW + LCD_HBP + LCD_HAW + LCD_HFP)

/* Total vertical height in lines */
#define LCD_VHEIGHT  (LCD_VSH + LCD_VBP + LCD_VAH + LCD_VFP)

/* Target frame rate */
#define LCD_FPS 70

/* Dot clock frequency */
#define LCD_DOTCLOCK_FREQ (LCD_FPS * LCD_HWIDTH * LCD_VHEIGHT)

#endif /* __LCD_ECHOPLAYER_H__ */
