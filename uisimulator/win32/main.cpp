#include "uisw32.h"
#include "lcd-win32.h"

#define FS 6

int main (void)
{
    int x;
    lcd_init ();

    while (1)
    {
        for (x = 0; x < 10; x++)
        {
            lcd_clear_display ();
	        lcd_position (x, 12, FS);
	        lcd_string ("Hello World!");
	        lcd_position (x, 32, FS);
	        lcd_string ("From the");
	        lcd_position (x, 40, FS);
	        lcd_string ("  Open Source  ");
	        lcd_position (x, 48, FS);
	        lcd_string ("Jukebox Project");
            lcd_update ();
        }
    }

    return 0;
}