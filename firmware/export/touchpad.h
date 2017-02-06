/***************************************************************************
 * __________ __ ___.
 * Open \______ \ ____ ____ | | _\_ |__ _______ ___
 * Source | _// _ \_/ ___\| |/ /| __ \ / _ \ \/ /
 * Jukebox | | ( <_> ) \___| < | \_\ ( <_> > < <
 * Firmware |____|_ /\____/ \___ >__|_ \|___ /\____/__/\_ \
 * \/ \/ \/ \/ \/
 * $Id$
 *
 * Copyright (C) 2013 by Jean-Louis Biasini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _TOUCHDEV_H_
#define _TOUCHDEV_H_

enum touchpad_mode
{
    /* Button mode: touchpad returns BUTTON_* codes based on the touch coordinates
     * using the table provided by the target driver. The touch coordinates are
     * still accessible from button data */
    TOUCHPAD_BUTTON = 0,
    /* Point mode: touchpad returns BUTTON_TOUCHPAD on touch and coordinates in
     * button data */
    TOUCHPAD_POINT,
};

/* Set and get touchpad mode */
void touchpad_set_mode(enum touchpad_mode mode);
enum touchpad_mode touchpad_get_mode(void);

/* Important defines for touchpad
 *
 * BUTTON_TOUCHPAD_ALL:
 *     a bitmap of all buttons are can be reported by the touchpad
 *
 * TOUCHPAD_WIDTH and TOUCHPAD_HEIGHT:
 *     coordinates (x,y) reported by the touchpad must satisfy
 *     0 <= x < TOUCHPAD_WIDTH and 0 <= y < TOUCHPAD_HEIGHT
 *
 * TOUCHPAD_WIDTH_PIXELS, TOUCHPAD_HEIGHT_PIXELS
 *     express the width/height of the touchpad in pixels, so that the code can
 *     understand the relationship between touchpad size and screen size
 */

/* This function is provided by the target button driver and maps coordinates
 * to a button (or 0 if not button).
 * FIXME in the future this should be turn into a library function that uses
 * a target specific map for button names, like touchscreen but more generic */
int touchpad_to_button(int x, int y);
/* This function must be called by the target button driver when the user touches
 * the touchpad, and ORed with any other non-touchpad buttons. It will return
 * fill the button data and return the relevant BUTTON_* code based on touchpad
 * mode. This function will call touchpad_to_button() in button mode. */
int touchpad_report_touch(int x, int y, int *data);

/* Enable or disable touchpad. This function will call the target-specific
 * touchpad_enable_device() function that can power down the touchpad if it
 * possible/wanted. */
void touchpad_enable(bool en);
bool touchpad_is_enabled(void);
/* This function filters-out all touchpad related buttons when the touchpad
 * is disabled. This is necessary because if the touchpad is not powered down
 * when disabled, the target button driver may still report touchpad events that
 * need to be ignored */
int touchpad_filter(int button);

#endif /* _TOUCHDEV_H_ */
