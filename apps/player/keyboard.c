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

#define KEYBOARD_PAGES 4

static char* kbd_setupkeys(int page)
{
    static char* lines[] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "abcdefghijklmnopqrstuvwxyz",
        "01234567890!@#$%&/(){}[]<>",
        "+-*_.,:;!?'"
    };

    return lines[page];
}

int kbd_input(char* text, int buflen)
{
    bool done = false;
    int page=0, x=0;
    char* line = kbd_setupkeys(page);
    int linelen = strlen(line);

    while(!done)
    {
        int i;
        int len = strlen(text);

        lcd_clear_display();

        /* draw chars */
        for (i=0; i < 11; i++)
            lcd_putc(i, 1, line[i+x]);

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

        case BUTTON_MENU:
            /* Page */
            if (++page == KEYBOARD_PAGES)
                page = 0;
            line = kbd_setupkeys(page);
            linelen = strlen(line);
            break;

        case BUTTON_RIGHT:
            if (x < linelen - 1) 
                x++; 
            else 
                x = 0;
            break;

        case BUTTON_LEFT:
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
            /* F2 accepts what was entered and continues */
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
