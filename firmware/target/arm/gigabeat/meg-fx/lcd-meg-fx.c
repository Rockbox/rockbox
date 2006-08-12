#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"

void lcd_init_device(void);
void lcd_update_rec(int, int, int, int);
void lcd_update(void);

/* LCD init */
void lcd_init_device(void)
{
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    memcpy(FRAME, &lcd_framebuffer, sizeof(lcd_framebuffer));
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
