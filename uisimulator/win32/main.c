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
	        lcd_puts (x, 12, "Hello World!", FS);
	        lcd_puts (x, 32, "From the", FS);
	        lcd_puts (x, 40, "  Open Source  ", FS);
	        lcd_puts (x, 48, "Jukebox Project", FS);
            lcd_update ();
        }
    }

    return 0;
}