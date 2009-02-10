/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2009 by Jens Arnold
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

#ifndef __SCREENDUMP_H__
#define __SCREENDUMP_H__

#include "config.h"

/* Make BMP colour map entries from R, G, B triples, without and with blending.
 * Not within HAVE_LCD_BITMAP because it is also used for the Player sim */
#define RED_CMP(c)   (((c) >> 16) & 0xff)
#define GREEN_CMP(c) (((c) >> 8) & 0xff)
#define BLUE_CMP(c)  ((c) & 0xff)

#define BMP_COLOR(c)  BLUE_CMP(c), GREEN_CMP(c), RED_CMP(c), 0
#define BMP_COLOR_MIX(c1, c2, num, den) \
        (BLUE_CMP(c2)  - BLUE_CMP(c1))  * (num) / (den) + BLUE_CMP(c1),  \
        (GREEN_CMP(c2) - GREEN_CMP(c1)) * (num) / (den) + GREEN_CMP(c1), \
        (RED_CMP(c2)   - RED_CMP(c1))   * (num) / (den) + RED_CMP(c1),   0

#define LE16_CONST(x) (x)&0xff, ((x)>>8)&0xff
#define LE32_CONST(x) (x)&0xff, ((x)>>8)&0xff, ((x)>>16)&0xff, ((x)>>24)&0xff


#ifdef BOOTLOADER

#define screen_dump()
#define remote_screen_dump()

#else /* !BOOTLOADER */

#ifdef HAVE_LCD_BITMAP
/* Save a .BMP file containing the current screen contents. */
void screen_dump(void);
void screen_dump_set_hook(void (*hook)(int fd));
#endif
#ifdef HAVE_REMOTE_LCD
/* Save a .BMP file containing the current remote screen contents. */
void remote_screen_dump(void);
#endif

#endif /* !BOOTLOADER */

#endif /* __SCREENDUMP_H__ */
