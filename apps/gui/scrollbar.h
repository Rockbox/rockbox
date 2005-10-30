/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_SCROLLBAR_H_
#define _GUI_SCROLLBAR_H_
#include <lcd.h>
#ifdef HAVE_LCD_BITMAP

struct screen;

enum orientation {
    VERTICAL,
    HORIZONTAL
};

/*
 * Draws a scrollbar on the given screen
 *  - screen : the screen to put the scrollbar on
 *  - x : x start position of the scrollbar
 *  - y : y start position of the scrollbar
 *  - width : you won't guess =(^o^)=
 *  - height : I won't tell you either !
 *  - items : total number of items on the screen
 *  - min_shown : index of the starting item on the screen
 *  - max_shown : index of the last item on the screen
 *  - orientation : either VERTICAL or HORIZONTAL
 */
extern void gui_scrollbar_draw(struct screen * screen, int x, int y,
                               int width, int height, int items,
                               int min_shown, int max_shown,
                               enum orientation orientation);
#endif /* HAVE_LCD_BITMAP */
#endif /* _GUI_SCROLLBAR_H_ */
