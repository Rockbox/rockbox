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

#ifndef _GUI_BUTTONBAR_H_
#define _GUI_BUTTONBAR_H_
#include "config.h"
#include "button.h"
#include "screen_access.h"


#ifdef HAVE_BUTTONBAR
#define BUTTONBAR_HEIGHT 8
#define BUTTONBAR_MAX_BUTTONS 3
#define BUTTONBAR_CAPTION_LENGTH 8


struct gui_buttonbar
{
    char caption[BUTTONBAR_MAX_BUTTONS][BUTTONBAR_CAPTION_LENGTH];
    struct screen * display;
};

/*
 * Initializes the buttonbar
 *  - buttonbar : the buttonbar
 */
extern void gui_buttonbar_init(struct gui_buttonbar * buttonbar);

/*
 * Attach the buttonbar to a screen
 *  - buttonbar : the buttonbar
 *  - display : the display to attach the buttonbar
 */
extern void gui_buttonbar_set_display(struct gui_buttonbar * buttonbar,
                                      struct screen * display);

/*
 * Set the caption of the items of the buttonbar
 *  - buttonbar : the buttonbar
 *  - caption1,2,3 : the first, second and thirds items of the bar
 */
extern void gui_buttonbar_set(struct gui_buttonbar * buttonbar,
                              const char *caption1,
                              const char *caption2,
                              const char *caption3);

/*
 * Disable the buttonbar
 *  - buttonbar : the buttonbar
 */
extern void gui_buttonbar_unset(struct gui_buttonbar * buttonbar);

/*
 * Draw the buttonbar on it's attached screen
 *  - buttonbar : the buttonbar
 */
extern void gui_buttonbar_draw(struct gui_buttonbar * buttonbar);

/*
 * Returns true if the buttonbar has something to display, false otherwise
 *  - buttonbar : the buttonbar
 */
extern bool gui_buttonbar_isset(struct gui_buttonbar * buttonbar);
#else
#define BUTTONBAR_HEIGHT 0
#endif
#endif /* _GUI_BUTTONBAR_H_ */
