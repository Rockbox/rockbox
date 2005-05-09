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
#include "string.h"

const char* const credits[] = {
#include "credits.raw" /* generated list of names from docs/CREDITS */
};

#ifdef HAVE_LCD_CHARCELLS
#define MAX(x, y) ((x) > (y) ? (x) : (y))
void roll_credits(void)
{
    int numnames = sizeof(credits)/sizeof(char*);
    int curr_name = 0;
    int curr_len = strlen(credits[0]);
    int curr_index = 0;
    int curr_line = 0;
    int name, len, new_len, line, x;

    while (1)
    {
        lcd_clear_display();

        name = curr_name;
        x = -curr_index;
        len = curr_len;
        line = curr_line;

        while (x < 11)
        {
            int x2;

            if (x < 0)
                lcd_puts(0, line, credits[name] - x);
            else
                lcd_puts(x, line, credits[name]);
                
            if (++name >= numnames)
                break;
            line ^= 1;

            x2 = x + len/2;
            if ((unsigned)x2 < 11)
                lcd_putc(x2, line, '*');

            new_len = strlen(credits[name]);
            x += MAX(len/2 + 2, len - new_len/2 + 1);
            len = new_len;
        }
        /* abort on keypress */
        if (button_get_w_tmo(HZ/8) & BUTTON_REL)
            return;

        if (++curr_index >= curr_len)
        {
            if (++curr_name >= numnames)
                break;
            new_len = strlen(credits[curr_name]);
            curr_index -= MAX(curr_len/2 + 2, curr_len - new_len/2 + 1);
            curr_len = new_len;
            curr_line ^= 1;
        }
    }
}
#else

void roll_credits(void)
{
    int i;
    int line = 0;
    int numnames = sizeof(credits)/sizeof(char*);
    char buffer[40];

    int y=LCD_HEIGHT;

    int height;
    int width;

    lcd_setfont(FONT_UI);

    lcd_getstringsize("A", &width, &height);

    while(1) {
        lcd_clear_display();
        for ( i=0; i <= (LCD_HEIGHT-y)/height; i++ )
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
