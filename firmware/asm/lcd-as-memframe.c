
#include <string.h>
#include "lcd.h"
void lcd_copy_buffer_rect(fb_data *dst, fb_data *src, int width, int height)
{
    do {
        memcpy(dst, src, width * sizeof(fb_data));
        src += LCD_WIDTH;
        dst += LCD_WIDTH;
    } while (--height);
}
