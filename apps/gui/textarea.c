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

#include "textarea.h"

void gui_textarea_clear(struct screen * display)
{
#ifdef HAVE_LCD_BITMAP
    int y_start = gui_textarea_get_ystart(display);
    int y_end = gui_textarea_get_yend(display);

    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(0, y_start, display->width, y_end - y_start);
    display->set_drawmode(DRMODE_SOLID);
    display->stop_scroll();
    screen_set_ymargin(display, y_start);
#else
    display->clear_display();
#endif
}

#ifdef HAVE_LCD_BITMAP
void gui_textarea_update(struct screen * display)
{
    int y_start = gui_textarea_get_ystart(display);
    int y_end = gui_textarea_get_yend(display);
    display->update_rect(0, y_start, display->width, y_end - y_start);
}
#endif

void gui_textarea_update_nblines(struct screen * display)
{
#ifdef HAVE_LCD_BITMAP
    int height=display->height;
    if(global_settings.statusbar)
        height -= STATUSBAR_HEIGHT;
#ifdef HAS_BUTTONBAR
    if(global_settings.buttonbar && display->has_buttonbar)
        height -= BUTTONBAR_HEIGHT;
#endif
    display->getstringsize("A", &display->char_width, &display->char_height);
    display->nb_lines = height / display->char_height;
#else
    display->char_width  = 1;
    display->char_height = 1;
    /* default on char based player supported by rb */
    display->nb_lines = MAX_LINES_ON_SCREEN;
#endif
}
