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

int show_logo(void)
{
#ifdef HAVE_LCD_BITMAP
    unsigned char buffer[112 * 8];

    int failure;
    int width=0;
    int height=0;

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

        for(i=0, eline=0; i< height; i+=8, eline++) {
            /* the bitmap function doesn't work with full-height bitmaps
               so we "stripe" the logo output */
            lcd_bitmap(&buffer[eline*width], 0, 10+i, width,
                       (height-i)>8?8:height-i, false);
        }
    }
    lcd_update();

#else
    char *rockbox = "ROCKbox!";
    lcd_puts(0, 0, rockbox);
#endif

    return 0;
}

void show_splash(void)
{
    if (show_logo() != 0) 
        return;

    sleep(200);
}

void version(void)
{
    lcd_clear_display();
    lcd_puts(0,0,appsversion);
    lcd_update();
    button_get(true);
}

void main_menu(void)
{
    int m;
    enum {
        Tetris, Screen_Saver, Splash, Credits, Sound, Version
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
        { Version,      "Version",      version }
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}
