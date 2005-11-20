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

#define KEYBOARD_PAGES 3

extern unsigned short *lcd_ascii;

static unsigned short* kbd_setupkeys(int page, int* len)
{
    static unsigned short lines[128];

    unsigned short ch;
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

    int len, i;
    int editpos, curpos, leftpos;
    unsigned short* line = kbd_setupkeys(page, &linelen);
    unsigned char temptext[12];

    int button, lastbutton = 0;

    editpos = strlen(text);

    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */ 

    while (!done)
    {
        len = strlen(text);
        
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
            temptext[0] = KEYBOARD_INSERT_LEFT;
            temptext[1] = line[x];
            temptext[2] = KEYBOARD_INSERT_RIGHT;
            for (i = 1; i < 8; i++)
            {
                temptext[i+2] = line[(x+i)%linelen];
            }
            temptext[i+2] = 0;
            lcd_puts(1, 0, temptext);

            /* write out the text */
            curpos = MIN(MIN(editpos, 10 - MIN(len - editpos, 3)), 9);
            leftpos = editpos - curpos;
            strncpy(temptext, text + leftpos, 10);
            temptext[10] = 0;
            
            if (leftpos)
                temptext[0] = '<';
            if (len - leftpos > 10)
                temptext[9] = '>';

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
                    if (editpos < len)
                    {
                        editpos++;
                        kbd_spellchar(text[editpos]);
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
                        kbd_spellchar(text[editpos]);
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
                        for (i = editpos; i < len; i++)
                            text[i-1] = text[i];
                        text[i-1] = '\0';
                        editpos--;
                    }
                }
                else  /* inserts the selected char */
                {
                    if (len + 1 < buflen)
                    {
                        for (i = len ; i > editpos; i--)
                            text[i] = text[i-1];
                        text[len+1] = 0;
                        text[editpos] = line[x];
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

