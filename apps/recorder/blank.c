/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "features.h"

#ifdef USE_SCREENSAVERS

#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>

#define SS_TITLE       "Blank"
#define SS_TITLE_FONT  2

Menu blank(void)
{
    int w, h;
    char *off = "[Off] to stop";
    int len = strlen(SS_TITLE);

    lcd_getfontsize(SS_TITLE_FONT, &w, &h);

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
    lcd_putsxy(LCD_WIDTH/2-len, (LCD_HEIGHT/2)-h, SS_TITLE, SS_TITLE_FONT);

    len = strlen(off);
    lcd_getfontsize(0, &w, &h);

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

    lcd_putsxy(LCD_WIDTH/2-len, LCD_HEIGHT-(2*h), off, 0);

    lcd_update();
    sleep(HZ);

    lcd_clear_display();
    lcd_update();

    while(1) {
        if(button_get(false))
            return MENU_OK;
        sleep(HZ/10);
    }
    
    return MENU_OK;
}

#endif



