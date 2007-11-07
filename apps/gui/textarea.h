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

#ifndef _GUI_TEXTAREA_H_
#define _GUI_TEXTAREA_H_
#include "screen_access.h"
#include "settings.h"
#include "statusbar.h"

struct text_message
{
    char **message_lines;
    int nb_lines;
};

/*
 * Clears the area in the screen in which text can be displayed
 * and sets the y margin properly
 *  - display : the screen structure
 */
extern void gui_textarea_clear(struct screen * display);

/*
 * Updates the area in the screen in which text can be displayed
 *  - display : the screen structure
 */
extern void gui_textarea_update(struct screen * display);

/*
 * Displays message lines on the given screen
 *  - display : the screen structure
 *  - message : the lines to display
 *  - ystart : the lineon which we start displaying
 * returns : the number of lines effectively displayed
 */
extern int gui_textarea_put_message(struct screen * display,
                                    struct text_message * message,
                                    int ystart);
/*
 * Compute the number of text lines the display can draw with the current font
 * Also updates the char height and width
 * - display : the screen structure
 */
extern void gui_textarea_update_nblines(struct screen * display);

/*
 * Speak a text_message. The message's lines may be virtual pointers
 * representing language / voicefont IDs (see settings.h).
 */
extern void talk_text_message(struct text_message * message, bool enqueue);

#ifdef HAVE_LCD_BITMAP
/*
 * Compute the number of pixels from which text can be displayed
 * - display : the screen structure
 * Returns the number of pixels
 */
#define gui_textarea_get_ystart(display) \
    ( (global_settings.statusbar)? STATUSBAR_HEIGHT : 0)

/*
 * Compute the number of pixels below which text can't be displayed
 * - display : the screen structure
 * Returns the number of pixels
 */
#ifdef HAS_BUTTONBAR
#define gui_textarea_get_yend(display) \
    ( (display)->height - ( (global_settings.buttonbar && \
                            (display)->has_buttonbar)? \
                            BUTTONBAR_HEIGHT : 0) )
#else
#define gui_textarea_get_yend(display) \
    ( (display)->height )
#endif /* HAS_BUTTONBAR */

#endif /* HAVE_LCD_BITMAP */

#endif /* _GUI_TEXTAREA_H_ */
