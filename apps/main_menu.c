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
#include "sound_menu.h"
#include "version.h"
#include "debug.h"

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#include "screensaver.h"
extern void tetris(void);
#endif

int show_logo( void )
{
#ifdef HAVE_LCD_BITMAP
    unsigned char buffer[112 * 8];
    char version[32];

    int failure;
    int height, width, font_h, font_w;

    failure = read_bmp_file("/rockbox112.bmp", &width, &height, buffer);

    debugf("read_bmp_file() returned %d, width %d height %d\n",
           failure, width, height);

    if (failure) {
        debugf("Unable to find \"/rockbox112.bmp\"\n");
        return -1;
    } else {

        int i;
        int eline;

        lcd_clear_display();

        for(i=0, eline=0; i < height; i+=8, eline++) {
            /* the bitmap function doesn't work with full-height bitmaps
               so we "stripe" the logo output */
            lcd_bitmap(&buffer[eline*width], 0, 10+i, width,
                       (height-i)>8?8:height-i, false);
        }
    }

    snprintf(version, sizeof(version), "Ver. %s", appsversion);
    lcd_getfontsize(0, &font_w, &font_h);

    /* lcd_puts needs line height in Chars on screen not pixels */
    width = ((LCD_WIDTH/font_w) - strlen(version)) / 2;
    height = ((height+10)/font_h)+1;

    lcd_puts(width, height, version);
#else
    char *rockbox = "ROCKbox!";
    lcd_puts(0, 0, rockbox);
    lcd_puts(0, 1, appsversion);
#endif

    lcd_update();


    return 0;
}

void show_splash(void)
{
    if (show_logo() != 0) 
        return;

    button_get(true);
}

void main_menu(void)
{
    int m;
    enum {
        Tetris, Screen_Saver, Splash, Credits, Sound
    };

    /* main menu */
    struct menu_items items[] = {
        { Sound,        "Sound",        sound_menu  },
#ifdef HAVE_LCD_BITMAP
        { Tetris,       "Tetris",       tetris      },
        { Screen_Saver, "Screen Saver", screensaver },
#endif
        { Splash,       "Splash",       show_splash },
        { Credits,      "Credits",      show_credits },
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}



