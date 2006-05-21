/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _BACKDROP_H
#define _BACKDROP_H

#ifdef HAVE_LCD_COLOR

#include "lcd.h"
#include "bmp.h"
#include "backdrop.h"
/*
extern fb_data main_backdrop[LCD_HEIGHT][LCD_WIDTH];
extern fb_data wps_backdrop[LCD_HEIGHT][LCD_WIDTH];
*/

bool load_main_backdrop(char* filename);
bool load_wps_backdrop(char* filename);

void unload_main_backdrop(void);
void unload_wps_backdrop(void);

void show_main_backdrop(void);
void show_wps_backdrop(void);

#endif /* HAVE_LCD_COLOR */

#endif /* _BACKDROP_H */
