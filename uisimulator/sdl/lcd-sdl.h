/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \ 
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __LCDSDL_H__
#define __LCDSDL_H__

#include "uisdl.h"
#include "lcd.h"

extern SDL_Surface*  lcd_surface;
#if LCD_DEPTH <= 8
extern SDL_Color   lcd_palette[(1<<LCD_DEPTH)];
#endif

#ifdef HAVE_REMOTE_LCD
extern SDL_Surface*  remote_surface;
extern SDL_Color   remote_palette[(1<<LCD_REMOTE_DEPTH)];
#endif

void simlcdinit(void);

#endif // #ifndef __LCDSDL_H__

