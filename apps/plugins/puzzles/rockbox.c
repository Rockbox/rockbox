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

/* rockbox frontend for puzzles */

#include "plugin.h"

#include "puzzles.h"
#include "keymaps.h"

#ifndef COMBINED
#include "lib/playback_control.h"
#endif
#include "lib/xlcd.h"
#include "fixedpoint.h"

/* how many ticks between timer callbacks */
#define TIMER_INTERVAL (HZ / 50)
#define BG_R .9f /* very light gray */
#define BG_G .9f
#define BG_B .9f

#ifdef COMBINED
#define SAVE_FILE PLUGIN_GAMES_DATA_DIR "/puzzles.sav"
#else
static char save_file_path[MAX_PATH];
#define SAVE_FILE ((const char*)save_file_path)
#endif

#define BG_COLOR LCD_RGBPACK((int)(255*BG_R), (int)(255*BG_G), (int)(255*BG_B))

#define MURICA

#ifdef MURICA
#define midend_serialize midend_serialise
#define midend_deserialize midend_deserialise
#define frontend_default_color frontend_default_colour
#define midend_colors midend_colours
#endif

static midend *me = NULL;
static unsigned *colors = NULL;
static int ncolors = 0;
static long last_keystate = 0;

#ifdef FOR_REAL
/* the "debug menu" is hidden by default in order to avoid the
 * naturally ensuing complaints from users */
static bool debug_mode = false;
static int help_times = 0;
#endif

static void fix_size(void);

static struct viewport clip_rect;
static bool clipped = false;
extern bool audiobuf_available;

static struct settings_t {
    int slowmo_factor;
    bool bulk, timerflash, clipoff, shortcuts, no_aa;
} settings;

/* clipping is implemented through viewports and offsetting
 * coordinates */
static void rb_clip(void *handle, int x, int y, int w, int h)
{
    if(!settings.clipoff)
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
        rb->lcd_set_viewport(&clip_rect);
        clipped = true;
    }
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
                         int fontsize, int align, int color, char *text)
{
    (void) fontsize;
    LOGF("rb_draw_text(%d %d %s)", x, y, text);

    offset_coords(&x, &y);

    /* TODO: variable font size */
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

    rb_color(color);
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_putsxy(x, y, text);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

static void rb_draw_rect(void *handle, int x, int y, int w, int h, int color)
{
    LOGF("rb_draw_rect(%d, %d, %d, %d, %d)", x, y, w, h, color);
    rb_color(color);
    offset_coords(&x, &y);
    rb->lcd_fillrect(x, y, w, h);
}

#define SWAP(a, b, t) do { t = a; a = b; b = t; } while(0);

#define fp_fpart(f, bits) ((f) & ((1 << (bits)) - 1))
#define fp_rfpart(f, bits) ((1 << (bits)) - fp_fpart(f, bits))

#define FRACBITS 16

/* most of our time drawing lines is spent in this function! */
static inline void plot(unsigned x, unsigned y, unsigned long a,
                        unsigned long r1, unsigned long g1, unsigned long b1,
                        unsigned cl, unsigned cr, unsigned cu, unsigned cd)
{
    /* This is really quite possibly the least efficient way of doing
       this. A better way would be in draw_antialiased_line(), but the
       problem with that is that the algorithms I investigated at
       least were incorrect at least part of the time and didn't make
       drawing much faster overall. */
    if(!(cl <= x && x < cr && cu <= y && y < cd))
        return;

    fb_data *ptr = rb->lcd_framebuffer + y * LCD_WIDTH + x;
    fb_data orig = *ptr;
    unsigned long r2, g2, b2;
#if LCD_DEPTH != 24
    r2 = RGB_UNPACK_RED(orig);
    g2 = RGB_UNPACK_GREEN(orig);
    b2 = RGB_UNPACK_BLUE(orig);
#else
    r2 = orig.r;
    g2 = orig.g;
    b2 = orig.b;
#endif

    unsigned long r, g, b;
    r = ((r1 * a) + (r2 * (256 - a))) >> 8;
    g = ((g1 * a) + (g2 * (256 - a))) >> 8;
    b = ((b1 * a) + (b2 * (256 - a))) >> 8;

#if LCD_DEPTH != 24
    *ptr = LCD_RGBPACK(r, g, b);
#else
    *ptr = (fb_data) {b, g, r};
#endif
}

#undef ABS
#define ABS(a) ((a)<0?-(a):(a))

/* speed benchmark: 34392 lines/sec vs 112687 non-antialiased
 * lines/sec at full optimization on ipod6g */

/* expects UN-OFFSET coordinates, directly access framebuffer */
static void draw_antialiased_line(int x0, int y0, int x1, int y1)
{
    /* fixed-point Wu's algorithm, modified for integer-only endpoints */

    /* passed to plot() to avoid re-calculation */
    unsigned short l = 0, r = LCD_WIDTH, u = 0, d = LCD_HEIGHT;
    if(clipped)
    {
        l = clip_rect.x;
        r = clip_rect.x + clip_rect.width;
        u = clip_rect.y;
        d = clip_rect.y + clip_rect.height;
    }

    bool steep = ABS(y1 - y0) > ABS(x1 - x0);
    int tmp;
    if(steep)
    {
        SWAP(x0, y0, tmp);
        SWAP(x1, y1, tmp);
    }
    if(x0 > x1)
    {
        SWAP(x0, x1, tmp);
        SWAP(y0, y1, tmp);
    }

    int dx, dy;
    dx = x1 - x0;
    dy = y1 - y0;

    if(!(dx << FRACBITS))
        return; /* bail out */

    long gradient = fp_div(dy << FRACBITS, dx << FRACBITS, FRACBITS);
    long intery = (y0 << FRACBITS);

    unsigned color = rb->lcd_get_foreground();
    unsigned long r1, g1, b1;
    r1 = RGB_UNPACK_RED(color);
    g1 = RGB_UNPACK_GREEN(color);
    b1 = RGB_UNPACK_BLUE(color);

    /* main loop */
    if(steep)
    {
        for(int x = x0; x <= x1; ++x, intery += gradient)
        {
            unsigned y = intery >> FRACBITS;
            unsigned alpha = fp_fpart(intery, FRACBITS) >> (FRACBITS - 8);

            plot(y, x, (1 << 8) - alpha, r1, g1, b1, l, r, u, d);
            plot(y + 1, x, alpha, r1, g1, b1, l, r, u, d);
        }
    }
    else
    {
        for(int x = x0; x <= x1; ++x, intery += gradient)
        {
            unsigned y = intery >> FRACBITS;
            unsigned alpha = fp_fpart(intery, FRACBITS) >> (FRACBITS - 8);

            plot(x, y, (1 << 8) - alpha, r1, g1, b1, l, r, u, d);
            plot(x, y + 1, alpha, r1, g1, b1, l, r, u, d);
        }
    }
}

static void rb_draw_line(void *handle, int x1, int y1, int x2, int y2,
                         int color)
{
    LOGF("rb_draw_line(%d, %d, %d, %d, %d)", x1, y1, x2, y2, color);
    rb_color(color);
    if(settings.no_aa)
    {
        offset_coords(&x1, &y1);
        offset_coords(&x2, &y2);
        rb->lcd_drawline(x1, y1, x2, y2);
    }
    else
        draw_antialiased_line(x1, y1, x2, y2);
}

/*
 * draw filled polygon
 * originally by Sebastian Leonhardt (ulmutul)
 * 'count' : number of coordinate pairs
 * 'pxy': array of coordinates. pxy[0]=x0,pxy[1]=y0,...
 * note: provide space for one extra coordinate, because the starting point
 * will automatically be inserted as end point.
 */

/*
 * helper function:
 * find points of intersection between polygon and scanline
 */

#define MAX_INTERSECTION 32

static void fill_poly_line(int scanline, int count, int *pxy)
{
    int i;
    int j;
    int num_of_intersects;
    int direct, old_direct;
    //intersections of every line with scanline (y-coord)
    int intersection[MAX_INTERSECTION];
    /* add starting point as ending point */
    pxy[count*2] = pxy[0];
    pxy[count*2+1] = pxy[1];

    old_direct=0;
    num_of_intersects=0;
    for (i=0; i<count*2; i+=2) {
        int x1=pxy[i];
        int y1=pxy[i+1];
        int x2=pxy[i+2];
        int y2=pxy[i+3];
        // skip if line is outside of scanline
        if (y1 < y2) {
            if (scanline < y1 || scanline > y2)
                continue;
        }
        else {
            if (scanline < y2 || scanline > y1)
                continue;
        }
        // calculate x-coord of intersection
        if (y1==y2) {
            direct=0;
        }
        else {
            direct = y1>y2 ? 1 : -1;
            // omit double intersections, if both lines lead in the same direction
            intersection[num_of_intersects] =
                x1+((scanline-y1)*(x2-x1))/(y2-y1);
            if ( (direct!=old_direct)
                  || (intersection[num_of_intersects] != intersection[num_of_intersects-1])
                )
                ++num_of_intersects;
        }
        old_direct = direct;
    }

    // sort points of intersection
    for (i=0; i<num_of_intersects-1; ++i) {
        for (j=i+1; j<num_of_intersects; ++j) {
            if (intersection[j]<intersection[i]) {
                int temp=intersection[i];
                intersection[i]=intersection[j];
                intersection[j]=temp;
            }
        }
    }
    // draw
    for (i=0; i<num_of_intersects; i+=2) {
        rb->lcd_hline(intersection[i], intersection[i+1], scanline);
    }
}

/* two extra elements at end of pxy needed */
static void v_fillarea(int count, int *pxy)
{
    int i;
    int y1, y2;

    // find min and max y coords
    y1=y2=pxy[1];
    for (i=3; i<count*2; i+=2) {
        if (pxy[i] < y1) y1 = pxy[i];
        else if (pxy[i] > y2) y2 = pxy[i];
    }

    for (i=y1; i<=y2; ++i) {
        fill_poly_line(i, count, pxy);
    }
}

static void rb_draw_poly(void *handle, int *coords, int npoints,
                         int fillcolor, int outlinecolor)
{
    LOGF("rb_draw_poly");

    if(fillcolor >= 0)
    {
        rb_color(fillcolor);
#if 1
        /* serious hack: draw a bunch of triangles between adjacent points */
        /* this generally works, even with some concave polygons */
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

#if 0
            /* debug code */
            rb->lcd_set_foreground(LCD_RGBPACK(255,0,0));
            rb->lcd_drawpixel(x1, y1);
            rb->lcd_drawpixel(x2, y2);
            rb->lcd_drawpixel(x3, y3);
            rb->lcd_update();
            rb->sleep(HZ);
            rb_color(fillcolor);
            rb->lcd_drawpixel(x1, y1);
            rb->lcd_drawpixel(x2, y2);
            rb->lcd_drawpixel(x3, y3);
            rb->lcd_update();
#endif
        }
#else
        int *pxy = smalloc(sizeof(int) * 2 * npoints + 2);
        /* copy points, offsetted */
        for(int i = 0; i < npoints; ++i)
        {
            pxy[2 * i + 0] = coords[2 * i + 0];
            pxy[2 * i + 1] = coords[2 * i + 1];
            offset_coords(&pxy[2*i+0], &pxy[2*i+1]);
        }
        v_fillarea(npoints, pxy);
        sfree(pxy);
#endif
    }

    /* draw outlines last so they're not covered by the fill */
    assert(outlinecolor >= 0);
    rb_color(outlinecolor);

    for(int i = 1; i < npoints; ++i)
    {
        int x1, y1, x2, y2;
        x1 = coords[2 * (i - 1)];
        y1 = coords[2 * (i - 1) + 1];
        x2 = coords[2 * i];
        y2 = coords[2 * i + 1];
        if(settings.no_aa)
        {
            offset_coords(&x1, &y1);
            offset_coords(&x2, &y2);
            rb->lcd_drawline(x1, y1,
                             x2, y2);
        }
        else
            draw_antialiased_line(x1, y1, x2, y2);
    }

    int x1, y1, x2, y2;
    x1 = coords[0];
    y1 = coords[1];
    x2 = coords[2 * (npoints - 1)];
    y2 = coords[2 * (npoints - 1) + 1];
    if(settings.no_aa)
    {
        offset_coords(&x1, &y1);
        offset_coords(&x2, &y2);

        rb->lcd_drawline(x1, y1,
                         x2, y2);
    }
    else
        draw_antialiased_line(x1, y1, x2, y2);
}

static void rb_draw_circle(void *handle, int cx, int cy, int radius,
                           int fillcolor, int outlinecolor)
{
    LOGF("rb_draw_circle(%d, %d, %d)", cx, cy, radius);
    offset_coords(&cx, &cy);

    if(fillcolor >= 0)
    {
        rb_color(fillcolor);
        xlcd_fillcircle(cx, cy, radius - 1);
    }

    assert(outlinecolor >= 0);
    rb_color(outlinecolor);
    xlcd_drawcircle(cx, cy, radius - 1);
}

struct blitter {
    bool have_data;
    int x, y;
    struct bitmap bmp;
};

/* originally from emcc.c */
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
    x0 = (x0 < 0 ? 0 : x0 > LCD_WIDTH - 1 ? LCD_WIDTH - 1: x0);
    x1 = (x1 < 0 ? 0 : x1 > LCD_WIDTH - 1 ? LCD_WIDTH - 1: x1);
    y0 = (y0 < 0 ? 0 : y0 > LCD_HEIGHT - 1 ? LCD_HEIGHT - 1: y0);
    y1 = (y1 < 0 ? 0 : y1 > LCD_HEIGHT - 1 ? LCD_HEIGHT - 1: y1);

    /* Transform back into x,y,w,h to return */
    *x = x0;
    *y = y0;
    *w = x1 - x0;
    *h = y1 - y0;
}

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

    offset_coords(&x, &y);
    trim_rect(&x, &y, &w, &h);
    rb->lcd_bitmap((fb_data*)bl->bmp.data, x, y, w, h);
}

static bool need_draw_update = false;

static int ud_l = 0, ud_u = 0, ud_r = LCD_WIDTH, ud_d = LCD_HEIGHT;

static void rb_draw_update(void *handle, int x, int y, int w, int h)
{
    LOGF("rb_draw_update(%d, %d, %d, %d)", x, y, w, h);

    /* It seems that the puzzles use a different definition of
     * "updating" the display than Rockbox does; by calling this
     * function, it tells us that it has either already drawn to the
     * updated area (as rockbox assumes), or that it WILL draw to the
     * said area. Thus we simply remember a rectangle that contains
     * all the updated regions and update it at the very end. */

    /* adapted from gtk.c */
    if (!need_draw_update || ud_l > x  ) ud_l = x;
    if (!need_draw_update || ud_r < x+w) ud_r = x+w;
    if (!need_draw_update || ud_u > y  ) ud_u = y;
    if (!need_draw_update || ud_d < y+h) ud_d = y+h;

    need_draw_update = true;
}

static void rb_start_draw(void *handle)
{
    (void) handle;

    /* ... mumble mumble ... not ... reentrant ... mumble mumble ... */

    need_draw_update = false;
    ud_l = 0;
    ud_r = LCD_WIDTH;
    ud_u = 0;
    ud_d = LCD_HEIGHT;
}

static void rb_end_draw(void *handle)
{
    (void) handle;

    LOGF("rb_end_draw");

    if(need_draw_update)
        rb->lcd_update_rect(ud_l, ud_u, ud_r - ud_l, ud_d - ud_u);
}

static char *titlebar = NULL;

static void rb_status_bar(void *handle, char *text)
{
    if(titlebar)
        sfree(titlebar);
    titlebar = dupstr(text);
    LOGF("game title is %s\n", text);
}

static void draw_title(void)
{
    const char *str = NULL;
    if(titlebar)
        str = titlebar;
    else
        str = midend_which_game(me)->name;

    /* quick hack */
    bool orig_clipped = clipped;
    if(orig_clipped)
        rb_unclip(NULL);

    int h;
    rb->lcd_setfont(FONT_UI);
    rb->lcd_getstringsize(str, NULL, &h);

    rb->lcd_set_foreground(BG_COLOR);
    rb->lcd_fillrect(0, LCD_HEIGHT - h, LCD_WIDTH, h);

    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_putsxy(0, LCD_HEIGHT - h, str);
    rb->lcd_update_rect(0, LCD_HEIGHT - h, LCD_WIDTH, h);

    if(orig_clipped)
        rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);
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

void frontend_default_color(frontend *fe, float *out)
{
    *out++ = BG_R;
    *out++ = BG_G;
    *out++ = BG_B;
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

    rb->gui_synclist_init(&list, &config_choices_formatter, (void*)list_str, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, n);
    rb->gui_synclist_limit_scroll(&list, false);

    rb->gui_synclist_select_item(&list, 0);

    rb->gui_synclist_set_title(&list, (char*)title, NOICON);
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

/* return value is only meaningful when type == C_STRING */
static bool do_configure_item(config_item *cfg)
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
            return false;
        }
        sfree(cfg->sval);
        cfg->sval = newstr;
        return true;
    }
    case C_BOOLEAN:
    {
        bool res = cfg->ival != 0;
        rb->set_bool(cfg->name, &res);

        /* seems to reset backdrop */
        rb->lcd_set_backdrop(NULL);

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
    return false;
}

const char *config_formatter(int sel, void *data, char *buf, size_t len)
{
    config_item *cfg = data;
    cfg += sel;
    rb->snprintf(buf, len, "%s", cfg->name);
    return buf;
}

static bool config_menu(void)
{
    char *title;
    config_item *config = midend_get_config(me, CFG_SETTINGS, &title);

    bool success = false;

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
            char *old_str;
            if(old.type == C_STRING)
                old_str = dupstr(old.sval);
            bool freed_str = do_configure_item(config + pos);
            char *err = midend_set_config(me, CFG_SETTINGS, config);
            if(err)
            {
                rb->splash(HZ, err);
                memcpy(config + pos, &old, sizeof(old));
                if(freed_str)
                    config[pos].sval = old_str;
            }
            else if(old.type == C_STRING)
            {
                /* success, and we duplicated the old string, so free it */
                sfree(old_str);
            }
            else
            {
                success = true;
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
    return success;
}

const char *preset_formatter(int sel, void *data, char *buf, size_t len)
{
    char *name;
    game_params *junk;
    midend_fetch_preset(me, sel, &name, &junk);
    rb->strlcpy(buf, name, len);
    return buf;
}

static bool presets_menu(void)
{
    if(!midend_num_presets(me))
    {
        rb->splash(HZ, "No presets!");
        return false;
    }

    /* display a list */
    struct gui_synclist list;

    rb->gui_synclist_init(&list, &preset_formatter, NULL, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, midend_num_presets(me));
    rb->gui_synclist_limit_scroll(&list, false);

    int current = midend_which_preset(me);
    rb->gui_synclist_select_item(&list, current >= 0 ? current : 0);

    rb->gui_synclist_set_title(&list, "Game Type", NOICON);
    while(1)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;
        switch(button)
        {
        case ACTION_STD_OK:
        {
            int sel = rb->gui_synclist_get_sel_pos(&list);
            char *junk;
            game_params *params;
            midend_fetch_preset(me, sel, &junk, &params);
            midend_set_params(me, params);
            return true;
        }
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
            return false;
        default:
            break;
        }
    }
}

static const struct {
    const char *game, *help;
} quick_help_text[] = {
    { "Black Box", "Find the hidden balls in the box by bouncing laser beams off them." },
    { "Bridges", "Connect all the islands with a network of bridges." },
    { "Cube", "Pick up all the blue squares by rolling the cube over them." },
    { "Dominosa", "Tile the rectangle with a full set of dominoes." },
    { "Fifteen", "Slide the tiles around to arrange them into order." },
    { "Filling", "Mark every square with the area of its containing region." },
    { "Flip", "Flip groups of squares to light them all up at once." },
    { "Flood", "Turn the grid the same colour in as few flood fills as possible." },
    { "Galaxies", "Divide the grid into rotationally symmetric regions each centred on a dot." },
    { "Guess", "Guess the hidden combination of colours." },
    { "Inertia", "Collect all the gems without running into any of the mines." },
    { "Keen", "Complete the latin square in accordance with the arithmetic clues." },
    { "Light Up", "Place bulbs to light up all the squares." },
    { "Loopy", "Draw a single closed loop, given clues about number of adjacent edges." },
    { "Magnets", "Place magnets to satisfy the clues and avoid like poles touching." },
    { "Map", "Colour the map so that adjacent regions are never the same colour." },
    { "Mines", "Find all the mines without treading on any of them." },
    { "Net", "Rotate each tile to reassemble the network." },
    { "Netslide", "Slide a row at a time to reassemble the network." },
    { "Palisade", "Divide the grid into equal-sized areas in accordance with the clues." },
    { "Pattern", "Fill in the pattern in the grid, given only the lengths of runs of black squares." },
    { "Pearl", "Draw a single closed loop, given clues about corner and straight squares." },
    { "Pegs", "Jump pegs over each other to remove all but one." },
    { "Range", "Place black squares to limit the visible distance from each numbered cell." },
    { "Rectangles", "Divide the grid into rectangles with areas equal to the numbers." },
    { "Same Game", "Clear the grid by removing touching groups of the same colour squares." },
    { "Signpost", "Connect the squares into a path following the arrows." },
    { "Singles", "Black out the right set of duplicate numbers." },
    { "Sixteen", "Slide a row at a time to arrange the tiles into order." },
    { "Slant", "Draw a maze of slanting lines that matches the clues." },
    { "Solo", "Fill in the grid so that each row, column and square block contains one of every digit." },
    { "Tents", "Place a tent next to each tree." },
    { "Towers", "Complete the latin square of towers in accordance with the clues." },
    { "Tracks", "Fill in the railway track according to the clues." },
    { "Twiddle", "Rotate the tiles around themselves to arrange them into order." },
    { "Undead", "Place ghosts, vampires and zombies so that the right numbers of them can be seen in mirrors." },
    { "Unequal", "Complete the latin square in accordance with the > signs." },
    { "Unruly", "Fill in the black and white grid to avoid runs of three." },
    { "Untangle", "Reposition the points so that the lines do not cross." },
};

static void quick_help(void)
{
#ifdef FOR_REAL
    if(++help_times >= 5)
    {
        rb->splash(HZ, "You are now a developer!");
        debug_mode = true;
    }
#endif

    for(int i = 0; i < ARRAYLEN(quick_help_text); ++i)
    {
        if(!strcmp(midend_which_game(me)->name, quick_help_text[i].game))
        {
            rb->splash(0, quick_help_text[i].help);
            rb->button_get(true);
            return;
        }
    }
}

static void full_help(void)
{
    /* TODO */
}

static void init_default_settings(void)
{
    settings.slowmo_factor = 1;
    settings.bulk = false;
    settings.timerflash = false;
    settings.clipoff = false;
    settings.shortcuts = false;
    settings.no_aa = false;
}

static void bench_aa(void)
{
    rb->sleep(0);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    int next = *rb->current_tick + HZ;
    int i = 0;
    while(*rb->current_tick < next)
    {
        draw_antialiased_line(0, 0, 20, 31);
        ++i;
    }
    rb->splashf(HZ, "%d AA lines/sec", i);
    next = *rb->current_tick + HZ;
    int j = 0;
    while(*rb->current_tick < next)
    {
        rb->lcd_drawline(0, 0, 20, 31);
        ++j;
    }
    rb->splashf(HZ, "%d normal lines/sec", j);
    rb->splashf(HZ, "Efficiency: %d%%", 100 * i / j);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

static void debug_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Debug Menu", NULL,
                        "Slowmo factor",
                        "Randomize colors",
                        "Toggle bulk update",
                        "Toggle flash pixel on timer",
                        "Toggle clip",
                        "Toggle shortcuts",
                        "Toggle antialias",
                        "Benchmark antialias",
                        "Back");
    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            rb->set_int("Slowmo factor", "", UNIT_INT, &settings.slowmo_factor, NULL, 1, 1, 15, NULL);
            break;
        case 1:
        {
            unsigned *ptr = colors;
            for(int i = 0; i < ncolors; ++i)
            {
                /* not seeded, who cares? */
                *ptr++ = LCD_RGBPACK(rb->rand()%255, rb->rand()%255, rb->rand()%255);
            }
            break;
        }
        case 2:
            settings.bulk = !settings.bulk;
            break;
        case 3:
            settings.timerflash = !settings.timerflash;
            break;
        case 4:
            settings.clipoff = !settings.clipoff;
            break;
        case 5:
            settings.shortcuts = !settings.shortcuts;
            break;
        case 6:
            settings.no_aa = !settings.no_aa;
            break;
        case 7:
            bench_aa();
            break;
        case 8:
        default:
            quit = true;
            break;
        }
    }
}

static int pausemenu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = (intptr_t) this_item;
    if(action == ACTION_REQUEST_MENUITEM)
    {
        switch(i)
        {
        case 3:
            if(!midend_can_undo(me))
                return ACTION_EXIT_MENUITEM;
            break;
        case 4:
            if(!midend_can_redo(me))
                return ACTION_EXIT_MENUITEM;
            break;
        case 5:
            if(!midend_which_game(me)->can_solve)
                return ACTION_EXIT_MENUITEM;
            break;
        case 7:
#ifdef FOR_REAL
            return ACTION_EXIT_MENUITEM;
#else
            break;
#endif
        case 8:
#ifdef COMBINED
            /* audio buf is used, so no playback */
            /* TODO: neglects app builds, but not many people will
             * care, I bet */
            return ACTION_EXIT_MENUITEM;
#else
            if(audiobuf_available)
                break;
            else
                return ACTION_EXIT_MENUITEM;
#endif
        case 9:
            if(!midend_num_presets(me))
                return ACTION_EXIT_MENUITEM;
            break;
        case 10:
#ifdef FOR_REAL
            if(debug_mode)
                break;
            return ACTION_EXIT_MENUITEM;
#else
            break;
#endif
        case 11:
            if(!midend_which_game(me)->can_configure)
                return ACTION_EXIT_MENUITEM;
            break;
        default:
            break;
        }
    }
    return action;
}

static void clear_and_draw(void)
{
    rb->lcd_clear_display();
    rb->lcd_update();

    midend_force_redraw(me);
    draw_title();
}

static void reset_drawing(void)
{
    rb->lcd_set_viewport(NULL);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(BG_COLOR);
}

static int pause_menu(void)
{
#define static auto
#define const
    MENUITEM_STRINGLIST(menu, NULL, pausemenu_cb,
                        "Resume Game",
                        "New Game",
                        "Restart Game",
                        "Undo",
                        "Redo",
                        "Solve",
                        "Quick Help",
                        "Extensive Help",
                        "Playback Control",
                        "Game Type",
                        "Debug Menu",
                        "Configure Game",
#ifdef COMBINED
                        "Select Another Game",
#endif
                        "Quit without Saving",
                        "Quit");
#undef static
#undef const
    /* HACK ALERT */
    char title[32] = { 0 };
    rb->snprintf(title, sizeof(title), "%s Menu", midend_which_game(me)->name);
    menu__.desc = title;

#ifdef FOR_REAL
    help_times = 0;
#endif

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
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
            midend_restart_game(me);
            fix_size();
            quit = true;
            break;
        case 3:
            if(!midend_can_undo(me))
                rb->splash(HZ, "Cannot undo.");
            else
                midend_process_key(me, 0, 0, 'u');
            quit = true;
            break;
        case 4:
            if(!midend_can_redo(me))
                rb->splash(HZ, "Cannot redo.");
            else
                midend_process_key(me, 0, 0, 'r');
            quit = true;
            break;
        case 5:
        {
            char *msg = midend_solve(me);
            if(msg)
                rb->splash(HZ, msg);
            quit = true;
            break;
        }
        case 6:
            quick_help();
            break;
        case 7:
            full_help();
            break;
        case 8:
            playback_control(NULL);
            break;
        case 9:
            if(presets_menu())
            {
                midend_new_game(me);
                fix_size();
                reset_drawing();
                clear_and_draw();
                quit = true;
            }
            break;
        case 10:
            debug_menu();
            break;
        case 11:
            if(config_menu())
            {
                midend_new_game(me);
                fix_size();
                reset_drawing();
                clear_and_draw();
                quit = true;
            }
            break;
#ifdef COMBINED
        case 12:
            return -1;
        case 13:
            return -2;
        case 14:
            return -3;
#else
        case 12:
            return -2;
        case 13:
            return -3;
#endif
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
static bool accept_input = true;

/* ignore the excess of LOGFs below... */
#ifdef LOGF_ENABLE
#undef LOGF_ENABLE
#endif
static int process_input(int tmo)
{
    LOGF("process_input start");
    LOGF("------------------");
    int state = 0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false); /* about to block for button input */
#endif

    int button = rb->button_get_w_tmo(tmo);

    /* weird stuff */
    exit_on_usb(button);

    button = rb->button_status();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    if(button == BTN_PAUSE)
    {
        want_redraw = false;
        /* quick hack to preserve the clipping state */
        bool orig_clipped = clipped;
        if(orig_clipped)
            rb_unclip(NULL);

        int rc = pause_menu();

        if(orig_clipped)
            rb_clip(NULL, clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height);

        last_keystate = 0;
        accept_input = true;

        return rc;
    }

    /* special case for inertia: moves occur after RELEASES */
    if(!strcmp("Inertia", midend_which_game(me)->name))
    {
        LOGF("received button 0x%08x", button);

        unsigned released = ~button & last_keystate;
        last_keystate = button;

        if(!button)
        {
            if(!accept_input)
            {
                LOGF("ignoring, all keys released but not accepting input before, can accept input later");
                accept_input = true;
                return 0;
            }
        }

        if(!released || !accept_input)
        {
            LOGF("released keys detected: 0x%08x", released);
            LOGF("ignoring, either no keys released or not accepting input");
            return 0;
        }

        button |= released;
        if(last_keystate)
        {
            LOGF("ignoring input from now until all released");
            accept_input = false;
        }
        LOGF("accepting event 0x%08x", button);
    }
    /* default is to ignore repeats except for untangle */
    else if(strcmp("Untangle", midend_which_game(me)->name))
    {
        /* start accepting input again after a release */
        if(!button)
        {
            accept_input = true;
            return 0;
        }
        /* ignore repeats */
        /* Untangle gets special treatment */
        if(!accept_input)
            return 0;
        accept_input = false;
    }

    switch(button)
    {
    case BTN_UP:
        state = CURSOR_UP;
        break;
    case BTN_DOWN:
        state = CURSOR_DOWN;
        break;
    case BTN_LEFT:
        state = CURSOR_LEFT;
        break;
    case BTN_RIGHT:
        state = CURSOR_RIGHT;
        break;

        /* handle diagonals (mainly for Inertia) */
    case BTN_DOWN | BTN_LEFT:
#ifdef BTN_DOWN_LEFT
    case BTN_DOWN_LEFT:
#endif
        state = '1' | MOD_NUM_KEYPAD;
        break;
    case BTN_DOWN | BTN_RIGHT:
#ifdef BTN_DOWN_RIGHT
    case BTN_DOWN_RIGHT:
#endif
        state = '3' | MOD_NUM_KEYPAD;
        break;
    case BTN_UP | BTN_LEFT:
#ifdef BTN_UP_LEFT
    case BTN_UP_LEFT:
#endif
        state = '7' | MOD_NUM_KEYPAD;
        break;
    case BTN_UP | BTN_RIGHT:
#ifdef BTN_UP_RIGHT
    case BTN_UP_RIGHT:
#endif
        state = '9' | MOD_NUM_KEYPAD;
        break;

    case BTN_FIRE:
        state = CURSOR_SELECT;
        break;

    default:
        break;
    }

    if(settings.shortcuts)
    {
        static bool shortcuts_ok = true;
        switch(button)
        {
        case BTN_LEFT | BTN_FIRE:
            if(shortcuts_ok)
                midend_process_key(me, 0, 0, 'u');
            shortcuts_ok = false;
            break;
        case BTN_RIGHT | BTN_FIRE:
            if(shortcuts_ok)
                midend_process_key(me, 0, 0, 'r');
            shortcuts_ok = false;
            break;
        case 0:
            shortcuts_ok = true;
            break;
        default:
            break;
        }
    }

    LOGF("process_input done");
    LOGF("------------------");
    return state;
}

static long last_tstamp;

static void timer_cb(void)
{
#if LCD_DEPTH != 24
    if(settings.timerflash)
    {
        static bool what = false;
        what = !what;
        if(what)
            rb->lcd_framebuffer[0] = LCD_BLACK;
        else
            rb->lcd_framebuffer[0] = LCD_WHITE;
        rb->lcd_update();
    }
#endif

    LOGF("timer callback");
    midend_timer(me, ((float)(*rb->current_tick - last_tstamp) / (float)HZ) / settings.slowmo_factor);
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

#ifdef COMBINED
/* can't use audio buffer */
char giant_buffer[1024*1024*4];
#else
/* points to pluginbuf */
char *giant_buffer = NULL;
#endif
static size_t giant_buffer_len = 0; /* set on start */

#ifdef COMBINED
const char *formatter(char *buf, size_t n, int i, const char *unit)
{
    rb->snprintf(buf, n, "%s", gamelist[i]->name);
    return buf;
}
#endif

static void fix_size(void)
{
    int w = LCD_WIDTH, h = LCD_HEIGHT, h_x;
    rb->lcd_setfont(FONT_UI);
    rb->lcd_getstringsize("X", NULL, &h_x);
    h -= h_x;
    midend_size(me, &w, &h, TRUE);
}

static void reset_tlsf(void)
{
    /* reset tlsf by nuking the signature */
    /* will make any already-allocated memory point to garbage */
    memset(giant_buffer, 0, 4);
    init_memory_pool(giant_buffer_len, giant_buffer);
}

static int read_wrapper(void *ptr, void *buf, int len)
{
    int fd = (int) ptr;
    return rb->read(fd, buf, len);
}

static void write_wrapper(void *ptr, void *buf, int len)
{
    int fd = (int) ptr;
    rb->write(fd, buf, len);
}

static void init_colors(void)
{
    float *floatcolors = midend_colors(me, &ncolors);

    /* convert them to packed RGB */
    colors = smalloc(ncolors * sizeof(unsigned));
    unsigned *ptr = colors;
    float *floatptr = floatcolors;
    for(int i = 0; i < ncolors; ++i)
    {
        int r = 255 * *(floatptr++);
        int g = 255 * *(floatptr++);
        int b = 255 * *(floatptr++);
        LOGF("color %d is %d %d %d", i, r, g, b);
        *ptr++ = LCD_RGBPACK(r, g, b);
    }
    sfree(floatcolors);
}

static char *init_for_game(const game *gm, int load_fd, bool draw)
{
    /* if we are loading a game tlsf has already been initialized */
    if(load_fd < 0)
        reset_tlsf();

    me = midend_new(NULL, gm, &rb_drawing, NULL);

    if(load_fd < 0)
        midend_new_game(me);
    else
    {
        char *ret = midend_deserialize(me, read_wrapper, (void*) load_fd);
        if(ret)
            return ret;
    }

    fix_size();

    init_colors();

    reset_drawing();

    if(draw)
    {
        clear_and_draw();
    }
    return NULL;
}

static void exit_handler(void)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

/* expects a totally free me* pointer */
static bool load_game(void)
{
    reset_tlsf();

    int fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0)
        return false;

    rb->splash(0, "Loading...");

    LOGF("opening %s", SAVE_FILE);

    char *game;
    char *ret = identify_game(&game, read_wrapper, (void*)fd);

    if(!*game && ret)
    {
        sfree(game);
        rb->splash(HZ, ret);
        rb->close(fd);
        return false;
    }
    else
    {
        /* seek to beginning */
        rb->lseek(fd, 0, SEEK_SET);

#ifdef COMBINED
        /* search for the game and initialize the midend */
        for(int i = 0; i < gamecount; ++i)
        {
            if(!strcmp(game, gamelist[i]->name))
            {
                sfree(ret);
                ret = init_for_game(gamelist[i], fd, true);
                if(ret)
                {
                    rb->splash(HZ, ret);
                    sfree(ret);
                    rb->close(fd);
                    return false;
                }
                rb->close(fd);
                /* success, we delete the save */
                rb->remove(SAVE_FILE);
                return true;
            }
        }
        rb->splashf(HZ, "Incompatible game %s reported as compatible!?!? REEEPORT MEEEE!!!!", game);
        rb->close(fd);
        return false;
#else
        if(!strcmp(game, thegame.name))
        {
            sfree(ret);
            ret = init_for_game(&thegame, fd, false);
            if(ret)
            {
                rb->splash(HZ, ret);
                sfree(ret);
                rb->close(fd);
                return false;
            }
            rb->close(fd);
            /* success, we delete the save */
            rb->remove(SAVE_FILE);
            return true;
        }
        rb->splashf(HZ, "Cannot load save game for %s!", game);
        return false;
#endif
    }
}

static void save_game(void)
{
    rb->splash(0, "Saving...");
    int fd = rb->open(SAVE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    midend_serialize(me, write_wrapper, (void*) fd);
    rb->close(fd);
    rb->lcd_update();
}

static bool load_success;

static int mainmenu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = (intptr_t) this_item;
    if(action == ACTION_REQUEST_MENUITEM)
    {
        switch(i)
        {
        case 0:
        case 7:
            if(!load_success)
                return ACTION_EXIT_MENUITEM;
            break;
        case 3:
#ifdef FOR_REAL
            return ACTION_EXIT_MENUITEM;
#else
            break;
#endif
        case 4:
#ifdef COMBINED
            /* audio buf is used, so no playback */
            /* TODO: neglects app builds, but not many people will
             * care, I bet */
            return ACTION_EXIT_MENUITEM;
#else
            if(audiobuf_available)
                break;
            else
                return ACTION_EXIT_MENUITEM;
#endif
        case 5:
            if(!midend_num_presets(me))
                return ACTION_EXIT_MENUITEM;
            break;
        case 6:
            if(!midend_which_game(me)->can_configure)
                return ACTION_EXIT_MENUITEM;
            break;
        default:
            break;
        }
    }
    return action;
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* boost for init */
    rb->cpu_boost(true);
#endif

#ifdef COMBINED
    giant_buffer_len = sizeof(giant_buffer);
#else
    giant_buffer = rb->plugin_get_buffer(&giant_buffer_len);
#endif

#ifndef COMBINED
    rb->snprintf(save_file_path, sizeof(save_file_path), "%s/sgt-%s.sav", PLUGIN_GAMES_DATA_DIR, thegame.htmlhelp_topic);
#endif

    rb_atexit(exit_handler);

    if(fabs(sqrt(3)/2 - sin(PI/3)) > .01)
        rb->splash(HZ, "WARNING: floating-point functions are being weird... report me!");

    init_default_settings();

    load_success = load_game();

#ifndef COMBINED
    if(!load_success)
    {
        /* our main menu expects a ready-to-use midend */
        init_for_game(&thegame, -1, false);
    }
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* about to go to menu or button block */
    rb->cpu_boost(false);
#endif

#ifdef FOR_REAL
    help_times = 0;
#endif

#ifndef COMBINED
#define static auto
#define const
    MENUITEM_STRINGLIST(menu, NULL, mainmenu_cb,
                        "Resume Game",
                        "New Game",
                        "Quick Help",
                        "Extensive Help",
                        "Playback Control",
                        "Game Type",
                        "Configure Game",
                        "Quit without Saving",
                        "Quit");
#undef static
#undef const

    /* HACK ALERT */
    char title[32] = { 0 };
    rb->snprintf(title, sizeof(title), "%s Menu", midend_which_game(me)->name);
    menu__.desc = title;

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            clear_and_draw();
            goto game_loop;
        case 1:
            if(!load_success)
            {
                clear_and_draw();
                goto game_loop;
            }
            quit = true;
            break;
        case 2:
            quick_help();
            break;
        case 3:
            full_help();
            break;
        case 4:
            playback_control(NULL);
            break;
        case 5:
            if(presets_menu())
            {
                midend_new_game(me);
                fix_size();
                init_colors();
                reset_drawing();
                clear_and_draw();
                goto game_loop;
            }
            break;
        case 6:
            if(config_menu())
            {
                midend_new_game(me);
                fix_size();
                init_colors();
                reset_drawing();
                clear_and_draw();
                goto game_loop;
            }
            break;
        case 8:
            if(load_success)
                save_game();
            /* fall through */
        case 7:
            /* we don't care about freeing anything because tlsf will
             * be wiped out the next time around */
            return PLUGIN_OK;
        default:
            break;
        }
    }
#else
    if(load_success)
        goto game_loop;
#endif

#ifdef COMBINED
    int gm = 0;
#endif
    while(1)
    {
#ifdef COMBINED
        if(rb->set_int("Choose Game", "", UNIT_INT, &gm, NULL, 1, 0, gamecount - 1, formatter))
            return PLUGIN_OK;

        init_for_game(gamelist[gm], -1, true);
#else
        init_for_game(&thegame, -1, true);
#endif

        last_keystate = 0;
        accept_input = true;

    game_loop:
        while(1)
        {
            want_redraw = true;

            draw_title();

            int button = process_input(timer_on ? TIMER_INTERVAL : -1);

            if(button < 0)
            {
                rb_unclip(NULL);
                deactivate_timer(NULL);

                if(titlebar)
                {
                    sfree(titlebar);
                    titlebar = NULL;
                }

                if(button == -1)
                {
                    /* new game */
                    midend_free(me);
                    break;
                }
                else if(button == -2)
                {
                    /* quit without saving */
                    midend_free(me);
                    sfree(colors);
                    exit(PLUGIN_OK);
                }
                else if(button == -3)
                {
                    /* save and quit */
                    save_game();
                    midend_free(me);
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
