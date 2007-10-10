/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Bj�rn Stenberg
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
#include "sprintf.h"
#include <string.h>
#include "settings.h"
#include "statusbar.h"
#include "talk.h"
#include "misc.h"
#include "rbunicode.h"
#include "lang.h"

#define KBD_BUF_SIZE  64
#define KEYBOARD_PAGES 3

static unsigned short *kbd_setupkeys(int page, int* len)
{
    static unsigned short kbdline[KBD_BUF_SIZE];
    const unsigned char *p;
    int i = 0;

    switch (page)
    {
        case 0: /* Capitals */
            p = "ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅ"
                "ÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝ";
            break;

        case 1: /* Small */
            p = "abcdefghijklmnopqrstuvwxyzßàáâãä"
                "åçèéêëìíîïñòóôõöøùúûüýÿ";
            break;

        default: /* Others */
            p = " !\"#$%&'()*+,-./0123456789:;<=>?@[]_{}~";
            break;
    }

    while (*p)
        p = utf8decode(p, &kbdline[i++]);

    *len = i;

    return kbdline;
}

/* Delimiters for highlighting the character selected for insertion */
#define KEYBOARD_INSERT_LEFT 0xe110
#define KEYBOARD_INSERT_RIGHT 0xe10f

#define KEYBOARD_CURSOR 0x7f
#define KEYBOARD_ARROW 0xe10c

/* helper function to spell a char if voice UI is enabled */
static void kbd_spellchar(char c)
{
    if (talk_menus_enabled()) /* voice UI? */
    {
         unsigned char tmp[5];
         /* store char to pass to talk_spell */
         unsigned char* utf8 = utf8encode(c, tmp);
         *utf8 = 0;

         if(c == ' ')
              talk_id(VOICE_BLANK, false);
        else talk_spell(tmp, false);
    }
}

static void say_edit(void)
{
    if (talk_menus_enabled())
        talk_id(VOICE_EDIT, false);
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
    unsigned short *line = kbd_setupkeys(page, &linelen);
    unsigned char temptext[36];
    unsigned char *utf8;

    int button, lastbutton = 0;

    editpos = utf8length(text);

    if (talk_menus_enabled()) /* voice UI? */
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
            
            lcd_putc(1, 0, KEYBOARD_INSERT_LEFT);
            lcd_putc(2, 0, line[x]);
            lcd_putc(3, 0, KEYBOARD_INSERT_RIGHT);
            for (i = 1; i < 8; i++)
            {
                lcd_putc(i + 3, 0, line[(x+i)%linelen]);
            }

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
                if (line_edit)
                    say_edit();
                else
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
                    utf8 = utf8encode(line[x], temptext);
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
                if (talk_menus_enabled()) /* voice UI? */
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

