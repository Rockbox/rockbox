/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* ideas for improvement:
 *  hyphenation of long words */

#include "plugin.h"

#include "fixedpoint.h"

#include "lib/helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

#define LINE_LEN 1024
#define WORD_MAX 64

#define MIN_WPM 100
#define MAX_WPM 1000
#define DEF_WPM 250
#define WPM_INCREMENT 25

/* mininum bytes to skip when seeking */
#define SEEK_INTERVAL 100

#define FOCUS_X (7 * LCD_WIDTH / 20)
#define FOCUS_Y 0

#define FRAME_COLOR LCD_BLACK
#define BACKGROUND_COLOR LCD_WHITE /* inside frame */

#ifdef HAVE_LCD_COLOR
#define WORD_COLOR LCD_RGBPACK(48,48,48)
#define FOCUS_COLOR LCD_RGBPACK(204,0,0)
#define OUTSIDE_COLOR LCD_RGBPACK(128,128,128)
#define BAR_COLOR LCD_RGBPACK(230,230,230)
#else
#define WORD_COLOR LCD_BLACK
#define OUTSIDE_COLOR BACKGROUND_COLOR
#endif

#define BOOKMARK_FILE VIEWERS_DATA_DIR "/speedread.dat"
#define CONFIG_FILE VIEWERS_DATA_DIR "/speedread.cfg"

#define ANIM_TIME (75 * HZ / 100)

int fd = -1; /* -1 = prescripted demo */

int word_num; /* which word on a line */
off_t line_offs, begin_offs; /* offsets from the "real" beginning of the file to the current line and end of BOM */

int line_len = -1, custom_font = FONT_UI;

const char *last_word = NULL;

static const char *get_next_word(void)
{
    if(fd >= 0)
    {
        static char line_buf[LINE_LEN];
        static char *end = NULL;

    next_line:

        if(line_len < 0)
        {
            line_offs = rb->lseek(fd, 0, SEEK_CUR);
            line_len = rb->read_line(fd, line_buf, LINE_LEN);
            if(line_len <= 0)
                return NULL;

            char *word = rb->strtok_r(line_buf, " ", &end);

            word_num = 0;

            if(!word)
                goto next_line;
            else
            {
                last_word = word;
                return word;
            }
        }

        char *word = rb->strtok_r(NULL, " ", &end);
        if(!word)
        {
            /* end of line */
            line_len = -1;
            goto next_line;
        }
        ++word_num;

        last_word = word;
        return word;
    }
    else
    {
        /* feed the user a quick demo */
        static const char *words[] = { "This", "plugin", "is", "for", "speed-reading", "plain", "text", "files.",
                                 "Please", "open", "a", "plain", "text", "file", "to", "read", "by", "using", "the", "context", "menu.",
                                 "Have", "a", "nice", "day!" };
        static unsigned idx = 0;
        if(idx + 1 > ARRAYLEN(words))
            return NULL;
        last_word = words[idx++];
        return last_word;
    }
}

static const char *get_last_word(void)
{
    if(last_word)
        return last_word;
    else
    {
        last_word = get_next_word();
        return last_word;
    }
}

static void cleanup(void)
{
    if(custom_font != FONT_UI)
        rb->font_unload(custom_font);
    backlight_use_settings();
}

/* returns height of drawn area */
static int reset_drawing(long proportion) /* 16.16 fixed point, goes from 0 --> 1 over time */
{
    int w, h;
    rb->lcd_getstringsize("X", &w, &h);

    /* clear word area */
    rb->lcd_set_foreground(BACKGROUND_COLOR);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, h * 3 / 2);

    if(proportion)
    {
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(BAR_COLOR);
#endif
        rb->lcd_fillrect(fp_mul(proportion, FOCUS_X << 16, 16) >> 16, FOCUS_Y, fp_mul((1 << 16) - proportion, LCD_WIDTH << 16, 16) >> 16, h * 3 / 2);
    }

    rb->lcd_set_foreground(FRAME_COLOR);

    /* draw frame */
    rb->lcd_fillrect(0, h * 3 / 2, LCD_WIDTH, w / 2);
    rb->lcd_fillrect(FOCUS_X - w / 4, FOCUS_Y + h * 5 / 4, w / 2, h / 2);

    rb->lcd_set_foreground(WORD_COLOR);

    return h * 3 / 2 + w / 2;
}

static void render_word(const char *word, int focus)
{
    /* focus char first */
    char buf[5] = { 0, 0, 0, 0, 0 };
    int idx = rb->utf8seek(word, focus);
    rb->memcpy(buf, word + idx, MIN(rb->utf8seek(word, focus + 1) - idx, 4));

    int focus_w;
    rb->lcd_getstringsize(buf, &focus_w, NULL);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(FOCUS_COLOR);
#endif

    rb->lcd_putsxy(FOCUS_X - focus_w / 2, FOCUS_Y, buf);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(WORD_COLOR);
#endif

    /* figure out how far left to shift */
    static char half[WORD_MAX];
    rb->strlcpy(half, word, rb->utf8seek(word, focus + 1));
    int w;
    rb->lcd_getstringsize(half, &w, NULL);

    int x = FOCUS_X - focus_w / 2 - w;

    /* first half */
    rb->lcd_putsxy(x, FOCUS_Y, half);

    /* second half */
    x = FOCUS_X + focus_w / 2;
    rb->lcd_putsxy(x, FOCUS_Y, word + rb->utf8seek(word, focus + 1));
}

static int calculate_focus(const char *word)
{
#if 0
    int len = rb->utf8length(word);
    int focus = -1;
    for(int i = len / 5; i < len / 2; ++i)
    {
        switch(tolower(word[rb->utf8seek(word, i)]))
        {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            focus = i;
            break;
        default:
            break;
        }
    }

    if(focus < 0)
        focus = len / 2;
    return focus;
#else
    int len = rb->utf8length(word);
    if(rb->utf8length(word) > 13)
        return 4;
    else
    {
        int tab[] = {0,1,1,1,1,2,2,2,2,3,3,3,3};
        return tab[len - 1];
    }
#endif
}

static int calculate_delay(const char *word, int wpm)
{
    long base = 60 * HZ / wpm;
    long timeout = base;
    int len = rb->utf8length(word);

    if(len > 6)
        timeout += base / 5 * (len - 6);

    if(rb->strchr(word, ',') || rb->strchr(word, '-'))
        timeout += base / 2;

    if(rb->strchr(word, '.') || rb->strchr(word, '!') || rb->strchr(word, '?') || rb->strchr(word, ';'))
        timeout += 3 * base / 2;
    return timeout;
}

static long render_screen(const char *word, int wpm)
{
    /* significant inspiration taken from spread0r */
    long timeout = calculate_delay(word, wpm);
    int focus = calculate_focus(word);

    rb->lcd_setfont(custom_font);

    int h = reset_drawing(0);

    render_word(word, focus);

    rb->lcd_setfont(FONT_UI);

    rb->lcd_update_rect(0, 0, LCD_WIDTH, h);
    return timeout;
}

static void begin_anim(void)
{
    long start = *rb->current_tick;
    long end = start + ANIM_TIME;

    const char *word = get_last_word();

    int focus = calculate_focus(word);

    rb->lcd_setfont(custom_font);

    while(*rb->current_tick < end)
    {
        int h = reset_drawing(fp_div((*rb->current_tick - start) << 16, ANIM_TIME << 16, 16));

        render_word(word, focus);
        rb->lcd_update_rect(0, 0, LCD_WIDTH, h);
    }

    rb->lcd_setfont(FONT_UI);
}

static void init_drawing(void)
{
    backlight_ignore_timeout();
    atexit(cleanup);

    rb->lcd_set_background(OUTSIDE_COLOR);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_clear_display();

    rb->lcd_update();
}

enum { NOTHING = 0, SLOWER, FASTER, FFWD, BACK, PAUSE, QUIT };

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

static int get_useraction(void)
{
    int button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts));

    switch(button)
    {
#ifdef HAVE_SCROLLWHEEL
    case PLA_SCROLL_FWD:
    case PLA_SCROLL_FWD_REPEAT:
#else
    case PLA_UP:
#endif
        return FASTER;
#ifdef HAVE_SCROLLWHEEL
    case PLA_SCROLL_BACK:
    case PLA_SCROLL_BACK_REPEAT:
#else
    case PLA_DOWN:
#endif
        return SLOWER;
    case PLA_SELECT:
        return PAUSE;
    case PLA_CANCEL:
        return QUIT;
    case PLA_LEFT_REPEAT:
    case PLA_LEFT:
        return BACK;
    case PLA_RIGHT_REPEAT:
    case PLA_RIGHT:
        return FFWD;
    default:
        exit_on_usb(button); /* handle poweroff and USB events */
        return 0;
    }
}

static void save_bookmark(const char *fname, int wpm)
{
    if(!fname)
        return;
    rb->splash(0, "Saving...");
    /* copy every line except the one to be changed */
    int bookmark_fd = rb->open(BOOKMARK_FILE, O_RDONLY);
    int tmp_fd = rb->open(BOOKMARK_FILE ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(bookmark_fd >= 0)
    {
        while(1)
        {
            /* space for the filename, 3, integers, and a null */
            static char line[MAX_PATH + 1 + 10 + 1 + 10 + 1 + 10 + 1];
            int len = rb->read_line(bookmark_fd, line, sizeof(line));
            if(len <= 0)
                break;

            char *end;
            rb->strtok_r(line, " ", &end);
            rb->strtok_r(NULL, " ", &end);
            rb->strtok_r(NULL, " ", &end);
            char *bookmark_name = rb->strtok_r(NULL, "", &end);

            if(!bookmark_name)
                continue; /* avoid crash */
            if(rb->strcmp(fname, bookmark_name))
            {
                /* go back and clean up after strtok */
                for(int i = 0; i < len - 1; ++i)
                    if(!line[i])
                        line[i] = ' ';

                rb->write(tmp_fd, line, len);
                rb->fdprintf(tmp_fd, "\n");
            }
        }
        rb->close(bookmark_fd);
    }
    rb->fdprintf(tmp_fd, "%ld %d %d %s\n", line_offs, word_num, wpm, fname);
    rb->close(tmp_fd);
    rb->rename(BOOKMARK_FILE ".tmp", BOOKMARK_FILE);
}

static bool load_bookmark(const char *fname, int *wpm)
{
    int bookmark_fd = rb->open(BOOKMARK_FILE, O_RDONLY);
    if(bookmark_fd >= 0)
    {
        while(1)
        {
            /* space for the filename, 2 integers, and a null */
            char line[MAX_PATH + 1 + 10 + 1 + 10 + 1];
            int len = rb->read_line(bookmark_fd, line, sizeof(line));
            if(len <= 0)
                break;

            char *end;
            char *tok = rb->strtok_r(line, " ", &end);
            if(!tok)
                continue;
            off_t offs = rb->atoi(tok);

            tok = rb->strtok_r(NULL, " ", &end);
            if(!tok)
                continue;
            int word = rb->atoi(tok);

            tok = rb->strtok_r(NULL, " ", &end);
            if(!tok)
                continue;
            *wpm = rb->atoi(tok);
            if(*wpm < MIN_WPM)
                *wpm = MIN_WPM;
            if(*wpm > MAX_WPM)
                *wpm = MAX_WPM;

            char *bookmark_name = rb->strtok_r(NULL, "", &end);

            if(!bookmark_name)
                continue;

            if(!rb->strcmp(fname, bookmark_name))
            {
                rb->lseek(fd, offs, SEEK_SET);
                for(int i = 0; i < word; ++i)
                    get_next_word();
                rb->close(bookmark_fd);
                return true;
            }
        }
        rb->close(bookmark_fd);
    }
    return false;
}

static void new_font(const char *path)
{
    if(custom_font != FONT_UI)
        rb->font_unload(custom_font);
    custom_font = rb->font_load(path);
    if(custom_font < 0)
        custom_font = FONT_UI;
}

static void save_font(const char *path)
{
    int font_fd = rb->open(CONFIG_FILE, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    rb->write(font_fd, path, rb->strlen(path));
    rb->close(font_fd);
}

static char font_buf[MAX_PATH + 1];

static void load_font(void)
{
    int font_fd = rb->open(CONFIG_FILE, O_RDONLY);
    if(font_fd < 0)
        return;
    int len = rb->read(font_fd, font_buf, MAX_PATH);
    font_buf[len] = '\0';
    rb->close(font_fd);
    new_font(font_buf);
}

static void font_menu(void)
{
    /* taken from text_viewer */
    struct browse_context browse;
    char font[MAX_PATH], name[MAX_FILENAME+10];

    rb->snprintf(name, sizeof(name), "%s.fnt", rb->global_settings->font_file);
    rb->browse_context_init(&browse, SHOW_FONT,
                            BROWSE_SELECTONLY|BROWSE_NO_CONTEXT_MENU,
                            "Font", Icon_Menu_setting, FONT_DIR, name);

    browse.buf = font;
    browse.bufsize = sizeof(font);

    rb->rockbox_browse(&browse);

    if (browse.flags & BROWSE_SELECTED)
    {
        new_font(font);
        save_font(font);
    }
}

static bool confirm_restart(void)
{
    const struct text_message prompt = { (const char*[]) {"Are you sure?", "This will erase your current position."}, 2};
    enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
    if(response == YESNO_NO)
        return false;
    else
        return true;
}

static int config_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Speedread Menu", NULL,
                        "Resume Reading",
                        "Restart from Beginning",
                        "Change Font",
                        "Quit");
    int rc = 0;
    int sel = 0;
    while(!rc)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rc = 1;
            break;
        case 1:
            if(fd >= 0 && confirm_restart())
            {
                rb->lseek(fd, begin_offs, SEEK_SET);
                line_len = -1;
                get_next_word();
                rc = 1;
            }
            break;
        case 2:
            font_menu();
            break;
        case 3:
            rc = 2;
            break;
        default:
            break;
        }
    }
    return rc - 1;
}

enum { SKIP = -1, FINISH = -2 };

static int poll_input(int *wpm, long *clear, const char *fname, off_t file_size)
{
    switch(get_useraction())
    {
    case FASTER:
        if(*wpm + WPM_INCREMENT <= MAX_WPM)
            *wpm += WPM_INCREMENT;
        rb->splashf(0, "%d wpm", *wpm);
        *clear = *rb->current_tick + HZ;
        break;
    case SLOWER:
        if(*wpm - WPM_INCREMENT >= MIN_WPM)
            *wpm -= WPM_INCREMENT;
        rb->splashf(0, "%d wpm", *wpm);
        *clear = *rb->current_tick + HZ;
        break;
    case FFWD:
        if(fd >= 0)
        {
            off_t base_offs = rb->lseek(fd, 0, SEEK_CUR);
            off_t offs = 0;

            do {
                offs += SEEK_INTERVAL;
                if(offs >= 1000 * SEEK_INTERVAL)
                    offs += 199 * SEEK_INTERVAL;
                else if(offs >= 100 * SEEK_INTERVAL)
                    offs += 99 * SEEK_INTERVAL;
                else if(offs >= 10 * SEEK_INTERVAL)
                    offs += 9 * SEEK_INTERVAL;
                rb->splashf(0, "%ld/%ld bytes", offs + base_offs, file_size);
                rb->sleep(HZ/20);
            } while(get_useraction() == FFWD && offs + base_offs < file_size && offs + base_offs >= 0);

            *clear = *rb->current_tick + HZ;

            rb->lseek(fd, offs, SEEK_CUR);

            /* discard the next word (or more likely, portion of a word) */
            line_len = -1;
            get_next_word();

            return SKIP;
        }
        break;
    case BACK:
        if(fd >= 0)
        {
            off_t base_offs = rb->lseek(fd, 0, SEEK_CUR);
            off_t offs = 0;

            do {
                offs -= SEEK_INTERVAL;
                if(offs <= -1000 * SEEK_INTERVAL)
                    offs -= 199 * SEEK_INTERVAL;
                else if(offs <= -100 * SEEK_INTERVAL)
                    offs -= 99 * SEEK_INTERVAL;
                else if(offs <= -10 * SEEK_INTERVAL)
                    offs -= 9 * SEEK_INTERVAL;
                rb->splashf(0, "%ld/%ld bytes", offs + base_offs, file_size);
                rb->sleep(HZ/20);
            } while(get_useraction() == FFWD && offs + base_offs < file_size && offs + base_offs >= 0);

            *clear = *rb->current_tick + HZ;

            rb->lseek(fd, offs, SEEK_CUR);

            /* discard the next word (or more likely, portion of a word) */
            line_len = -1;
            get_next_word();

            return SKIP;
        }
        break;
    case PAUSE:
    case QUIT:
        if(config_menu())
        {
            save_bookmark(fname, *wpm);
            return FINISH;
        }
        else
        {
            init_drawing();
            begin_anim();
        }
        break;
    case NOTHING:
    default:
        break;
    }
    return 0;
}

enum plugin_status plugin_start(const void *param)
{
    const char *fname = param;

    off_t file_size = 0;

    load_font();

    bool loaded = false;

    int wpm = DEF_WPM;

    if(fname)
    {
        fd = rb->open_utf8(fname, O_RDONLY);

        begin_offs = rb->lseek(fd, 0, SEEK_CUR); /* skip BOM */
        file_size = rb->lseek(fd, 0, SEEK_END);
        rb->lseek(fd, begin_offs, SEEK_SET);

        loaded = load_bookmark(fname, &wpm);
    }

    init_drawing();

    long clear = -1;
    if(loaded)
    {
        rb->splash(0, "Loaded bookmark.");
        clear = *rb->current_tick + HZ;
    }

    begin_anim();

    /* main loop */
    while(1)
    {
        switch(poll_input(&wpm, &clear, fname, file_size))
        {
        case SKIP:
            continue;
        case FINISH:
            goto done;
        default:
            break;
        }

        const char *word = get_next_word();
        if(!word)
            break;
        bool want_full_update = false;
        if(TIME_AFTER(*rb->current_tick, clear) && clear != -1)
        {
            clear = -1;
            rb->lcd_clear_display();
            want_full_update = true;
        }
        long interval = render_screen(word, wpm);

        long frame_done = *rb->current_tick + interval;

        if(want_full_update)
            rb->lcd_update();

        while(!TIME_AFTER(*rb->current_tick, frame_done))
        {
            switch(poll_input(&wpm, &clear, fname, file_size))
            {
            case SKIP:
                goto next_word;
            case FINISH:
                goto done;
            default:
                break;
            }
            rb->yield();
        }
    next_word:
        ;
    }

done:
    rb->close(fd);

    return PLUGIN_OK;
}
