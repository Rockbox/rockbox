/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#ifdef HAVE_LCD_BITMAP

#include "lcd.h"
#include "button.h"
#include "kernel.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>

#define SS_TITLE       "Bouncer"
#define SS_TITLE_FONT  2

unsigned char table[]={
26,28,30,33,35,37,39,40,42,43,45,46,46,47,47,47,47,47,46,46,45,43,42,40,39,37,35,33,30,28,26,24,21,19,17,14,12,10,8,7,5,4,2,1,1,0,0,0,0,0,1,1,2,4,5,7,8,10,12,14,17,19,21,23,
};

unsigned char xtable[]={
54,59,64,69,73,77,81,85,88,91,94,96,97,99,99,99,99,99,97,96,94,91,88,85,81,77,73,69,64,59,54,50,45,40,35,30,26,22,18,14,11,8,5,3,2,0,0,0,0,0,2,3,5,8,11,14,18,22,26,30,35,40,45,49,
};

#define XDIFF -4
#define YDIFF -6

extern const unsigned char char_gen_12x16[][22];

static void loopit(void)
{
    int b;
    int y=0;
    int x=16;
    int yy,xx;
    unsigned int i;

    char rock[]={'R', 'O', 'C', 'K', 'b', 'o', 'x'};

    lcd_clear_display();
    while(1)
    {
        b = button_get(false);
        if ( b & BUTTON_OFF )
            return;
#if 1
        lcd_clear_display();
#else
        lcd_clearrect(xtable[x&63], table[y&63], 11, 16);
        lcd_clearrect(xtable[(x+XDIFF)&63], table[(y+YDIFF)&63], 11, 16);
        lcd_clearrect(xtable[(x+XDIFF*2)&63], table[(y+YDIFF*2)&63], 11, 16);
        lcd_clearrect(xtable[(x+XDIFF*3)&63], table[(y+YDIFF*3)&63], 11, 16);
        lcd_clearrect(xtable[(x+XDIFF*4)&63], table[(y+YDIFF*4)&63], 11, 16);
        lcd_clearrect(xtable[(x+XDIFF*5)&63], table[(y+YDIFF*5)&63], 11, 16);
        lcd_clearrect(xtable[(x+XDIFF*6)&63], table[(y+YDIFF*6)&63], 11, 16);
#endif
        y+=3;
        x++;

        yy=y;
        xx=x;
        for(i=0; i<sizeof(rock)/sizeof(rock[0]); i++, yy+=YDIFF, xx+=XDIFF)
          lcd_bitmap((char *)char_gen_12x16[rock[i]-0x20],
                     xtable[xx&63], table[yy&63],
                     11, 16, false);
        lcd_update();

        sleep(HZ/10);
    }
}


void bounce(void)
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

    lcd_putsxy(LCD_WIDTH/2-len, LCD_HEIGHT-(2*h), off,0);

    lcd_update();
    sleep(HZ);
    loopit();
}

#endif
