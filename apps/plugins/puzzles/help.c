#include "help.h"
#include "lib/simple_viewer.h"

void full_help(const char *name)
{
    unsigned old_bg = rb->lcd_get_background();

    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);

    view_text(name, help_text);

    rb->lcd_set_background(old_bg);
}
