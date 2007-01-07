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
    /* ON follows the setting of the backlight - same brightness */
    BUTTONLIGHT_ON,

    /* buttonlights always off */
    BUTTONLIGHT_OFF,

    /* buttonlights always on but set at lowest brightness */
    BUTTONLIGHT_FAINT,

    /* buttonlights flicker when triggered */
    BUTTONLIGHT_FLICKER,
};


/* call to flicker the button lights */
void __buttonlight_flicker(unsigned short brightness);


/* only use the XX__ENTRY when setting the mode */
void __buttonlight_mode(enum buttonlight_mode mode);


/* select which led to use on the button lights. Other combinations are
 * possible, but don't look very good.
 */
void __buttonlight_select(enum buttonlight_selection selection);


void __backlight_init(void);
void __backlight_on(void);
void __backlight_off(void);
void __backlight_set_brightness(int val);

/* true: backlight fades off - false: backlight fades on */
void __backlight_dim(bool dim);

#endif
