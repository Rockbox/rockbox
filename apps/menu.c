/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd.h"
#include "menu.h"

#include "tree.h"
#include "credits.h"

#ifdef HAVE_LCD_BITMAP

#include "screensaver.h"
extern void tetris(void);

#define MENU_ITEM_FONT    0
#define MENU_ITEM_Y_LOC   6
#define MENU_LINE_HEIGHT  8

enum Main_Menu_Ids {
    Tetris, Screen_Saver, Browse, Splash, Credits, Last_Id
};

struct main_menu_items items[] = {
    { Tetris,       "Tetris",       tetris      },
    { Screen_Saver, "Screen Saver", screensaver },
    { Browse,       "Browse",       browse_root },
	{ Splash,       "Splash",       show_splash },
	{ Credits,      "Credits",      show_credits },
};

/* Global values for menuing */
int menu_top;
int menu_bottom;
int menu_line_height;
int cursor;

int get_line_height(void)
{
    return menu_line_height;
}

int is_cursor_menu_top(void)
{
    return ((cursor == menu_top) ? 1 : 0);
}

int is_cursor_menu_bottom(void)
{
    return ((cursor == menu_bottom) ? 1 : 0);
}

void put_cursor_menu_top(void)
{
    put_cursor(menu_top);
}

void put_cursor_menu_bottom(void)
{
    put_cursor(menu_bottom);
}

void move_cursor_up(void)
{
    put_cursor(cursor-1);
}

void move_cursor_down(void)
{
    put_cursor(cursor+1);
}

void redraw_cursor(void)
{
    lcd_putsxy(0, cursor*menu_line_height, "-", 0);
}

/* 
 * Move the cursor to a particular id, 
 *   current: where it is now 
 *   target: where you want it to be 
 */
void put_cursor(int target)
{
    lcd_putsxy(0, cursor*menu_line_height, " ",0);
    cursor = target;
    lcd_putsxy(0, cursor*menu_line_height, "-",0);
}

/* We call the function pointer related to the current cursor position */
void execute_menu_item(void)
{
    /* call the proper function for this line */
    items[cursor].function();
}

void add_menu_item(int location, char *string)
{
    lcd_putsxy(MENU_ITEM_Y_LOC, MENU_LINE_HEIGHT*location, string,
               MENU_ITEM_FONT);
    if (location < menu_top)
        menu_top = location;
    if (location > menu_bottom)
        menu_bottom = location;
}

int show_logo(void)
{
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

    for(i=0, eline=0; i< height; i+=8, eline++) {
      int x,y;
      
      /* the bitmap function doesn't work with full-height bitmaps
         so we "stripe" the logo output */

      lcd_bitmap(&buffer[eline*width], 0, 10+i, width,
                 (height-i)>8?8:height-i, false);
      
#if 0
      /* for screen output debugging */
      for(y=0; y<8 && (i+y < height); y++) {
        for(x=0; x < width; x++) {

          if(buffer[eline*width + x] & (1<<y)) {
            printf("*");
          }
          else
            printf(" ");
        }
        printf("\n");
      }
#endif
    }
  }

  return 0;
}

void menu_init(void)
{
    menu_top = Tetris;
    menu_bottom = Last_Id-1;
    menu_line_height = MENU_LINE_HEIGHT;
    cursor = menu_top;
	lcd_clear_display();
	lcd_update();
}

void menu_draw(void)
{
    int i = 0;

    for (i = i; i < Last_Id; i++)
        add_menu_item(items[i].menu_id, (char *) items[i].menu_desc);

    redraw_cursor();
	lcd_update();
}

#endif /* end HAVE_LCD_BITMAP */



void show_splash(void)
{
#ifdef HAVE_LCD_BITMAP
	lcd_clear_display();

	if (show_logo() == 0) {
		lcd_update();
		busy_wait();
	}
#else
	char *rockbox = "ROCKbox!";
	lcd_puts(0, 0, rockbox);
	busy_wait();
#endif
}


