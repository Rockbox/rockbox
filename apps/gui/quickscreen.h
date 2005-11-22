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
#include "button.h"
#include "config.h"
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H100_PAD) ||\
    (CONFIG_KEYPAD == IRIVER_H300_PAD)

#ifndef _GUI_QUICKSCREEN_H_
#define _GUI_QUICKSCREEN_H_

#define HAS_QUICKSCREEN

#include "option_select.h"
#include "screen_access.h"

#define QUICKSCREEN_LEFT        BUTTON_LEFT
#define QUICKSCREEN_BOTTOM      BUTTON_DOWN
#define QUICKSCREEN_BOTTOM_INV  BUTTON_UP
#define QUICKSCREEN_RIGHT       BUTTON_RIGHT

#if CONFIG_KEYPAD == RECORDER_PAD
#define QUICKSCREEN_QUIT        BUTTON_F2
#define QUICKSCREEN_QUIT2       BUTTON_F3
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define QUICKSCREEN_QUIT        BUTTON_MODE
#define QUICKSCREEN_QUIT2       BUTTON_OFF
#define QUICKSCREEN_RC_QUIT     BUTTON_RC_MODE
#ifdef CONFIG_REMOTE_KEYPAD
#define QUICKSCREEN_RC_LEFT        BUTTON_RC_REW
#define QUICKSCREEN_RC_BOTTOM      BUTTON_RC_VOL_DOWN
#define QUICKSCREEN_RC_BOTTOM_INV  BUTTON_RC_VOL_UP
#define QUICKSCREEN_RC_RIGHT       BUTTON_RC_FF
#endif

#endif

struct gui_quickscreen;
/*
 * Callback function called each time the quickscreen gets modified
 *  - qs : the quickscreen that did the modification
 */
typedef void (quickscreen_callback)(struct gui_quickscreen * qs);

struct gui_quickscreen
{
    struct option_select *left_option;
    struct option_select *bottom_option;
    struct option_select *right_option;
    char * left_right_title;
    quickscreen_callback *callback;
};

/*
 * Initializes a quickscreen
 *  - qs : the quickscreen
 *  - left_option, bottom_option, right_option : a list of choices
 *                                               for each option
 *  - left_right_title : the 2nd line of the title
 *                       on the left and on the right
 *  - callback : a callback function called each time the quickscreen
 *               gets modified
 */
void gui_quickscreen_init(struct gui_quickscreen * qs,
                          struct option_select *left_option,
                          struct option_select *bottom_option,
                          struct option_select *right_option,
                          char * left_right_title,
                          quickscreen_callback *callback);
/*
 * Draws the quickscreen on a given screen
 *  - qs : the quickscreen
 *  - display : the screen to draw on
 */
void gui_quickscreen_draw(struct gui_quickscreen * qs, struct screen * display);

/*
 * Does the actions associated to the given button if any
 *  - qs : the quickscreen
 *  - button : the key we are going to analyse
 * returns : true if the button corresponded to an action, false otherwise
 */
bool gui_quickscreen_do_button(struct gui_quickscreen * qs, int button);

/*
 * Draws the quickscreen on all available screens
 *  - qs : the quickscreen
 */
void gui_syncquickscreen_draw(struct gui_quickscreen * qs);

/*
 * Runs the quickscreen on all available screens
 *  - qs : the quickscreen
 * returns : true if usb was connected, false otherwise
 */
bool gui_syncquickscreen_run(struct gui_quickscreen * qs);

#endif /*_GUI_QUICK_SCREEN_H_*/
#endif /* CONFIG_KEYPAD */
