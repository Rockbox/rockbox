/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: sokoban.c,v 0.01 2002/06/15 
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

#include <sprintf.h>
#include "config.h"
#include "sokoban.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>

#define SOKOBAN_TITLE   "Sokoban"
#define SOKOBAN_TITLE_FONT  2
#define NUM_LEVELS  25

int board[16][20];
int current_level=0;
int moves=0;
int row=0;
int col=0;
int boxes_to_go=0;
int current_spot=1;

/* 320 boxes per level */
char levels[320*NUM_LEVELS] = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000002220000000000000000023200000000000000000212222000000000000222414320000000000002314522200000000000022224200000000000000000232000000000000000002220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222000000000000000211120000000000000002544202220000000000021412023200000000000222122232000000000000221111320000000000002111211200000000000021112222000000000000222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222222200000000000002111112220000000000224222111200000000002151411412000000000021332141220000000000223321112000000000000222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222200000000000022221132000000000000211141120000000000002144143200000000000022522332000000000000022222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222200000000000000002512220000000000000021411200000000000002221212200000000000023212112000000000000234112120000000000002311141200000000000022222222000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222222200000000000022112152000000000000211121120000000000002414141200000000000021422112000000000022214121220000000000233333112000000000002222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222200000000000022211112000000000002231422122000000000023341411520000000000233141412200000000002222221120000000000000000222200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222200000000000000211112000000000000222444120000000000002514331200000000000021433322000000000000222211200000000000000002222000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222200000000000000002332000000000000000221322000000000000002114320000000000000221411220000000000002112441200000000000021151112000000000000222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222000000000000022211520000000000000211431220000000000002113431200000000000022211412000000000000002111220000000000000022222000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222222200000000000021121112000000000000214334120000000000002543112200000000000021433412000000000000211211120000000000002222222200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222220000000000000021111222000000000000214111120000000000222141221200000000002333141112000000000023334241220000000000222212141200000000000002115112000000000000022222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222222000000000000002111120000000000000021444220000000000000211233222000000000002211334120000000000002151111200000000000022222222000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222222220000000000002111231200000000000221143332000000000002114121320000000000221224212200000000002111411412000000000021112111120000000000222222251200000000000000002222000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222220000000000000233331200000000000022233342220000000000211424141200000000002144112412000000000021111211120000000000222215122200000000000002222200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222222220000000000002331111200000000000023341452000000000000242444220000000000002334141200000000000023311112000000000000222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222222000000000000211111120000000000002124411200000000000021333212000000000000223334122000000000000212214120000000000002411411200000000000021121152000000000000222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222200000000000002221112222000000000021114141120000000000214111415200000000002224422222000000000000211332000000000000002333320000000000000022222200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222222220000000000021111111200000000000214343152000000000002134343120000000000021434341200000000000211111112000000000002222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000022222222200000000002211111112000000000021112424120000000000214411343200000000002152223332000000000022220222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222220000000000000023133200000000000000231432000000000000022211422000000000000214114120000000000002124221200000000000021115112000000000000222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222200000000000000021112222000000000002212411120000000000021411441200000000000212423132000000000002111533320000000000022222222200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222200000000000022221112200000000000214141112000000000002523132120000000000021231321200000000000211141412000000000002211122220000000000002222200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222220000000000002222311220000000000021434311200000000000254212412000000000002143131120000000000022224241200000000000002313112000000000000022222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002222222200000000022221111312000000000211414143120000000002113222232200000000021434141520000000000211311112200000000002222222220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";


void load_level(int level_to_load) {
    int a = 0;
    int b = 0;
    int c = 0;
    current_spot=1;
    boxes_to_go = 0;
    /* load level into board */
    /* get to the current level in the level array */
    a = level_to_load*320;
    
    for(b=0 ; b<16 ; b++) {
        for (c=0 ; c<20 ; c++) {
            if (levels[a] == '0')
                board[b][c]=0;
            if (levels[a] == '1')
                board[b][c]=1;
            if (levels[a] == '2')
                board[b][c]=2;
            if (levels[a] == '3')
                board[b][c]=3;
            if (levels[a] == '4')
                board[b][c]=4;
            if (levels[a] == '5')
                board[b][c]=5;
            a++;
            if (board[b][c]==5) {
                row = b;
                col = c;
            }
            if (board[b][c]==3)
                boxes_to_go++;        
        }
    }
    return;
}

void update_screen(void) {
    int b = 0;
    int c = 0;
    char s[25];

    /* load the board to the screen */
    for(b=0 ; b<16 ; b++) {
        for (c=0 ; c<20 ; c++) {
   
            /* this is a black space */
            if (board[b][c]==0) {
                lcd_drawrect (c*4, b*4, c*4+3, b*4+3);
                lcd_drawrect (c*4+1, b*4+1, c*4+2, b*4+2);
            }    
            /* this is a wall */
            if (board[b][c]==2) {
                lcd_drawpixel (c*4, b*4);
                lcd_drawpixel (c*4+2, b*4);
                lcd_drawpixel (c*4+1, b*4+1);
                lcd_drawpixel (c*4+3, b*4+1);
                lcd_drawpixel (c*4,   b*4+2);
                lcd_drawpixel (c*4+2, b*4+2);
                lcd_drawpixel (c*4+1, b*4+3);
                lcd_drawpixel (c*4+3, b*4+3);
            }    
            /* this is a box */
            if (board[b][c]==4) {
                lcd_drawrect (c*4, b*4, c*4+3, b*4+3);
            }    
            /* this is a home location */
            if (board[b][c]==3) {
                lcd_drawrect (c*4+1, b*4+1, c*4+2, b*4+2);
            }  
            /* this is you */
            if (board[b][c]==5) {
                lcd_drawline (c*4+1, b*4, c*4+2, b*4);
                lcd_drawline (c*4, b*4+1, c*4+3, b*4+1);
                lcd_drawline (c*4+1, b*4+2, c*4+2, b*4+2);
                lcd_drawpixel (c*4, b*4+3);
                lcd_drawpixel (c*4+3, b*4+3);
            }
            /* this is a box on a home spot */ 
            if (board[b][c]==7) {
                lcd_drawrect (c*4, b*4, c*4+3, b*4+3);
                lcd_drawrect (c*4+1, b*4+1, c*4+2, b*4+2);                
            }       
        }
    }
    

    snprintf (s, sizeof(s), "%d", current_level+1);
    lcd_putsxy (86, 20, s, 0);
    snprintf (s, sizeof(s), "%d", moves);
    lcd_putsxy (86, 52, s, 0);

    lcd_drawrect (80,0,111,31);
    lcd_drawrect (80,32,111,63);
    lcd_putsxy (81, 10, "Level", 0);
    lcd_putsxy (81, 42, "Moves", 0);
    /* print out the screen */
    lcd_update();
}



void sokoban_loop(void) {
    int ii = 0;
    int b = 0;
    moves = 0;
    current_level = 0;
    load_level(current_level);
    update_screen();


    while(1) {
        b = button_get(false);

        if ( b & BUTTON_OFF ) {
            /* get out of here */
            return; 
        }


        if ( b & BUTTON_F3 ) {
            /* increase level */
            boxes_to_go=0; 
        }

        if ( b & BUTTON_F2 ) {
            /* same level */
            load_level(current_level);
            lcd_clear_display();
            update_screen();
            moves=0;
            
        }

        if ( b & BUTTON_F1 ) {
            /* previous level */
            if (current_level==0) {
                current_level=0;
            }
            else {
                current_level--;
            }
            load_level(current_level);
            lcd_clear_display();
            update_screen();
            moves=0;
        }

        if ( b & BUTTON_LEFT ) {
            /* if it is a blank spot */
            if (board[row][col-1]==1) {
                 board[row][col-1]=5;
                 board[row][col]=current_spot;
                 current_spot=1;
                 col--;
                 moves++;
            }
            /* if it is a home spot */
            else if (board[row][col-1]==3) {
                 board[row][col-1]=5;
                 board[row][col]=current_spot;
                 current_spot=3;
                 col--;
                 moves++;
            }
            else if (board[row][col-1]==4) {
                 /* if there is a wall then do not move the box */
                 if(board[row][col-2]==2) {
                     /* do nothing */
                 }
                 /* if we are going from blank to blank */
                 else if(board[row][col-2]==1) {
                     board[row][col-2]=board[row][col-1];
                     board[row][col-1]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=1;
                     col--;
                     moves++;
                 }
                 /* if we are going from a blank to home */
                 else if(board[row][col-2]==3) {
                     board[row][col-2]=7;
                     board[row][col-1]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=1;
                     col--; 
                     boxes_to_go--;
                     moves++;    
                 }        
            }
            else if (board[row][col-1]==7) {
                 /* if there is a wall then do not move the box */
                 if(board[row][col-2]==2) {
                     /* do nothing */
                 }
                 /* we are going from a home to a blank */
                 else if(board[row][col-2]==1) {
                     board[row][col-2]=4;
                     board[row][col-1]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=3;
                     col--;
                     boxes_to_go++;
                     moves++;
                 }
                 /* if we are going from a home to home */
                 else if(board[row][col-2]==3) {
                     board[row][col-2]=7;
                     board[row][col-1]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=3;
                     col--;
                     moves++;      
                 }
            }
            lcd_clear_display();
            update_screen();          
        }


        if ( b & BUTTON_RIGHT ) {
            /* if it is a blank spot */
            if (board[row][col+1]==1) {
                 board[row][col+1]=5;
                 board[row][col]=current_spot;
                 current_spot=1;
                 col++;
                 moves++;
            }
            /* if it is a home spot */
            else if (board[row][col+1]==3) {
                 board[row][col+1]=5;
                 board[row][col]=current_spot;
                 current_spot=3;
                 col++;
                 moves++;
            }
            else if (board[row][col+1]==4) {
                 /* if there is a wall then do not move the box */
                 if(board[row][col+2]==2) {
                     /* do nothing */
                 }
                 /* if we are going from blank to blank */
                 else if(board[row][col+2]==1) {
                     board[row][col+2]=board[row][col+1];
                     board[row][col+1]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=1;
                     col++;
                     moves++;
                 }
                 /* if we are going from a blank to home */
                 else if(board[row][col+2]==3) {
                     board[row][col+2]=7;
                     board[row][col+1]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=1;
                     col++; 
                     boxes_to_go--;
                     moves++;    
                 }        
            }
            else if (board[row][col+1]==7) {
                 /* if there is a wall then do not move the box */
                 if(board[row][col+2]==2) {
                     /* do nothing */
                 }
                 /* we are going from a home to a blank */
                 else if(board[row][col+2]==1) {
                     board[row][col+2]=4;
                     board[row][col+1]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=3;
                     col++;
                     boxes_to_go++;
                     moves++;
                 }
                 /* if we are going from a home to home */
                 else if(board[row][col+2]==3) {
                     board[row][col+2]=7;
                     board[row][col+1]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=3;
                     col++;
                     moves++;      
                 }
            }
            lcd_clear_display();
            update_screen();          
        }

        if ( b & BUTTON_UP ) {
            /* if it is a blank spot */
            if (board[row-1][col]==1) {
                 board[row-1][col]=5;
                 board[row][col]=current_spot;
                 current_spot=1;
                 row--;
                 moves++;
            }
            /* if it is a home spot */
            else if (board[row-1][col]==3) {
                 board[row-1][col]=5;
                 board[row][col]=current_spot;
                 current_spot=3;
                 row--;
                 moves++;
            }
            else if (board[row-1][col]==4) {
                 /* if there is a wall then do not move the box */
                 if(board[row-2][col]==2) {
                     /* do nothing */
                 }
                 /* if we are going from blank to blank */
                 else if(board[row-2][col]==1) {
                     board[row-2][col]=board[row-1][col];
                     board[row-1][col]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=1;
                     row--;
                     moves++;
                 }
                 /* if we are going from a blank to home */
                 else if(board[row-2][col]==3) {
                     board[row-2][col]=7;
                     board[row-1][col]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=1;
                     row--; 
                     boxes_to_go--;
                     moves++;    
                 }        
            }
            else if (board[row-1][col]==7) {
                 /* if there is a wall then do not move the box */
                 if(board[row-2][col]==2) {
                     /* do nothing */
                 }
                 /* we are going from a home to a blank */
                 else if(board[row-2][col]==1) {
                     board[row-2][col]=4;
                     board[row-1][col]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=3;
                     row--;
                     boxes_to_go++;
                     moves++;
                 }
                 /* if we are going from a home to home */
                 else if(board[row-2][col]==3) {
                     board[row-2][col]=7;
                     board[row-1][col]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=3;
                     row--;
                     moves++;      
                 }
            }
            lcd_clear_display();
            update_screen();          
        }

        if ( b & BUTTON_DOWN ) {
            /* if it is a blank spot */
            if (board[row+1][col]==1) {
                 board[row+1][col]=5;
                 board[row][col]=current_spot;
                 current_spot=1;
                 row++;
                 moves++;
            }
            /* if it is a home spot */
            else if (board[row+1][col]==3) {
                 board[row+1][col]=5;
                 board[row][col]=current_spot;
                 current_spot=3;
                 row++;
                 moves++;
            }
            else if (board[row+1][col]==4) {
                 /* if there is a wall then do not move the box */
                 if(board[row+2][col]==2) {
                     /* do nothing */
                 }
                 /* if we are going from blank to blank */
                 else if(board[row+2][col]==1) {
                     board[row+2][col]=board[row+1][col];
                     board[row+1][col]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=1;
                     row++;
                     moves++;
                 }
                 /* if we are going from a blank to home */
                 else if(board[row+2][col]==3) {
                     board[row+2][col]=7;
                     board[row+1][col]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=1;
                     row++; 
                     boxes_to_go--;
                     moves++;    
                 }        
            }
            else if (board[row+1][col]==7) {
                 /* if there is a wall then do not move the box */
                 if(board[row+2][col]==2) {
                     /* do nothing */
                 }
                 /* we are going from a home to a blank */
                 else if(board[row+2][col]==1) {
                     board[row+2][col]=4;
                     board[row+1][col]=board[row][col];
                     board[row][col]=current_spot;
                     current_spot=3;
                     row++;
                     boxes_to_go++;
                     moves++;
                 }
                 /* if we are going from a home to home */
                 else if(board[row+2][col]==3) {
                     board[row+2][col]=7;
                     board[row+1][col]=board[row][col];
                     board[row][col]=current_spot; 
                     current_spot=3;
                     row++;
                     moves++;      
                 }
            }
            lcd_clear_display();
            update_screen();          
        }


      
        if (boxes_to_go==0) {
            moves=0;
            current_level++;
            if (current_level == NUM_LEVELS) {
                for(ii=0; ii<30 ; ii++) {
                    lcd_clear_display();
                    lcd_putsxy(10, 20, "YOU WIN!!", 2);
                    lcd_update();
                    lcd_invertrect(0,0,111,63);
                    lcd_update();
                }
                return;
            }
            load_level(current_level);
            lcd_clear_display();
            update_screen();
        }
    }
}


void sokoban(void)
{
    int w, h;
    int len = strlen(SOKOBAN_TITLE);

    lcd_getfontsize(SOKOBAN_TITLE_FONT, &w, &h);

    /* Get horizontel centering for text */
    len *= w;
    if (len%2 != 0)
        len = ((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    lcd_clear_display();
    lcd_putsxy(LCD_WIDTH/2-len, (LCD_HEIGHT/2)-h, SOKOBAN_TITLE, 
               SOKOBAN_TITLE_FONT);

    lcd_update();
    sleep(HZ*2);

    lcd_clear_display();

    lcd_putsxy( 3,12,  "[Off] to stop", 0);
    lcd_putsxy( 3,22, "[F1] - level",0);
    lcd_putsxy( 3,32, "[F2] same level",0);
    lcd_putsxy( 3,42, "[F3] + level",0);

    lcd_update();
    sleep(HZ*2);
    lcd_clear_display();
    sokoban_loop();
}
