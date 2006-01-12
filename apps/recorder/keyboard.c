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
#include "debug_menu.h"
#include "sprintf.h"
#include <string.h>
#include "font.h"
#include "screens.h"
#include "statusbar.h"
#include "talk.h"
#include "settings.h"
#include "misc.h"
#include "rbunicode.h"
#include "buttonbar.h"
#include "logf.h"

#define KEYBOARD_MARGIN 3

#if (LCD_WIDTH >= 160) && (LCD_HEIGHT >= 96)
#define KEYBOARD_LINES 8
#define KEYBOARD_PAGES 1

#else
#define KEYBOARD_LINES 4
#define KEYBOARD_PAGES 3

#endif


#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define KBD_CURSOR_RIGHT (BUTTON_ON | BUTTON_RIGHT)
#define KBD_CURSOR_LEFT (BUTTON_ON | BUTTON_LEFT)
#define KBD_SELECT BUTTON_SELECT
#define KBD_PAGE_FLIP BUTTON_MODE  /* unused */
#define KBD_DONE_PRE BUTTON_ON
#define KBD_DONE (BUTTON_ON | BUTTON_REL)
#define KBD_ABORT BUTTON_OFF
#define KBD_BACKSPACE BUTTON_REC
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN
#define HAVE_MORSE_INPUT

#elif CONFIG_KEYPAD == RECORDER_PAD
#define KBD_CURSOR_RIGHT (BUTTON_ON | BUTTON_RIGHT)
#define KBD_CURSOR_LEFT (BUTTON_ON | BUTTON_LEFT)
#define KBD_SELECT BUTTON_PLAY
#define KBD_PAGE_FLIP BUTTON_F1
#define KBD_DONE BUTTON_F2
#define KBD_ABORT BUTTON_OFF
#define KBD_BACKSPACE BUTTON_F3
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD /* restricted Ondio keypad */
#define KBD_MODES /* Ondio uses 2 modes, picker and line edit */
#define KBD_SELECT (BUTTON_MENU | BUTTON_REL) /* backspace in line edit */
#define KBD_SELECT_PRE BUTTON_MENU
#define KBD_DONE (BUTTON_MENU | BUTTON_REPEAT)
#define KBD_ABORT BUTTON_OFF
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == GMINI100_PAD
#define KBD_CURSOR_RIGHT (BUTTON_MENU | BUTTON_RIGHT)
#define KBD_CURSOR_LEFT (BUTTON_MENU | BUTTON_LEFT)
#define KBD_SELECT (BUTTON_PLAY | BUTTON_REL)
#define KBD_SELECT_PRE BUTTON_PLAY
#define KBD_PAGE_FLIP BUTTON_ON
#define KBD_DONE (BUTTON_PLAY | BUTTON_REPEAT)
#define KBD_ABORT BUTTON_OFF
#define KBD_BACKSPACE (BUTTON_MENU | BUTTON_PLAY)
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)

#define KBD_MODES /* iPod uses 2 modes, picker and line edit */
#define KBD_SELECT (BUTTON_SELECT | BUTTON_REL) /* backspace in line edit */
#define KBD_SELECT_PRE BUTTON_SELECT
#define KBD_DONE (BUTTON_SELECT | BUTTON_REPEAT)
#define KBD_ABORT BUTTON_MENU

#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_SCROLL_BACK
#define KBD_DOWN BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD

/* TODO: Check keyboard mappings */

#define KBD_MODES /* iFP7xx uses 2 modes, picker and line edit */
#define KBD_SELECT (BUTTON_SELECT | BUTTON_REL) /* backspace in line edit */
#define KBD_SELECT_PRE BUTTON_SELECT
#define KBD_DONE BUTTON_MODE
#define KBD_ABORT BUTTON_PLAY
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#endif

#if KEYBOARD_PAGES == 1
static const char * const kbdpages[KEYBOARD_PAGES][KEYBOARD_LINES] = {
    {   "ABCDEFG abcdefg !?\" @#$%+'",
        "HIJKLMN hijklmn 789 &_()-`",
        "OPQRSTU opqrstu 456 §|{}/<",
        "VWXYZ., vwxyz.,0123 ~=[]*>",
        "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË ¢£¤¥¦§©®",
        "àáâãäåæ ìíîï èéêë «»°ºª¹²³",
        "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ ¯±×÷¡¿µ·",
        "òóôõöø çðþýÿ ùúûü ¼½¾¬¶¨  "  },
};

#else
static const char * const kbdpages[KEYBOARD_PAGES][KEYBOARD_LINES] = {
    {   "ABCDEFG !?\" @#$%+'",
        "HIJKLMN 789 &_()-`",
        "OPQRSTU 456 §|{}/<",
        "VWXYZ.,0123 ~=[]*>" },

    {   "abcdefg ¢£¤¥¦§©®¬",
        "hijklmn «»°ºª¹²³¶",
        "opqrstu ¯±×÷¡¿µ·¨",
        "vwxyz., ¼½¾      "  },

    {   "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË",
        "àáâãäåæ ìíîï èéêë",
        "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ",
        "òóôõöø çðþýÿ ùúûü"  },
};

#endif

#ifdef HAVE_MORSE_INPUT
/* FIXME: We should put this to a configuration file. */
static const char *morse_alphabets = 
    "abcdefghijklmnopqrstuvwxyz1234567890,.?-@ ";
static const unsigned char morse_codes[] = {
    0x05,0x18,0x1a,0x0c,0x02,0x12,0x0e,0x10,0x04,0x17,0x0d,0x14,0x07,
    0x06,0x0f,0x16,0x1d,0x0a,0x08,0x03,0x09,0x11,0x0b,0x19,0x1b,0x1c,
    0x2f,0x27,0x23,0x21,0x20,0x30,0x38,0x3c,0x3e,0x3f,
    0x73,0x55,0x4c,0x61,0x5a,0x80 };

static bool morse_mode = false;
#endif

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
#if defined(KBD_PAGE_FLIP) || (KEYBOARD_PAGES > 1)
    int page = 0;
#endif
    int font_w = 0, font_h = 0, i, j;
    int x = 0, y = 0;
    int main_x, main_y, max_chars;
    int status_y1, status_y2;
    int len, len_utf8, c = 0;
    int editpos, curpos, leftpos;
    bool redraw = true;
    unsigned char *utf8;
    const char * const *line;
#ifdef HAVE_MORSE_INPUT
    bool morse_reading = false;
    unsigned char morse_code = 0;
    int morse_tick = 0, morse_len;
    char buf[2];
#endif
#ifdef KBD_MODES
    bool line_edit = false;
#endif

    char outline[256];
    struct font* font = font_get(FONT_SYSFIXED);
    int button, lastbutton = 0;
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
#endif
    lcd_setfont(FONT_SYSFIXED);
    font_w = font->maxwidth;
    font_h = font->height;

#ifdef HAVE_MORSE_INPUT
    if (morse_mode)
        main_y = LCD_HEIGHT - font_h;
    else
#endif
        main_y = (KEYBOARD_LINES + 1) * font_h + (2*KEYBOARD_MARGIN);
    main_x = 0;
    status_y1 = LCD_HEIGHT - font_h;
    status_y2 = LCD_HEIGHT;

    editpos = utf8length(text);
    
    max_chars = LCD_WIDTH / font_w - 2; /* leave room for < and > */
    line = kbdpages[0];

    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */

    while(!done)
    {
        len = strlen(text);
        len_utf8 = utf8length(text);

        if(redraw)
        {
            lcd_clear_display();

            lcd_setfont(FONT_SYSFIXED);

#ifdef HAVE_MORSE_INPUT
            if (morse_mode)
            {
                x = 0;
                y = font_h;
                buf[1] = '\0';
                /* Draw morse code table with code descriptions. */
                for (i = 0; morse_alphabets[i] != '\0'; i++)
                {
                    buf[0] = morse_alphabets[i];
                    lcd_putsxy(x, y, buf);

                    for (j = 0; (morse_codes[i] >> j) > 0x01; j++) ;
                    morse_len = j;

                    x += font_w + 3;
                    for (j = 0; j < morse_len; j++)
                    {
                        if ((morse_codes[i] >> (morse_len-j-1)) & 0x01)
                            lcd_fillrect(x + j*4, y + 2, 3, 4);
                        else
                            lcd_fillrect(x + j*4, y + 3, 1, 2);
                    }
                    
                    x += font_w * 5 - 3;
                    if (x >= LCD_WIDTH - (font_w*6))
                    {
                        x = 0;
                        y += font_h;
                    }
                }
            }
            else
#endif
            {
                /* draw page */
                for (i=0; i < KEYBOARD_LINES; i++)
                    lcd_putsxy(0, 8+i * font_h, line[i]);
                
            }

            /* separator */
            lcd_hline(0, LCD_WIDTH - 1, main_y - KEYBOARD_MARGIN);

            /* write out the text */
            curpos = MIN(editpos, max_chars - MIN(len_utf8 - editpos, 2));
            leftpos = editpos - curpos;
            utf8 = text + utf8seek(text, leftpos);
            i=j=0;
            while (*utf8 && i < max_chars) {
                outline[j++] = *utf8++;
                if ((*utf8 & MASK) != COMP)
                    i++;
            }
            outline[j] = 0;
            
            lcd_putsxy(font_w, main_y, outline);

            if (leftpos)
                lcd_putsxy(0, main_y, "<");
            if (len_utf8 - leftpos > max_chars)
                lcd_putsxy(LCD_WIDTH - font_w, main_y, ">");
            
            /* cursor */
            i = (curpos + 1) * font_w;
            lcd_vline(i, main_y, main_y + font_h);
            
#ifdef HAS_BUTTONBAR
            /* draw the status bar */
            gui_buttonbar_set(&buttonbar, "Shift", "OK", "Del");
            gui_buttonbar_draw(&buttonbar);
#endif
            
#ifdef KBD_MODES
            if (!line_edit)
#endif
            {
                /* highlight the key that has focus */
                lcd_set_drawmode(DRMODE_COMPLEMENT);
                lcd_fillrect(font_w * x, 8 + font_h * y, font_w, font_h);
                lcd_set_drawmode(DRMODE_SOLID);
            }

            gui_syncstatusbar_draw(&statusbars, true);
            lcd_update();
        }

        /* The default action is to redraw */
        redraw = true;

        button = button_get_w_tmo(HZ/2);
#ifdef HAVE_MORSE_INPUT
        if (morse_mode)
        {
            /* Remap some buttons for morse mode. */
            if (button == KBD_LEFT || button == (KBD_LEFT | BUTTON_REPEAT))
                button = KBD_CURSOR_LEFT;
            if (button == KBD_RIGHT || button == (KBD_RIGHT | BUTTON_REPEAT))
                button = KBD_CURSOR_RIGHT;
        }
#endif
        
        switch ( button ) {

            case KBD_ABORT:
                lcd_setfont(FONT_UI);
                return -1;
                break;

#if defined(KBD_PAGE_FLIP)
            case KBD_PAGE_FLIP:
#ifdef HAVE_MORSE_INPUT
                if (morse_mode)
                {
                    main_y = (KEYBOARD_LINES + 1) * font_h + (2*KEYBOARD_MARGIN);
                    morse_mode = false;
                    x = y = 0;
                }
                else
#endif
                if (++page == KEYBOARD_PAGES)
                {
                    page = 0;
#ifdef HAVE_MORSE_INPUT
                    main_y = LCD_HEIGHT - font_h;
                    morse_mode = true;
                    /* FIXME: We should talk something like Morse mode.. */
                    break ;
#endif
                }
                line = kbdpages[page];
                c = utf8seek(line[y], x);
                kbd_spellchar(line[y][c]);
                break;
#endif

            case KBD_RIGHT:
            case KBD_RIGHT | BUTTON_REPEAT:
#ifdef HAVE_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit) /* right doubles as cursor_right in line_edit */
                {
                    if (editpos < len_utf8)
                    {
                        editpos++;
                        c = utf8seek(text, editpos);
                        kbd_spellchar(text[c]);
                    }
                }
                else
#endif
                {   
                    if (x < (int)utf8length(line[y]) - 1)
                        x++;
                    else
                    {
                        x = 0;
#if !defined(KBD_PAGE_FLIP) && KEYBOARD_PAGES > 1   
                        /* no dedicated flip key - flip page on wrap */
                        if (++page == KEYBOARD_PAGES)
                            page = 0;
                        line = kbdpages[page];
#endif
                    }
                    c = utf8seek(line[y], x);
                    kbd_spellchar(line[y][c]);
                }
                break;

            case KBD_LEFT:
            case KBD_LEFT | BUTTON_REPEAT:
#ifdef HAVE_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit) /* left doubles as cursor_left in line_edit */
                {
                    if (editpos)
                    {
                        editpos--;
                        c = utf8seek(text, editpos);
                        kbd_spellchar(text[c]);
                    }
                }
                else
#endif
                {   
                    if (x)
                        x--;
                    else
                    {
#if !defined(KBD_PAGE_FLIP) && KEYBOARD_PAGES > 1   
                        /* no dedicated flip key - flip page on wrap */
                        if (--page < 0)
                            page = (KEYBOARD_PAGES-1);
                        line = kbdpages[page];
#endif
                        x = utf8length(line[y]) - 1;
                    }
                    c = utf8seek(line[y], x);
                    kbd_spellchar(line[y][c]);
                }
                break;

            case KBD_DOWN:
            case KBD_DOWN | BUTTON_REPEAT:
#ifdef HAVE_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit)
                {
                    y = 0;
                    line_edit = false;
                }
                else
                {
#endif
                    if (y < KEYBOARD_LINES - 1)
                        y++;
                    else
#ifndef KBD_MODES
                        y=0;
#else
                        line_edit = true;
                }
                if (!line_edit)
#endif
                    c = utf8seek(line[y], x);
                    kbd_spellchar(line[y][c]);
                break;

            case KBD_UP:
            case KBD_UP | BUTTON_REPEAT:
#ifdef HAVE_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit)
                {
                    y = KEYBOARD_LINES - 1;
                    line_edit = false;
                }
                else
                {
#endif
                    if (y)
                        y--;
                    else
#ifndef KBD_MODES
                        y = KEYBOARD_LINES - 1;
#else
                        line_edit = true;
                }
                if (!line_edit)
#endif
                    c = utf8seek(line[y], x);
                    kbd_spellchar(line[y][c]);
                break;

            case KBD_DONE:
                /* accepts what was entered and continues */
#ifdef KBD_DONE_PRE
                if (lastbutton != KBD_DONE_PRE)
                    break;
#endif
                done = true;
                break;

#ifdef HAVE_MORSE_INPUT
            case KBD_SELECT | BUTTON_REL:
                if (morse_mode && morse_reading)
                {
                    morse_code <<= 1;
                    if ((current_tick - morse_tick) > HZ/5)
                        morse_code |= 0x01;
                }
                
                break;
#endif
                
            case KBD_SELECT:
#ifdef HAVE_MORSE_INPUT
                if (morse_mode)
                {
                    morse_tick = current_tick;
                    if (!morse_reading)
                    {
                        morse_reading = true;
                        morse_code = 1;
                    }
                    break;
                }
#endif
                
                /* inserts the selected char */
#ifdef KBD_SELECT_PRE
                if (lastbutton != KBD_SELECT_PRE)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit) /* select doubles as backspace in line_edit */
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
                else
#endif
                {
                    const unsigned char *inschar = line[y] + utf8seek(line[y], x);
                    j = 0;
                    do {
                        j++;
                    } while ((inschar[j] & MASK) == COMP);
                    if (len + j < buflen)
                    {
                        int k = len_utf8;
                        for (i = len+j; k >= editpos; i--) {
                            text[i] = text[i-j];
                            if ((text[i] & MASK) != COMP)
                                k--;
                        }
                        while (j--)
                            text[i--] = inschar[j];
                        editpos++;
                    }
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);   /* speak revised text */
                break;

#ifndef KBD_MODES
            case KBD_BACKSPACE:
            case KBD_BACKSPACE | BUTTON_REPEAT:
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
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);   /* speak revised text */
                break;

            case KBD_CURSOR_RIGHT:
            case KBD_CURSOR_RIGHT | BUTTON_REPEAT:
                if (editpos < len_utf8)
                {
                    editpos++;
                    c = utf8seek(text, editpos);
                    kbd_spellchar(text[c]);
                }
                break;

            case KBD_CURSOR_LEFT:
            case KBD_CURSOR_LEFT | BUTTON_REPEAT:
                if (editpos)
                {
                    editpos--;
                    c = utf8seek(text, editpos);
                    kbd_spellchar(text[c]);
                }
                break;
#endif /* !KBD_MODES */

            case BUTTON_NONE:
                gui_syncstatusbar_draw(&statusbars, false);
                redraw = false;
#ifdef HAVE_MORSE_INPUT
                if (morse_reading)
                {
                    logf("Morse: 0x%02x", morse_code);
                    morse_reading = false;

                    for (j = 0; morse_alphabets[j] != '\0'; j++)
                    {
                        if (morse_codes[j] == morse_code)
                            break ;
                    }

                    if (morse_alphabets[j] == '\0')
                    {
                        logf("Morse code not found");
                        break ;
                    }
                    
                    if (len + 1 < buflen)
                    {
                        for (i = len ; i > editpos; i--)
                            text[i] = text[i-1];
                        text[len+1] = 0;
                        text[editpos] = morse_alphabets[j];
                        editpos++;
                    }
                    if (global_settings.talk_menu) /* voice UI? */
                        talk_spell(text, false);   /* speak revised text */
                    redraw = true;
                }
#endif

                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    lcd_setfont(FONT_SYSFIXED);
                break;

        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
    lcd_setfont(FONT_UI);

    return 0;
}
