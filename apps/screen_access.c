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

#include <lcd.h>
#include <lcd-remote.h>
#include <font.h>
#include <button.h>
#include <sprintf.h>
#include <settings.h>
#include <kernel.h>
#include <icons.h>

#include "screen_access.h"

struct screen screens[NB_SCREENS];

void screen_init(struct screen * screen, enum screen_type screen_type)
{
    switch(screen_type)
    {
#ifdef HAVE_REMOTE_LCD
        case SCREEN_REMOTE:
            screen->width=LCD_REMOTE_WIDTH;
            screen->height=LCD_REMOTE_HEIGHT;
            screen->is_color=false;
            screen->depth=1;
#ifdef HAVE_LCD_BITMAP
            screen->setmargins=&lcd_remote_setmargins;
            screen->getymargin=&lcd_remote_getymargin;
            screen->getxmargin=&lcd_remote_getxmargin;
            screen->setfont=&lcd_remote_setfont;
            screen->getstringsize=&lcd_remote_getstringsize;
            screen->putsxy=&lcd_remote_putsxy;
            screen->mono_bitmap=&lcd_remote_mono_bitmap;
            screen->set_drawmode=&lcd_remote_set_drawmode;
#if LCD_DEPTH > 1
            /* since lcd_remote_set_background doesn't exist yet ... */
            screen->set_background=NULL;
#endif /* LCD_DEPTH > 1 */
            screen->update_rect=&lcd_remote_update_rect;
            screen->fillrect=&lcd_remote_fillrect;
            screen->drawrect=&lcd_remote_drawrect;
            screen->drawpixel=&lcd_remote_drawpixel;
            screen->drawline=&lcd_remote_drawline;
            screen->vline=&lcd_remote_vline;
            screen->hline=&lcd_remote_hline;
            screen->scroll_speed=&lcd_remote_scroll_speed;
            screen->scroll_delay=&lcd_remote_scroll_delay;
            screen->scroll_step=&lcd_remote_scroll_step;
            screen->puts_scroll_style=&lcd_remote_puts_scroll_style;
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS
            screen->double_height=&lcd_double_height;
            screen->putc=&lcd_remote_putc;
#endif /* HAVE_LCD_CHARCELLS */
            screen->init=&lcd_remote_init;
            screen->puts_scroll=&lcd_remote_puts_scroll;
            screen->stop_scroll=&lcd_remote_stop_scroll;
            screen->clear_display=&lcd_remote_clear_display;
            screen->update=&lcd_remote_update;
            screen->puts=&lcd_remote_puts;

            break;
#endif /* HAVE_REMOTE_LCD */

        case SCREEN_MAIN:
        default:
#ifdef HAVE_LCD_COLOR
            screen->is_color=true;
#else
            screen->is_color=false;
#endif
            screen->depth=LCD_DEPTH;
#ifdef HAVE_LCD_BITMAP
            screen->width=LCD_WIDTH;
            screen->height=LCD_HEIGHT;
            screen->setmargins=&lcd_setmargins;
            screen->getymargin=&lcd_getymargin;
            screen->getxmargin=&lcd_getxmargin;
            screen->setfont=&lcd_setfont;
            screen->getstringsize=&lcd_getstringsize;
            screen->putsxy=&lcd_putsxy;
            screen->mono_bitmap=&lcd_mono_bitmap;
            screen->set_drawmode=&lcd_set_drawmode;
#if LCD_DEPTH > 1
            screen->set_background=&lcd_set_background;
#endif
            screen->update_rect=&lcd_update_rect;
            screen->fillrect=&lcd_fillrect;
            screen->drawrect=&lcd_drawrect;
            screen->drawpixel=&lcd_drawpixel;
            screen->drawline=&lcd_drawline;
            screen->vline=&lcd_vline;
            screen->hline=&lcd_hline;
            screen->scroll_speed=&lcd_scroll_speed;
            screen->scroll_delay=&lcd_scroll_delay;
            screen->scroll_step=&lcd_scroll_step;
            screen->puts_scroll_style=&lcd_puts_scroll_style;
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS
            screen->width=11; /* width in characters instead of pixels */
            screen->height=2;
            screen->double_height=&lcd_double_height;
            screen->putc=&lcd_putc;
#ifdef SIMULATOR
            screen->icon=&sim_lcd_icon;
#else
            screen->icon=&lcd_icon;
#endif

#endif /* HAVE_LCD_CHARCELLS */
            screen->init=&lcd_init;
            screen->puts_scroll=&lcd_puts_scroll;
            screen->stop_scroll=&lcd_stop_scroll;
            screen->clear_display=&lcd_clear_display;
#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
            screen->update=&lcd_update;
#endif
            screen->puts=&lcd_puts;
            break;
    }
#ifdef HAVE_LCD_BITMAP
    screen->setfont(FONT_UI);
#endif
    screen->screen_type=screen_type;
    screen_update_nblines(screen);
}


/*
 * Returns the number of text lines that can be drawn on the given screen
 * with it's current font
 */
void screen_update_nblines(struct screen * screen)
{
#ifdef HAVE_LCD_BITMAP
    int height=screen->height;
    if(global_settings.statusbar)
        height -= STATUSBAR_HEIGHT;
#ifdef HAS_BUTTONBAR
    if(global_settings.buttonbar)
        height -= BUTTONBAR_HEIGHT;
#endif
    screen->getstringsize("A", &screen->char_width, &screen->char_height);
    screen->nb_lines = height / screen->char_height;
#else
    screen->char_width=1;
    screen->char_height=1;
    /* default on char based player supported by rb */
    screen->nb_lines = MAX_LINES_ON_SCREEN;
#endif

}

void screen_access_init(void)
{
    screen_init(&screens[0], SCREEN_MAIN);
#ifdef HAVE_REMOTE_LCD
    screen_init(&screens[1], SCREEN_REMOTE);
#endif
}

void screen_access_update_nb_lines(void)
{
    int i;
    for(i=0;i<NB_SCREENS;++i)
        screen_update_nblines(&screens[i]);
}
