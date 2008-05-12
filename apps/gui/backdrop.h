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

#if LCD_DEPTH > 1

#include "lcd.h"
#include "bmp.h"

bool load_main_backdrop(const char* filename);
bool load_wps_backdrop(const char* filename);

void unload_main_backdrop(void);
void unload_wps_backdrop(void);

void show_main_backdrop(void);
void show_wps_backdrop(void);

#endif /* LCD_DEPTH > 1 */

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
bool load_remote_wps_backdrop(const char* filename);
void unload_remote_wps_backdrop(void);
void show_remote_wps_backdrop(void);
void show_remote_main_backdrop(void); /* only clears the wps backdrop */
#endif

#endif /* _BACKDROP_H */
