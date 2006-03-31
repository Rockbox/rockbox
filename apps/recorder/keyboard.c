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
#include "icons.h"
#include "file.h"
#include "hangul.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if CONFIG_KEYPAD == RECORDER_PAD
#define BUTTONBAR_HEIGHT 8
#else
#define BUTTONBAR_HEIGHT 0
#endif

#if (LCD_WIDTH >= 160) && (LCD_HEIGHT >= 96)
#define DEFAULT_LINES 8
#else
#define DEFAULT_LINES 4
#endif

#define DEFAULT_MARGIN 6
#define KBD_BUF_SIZE 500

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
#define KBD_MORSE_INPUT (BUTTON_ON | BUTTON_MODE)

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

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)

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

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD

/* TODO: Check keyboard mappings */

#define KBD_MODES /* iAudio X5 uses 2 modes, picker and line edit */
#define KBD_SELECT (BUTTON_SELECT | BUTTON_REL) /* backspace in line edit */
#define KBD_SELECT_PRE BUTTON_SELECT
#define KBD_DONE BUTTON_PLAY
#define KBD_ABORT BUTTON_REC
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD

/* TODO: Check keyboard mappings */

#define KBD_MODES /* iAudio X5 uses 2 modes, picker and line edit */
#define KBD_SELECT (BUTTON_MENU | BUTTON_REL) /* backspace in line edit */
#define KBD_SELECT_PRE BUTTON_MENU
#define KBD_DONE BUTTON_PLAY
#define KBD_ABORT BUTTON_REC
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == GIGABEAT_PAD

/* TODO: Check keyboard mappings */

#define KBD_MODES /* Gigabeat uses 2 modes, picker and line edit */
#define KBD_SELECT (BUTTON_MENU | BUTTON_REL) /* backspace in line edit */
#define KBD_SELECT_PRE BUTTON_MENU
#define KBD_DONE BUTTON_POWER
#define KBD_ABORT BUTTON_A
#define KBD_LEFT BUTTON_LEFT
#define KBD_RIGHT BUTTON_RIGHT
#define KBD_UP BUTTON_UP
#define KBD_DOWN BUTTON_DOWN

#endif

#if (LCD_WIDTH >= 160) && (LCD_HEIGHT >= 96)
static const unsigned char * default_kbd = 
    "ABCDEFG abcdefg !?\" @#$%+'\n"
    "HIJKLMN hijklmn 789 &_()-`\n"
    "OPQRSTU opqrstu 456 §|{}/<\n"
    "VWXYZ., vwxyz.,0123 ~=[]*>\n"
    "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË ¢£¤¥¦§©®\n"
    "àáâãäåæ ìíîï èéêë «»°ºª¹²³\n"
    "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ ¯±×÷¡¿µ·\n"
    "òóôõöø çðþýÿ ùúûü ¼½¾¬¶¨";
#else
static const unsigned char * default_kbd = 
    "ABCDEFG !?\" @#$%+'\n"
    "HIJKLMN 789 &_()-`\n"
    "OPQRSTU 456 §|{}/<\n"
    "VWXYZ.,0123 ~=[]*>\n"

    "abcdefg ¢£¤¥¦§©®¬\n"
    "hijklmn «»°ºª¹²³¶\n"
    "opqrstu ¯±×÷¡¿µ·¨\n"
    "vwxyz., ¼½¾      \n"

    "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË\n"
    "àáâãäåæ ìíîï èéêë\n"
    "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ\n"
    "òóôõöø çðþýÿ ùúûü";
#endif

static unsigned short kbd_buf[KBD_BUF_SIZE];
static bool kbd_loaded = false;
static int nchars = 0;

#ifdef KBD_MORSE_INPUT
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

/* Loads a custom keyboard into memory
   call with NULL to reset keyboard    */
int load_kbd(unsigned char* filename)
{
    int fd, count;
    int i = 0;
    unsigned char buf[4];

    if (filename == NULL) {
        kbd_loaded = false;
        return 0;
    }

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
        return 1;

    while (read(fd, buf, 1) == 1 && i < KBD_BUF_SIZE) {
        /* check how many bytes to read */
        if (buf[0] < 0x80) {
            count = 0;
        } else if (buf[0] < 0xe0) {
            count = 1;
        } else if (buf[0] < 0xf0) {
            count = 2;
        } else if (buf[0] < 0xf5) {
            count = 3;
        } else {
            /* Invalid size. */
            continue;
        }

        if (read(fd, &buf[1], count) != count) {
            close(fd);
            kbd_loaded = false;
            return 1;
        }

        utf8decode(buf, &kbd_buf[i]);
        if (kbd_buf[i] != 0xFEFF && kbd_buf[i] != '\n' && 
            kbd_buf[i] != '\r') /*skip BOM & newlines */
            i++;
    }

    close(fd);
    kbd_loaded = true;
    nchars = i;
    return 0;

}

/* helper function to spell a char if voice UI is enabled */
static void kbd_spellchar(unsigned short c)
{
    static char spell_char[2] = "\0\0"; /* store char to pass to talk_spell */

    if (global_settings.talk_menu && c < 128) /* voice UI? */
    {
        spell_char[0] = (char)c;
        talk_spell(spell_char, false);
    }
}

void kbd_inschar(unsigned char* text, int buflen, int* editpos, unsigned short ch)
{
    int i, j, k, len;
    unsigned char tmp[4];
    unsigned char* utf8;

    len = strlen(text);
    k = utf8length(text);
    utf8 = utf8encode(ch, tmp);
    j = (long)utf8 - (long)tmp;

    if (len + j < buflen)
        {
        for (i = len+j; k >= *editpos; i--) {
            text[i] = text[i-j];
            if ((text[i] & MASK) != COMP)
                k--;
        }
        while (j--)
            text[i--] = tmp[j];
        (*editpos)++;
    }
    return;
}

void kbd_delchar(unsigned char* text, int* editpos)
{
    int i = 0;
    unsigned char* utf8;

    if (*editpos > 0)
    {
        utf8 = text + utf8seek(text, *editpos);
        do {
            i++;
            utf8--;
        } while ((*utf8 & MASK) == COMP);
        while (utf8[i]) {
            *utf8 = utf8[i];
            utf8++;
        }
        *utf8 = 0;
        (*editpos)--;
    }

    return;
}

int kbd_input(char* text, int buflen)
{
    bool done = false;
    int page = 0;
    int font_w = 0, font_h = 0, text_w = 0;
    int i = 0, j, k, w;
    int x = 0, y = 0;
    int main_x, main_y, max_chars, max_chars_text;
    int len_utf8, c = 0;
    int editpos, curpos, leftpos;
    int lines, pages, keyboard_margin;
    int curfont;
    int statusbar_size = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
    unsigned short ch, tmp, hlead = 0, hvowel = 0, htail = 0;
    bool hangul = false;
    bool redraw = true;
    unsigned char *utf8;
    const unsigned char *p;
#ifdef KBD_MORSE_INPUT
    bool morse_reading = false;
    unsigned char morse_code = 0;
    int morse_tick = 0, morse_len, old_main_y;
    char buf[2];
#endif
#ifdef KBD_MODES
    bool line_edit = false;
#endif

    char outline[256];
    struct font* font;
    int button, lastbutton = 0;
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    bool buttonbar_config = global_settings.buttonbar;
    global_settings.buttonbar = true;
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
#endif

    if (!kbd_loaded) {
        curfont = FONT_SYSFIXED;
        p = default_kbd;
        while (*p != 0) {
            p = utf8decode(p, &kbd_buf[i]);
            if (kbd_buf[i] == '\n')
                while (i % (LCD_WIDTH/6))
                    kbd_buf[i++] = ' ';
            else
                i++;
        }
        nchars = i;
    }
    else
        curfont = FONT_UI;

    font = font_get(curfont);
    font_h = font->height;

    /* check if FONT_UI fits the screen */
    if (2*font_h+3+statusbar_size + BUTTONBAR_HEIGHT > LCD_HEIGHT) {
        font = font_get(FONT_SYSFIXED);
        font_h = font->height;
        curfont = FONT_SYSFIXED;
    }

    lcd_setfont(curfont);

    /* find max width of keyboard glyphs */
    for (i=0; i<nchars; i++) {
        w = font_get_width(font, kbd_buf[i]);
        if (w > font_w)
            font_w = w;
    }

    /* find max width for text string */
    utf8 = text;
    text_w = font_w;
    while (*utf8) {
        utf8 = (unsigned char*)utf8decode(utf8, &ch);
        w = font_get_width(font, ch);
        if (w > text_w)
            text_w = w;
    }
    max_chars_text = LCD_WIDTH / text_w - 2;

    /* calculate keyboard grid size */
    max_chars = LCD_WIDTH / font_w;
    if (!kbd_loaded) {
        lines = DEFAULT_LINES;
        keyboard_margin = DEFAULT_MARGIN;
    } else {
        lines = (LCD_HEIGHT - BUTTONBAR_HEIGHT - statusbar_size) / font_h - 1;
        keyboard_margin = LCD_HEIGHT - BUTTONBAR_HEIGHT - statusbar_size - (lines+1)*font_h;
        if (keyboard_margin < 3) {
            lines--;
            keyboard_margin += font_h;
        }
        if (keyboard_margin > 6)
            keyboard_margin = 6;
    }

    pages = (nchars + (lines*max_chars-1))/(lines*max_chars);
    if (pages == 1 && kbd_loaded)
        lines = (nchars + max_chars - 1) / max_chars;

    main_y = font_h*lines + keyboard_margin + statusbar_size;
    main_x = 0;
    keyboard_margin -= keyboard_margin/2;

#ifdef KBD_MORSE_INPUT
    old_main_y = main_y;
    if (morse_mode)
        main_y = LCD_HEIGHT - font_h;
#endif

    editpos = utf8length(text);

    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */

    while(!done)
    {
        len_utf8 = utf8length(text);

        if(redraw)
        {
            lcd_clear_display();

#ifdef KBD_MORSE_INPUT
            if (morse_mode)
            {
                lcd_setfont(FONT_SYSFIXED); /* Draw morse code screen with sysfont */
                w = 6; /* sysfixed font width */
                x = 0;
                y = statusbar_size;
                buf[1] = '\0';
                /* Draw morse code table with code descriptions. */
                for (i = 0; morse_alphabets[i] != '\0'; i++)
                {
                    buf[0] = morse_alphabets[i];
                    lcd_putsxy(x, y, buf);

                    for (j = 0; (morse_codes[i] >> j) > 0x01; j++) ;
                    morse_len = j;

                    x += w + 3;
                    for (j = 0; j < morse_len; j++)
                    {
                        if ((morse_codes[i] >> (morse_len-j-1)) & 0x01)
                            lcd_fillrect(x + j*4, y + 2, 3, 4);
                        else
                            lcd_fillrect(x + j*4, y + 3, 1, 2);
                    }
                    
                    x += w * 5 - 3;
                    if (x >= LCD_WIDTH - (w*6))
                    {
                        x = 0;
                        y += 8; /* sysfixed font height */
                    }
                }
            }
            else
#endif
            {
                /* draw page */
                lcd_setfont(curfont);
                k = page*max_chars*lines;
                for (i=j=0; j < lines && k < nchars; k++) {
                    utf8 = utf8encode(kbd_buf[k], outline);
                    *utf8 = 0;
                    lcd_getstringsize(outline, &w, NULL);
                    lcd_putsxy(i*font_w + (font_w-w)/2, j*font_h + statusbar_size, outline);
                    if (++i == max_chars) {
                        i = 0;
                        j++;
                    }
                }
            }

            /* separator */
            lcd_hline(0, LCD_WIDTH - 1, main_y - keyboard_margin);

            /* write out the text */
            lcd_setfont(curfont);
            i=j=0;
            curpos = MIN(editpos, max_chars_text - MIN(len_utf8 - editpos, 2));
            leftpos = editpos - curpos;
            utf8 = text + utf8seek(text, leftpos);

            while (*utf8 && i < max_chars_text) {
                outline[j++] = *utf8++;
                if ((*utf8 & MASK) != COMP) {
                    outline[j] = 0;
                    j=0;
                    i++;
                    lcd_getstringsize(outline, &w, NULL);
                    lcd_putsxy(i*text_w + (text_w-w)/2, main_y, outline);
                }
            }

            if (leftpos) {
                lcd_getstringsize("<", &w, NULL);
                lcd_putsxy(text_w - w, main_y, "<");
            }
            if (len_utf8 - leftpos > max_chars_text)
                lcd_putsxy(LCD_WIDTH - text_w, main_y, ">");

            /* cursor */
            i = (curpos + 1) * text_w;
            lcd_vline(i, main_y, main_y + font_h);
            if (hangul) /* draw underbar */
                lcd_hline(curpos*text_w, (curpos+1)*text_w, main_y+font_h-1);

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
                lcd_fillrect(font_w * x, statusbar_size + font_h * y, font_w, font_h);
                lcd_set_drawmode(DRMODE_SOLID);
            }

            gui_syncstatusbar_draw(&statusbars, true);
            lcd_update();
        }

        /* The default action is to redraw */
        redraw = true;

        button = button_get_w_tmo(HZ/2);
#ifdef KBD_MORSE_INPUT
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
#ifdef HAS_BUTTONBAR
                global_settings.buttonbar=buttonbar_config;
#endif
                return -1;
                break;

#if defined(KBD_PAGE_FLIP)
            case KBD_PAGE_FLIP:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
                if (++page == pages)
                    page = 0;
                k = (page*lines + y)*max_chars + x;
                kbd_spellchar(kbd_buf[k]);
                break;
#endif

#ifdef KBD_MORSE_INPUT
            case KBD_MORSE_INPUT:
                morse_mode = !morse_mode;
                x = y = page = 0;
                if (morse_mode) {
                    old_main_y = main_y;
                    main_y = LCD_HEIGHT - font_h;
                } else
                    main_y = old_main_y;
                /* FIXME: We should talk something like Morse mode.. */
                break;
#endif
            case KBD_RIGHT:
            case KBD_RIGHT | BUTTON_REPEAT:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit) /* right doubles as cursor_right in line_edit */
                {
                    if (hangul)
                        hangul = false;
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
                    if (++x == max_chars) {
                        x = 0;
#if !defined(KBD_PAGE_FLIP)
                        /* no dedicated flip key - flip page on wrap */
                        if (++page == pages)
                            page = 0;
#endif
                    }
                k = (page*lines + y)*max_chars + x;
                kbd_spellchar(kbd_buf[k]);
                }
                break;

            case KBD_LEFT:
            case KBD_LEFT | BUTTON_REPEAT:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit) /* left doubles as cursor_left in line_edit */
                {
                    if (hangul)
                        hangul = false;
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
#if !defined(KBD_PAGE_FLIP)
                        /* no dedicated flip key - flip page on wrap */
                        if (--page < 0)
                            page = (pages-1);
#endif
                        x = max_chars - 1;
                    }
                k = (page*lines + y)*max_chars + x;
                kbd_spellchar(kbd_buf[k]);
                }
                break;

            case KBD_DOWN:
            case KBD_DOWN | BUTTON_REPEAT:
#ifdef KBD_MORSE_INPUT
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
                    if (y < lines - 1)
                        y++;
                    else
#ifndef KBD_MODES
                        y=0;
#else
                        line_edit = true;
                }
                if (!line_edit)
#endif
                {
                    k = (page*lines + y)*max_chars + x;
                    kbd_spellchar(kbd_buf[k]);
                }
                break;

            case KBD_UP:
            case KBD_UP | BUTTON_REPEAT:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit)
                {
                    y = lines - 1;
                    line_edit = false;
                }
                else
                {
#endif
                    if (y)
                        y--;
                    else
#ifndef KBD_MODES
                        y = lines - 1;
#else
                        line_edit = true;
                }
                if (!line_edit)
#endif
                {
                    k = (page*lines + y)*max_chars + x;
                    kbd_spellchar(kbd_buf[k]);
                }
                break;

            case KBD_DONE:
                /* accepts what was entered and continues */
#ifdef KBD_DONE_PRE
                if (lastbutton != KBD_DONE_PRE)
                    break;
#endif
                done = true;
                break;

#ifdef KBD_MORSE_INPUT
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
#ifdef KBD_MORSE_INPUT
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
                if (line_edit) { /* select doubles as backspace in line_edit */
                    if (hangul) {
                        if (htail)
                            htail = 0;
                        else if (hvowel)
                            hvowel = 0;
                        else
                            hangul = false;
                    }
                    kbd_delchar(text, &editpos);
                    if (hangul) {
                        if (hvowel)
                            ch = hangul_join(hlead, hvowel, htail);
                        else
                            ch = hlead;
                        kbd_inschar(text, buflen, &editpos, ch);
                    }
                }
                else
#endif
                {
                    /* find input char */
                    k = (page*lines + y)*max_chars + x;
                    if (k < nchars)
                        ch = kbd_buf[k];
                    else
                        ch = ' ';

                    /* check for hangul input */
                    if (ch >= 0x3131 && ch <= 0x3163) {
                        if (!hangul) {
                            hlead=hvowel=htail=0;
                            hangul = true;
                        }
                        if (!hvowel)
                            hvowel = ch;
                        else if (!htail)
                            htail = ch;
                        else { /* previous hangul complete */
                            /* check whether tail is actually lead of next char */
                            if ((tmp = hangul_join(htail, ch, 0)) != 0xfffd) {
                                tmp = hangul_join(hlead, hvowel, 0);
                                kbd_delchar(text, &editpos);
                                kbd_inschar(text, buflen, &editpos, tmp);
                                /* insert dummy char */
                                kbd_inschar(text, buflen, &editpos, ' ');
                                hlead = htail;
                                hvowel = ch;
                                htail = 0;
                            } else {
                                hvowel=htail=0;
                                hlead = ch;
                            }
                        }
                        /* combine into hangul */
                        if ((tmp = hangul_join(hlead, hvowel, htail)) != 0xfffd) {
                            kbd_delchar(text, &editpos);
                            ch = tmp;
                        } else {
                            hvowel=htail=0;
                            hlead = ch;
                        }
                    } else {
                        hangul = false;
                    }
                    /* insert char */
                    kbd_inschar(text, buflen, &editpos, ch);
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);   /* speak revised text */
                break;

#ifndef KBD_MODES
            case KBD_BACKSPACE:
            case KBD_BACKSPACE | BUTTON_REPEAT:
                if (hangul) {
                    if (htail)
                        htail = 0;
                    else if (hvowel)
                        hvowel = 0;
                    else
                        hangul = false;
                }
                kbd_delchar(text, &editpos);
                if (hangul) {
                    if (hvowel)
                        ch = hangul_join(hlead, hvowel, htail);
                    else
                        ch = hlead;
                    kbd_inschar(text, buflen, &editpos, ch);
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);   /* speak revised text */
                break;

            case KBD_CURSOR_RIGHT:
            case KBD_CURSOR_RIGHT | BUTTON_REPEAT:
                if (hangul)
                    hangul = false;
                if (editpos < len_utf8)
                {
                    editpos++;
                    c = utf8seek(text, editpos);
                    kbd_spellchar(text[c]);
                }
                break;

            case KBD_CURSOR_LEFT:
            case KBD_CURSOR_LEFT | BUTTON_REPEAT:
                if (hangul)
                    hangul = false;
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
#ifdef KBD_MORSE_INPUT
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

                    /* turn off hangul input */
                    if (hangul)
                        hangul = false;

                    kbd_inschar(text, buflen, &editpos, morse_alphabets[j]);

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
#ifdef HAS_BUTTONBAR
    global_settings.buttonbar=buttonbar_config;
#endif
    lcd_setfont(FONT_UI);

    return 0;
}
