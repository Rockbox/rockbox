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
#include "kernel.h"
#include "system.h"
#include "version.h"
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
#include "hangul.h"
#include "action.h"
#include "icon.h"
#include "pcmbuf.h"
#include "lang.h"
#include "keyboard.h"

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
#define KBD_CURSOR_KEYS /* certain key combos move the cursor even if not
                           in line edit mode */
#define KBD_MODES /* I-Rivers can use picker, line edit and cursor keys */
#define KBD_MORSE_INPUT /* I-Rivers have a Morse input mode */

#elif CONFIG_KEYPAD == ONDIO_PAD /* restricted Ondio keypad */
#define KBD_MODES /* Ondio uses 2 modes, picker and line edit */

#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define KBD_MODES /* iPod uses 2 modes, picker and line edit */
#define KBD_MORSE_INPUT

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define KBD_MODES /* iFP7xx uses 2 modes, picker and line edit */

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD) || (CONFIG_KEYPAD == IAUDIO_M3_PAD)
#define KBD_MODES /* iAudios use 2 modes, picker and line edit */

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define KBD_MODES /* iriver H10 uses 2 modes, picker and line edit */
#define KBD_MORSE_INPUT

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define KBD_CURSOR_KEYS
#define KBD_MODES
#define KBD_MORSE_INPUT

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define KBD_CURSOR_KEYS
#define KBD_MODES
#endif

struct keyboard_parameters
{
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
#ifdef KBD_MODES
    bool line_edit;
#endif
    bool hangul;
    unsigned short hlead, hvowel, htail;
};

static struct keyboard_parameters kbd_param[NB_SCREENS];
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
    int fd, l;
    int i = 0;
    unsigned char buf[4];

    if (filename == NULL)
    {
        kbd_loaded = false;
        return 0;
    }

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
        return 1;

    while (read(fd, buf, 1) == 1 && i < KBD_BUF_SIZE)
    {
        /* check how many bytes to read for this character */
        static const unsigned char sizes[4] = { 0x80, 0xe0, 0xf0, 0xf5 };
        size_t count;

        for (count = 0; count < ARRAYLEN(sizes); count++)
        {
            if (buf[0] < sizes[count])
                break;
        }

        if (count >= ARRAYLEN(sizes))
            continue; /* Invalid size. */

        if (read(fd, &buf[1], count) != (ssize_t)count)
        {
            close(fd);
            kbd_loaded = false;
            return 1;
        }

        FOR_NB_SCREENS(l)
            utf8decode(buf, &kbd_param[l].kbd_buf[i]);

        if (kbd_param[0].kbd_buf[i] != 0xFEFF &&
            kbd_param[0].kbd_buf[i] != '\r') /*skip BOM & carriage returns */
        {
            i++;
        }
    }

    close(fd);
    kbd_loaded = true;

    FOR_NB_SCREENS(l)
        kbd_param[l].nchars = i;

    return 0;
}

/* helper function to spell a char if voice UI is enabled */
static void kbd_spellchar(unsigned short c)
{
    if (global_settings.talk_menu) /* voice UI? */
    {
        unsigned char tmp[5];
        /* store char to pass to talk_spell */
        unsigned char* utf8 = utf8encode(c, tmp);
        *utf8 = 0;

        if(c == ' ')
            talk_id(VOICE_BLANK, false);
        else
            talk_spell(tmp, false);
    }
}

#ifdef KBD_MODES
static void say_edit(void)
{
    if(global_settings.talk_menu)
        talk_id(VOICE_EDIT, false);
}
#endif

static void kbd_inschar(unsigned char* text, int buflen,
                        int* editpos, unsigned short ch)
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
        for (i = len+j; k >= *editpos; i--)
        {
            text[i] = text[i-j];
            if ((text[i] & MASK) != COMP)
                k--;
        }

        while (j--)
            text[i--] = tmp[j];

        (*editpos)++;
    }
}

static void kbd_delchar(unsigned char* text, int* editpos)
{
    int i = 0;
    unsigned char* utf8;

    if (*editpos > 0)
    {
        utf8 = text + utf8seek(text, *editpos);

        do
        {
            i++;
            utf8--;
        }
        while ((*utf8 & MASK) == COMP);

        while (utf8[i])
        {
            *utf8 = utf8[i];
            utf8++;
        }

        *utf8 = 0;
        (*editpos)--;
    }
}

/* Lookup k value based on state of param (pm) */
static int get_param_k(const struct keyboard_parameters *pm)
{
    return (pm->page*pm->lines + pm->y)*pm->max_chars + pm->x;
}

int kbd_input(char* text, int buflen)
{
    bool done = false;
#ifdef CPU_ARM
    /* This seems to keep the sizes for ARM way down */
    struct keyboard_parameters * volatile param = kbd_param;
#else
    struct keyboard_parameters * const param = kbd_param;
#endif
    int l; /* screen loop variable */
    int text_w = 0;
    int editpos;                /* Edit position on all screens */
    const int statusbar_size = global_settings.statusbar
                                    ? STATUSBAR_HEIGHT : 0;
    unsigned short ch;
    unsigned char *utf8;
    bool cur_blink = true;      /* Cursor on/off flag */
#ifdef KBD_MORSE_INPUT
    bool morse_reading = false;
    unsigned char morse_code = 0;
    int morse_tick = 0;
    char buf[2];
#endif

    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &param[l];
#if LCD_WIDTH >= 160 && LCD_HEIGHT >= 96
        struct screen *sc = &screens[l];

        if (sc->width >= 160 && sc->height >= 96)
        {
            pm->default_kbd =
                "ABCDEFG abcdefg !?\" @#$%+'\n"
                "HIJKLMN hijklmn 789 &_()-`\n"
                "OPQRSTU opqrstu 456 §|{}/<\n"
                "VWXYZ., vwxyz.,0123 ~=[]*>\n"
                "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË ¢£¤¥¦§©®\n"
                "àáâãäåæ ìíîï èéêë «»°ºª¹²³\n"
                "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ ¯±×÷¡¿µ·\n"
                "òóôõöø çðþýÿ ùúûü ¼½¾¬¶¨:;";

            pm->DEFAULT_LINES = 8;
        }
        else
#endif /* LCD_WIDTH >= 160 && LCD_HEIGHT >= 96 */
        {
            pm->default_kbd =
                "ABCDEFG !?\" @#$%+'\n"
                "HIJKLMN 789 &_()-`\n"
                "OPQRSTU 456 §|{}/<\n"
                "VWXYZ.,0123 ~=[]*>\n"

                "abcdefg ¢£¤¥¦§©®¬\n"
                "hijklmn «»°ºª¹²³¶\n"
                "opqrstu ¯±×÷¡¿µ·¨\n"
                "vwxyz., :;¼½¾    \n"

                "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË\n"
                "àáâãäåæ ìíîï èéêë\n"
                "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ\n"
                "òóôõöø çðþýÿ ùúûü";

            pm->DEFAULT_LINES = 4;
        }
    }

    char outline[256];
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
    bool buttonbar_config = global_settings.buttonbar;

    global_settings.buttonbar = true;
    gui_buttonbar_init(&buttonbar);

    FOR_NB_SCREENS(l)
        gui_buttonbar_set_display(&buttonbar, &screens[l]);
#endif

    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &param[l];

        if ( !kbd_loaded )
        {
            /* Copy default keyboard to buffer */
            const unsigned char *p = pm->default_kbd;
            int i = 0;

            pm->curfont = FONT_SYSFIXED;

            while (*p != 0)
                p = utf8decode(p, &pm->kbd_buf[i++]);

            pm->nchars = i;
        }
        else
        {
            pm->curfont = FONT_UI;
        }
    }

    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &param[l];
        struct screen *sc = &screens[l];
        int i, w;

        pm->font = font_get(pm->curfont);
        pm->font_h = pm->font->height;

        /* check if FONT_UI fits the screen */
        if (2*pm->font_h + 3 + statusbar_size + BUTTONBAR_HEIGHT > sc->height)
        {
            pm->font = font_get(FONT_SYSFIXED);
            pm->font_h = pm->font->height;
            pm->curfont = FONT_SYSFIXED;
        }

        sc->setfont(pm->curfont);
        pm->font_w = 0; /* reset font width */
        /* find max width of keyboard glyphs */
        for (i = 0; i < pm->nchars; i++)
        {
            w = font_get_width(pm->font, pm->kbd_buf[i]);
            if ( w > pm->font_w )
                pm->font_w = w;
        }

        /* Since we're going to be adding spaces, make sure that we check
         * their width too */
        w = font_get_width( pm->font, ' ' );
        if ( w > pm->font_w )
            pm->font_w = w;
    }

    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &param[l];
        struct screen *sc = &screens[l];
        int i = 0;

        /* Pad lines with spaces */
        while (i < pm->nchars)
        {
            if (pm->kbd_buf[i] == '\n')
            {
                int k = sc->width / pm->font_w
                        - i % ( sc->width / pm->font_w ) - 1;
                int j;

                if (k == sc->width / pm->font_w - 1)
                {
                    pm->nchars--;

                    for (j = i; j < pm->nchars; j++)
                    {
                        pm->kbd_buf[j] = pm->kbd_buf[j + 1];
                    }
                }
                else
                {
                    if (pm->nchars + k - 1 >= KBD_BUF_SIZE)
                    {   /* We don't want to overflow the buffer */
                        k = KBD_BUF_SIZE - pm->nchars;
                    }

                    for (j = pm->nchars + k - 1; j > i + k; j--)
                    {
                        pm->kbd_buf[j] = pm->kbd_buf[j-k];
                    }

                    pm->nchars += k;
                    k++;

                    while (k--)
                    {
                        pm->kbd_buf[i++] = ' ';
                    }
                }
            }
            else
            {
                i++;
            }
        }
    }

    /* Find max width for text string */
    utf8 = text;
    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &param[l];
        struct screen *sc = &screens[l];

        text_w = pm->font_w;

        while (*utf8)
        {
            int w = font_get_width(pm->font, ch);
            utf8 = (unsigned char*)utf8decode(utf8, &ch);

            if (w > text_w)
                text_w = w;
        }

        pm->max_chars_text = sc->width / text_w - 2;

        /* Calculate keyboard grid size */
        pm->max_chars = sc->width / pm->font_w;

        if (!kbd_loaded)
        {
            pm->lines = pm->DEFAULT_LINES;
            pm->keyboard_margin = DEFAULT_MARGIN;
        }
        else
        {
            pm->lines = (sc->height - BUTTONBAR_HEIGHT - statusbar_size)
                            / pm->font_h - 1;
            pm->keyboard_margin = sc->height - BUTTONBAR_HEIGHT -
                statusbar_size - (pm->lines+1)*pm->font_h;

            if (pm->keyboard_margin < 3)
            {
                pm->lines--;
                pm->keyboard_margin += pm->font_h;
            }

            if (pm->keyboard_margin > 6)
                pm->keyboard_margin = 6;
        }

        pm->pages = (pm->nchars + (pm->lines*pm->max_chars-1))
             / (pm->lines*pm->max_chars);

        if (pm->pages == 1 && kbd_loaded)
            pm->lines = (pm->nchars + pm->max_chars - 1) / pm->max_chars;

        pm->main_y = pm->font_h*pm->lines + pm->keyboard_margin + statusbar_size;
        pm->main_x = 0;
        pm->keyboard_margin -= pm->keyboard_margin/2;

#ifdef KBD_MORSE_INPUT
        pm->old_main_y = pm->main_y;
        if (morse_mode)
            pm->main_y = sc->height - pm->font_h;
#endif
    }

    /* Initial edit position is after last character */
    editpos = utf8length(text);

    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */


    while (!done)
    {
        /* These declarations are assigned to the screen on which the key
           action occurred - pointers save a lot of space over array notation
           when accessing the same array element countless times */
        int button;
#if NB_SCREENS > 1
        int button_screen;
#else
        const int button_screen = 0;
#endif
        struct keyboard_parameters *pm;
        struct screen *sc;

        int len_utf8 = utf8length(text);

        FOR_NB_SCREENS(l)
            screens[l].clear_display();

#ifdef KBD_MORSE_INPUT
        if (morse_mode)
        {
            FOR_NB_SCREENS(l)
            {
                /* declare scoped pointers inside screen loops - hide the
                   declarations from previous block level */
                const int w = 6; /* sysfixed font width */
                struct keyboard_parameters *pm = &param[l];
                struct screen *sc = &screens[l];
                int i;

                sc->setfont(FONT_SYSFIXED); /* Draw morse code screen with sysfont */
                pm->x = 0;
                pm->y = statusbar_size;
                buf[1] = '\0';

                /* Draw morse code table with code descriptions. */
                for (i = 0; morse_alphabets[i] != '\0'; i++)
                {
                    int morse_len;
                    int j;

                    buf[0] = morse_alphabets[i];
                    sc->putsxy(pm->x, pm->y, buf);

                    for (j = 0; (morse_codes[i] >> j) > 0x01; j++) ;
                    morse_len = j;

                    pm->x += w + 3;
                    for (j = 0; j < morse_len; j++)
                    {
                        if ((morse_codes[i] >> (morse_len-j-1)) & 0x01)
                            sc->fillrect(pm->x + j*4, pm->y + 2, 3, 4);
                        else
                            sc->fillrect(pm->x + j*4, pm->y + 3, 1, 2);
                    }

                    pm->x += w*5 - 3;
                    if (pm->x >= sc->width - w*6)
                    {
                        pm->x = 0;
                        pm->y += 8; /* sysfixed font height */
                    }
                }
            }
        }
        else
#endif /* KBD_MORSE_INPUT */
        {
            /* draw page */
            FOR_NB_SCREENS(l)
            {
                struct keyboard_parameters *pm = &param[l];
                struct screen *sc = &screens[l];
                int i, j, k;

                sc->setfont(pm->curfont);

                k = pm->page*pm->max_chars*pm->lines;

                for (i = j = 0; j < pm->lines && k < pm->nchars; k++)
                {
                    int w;
                    utf8 = utf8encode(pm->kbd_buf[k], outline);
                    *utf8 = 0;

                    sc->getstringsize(outline, &w, NULL);
                    sc->putsxy(i*pm->font_w + (pm->font_w-w) / 2,
                               j*pm->font_h + statusbar_size, outline);

                    if (++i >= pm->max_chars)
                    {
                        i = 0;
                        j++;
                    }
                }
            }
        }

        /* separator */
        FOR_NB_SCREENS(l)
        {
            struct keyboard_parameters *pm = &param[l];
            struct screen *sc = &screens[l];
            int i = 0, j = 0;

            /* Clear text area one pixel above separator line so any overdraw
               doesn't collide */
            sc->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
            sc->fillrect(0, pm->main_y - pm->keyboard_margin - 1,
                         sc->width, pm->font_h + 4);
            sc->set_drawmode(DRMODE_SOLID);

            sc->hline(0, sc->width - 1, pm->main_y - pm->keyboard_margin);

            /* write out the text */
            sc->setfont(pm->curfont);

            pm->curpos = MIN(editpos, pm->max_chars_text
                                - MIN(len_utf8 - editpos, 2));
            pm->leftpos = editpos - pm->curpos;
            utf8 = text + utf8seek(text, pm->leftpos);

            text_w = pm->font_w;

            while (*utf8 && i < pm->max_chars_text)
            {
                outline[j++] = *utf8++;

                if ((*utf8 & MASK) != COMP)
                {
                    int w;
                    outline[j] = 0;
                    j=0;
                    i++;
                    sc->getstringsize(outline, &w, NULL);
                    sc->putsxy(i*text_w + (text_w-w)/2, pm->main_y, outline);
                }
            }

            if (pm->leftpos > 0)
            {
                /* Draw nicer bitmap arrow if room, else settle for "<". */
                if (text_w >= 6 && pm->font_h >= 8)
                {
                    screen_put_iconxy(sc, (text_w - 6) / 2,
                                      pm->main_y + (pm->font_h - 8) / 2 ,
                                      Icon_Reverse_Cursor);
                }
                else
                {
                    int w;
                    sc->getstringsize("<", &w, NULL);
                    sc->putsxy(text_w - w, pm->main_y, "<");
                }
            }

            if (len_utf8 - pm->leftpos > pm->max_chars_text)
            {
                /* Draw nicer bitmap arrow if room, else settle for ">". */
                if (text_w >= 6 && pm->font_h >= 8)
                {
                    screen_put_iconxy(sc, sc->width - text_w + (text_w - 6) / 2,
                                      pm->main_y + (pm->font_h - 8) / 2,
                                      Icon_Cursor);
                }
                else
                {
                    sc->putsxy(sc->width - text_w, pm->main_y, ">");
                }
            }

            /* cursor */
            i = (pm->curpos + 1) * text_w;

            if (cur_blink)
                sc->vline(i, pm->main_y, pm->main_y + pm->font_h - 1);

            if (pm->hangul) /* draw underbar */
                sc->hline(pm->curpos*text_w, (pm->curpos+1)*text_w,
                          pm->main_y + pm->font_h - 1);
        }

        cur_blink = !cur_blink;

#ifdef HAS_BUTTONBAR
        /* draw the button bar */
        gui_buttonbar_set(&buttonbar, "Shift", "OK", "Del");
        gui_buttonbar_draw(&buttonbar);
#endif

        FOR_NB_SCREENS(l)
        {
            struct keyboard_parameters *pm = &param[l];
            struct screen *sc = &screens[l];

            sc->set_drawmode(DRMODE_COMPLEMENT);
#ifdef KBD_MODES
            if (pm->line_edit)
                sc->fillrect(0, pm->main_y - pm->keyboard_margin + 2,
                             sc->width, pm->font_h + 2);
            else /* highlight the key that has focus */
#endif
                sc->fillrect(pm->font_w*pm->x,
                             statusbar_size + pm->font_h*pm->y,
                             pm->font_w, pm->font_h);
            sc->set_drawmode(DRMODE_SOLID);
        }

        gui_syncstatusbar_draw(&statusbars, true);
        FOR_NB_SCREENS(l)
            screens[l].update();

        button = get_action(CONTEXT_KEYBOARD, HZ/2);
#if NB_SCREENS > 1
        button_screen = (get_action_statuscode(NULL) & ACTION_REMOTE) ? 1 : 0;
#endif
        pm = &param[button_screen];
        sc = &screens[button_screen];

#if defined KBD_MORSE_INPUT && !defined KBD_MODES
        if (morse_mode)
        {
            /* Remap some buttons for morse mode. */
            if (button == ACTION_KBD_LEFT)
                button = ACTION_KBD_CURSOR_LEFT;
            if (button == ACTION_KBD_RIGHT)
                button = ACTION_KBD_CURSOR_RIGHT;
        }
#endif

        switch ( button )
        {
            case ACTION_KBD_ABORT:
                FOR_NB_SCREENS(l)
                    screens[l].setfont(FONT_UI);

#ifdef HAS_BUTTONBAR
                global_settings.buttonbar=buttonbar_config;
#endif
                return -1;
                break;

            case ACTION_KBD_PAGE_FLIP:
            {
                int k;
#ifdef KBD_MORSE_INPUT
                if (morse_mode)
                    break;
#endif
                if (++pm->page >= pm->pages)
                     pm->page = 0;

                k = get_param_k(pm);
                kbd_spellchar(pm->kbd_buf[k]);
                break;
                }

#ifdef KBD_MORSE_INPUT
            case ACTION_KBD_MORSE_INPUT:
                morse_mode = !morse_mode;

                FOR_NB_SCREENS(l)
                {
                    struct keyboard_parameters *pm = &param[l];
                    struct screen *sc = &screens[l];

                    pm->x = pm->y = pm->page = 0;

                    if (morse_mode)
                    {
                        pm->old_main_y = pm->main_y;
                        pm->main_y = sc->height - pm->font_h;
                    }
                    else
                    {
                        pm->main_y = pm->old_main_y;
                    }
                }
                /* FIXME: We should talk something like Morse mode.. */
                break;
#endif /* KBD_MORSE_INPUT */

            case ACTION_KBD_RIGHT:
#ifdef KBD_MODES
#ifdef KBD_MORSE_INPUT
                /* allow cursor change in non line edit morse mode */
                if (pm->line_edit || morse_mode)
#else
                /* right doubles as cursor_right in line_edit */
                if (pm->line_edit)
#endif
                {
                    pm->hangul = false;

                    if (editpos < len_utf8)
                    {
                        int c = utf8seek(text, ++editpos);
                        kbd_spellchar(text[c]);
                    } 
#if CONFIG_CODEC == SWCODEC
                    else if (global_settings.talk_menu)
                        pcmbuf_beep(1000, 150, 1500);
#endif
                }
                else
#endif /* KBD_MODES */
                {
                    int k;
#ifdef KBD_MORSE_INPUT
                    if (morse_mode)
                        break;
#endif
                    if (++pm->x >= pm->max_chars)
                    {
#ifndef KBD_PAGE_FLIP
                        /* no dedicated flip key - flip page on wrap */
                        if (++pm->page >= pm->pages)
                            pm->page = 0;
#endif
                        pm->x = 0;
                    }

                    k = get_param_k(pm);
                    kbd_spellchar(pm->kbd_buf[k]);
                }
                break;

            case ACTION_KBD_LEFT:
#ifdef KBD_MODES
#ifdef KBD_MORSE_INPUT
                /* allow cursor change in non line edit morse mode */
                if (pm->line_edit || morse_mode)
#else
                /* left doubles as cursor_left in line_edit */
                if (pm->line_edit)
#endif
                {
                    pm->hangul = false;

                    if (editpos > 0)
                    {
                        int c = utf8seek(text, --editpos);
                        kbd_spellchar(text[c]);
                    } 
#if CONFIG_CODEC == SWCODEC
                    else if (global_settings.talk_menu)
                        pcmbuf_beep(1000, 150, 1500);
#endif
                }
                else
#endif /* KBD_MODES */
                {
                    int k;
#ifdef KBD_MORSE_INPUT
                    if (morse_mode)
                        break;
#endif
                    if (--pm->x < 0)
                    {
#ifndef KBD_PAGE_FLIP
                        /* no dedicated flip key - flip page on wrap */
                        if (--pm->page < 0)
                            pm->page = pm->pages - 1;
#endif
                        pm->x = pm->max_chars - 1;
                    }

                    k = get_param_k(pm);
                    kbd_spellchar(pm->kbd_buf[k]);
                }
                break;

            case ACTION_KBD_DOWN:
#ifdef KBD_MORSE_INPUT
#ifdef KBD_MODES
                if (morse_mode)
                {
                    pm->line_edit = !pm->line_edit;
                    if(pm->line_edit)
                        say_edit();
                }
                else
#else
                if (morse_mode)
                    break;
#endif
#endif /* KBD_MORSE_INPUT */
                {
#ifdef KBD_MODES
                    if (pm->line_edit)
                    {
                        pm->y = 0;
                        pm->line_edit = false;
                    }
                    else
#endif
                    if (++pm->y >= pm->lines)
#ifdef KBD_MODES
                    {
                        pm->line_edit = true;   
                        say_edit();
                    }
#else
                        pm->y = 0;
#endif
                }
#ifdef KBD_MODES
                if (!pm->line_edit)
#endif
                {
                    int k = get_param_k(pm);
                    kbd_spellchar(pm->kbd_buf[k]);
                }
                break;

            case ACTION_KBD_UP:
#ifdef KBD_MORSE_INPUT
#ifdef KBD_MODES
                if (morse_mode)
                {
                    pm->line_edit = !pm->line_edit;
                    if(pm->line_edit)
                        say_edit();
                }
                else
#else
                if (morse_mode)
                    break;
#endif
#endif /* KBD_MORSE_INPUT */
                {
#ifdef KBD_MODES
                    if (pm->line_edit)
                    {
                        pm->y = pm->lines - 1;
                        pm->line_edit = false;
                    }
                    else
#endif
                    if (--pm->y < 0)
#ifdef KBD_MODES
                    {
                        pm->line_edit = true;
                        say_edit();
                    }
#else
                        pm->y = pm->lines - 1;
#endif
                }
#ifdef KBD_MODES
                if (!pm->line_edit)
#endif
                {
                    int k = get_param_k(pm);
                    kbd_spellchar(pm->kbd_buf[k]);
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
#endif /* KBD_MORSE_INPUT */

            case ACTION_KBD_SELECT:
            case ACTION_KBD_SELECT_REM:
#ifdef KBD_MORSE_INPUT
#ifdef KBD_MODES
                if (morse_mode && !pm->line_edit)
#else
                if (morse_mode)
#endif
                {
                    morse_tick = current_tick;

                    if (!morse_reading)
                    {
                        morse_reading = true;
                        morse_code = 1;
                    }
                    break;
                }
#endif /* KBD_MORSE_INPUT */

                /* inserts the selected char */
#ifdef KBD_MODES
                if (pm->line_edit)
                { /* select doubles as backspace in line_edit */
                    if (pm->hangul)
                    {
                        if (pm->htail)
                            pm->htail = 0;
                        else if (pm->hvowel)
                            pm->hvowel = 0;
                        else
                            pm->hangul = false;
                    }

                    kbd_delchar(text, &editpos);

                    if (pm->hangul)
                    {
                        if (pm->hvowel)
                            ch = hangul_join(pm->hlead, pm->hvowel, pm->htail);
                        else
                            ch = pm->hlead;
                        kbd_inschar(text, buflen, &editpos, ch);
                    }
                }
                else
#endif /* KBD_MODES */
                {
                    /* find input char */
                    int k = get_param_k(pm);
                    ch = (k < pm->nchars) ? pm->kbd_buf[k] : ' ';

                    /* check for hangul input */
                    if (ch >= 0x3131 && ch <= 0x3163)
                    {
                        unsigned short tmp;

                        if (!pm->hangul)
                        {
                            pm->hlead = pm->hvowel = pm->htail = 0;
                            pm->hangul = true;
                        }

                        if (!pm->hvowel)
                        {
                            pm->hvowel = ch;
                        }
                        else if (!pm->htail)
                        {
                            pm->htail = ch;
                        }
                        else
                        { /* previous hangul complete */
                            /* check whether tail is actually lead of next char */
                            tmp = hangul_join(pm->htail, ch, 0);

                            if (tmp != 0xfffd)
                            {
                                tmp = hangul_join(pm->hlead, pm->hvowel, 0);
                                kbd_delchar(text, &editpos);
                                kbd_inschar(text, buflen, &editpos, tmp);
                                /* insert dummy char */
                                kbd_inschar(text, buflen, &editpos, ' ');
                                pm->hlead = pm->htail;
                                pm->hvowel = ch;
                                pm->htail = 0;
                            }
                            else
                            {
                                pm->hvowel = pm->htail = 0;
                                pm->hlead = ch;
                            }
                        }

                        /* combine into hangul */
                        tmp = hangul_join(pm->hlead, pm->hvowel, pm->htail);

                        if (tmp != 0xfffd)
                        {
                            kbd_delchar(text, &editpos);
                            ch = tmp;
                        }
                        else
                        {
                            pm->hvowel = pm->htail = 0;
                            pm->hlead = ch;
                        }
                    }
                    else
                    {
                        pm->hangul = false;
                    }

                    /* insert char */
                    kbd_inschar(text, buflen, &editpos, ch);
                }

                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);

                /* speak revised text */
                break;

#if !defined (KBD_MODES) || defined (KBD_CURSOR_KEYS)
            case ACTION_KBD_BACKSPACE:
                if (pm->hangul)
                {
                    if (pm->htail)
                        pm->htail = 0;
                    else if (pm->hvowel)
                        pm->hvowel = 0;
                    else
                        pm->hangul = false;
                }

                kbd_delchar(text, &editpos);

                if (pm->hangul)
                {
                    if (pm->hvowel)
                        ch = hangul_join(pm->hlead, pm->hvowel, pm->htail);
                    else
                        ch = pm->hlead;
                    kbd_inschar(text, buflen, &editpos, ch);
                }

                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false);   /* speak revised text */
                break;

            case ACTION_KBD_CURSOR_RIGHT:
                pm->hangul = false;

                if (editpos < len_utf8)
                {
                    int c = utf8seek(text, ++editpos);
                    kbd_spellchar(text[c]);
                }
#if CONFIG_CODEC == SWCODEC
                else if (global_settings.talk_menu)
                    pcmbuf_beep(1000, 150, 1500);
#endif
                break;

            case ACTION_KBD_CURSOR_LEFT:
                pm->hangul = false;

                if (editpos > 0)
                {
                    int c = utf8seek(text, --editpos);
                    kbd_spellchar(text[c]);
                }
#if CONFIG_CODEC == SWCODEC
                else if (global_settings.talk_menu)
                    pcmbuf_beep(1000, 150, 1500);
#endif
                break;
#endif /* !defined (KBD_MODES) || defined (KBD_CURSOR_KEYS) */

            case BUTTON_NONE:
                gui_syncstatusbar_draw(&statusbars, false);
#ifdef KBD_MORSE_INPUT
                if (morse_reading)
                {
                    int j;
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
                    FOR_NB_SCREENS(l)
                        param[l].hangul = false;
                    kbd_inschar(text, buflen, &editpos, morse_alphabets[j]);

                    if (global_settings.talk_menu) /* voice UI? */
                        talk_spell(text, false);   /* speak revised text */
                }
#endif /* KBD_MORSE_INPUT */
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    FOR_NB_SCREENS(l)
                        screens[l].setfont(FONT_SYSFIXED);
                }
                break;

        } /* end switch */

        if (button != BUTTON_NONE)
        {
            cur_blink = true;
        }
    }

#ifdef HAS_BUTTONBAR
    global_settings.buttonbar = buttonbar_config;
#endif

    FOR_NB_SCREENS(l)
        screens[l].setfont(FONT_UI);

    return 0;
}
