/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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

#ifndef _LED_MINI2440_H_
#define _LED_MINI2440_H_

/* LED functions for debug etc */

#define LED1        0x0020      /* GPB5 */
#define LED2        0x0040      /* GPB6 */
#define LED3        0x0080      /* GPB7 */
#define LED4        0x0100      /* GPB8 */

#define LED_NONE    0x0000
#define LED_ALL (LED1|LED2|LED3|LED4)

void led_init (void);

/* Turn on one or more LEDS */
void set_leds (int led_mask);

/* Turn off one or more LEDS */
void clear_leds (int led_mask);

/* Alternate flash of pattern1 and pattern2 - never returns */
void led_flash (int led_pattern1, int led_pattern2);

#endif /* _LED_MINI2440_H_ */
