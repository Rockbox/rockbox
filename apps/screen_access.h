/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _SCREEN_ACCESS_H_
#define _SCREEN_ACCESS_H_

#include "lcd.h"
#include "buttonbar.h"

enum screen_type {
    SCREEN_MAIN
#ifdef HAVE_REMOTE_LCD
    ,SCREEN_REMOTE
#endif
};

#ifdef HAVE_REMOTE_LCD
#define NB_SCREENS 2
#else
#define NB_SCREENS 1
#endif

#ifdef HAVE_LCD_CHARCELLS
#define MAX_LINES_ON_SCREEN 2
#endif

struct screen
{
    int width, height;
    int nb_lines;
    enum screen_type screen_type;
    bool is_color;
    int depth;
    int char_width;
    int char_height;
#ifdef HAVE_LCD_BITMAP

    void (*setmargins)(int x, int y);
    int (*getxmargin)(void);
    int (*getymargin)(void);

    void (*setfont)(int newfont);
    int (*getstringsize)(const unsigned char *str, int *w, int *h);
    void (*putsxy)(int x, int y, const unsigned char *str);

    void (*scroll_step)(int pixels);
    void (*puts_scroll_style)(int x, int y,
                              const unsigned char *string, int style);
    void (*mono_bitmap)(const unsigned char *src,
                        int x, int y, int width, int height);
    void (*set_drawmode)(int mode);
#if LCD_DEPTH > 1
    void (*set_background)(int brightness);
#endif /* LCD_DEPTH > 1 */
    void (*update_rect)(int x, int y, int width, int height);
    void (*fillrect)(int x, int y, int width, int height);
    void (*drawrect)(int x, int y, int width, int height);
    void (*drawpixel)(int x, int y);
    void (*drawline)(int x1, int y1, int x2, int y2);
    void (*vline)(int x, int y1, int y2);
    void (*hline)(int x1, int x2, int y);
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS
    void (*double_height)(bool on);
    void (*putc)(int x, int y, unsigned short ch);
    void (*icon)(int icon, bool enable);
#endif
    void (*init)(void);
    void (*puts_scroll)(int x, int y, const unsigned char *string);
    void (*scroll_speed)(int speed);
    void (*scroll_delay)(int ms);
    void (*stop_scroll)(void);
    void (*clear_display)(void);
#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
    void (*update)(void);
#endif

    void (*puts)(int x, int y, const unsigned char *str);
};

/*
 * Initializes the given screen structure for a given display
 * - screen : the screen structure
 * - display_type : currently 2 possibles values : MAIN or REMOTE
 */
extern void screen_init(struct screen * screen, enum screen_type screen_type);

/*
 * Compute the number of text lines the display can draw with the current font
 * - screen : the screen structure
 * Returns the number of text lines
 */
extern void screen_update_nblines(struct screen * screen);

#ifdef HAVE_LCD_BITMAP
/*
 * Compute the number of pixels from which text can be displayed
 * - screen : the screen structure
 * Returns the number of pixels
 */
#define screen_get_text_y_start(screen) \
    (global_settings.statusbar?STATUSBAR_HEIGHT:0)

/*
 * Compute the number of pixels below which text can't be displayed
 * - screen : the screen structure
 * Returns the number of pixels
 */
#ifdef HAS_BUTTONBAR
#define screen_get_text_y_end(screen) \
    ( (screen)->height - (global_settings.buttonbar?BUTTONBAR_HEIGHT:0) )
#else
#define screen_get_text_y_end(screen) \
    ( (screen)->height )
#endif /* HAS_BUTTONBAR */

#endif /* HAVE_LCD_BITMAP */

/*
 * Initializes the whole screen_access api
 */
extern void screen_access_init(void);

/*
 * Just recalculate the number of text lines that can be displayed
 * on each screens in case of poilice change for example
 */
extern void screen_access_update_nb_lines(void);

/*
 * exported screens array that should be used
 * by each app that wants to write to access display
 */
extern struct screen screens[NB_SCREENS];

#endif /*_SCREEN_ACCESS_H_*/
