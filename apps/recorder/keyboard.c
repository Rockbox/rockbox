/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Bj�n Stenberg
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
#include "action.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if CONFIG_KEYPAD == RECORDER_PAD
#define BUTTONBAR_HEIGHT 8
#else
#define BUTTONBAR_HEIGHT 0
#endif

#define DEFAULT_MARGIN 6
#define KBD_BUF_SIZE 500

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define KBD_MORSE_INPUT /* I-Rivers have a Morse input mode */

#elif CONFIG_KEYPAD == ONDIO_PAD /* restricted Ondio keypad */
#define KBD_MODES /* Ondio uses 2 modes, picker and line edit */

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define KBD_MODES /* iPod uses 2 modes, picker and line edit */

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define KBD_MODES /* iFP7xx uses 2 modes, picker and line edit */

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define KBD_MODES /* iAudio X5 uses 2 modes, picker and line edit */

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define KBD_MODES /* iriver H10 uses 2 modes, picker and line edit */
#endif

struct keyboard_parameters {
    const unsigned char* default_kbd;
    int DEFAULT_LINES;
    unsigned short kbd_buf[KBD_BUF_SIZE];
    int nchars;
    int font_w;
    int font_h;
    struct font* font;
    int curfont;
    int main_x;
    int main_y;
    int max_chars;
    int max_chars_text;
    int lines;
    int pages;
    int keyboard_margin;
    int old_main_y;
    int curpos;
    int leftpos;
    int page;
    int x;
    int y;
};

struct keyboard_parameters param[NB_SCREENS];
static bool kbd_loaded = false;

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
    int fd, count, l;
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

        FOR_NB_SCREENS(l)
            utf8decode(buf, &param[l].kbd_buf[i]);

        if (param[0].kbd_buf[i] != 0xFEFF &&
            param[0].kbd_buf[i] != '\r') /*skip BOM & carriage returns */
            i++;
    }

    close(fd);
    kbd_loaded = true;
    FOR_NB_SCREENS(l)
        param[l].nchars = i;
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
    int i, j, k, w, l;
    int text_w = 0;
    int len_utf8, c = 0;
    int editpos;
    int statusbar_size = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
    unsigned short ch, tmp, hlead = 0, hvowel = 0, htail = 0;
    bool hangul = false;
    unsigned char *utf8;
    const unsigned char *p;
    bool cur_blink = true;
#ifdef KBD_MORSE_INPUT
    bool morse_reading = false;
    unsigned char morse_code = 0;
    int morse_tick = 0, morse_len;
    char buf[2];
#endif
    int char_screen = 0;

    FOR_NB_SCREENS(l)
    {
        if ((screens[l].width >= 160) && (screens[l].height >= 96))
        {
             param[l].default_kbd =
                      "ABCDEFG abcdefg !?\" @#$%+'\n"
                      "HIJKLMN hijklmn 789 &_()-`\n"
                      "OPQRSTU opqrstu 456 §|{}/<\n"
                      "VWXYZ., vwxyz.,0123 ~=[]*>\n"
                      "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË ¢£¤¥¦§©®\n"
                      "àáâãäåæ ìíîï èéêë «»°ºª¹²³\n"
                      "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ ¯±×÷¡¿µ·\n"
                      "òóôõöø çðþýÿ ùúûü ¼½¾¬¶¨";

             param[l].DEFAULT_LINES = 8;
        }
        else
        {
             param[l].default_kbd =
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

              param[l].DEFAULT_LINES = 4;
         }
    }
#ifdef KBD_MODES
    bool line_edit = false;
#endif

    char outline[256];
    int button;
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    bool buttonbar_config = global_settings.buttonbar;
    global_settings.buttonbar = true;
    gui_buttonbar_init(&buttonbar);
    FOR_NB_SCREENS(l)
        gui_buttonbar_set_display(&buttonbar, &(screens[l]) );
#endif
    FOR_NB_SCREENS(l)
    {
        if( !kbd_loaded )
        {
            /* Copy default keyboard to buffer */
            i = 0;
            param[l].curfont = FONT_SYSFIXED;
            p = param[l].default_kbd;
            while (*p != 0)
                p = utf8decode(p, &param[l].kbd_buf[i++]);
            param[l].nchars = i;
        }
        else
            param[l].curfont = FONT_UI;
    }
    FOR_NB_SCREENS(l)
    {
        param[l].font = font_get(param[l].curfont);
        param[l].font_h = param[l].font->height;

            /* check if FONT_UI fits the screen */
        if (2*param[l].font_h+3+statusbar_size + BUTTONBAR_HEIGHT > screens[l].height) {
            param[l].font = font_get(FONT_SYSFIXED);
            param[l].font_h = param[l].font->height;
            param[l].curfont = FONT_SYSFIXED;
        }

        screens[l].setfont(param[l].curfont);
        /* find max width of keyboard glyphs */
        for (i=0; i<param[l].nchars; i++) {
            w = font_get_width(param[l].font, param[l].kbd_buf[i]);
            if (w > param[l].font_w)
                param[l].font_w = w;
        }
        /* Since we're going to be adding spaces, make sure that we check
         * their width too */
        w = font_get_width( param[l].font, ' ' );
        if( w > param[l].font_w )
            param[l].font_w = w;
    }
    FOR_NB_SCREENS(l)
    {
        i = 0;
        /* Pad lines with spaces */
        while( i < param[l].nchars )
        {
            if( param[l].kbd_buf[i] == '\n' )
            {
                k = ( screens[l].width / param[l].font_w )
                        - i % ( screens[l].width / param[l].font_w ) - 1;
                if( k == ( screens[l].width / param[l].font_w ) - 1 )
                {
                    param[l].nchars--;
                    for( j = i; j < param[l].nchars; j++ )
                    {
                        param[l].kbd_buf[j] = param[l].kbd_buf[j+1];
                    }
                }
                else
                {
                    if( param[l].nchars + k - 1 >= KBD_BUF_SIZE )
                    {   /* We don't want to overflow the buffer */
                        k = KBD_BUF_SIZE - param[l].nchars;
                    }
                    for( j = param[l].nchars + k - 1; j > i+k; j-- )
                    {
                        param[l].kbd_buf[j] = param[l].kbd_buf[j-k];
                    }
                    param[l].nchars += k;
                    k++;
                    while( k-- )
                    {
                        param[l].kbd_buf[i++] = ' ';
                    }
                }
            }
            else
                i++;
        }
    }

    /* find max width for text string */
    utf8 = text;
    FOR_NB_SCREENS(l)
    {
        text_w = param[l].font_w;
        while (*utf8) {
            utf8 = (unsigned char*)utf8decode(utf8, &ch);
            w = font_get_width(param[l].font, ch);
            if (w > text_w)
                text_w = w;
        }
        param[l].max_chars_text = screens[l].width / text_w - 2;

    /* calculate keyboard grid size */
        param[l].max_chars = screens[l].width / param[l].font_w;
        if (!kbd_loaded) {
            param[l].lines = param[l].DEFAULT_LINES;
            param[l].keyboard_margin = DEFAULT_MARGIN;
        } else {
            param[l].lines = (screens[l].height - BUTTONBAR_HEIGHT - statusbar_size) / param[l].font_h - 1;
            param[l].keyboard_margin = screens[l].height - BUTTONBAR_HEIGHT -
                statusbar_size - (param[l].lines+1)*param[l].font_h;
            if (param[l].keyboard_margin < 3) {
                param[l].lines--;
                param[l].keyboard_margin += param[l].font_h;
            }
            if (param[l].keyboard_margin > 6)
                param[l].keyboard_margin = 6;
        }

        param[l].pages = (param[l].nchars + (param[l].lines*param[l].max_chars-1))
             /(param[l].lines*param[l].max_chars);
        if (param[l].pages == 1 && kbd_loaded)
            param[l].lines = (param[l].nchars + param[l].max_chars - 1) / param[l].max_chars;

        param[l].main_y = param[l].font_h*param[l].lines + param[l].keyboard_margin + statusbar_size;
        param[l].main_x = 0;
        param[l].keyboard_margin -= param[l].keyboard_margin/2;

#ifdef KBD_MORSE_INPUT
        param[l].old_main_y = param[l].main_y;
        if (morse_mode)
            param[l].main_y = screens[l].height - param[l].font_h;
#endif
    }
    editpos = utf8length(text);


    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */

    action_signalscreenchange();
    while(!done)
    {
        len_utf8 = utf8length(text);
        FOR_NB_SCREENS(l)
            screens[l].clear_display();

#ifdef KBD_MORSE_INPUT
        if (morse_mode)
        {
            FOR_NB_SCREENS(l)
            {
                screens[l].setfont(FONT_SYSFIXED); /* Draw morse code screen with sysfont */
                w = 6; /* sysfixed font width */
                param[l].x = 0;
                param[l].y = statusbar_size;
                buf[1] = '\0';
                /* Draw morse code table with code descriptions. */
                for (i = 0; morse_alphabets[i] != '\0'; i++)
                {
                    buf[0] = morse_alphabets[i];
                    screens[l].putsxy(param[l].x, param[l].y, buf);

                    for (j = 0; (morse_codes[i] >> j) > 0x01; j++) ;
                    morse_len = j;

                    param[l].x += w + 3;
                    for (j = 0; j < morse_len; j++)
                    {
                        if ((morse_codes[i] >> (morse_len-j-1)) & 0x01)
                            screens[l].fillrect(param[l].x + j*4, param[l].y + 2, 3, 4);
                        else
                            screens[l].fillrect(param[l].x + j*4, param[l].y + 3, 1, 2);
                    }

                    param[l].x += w * 5 - 3;
                    if (param[l].x >= screens[l].width - (w*6))
                    {
                        param[l].x = 0;
                        param[l].y += 8; /* sysfixed font height */
                    }
                }
            }
        }
        else
#endif
        {
            /* draw page */
            FOR_NB_SCREENS(l)
            {
                screens[l].setfont(param[l].curfont);
                k = param[l].page*param[l].max_chars*param[l].lines;
                for (i=j=0; j < param[l].lines && k < param[l].nchars; k++) {
                    utf8 = utf8encode(param[l].kbd_buf[k], outline);
                    *utf8 = 0;
                    screens[l].getstringsize(outline, &w, NULL);
                    screens[l].putsxy(i*param[l].font_w + (param[l].font_w-w)/2, j*param[l].font_h
                          + statusbar_size, outline);
                    if (++i == param[l].max_chars) {
                        i = 0;
                        j++;
                    }
                }
            }
        }

        /* separator */
        FOR_NB_SCREENS(l)
        {
            screens[l].hline(0, screens[l].width - 1, param[l].main_y - param[l].keyboard_margin);

        /* write out the text */
            screens[l].setfont(param[l].curfont);

            i=j=0;
            param[l].curpos = MIN(editpos, param[l].max_chars_text
                                - MIN(len_utf8 - editpos, 2));
            param[l].leftpos = editpos - param[l].curpos;
            utf8 = text + utf8seek(text, param[l].leftpos);

            text_w = param[l].font_w;
            while (*utf8 && i < param[l].max_chars_text) {
                outline[j++] = *utf8++;
                if ((*utf8 & MASK) != COMP) {
                    outline[j] = 0;
                    j=0;
                    i++;
                    screens[l].getstringsize(outline, &w, NULL);
                    screens[l].putsxy(i*text_w + (text_w-w)/2, param[l].main_y, outline);
                }
            }

            if (param[l].leftpos) {
                screens[l].getstringsize("<", &w, NULL);
                screens[l].putsxy(text_w - w, param[l].main_y, "<");
            }
            if (len_utf8 - param[l].leftpos > param[l].max_chars_text)
                screens[l].putsxy(screens[l].width - text_w, param[l].main_y, ">");

            /* cursor */
            i = (param[l].curpos + 1) * text_w;
            if (cur_blink)
                screens[l].vline(i, param[l].main_y, param[l].main_y + param[l].font_h-1);

            if (hangul) /* draw underbar */
                screens[l].hline(param[l].curpos*text_w, (param[l].curpos+1)*text_w,
                                       param[l].main_y+param[l].font_h-1);
        }
        cur_blink = !cur_blink;
#ifdef HAS_BUTTONBAR
        /* draw the button bar */
        gui_buttonbar_set(&buttonbar, "Shift", "OK", "Del");
        gui_buttonbar_draw(&buttonbar);
#endif

#ifdef KBD_MODES
        if (!line_edit)
#endif
        {
            /* highlight the key that has focus */
            FOR_NB_SCREENS(l)
            {
                screens[l].set_drawmode(DRMODE_COMPLEMENT);
                screens[l].fillrect(param[l].font_w * param[l].x,
                                        statusbar_size + param[l].font_h * param[l].y,
                                        param[l].font_w, param[l].font_h);
                screens[l].set_drawmode(DRMODE_SOLID);
            }
        }
#ifdef KBD_MODES
        else
        {
            FOR_NB_SCREENS(l)
            {
                screens[l].set_drawmode(DRMODE_COMPLEMENT);
                screens[l].fillrect(0, param[l].main_y - param[l].keyboard_margin + 2,
                                         screens[l].width, param[l].font_h+2);
                screens[l].set_drawmode(DRMODE_SOLID);
            }
        }
#endif

        gui_syncstatusbar_draw(&statusbars, true);
        FOR_NB_SCREENS(l)
        screens[l].update();

        button = get_action(CONTEXT_KEYBOARD,HZ/2);
#ifdef KBD_MORSE_INPUT
        if (morse_mode)
        {
            /* Remap some buttons for morse mode. */
            if (button == ACTION_KBD_LEFT)
                button = ACTION_KBD_CURSOR_LEFT;
            if (button == ACTION_KBD_RIGHT)
                button = ACTION_KBD_CURSOR_RIGHT;
        }
#endif

        switch ( button ) {

            case ACTION_KBD_ABORT:
                FOR_NB_SCREENS(l)
                    screens[l].setfont(FONT_UI);

#ifdef HAS_BUTTONBAR
                global_settings.buttonbar=buttonbar_config;
#endif
                return -1;
                break;

            case ACTION_KBD_PAGE_FLIP:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
                FOR_NB_SCREENS(l)
                {
                    if (++param[l].page == param[l].pages)
                         param[l].page = 0;
                    k = (param[l].page*param[l].lines +
                          param[l].y)*param[l].max_chars + param[l].x;
                    kbd_spellchar(param[l].kbd_buf[k]);
                }
                break;

#ifdef KBD_MORSE_INPUT
            case ACTION_KBD_MORSE_INPUT:

                morse_mode = !morse_mode;
                FOR_NB_SCREENS(l)
                {
                    param[l].x = param[l].y = param[l].page = 0;
                    if (morse_mode) {
                        param[l].old_main_y = param[l].main_y;
                        param[l].main_y = screens[l].height - param[l].font_h;
                    } else
                        param[l].main_y = param[l].old_main_y;
                }
                /* FIXME: We should talk something like Morse mode.. */
                break;
#endif

            case ACTION_KBD_RIGHT:
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
                    FOR_NB_SCREENS(l)
                    {
                        if (++param[l].x == param[l].max_chars) {
                            param[l].x = 0;
#if !defined(KBD_PAGE_FLIP)
                        /* no dedicated flip key - flip page on wrap */
                            if (++param[l].page == param[l].pages)
                                param[l].page = 0;
#endif
                        }
                        k = (param[l].page*param[l].lines + param[l].y)*param[l].max_chars + param[l].x;
                        kbd_spellchar(param[l].kbd_buf[k]);
                    }
                }
                break;

            case ACTION_KBD_LEFT:
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
                    FOR_NB_SCREENS(l)
                    {
                        if (param[l].x)
                            param[l].x--;
                        else
                        {
#if !defined(KBD_PAGE_FLIP)
                        /* no dedicated flip key - flip page on wrap */
                            if (--param[l].page < 0)
                                param[l].page = (param[l].pages-1);
#endif
                            param[l].x = param[l].max_chars - 1;
                        }
                        k = (param[l].page*param[l].lines +
                            param[l].y)*param[l].max_chars + param[l].x;
                        kbd_spellchar(param[l].kbd_buf[k]);
                    }
                }
                break;

            case ACTION_KBD_DOWN:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit)
                {
                    FOR_NB_SCREENS(l)
                        param[l].y=0;
                    line_edit = false;
                }
                else
                {
#endif
                    FOR_NB_SCREENS(l)
                    {
                        if (param[l].y < param[l].lines - 1)
                            param[l].y++;
                        else
#ifndef KBD_MODES
                            param[l].y=0;}
#else
                            line_edit = true;
                    }
                }
                if (!line_edit)
#endif
                {
                    FOR_NB_SCREENS(l)
                    {
                        k = (param[l].page*param[l].lines + param[l].y)*
                                 param[l].max_chars + param[l].x;
                        kbd_spellchar(param[l].kbd_buf[k]);
                    }
                }
                break;

            case ACTION_KBD_UP:
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
#ifdef KBD_MODES
                if (line_edit)
                {
                    FOR_NB_SCREENS(l)
                        param[l].y = param[l].lines - 1;
                    line_edit = false;
                }
                else
                {
#endif
                    FOR_NB_SCREENS(l)
                    {
                        if (param[l].y)
                            param[l].y--;
                        else
#ifndef KBD_MODES
                            param[l].y = param[l].lines - 1;}
#else
                        line_edit = true;
                    }
                }
                if (!line_edit)
#endif
                {
                    FOR_NB_SCREENS(l)
                    {
                        k = (param[l].page*param[l].lines + param[l].y)*
                               param[l].max_chars + param[l].x;
                        kbd_spellchar(param[l].kbd_buf[k]);
                    }
                }
                break;

            case ACTION_KBD_DONE:
                /* accepts what was entered and continues */
                done = true;
                break;

#ifdef KBD_MORSE_INPUT
            case ACTION_KBD_MORSE_SELECT:
                if (morse_mode && morse_reading)
                {
                    morse_code <<= 1;
                    if ((current_tick - morse_tick) > HZ/5)
                        morse_code |= 0x01;
                }

                break;
#endif

            case ACTION_KBD_SELECT:
            case ACTION_KBD_SELECT_REM:
                if (button == ACTION_KBD_SELECT_REM)
                    char_screen = 1;
                else
                    char_screen = 0;
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
                        k = (param[char_screen].page*param[char_screen].lines +
                            param[char_screen].y)*param[char_screen].max_chars +
                            param[char_screen].x;
                        if (k < param[char_screen].nchars)
                            ch = param[char_screen].kbd_buf[k];
                        else
                            ch = ' ';
                    /* check for hangul input */
                    if (ch >= 0x3131 && ch <= 0x3163)
                    {
                        if (!hangul)
                        {
                            hlead=hvowel=htail=0;
                            hangul = true;
                        }
                        if (!hvowel)
                            hvowel = ch;
                        else if (!htail)
                           htail = ch;
                        else
                        { /* previous hangul complete */
                            /* check whether tail is actually lead of next char */
                            if ((tmp = hangul_join(htail, ch, 0)) != 0xfffd)
                            {
                                tmp = hangul_join(hlead, hvowel, 0);
                                kbd_delchar(text, &editpos);
                                kbd_inschar(text, buflen, &editpos, tmp);
                                /* insert dummy char */
                                kbd_inschar(text, buflen, &editpos, ' ');
                                hlead = htail;
                                hvowel = ch;
                                htail = 0;
                            }
                            else
                            {
                                hvowel=htail=0;
                                hlead = ch;
                            }
                        }
                        /* combine into hangul */
                        if ((tmp = hangul_join(hlead, hvowel, htail)) != 0xfffd)
                        {
                            kbd_delchar(text, &editpos);
                            ch = tmp;
                        }
                        else
                        {
                            hvowel=htail=0;
                            hlead = ch;
                        }
                    }
                    else
                        hangul = false;

                    /* insert char */
                    kbd_inschar(text, buflen, &editpos, ch);
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);

                /* speak revised text */
                break;

#ifndef KBD_MODES
            case ACTION_KBD_BACKSPACE:
                if (hangul)
                {
                    if (htail)
                        htail = 0;
                    else if (hvowel)
                        hvowel = 0;
                    else
                        hangul = false;
                }
                kbd_delchar(text, &editpos);
                if (hangul)
                {
                    if (hvowel)
                        ch = hangul_join(hlead, hvowel, htail);
                    else
                        ch = hlead;
                    kbd_inschar(text, buflen, &editpos, ch);
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);   /* speak revised text */
                break;

            case ACTION_KBD_CURSOR_RIGHT:

                if (hangul)
                    hangul = false;

                if (editpos < len_utf8)
                {
                    editpos++;
                    c = utf8seek(text, editpos);
                    kbd_spellchar(text[c]);
                }
                break;

            case ACTION_KBD_CURSOR_LEFT:

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
                }
#endif

                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    FOR_NB_SCREENS(l)
                        screens[l].setfont(FONT_SYSFIXED);
                break;

        }
        if (button != BUTTON_NONE)
        {
            cur_blink = true;
        }
    }
    action_signalscreenchange();
#ifdef HAS_BUTTONBAR
    global_settings.buttonbar=buttonbar_config;
#endif
    FOR_NB_SCREENS(l)
        screens[l].setfont(FONT_UI);
    return 0;
}
