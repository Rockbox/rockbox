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
#include "kernel.h"
#include "button.h"

char* credits[] = {
    "Bjorn Stenberg",
    "Linus Nielsen Feltzing",
    "Andy Choi", 
    "Andrew Jamieson", 
    "Paul Suade", 
    "Joachim Schiffer", 
    "Daniel Stenberg", 
    "Alan Korr", 
    "Gary Czvitkovicz", 
    "Stuart Martin", 
    "Felix Arends", 
    "Ulf Ralberg", 
    "David Hardeman", 
    "Thomas Saeys", 
    "Grant Wier", 
    "Julien Labruyere", 
    "Nicolas Sauzede", 
    "Robert Hak", 
    "Dave Chapman", 
    "Stefan Meyer", 
    "Eric Linenberg",
    "Tom Cvitan",
    "Magnus Oman",
    "Jerome Kuptz",
    "Julien Boissinot",
    "Nuutti Kotivuori",
    "Heikki Hannikainen",
    "Hardeep Sidhu",
    "Markus Braun",
    "Justin Heiner",
    "Magnus Holmgren",
    "Bill Napier",
    "George Styles",
	"Mats Lidell"
};

#ifdef HAVE_LCD_BITMAP
#define MAX_LINES 7
#define DISPLAY_TIME  HZ*2
#else
#define MAX_LINES 2
#define DISPLAY_TIME  HZ
#endif

void roll_credits(void)
{
    int i;
    int j;
    int line = 0;
    int numnames = sizeof(credits)/sizeof(char*);

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,8);
#endif

    for ( i=0; i < numnames; i += MAX_LINES )
    {
        lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
        lcd_putsxy(0, 0, " [Credits]",0);
#endif
        for(line = 0;line < MAX_LINES && line+i < numnames;line++)
        {
            lcd_puts(0, line, credits[line+i]);
        }

        lcd_update();

        /* abort on keypress */
        for ( j=0;j<10;j++ )
        {
            sleep(DISPLAY_TIME/10);
            if (button_get(false))
                return;
        }
    }
    return;
}
