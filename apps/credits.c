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

//#ifdef __ROCKBOX_CREDITS_H__

#include "credits.h"
#include "lcd.h"
#include "kernel.h"

#define DISPLAY_TIME  200

struct credit credits[CREDIT_COUNT] = {
    { "[Credits]",               ""                                  },
    { "Björn Stenberg",          "Originator, project manager, code" },
    { "Linus Nielsen Feltzing",  "Electronics, code"                 },
    { "Andy Choi",               "Checksums"                         },
    { "Andrew Jamieson",         "Schematics, electronics"           },
    { "Paul Suade",              "Serial port setup"                 },
    { "Joachim Schiffer",        "Schematics, electronics"           },
    { "Daniel Stenberg",         "Code"                              },
    { "Alan Korr",               "Code"                              },
    { "Gary Czvitkovicz",        "Code"                              },
    { "Stuart Martin",           "Code"                              },
    { "Felix Arends",            "Code"                              },
    { "Ulf Ralberg",             "Thread embryo"                     },
    { "David Härdeman",          "Initial ID3 code"                  },
    { "Thomas Saeys",            "Logo"                              },
    { "Grant Wier",              "Code"                              },
    { "Julien Labruyére",        "Donated Archos Player"             },
    { "Nicolas Sauzede",         "Display research"                  },
    { "Robert Hak",              "Code, FAQ, Sarcasm"                },
    { "Dave Chapman",            "Code"                              },
    { "Stefan Meyer",            "Code"                              },
};

void show_credits(void)
{
    int i = 0;
	int line = 0;

	lcd_clear_display();

	while(i < CREDIT_COUNT-1) {
		if ((line % 4 == 0) && (line!=0)) {
			lcd_puts(0, 0, (char *)credits[0].name);
			lcd_update();
			sleep(DISPLAY_TIME);
			lcd_clear_display();
			line=0;
		}
		lcd_puts(0, ++line, (char *)credits[++i].name);
	}

	if ((i-1)%4 != 0) {
			lcd_puts(0, 0, (char *)credits[0].name);
			lcd_update();
			sleep(DISPLAY_TIME);
			lcd_clear_display();
	}

}

//#endif
