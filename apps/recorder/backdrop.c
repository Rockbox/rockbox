/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include "config.h"
#include "lcd.h"
#include "backdrop.h"

fb_data main_backdrop[LCD_HEIGHT][LCD_WIDTH];
fb_data wps_backdrop[LCD_HEIGHT][LCD_WIDTH];

bool load_main_backdrop(char* filename)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)&main_backdrop[0][0];
    ret = read_bmp_file(filename, &bm, sizeof(main_backdrop), FORMAT_NATIVE);

    if ((ret > 0) && (bm.width == LCD_WIDTH) 
                  && (bm.height == LCD_HEIGHT)) {
        lcd_set_backdrop(&main_backdrop[0][0]);
        return true;
    } else {
        lcd_set_backdrop(NULL);
        return false;
    }
}
