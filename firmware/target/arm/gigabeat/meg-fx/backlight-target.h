/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H


/* select the led */
enum buttonlight_selection
{
    /* all leds */
    BUTTONLIGHT_LED_ALL,

    /* only the menu/power led (two buttons for one LED) */
    BUTTONLIGHT_LED_MENU
};


/* Use these to set the buttonlight mode */
enum buttonlight_mode
{
    /* ON follows the setting */
    BUTTONLIGHT_ON,

    /* buttonlights always off */
    BUTTONLIGHT_OFF,

    /* buttonlights always on but set at lowest brightness */
    BUTTONLIGHT_FAINT,

    /* buttonlights flicker when triggered - continues to flicker
     * even if the flicker is still asserted.
     */
    BUTTONLIGHT_FLICKER,

    /* buttonlights solid for as long as triggered */
    BUTTONLIGHT_SIGNAL,
    
    /* buttonlights follow backlight */
    BUTTONLIGHT_FOLLOW,
    
    /* buttonlights show battery charging */
    BUTTONLIGHT_CHARGING,
};


/* Call this to flicker or signal the button lights. Only is effective for
 * modes that take a trigger input.
 */
void __buttonlight_trigger(void);


/* select which led to use on the button lights. Other combinations are
 * possible, but don't look very good.
 */

/* map the mode from the command into the state machine entries */
/* See enum buttonlight_mode for available functions */
void __buttonlight_mode(enum buttonlight_mode mode, 
                        enum buttonlight_selection selection,
                        unsigned short brightness);


bool __backlight_init(void);
void __backlight_on(void);
void __backlight_off(void);
void __backlight_set_brightness(int val);

/* true: backlight fades off - false: backlight fades on */
void __backlight_dim(bool dim);

#endif
