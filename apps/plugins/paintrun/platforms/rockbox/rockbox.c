#include "plugin.h"

#include "fixedpoint.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/xlcd.h"

#include "../../src/compat.h"
#include "../../src/dash.h"
#include "rockbox.h"

void plat_clear(void)
{
    rb->lcd_clear_display();
}

void plat_set_background(unsigned col)
{
    rb->lcd_set_background(col);
}

void plat_set_foreground(unsigned col)
{
    rb->lcd_set_foreground(col);
}

void plat_hline(int x1, int x2, int y)
{
    rb->lcd_hline(x1, x2, y);
}

void plat_vline(int y1, int y2, int x)
{
    rb->lcd_vline(y1, y2, x);
}

void plat_drawpixel(int x, int y)
{
    rb->lcd_drawpixel(x, y);
}

void plat_drawrect(int x, int y, int w, int h)
{
    rb->lcd_drawrect(x, y, w, h);
}

void plat_fillrect(int x, int y, int w, int h)
{
    rb->lcd_fillrect(x, y, w, h);
}

void plat_filltri(int x1, int y1, int x2, int y2, int x3, int y3)
{
    xlcd_filltriangle(x1, y1,
                      x2, y2,
                      x3, y3);
}

void plat_logf(const char *fmt, ...)
{
#ifdef ROCKBOX_HAS_LOGF
    va_list ap;
    va_start(ap, fmt);
    char buf[128];
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    LOGF("%s", buf);
    va_end(ap);
#endif
}

void plat_update(void)
{
    rb->lcd_update();
}

unsigned int plat_rand(void)
{
    return rb->rand();
}

long plat_time(void)
{
    return *rb->current_tick * (1000/HZ);
}

void plat_sleep(long ms)
{
    rb->sleep(ms / (1000/HZ));
}

void plat_fillcircle(int cx, int cy, int r)
{
    int d = 3 - (r * 2);
    int x = 0;
    int y = r;
    while(x <= y)
    {
        rb->lcd_hline(cx - x, cx + x, cy + y);
        rb->lcd_hline(cx - x, cx + x, cy - y);
        rb->lcd_hline(cx - y, cx + y, cy + x);
        rb->lcd_hline(cx - y, cx + y, cy - x);
        if(d < 0)
        {
            d += (x * 4) + 6;
        }
        else
        {
            d += ((x - y) * 4) + 10;
            --y;
        }
        ++x;
    }
}

enum keyaction_t action;

void plat_yield(void)
{
    rb->yield();
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int button;
    switch(button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts)))
    {
    case PLA_SELECT:
    case PLA_SELECT_REPEAT:
        action = ACTION_JUMP;
        break;
    case PLA_EXIT:
        exit(PLUGIN_OK);
        break;
    default:
        exit_on_usb(button);
    }
}

void plat_gameover(struct game_ctx_t *ctx)
{
    return;
}

void plat_drawscore(int score)
{
    rb->lcd_putsf(0, 0, "Score: %dm", score);
}

void plat_paused(struct game_ctx_t *ctx)
{
    return;
}

enum keyaction_t plat_pollaction(void)
{
    enum keyaction_t ret = action;
    action = NONE;
    return ret;
}

enum menuaction_t plat_domenu(void)
{
    return MENU_DOGAME;
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    action = NONE;
    dash_main();
    return PLUGIN_OK;
}
