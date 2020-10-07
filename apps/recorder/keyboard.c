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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "kernel.h"
#include "system.h"
#include "string-extra.h"
#include "font.h"
#include "screens.h"
#include "talk.h"
#include "settings.h"
#include "misc.h"
#include "rbunicode.h"
#include "logf.h"
#include "hangul.h"
#include "action.h"
#include "icon.h"
#include "pcmbuf.h"
#include "lang.h"
#include "keyboard.h"
#include "viewport.h"
#include "file.h"
#include "splash.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif


#define DEFAULT_MARGIN 6
#define KBD_BUF_SIZE 500

#ifdef HAVE_TOUCHSCREEN
#define MIN_GRID_SIZE   16
#define GRID_SIZE(s, x)   \
        ((s) == SCREEN_MAIN && MIN_GRID_SIZE > (x) ? MIN_GRID_SIZE: (x))
#endif

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD) \
    || (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
    || (CONFIG_KEYPAD == IPOD_3G_PAD) \
    || (CONFIG_KEYPAD == IPOD_4G_PAD) \
    || (CONFIG_KEYPAD == IRIVER_H10_PAD) \
    || (CONFIG_KEYPAD == GIGABEAT_PAD) \
    || (CONFIG_KEYPAD == GIGABEAT_S_PAD) \
    || (CONFIG_KEYPAD == MROBE100_PAD) \
    || (CONFIG_KEYPAD == SANSA_E200_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_SA9200_PAD) \
    || (CONFIG_KEYPAD == PBELL_VIBE500_PAD) \
    || (CONFIG_KEYPAD == SANSA_CONNECT_PAD) \
    || (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) \
    || (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)

/* certain key combos toggle input mode between keyboard input and Morse input */
#define KBD_TOGGLE_INPUT
#endif

#define CHANGED_PICKER 1
#define CHANGED_CURSOR 2
#define CHANGED_TEXT   3

struct keyboard_parameters
{
    unsigned short kbd_buf[KBD_BUF_SIZE];
    unsigned short *kbd_buf_ptr;
    unsigned short max_line_len;
    int default_lines;
    int last_k;
    int last_i;
    int font_w;
    int font_h;
    int text_w;
    int curfont;
    int main_y;
#ifdef HAVE_MORSE_INPUT
    int old_main_y;
#endif
    int max_chars;
    int max_chars_text;
    int lines;
    int pages;
    int keyboard_margin;
    int curpos;
    int leftpos;
    int page;
    int x;
    int y;
    bool line_edit;
#ifdef HAVE_TOUCHSCREEN
    bool show_buttons;
#endif
};

struct edit_state
{
    char* text;
    int buflen;
    int len_utf8;
    int editpos;        /* Edit position on all screens */
    bool cur_blink;     /* Cursor on/off flag */
    bool hangul;
    unsigned short hlead, hvowel, htail;
#ifdef HAVE_MORSE_INPUT
    bool morse_mode;
    bool morse_reading;
    unsigned char morse_code;
    int morse_tick;
#endif
    int changed;
};

static struct keyboard_parameters kbd_param[NB_SCREENS];
static bool kbd_loaded = false;

#ifdef HAVE_MORSE_INPUT
/* FIXME: We should put this to a configuration file. */
static const char * const morse_alphabets =
    "abcdefghijklmnopqrstuvwxyz1234567890,.?-@ ";
static const unsigned char morse_codes[] = {
    0x05,0x18,0x1a,0x0c,0x02,0x12,0x0e,0x10,0x04,0x17,0x0d,0x14,0x07,
    0x06,0x0f,0x16,0x1d,0x0a,0x08,0x03,0x09,0x11,0x0b,0x19,0x1b,0x1c,
    0x2f,0x27,0x23,0x21,0x20,0x30,0x38,0x3c,0x3e,0x3f,
    0x73,0x55,0x4c,0x61,0x5a,0x80 };
#endif

/* Loads a custom keyboard into memory
   call with NULL to reset keyboard    */
int load_kbd(unsigned char* filename)
{
    int fd;
    int i, line_len, max_line_len;
    unsigned char buf[4];
    unsigned short *pbuf;

    if (filename == NULL)
    {
        kbd_loaded = false;
        return 0;
    }

    fd = open_utf8(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
        return 1;

    pbuf = kbd_param[0].kbd_buf;
    line_len = 0;
    max_line_len = 1;
    i = 1;
    while (read(fd, buf, 1) == 1 && i < KBD_BUF_SIZE-1)
    {
        /* check how many bytes to read for this character */
        static const unsigned char sizes[4] = { 0x80, 0xe0, 0xf0, 0xf5 };
        size_t count;
        unsigned short ch;

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

        utf8decode(buf, &ch);
        if (ch != 0xFEFF && ch != '\r') /* skip BOM & carriage returns */
        {
            i++;
            if (ch == '\n')
            {
                if (max_line_len < line_len)
                    max_line_len = line_len;
                *pbuf = line_len;
                pbuf += line_len + 1;
                line_len = 0;
            }
            else
                pbuf[++line_len] = ch;
        }
    }

    close(fd);
    kbd_loaded = true;

    if (max_line_len < line_len)
        max_line_len = line_len;
    if (i == 1 || line_len != 0) /* ignore last empty line */
    {
        *pbuf = line_len;
        pbuf += line_len + 1;
    }
    *pbuf = 0xFEFF; /* mark end of characters */
    i++;
    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &kbd_param[l];
#if NB_SCREENS > 1
        if (l > 0)
            memcpy(pm->kbd_buf, kbd_param[0].kbd_buf, i*sizeof(unsigned short));
#endif
        /* initialize parameters */
        pm->x = pm->y = pm->page = 0;
        pm->default_lines = 0;
        pm->max_line_len = max_line_len;
    }

    return 0;
}

/* helper function to spell a char */
static void kbd_spellchar(unsigned short c)
{
    unsigned char tmp[5];
    /* store char to pass to talk_spell */
    unsigned char* utf8 = utf8encode(c, tmp);
    *utf8 = 0;

    if (c == ' ')
        talk_id(VOICE_BLANK, false);
    else
        talk_spell(tmp, false);
}

static void kbd_inschar(struct edit_state *state, unsigned short ch)
{
    int i, j, len;
    unsigned char tmp[4];
    unsigned char* utf8;

    len = strlen(state->text);
    utf8 = utf8encode(ch, tmp);
    j = (intptr_t)utf8 - (intptr_t)tmp;

    if (len + j < state->buflen)
    {
        i = utf8seek(state->text, state->editpos);
        utf8 = state->text + i;
        memmove(utf8 + j, utf8, len - i + 1);
        memcpy(utf8, tmp, j);
        state->editpos++;
        state->changed = CHANGED_TEXT;
    }
}

static void kbd_delchar(struct edit_state *state)
{
    int i, j, len;
    unsigned char* utf8;

    if (state->editpos > 0)
    {
        state->editpos--;
        len = strlen(state->text);
        i = utf8seek(state->text, state->editpos);
        utf8 = state->text + i;
        j = utf8seek(utf8, 1);
        memmove(utf8, utf8 + j, len - i - j + 1);
        state->changed = CHANGED_TEXT;
    }
}

/* Lookup k value based on state of param (pm) */
static unsigned short get_kbd_ch(struct keyboard_parameters *pm, int x, int y)
{
    int i = 0, k = pm->page*pm->lines + y, n;
    unsigned short *pbuf;
    if (k >= pm->last_k)
    {
        i = pm->last_i;
        k -= pm->last_k;
    }
    for (pbuf = &pm->kbd_buf_ptr[i]; (i = *pbuf) != 0xFEFF; pbuf += i + 1)
    {
        n = i ? (i + pm->max_chars - 1) / pm->max_chars : 1;
        if (k < n) break;
        k -= n;
    }
    if (y == 0 && i != 0xFEFF)
    {
        pm->last_k = pm->page*pm->lines - k;
        pm->last_i = pbuf - pm->kbd_buf_ptr;
    }
    k = k * pm->max_chars + x;
    return (*pbuf != 0xFEFF && k < *pbuf)? pbuf[k+1]: ' ';
}

static void kbd_calc_params(struct keyboard_parameters *pm,
                            struct screen *sc, struct edit_state *state);
static void kbd_draw_picker(struct keyboard_parameters *pm,
                            struct screen *sc, struct edit_state *state);
static void kbd_draw_edit_line(struct keyboard_parameters *pm,
                               struct screen *sc, struct edit_state *state);
#ifdef HAVE_TOUCHSCREEN
static void kbd_draw_buttons(struct keyboard_parameters *pm, struct screen *sc);
static int keyboard_touchscreen(struct keyboard_parameters *pm,
                                struct screen *sc, struct edit_state *state);
#endif
static void kbd_insert_selected(struct keyboard_parameters *pm,
                                struct edit_state *state);
static void kbd_backspace(struct edit_state *state);
static void kbd_move_cursor(struct edit_state *state, int dir);
static void kbd_move_picker_horizontal(struct keyboard_parameters *pm,
                                       struct edit_state *state, int dir);
static void kbd_move_picker_vertical(struct keyboard_parameters *pm,
                                     struct edit_state *state, int dir);

int kbd_input(char* text, int buflen, unsigned short *kbd)
{
    bool done = false;
    struct keyboard_parameters * const param = kbd_param;
    struct edit_state state;
    unsigned short ch;
    int ret = 0; /* assume success */
    FOR_NB_SCREENS(l)
    {
        viewportmanager_theme_enable(l, false, NULL);
    }

    /* initialize state */
    state.text = text;
    state.buflen = buflen;
    /* Initial edit position is after last character */
    state.editpos = utf8length(state.text);
    state.cur_blink = true;
#ifdef HAVE_MORSE_INPUT
    state.morse_mode = global_settings.morse_input;
    state.morse_reading = false;
#endif
    state.hangul = false;
    state.changed = 0;

    if (!kbd_loaded)
    {
        /* Copy default keyboard to buffer */
        FOR_NB_SCREENS(l)
        {
            struct keyboard_parameters *pm = &param[l];
            unsigned short *pbuf;
            const unsigned char *p;
            int len = 0;

#if LCD_WIDTH >= 160 && LCD_HEIGHT >= 96
            struct screen *sc = &screens[l];

            if (sc->getwidth() >= 160 && sc->getheight() >= 96)
            {
                p = "ABCDEFG abcdefg !?\" @#$%+'\n"
                    "HIJKLMN hijklmn 789 &_()-`\n"
                    "OPQRSTU opqrstu 456 §|{}/<\n"
                    "VWXYZ., vwxyz.,0123 ~=[]*>\n"
                    "ÀÁÂÃÄÅÆ ÌÍÎÏ ÈÉÊË ¢£¤¥¦§©®\n"
                    "àáâãäåæ ìíîï èéêë «»°ºª¹²³\n"
                    "ÓÒÔÕÖØ ÇÐÞÝß ÙÚÛÜ ¯±×÷¡¿µ·\n"
                    "òóôõöø çðþýÿ ùúûü ¼½¾¬¶¨:;";

                pm->default_lines = 8;
                pm->max_line_len = 26;
            }
            else
#endif /* LCD_WIDTH >= 160 && LCD_HEIGHT >= 96 */
            {
                p = "ABCDEFG !?\" @#$%+'\n"
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

                pm->default_lines = 4;
                pm->max_line_len = 18;
            }

            pbuf = pm->kbd_buf;
            while (*p)
            {
                p = utf8decode(p, &pbuf[len+1]);
                if (pbuf[len+1] == '\n')
                {
                    *pbuf = len;
                    pbuf += len+1;
                    len = 0;
                }
                else
                    len++;
            }
            *pbuf = len;
            pbuf[len+1] = 0xFEFF;   /* mark end of characters */

            /* initialize parameters */
            pm->x = pm->y = pm->page = 0;
        }
        kbd_loaded = true;
    }

    FOR_NB_SCREENS(l)
    {
        struct keyboard_parameters *pm = &param[l];

        if(kbd) /* user supplied custom layout */
            pm->kbd_buf_ptr = kbd;
        else
            pm->kbd_buf_ptr = pm->kbd_buf; /* internal layout buffer */

        struct screen *sc = &screens[l];
        kbd_calc_params(pm, sc, &state);
    }

    if (global_settings.talk_menu)      /* voice UI? */
        talk_spell(state.text, true);   /* spell initial text */

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

        state.len_utf8 = utf8length(state.text);

        FOR_NB_SCREENS(l)
        {
            /* declare scoped pointers inside screen loops - hide the
               declarations from previous block level */
            struct keyboard_parameters *pm = &param[l];
            struct screen *sc = &screens[l];
            sc->clear_display();
            kbd_draw_picker(pm, sc, &state);
            kbd_draw_edit_line(pm, sc, &state);
#ifdef HAVE_TOUCHSCREEN
            if (pm->show_buttons)
                kbd_draw_buttons(pm, sc);
#endif
        }

        FOR_NB_SCREENS(l)
            screens[l].update();

        state.cur_blink = !state.cur_blink;

        button = get_action(
#ifdef HAVE_MORSE_INPUT
                state.morse_mode? CONTEXT_MORSE_INPUT:
#endif
                            CONTEXT_KEYBOARD, HZ/2);
#if NB_SCREENS > 1
        button_screen = (get_action_statuscode(NULL) & ACTION_REMOTE) ? 1 : 0;
#endif
        pm = &param[button_screen];
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
        {
            struct screen *sc = &screens[button_screen];
            button = keyboard_touchscreen(pm, sc, &state);
        }
#endif

        /* Remap some buttons to allow to move
         * cursor in line edit mode and morse mode. */
        if (pm->line_edit
#ifdef HAVE_MORSE_INPUT
            || state.morse_mode
#endif /* HAVE_MORSE_INPUT */
            )
        {
            if (button == ACTION_KBD_LEFT)
                button = ACTION_KBD_CURSOR_LEFT;
            if (button == ACTION_KBD_RIGHT)
                button = ACTION_KBD_CURSOR_RIGHT;
        }

        switch ( button )
        {
            case ACTION_KBD_DONE:
                /* accepts what was entered and continues */
                ret = 0;
                done = true;
                break;

            case ACTION_KBD_ABORT:
                ret = -1;
                done = true;
                break;

            case ACTION_KBD_PAGE_FLIP:
#ifdef HAVE_MORSE_INPUT
                if (state.morse_mode)
                    break;
#endif
                if (++pm->page >= pm->pages)
                    pm->page = 0;

                state.changed = CHANGED_PICKER;
                break;

            case ACTION_KBD_RIGHT:
                kbd_move_picker_horizontal(pm, &state, 1);
                break;

            case ACTION_KBD_LEFT:
                kbd_move_picker_horizontal(pm, &state, -1);
                break;

            case ACTION_KBD_DOWN:
                kbd_move_picker_vertical(pm, &state, 1);
                break;

            case ACTION_KBD_UP:
                kbd_move_picker_vertical(pm, &state, -1);
                break;

#ifdef HAVE_MORSE_INPUT
#ifdef KBD_TOGGLE_INPUT
            case ACTION_KBD_MORSE_INPUT:
                state.morse_mode = !state.morse_mode;
                state.changed = CHANGED_PICKER;

                FOR_NB_SCREENS(l)
                {
                    struct keyboard_parameters *pm = &param[l];
                    int y = pm->main_y;
                    pm->main_y = pm->old_main_y;
                    pm->old_main_y = y;
                }
                break;
#endif /* KBD_TOGGLE_INPUT */

            case ACTION_KBD_MORSE_SELECT:
                if (state.morse_mode && state.morse_reading)
                {
                    state.morse_code <<= 1;
                    if ((current_tick - state.morse_tick) > HZ/5)
                        state.morse_code |= 0x01;
                }
                break;
#endif /* HAVE_MORSE_INPUT */

            case ACTION_KBD_SELECT:
                /* select doubles as backspace in line_edit */
                if (pm->line_edit)
                    kbd_backspace(&state);
                else
#ifdef HAVE_MORSE_INPUT
                if (state.morse_mode)
                {
                    state.morse_tick = current_tick;

                    if (!state.morse_reading)
                    {
                        state.morse_reading = true;
                        state.morse_code = 1;
                    }
                }
                else
#endif /* HAVE_MORSE_INPUT */
                    kbd_insert_selected(pm, &state);
                break;

            case ACTION_KBD_BACKSPACE:
                kbd_backspace(&state);
                break;

            case ACTION_KBD_CURSOR_RIGHT:
                kbd_move_cursor(&state, 1);
                break;

            case ACTION_KBD_CURSOR_LEFT:
                kbd_move_cursor(&state, -1);
                break;

            case ACTION_NONE:
#ifdef HAVE_MORSE_INPUT
                if (state.morse_reading)
                {
                    int j;
                    logf("Morse: 0x%02x", state.morse_code);
                    state.morse_reading = false;

                    for (j = 0; morse_alphabets[j] != '\0'; j++)
                    {
                        if (morse_codes[j] == state.morse_code)
                            break ;
                    }

                    if (morse_alphabets[j] == '\0')
                    {
                        logf("Morse code not found");
                        break ;
                    }

                    /* turn off hangul input */
                    state.hangul = false;
                    kbd_inschar(&state, morse_alphabets[j]);
                }
#endif /* HAVE_MORSE_INPUT */
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    FOR_NB_SCREENS(l)
                        screens[l].setfont(FONT_SYSFIXED);
                }
                break;

        } /* end switch */

        if (button != ACTION_NONE)
        {
            state.cur_blink = true;
        }
        if (global_settings.talk_menu)  /* voice UI? */
        {
            if (state.changed == CHANGED_PICKER)
            {
                if (pm->line_edit)
                {
                    talk_id(VOICE_EDIT, false);
                }
                else
#ifdef HAVE_MORSE_INPUT
                /* FIXME: We should talk something like Morse mode.. */
                if (!state.morse_mode)
#endif
                {
                    ch = get_kbd_ch(pm, pm->x, pm->y);
                    kbd_spellchar(ch);
                }
            }
            else if (state.changed == CHANGED_CURSOR)
            {
                int c = utf8seek(state.text, state.editpos);
                kbd_spellchar(state.text[c]);
            }
            else if (state.changed == CHANGED_TEXT)
                talk_spell(state.text, false);  /* speak revised text */
        }
        state.changed = 0;
    }

    if (ret < 0)
        splash(HZ/2, ID2P(LANG_CANCEL));

#if defined(HAVE_MORSE_INPUT) && defined(KBD_TOGGLE_INPUT)
    if (global_settings.morse_input != state.morse_mode)
    {
        global_settings.morse_input = state.morse_mode;
        settings_save();
    }
#endif /* HAVE_MORSE_INPUT && KBD_TOGGLE_INPUT */

    FOR_NB_SCREENS(l)
    {
        screens[l].setfont(FONT_UI);
        viewportmanager_theme_undo(l, false);
    }
    return ret;
}

static void kbd_calc_params(struct keyboard_parameters *pm,
                            struct screen *sc, struct edit_state *state)
{
    struct font* font;
    const unsigned char *p;
    unsigned short ch, *pbuf;
    int icon_w, sc_w, sc_h, w;
    int i, total_lines;
#ifdef HAVE_TOUCHSCREEN
    int button_h = 0;
    bool flippage_button = false;
    pm->show_buttons = (sc->screen_type == SCREEN_MAIN &&
                                (touchscreen_get_mode() == TOUCHSCREEN_POINT));
#endif

    pm->curfont = pm->default_lines ? FONT_SYSFIXED : sc->getuifont();
    font = font_get(pm->curfont);
    pm->font_h = font->height;

    /* check if FONT_UI fits the screen */
    if (2*pm->font_h + 3 > sc->getheight())
    {
        pm->curfont = FONT_SYSFIXED;
        font = font_get(FONT_SYSFIXED);
        pm->font_h = font->height;
    }
#ifdef HAVE_TOUCHSCREEN
    pm->font_h = GRID_SIZE(sc->screen_type, pm->font_h);
#endif

    /* find max width of keyboard glyphs.
     * since we're going to be adding spaces,
     * max width is at least their width */
    pm->font_w = font_get_width(font, ' ');
    for (pbuf = pm->kbd_buf_ptr; *pbuf != 0xFEFF; pbuf += i)
    {
        for (i = 0; ++i <= *pbuf; )
        {
            w = font_get_width(font, pbuf[i]);
            if (pm->font_w < w)
                pm->font_w = w;
        }
    }

    /* Find max width for text string */
    pm->text_w = pm->font_w;
    p = state->text;
    while (*p)
    {
        p = utf8decode(p, &ch);
        w = font_get_width(font, ch);
        if (pm->text_w < w)
            pm->text_w = w;
    }

#ifdef HAVE_TOUCHSCREEN
    pm->font_w = GRID_SIZE(sc->screen_type, pm->font_w);
#endif
    /* calculate how many characters to put in a row. */
    icon_w = get_icon_width(sc->screen_type);
    sc_w = sc->getwidth();
    if (pm->font_w < sc_w / pm->max_line_len)
        pm->font_w = sc_w / pm->max_line_len;
    pm->max_chars = sc_w / pm->font_w;
    pm->max_chars_text = (sc_w - icon_w * 2 - 2) / pm->text_w;
    if (pm->max_chars_text < 3 && icon_w > pm->text_w)
        pm->max_chars_text = sc_w / pm->text_w - 2;

    /* calculate pm->pages and pm->lines */
    sc_h = sc->getheight();
#ifdef HAVE_TOUCHSCREEN
    /* add space for buttons */
    if (pm->show_buttons)
    {
        /* reserve place for OK/Del/Cancel buttons, use ui font for them */
        button_h = GRID_SIZE(sc->screen_type, sc->getcharheight());
        sc_h -= MAX(MIN_GRID_SIZE*2, button_h);
    }
recalc_param:
#endif
    pm->lines = sc_h / pm->font_h - 1;

    if (pm->default_lines && pm->lines > pm->default_lines)
        pm->lines = pm->default_lines;

    pm->keyboard_margin = sc_h - (pm->lines+1)*pm->font_h;

    if (pm->keyboard_margin < 3 && pm->lines > 1)
    {
        pm->lines--;
        pm->keyboard_margin += pm->font_h;
    }

    if (pm->keyboard_margin > DEFAULT_MARGIN)
        pm->keyboard_margin = DEFAULT_MARGIN;

    total_lines = 0;
    for (pbuf = pm->kbd_buf_ptr; (i = *pbuf) != 0xFEFF; pbuf += i + 1)
        total_lines += (i ? (i + pm->max_chars - 1) / pm->max_chars : 1);

    pm->pages = (total_lines + pm->lines - 1) / pm->lines;
    pm->lines = (total_lines + pm->pages - 1) / pm->pages;
#ifdef HAVE_TOUCHSCREEN
    if (pm->pages > 1 && pm->show_buttons && !flippage_button)
    {
        /* add space for flip page button */
        sc_h -= button_h;
        flippage_button = true;
        goto recalc_param;
    }
#endif
    if (pm->page >= pm->pages)
        pm->x = pm->y = pm->page = 0;

    pm->main_y = pm->font_h*pm->lines + pm->keyboard_margin;
    pm->keyboard_margin -= pm->keyboard_margin/2;
#ifdef HAVE_TOUCHSCREEN
    /* flip page button is put between piker and edit line */
    if (flippage_button)
        pm->main_y += button_h;
#endif

#ifdef HAVE_MORSE_INPUT
    pm->old_main_y = sc_h - pm->font_h;
    if (state->morse_mode)
    {
        int y = pm->main_y;
        pm->main_y = pm->old_main_y;
        pm->old_main_y = y;
    }
#endif
}

static void kbd_draw_picker(struct keyboard_parameters *pm,
                            struct screen *sc, struct edit_state *state)
{
    char outline[8];
#ifdef HAVE_MORSE_INPUT
    if (state->morse_mode)
    {
        const int w = 6, h = 8; /* sysfixed font width, height */
        int i, j, x, y;
        int sc_w = sc->getwidth(), sc_h = pm->main_y - pm->keyboard_margin - 1;

        /* Draw morse code screen with sysfont */
        sc->setfont(FONT_SYSFIXED);
        x = 0;
        y = 0;
        outline[1] = '\0';

        /* Draw morse code table with code descriptions. */
        for (i = 0; morse_alphabets[i] != '\0'; i++)
        {
            int morse_code;

            outline[0] = morse_alphabets[i];
            sc->putsxy(x, y, outline);

            morse_code = morse_codes[i];
            for (j = 0; morse_code > 0x01; morse_code >>= 1)
                j++;

            x += w + 3 + j*4;
            morse_code = morse_codes[i];
            for (; morse_code > 0x01; morse_code >>= 1)
            {
                x -= 4;
                if (morse_code & 0x01)
                    sc->fillrect(x, y + 2, 3, 4);
                else
                    sc->fillrect(x, y + 3, 1, 2);
            }

            x += w*5 - 3;
            if (x + w*6 >= sc_w)
            {
                x = 0;
                y += h;
                if (y + h >= sc_h)
                    break;
            }
        }
    }
    else
#else
    (void) state;
#endif /* HAVE_MORSE_INPUT */
    {
        /* draw page */
        int i, j;
        int w, h;
        unsigned short ch;
        unsigned char *utf8;

        sc->setfont(pm->curfont);

        for (j = 0; j < pm->lines; j++)
        {
            for (i = 0; i < pm->max_chars; i++)
            {
                ch = get_kbd_ch(pm, i, j);
                utf8 = utf8encode(ch, outline);
                *utf8 = 0;

                sc->getstringsize(outline, &w, &h);
                sc->putsxy(i*pm->font_w + (pm->font_w-w) / 2,
                           j*pm->font_h + (pm->font_h-h) / 2, outline);
            }
        }

        if (!pm->line_edit)
        {
            /* highlight the key that has focus */
            sc->set_drawmode(DRMODE_COMPLEMENT);
            sc->fillrect(pm->font_w*pm->x, pm->font_h*pm->y,
                         pm->font_w, pm->font_h);
            sc->set_drawmode(DRMODE_SOLID);
        }
    }
}

static void kbd_draw_edit_line(struct keyboard_parameters *pm,
                               struct screen *sc, struct edit_state *state)
{
    char outline[8];
    unsigned char *utf8;
    int i = 0, j = 0, icon_w, w;
    int sc_w = sc->getwidth();
    int y = pm->main_y - pm->keyboard_margin;
    int text_margin = (sc_w - pm->text_w * pm->max_chars_text) / 2;

    /* Clear text area one pixel above separator line so any overdraw
       doesn't collide */
    screen_clear_area(sc, 0, y - 1, sc_w, pm->font_h + 6);

    sc->hline(0, sc_w - 1, y);

    /* write out the text */
    sc->setfont(pm->curfont);

    pm->leftpos = MAX(0, MIN(state->len_utf8, state->editpos + 2)
                            - pm->max_chars_text);
    pm->curpos = state->editpos - pm->leftpos;
    utf8 = state->text + utf8seek(state->text, pm->leftpos);

    while (*utf8 && i < pm->max_chars_text)
    {
        j = utf8seek(utf8, 1);
        strlcpy(outline, utf8, j+1);
        sc->getstringsize(outline, &w, NULL);
        sc->putsxy(text_margin + i*pm->text_w + (pm->text_w-w)/2,
                   pm->main_y, outline);
        utf8 += j;
        i++;
    }

    icon_w = get_icon_width(sc->screen_type);
    if (pm->leftpos > 0)
    {
        /* Draw nicer bitmap arrow if room, else settle for "<". */
        if (text_margin >= icon_w)
        {
            screen_put_icon_with_offset(sc, 0, 0,
                                        (text_margin - icon_w) / 2,
                                        pm->main_y, Icon_Reverse_Cursor);
        }
        else
        {
            sc->getstringsize("<", &w, NULL);
            sc->putsxy(text_margin - w, pm->main_y, "<");
        }
    }

    if (state->len_utf8 - pm->leftpos > pm->max_chars_text)
    {
        /* Draw nicer bitmap arrow if room, else settle for ">". */
        if (text_margin >= icon_w)
        {
            screen_put_icon_with_offset(sc, 0, 0,
                                        sc_w - (text_margin + icon_w) / 2,
                                        pm->main_y, Icon_Cursor);
        }
        else
        {
            sc->putsxy(sc_w - text_margin, pm->main_y, ">");
        }
    }

    /* cursor */
    i = text_margin + pm->curpos * pm->text_w;

    if (state->cur_blink)
        sc->vline(i, pm->main_y, pm->main_y + pm->font_h - 1);

    if (state->hangul) /* draw underbar */
        sc->hline(i - pm->text_w, i, pm->main_y + pm->font_h - 1);

    if (pm->line_edit)
    {
        sc->set_drawmode(DRMODE_COMPLEMENT);
        sc->fillrect(0, y + 2, sc_w, pm->font_h + 2);
        sc->set_drawmode(DRMODE_SOLID);
    }
}

#ifdef HAVE_TOUCHSCREEN
static void kbd_draw_buttons(struct keyboard_parameters *pm, struct screen *sc)
{
    struct viewport vp;
    int button_h, text_h, text_y;
    int sc_w = sc->getwidth(), sc_h = sc->getheight();
    viewport_set_defaults(&vp, sc->screen_type);
    vp.flags |= VP_FLAG_ALIGN_CENTER;
    struct viewport *last_vp = sc->set_viewport(&vp);
    text_h = sc->getcharheight();
    button_h = GRID_SIZE(sc->screen_type, text_h);
    text_y = (button_h - text_h) / 2 + 1;
    vp.x = 0;
    vp.y = 0;
    vp.width = sc_w;
    vp.height = button_h;
    /* draw buttons */
    if (pm->pages > 1)
    {
        /* button to flip page. */
        vp.y = pm->lines*pm->font_h;
        sc->hline(0, sc_w - 1, 0);
        sc->putsxy(0, text_y, ">");
    }
    /* OK/Del/Cancel buttons */
    button_h = MAX(MIN_GRID_SIZE*2, button_h);
    text_y = (button_h - text_h) / 2 + 1;
    vp.y = sc_h - button_h - 1;
    vp.height = button_h;
    sc->hline(0, sc_w - 1, 0);
    vp.width = sc_w/3;
    sc->putsxy(0, text_y, str(LANG_KBD_OK));
    vp.x += vp.width;
    sc->vline(0, 0, button_h);
    sc->putsxy(0, text_y, str(LANG_KBD_DELETE));
    vp.x += vp.width;
    sc->vline(0, 0, button_h);
    sc->putsxy(0, text_y, str(LANG_KBD_CANCEL));
    sc->set_viewport(last_vp);
}

static int keyboard_touchscreen(struct keyboard_parameters *pm,
                                struct screen *sc, struct edit_state *state)
{
    short x, y;
    const int button = action_get_touchscreen_press(&x, &y);
    const int sc_w = sc->getwidth(), sc_h = sc->getheight();
    int button_h = MAX(MIN_GRID_SIZE*2, sc->getcharheight());
#ifdef HAVE_MORSE_INPUT
    if (state->morse_mode && y < pm->main_y - pm->keyboard_margin)
    {
        /* don't return ACTION_NONE since it has effect in morse mode. */
        return button == BUTTON_TOUCHSCREEN? ACTION_KBD_SELECT:
               button & BUTTON_REL? ACTION_KBD_MORSE_SELECT: ACTION_STD_OK;
    }
#else
    (void) state;
#endif
    if (x < 0 || y < 0)
        return ACTION_NONE;
    if (y < pm->lines*pm->font_h)
    {
        if (x/pm->font_w < pm->max_chars)
        {
            /* picker area */
            state->changed = CHANGED_PICKER;
            pm->x = x/pm->font_w;
            pm->y = y/pm->font_h;
            pm->line_edit = false;
            if (button == BUTTON_REL)
                return ACTION_KBD_SELECT;
        }
    }
    else if (y < pm->main_y - pm->keyboard_margin)
    {
        /* button to flip page */
        if (button == BUTTON_REL)
            return ACTION_KBD_PAGE_FLIP;
    }
    else if (y < sc_h - button_h)
    {
        /* edit line */
        if (button & (BUTTON_REPEAT|BUTTON_REL))
        {
            if (x < sc_w/2)
                return ACTION_KBD_CURSOR_LEFT;
            else
                return ACTION_KBD_CURSOR_RIGHT;
        }
    }
    else
    {
        /* OK/Del/Cancel button */
        if (button == BUTTON_REL)
        {
            if (x < sc_w/3)
                return ACTION_KBD_DONE;
            else if (x < (sc_w/3) * 2)
                return ACTION_KBD_BACKSPACE;
            else
                return ACTION_KBD_ABORT;
        }
    }
    return ACTION_NONE;
}
#endif

/* inserts the selected char */
static void kbd_insert_selected(struct keyboard_parameters *pm,
                                struct edit_state *state)
{
    /* find input char */
    unsigned short ch = get_kbd_ch(pm, pm->x, pm->y);

    /* check for hangul input */
    if (ch >= 0x3131 && ch <= 0x3163)
    {
        unsigned short tmp;

        if (!state->hangul)
        {
            state->hlead = state->hvowel = state->htail = 0;
            state->hangul = true;
        }

        if (!state->hvowel)
        {
            state->hvowel = ch;
        }
        else if (!state->htail)
        {
            state->htail = ch;
        }
        else
        {
            /* previous hangul complete */
            /* check whether tail is actually lead of next char */
            tmp = hangul_join(state->htail, ch, 0);

            if (tmp != 0xfffd)
            {
                tmp = hangul_join(state->hlead, state->hvowel, 0);
                kbd_delchar(state);
                kbd_inschar(state, tmp);
                /* insert dummy char */
                kbd_inschar(state, ' ');
                state->hlead = state->htail;
                state->hvowel = ch;
                state->htail = 0;
            }
            else
            {
                state->hvowel = state->htail = 0;
                state->hlead = ch;
            }
        }

        /* combine into hangul */
        tmp = hangul_join(state->hlead, state->hvowel, state->htail);

        if (tmp != 0xfffd)
        {
            kbd_delchar(state);
            ch = tmp;
        }
        else
        {
            state->hvowel = state->htail = 0;
            state->hlead = ch;
        }
    }
    else
    {
        state->hangul = false;
    }

    /* insert char */
    kbd_inschar(state, ch);
}

static void kbd_backspace(struct edit_state *state)
{
    unsigned short ch;
    if (state->hangul)
    {
        if (state->htail)
            state->htail = 0;
        else if (state->hvowel)
            state->hvowel = 0;
        else
            state->hangul = false;
    }

    kbd_delchar(state);

    if (state->hangul)
    {
        if (state->hvowel)
            ch = hangul_join(state->hlead, state->hvowel, state->htail);
        else
            ch = state->hlead;
        kbd_inschar(state, ch);
    }
}

static void kbd_move_cursor(struct edit_state *state, int dir)
{
    state->hangul = false;
    state->editpos += dir;

    if (state->editpos >= 0 && state->editpos <= state->len_utf8)
    {
        state->changed = CHANGED_CURSOR;
    }
    else if (state->editpos > state->len_utf8)
    {
        state->editpos = 0;
        if (global_settings.talk_menu) beep_play(1000, 150, 1500);
    }
    else if (state->editpos < 0)
    {
        state->editpos = state->len_utf8;
        if (global_settings.talk_menu) beep_play(1000, 150, 1500);
    }
}

static void kbd_move_picker_horizontal(struct keyboard_parameters *pm,
                                       struct edit_state *state, int dir)
{
    state->changed = CHANGED_PICKER;

    pm->x += dir;
    if (pm->x < 0)
    {
        if (--pm->page < 0)
            pm->page = pm->pages - 1;
        pm->x = pm->max_chars - 1;
    }
    else if (pm->x >= pm->max_chars)
    {
        if (++pm->page >= pm->pages)
            pm->page = 0;
        pm->x = 0;
    }
}

static void kbd_move_picker_vertical(struct keyboard_parameters *pm,
                                     struct edit_state *state, int dir)
{
    state->changed = CHANGED_PICKER;

#ifdef HAVE_MORSE_INPUT
    if (state->morse_mode)
    {
        pm->line_edit = !pm->line_edit;
        return;
    }
#endif /* HAVE_MORSE_INPUT */

    pm->y += dir;
    if (pm->line_edit)
    {
        pm->y = (dir > 0 ? 0 : pm->lines - 1);
        pm->line_edit = false;
    }
    else if (pm->y < 0 || pm->y >= pm->lines)
    {
        pm->line_edit = true;
    }
}
