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
#include "system.h"
#include "version.h"
#include "debug_menu.h"
#include "sprintf.h"
#include <string.h>
#include "lcd-player-charset.h"
#include "settings.h"
#include "statusbar.h"
#include "talk.h"
#include "misc.h"
#include "rbunicode.h"

#define KEYBOARD_PAGES 3

extern unsigned short *lcd_ascii;

static unsigned char* kbd_setupkeys(int page, int* len)
{
    static unsigned char lines[128];

    unsigned ch;
    int i = 0;

    switch (page) 
    {
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

/* helper function to spell a char if voice UI is enabled */
static void kbd_spellchar(char c)
{
    static char spell_char[2] = "\0\0"; /* store char to pass to talk_spell */

    if (global_settings.talk_menu) /* voice UI? */
    {
        spell_char[0] = c; 
        talk_spell(spell_char, false);
    }
}

int kbd_input(char* text, int buflen)
{
    bool done = false;
    bool redraw = true;
    bool line_edit = false;
    int page = 0, x = 0;
    int linelen;

    int len, len_utf8, i, j;
    int editpos, curpos, leftpos;
    unsigned char *line = kbd_setupkeys(page, &linelen);
    unsigned char temptext[36];
    unsigned char tmp;
    unsigned char *utf8;

    int button, lastbutton = 0;

    editpos = utf8length(text);

    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */ 

    while (!done)
    {
        len = strlen(text);
        len_utf8 = utf8length(text);

        if (redraw)
        {
            if (line_edit) 
            {
                lcd_putc(0, 0, ' ');
                lcd_putc(0, 1, KEYBOARD_ARROW);
            }
            else
            {
                lcd_putc(0, 0, KEYBOARD_ARROW);
                lcd_putc(0, 1, ' ');
            }

            /* Draw insert chars */
            utf8 = temptext;
            tmp = KEYBOARD_INSERT_LEFT;
            utf8 = iso_decode(&tmp, utf8, 0, 1);
            utf8 = iso_decode(&line[x], utf8, 0, 1);
            tmp = KEYBOARD_INSERT_RIGHT;
            utf8 = iso_decode(&tmp, utf8, 0, 1);
            for (i = 1; i < 8; i++)
            {
                utf8 = iso_decode(&line[(x+i)%linelen], utf8, 0, 1);
            }
            *utf8 = 0;
            lcd_puts(1, 0, temptext);

            /* write out the text */
            curpos = MIN(MIN(editpos, 10 - MIN(len_utf8 - editpos, 3)), 9);
            leftpos = editpos - curpos;
            if (!leftpos) {
                utf8 = text + utf8seek(text, leftpos);
                i = 0;
                j = 0;
            } else {
                temptext[0] = '<';
                i = 1;
                j = 1;
                utf8 = text + utf8seek(text, leftpos+1);
            }
            while (*utf8 && i < 10) {
                temptext[j++] = *utf8++;
                if ((*utf8 & MASK) != COMP)
                    i++;
            }
            temptext[j] = 0;


            if (len_utf8 - leftpos > 10) {
                utf8 = temptext + utf8seek(temptext, 9);
                *utf8++ = '>';
                *utf8 = 0;
            }

            lcd_remove_cursor();
            lcd_puts(1, 1, temptext);
            lcd_put_cursor(curpos + 1, 1, KEYBOARD_CURSOR);
            
            gui_syncstatusbar_draw(&statusbars, true);
        }

        /* The default action is to redraw */
        redraw = true;

        button = button_get_w_tmo(HZ/2);
        switch (button)
        {
            case BUTTON_STOP:  /* abort */
                return -1;
                break;
            
            case BUTTON_MENU:  /* page flip */
                if (++page == KEYBOARD_PAGES)
                    page = 0;
                line = kbd_setupkeys(page, &linelen);
                if (x > linelen - 1)
                    x = linelen - 1;
                kbd_spellchar(line[x]);
                break;

            case BUTTON_ON:    /* toggle mode */
                line_edit = !line_edit;
                if (!line_edit)
                    kbd_spellchar(line[x]);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (line_edit)
                {
                    if (editpos < len_utf8)
                    {
                        editpos++;
                        int c = utf8seek(text, editpos);
                        kbd_spellchar(text[c]);
                    }
                }
                else
                {
                    if (++x >= linelen)
                        x = 0;
                    kbd_spellchar(line[x]);
                }
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (line_edit)
                {
                    if (editpos)
                    {
                        editpos--;
                        int c = utf8seek(text, editpos);
                        kbd_spellchar(text[c]);
                    }
                }
                else
                {
                    if (--x < 0)
                        x = linelen - 1;
                    kbd_spellchar(line[x]);
                }
                break;

            case BUTTON_PLAY | BUTTON_REPEAT:
                /* accepts what was entered and continues */
                done = true;
                break;

            case BUTTON_PLAY | BUTTON_REL:
                if (lastbutton != BUTTON_PLAY)
                    break;
                if (line_edit) /* backspace in line_edit */
                {
                    if (editpos > 0)
                    {
                        utf8 = text + utf8seek(text, editpos);
                        i = 0;
                        do {
                            i++;
                            utf8--;
                        } while ((*utf8 & MASK) == COMP);
                        while (utf8[i]) {
                            *utf8 = utf8[i];
                            utf8++;
                        }
                        *utf8 = 0;
                        editpos--;
                    }
                }
                else  /* inserts the selected char */
                {
                    utf8 = iso_decode((unsigned char*)&line[x], temptext, 0, 1);
                    *utf8 = 0;
                    j = strlen(temptext);
                    if (len + j < buflen)
                    {
                        int k = len_utf8;
                        for (i = len+j; k >= editpos; i--) {
                            text[i] = text[i-j];
                            if ((text[i] & MASK) != COMP)
                                k--;
                        }
                        while (j--)
                            text[i--] = temptext[j];
                        editpos++;
                    }
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false); /* speak revised text */
                break;

            case BUTTON_NONE:
                gui_syncstatusbar_draw(&statusbars, false);
                redraw = false;
                break;

            default:
                default_event_handler(button);
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
    
    return 0;
}

