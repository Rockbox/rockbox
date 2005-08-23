/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: not checked in
 *
 * Copyright (C) 2002 Markus Braun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __WIDGETS_H__
#define __WIDGETS_H__
#include <lcd.h>

#ifdef HAVE_LCD_BITMAP
/* Orientation for scrollbar */
enum {
    VERTICAL = 0,
    HORIZONTAL
};

extern void scrollbar(int x, int y, int width, int height, int items, int min_shown, int max_shown, int orientation);
extern void checkbox(int x, int y, int width, int height, bool checked);
#endif /* HAVE_LCD_BITMAP */
#endif /* __WIDGETS_H__ */
