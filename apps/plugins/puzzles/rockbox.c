/* rockbox frontend */

#include "plugin.h"

#include "puzzles.h"

#include "lib/pluginlib_actions.h"
#include "lib/xlcd.h"

static midend *me = NULL;
static unsigned *colors = NULL;
static int ncolors = 0;

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

    rb_color(outlinecolour);
    assert(outlinecolour >= 0);
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

    rb->lcd_drawline(x1, y1,
                     x2, y2);
}

static void rb_draw_circle(void *handle, int cx, int cy, int radius,
                           int fillcolour, int outlinecolour)
{
    LOGF("rb_draw_circle(%d, %d, %d)", cx, cy, radius);
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
    bl->bmp.data = NULL;
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

static void rb_blitter_save(void *handle, blitter *bl, int x, int y)
{
    LOGF("rb_blitter_save");
    /* no viewport offset */
#if defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT == VERTICAL_STRIDE)
#error no vertical stride
#else
    if(bl->bmp.data)
    {
        int w = bl->bmp.width, h = bl->bmp.height;
        trim_rect(&x, &y, &w, &h);
        for(int i = 0; i < h; ++i)
        {
            /* copy line-by-line */
            rb->memcpy(bl->bmp.data + sizeof(fb_data) * (y + i) * w,
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
    rb->lcd_bitmap(bl->bmp.data, x, y, bl->bmp.width, bl->bmp.height);
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
    *out++ = 1.0;
    *out++ = 1.0;
    *out++ = 1.0;
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

static int process_input(int tmo)
{
    int state = 0;
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int button = pluginlib_getaction(tmo, plugin_contexts, ARRAYLEN(plugin_contexts));
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
        return -1;
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
    midend_timer(me, (float)(*rb->current_tick - last_tstamp) / HZ);
    last_tstamp = *rb->current_tick;
}

static volatile bool timer_on = false;

void activate_timer(frontend *fe)
{
    timer_on = true;
}

void deactivate_timer(frontend *fe)
{
    timer_on = false;
}

/* can't use audio buffer */
char giant_buffer[1024*1024*1];

const char *formatter(char *buf, size_t n, int i, const char *unit)
{
    rb->snprintf(buf, n, "%d: %s", i, gamelist[i]->name);
    return buf;
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    init_memory_pool(sizeof(giant_buffer), giant_buffer);

    int gm = 0;
    while(1)
    {
        if(rb->set_int("Choose Game", "", UNIT_INT, &gm, NULL, 1, 0, gamecount - 1, formatter))
            return PLUGIN_OK;

        me = midend_new(NULL, gamelist[gm], &rb_drawing, NULL);
        midend_new_game(me);
        int w = LCD_WIDTH, h = LCD_HEIGHT;
        midend_size(me, &w, &h, TRUE);
        static float *floatcolors = NULL;
        floatcolors = midend_colours(me, &ncolors);

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

        rb->lcd_set_viewport(NULL);
        rb->lcd_set_backdrop(NULL);
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_set_background(LCD_WHITE);
        rb->lcd_clear_display();
        rb->lcd_update();

        midend_redraw(me);

        while(1)
        {
            int button = process_input(timer_on ? HZ / 20 : -1);
            if(button < 0)
            {
                rb_unclip(NULL);
                deactivate_timer(NULL);
                midend_free(me);
                break;
            }
            midend_process_key(me, 0, 0, button);
            midend_redraw(me);

            if(timer_on)
                timer_cb();

            rb->yield();
        }
        sfree(colors);
    }
}
