/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak (rhak at ramapo.edu)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifdef HAVE_LCD_BITMAP

#include "screensaver.h"
#include "types.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"

#ifdef SIMULATOR
#include <stdio.h>
#include <string.h>
#endif

#define SS_TITLE       "Boxes"
#define SS_TITLE_FONT  2

void ss_loop(void)
{
    int b;
    int x2 = LCD_WIDTH/2;
    int y2 = LCD_HEIGHT/2;
    int x = LCD_WIDTH/2;
    int y = LCD_HEIGHT/2;
    int i = 0;
    int center = 0;
    int factor = 0;
    int offset = 0;

    if (LCD_HEIGHT < LCD_WIDTH)
        center = LCD_HEIGHT/2;
    else
        center = LCD_WIDTH/2;

    i = center+1;
    while(1)
    {
        /* Grow */
        if ( i < 0 ) {
            factor = 1;
            i = 1;
        }

        /* Shrink */
        if (i >= center) {
            factor = -1;
            i = center;
        }

        offset=i*factor;

        b = button_get();
        if ( b & BUTTON_OFF )
            return;

        lcd_clear_display();
        lcd_drawrect(x-offset, y-offset, x2+offset, y2+offset);
        lcd_update();

        i+=factor;

        sleep(10);
    }
}


void screensaver(void)
{
    char w, h;
    char *off = "[Off] to stop";
    int len = strlen(SS_TITLE);

    lcd_fontsize(SS_TITLE_FONT, &w, &h);

    /* Get horizontel centering for text */
    len *= w;
    if (len%2 != 0)
        len = ((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    lcd_clear_display();
    lcd_puts(LCD_WIDTH/2-len, (LCD_HEIGHT/2)-h, SS_TITLE, SS_TITLE_FONT);

    len = strlen(off);
    lcd_fontsize(0, &w, &h);

    /* Get horizontel centering for text */
    len *= w;
    if (len%2 != 0)
        len = ((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    lcd_puts(LCD_WIDTH/2-len, LCD_HEIGHT-(2*h), off, 0);

    lcd_update();
    sleep(150);
    ss_loop();
}

#endif
