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
#include "sprintf.h"
#include <string.h>

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#include "icons.h"
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

    int i;
    int eline;

    failure = read_bmp_file("/rockbox112.bmp", &width, &height, buffer);

    debugf("read_bmp_file() returned %d, width %d height %d\n",
           failure, width, height);

    lcd_clear_display();

    if (failure) {
        unsigned char *ptr=rockbox112x37;
        for(i=0; i < 37; i+=8) {
            /* the bitmap function doesn't work with full-height bitmaps
               so we "stripe" the logo output */
            lcd_bitmap(ptr, 0, 10+i, 112, (37-i)>8?8:37-i, false);
            ptr += 112;
        }
        height = 37;

    }
    else {

        for(i=0, eline=0; i < height; i+=8, eline++) {
            /* the bitmap function doesn't work with full-height bitmaps
               so we "stripe" the logo output */
            lcd_bitmap(&buffer[eline*width], 0, 10+i, width,
                       (height-i)>8?8:height-i, false);
        }
    }

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
    enum {
        Tetris, Screen_Saver, Version, Sound
    };

    /* main menu */
    struct menu_items items[] = {
        { Sound,        "Sound",        sound_menu  },
#ifdef HAVE_LCD_BITMAP
        { Tetris,       "Tetris",       tetris      },
        { Screen_Saver, "Screen Saver", screensaver },
#endif
        { Version,      "Version",      show_credits },
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}
