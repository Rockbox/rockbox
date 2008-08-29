#include "font.h"
#include "screen_access.h"
//#include <windef.h>
#include "api.h"
#include "defs.h"
#include "proxy.h"
#include "dummies.h"

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCD_WIDTH,
    .height   = LCD_HEIGHT,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = LCD_DEFAULT_BG,
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
};

struct viewport* current_vp = &default_vp;

void   get_current_vp(struct viewport_api *avp){
    avp->x = current_vp->x;
    avp->y = current_vp->y;
    avp->width = current_vp->width;
    avp->height = current_vp->height;

    //TODO: font_get(current_vp->font)->height;
    avp->fontheight = sysfont.height;
}

void lcd_set_viewport(struct viewport* vp)
{
    if (vp == NULL){
        current_vp = &default_vp;
        DEBUGF3("lcd_set_viewport(struct viewport* vp= DEFAULT)\n");
    }
    else{
        current_vp = vp;
        DEBUGF3("lcd_set_viewport(struct viewport* vp=%x,vpx=%d,vpy=%d,vpw=%d,vph=%d)\n",vp,vp->x,vp->y,vp->width,vp->height);
    }
}

void lcd_update_viewport(void)
{
    //lcd_update_rect(current_vp->x, current_vp->y,current_vp->width, current_vp->height);
}

void lcd_update_viewport_rect(int x, int y, int width, int height)
{
    //lcd_update_rect(current_vp->x + x, current_vp->y + y, width, height);
}
/*** parameter handling ***/

void lcd_set_drawmode(int mode)
{
    current_vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_get_drawmode(void)
{
    return current_vp->drawmode;
}

void lcd_set_foreground(unsigned color)
{
    current_vp->fg_pattern = color;
}

unsigned lcd_get_foreground(void)
{
    return current_vp->fg_pattern;
}

void lcd_set_background(unsigned color)
{
    current_vp->bg_pattern = color;
}

unsigned lcd_get_background(void)
{
    return current_vp->bg_pattern;
}

void lcd_set_selector_start(unsigned color)
{
    current_vp->lss_pattern = color;
}

void lcd_set_selector_end(unsigned color)
{
    current_vp->lse_pattern = color;
}

void lcd_set_selector_text(unsigned color)
{
    current_vp->lst_pattern = color;
}

void lcd_set_drawinfo(int mode, unsigned fg_color, unsigned bg_color)
{
    //lcd_set_drawmode(mode);
    current_vp->fg_pattern = fg_color;
    current_vp->bg_pattern = bg_color;
}

int lcd_getwidth(void)
{
    return current_vp->width;
}

int lcd_getheight(void)
{
    return current_vp->height;
}

void lcd_setfont(int newfont)
{
    current_vp->font = newfont;
}

int lcd_getfont(void)
{
    return current_vp->font;
}

/* Clear the whole display */
void lcd_clear_display(void)
{
    struct viewport* old_vp = current_vp;

    current_vp = &default_vp;

    lcd_clear_viewport();

    current_vp = old_vp;
}

void lcd_clear_viewport(){
    DEBUGF2("lcd_clear_viewport()\n");
    xapi->clear_viewport(current_vp->x,current_vp->y,current_vp->width,current_vp->height,current_vp->bg_pattern);

}
void lcd_scroll_stop(struct viewport* vp){
    DEBUGF3("lcd_scroll_stop(struct viewport* vp=%x)\n",vp);
}
