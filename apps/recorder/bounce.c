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
#include "options.h"

#ifdef USE_DEMOS

#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"
#include "sprintf.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>

#define SS_TITLE       "Bouncer"
#define SS_TITLE_FONT  2

static unsigned char table[]={
26,28,30,33,35,37,39,40,42,43,45,46,46,47,47,47,47,47,46,46,45,43,42,40,39,37,35,33,30,28,26,24,21,19,17,14,12,10,8,7,5,4,2,1,1,0,0,0,0,0,1,1,2,4,5,7,8,10,12,14,17,19,21,23,
};

static unsigned char xtable[]={
54,58,63,67,71,75,79,82,85,88,91,93,95,97,98,99,99,99,99,99,97,96,94,92,90,87,84,80,77,73,69,65,60,56,52,47,43,39,34,30,26,22,19,15,12,9,7,5,3,2,0,0,0,0,0,1,2,4,6,8,11,14,17,20,24,28,32,36,41,45,49
};

static signed char speed[]={
  1,2,3,3,3,2,1,0,-1,-2,-2,-2,-1,0,0,1,
};


#define XDIFF -4
#define YDIFF -6

extern const unsigned char char_gen_12x16[][22];

enum {
  NUM_XSANKE,
  NUM_YSANKE,
  NUM_XADD,
  NUM_YADD,
  NUM_XDIST,
  NUM_YDIST,

  NUM_LAST
};

struct counter {
  char *what;
  int num;
};

struct counter values[]={
  {"xsanke", 1},
  {"ysanke", 1},
  {"xadd", 1},
  {"yadd", 1},
  {"xdist", -4},
  {"ydistt", -6},
};

static void loopit(void)
{
    int b;
    unsigned int y=100;
    unsigned int x=100;
    unsigned int yy,xx;
    unsigned int i;
    unsigned int ysanke=0;
    unsigned int xsanke=0;

    char rock[]="ROCKbox";

    int show=0;
    int timeout=0;
    char buffer[30];

    lcd_clear_display();
    while(1)
    {
        b = button_get_w_tmo(HZ/10);
        if ( b & BUTTON_OFF )
            break;
        else if(b != BUTTON_NONE)
            timeout=20;

        y+= speed[ysanke&15] + values[NUM_YADD].num;
        x+= speed[xsanke&15] + values[NUM_XADD].num;

        lcd_clear_display();
        if(timeout) {
            switch(b) {
                case BUTTON_LEFT:
                  values[show].num--;
                  break;
                case BUTTON_RIGHT:
                  values[show].num++;
                  break;
                case BUTTON_UP:
                  if(++show == NUM_LAST)
                      show=0;
                  break;
                case BUTTON_DOWN:
                  if(--show < 0)
                      show=NUM_LAST-1;
                  break;
            }
            snprintf(buffer, 30, "%s: %d",
                     values[show].what, values[show].num);
            lcd_putsxy(0, 56, buffer, 0);
            timeout--;
        }
        for(i=0, yy=y, xx=x;
            i<sizeof(rock)-1;
            i++, yy+=values[NUM_YDIST].num, xx+=values[NUM_XDIST].num)
          lcd_bitmap((char *)char_gen_12x16[rock[i]-0x20],
                     xtable[xx%71], table[yy&63],
                     11, 16, false);
        lcd_update();

        ysanke+= values[NUM_YSANKE].num;
        xsanke+= values[NUM_XSANKE].num;
    }
}


Menu bounce(void)
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

    return MENU_OK;
}

#endif
