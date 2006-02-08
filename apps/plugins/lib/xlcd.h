/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Additional LCD routines not present in the core itself
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __XLCD_H__
#define __XLCD_H__

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

void xlcd_init(struct plugin_api* newrb);
void xlcd_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3);

#if LCD_DEPTH >= 8
void xlcd_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height);
void xlcd_gray_bitmap(const unsigned char *src, int x, int y, int width,
                      int height);
#endif

void xlcd_scroll_left(int count);
void xlcd_scroll_right(int count);
void xlcd_scroll_up(int count);
void xlcd_scroll_down(int count);

#endif /* HAVE_LCD_BITMAP */
#endif /* __XLCD_H__ */

