/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Franklin Wei
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

#include "plugin.h"

#include "puzzles.h"

#include "lib/pluginlib_actions.h"
#include "lib/xlcd.h"

/* how many ticks between timer callbacks */
#define TIMER_INTERVAL (HZ / 50)
#define BG_R .9f
#define BG_G .9f
#define BG_B .9f

#define BG_COLOR LCD_RGBPACK((int)(255*BG_R), (int)(255*BG_R), (int)(255*BG_B))

static midend *me = NULL;
static unsigned *colors = NULL;
static int ncolors = 0;

static void init_for_game(const game *gm);
static void fix_size(void);

static void rb_start_draw(void *handle)
{
}

static struct viewport clip_rect;
static bool clipped = false;

static void rb_clip(void *handle, int x, int y, int w, int h)
{
    LOGF("rb_clip(%d %d %d %d)", x, y, w, h);
    clip_rect.x = x;
    clip_rect.y = y;
    clip_rect.width  = w;
    clip_rect.height = h;
    clip_rect.font = FONT_UI;
    clip_rect.drawmode = DRMODE_SOLID;
#if LCD_DEPTH > 1
    clip_rect.fg_pattern = LCD_DEFAULT_FG;
    clip_rect.bg_pattern = LCD_DEFAULT_BG;
#endif
    rb->lcd_set_viewport(NULL);
    rb->lcd_set_viewport(&clip_rect);
    clipped = true;
}

static void rb_unclip(void *handle)
{
    LOGF("rb_unclip");
    rb->lcd_set_viewport(NULL);
    clipped = false;
}

static void offset_coords(int *x, int *y)
{
    if(clipped)
    {
        *x -= clip_rect.x;
        *y -= clip_rect.y;
    }
}

static void rb_color(int n)
{
    if(n < 0)
    {
        fatal("bad color %d", n);
        return;
    }
    rb->lcd_set_foreground(colors[n]);
}

static void rb_draw_text(void *handle, int x, int y, int fonttype,
                         int fontsize, int align, int colour, char *text)
{
    LOGF("rb_draw_text(%d %d %s)", x, y, text);

    offset_coords(&x, &y);

    switch(fonttype)
    {
    case FONT_FIXED:
        rb->lcd_setfont(FONT_SYSFIXED);
        break;
    case FONT_VARIABLE:
        rb->lcd_setfont(FONT_UI);
        break;
    default:
        fatal("bad font");
        break;
    }

    int w, h;
    rb->lcd_getstringsize(text, &w, &h);

    static int cap_h = -1;
    if(cap_h < 0)
        rb->lcd_getstringsize("X", NULL, &cap_h);

    if(align & ALIGN_VNORMAL)
        y -= h;
    else if(align & ALIGN_VCENTRE)
        y -= cap_h / 2;

    if(align & ALIGN_HCENTRE)
        x -= w / 2;
    else if(align & ALIGN_HRIGHT)
        x -= w;

    rb_color(colour);
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_putsxy(x, y, text);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

static void rb_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
    LOGF("rb_draw_rect(%d, %d, %d, %d, %d)", x, y, w, h, colour);
    rb_color(colour);
    offset_coords(&x, &y);
    rb->lcd_fillrect(x, y, w, h);
}

static void rb_draw_line(void *handle, int x1, int y1, int x2, int y2,
                         int colour)
{
    LOGF("rb_draw_line(%d, %d, %d, %d, %d)", x1, y1, x2, y2, colour);
    offset_coords(&x1, &y1);
    offset_coords(&x2, &y2);
    rb_color(colour);
    rb->lcd_drawline(x1, y1, x2, y2);
}

static void rb_draw_poly(void *handle, int *coords, int npoints,
                         int fillcolour, int outlinecolour)
{
    LOGF("rb_draw_poly");
    if(fillcolour >= 0)
    {
        rb_color(fillcolour);
        for(int i = 2; i < npoints; ++i)
        {
            int x1, y1, x2, y2, x3, y3;
            x1 = coords[0];
            y1 = coords[1];
            x2 = coords[(i - 1) * 2];
            y2 = coords[(i - 1) * 2 + 1];
            x3 = coords[i * 2];
            y3 = coords[i * 2 + 1];
            offset_coords(&x1, &y1);
            offset_coords(&x2, &y2);
            offset_coords(&x3, &y3);
            xlcd_filltriangle(x1, y1,
                              x2, y2,
                              x3, y3);
        }
    }

    assert(outlinecolour >= 0);
    rb_color(outlinecolour);

    for(int i = 1; i < npoints; ++i)
    {
        int x1, y1, x2, y2;
        x1 = coords[2 * (i - 1)];
        y1 = coords[2 * (i - 1) + 1];
        x2 = coords[2 * i];
        y2 = coords[2 * i + 1];
        offset_coords(&x1, &y1);
        offset_coords(&x2, &y2);
        rb->lcd_drawline(x1, y1,
                         x2, y2);
    }

    int x1, y1, x2, y2;
    x1 = coords[0];
    y1 = coords[1];
    x2 = coords[2 * (npoints - 1)];
    y2 = coords[2 * (npoints - 1) + 1];
    offset_coords(&x1, &y1);
    offset_coords(&x2, &y2);

    rb->lcd_drawline(x1, y1,
                     x2, y2);
}

static void rb_draw_circle(void *handle, int cx, int cy, int radius,
                           int fillcolour, int outlinecolour)
{
    LOGF("rb_draw_circle(%d, %d, %d)", cx, cy, radius);
    offset_coords(&cx, &cy);
    if(fillcolour >= 0)
    {
        rb_color(fillcolour);
        xlcd_fillcircle(cx, cy, radius);
    }
    assert(outlinecolour >= 0);
    rb_color(outlinecolour);
    xlcd_drawcircle(cx, cy, radius);
}

struct blitter {
    bool have_data;
    int x, y;
    struct bitmap bmp;
};

static blitter *rb_blitter_new(void *handle, int w, int h)
{
    LOGF("rb_blitter_new");
    blitter *b = snew(blitter);
    b->bmp.width = w;
    b->bmp.height = h;
    b->bmp.data = smalloc(w * h * sizeof(fb_data));
    b->have_data = false;
    return b;
}

static void rb_blitter_free(void *handle, blitter *bl)
{
    LOGF("rb_blitter_free");
    sfree(bl->bmp.data);
    sfree(bl);
    return;
}

static void trim_rect(int *x, int *y, int *w, int *h)
{
    int x0, x1, y0, y1;

    /*
     * Reduce the size of the copied rectangle to stop it going
     * outside the bounds of the canvas.
     */

    /* Transform from x,y,w,h form into coordinates of all edges */
    x0 = *x;
    y0 = *y;
    x1 = *x + *w;
    y1 = *y + *h;

    /* Clip each coordinate at both extremes of the canvas */
    x0 = (x0 < 0 ? 0 : x0 > LCD_WIDTH ? LCD_WIDTH : x0);
    x1 = (x1 < 0 ? 0 : x1 > LCD_WIDTH ? LCD_WIDTH : x1);
    y0 = (y0 < 0 ? 0 : y0 > LCD_HEIGHT ? LCD_HEIGHT : y0);
    y1 = (y1 < 0 ? 0 : y1 > LCD_HEIGHT ? LCD_HEIGHT : y1);

    /* Transform back into x,y,w,h to return */
    *x = x0;
    *y = y0;
    *w = x1 - x0;
    *h = y1 - y0;
}

/* copy a section of the framebuffer */
static void rb_blitter_save(void *handle, blitter *bl, int x, int y)
{
    /* no viewport offset */
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
#error no vertical stride
#else
    if(bl->bmp.data)
    {
        int w = bl->bmp.width, h = bl->bmp.height;
        trim_rect(&x, &y, &w, &h);
        LOGF("rb_blitter_save(%d, %d, %d, %d)", x, y, w, h);
        for(int i = 0; i < h; ++i)
        {
            /* copy line-by-line */
            rb->memcpy(bl->bmp.data + sizeof(fb_data) * i * w,
                       rb->lcd_framebuffer + (y + i) * LCD_WIDTH + x,
                       w * sizeof(fb_data));
        }
        bl->x = x;
        bl->y = y;
        bl->have_data = true;
    }
#endif
}

static void rb_blitter_load(void *handle, blitter *bl, int x, int y)
{
    LOGF("rb_blitter_load");
    if(!bl->have_data)
        return;
    int w = bl->bmp.width, h = bl->bmp.height;

    if(x == BLITTER_FROMSAVED) x = bl->x;
    if(y == BLITTER_FROMSAVED) y = bl->y;

    trim_rect(&x, &y, &w, &h);
    offset_coords(&x, &y);
    rb->lcd_bitmap(bl->bmp.data, x, y, w, h);
}

static void rb_draw_update(void *handle, int x, int y, int w, int h)
{
    LOGF("rb_draw_update(%d, %d, %d, %d)", x, y, w, h);
    rb->lcd_update_rect(x, y, w, h);
}

static void rb_end_draw(void *handle)
{
    LOGF("rb_end_draw");
}

static void rb_status_bar(void *handle, char *text)
{
    LOGF("game title is %s\n", text);
}

static char *rb_text_fallback(void *handle, const char *const *strings,
                              int nstrings)
{
    return dupstr(strings[0]);
}

const drawing_api rb_drawing = {
    rb_draw_text,
    rb_draw_rect,
    rb_draw_line,
    rb_draw_poly,
    rb_draw_circle,
    rb_draw_update,
    rb_clip,
    rb_unclip,
    rb_start_draw,
    rb_end_draw,
    rb_status_bar,
    rb_blitter_new,
    rb_blitter_free,
    rb_blitter_save,
    rb_blitter_load,
    NULL, NULL, NULL, NULL, NULL, NULL, /* {begin,end}_{doc,page,puzzle} */
    NULL, NULL,                        /* line_width, line_dotted */
    rb_text_fallback,
    NULL,
};

void frontend_default_colour(frontend *fe, float *out)
{
    *out++ = 0.9;
    *out++ = 0.9;
    *out++ = 0.9;
}

void fatal(char *fmt, ...)
{
    va_list ap;

    rb->splash(HZ, "FATAL ERROR");

    va_start(ap, fmt);
    char buf[80];
    rb->vsnprintf(buf, 80, fmt, ap);
    rb->splash(HZ * 2, buf);
    va_end(ap);

    exit(1);
}

void get_random_seed(void **randseed, int *randseedsize)
{
    *randseed = snew(long);
    long seed = *rb->current_tick;
    rb->memcpy(*randseed, &seed, sizeof(seed));
    //*(long*)*randseed = 42; // debug
    //rb->splash(HZ, "DEBUG SEED ON");
    *randseedsize = sizeof(long);
}

const char *config_choices_formatter(int sel, void *data, char *buf, size_t len)
{
    /* we can't rely on being called in any particular order */
    char *list = dupstr(data);
    char delimbuf[2] = { *list, 0 };
    char *save = NULL;
    char *str = rb->strtok_r(list, delimbuf, &save);
    for(int i = 0; i < sel; ++i)
        str = rb->strtok_r(NULL, delimbuf, &save);
    rb->snprintf(buf, len, "%s", str);
    sfree(list);
    return buf;
}

static int list_choose(const char *list_str, const char *title)
{
    char delim = *list_str;

    const char *ptr = list_str + 1;
    int n = 0;
    while(ptr)
    {
        n++;
        ptr = strchr(ptr + 1, delim);
    }

    struct gui_synclist list;

    rb->gui_synclist_init(&list, &config_choices_formatter, list_str, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, n);
    rb->gui_synclist_limit_scroll(&list, false);

    rb->gui_synclist_select_item(&list, 0);

    rb->gui_synclist_set_title(&list, title, NOICON);
    while (1)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;
        switch(button)
        {
        case ACTION_STD_OK:
            return rb->gui_synclist_get_sel_pos(&list);
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
            return -1;
        default:
            break;
        }
    }
}

static void do_configure_item(config_item *cfg)
{
    switch(cfg->type)
    {
    case C_STRING:
    {
#define MAX_STRLEN 128
        char *newstr = smalloc(MAX_STRLEN);
        rb->strlcpy(newstr, cfg->sval, MAX_STRLEN);
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_set_background(LCD_BLACK);
        if(rb->kbd_input(newstr, MAX_STRLEN) < 0)
        {
            sfree(newstr);
            break;
        }
        sfree(cfg->sval);
        cfg->sval = newstr;
        break;
    }
    case C_BOOLEAN:
    {
        bool res = cfg->ival != 0;
        rb->set_bool(cfg->name, &res);
        cfg->ival = res;
        break;
    }
    case C_CHOICES:
    {
        int sel = list_choose(cfg->sval, cfg->name);
        if(sel >= 0)
            cfg->ival = sel;
        break;
    }
    default:
        fatal("bad type");
        break;
    }
}

const char *config_formatter(int sel, void *data, char *buf, size_t len)
{
    config_item *cfg = data;
    cfg += sel;
    rb->snprintf(buf, len, "%s", cfg->name);
    return buf;
}

static void config_menu(void)
{
    char *title;
    config_item *config = midend_get_config(me, CFG_SETTINGS, &title);

    if(!config)
    {
        rb->splash(HZ, "Nothing to configure.");
        goto done;
    }

    /* count */
    int n = 0;
    config_item *ptr = config;
    while(ptr->type != C_END)
    {
        n++;
        ptr++;
    }

    /* display a list */
    struct gui_synclist list;

    rb->gui_synclist_init(&list, &config_formatter, config, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, n);
    rb->gui_synclist_limit_scroll(&list, false);

    rb->gui_synclist_select_item(&list, 0);

    bool done = false;
    rb->gui_synclist_set_title(&list, title, NOICON);
    while (!done)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;
        switch(button)
        {
        case ACTION_STD_OK:
        {
            config_item old;
            int pos = rb->gui_synclist_get_sel_pos(&list);
            memcpy(&old, config + pos, sizeof(old));
            do_configure_item(config + pos);
            char *err = midend_set_config(me, CFG_SETTINGS, config);
            if(err)
            {
                rb->splash(HZ, err);
                memcpy(config + pos, &old, sizeof(old));
            }
            break;
        }
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
            done = true;
            break;
        default:
            break;
        }
    }

done:
    sfree(title);
    free_cfg(config);
}

static int pause_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Paused", NULL,
                        "Resume Game",
                        "New Game",
                        "Undo",
                        "Redo",
                        "Solve",
                        "Configure Game",
                        "Select Another Game",
                        "Quit");
    bool quit = false;
    while(!quit)
    {
        int sel = 0;
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            quit = true;
            break;
        case 1:
            midend_new_game(me);
            fix_size();
            quit = true;
            break;
        case 2:
            if(!midend_can_undo(me))
                rb->splash(HZ, "Cannot undo.");
            else
                midend_process_key(me, 0, 0, 'u');
            quit = true;
            break;
        case 3:
            if(!midend_can_redo(me))
                rb->splash(HZ, "Cannot redo.");
            else
                midend_process_key(me, 0, 0, 'r');
            quit = true;
            break;
        case 4:
        {
            char *msg = midend_solve(me);
            if(msg)
                rb->splash(HZ, msg);
            quit = true;
            break;
        }
        case 5:
            config_menu();
            break;
        case 6:
            return -1;
        case 7:
            return -2;
        default:
            break;
        }
    }
    rb->lcd_set_background(BG_COLOR);
    rb->lcd_clear_display();
    rb->lcd_update();
    midend_force_redraw(me);
    return 0;
}

static bool want_redraw = true;

static int process_input(int tmo)
{
    int state = 0;
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false); /* about to block for button input */
#endif

    int button = pluginlib_getaction(tmo, plugin_contexts, ARRAYLEN(plugin_contexts));

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    switch(button)
    {
    case PLA_UP:
        state |= CURSOR_UP;
        break;
    case PLA_DOWN:
        state |= CURSOR_DOWN;
        break;
    case PLA_LEFT:
        state |= CURSOR_LEFT;
        break;
    case PLA_RIGHT:
        state |= CURSOR_RIGHT;
        break;
    case PLA_SELECT:
        state |= CURSOR_SELECT;
        break;
    case PLA_CANCEL:
        want_redraw = false;
        return pause_menu();
    default:
        exit_on_usb(button);
        break;
    }
    return state;
}

static long last_tstamp;

static void timer_cb(void)
{
    LOGF("timer callback");
    midend_timer(me, (float)(*rb->current_tick - last_tstamp) / (float)HZ);
    last_tstamp = *rb->current_tick;
}

static volatile bool timer_on = false;

void activate_timer(frontend *fe)
{
    last_tstamp = *rb->current_tick;
    timer_on = true;
}

void deactivate_timer(frontend *fe)
{
    timer_on = false;
}

/* can't use audio buffer */
char giant_buffer[1024*1024*4];

const char *formatter(char *buf, size_t n, int i, const char *unit)
{
    rb->snprintf(buf, n, "%s", gamelist[i]->name);
    return buf;
}

static void fix_size(void)
{
    int w = LCD_WIDTH, h = LCD_HEIGHT;
    midend_size(me, &w, &h, TRUE);
}

static void init_for_game(const game *gm)
{
    /* reset tlsf by nuking the signature */
    /* will make any already-allocated memory point to garbage */
    memset(giant_buffer, 0, 4);
    init_memory_pool(sizeof(giant_buffer), giant_buffer);

    me = midend_new(NULL, gm, &rb_drawing, NULL);
    midend_new_game(me);

    fix_size();

    float *floatcolors = midend_colours(me, &ncolors);

    /* convert them to packed RGB */
    colors = smalloc(ncolors * sizeof(unsigned));
    unsigned *ptr = colors;
    for(int i = 0; i < ncolors; ++i)
    {
        int r = 255 * *(floatcolors++);
        int g = 255 * *(floatcolors++);
        int b = 255 * *(floatcolors++);
        LOGF("color %d is %d %d %d", i, r, g, b);
        *ptr++ = LCD_RGBPACK(r, g, b);
    }

    /* seems to crash */
    /* actually it doesn't matter if this memory is leaked as
     * resetting tlsf makes it forget about it anyway */
    //sfree(floatcolors);

    rb->lcd_set_viewport(NULL);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(BG_COLOR);
    rb->lcd_clear_display();
    rb->lcd_update();

    midend_force_redraw(me);
}

static void exit_handler(void)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb_atexit(exit_handler);
    rb->cpu_boost(true);
#endif

    LOGF("acos(.5) = %f", acos(.5));
    LOGF("sqrt(3)/2 = sin(60) = %f = %f", sqrt(3)/2, sin(PI/3));

    int gm = 0;
    while(1)
    {
        if(rb->set_int("Choose Game", "", UNIT_INT, &gm, NULL, 1, 0, gamecount - 1, formatter))
            return PLUGIN_OK;

        init_for_game(gamelist[gm]);

        while(1)
        {
            want_redraw = true;

            int button = process_input(timer_on ? TIMER_INTERVAL : -1);

            if(button < 0)
            {
                rb_unclip(NULL);
                deactivate_timer(NULL);
                midend_free(me);
                /* new game */
                if(button == -1)
                    break;
                else if(button == -2)
                {
                    sfree(colors);
                    exit(PLUGIN_OK);
                }
            }

            if(button)
                midend_process_key(me, 0, 0, button);

            if(want_redraw)
                midend_redraw(me);

            if(timer_on)
                timer_cb();

            rb->yield();
        }
        sfree(colors);
    }
}
