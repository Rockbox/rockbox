/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Robert Hak <rhak at ramapo.edu>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "credits.h"
#include "lcd.h"
#include "font.h"
#include "kernel.h"
#include "button.h"
#include "sprintf.h"

const char* const credits[] = {
#include "credits.raw" /* generated list of names from docs/CREDITS */
};

#ifdef HAVE_LCD_BITMAP
#define MAX_LINES 7
#define DISPLAY_TIME  HZ*2
#else
#define MAX_LINES 2
#define DISPLAY_TIME  HZ
#endif

#ifdef HAVE_LCD_CHARCELLS
void roll_credits(void)
{
    int i;
    int line = 0;
    int numnames = sizeof(credits)/sizeof(char*);

    lcd_clear_display();

    for ( i=0; i < numnames; i += MAX_LINES )
    {
        lcd_clear_display();
        for(line = 0;line < MAX_LINES && line+i < numnames;line++)
        {
            lcd_puts(0, line, credits[line+i]);
        }

        /* abort on keypress */
        if (button_get_w_tmo(DISPLAY_TIME) & BUTTON_REL)
            return;
    }
    return;
}
#else

void roll_credits(void)
{
    int i;
    int line = 0;
    int numnames = sizeof(credits)/sizeof(char*);
    char buffer[40];

    int y=64;

    int height;
    int width;

    lcd_setfont(FONT_UI);

    lcd_getstringsize("A", &width, &height);

    while(1) {
        lcd_clear_display();
        for ( i=0; i <= (64-y)/height; i++ )
            lcd_putsxy(0, i*height+y, line+i<numnames?credits[line+i]:"");
        snprintf(buffer, sizeof(buffer), " [Credits] %2d/%2d  ",
                 line+1, numnames);
        lcd_clearrect(0, 0, LCD_WIDTH, height);
        lcd_putsxy(0, 0, buffer);
        lcd_update();

        if (button_get_w_tmo(HZ/20) & BUTTON_REL)
            return;

        y--;

        if(y<0) {
            line++;
            if(line >= numnames)
                break;
            y+=height;
        }

    }
    return;
}
#endif
