/*********************************************************************/
/* menu.c - user menu for rockboy                                    */
/*                                                                   */
/* Note: this file only exposes one function: do_user_menu().        */
/*********************************************************************/

#include "stdlib.h"
#include "string.h"
#include "button.h"
#include "rockmacros.h"

/* load/save state function declarations */
static void do_slot_menu(bool is_load);

#define MENU_CANCEL (-1)
static int do_menu(char *title, char **items, size_t num_items, int sel_item);

/* main menu items */
#define MAIN_MENU_TITLE "RockBoy Menu"
typedef enum {
  ITEM_BACK,
  ITEM_LOAD,
  ITEM_SAVE,
  ITEM_OPT,
  ITEM_QUIT,
  ITEM_LAST
} MainMenuItem;

/* strings for the main menu */
static const char *main_menu[] = {
  "Back to Game",
  "Load State...",
  "Save State...",
  "Options...",
  "Quit RockBoy"
};

typedef enum {
  ITEM_SLOT1,
  ITEM_SLOT2,
  ITEM_SLOT3,
  ITEM_SLOT4,
  ITEM_SLOT5
} SlotMenuItem;

/* this is evil, but we snprintf() into it later :( */
static const char *slot_menu[] = {
  "1.                  ",
  "2.                  ",
  "3.                  ",
  "4.                  ",
  "5.                  "
};

/*
 * do_user_menu - create the user menu on the screen.
 *
 * Returns USER_MENU_QUIT if the user selected "quit", otherwise 
 * returns zero.
 *
 * Note: this is the only non-static function in this file at the
 * moment.  In the future I may turn do_menu/etc into a proper API, in
 * which case they'll be exposed as well.
 *
 */
int do_user_menu(void) {
  int mi, ret, num_items;
  bool done = false;

  /* set defaults */
  ret = 0; /* return value */
  mi = 0; /* initial menu selection */
  num_items = sizeof(main_menu) / sizeof(char*);
  
  /* loop until we should exit menu */
  while (!done) {
    /* get item selection */
    mi = do_menu(MAIN_MENU_TITLE, (char**) main_menu, num_items, mi);
    
    /* handle selected menu item */
    switch (mi) {
      case ITEM_QUIT:
        ret = USER_MENU_QUIT;
      case MENU_CANCEL:
      case ITEM_BACK:
        done = true;
        break;
      case ITEM_LOAD:
        do_slot_menu(1);
        break;
      case ITEM_SAVE:
        do_slot_menu(0);
        break;
    }
  }

  /* return somethin' */
  return ret;
}

/*
 * do_load_menu - prompt the user for a memory slot
 *
 * TOOD
 *
 */
static void do_slot_menu(bool is_load) {
  int i, mi, ret, num_items;
  bool done = false;
  char *title;

  /* set defaults */
  ret = 0; /* return value */
  mi = 0; /* initial menu selection */
  num_items = sizeof(slot_menu) / sizeof(char*);
  
  /* create menu items */
  for (i = 0; i < num_items; i++) {
    /* TODO: get slot info here */
    snprintf((char*) slot_menu[i], sizeof(slot_menu[i]), "%2d. %s", i + 1, "<empty>");
  }
  
  /* set menu title */
  title = is_load ? "Load State" : "Save State";

  /* loop until we should exit menu */
  while (!done) {
    /* get item selection */
    mi = do_menu(title, (char**) slot_menu, num_items, mi);
    
    /* handle selected menu item */
    if (mi == MENU_CANCEL) {
      done = true;
      break;
    } else {
      if (is_load) {
        /* TODO: load slot here */
      } else {
        /* TODO: save slot here */
      }
    }
  }
}

/*********************************************************************/
/*  MENU FUNCTIONS                                                   */
/*********************************************************************/
/* at some point i'll make this a generic menu interface, but for now,
 * these defines will suffice */
#define MENU_X 10
#define MENU_Y 10
#define MENU_WIDTH (LCD_WIDTH - 2 * MENU_X)
#define MENU_HEIGHT (LCD_HEIGHT - 2 * MENU_Y)
#define MENU_RECT MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT
#define SHADOW_RECT MENU_X + 1, MENU_Y + 1, MENU_WIDTH, MENU_HEIGHT
#define MENU_ITEM_PAD 2

/*
 * select_item - select menu item (after deselecting current item)
 */
static void select_item(char *title, int curr_item, size_t item_i) {
  int x, y, w, h;

  /* get size of title, use that as height ofr all lines */
  rb->lcd_getstringsize(title, &w, &h);
  h += MENU_ITEM_PAD * 2;

  /* calc x and width */
  x = MENU_X + MENU_ITEM_PAD;
  w = MENU_WIDTH - 2 * MENU_ITEM_PAD;

  /* if there is a current item, then deselect it */
  if (curr_item >= 0) {
    /* deselect old item */
    y = MENU_Y + h + MENU_ITEM_PAD * 2; /* account for title */
    y += h * curr_item;
    rb->lcd_invertrect(x, y, w, h);
  }

  /* select new item */
  curr_item = item_i;

  /* select new item */
  y = MENU_Y + h + MENU_ITEM_PAD * 2; /* account for title */
  y += h * curr_item;
  rb->lcd_invertrect(x, y, w, h);

  /* update the menu window */
  rb->lcd_update_rect(MENU_RECT);
}

/*
 * draw_menu - draw menu on screen
 *
 * Returns MENU_CANCEL if the user cancelled, or the item number of the
 * selected item.
 *
 */
static void draw_menu(char *title, char **items, size_t num_items)  {
  size_t i;
  int x, y, w, h, by;

  /* set to default? font */
  /* rb->lcd_setfont(0); */
  
  /* draw the outline */
  rb->lcd_fillrect(SHADOW_RECT);
  rb->lcd_clearrect(MENU_RECT);
  rb->lcd_drawrect(MENU_RECT);

  /* calculate x/y */
  x = MENU_X + MENU_ITEM_PAD;
  y = MENU_Y + MENU_ITEM_PAD * 2;
  rb->lcd_getstringsize(title, &w, &h);
  h += MENU_ITEM_PAD * 2;

  /* draw menu stipple */
  for (i = MENU_Y; i < (size_t) y + h; i += 2)
    rb->lcd_drawline(MENU_X, i, MENU_X + MENU_WIDTH, i);

  /* clear title rect */
  rb->lcd_clearrect((LCD_WIDTH - w) / 2 - 2, y - 2, w + 4, h);
  
  /* draw centered title on screen */
  rb->lcd_putsxy((LCD_WIDTH - w)/2, y, title);
  
  /* calculate base Y for items */
  by = y + h + MENU_ITEM_PAD;
  
  /* iterate over each item and draw it on the screen */
  for (i = 0; i < num_items; i++)
    rb->lcd_putsxy(x, by + h * i, items[i]);

  /* update the screen */
  rb->lcd_update();
}

/*
 * do_menu - draw menu on screen.
 *
 * Draw a menu titled @title on the screen, with @num_items elements
 * from @items, and select the @sel element.  If in doubt, set @sel to
 * -1 :).
 *
 */
static int do_menu(char *title, char **items, size_t num_items, int sel) {
  int btn, sel_item, ret, curr_item;
  bool done = false;
  ret = MENU_CANCEL;

  /* draw menu on screen and select the first item */
  draw_menu(title, items, num_items);
  curr_item = -1;
  select_item(title, curr_item, sel);
  curr_item = sel;

  /* make sure button state is empty */
  while (rb->button_get(false) != BUTTON_NONE) 
    rb->yield();

  /* loop until the menu is finished */
  while (!done) {
    /* grab a button */
    btn = rb->button_get(true);

    /* handle the button */
    switch (btn) {
      case BUTTON_DOWN:
        /* select next item in list */
        sel_item = curr_item + 1;
        if (sel_item >= (int) num_items)
          sel_item = 0;
        select_item(title, curr_item, sel_item);
        curr_item = sel_item;
        break;
      case BUTTON_UP:
        /* select prev item in list */
        sel_item = curr_item - 1;
        if (sel_item < 0)
          sel_item = num_items - 1;
        select_item(title, curr_item, sel_item);
        curr_item = sel_item;
        break;
      case BUTTON_RIGHT:
        /* select current item */
        ret = curr_item;
        done = true;
        break;
      case BUTTON_LEFT:
      case BUTTON_SELECT:
        /* cancel out of menu */
        ret = MENU_CANCEL;
        done = true;
        break;
    }
    
    /* give the OS some love */
    rb->yield();
  }
  
  /* return selected item */
  return ret;
}
