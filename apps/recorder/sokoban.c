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

#include "sokoban_levels.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>
#include "lang.h"
#define SOKOBAN_TITLE   "Sokoban"
#define SOKOBAN_TITLE_FONT  2
#define NUM_LEVELS  sizeof(levels)/320

static void load_level(int);
static void update_screen(void);
static bool sokoban_loop(void);
static void copy_current_state_to_undo(void);
static void copy_current_undo_to_state(void);

static char board[16][20];
static char undo_board[16][20];
static int current_level=0;
static int undo_current_level=0;
static int moves=0;
static int undo_moves=0;
static int row=0;
static int undo_row=0;
static int col=0;
static int undo_col=0;
static int boxes_to_go=0;
static int undo_boxes_to_go=0;
static char current_spot= ' ';
static char undo_current_spot=' ';



static void copy_current_state_to_undo(void) {
    int a = 0;
    int b = 0;
    
    for (a=0 ; a<16 ; a++) {
        for (b=0; b<16 ; b++) {
            undo_board[a][b] = board[a][b];
        }
    }
    undo_current_level = current_level;
    undo_moves = moves;
    undo_row = row;
    undo_col = col;
    undo_boxes_to_go = boxes_to_go;
    undo_current_spot = current_spot;

    return;
}

static void copy_current_undo_to_state(void) {
    int a = 0;
    int b = 0;
    
    for (a=0 ; a<16 ; a++) {
        for (b=0; b<16 ; b++) {
            board[a][b] = undo_board[a][b];
        }
    }
    current_level = undo_current_level;
    moves = undo_moves-1;
    row = undo_row;
    col = undo_col;
    boxes_to_go = undo_boxes_to_go;
    current_spot = undo_current_spot;
    return;
}

static void load_level (int level_to_load) {
    int a = 0;
    int b = 0;
    int c = 0;
    current_spot=' ';
    boxes_to_go = 0;
    /* load level into board */
    /* get to the current level in the level array */
    
    for(b=0 ; b<16 ; b++) {
        for (c=0 ; c<20 ; c++) {
            board[b][c] = levels[level_to_load][a]/* - '0'*/;
            a++;
            if (board[b][c]=='@') {
                row = b;
                col = c;
            }
            if (board[b][c]=='.')
                boxes_to_go++;        
        }
    }
    return;
}

static void update_screen(void) {
    int b = 0;
    int c = 0;
    char s[25];

    /* load the board to the screen */
    for(b=0 ; b<16 ; b++) {
        for (c=0 ; c<20 ; c++) {
            switch ( board[b][c] ) {
                case 'X': /* this is a black space */
                    lcd_drawrect (c*4, b*4, 4, 4);
                    lcd_drawrect (c*4+1, b*4+1, 2, 2);
                    break;

                case '#': /* this is a wall */
                    lcd_drawpixel (c*4, b*4);
                    lcd_drawpixel (c*4+2, b*4);
                    lcd_drawpixel (c*4+1, b*4+1);
                    lcd_drawpixel (c*4+3, b*4+1);
                    lcd_drawpixel (c*4,   b*4+2);
                    lcd_drawpixel (c*4+2, b*4+2);
                    lcd_drawpixel (c*4+1, b*4+3);
                    lcd_drawpixel (c*4+3, b*4+3);
                    break;

                case '.': /* this is a home location */
                    lcd_drawrect (c*4+1, b*4+1, 2, 2);
                    break;

                case '$': /* this is a box */
                    lcd_drawrect (c*4, b*4, 4, 4);
                    break;

                case '@': /* this is you */
                    lcd_drawline (c*4+1, b*4, c*4+2, b*4);
                    lcd_drawline (c*4, b*4+1, c*4+3, b*4+1);
                    lcd_drawline (c*4+1, b*4+2, c*4+2, b*4+2);
                    lcd_drawpixel (c*4, b*4+3);
                    lcd_drawpixel (c*4+3, b*4+3);
                    break;

                case '%': /* this is a box on a home spot */ 
                    lcd_drawrect (c*4, b*4, 4, 4);
                    lcd_drawrect (c*4+1, b*4+1, 2, 2);
                    break;
            }
        }
    }
    

    snprintf (s, sizeof(s), "%d", current_level+1);
    lcd_putsxy (86, 22, s);
    snprintf (s, sizeof(s), "%d", moves);
    lcd_putsxy (86, 54, s);

    lcd_drawrect (80,0,32,32);
    lcd_drawrect (80,32,32,64);
    lcd_putsxy (81, 10, str(LANG_SOKOBAN_LEVEL));
    lcd_putsxy (81, 42, str(LANG_SOKOBAN_MOVE));
    /* print out the screen */
    lcd_update();
}



static bool sokoban_loop(void)
{
    int ii = 0;
    moves = 0;
    current_level = 0;
    load_level(current_level);
    update_screen();

    while(1) {
       
        bool idle = false;
        switch ( button_get(true) ) {
            case BUTTON_OFF:
                /* get out of here */
                return false; 

            case BUTTON_F3:
                /* increase level */
                boxes_to_go=0;
                idle=true;
                break;

            case BUTTON_ON:
                /* this is UNDO */
                copy_current_undo_to_state();
                break;

            case BUTTON_F2:
                /* same level */
                load_level(current_level);
                moves=0;
                idle=true;
                load_level(current_level);
                lcd_clear_display();
                update_screen();
                break;
            
            case BUTTON_F1:
                /* previous level */
                if (current_level)
                    current_level--;
                load_level(current_level);
                moves=0;
                idle=true;
                load_level(current_level);
                lcd_clear_display();
                update_screen();
                break;

            case BUTTON_LEFT:
                copy_current_state_to_undo();
                switch ( board[row][col-1] ) {
                    case ' ': /* if it is a blank spot */
                        board[row][col-1]='@';
                        board[row][col]=current_spot;
                        current_spot=' ';
                        break;

                    case '.': /* if it is a home spot */
                        board[row][col-1]='@';
                        board[row][col]=current_spot;
                        current_spot='.';
                        break;

                    case '$':
                        switch ( board[row][col-2] ) {
                            case ' ': /* if we are going from blank to blank */
                                board[row][col-2]=board[row][col-1];
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=' ';
                                break;

                            case '.': /* if we are going from a blank to home */
                                board[row][col-2]='%';
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=' ';
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case '%':
                        switch ( board[row][col-2] ) {
                            case ' ': /* we are going from a home to a blank */
                                board[row][col-2]='$';
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot='.';
                                boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                board[row][col-2]='%';
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot='.';
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    col--;
                break;

            case BUTTON_RIGHT: /* if it is a blank spot */
                copy_current_state_to_undo();
                switch ( board[row][col+1] ) {
                    case ' ':
                        board[row][col+1]='@';
                        board[row][col]=current_spot;
                        current_spot=' ';
                        break;

                    case '.': /* if it is a home spot */
                        board[row][col+1]='@';
                        board[row][col]=current_spot;
                        current_spot='.';
                        break;

                    case '$': 
                        switch ( board[row][col+2] ) {
                            case ' ': /* if we are going from blank to blank */
                                board[row][col+2]=board[row][col+1];
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=' ';
                                break;

                            case '.': /* if we are going from a blank to home */
                                board[row][col+2]='%';
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=' ';
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case '%':
                        switch ( board[row][col+2] ) {
                            case ' ': /* we are going from a home to a blank */
                                board[row][col+2]='$';
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot='.';
                                boxes_to_go++;
                                break;

                            case '.':
                                board[row][col+2]='%';
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot='.';
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    col++;
                break;

            case BUTTON_UP:
                copy_current_state_to_undo();
                switch ( board[row-1][col] ) {
                    case ' ': /* if it is a blank spot */
                        board[row-1][col]='@';
                        board[row][col]=current_spot;
                        current_spot=' ';
                        break;

                    case '.': /* if it is a home spot */
                        board[row-1][col]='@';
                        board[row][col]=current_spot;
                        current_spot='.';
                        break;

                    case '$':
                        switch ( board[row-2][col] ) {
                            case ' ': /* if we are going from blank to blank */
                                board[row-2][col]=board[row-1][col];
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=' ';
                                break;

                            case '.': /* if we are going from a blank to home */
                                board[row-2][col]='%';
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=' ';
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case '%':
                        switch ( board[row-2][col] ) {
                            case ' ': /* we are going from a home to a blank */
                                board[row-2][col]='$';
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot='.';
                                boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                board[row-2][col]='%';
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot='.';
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    row--;
                break;

            case BUTTON_DOWN:
                copy_current_state_to_undo();
                switch ( board[row+1][col] ) {
                    case ' ': /* if it is a blank spot */
                        board[row+1][col]='@';
                        board[row][col]=current_spot;
                        current_spot=' ';
                        break;

                    case '.': /* if it is a home spot */
                        board[row+1][col]='@';
                        board[row][col]=current_spot;
                        current_spot='.';
                        break;

                    case '$':
                        switch ( board[row+2][col] ) {
                            case ' ': /* if we are going from blank to blank */
                                board[row+2][col]=board[row+1][col];
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=' ';
                                break;

                            case '.': /* if we are going from a blank to home */
                                board[row+2][col]='%';
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=' ';
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case '%':
                        switch ( board[row+2][col] ) {
                            case ' ': /* we are going from a home to a blank */
                                board[row+2][col]='$';
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot='.';
                                boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                board[row+2][col]='%';
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot='.';
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    row++;
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                return true;

            default:
                idle = true;
                break;
        }
        
        if (!idle) {
            moves++;
            lcd_clear_display();
            update_screen();
        }

        if (boxes_to_go==0) {
            moves=0;
            current_level++;
            if (current_level == NUM_LEVELS) {
                for(ii=0; ii<300 ; ii++) {
                    lcd_clear_display();
                    lcd_putsxy(10, 20, str(LANG_SOKOBAN_WIN));
                    lcd_update();
                    lcd_invertrect(0,0,111,63);
                    lcd_update();
                    if ( button_get(false) )
                        return false;
                }
                return false;
            }
            load_level(current_level);
            lcd_clear_display();
            update_screen();
        }
    }

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
        len = ((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    lcd_clear_display();
    lcd_putsxy(LCD_WIDTH/2-len, (LCD_HEIGHT/2)-h, SOKOBAN_TITLE);

    lcd_update();
    sleep(HZ*2);

    lcd_clear_display();

    lcd_putsxy( 3,12, str(LANG_SOKOBAN_QUIT));
    lcd_putsxy( 3,22, str(LANG_SOKOBAN_F1));
    lcd_putsxy( 3,32, str(LANG_SOKOBAN_F2));
    lcd_putsxy( 3,42, str(LANG_SOKOBAN_F3));

    lcd_update();
    sleep(HZ*2);
    lcd_clear_display();
    result = sokoban_loop();

    lcd_setfont(FONT_UI);

    return result;
}

#endif
