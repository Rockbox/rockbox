/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Grayscale framework
*
* Copyright (C) 2004 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __GRAY_H__
#define __GRAY_H__

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */

/* This is a generic framework to use grayscale display within rockbox
 * plugins. It obviously does not work for the player.
 */

/* every framework needs such a function, and it has to be called as
 * the very first one */
void gray_init(struct plugin_api* newrb);

/* general functions */
int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth, int *buf_taken);
void gray_release_buffer(void);
void gray_position_display(int x, int by);
void gray_show_display(bool enable);

/* functions affecting the whole display */
void gray_clear_display(void);
void gray_black_display(void);
void gray_deferred_update(void);

/* scrolling functions */
void gray_scroll_left(int count, bool black_border);
void gray_scroll_right(int count, bool black_border);
void gray_scroll_up8(bool black_border);
void gray_scroll_down8(bool black_border);
void gray_scroll_up(int count, bool black_border);
void gray_scroll_down(int count, bool black_border);

/* pixel functions */
void gray_drawpixel(int x, int y, int brightness);
void gray_invertpixel(int x, int y);

/* line functions */
void gray_drawline(int x1, int y1, int x2, int y2, int brightness);
void gray_invertline(int x1, int y1, int x2, int y2);

/* rectangle functions */
void gray_drawrect(int x1, int y1, int x2, int y2, int brightness);
void gray_fillrect(int x1, int y1, int x2, int y2, int brightness);
void gray_invertrect(int x1, int y1, int x2, int y2);

/* bitmap functions */
void gray_drawgraymap(unsigned char *src, int x, int y, int nx, int ny,
                      int stride);
void gray_drawbitmap(unsigned char *src, int x, int y, int nx, int ny,
                     int stride, bool draw_bg, int fg_brightness,
                     int bg_brightness);

/* font support */
void gray_setfont(int newfont);
int gray_getstringsize(unsigned char *str, int *w, int *h);

#endif /* HAVE_LCD_BITMAP */
#endif /* SIMULATOR */
#endif /* __GRAY_H__ */
