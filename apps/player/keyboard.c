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
#include "lcd-player-charset.h"
#include "lang.h"
#include "debug.h"

#define KEYBOARD_PAGES 3

extern unsigned short *lcd_ascii;

static unsigned short* kbd_setupkeys(int page, int* len)
{
    static unsigned short lines[128];

    unsigned short ch;
    int i = 0;

    switch (page) {
    case 0: /* Capitals */
        for (ch = 'A'; ch <= 'Z'; ch++)
            lines[i++] = ch;
        for (ch = 0xc0; ch <= 0xdd; ch++)
            if (lcd_ascii[ch] != NOCHAR_NEW && lcd_ascii[ch] != NOCHAR_OLD)
                lines[i++] = ch;
        break;

    case 1: /* Small */
        for (ch = 'a'; ch <= 'z'; ch++)
            lines[i++] = ch;
        for (ch = 0xdf; ch <= 0xff; ch++)
            if (lcd_ascii[ch] != NOCHAR_NEW && lcd_ascii[ch] != NOCHAR_OLD)
                lines[i++] = ch;
        break;

    case 2: /* Others */
        for (ch = ' '; ch <= '@'; ch++) 
          lines[i++] = ch;
        break;
    }

    lines[i] = 0;
    *len = i;

    return lines;
}

/* Delimiters for highlighting the character selected for insertion */
#define KEYBOARD_INSERT_LEFT 0x81
#define KEYBOARD_INSERT_RIGHT 0x82

#define KEYBOARD_CURSOR 0x7f
#define KEYBOARD_ARROW 0x92

#define MENU_LINE_INPUT 0
#define MENU_LINE_NEWCHARS 1
#define MENU_LINE_BACKSPACE 2
#define MENU_LINE_DELETE 3
#define MENU_LINE_ACCEPT 4
#define MENU_LINE_QUIT 5
#define MENU_LINE_LAST 5

int kbd_input(char* text, int buflen)
{
    bool done = false;
    int page=0, x=0;
    int linelen;
    unsigned short* line = kbd_setupkeys(page, &linelen);
    int menu_line=0;
    int cursor_pos=0;
    int button_pressed;
    unsigned char temptext[12];
    int old_cursor_pos=0;               /* Windowed cursor movement */
    int left_pos=0;

    lcd_clear_display();

    old_cursor_pos=cursor_pos=strlen(text);
    if (9<cursor_pos)
        left_pos=cursor_pos-9;

    while (!done) {
        int i, p;
        int len = strlen(text);
        int scrpos;
        int dir;

        scrpos = cursor_pos - left_pos;
        dir = cursor_pos - old_cursor_pos;

        /* Keep the cursor on screen, with a 2 character scroll margin */
        if(dir < 0) {
           if(scrpos < 2) {
              left_pos = cursor_pos - 2;
              if(left_pos < 0)
                 left_pos = 0;
           }
        }
        if(dir > 0) {
           if(scrpos > 7) {
              left_pos = cursor_pos - 9;
              if(left_pos < 0)
                 left_pos = 0;
              if(left_pos > len - 9)
                 left_pos = len - 9;
           }
        }

        p=0;
        i = left_pos;
        while (p<10 && text[i]) {
            temptext[p++]=text[i++];
        }
        temptext[p]=0;
        lcd_remove_cursor();
        lcd_puts(1, 0, temptext);
        lcd_put_cursor(cursor_pos-left_pos+1, 0, 0x7f);
        old_cursor_pos=cursor_pos;

        switch (menu_line) {
            case MENU_LINE_INPUT:
            case MENU_LINE_NEWCHARS:
                /* Draw insert chars */
                temptext[0]=KEYBOARD_INSERT_LEFT;
                temptext[1]=line[x%linelen];
                temptext[2]=KEYBOARD_INSERT_RIGHT;
                for (i=1; i < 8; i++) {
                    temptext[i+2]=line[(i+x)%linelen];
                }
                temptext[i+2]=0;
                lcd_puts(1, 1, temptext);
                break;
            case MENU_LINE_BACKSPACE:
                lcd_puts_scroll(1, 1, str(LANG_PLAYER_KEYBOARD_BACKSPACE));
                break;
            case MENU_LINE_DELETE:
                lcd_puts_scroll(1, 1, str(LANG_PLAYER_KEYBOARD_DELETE));
                break;
            case MENU_LINE_ACCEPT:
                lcd_puts_scroll(1, 1, str(LANG_PLAYER_KEYBOARD_ACCEPT));
                break;
            case MENU_LINE_QUIT:
                lcd_puts_scroll(1, 1, str(LANG_PLAYER_KEYBOARD_ABORT));
                break;
        }
        if (menu_line==MENU_LINE_INPUT) {
            lcd_putc(0, 0, KEYBOARD_ARROW);
            lcd_putc(0, 1, ' ');
        } else {
            lcd_putc(0, 0, ' ');
            lcd_putc(0, 1, KEYBOARD_ARROW);
        }
          
        lcd_update();

        button_pressed=button_get(true);
        switch (menu_line) 
        {
            case MENU_LINE_INPUT:
                switch (button_pressed)
                {
                    case BUTTON_PLAY:
                    case BUTTON_PLAY | BUTTON_REPEAT:
                        if (cursor_pos<len)
                            cursor_pos++;
                        button_pressed=BUTTON_NONE;
                        break;

                    case BUTTON_STOP:
                    case BUTTON_STOP | BUTTON_REPEAT:
                        if (cursor_pos>0)
                            cursor_pos--;
                        button_pressed=BUTTON_NONE;
                        break;
                }
                break;
                
            case MENU_LINE_NEWCHARS:
                switch (button_pressed)
                {
                    case BUTTON_PLAY:
                    case BUTTON_PLAY | BUTTON_REPEAT:
                        x=(x+1+linelen)%linelen;
                        button_pressed=BUTTON_NONE;
                        break;
                    case BUTTON_STOP:
                    case BUTTON_STOP | BUTTON_REPEAT:
                        x=(x-1+linelen)%linelen;
                        button_pressed=BUTTON_NONE;
                        break;
            
                    case BUTTON_MENU:
                        /* shift */
                        if (++page == KEYBOARD_PAGES)
                            page = 0;
                        line = kbd_setupkeys(page, &linelen);
                        break;

                    case BUTTON_ON:
                        if (len < buflen) {
                            /* ON insert the char */
                            for (i=len+1; i>cursor_pos; i--) {
                                text[i]=text[i-1];
                            }
                            text[cursor_pos]=line[x];
                            button_pressed=BUTTON_NONE;
                            cursor_pos++;
                        }
                        break;
                }
                break;

            case MENU_LINE_BACKSPACE:
                switch (button_pressed) {
                    case BUTTON_ON:
                    case BUTTON_PLAY:
                    case BUTTON_PLAY | BUTTON_REPEAT:
                        button_pressed=BUTTON_NONE;
                        if (0 < cursor_pos) {
                            for (i=--cursor_pos; i<=len; i++) {
                                text[i]=text[i+1];
                            }
                        }
                        break;
                    case BUTTON_STOP:
                    case BUTTON_STOP | BUTTON_REPEAT:
                        button_pressed=BUTTON_NONE;
                        for (i=cursor_pos; i<=len; i++) {
                            text[i]=text[i+1];
                        }
                        break;
                }
                break;
            case MENU_LINE_DELETE:
                switch (button_pressed) {
                    case BUTTON_ON:
                    case BUTTON_PLAY:
                    case BUTTON_PLAY | BUTTON_REPEAT:
                        button_pressed=BUTTON_NONE;
                        for (i=cursor_pos; i<=len; i++) {
                            text[i]=text[i+1];
                        }
                        break;
                    case BUTTON_STOP:
                    case BUTTON_STOP | BUTTON_REPEAT:
                        button_pressed=BUTTON_NONE;
                        if (0 < cursor_pos)
                            for (i=--cursor_pos; i<=len; i++) {
                                text[i]=text[i+1];
                            }
                        break;
                }
                break;

            case MENU_LINE_ACCEPT:
                switch (button_pressed) {
                    case BUTTON_ON:
                    case BUTTON_PLAY:
                    case BUTTON_PLAY | BUTTON_REPEAT:
                        button_pressed=BUTTON_NONE;
                        done=true;
                        break;
                }
                break;

            case MENU_LINE_QUIT:
                switch (button_pressed) {
                    case BUTTON_ON:
                    case BUTTON_PLAY:
                    case BUTTON_PLAY | BUTTON_REPEAT:
                        return 1;
                        break;
                }
                break;
        }

        /* Handle unhandled events */
        switch (button_pressed) {
            case BUTTON_NONE:
                /* button is already handled */
                break;

            case BUTTON_MENU | BUTTON_STOP:
                break;
        
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (menu_line<MENU_LINE_LAST)
                    menu_line++;
                else
                  menu_line=0;
                break;
          
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (menu_line>0)
                    menu_line--;
                else
                  menu_line=MENU_LINE_LAST;
                break;
        }
    }

    return 0;
}
