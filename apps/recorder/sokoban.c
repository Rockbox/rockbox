/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Eric Linenberg
 * February 2003: Robert Hak performs a cleanup/rewrite/feature addition.
 *                Eric smiles.  Bjorn cris.  Linus say 'huh?'.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "options.h"

#ifdef USE_GAMES

#include <sprintf.h>
#include "sokoban.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"
#include "screens.h"
#include "font.h"
#include "file.h"

#include "debug.h"
#include "sokoban_levels.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>
#include "lang.h"
#define SOKOBAN_TITLE       "Sokoban"
#define SOKOBAN_TITLE_FONT  2
#define LEVELS_FILE         "/sokoban.levels"
#define NUM_LEVELS          sizeof(levels)/320

#define ROWS                16
#define COLS                20
#define MAX_UNDOS           5

static void init_undo(void);
static void undo(void);
static void add_undo(int button);

static void init_boards(void);
static void load_level(short level);
static void draw_level(short level);
static void update_screen(void);
static bool sokoban_loop(void);

/* The Location, Undo and LevelInfo structs are OO-flavored.  
 * (oooh!-flavored as Schnueff puts it.)  It makes more you have to know, 
 * but the overall data layout becomes more manageable. */

/* We use the same three values in 2 structs.  Makeing them a struct
 * hopefully ensures that if you change things in one, the other changes
 * as well. */
struct LevelInfo {
    short level;
    short moves;
    short boxes_to_go;
};

/* What a given location on the board looks like at a given time */
struct Location {
    char spot;
    short row;
    short col;
};

/* A single level of undo.  Each undo move can affect upto,
 * but not more then, 3 spots on the board */
struct Undo {
    struct LevelInfo level;
    struct Location location[3];
};

/* Our full undo history */
static struct UndoInfo {
    short count;    /* How many undos are there in history */
    short current;  /* Which history is the current undo */
    struct Undo history[MAX_UNDOS];
} undo_info;

/* Our playing board */
static struct BoardInfo {
    char board[ROWS][COLS];
    struct LevelInfo level;
    struct Location player;
} current_info;


static void init_undo(void)
{
    undo_info.count = 0;
    undo_info.current = 0;
}

static void undo(void)
{
    struct Undo *undo;
    int i = 0;
    short row, col;

    if (undo_info.count == 0)
        return;

    /* Update board info */
    undo = &undo_info.history[undo_info.current];
    
    current_info.level = undo->level;
    current_info.player = undo->location[0];

    row = undo->location[0].row;
    col = undo->location[0].col;
    current_info.board[row][col] = '@';

    /* Update the two other possible spots */
    for (i = 1; i < 3; i++) {
        if (undo->location[i].spot != '\0') {
            row = undo->location[i].row;
            col = undo->location[i].col;
            current_info.board[row][col] = undo->location[i].spot;
            undo->location[i].spot = '\0';
        }
    }

    /* Remove this undo from the list */
    if (undo_info.current == 0) {
        if (undo_info.count > 1)
            undo_info.current = MAX_UNDOS - 1;
    } else {
        undo_info.current--;
    }
    
    undo_info.count--;

    return;
}

static void add_undo(int button)
{
    struct Undo *undo;
    int row, col, i;
    bool storable;

    if ((button != BUTTON_LEFT) && (button != BUTTON_RIGHT) &&
        (button != BUTTON_UP) && (button != BUTTON_DOWN))
        return;

    if (undo_info.count != 0) {
        if (undo_info.current < (MAX_UNDOS - 1)) 
            undo_info.current++;
        else
            undo_info.current = 0;
    }

    /* Make what follows more readable */
    undo = &undo_info.history[undo_info.current];

    /* Store our level info */
    undo->level = current_info.level;

    /* Store our player info */
    undo->location[0] = current_info.player;

    /* Now we need to store upto 2 blocks that may be affected.  
     * If player.spot is NULL, then there is no info stored 
     * for that block */

    row = current_info.player.row;
    col = current_info.player.col;

    /* This must stay as _1_ because the first block (0) is the player */
    for (i = 1; i < 3; i++) {
        storable = true;

        switch (button) {
        case BUTTON_LEFT:
            col--;
            if (col < 0) 
                storable = false;
            break;

        case BUTTON_RIGHT:
            col++;
            if (col >= COLS) 
                storable = false;
            break;

        case BUTTON_UP:
            row--;
            if (row < 0)
                storable = false;
            break;
            
        case BUTTON_DOWN:
            row++;
            if (row >= ROWS)
                storable = false;
            break;

        default:
            return;
        }

        if (storable) {
            undo->location[i].col = col;
            undo->location[i].row = row;
            undo->location[i].spot = current_info.board[row][col];
        } else {
            undo->location[i].spot = '\0';
        }
    }

    if (undo_info.count < MAX_UNDOS) 
        undo_info.count++;
}

static void init_boards(void)
{
    current_info.level.level = 0;
    current_info.level.moves = 0;
    current_info.level.boxes_to_go = 0;
    current_info.player.row = 0;
    current_info.player.col = 0;
    current_info.player.spot = ' ';

    init_undo();
}

static void load_level(short level_to_load) 
{
    short a = 0, b = 0, c = 0;

    current_info.player.spot=' ';
    current_info.level.boxes_to_go = 0;
    current_info.level.moves = 0;

    for (b = 0; b < ROWS; b++) {
        for (c = 0; c < COLS; c++) {
            current_info.board[b][c] = levels[level_to_load][a];
            a++;

            if (current_info.board[b][c] == '@') {
                current_info.player.row = b;
                current_info.player.col = c;
            }

            if (current_info.board[b][c] == '.')
                current_info.level.boxes_to_go++;        
        }
    }

    return;
}

static void update_screen(void) 
{
    short b = 0, c = 0;
    short rows = 0, cols = 0;
    char s[25];
    
    short magnify = 4;

    /* load the board to the screen */
    for (rows=0 ; rows < ROWS ; rows++) {
        for (cols = 0 ; cols < COLS ; cols++) {
            c = cols * magnify;
            b = rows * magnify;

            switch(current_info.board[rows][cols]) {
            case 'X': /* black space */
                lcd_drawrect(c, b, magnify, magnify);
                lcd_drawrect(c+1, b+1, 2, 2);
                break;
                
            case '#': /* this is a wall */
                lcd_drawpixel(c, b);
                lcd_drawpixel(c+2, b);
                lcd_drawpixel(c+1, b+1);
                lcd_drawpixel(c+3, b+1);
                lcd_drawpixel(c, b+2);
                lcd_drawpixel(c+2, b+2);
                lcd_drawpixel(c+1, b+3);
                lcd_drawpixel(c+3, b+3);
                break;

            case '.': /* this is a home location */
                lcd_drawrect(c+1, b+1, 2, 2);
                break;

            case '$': /* this is a box */
                lcd_drawrect(c, b, magnify, magnify);
                break;

            case '@': /* this is you */
                lcd_drawline(c+1, b, c+2, b);
                lcd_drawline(c, b+1, c+3, b+1);
                lcd_drawline(c+1, b+2, c+2, b+2);

                lcd_drawpixel(c, b+3);
                lcd_drawpixel(c+3, b+3);
                break;

            case '%': /* this is a box on a home spot */ 
                lcd_drawrect(c, b, magnify, magnify);
                lcd_drawrect(c+1, b+1, 2, 2);
                break;
            }
        }
    }
    

    snprintf(s, sizeof(s), "%d", current_info.level.level+1);
    lcd_putsxy(86, 22, s);
    snprintf(s, sizeof(s), "%d", current_info.level.moves);
    lcd_putsxy(86, 54, s);

    lcd_drawrect(80,0,32,32);
    lcd_drawrect(80,32,32,64);
    lcd_putsxy(81, 10, str(LANG_SOKOBAN_LEVEL));
    lcd_putsxy(81, 42, str(LANG_SOKOBAN_MOVE));

    /* print out the screen */
    lcd_update();
}

static void draw_level(short level)
{
    load_level(level);
    lcd_clear_display();
    update_screen();
}

static bool sokoban_loop(void)
{
    char new_spot;
    bool moved = true;
    int i = 0, button = 0;
    short r = 0, c = 0;

    current_info.level.level = 0;

    load_level(current_info.level.level);
    update_screen(); 

    while (1) {
        moved = true;

        r = current_info.player.row;
        c = current_info.player.col;

        button = button_get(true);

        add_undo(button);

        switch(button) 
        {
        case BUTTON_OFF:
            /* get out of here */
            return false; 

        case BUTTON_ON:
        case BUTTON_ON | BUTTON_REPEAT:
            /* this is UNDO */
            undo();
            lcd_clear_display();
            update_screen(); 
            moved = false;
            break;

        case BUTTON_F3:
        case BUTTON_F3 | BUTTON_REPEAT:
            /* increase level */
            init_undo();
            current_info.level.boxes_to_go=0;
            moved = true;
            break;

        case BUTTON_F1:
        case BUTTON_F1 | BUTTON_REPEAT:
            /* previous level */
            init_undo();
            if (current_info.level.level)
                current_info.level.level--;

            draw_level(current_info.level.level);
            moved = false;
            break;

        case BUTTON_F2:
        case BUTTON_F2 | BUTTON_REPEAT:
            /* same level */
            init_undo();
            draw_level(current_info.level.level);
            moved = false;
            break;

        case BUTTON_LEFT:
            switch(current_info.board[r][c-1]) 
            {
            case ' ': /* if it is a blank spot */
            case '.': /* if it is a home spot */
                new_spot = current_info.board[r][c-1];
                current_info.board[r][c-1] = '@';
                current_info.board[r][c] = current_info.player.spot;
                current_info.player.spot = new_spot;
                break;

            case '$':
                switch(current_info.board[r][c-2])
                {
                case ' ': /* going from blank to blank */
                    current_info.board[r][c-2] = current_info.board[r][c-1];
                    current_info.board[r][c-1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = ' ';
                    break;

                case '.': /* going from a blank to home */
                    current_info.board[r][c-2] = '%';
                    current_info.board[r][c-1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = ' ';
                    current_info.level.boxes_to_go--;
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            case '%':
                switch(current_info.board[r][c-2]) {
                case ' ': /* we are going from a home to a blank */
                    current_info.board[r][c-2] = '$';
                    current_info.board[r][c-1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = '.';
                    current_info.level.boxes_to_go++;
                    break;

                case '.': /* if we are going from a home to home */
                    current_info.board[r][c-2] = '%';
                    current_info.board[r][c-1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = '.';
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            default:
                moved = false;
                break;
            }

            if (moved)
                current_info.player.col--;
            break;

        case BUTTON_RIGHT: /* if it is a blank spot */
            switch(current_info.board[r][c+1]) {
            case ' ':
            case '.': /* if it is a home spot */
                new_spot = current_info.board[r][c+1];
                current_info.board[r][c+1] = '@';
                current_info.board[r][c] = current_info.player.spot;
                current_info.player.spot = new_spot;
                break;

            case '$': 
                switch(current_info.board[r][c+2]) {
                case ' ': /* going from blank to blank */
                    current_info.board[r][c+2] = current_info.board[r][c+1];
                    current_info.board[r][c+1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = ' ';
                    break;

                case '.': /* going from a blank to home */
                    current_info.board[r][c+2] = '%';
                    current_info.board[r][c+1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = ' ';
                    current_info.level.boxes_to_go--;
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            case '%':
                switch(current_info.board[r][c+2]) {
                case ' ': /* going from a home to a blank */
                    current_info.board[r][c+2] = '$';
                    current_info.board[r][c+1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = '.';
                    current_info.level.boxes_to_go++;
                    break;

                case '.':
                    current_info.board[r][c+2] = '%';
                    current_info.board[r][c+1] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = '.';
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            default:
                moved = false;
                break;
            }

            if (moved)
                current_info.player.col++;
            break;

        case BUTTON_UP:
            switch(current_info.board[r-1][c]) {
            case ' ': /* if it is a blank spot */
            case '.': /* if it is a home spot */
                new_spot = current_info.board[r-1][c];
                current_info.board[r-1][c] = '@';
                current_info.board[r][c] = current_info.player.spot;
                current_info.player.spot = new_spot;
                break;

            case '$':
                switch(current_info.board[r-2][c]) {
                case ' ': /* going from blank to blank */
                    current_info.board[r-2][c] = current_info.board[r-1][c];
                    current_info.board[r-1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = ' ';
                    break;

                case '.': /* going from a blank to home */
                    current_info.board[r-2][c] = '%';
                    current_info.board[r-1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = ' ';
                    current_info.level.boxes_to_go--;
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            case '%':
                switch(current_info.board[r-2][c]) {
                case ' ': /* we are going from a home to a blank */
                    current_info.board[r-2][c] = '$';
                    current_info.board[r-1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = '.';
                    current_info.level.boxes_to_go++;
                    break;

                case '.': /* if we are going from a home to home */
                    current_info.board[r-2][c] = '%';
                    current_info.board[r-1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = '.';
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            default:
                moved = false;
                break;
            }

            if (moved)
                current_info.player.row--;
            break;

        case BUTTON_DOWN:
            switch(current_info.board[r+1][c]) {
            case ' ': /* if it is a blank spot */
            case '.': /* if it is a home spot */
                new_spot = current_info.board[r+1][c];
                current_info.board[r+1][c] = '@';
                current_info.board[r][c] = current_info.player.spot;
                current_info.player.spot = new_spot;
                break;

            case '$':
                switch(current_info.board[r+2][c]) {
                case ' ': /* going from blank to blank */
                    current_info.board[r+2][c] = current_info.board[r+1][c];
                    current_info.board[r+1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = ' ';
                    break;

                case '.': /* going from a blank to home */
                    current_info.board[r+2][c] = '%';
                    current_info.board[r+1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = ' ';
                    current_info.level.boxes_to_go--;
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            case '%':
                switch(current_info.board[r+2][c]) {
                case ' ': /* going from a home to a blank */
                    current_info.board[r+2][c] = '$';
                    current_info.board[r+1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot;
                    current_info.player.spot = '.';
                    current_info.level.boxes_to_go++;
                    break;

                case '.': /* going from a home to home */
                    current_info.board[r+2][c] = '%';
                    current_info.board[r+1][c] = current_info.board[r][c];
                    current_info.board[r][c] = current_info.player.spot; 
                    current_info.player.spot = '.';
                    break;

                default:
                    moved = false;
                    break;
                }
                break;

            default:
                moved = false;
                break;
            }

            if (moved)
                current_info.player.row++;
            break;

        case SYS_USB_CONNECTED:
            usb_screen();
            return true;

        default:
            moved = false;
            break;
        }
        
        if (moved) {
            current_info.level.moves++;
            lcd_clear_display();
            update_screen(); 
        }

        /* We have completed this level */
        if (current_info.level.boxes_to_go == 0) {
            current_info.level.level++;

			/* clear undo stats */
			init_undo();

            lcd_clear_display();
			
            if (current_info.level.level == NUM_LEVELS) {
                lcd_putsxy(10, 20, str(LANG_SOKOBAN_WIN));

                for (i = 0; i < 30000 ; i++) {
                    lcd_invertrect(0, 0, 111, 63);
                    lcd_update();

                    if (button_get(false))
                        break;
                }

                return false;
            }

            load_level(current_info.level.level);
            update_screen();
        }

    } /* end while */

    return false;
}


bool sokoban(void)
{
    bool result;
    int w, h;
    int len;

    lcd_setfont(FONT_SYSFIXED);

    lcd_getstringsize(SOKOBAN_TITLE, &w, &h);

    /* Get horizontel centering for text */
    len = w;
    if (len%2 != 0)
        len =((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    lcd_clear_display();
    lcd_putsxy(LCD_WIDTH/2-len,(LCD_HEIGHT/2)-h, SOKOBAN_TITLE);

    lcd_update();
    sleep(HZ*2);

    lcd_clear_display();

    lcd_putsxy(3,  6, str(LANG_SOKOBAN_QUIT));
    lcd_putsxy(3, 16, str(LANG_SOKOBAN_ON));
    lcd_putsxy(3, 26, str(LANG_SOKOBAN_F1));
    lcd_putsxy(3, 36, str(LANG_SOKOBAN_F2));
    lcd_putsxy(3, 46, str(LANG_SOKOBAN_F3));

    lcd_update();
    sleep(HZ*2);
    lcd_clear_display();

    init_boards();
    result = sokoban_loop();

    lcd_setfont(FONT_UI);

    return result;
}

#endif




