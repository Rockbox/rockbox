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

struct credit {
    char *name;
    char *desc;
};

struct credit credits[] = {
    { "Bjorn Stenberg",          "Originator, Project manager, Code" },
    { "Linus Nielsen Feltzing",  "Electronics, Code"                 },
    { "Andy Choi",               "Checksums"                         },
    { "Andrew Jamieson",         "Schematics, Electronics"           },
    { "Paul Suade",              "Serial port setup"                 },
    { "Joachim Schiffer",        "Schematics, Electronics"           },
    { "Daniel Stenberg",         "Code"                              },
    { "Alan Korr",               "Code"                              },
    { "Gary Czvitkovicz",        "Code"                              },
    { "Stuart Martin",           "Code"                              },
    { "Felix Arends",            "Code"                              },
    { "Ulf Ralberg",             "Thread embryo"                     },
    { "David Hardeman",          "Initial ID3 code"                  },
    { "Thomas Saeys",            "Logo"                              },
    { "Grant Wier",              "Code"                              },
    { "Julien Labruyere",        "Donated Archos Player"             },
    { "Nicolas Sauzede",         "Display research"                  },
    { "Robert Hak",              "Code, FAQ, Sarcasm"                },
    { "Dave Chapman",            "Code"                              },
    { "Stefan Meyer",            "Code"                              },
    { "Eric Linenberg",          "Sokoban"                           },
};

#ifdef HAVE_LCD_BITMAP
#define MAX_LINES 6
#define DISPLAY_TIME  HZ*2
#else
#define MAX_LINES 2
#define DISPLAY_TIME  HZ
#endif

void roll_credits(void)
{
    unsigned int i;
    int j;
    int line = 0;

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,9);
#endif

    for ( i=0; i<sizeof(credits)/sizeof(struct credit); i++ ) {
#ifdef HAVE_LCD_BITMAP
        lcd_putsxy(0, 0, " [Credits]",0);
#endif
        lcd_puts(0, line, credits[i].name);
        line++;
        if ( line == MAX_LINES ) {
            lcd_update();
            /* abort on keypress */
            for ( j=0;j<10;j++ ) {
                sleep(DISPLAY_TIME/10);
                if (button_get(false))
                    return;
            }
            lcd_clear_display();
            line=0;
        }
    }
    if ( line != MAX_LINES ) {
        lcd_update();
        /* abort on keypress */
        for ( j=0;j<10;j++ ) {
            sleep(DISPLAY_TIME/10);
            if (button_get(false))
                return;
        }
    }
}
