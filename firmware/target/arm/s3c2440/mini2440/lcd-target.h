/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins, Lyre Project
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

extern void lcd_enable(bool state);

/* Setup for Mini2440, 3.5" TFT LCD Touchscreen */

/* Config values for LCDCON1 */
#define LCD_CLKVAL  4
#define LCD_MMODE   0
#define LCD_PNRMODE 3
#define LCD_BPPMODE 12
#define LCD_ENVID   1

/* Config values for LCDCON2 */
#define LCD_UPPER_MARGIN 1
#define LCD_LOWER_MARGIN 4
#define LCD_VSYNC_LEN    1

/* Config values for LCDCON3 */
#define LCD_RIGHT_MARGIN 0
#define LCD_LEFT_MARGIN  25

/* Config values for LCDCON4 */
#define LCD_HSYNC_LEN 4
