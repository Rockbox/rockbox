/* rockbox frontend */

#include "plugin.h"

#include "puzzles.h"

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

static void rb_draw_text(void *handle, int x, int y, int fonttype,
                         int fontsize, int align, int colour, char *text)
{
    LOGF("rb_draw_text(%d %d %s)", x, y, text);
    offset_coords(&x, &y);
    /* we just ignore the parameters and draw */
    rb->lcd_putsxy(x, y, text);
}

static void rb_color(int n)
{
    rb->lcd_set_foreground(colors[n]);
}

static void rb_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
    LOGF("rb_draw_rect(%d, %d, %d, %d, %d)", x, y, w, h, colour);
    rb_color(colour);
    offset_coords(&x, &y);
    rb->lcd_drawrect(x, y, w, h);
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
    /* TODO */
}

static void rb_draw_circle(void *handle, int cx, int cy, int radius,
                           int fillcolour, int outlinecolour)
{
    LOGF("rb_draw_circle(%d, %d, %d)", cx, cy, radius);
    /* TODO */
}

static blitter *rb_blitter_new(void *handle, int w, int h)
{
    LOGF("rb_blitter_new");
    return NULL;
}

static void rb_blitter_free(void *handle, blitter *bl)
{
    LOGF("rb_blitter_free");
    return NULL;
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
#if 0
    x0 = (x0 < 0 ? 0 : x0 > canvas_w ? canvas_w : x0);
    x1 = (x1 < 0 ? 0 : x1 > canvas_w ? canvas_w : x1);
    y0 = (y0 < 0 ? 0 : y0 > canvas_h ? canvas_h : y0);
    y1 = (y1 < 0 ? 0 : y1 > canvas_h ? canvas_h : y1);
#endif

    /* Transform back into x,y,w,h to return */
    *x = x0;
    *y = y0;
    *w = x1 - x0;
    *h = y1 - y0;
}

static void rb_blitter_save(void *handle, blitter *bl, int x, int y)
{
    LOGF("rb_blitter_save");
    /* TODO */
}

static void rb_blitter_load(void *handle, blitter *bl, int x, int y)
{
    LOGF("rb_blitter_load");
    /* TODO */
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

static long last_tstamp;

static void timer_cb(void)
{
    midend_timer(me, (float)(*rb->current_tick - last_tstamp) / HZ);
    last_tstamp = *rb->current_tick;
    rb->lcd_update();
}

void activate_timer(frontend *fe)
{
    last_tstamp = *rb->current_tick;
    LOGF("activate_timer called");
    rb->timer_register(1, NULL, TIMER_FREQ / 50, timer_cb IF_COP(, CPU));
}

void deactivate_timer(frontend *fe)
{
    rb->timer_unregister();
}

/* can't use audio buffer */
char giant_buffer[1024*1024*1];

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    init_memory_pool(sizeof(giant_buffer), giant_buffer);

    me = midend_new(NULL, gamelist[1], &rb_drawing, NULL);
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

    rb->lcd_clear_display();
    midend_redraw(me);

    while(1)
    {
        rb->yield();
    }
}
