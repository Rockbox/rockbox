/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "version.h"
#include "debug_menu.h"
#include "sprintf.h"
#include <string.h>

#include "bmp.h"
#include "icons.h"
#include "font.h"

#define KEYBOARD_LINES 4
#define KEYBOARD_PAGES 3

static void kbd_setupkeys(char* line[KEYBOARD_LINES], int page)
{
    switch (page) {
    case 0:
        line[0] = "ABCDEFG !?\" @#$%+'";
        line[1] = "HIJKLMN 789 &_()-`";
        line[2] = "OPQRSTU 456 §|{}/<";
        line[3] = "VWXYZ.,0123 ~=[]*>";
        break;

    case 1:
        line[0] = "abcdefg ¢£¤¥¦§©®¬";
        line[1] = "hijklmn «»°ºª¹²³¶";
        line[2] = "opqrstu ¯±×÷¡¿µ·¨";
        line[3] = "vwxyz   ¼½¾      ";
        break;

    case 2:
        line[0] = "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË";
        line[1] = "àáâãäåæ ìíîï èéêë";
        line[2] = "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ";
        line[3] = "òóôõöø çðþýÿ ùúûü";
        break;
    }
}

static void kbd_draw_statusbar_button(int num, char* caption, int y, int fw)
{
    int x, x2, tw, cx;
    x = num*(LCD_WIDTH/3);
    x2 = (num+1)*(LCD_WIDTH/3);
    tw = fw * strlen(caption);
    cx = x2 - x;
    /* center the text */
    lcd_putsxy((x + (cx/2)) - (tw/2), y, caption);
    lcd_invertrect(x, y - 1, (x2-x)-1, LCD_HEIGHT-y+1);
}

int kbd_input(char* text, int buflen)
{
    bool done = false;
    int page = 0;

    int font_w = 0, font_h = 0, i;
    int x = 0, y = 0;
    int main_x, main_y, max_chars, margin;
    int status_y1, status_y2, curpos;
    int len;
    char* line[KEYBOARD_LINES]; 

    char c = 0;
    struct font* font = font_get(FONT_SYSFIXED);

    lcd_setfont(FONT_SYSFIXED);
    font_w = font->maxwidth;
    font_h = font->height;

    margin = 3;
    main_y = (KEYBOARD_LINES + 1) * font_h + margin;
    main_x = 0;
    status_y1 = LCD_HEIGHT - font_h;
    status_y2 = LCD_HEIGHT;

    max_chars = LCD_WIDTH / font_w;
    kbd_setupkeys(line, page);

    while(!done)
    {
        lcd_clear_display();

        /* draw page */
        for (i=0; i < KEYBOARD_LINES; i++)
            lcd_putsxy(0, i * font_h, line[i]);

        len = strlen(text);
        
        /* separator */
        lcd_drawline(0, main_y - margin, LCD_WIDTH, main_y - margin);
		
        /* write out the text */
        if (len <= max_chars)
        {	  
            /* if we have enough room */
            lcd_putsxy(main_x, main_y, text);
            curpos = main_x + len * font_w;
        }
        else
        {
            /* if we don't have enough room, write out the last bit only */
            lcd_putsxy(0, main_y, "<");
            lcd_invertrect(0, main_y, font_w, font_h);
			
            lcd_putsxy(font_w, main_y, text + len - (max_chars-1));
            curpos = main_x + max_chars * font_w;
        }

        /* cursor */
        lcd_drawline(curpos, main_y, curpos, main_y + font_h);

        /* draw the status bar */
        kbd_draw_statusbar_button(0, "Shift", status_y1, font_w);
        kbd_draw_statusbar_button(1, "Cancl", status_y1, font_w);
        kbd_draw_statusbar_button(2, "Del", status_y1, font_w);
		
        /* highlight the key that has focus */		
        lcd_invertrect(font_w * x, font_h * y, font_w, font_h);
        lcd_update();

        switch ( button_get(true) ) {

            case BUTTON_OFF:
            case BUTTON_F2:
                /* abort */
                lcd_setfont(FONT_UI);
                return -1;
                break;

            case BUTTON_F1:
                /* Page */
                if (++page == KEYBOARD_PAGES)
                    page = 0;
                kbd_setupkeys(line, page);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (x < (int)strlen(line[y]) - 1) 
                    x++; 
                else 
                    x = 0;
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (x)
                    x--;
                else
                    x = strlen(line[y]) - 1;
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                if (y < KEYBOARD_LINES - 1)
                    y++;
                else
                    y=0;
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                if (y)
                    y--;
                else
                    y = KEYBOARD_LINES - 1;
                break;

            case BUTTON_F3:
                /* backspace */
                if (len)
                    text[len-1] = 0;
                break;

            case BUTTON_ON:
                /* ON accepts what was entered and continues */
                done = true;
                break;

            case BUTTON_PLAY:
                /* PLAY inserts the selected char */
                if (len<buflen)
                {
                    c = line[y][x];
                    text[len] = c;
                    text[len+1] = 0;
                }
                break;
        }
    }
    lcd_setfont(FONT_UI);
    return 0;
}
