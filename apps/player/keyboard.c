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

#include "debug.h"

#define KEYBOARD_PAGES 3

unsigned short *lcd_ascii;

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
#define MENU_LINE_FILENAME 0
#define MENU_LINE_NEWCHARS 1
#define MENU_LINE_DELETE 2
#define MENU_LINE_ACCEPT 3
#define MENU_LINE_QUIT 4
#define MENU_LINE_LAST 4

    lcd_clear_display();

    while(!done) {
        int i, p;
        int len = strlen(text);

        /* draw filename */
        p=0;
        i=0;
        if (cursor_pos>7) {
            i=cursor_pos-7;
        }
        
        while (p<10 && line[i]) {
            if (i == cursor_pos)
                temptext[p++]=0x7f;
            temptext[p++]=text[i];
            i++;
        }
        temptext[p]=0;
        lcd_puts(1, 0, temptext);

        switch (menu_line) {
        case MENU_LINE_FILENAME:
        case MENU_LINE_NEWCHARS:
            /* Draw insert chars */
            temptext[0]=0x81;
            temptext[1]=line[x%linelen];
            temptext[2]=0x82;
            for (i=1; i < 8; i++) {
                temptext[i+2]=line[(i+x)%linelen];
            }
            temptext[p]=0;
            lcd_puts(1, 1, temptext);
            break;
        case MENU_LINE_DELETE:
            lcd_puts_scroll(1, 1, "Delete next char");
            break;
        case MENU_LINE_ACCEPT:
            lcd_puts_scroll(1, 1, "Accept");
            break;
        case MENU_LINE_QUIT:
            lcd_puts_scroll(1, 1, "Cancel");
            break;
        }
        if (menu_line==MENU_LINE_FILENAME) {
            lcd_putc(0, 0, 0x92);
            lcd_putc(0, 1, ' ');
        } else {
            lcd_putc(0, 0, ' ');
            lcd_putc(0, 1, 0x92);
        }
          
        lcd_update();

        button_pressed=button_get(true);
        if (menu_line==MENU_LINE_FILENAME) {
            switch ( button_pressed ) {
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                if (cursor_pos<len)
                    cursor_pos++;
                button_pressed=0;
                break;
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                if (cursor_pos>0)
                    cursor_pos--;
                button_pressed=0;
                break;
            }
        } else if (menu_line==MENU_LINE_NEWCHARS) {
            switch ( button_pressed ) {

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                x=(x+1+linelen)%linelen;
                button_pressed=0;
                break;
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                x=(x-1+linelen)%linelen;
                button_pressed=0;
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
                    for (i=len; i>cursor_pos; i--) {
                        text[i]=text[i-1];
                    }
                    text[cursor_pos]=line[x];
                    button_pressed=0;
                    cursor_pos++;
                }
                break;
            }
        } else if (menu_line==MENU_LINE_DELETE) {
            switch ( button_pressed ) {
            case BUTTON_ON:
            case BUTTON_PLAY:
            case BUTTON_PLAY | BUTTON_REPEAT:
                button_pressed=0;
                for (i=cursor_pos; i<=len; i++) {
                    text[i]=text[i+1];
                }
                break;
            }
        } else if (menu_line==MENU_LINE_ACCEPT) {
            switch ( button_pressed ) {
            case BUTTON_ON:
            case BUTTON_PLAY:
            case BUTTON_PLAY | BUTTON_REPEAT:
                button_pressed=0;
                done=true;
                break;
            }
        } else if (menu_line==MENU_LINE_QUIT) {
            switch ( button_pressed ) {
            case BUTTON_ON:
            case BUTTON_PLAY:
            case BUTTON_PLAY | BUTTON_REPEAT:
                return 1;
                break;
            }
        }

        /* Handle unhandled events */
        switch ( button_pressed ) {
        case 0:
            /* button is already handled */
            break;

        case BUTTON_MENU | BUTTON_STOP:
            break;
        
        case BUTTON_RIGHT:
        case BUTTON_RIGHT | BUTTON_REPEAT:
            if (menu_line<MENU_LINE_LAST)
                menu_line++;
            break;
          
        case BUTTON_LEFT:
        case BUTTON_LEFT | BUTTON_REPEAT:
            if (menu_line>0)
                menu_line--;
            break;
        }
    }
    return 0;
}
