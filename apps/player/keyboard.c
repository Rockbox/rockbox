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
    static unsigned short lines[256];

    unsigned short ch;
    int i = 0;

    switch (page) {
    case 0: /* Capitals */
        for (ch = 'A'; ch <= 'Z'; ch++) lines[i++] = ch;
        for (ch = 0xc0; ch <= 0xdd; ch++)
            if (lcd_ascii[ch] != NOCHAR_NEW && lcd_ascii[ch] != NOCHAR_OLD)
                lines[i++] = ch;
        break;

    case 1: /* Small */
        for (ch = 'a'; ch <= 'z'; ch++) lines[i++] = ch;
        for (ch = 0xdf; ch <= 0xff; ch++)
            if (lcd_ascii[ch] != NOCHAR_NEW && lcd_ascii[ch] != NOCHAR_OLD)
                lines[i++] = ch;
        break;

    case 2: /* Others */
        for (ch = ' '; ch <= '@'; ch++) lines[i++] = ch;
        for (ch = 0x5b; ch <= 0x60; ch++)
            if (lcd_ascii[ch] != NOCHAR_NEW && lcd_ascii[ch] != NOCHAR_OLD)
                lines[i++] = ch;
        for (ch = 0x07b; ch <= 0x0bf; ch++)
            if (lcd_ascii[ch] != NOCHAR_NEW && lcd_ascii[ch] != NOCHAR_OLD)
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

    while(!done)
    {
        int i;
        int len = strlen(text);

        lcd_clear_display();

        /* draw chars */
        for (i=0; i < 11; i++) {
            if (line[i+x])
                lcd_putc(i, 1, line[i+x]);
            else
                break;
        }
        
        /* write out the text */
        if (len <= 11) {
            /* if we have enough room */
            lcd_puts(0, 0, text);
        }
        else {
            /* if we don't have enough room, write out the last bit only */
            lcd_putc(0, 0, '<');
            lcd_puts(1, 0, text + len - 10);
        }
        lcd_update();

        switch ( button_get(true) ) {

            case BUTTON_MENU | BUTTON_STOP:
                /* abort */
                return -1;
                break;

            case BUTTON_MENU:
                /* shift */
                if (++page == KEYBOARD_PAGES)
                    page = 0;
                line = kbd_setupkeys(page, &linelen);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (x < linelen - 1) 
                    x++; 
                else 
                    x = 0;
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (x)
                    x--;
                else
                    x = linelen - 1;
                break;

            case BUTTON_STOP:
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
                    text[len] = line[x];
                    text[len+1] = 0;
                }
                break;
        }
    }
    return 0;
}
