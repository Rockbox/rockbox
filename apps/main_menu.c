/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "menu.h"
#include "tree.h"
#include "credits.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "main_menu.h"
#include "version.h"
#include "debug.h"
#include "sprintf.h"
#include <string.h>
#include "playlist.h"
#include "settings.h"
#include "settings_menu.h"
#include "sound_menu.h"

#ifdef HAVE_LCD_BITMAP
#include "games_menu.h"
#include "screensavers_menu.h"
#include "bmp.h"
#include "icons.h"

#ifndef SIMULATOR
#ifdef ARCHOS_RECORDER
#include "adc.h"
#endif
#endif
#endif

void dbg_ports(void);

int show_logo( void )
{
#ifdef HAVE_LCD_BITMAP
    char version[32];
    unsigned char *ptr=rockbox112x37;
    int height, i, font_h, font_w;

    lcd_clear_display();

    for(i=0; i < 37; i+=8) {
        /* the bitmap function doesn't work with full-height bitmaps
           so we "stripe" the logo output */
        lcd_bitmap(ptr, 0, 10+i, 112, (37-i)>8?8:37-i, false);
        ptr += 112;
    }
    height = 37;

#if 0
    /*
     * This code is not used anymore, but I kept it here since it shows
     * one way of using the BMP reader function to display an externally
     * providing logo.
     */
    unsigned char buffer[112 * 8];
    int width;

    int i;
    int eline;

    int failure;
    failure = read_bmp_file("/rockbox112.bmp", &width, &height, buffer);

    debugf("read_bmp_file() returned %d, width %d height %d\n",
           failure, width, height);

        for(i=0, eline=0; i < height; i+=8, eline++) {
            /* the bitmap function doesn't work with full-height bitmaps
               so we "stripe" the logo output */
            lcd_bitmap(&buffer[eline*width], 0, 10+i, width,
                       (height-i)>8?8:height-i, false);
        }
    }
#endif

    snprintf(version, sizeof(version), "Ver. %s", appsversion);
    lcd_getfontsize(0, &font_w, &font_h);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(version)*font_w)/2),
               height+10+font_h, version, 0);
    lcd_update();

#else
    char *rockbox = "ROCKbox!";
    lcd_clear_display();
#ifdef HAVE_NEW_CHARCELL_LCD
    lcd_double_height(true);
#endif
    lcd_puts(0, 0, rockbox);
    lcd_puts(0, 1, appsversion);
#endif

    return 0;
}

void show_credits(void)
{
    int j = 0;

    show_logo();
#ifdef HAVE_NEW_CHARCELL_LCD
    lcd_double_height(false);
#endif
    
    for (j = 0; j < 10; j++) {
        sleep((HZ*2)/10);

        if (button_get(false))
            return;	
    }
    roll_credits();
}

void main_menu(void)
{
    int m;

    /* main menu */
    struct menu_items items[] = {
        { "Sound Settings",     sound_menu        },
        { "General Settings",   settings_menu     },
#ifdef HAVE_LCD_BITMAP
        { "Games",              games_menu        },
        { "Screensavers",       screensavers_menu },
#endif
        { "Version",            show_credits      },
#ifndef SIMULATOR
#ifdef ARCHOS_RECORDER
        { "Debug (keep out!)",  dbg_ports         },
#endif
#endif
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
#ifndef SIMULATOR
#ifdef ARCHOS_RECORDER
/* Test code!!! */
void dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;

    lcd_clear_display();

    while(1)
    {
	porta = PADR;
	portb = PBDR;
	portc = PCDR;

	snprintf(buf, 32, "PADR: %04x", porta);
	lcd_puts(0, 0, buf);
	snprintf(buf, 32, "PBDR: %04x", portb);
	lcd_puts(0, 1, buf);
	snprintf(buf, 32, "PCDR: %02x", portc);
	lcd_puts(0, 2, buf);

	snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
	lcd_puts(0, 3, buf);
	snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
	lcd_puts(0, 4, buf);
	snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
	lcd_puts(0, 5, buf);
	snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
	lcd_puts(0, 6, buf);
	
	lcd_update();
	sleep(HZ/10);

	button = button_get(false);

	switch(button)
	{
	case BUTTON_ON:
	    /* Toggle the charger */
	    PBDR ^= 0x20;
	    break;

#ifdef HAVE_RECORDER_KEYPAD	    
	case BUTTON_OFF:
#else
        case BUTTON_STOP:
#endif	    /* Disable the charger */
	    PBDR |= 0x20;
	    return;
	}
    }
}
#endif
#endif
