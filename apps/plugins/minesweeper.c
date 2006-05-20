/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 dionoea (Antoine Cellerier)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************************************************************
Mine Sweeper by dionoea
*****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/*what the minesweeper() function can return */
#define MINESWEEPER_USB  3
#define MINESWEEPER_QUIT 2
#define MINESWEEPER_LOSE 1
#define MINESWEEPER_WIN  0

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define MINESWP_UP BUTTON_UP
#define MINESWP_DOWN BUTTON_DOWN
#define MINESWP_QUIT BUTTON_OFF
#define MINESWP_START BUTTON_ON
#define MINESWP_TOGGLE BUTTON_PLAY
#define MINESWP_TOGGLE2 BUTTON_F1
#define MINESWP_DISCOVER BUTTON_ON
#define MINESWP_DISCOVER2 BUTTON_F2
#define MINESWP_INFO BUTTON_F3
#define MINESWP_RIGHT (BUTTON_F1 | BUTTON_RIGHT)
#define MINESWP_LEFT (BUTTON_F1 | BUTTON_LEFT)

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MINESWP_UP BUTTON_UP
#define MINESWP_DOWN BUTTON_DOWN
#define MINESWP_QUIT BUTTON_OFF
#define MINESWP_START BUTTON_MENU
#define MINESWP_TOGGLE_PRE BUTTON_MENU
#define MINESWP_TOGGLE (BUTTON_MENU | BUTTON_REL)
#define MINESWP_DISCOVER (BUTTON_MENU | BUTTON_REPEAT)
#define MINESWP_INFO (BUTTON_MENU | BUTTON_OFF)
#define MINESWP_RIGHT (BUTTON_MENU | BUTTON_RIGHT)
#define MINESWP_LEFT (BUTTON_MENU | BUTTON_LEFT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MINESWP_UP BUTTON_UP
#define MINESWP_DOWN BUTTON_DOWN
#define MINESWP_QUIT BUTTON_OFF
#define MINESWP_START BUTTON_SELECT
#define MINESWP_TOGGLE BUTTON_ON
#define MINESWP_DISCOVER BUTTON_SELECT
#define MINESWP_INFO BUTTON_MODE
#define MINESWP_RIGHT (BUTTON_ON | BUTTON_RIGHT)
#define MINESWP_LEFT (BUTTON_ON | BUTTON_LEFT)

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
#define MINESWP_UP BUTTON_SCROLL_BACK
#define MINESWP_DOWN BUTTON_SCROLL_FWD
#define MINESWP_QUIT BUTTON_MENU
#define MINESWP_START BUTTON_SELECT
#define MINESWP_TOGGLE BUTTON_PLAY
#define MINESWP_DISCOVER (BUTTON_SELECT | BUTTON_PLAY)
#define MINESWP_INFO (BUTTON_SELECT | BUTTON_MENU)
#define MINESWP_RIGHT (BUTTON_SELECT | BUTTON_RIGHT)
#define MINESWP_LEFT (BUTTON_SELECT | BUTTON_LEFT)

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define MINESWP_UP BUTTON_UP
#define MINESWP_DOWN BUTTON_DOWN
#define MINESWP_QUIT BUTTON_POWER
#define MINESWP_START BUTTON_REC
#define MINESWP_TOGGLE BUTTON_PLAY
#define MINESWP_DISCOVER BUTTON_SELECT
#define MINESWP_INFO (BUTTON_REC | BUTTON_PLAY)
#define MINESWP_RIGHT (BUTTON_PLAY | BUTTON_RIGHT)
#define MINESWP_LEFT (BUTTON_PLAY | BUTTON_LEFT)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define MINESWP_UP BUTTON_UP
#define MINESWP_DOWN BUTTON_DOWN
#define MINESWP_QUIT BUTTON_A
#define MINESWP_START BUTTON_SELECT
#define MINESWP_TOGGLE BUTTON_POWER
#define MINESWP_DISCOVER BUTTON_SELECT
#define MINESWP_INFO BUTTON_MENU
#define MINESWP_RIGHT (BUTTON_SELECT | BUTTON_RIGHT)
#define MINESWP_LEFT (BUTTON_SELECT | BUTTON_LEFT)

#endif

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;


/* define how numbers are displayed (that way we don't have to */
/* worry about fonts) */
static unsigned char num[10][8] = {
    /*reading the sprites:
      on screen f123
                4567
                890a
                bcde

      in binary b84f
                c951
                d062
                ea73
    */

    /* 0 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x00, /* ........ */
     0x00, /* ........ */
     0x00, /* ........ */
     0x00, /* ........ */
     0x00, /* ........ */
     0x00},/* ........ */
    /* 1 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x00, /* ...OO... */
     0x44, /* ....O... */
     0x7c, /* ....O... */
     0x40, /* ....O... */
     0x00, /* ...OOO.. */
     0x00},/* ........ */
    /* 2 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x48, /* ...OO... */
     0x64, /* ..O..O.. */
     0x54, /* ....O... */
     0x48, /* ...O.... */
     0x00, /* ..OOOO.. */
     0x00},/* ........ */
    /* 3 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x44, /* ..OOO... */
     0x54, /* .....O.. */
     0x54, /* ...OO... */
     0x28, /* .....O.. */
     0x00, /* ..OOO... */
     0x00},/* ........ */
    /* 4 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x1c, /* ..O..... */
     0x10, /* ..O..... */
     0x70, /* ..OOOO.. */
     0x10, /* ....O... */
     0x00, /* ....O... */
     0x00},/* ........ */
    /* 5 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x5c, /* ..OOOO.. */
     0x54, /* ..O..... */
     0x54, /* ..OOO... */
     0x24, /* .....O.. */
     0x00, /* ..OOO... */
     0x00},/* ........ */
    /* 6 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x38, /* ...OOO.. */
     0x54, /* ..O..... */
     0x54, /* ..OOO... */
     0x24, /* ..O..O.. */
     0x00, /* ...OO... */
     0x00},/* ........ */
    /* 7 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x44, /* ..OOOO.. */
     0x24, /* .....O.. */
     0x14, /* ....O... */
     0x0c, /* ...O.... */
     0x00, /* ..O..... */
     0x00},/* ........ */
    /* 8 */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x28, /* ...OO... */
     0x54, /* ..O..O.. */
     0x54, /* ...OO... */
     0x28, /* ..O..O.. */
     0x00, /* ...OO... */
     0x00},/* ........ */
     /* mine */
    {0x00, /* ........ */
     0x00, /* ........ */
     0x18, /* ...OO... */
     0x3c, /* ..OOOO.. */
     0x3c, /* ..OOOO.. */
     0x18, /* ...OO... */
     0x00, /* ........ */
     0x00},/* ........ */

};

/* the tile struct
if there is a mine, mine is true
if tile is known by player, known is true
if tile has a flag, flag is true
neighbors is the total number of mines arround tile
*/
typedef struct tile {
    unsigned char mine : 1;
    unsigned char known : 1;
    unsigned char flag : 1;
    unsigned char neighbors : 4;
} tile;

/* the height and width of the field */
int height = LCD_HEIGHT/8;
int width = LCD_WIDTH/8;

/* The Minefield. Caution it is defined as Y, X! Not the opposite. */
tile minefield[LCD_HEIGHT/8][LCD_WIDTH/8];

/* total number of mines on the game */
int mine_num = 0;

/* percentage of mines on minefield used durring generation */
int p=16;

/* number of tiles left on the game */
int tiles_left;

/* Because mines are set after the first move... */
bool no_mines = true;

/* We need a stack (created on discover()) for the cascade algorithm. */
int stack_pos = 0;

/* Functions to center the board on screen. */
int c_height(void){
    return LCD_HEIGHT/16 - height/2;
}

int c_width(void){
    return LCD_WIDTH/16 - width/2;
}

void push (int *stack, int y, int x){

    if(stack_pos <= height*width){
        stack_pos++;
        stack[stack_pos] = y;
        stack_pos++;
        stack[stack_pos] = x;
    }
}

/* Unveil tiles and push them to stack if they are empty. */
void unveil(int *stack, int y, int x){
    
    if(x < c_width() || y < c_height() || x > c_width() + width-1
       || y > c_height() + height-1 || minefield[y][x].known
       || minefield[y][x].mine || minefield[y][x].flag) return;
        
    if(minefield[y][x].neighbors == 0){
        minefield[y][x].known = 1;
        push(stack, y, x);
    } else
        minefield[y][x].known = 1;
}

void discover(int y, int x){
    
    int stack[height*width];
    
    /* Selected tile. */
    if(x < c_width() || y < c_height() || x > c_width() + width-1
       || y > c_height() + height-1 || minefield[y][x].known
       || minefield[y][x].mine || minefield[y][x].flag) return;
                
    minefield[y][x].known = 1;
    /* Exit if the tile is not empty. (no mines nearby) */
    if(minefield[y][x].neighbors) return;
    
    push(stack, y, x);
        
    /* Scan all nearby tiles. If we meet a tile with a number we just unveil
       it. If we meet an empty tile, we push the location in stack. For each
       location in stack we do the same thing. (scan again all nearby tiles) */
    while(stack_pos){
        /* Retrieve x, y from stack. */
        x = stack[stack_pos];
        y = stack[stack_pos-1];
        
        /* Pop. */
        if(stack_pos > 0) stack_pos -= 2;
        else rb->splash(HZ,true,"ERROR");
            
        unveil(stack, y-1, x-1);
        unveil(stack, y-1, x);
        unveil(stack, y-1, x+1);
        unveil(stack, y, x+1);
        unveil(stack, y+1, x+1);
        unveil(stack, y+1, x);
        unveil(stack, y+1, x-1);
        unveil(stack, y, x-1);
    }    
}

/* Reset the whole board for a new game. */
void minesweeper_init(void){
    int i,j;

    for(i=0;i<LCD_HEIGHT/8;i++){
        for(j=0;j<LCD_WIDTH/8;j++){
            minefield[i][j].known = 0;
            minefield[i][j].flag = 0;
            minefield[i][j].mine = 0;
            minefield[i][j].neighbors = 0;
        }
    }
    no_mines = true;
    tiles_left = width*height;
}


/* put mines on the mine field */
/* there is p% chance that a tile is a mine */
/* if the tile has coordinates (x,y), then it can't be a mine */
void minesweeper_putmines(int p, int x, int y){
    int i,j;

    mine_num = 0;
    for(i=c_height();i<c_height() + height;i++){
        for(j=c_width();j<c_width() + width;j++){
            if(rb->rand()%100<p && !(y==i && x==j)){
                minefield[i][j].mine = 1;
                mine_num++;
            } else {
                minefield[i][j].mine = 0;
            }
            minefield[i][j].neighbors = 0;
        }
    }
            
    /* we need to compute the neighbor element for each tile */
    for(i=c_height();i<c_height() + height;i++){
        for(j=c_width();j<c_width() + width;j++){
            if(i>0){
                if(j>0)
                    minefield[i][j].neighbors += minefield[i-1][j-1].mine;
                minefield[i][j].neighbors += minefield[i-1][j].mine;
                if(j<c_width() + width-1)
                    minefield[i][j].neighbors += minefield[i-1][j+1].mine;
            }
            if(j>0)
                minefield[i][j].neighbors += minefield[i][j-1].mine;
            if(j<c_width() + width-1)
                minefield[i][j].neighbors += minefield[i][j+1].mine;
            if(i<c_height() + height-1){
                if(j>0)
                    minefield[i][j].neighbors += minefield[i+1][j-1].mine;
                minefield[i][j].neighbors += minefield[i+1][j].mine;
                if(j<c_width() + width-1)
                    minefield[i][j].neighbors += minefield[i+1][j+1].mine;
            }
        }
    }
    
    no_mines = false;
    /* In case the user is lucky and there are no mines positioned. */
    if(!mine_num && height*width != 1) minesweeper_putmines(p, x, y);
}

/* A function that will uncover all the board, when the user wins or loses.
   can easily be expanded, (just a call assigned to a button) as a solver. */
void mine_show(void){
    int i, j, button;
            
    for(i=c_height();i<c_height() + height;i++){
        for(j=c_width();j<c_width() + width;j++){
#if LCD_DEPTH > 1
            rb->lcd_set_foreground(LCD_DARKGRAY);
            rb->lcd_drawrect(j*8,i*8,8,8);
            rb->lcd_set_foreground(LCD_BLACK);
#else
            rb->lcd_drawrect(j*8,i*8,8,8);
#endif
            if(!minefield[i][j].known){
                if(minefield[i][j].mine){
                    rb->lcd_set_drawmode(DRMODE_FG);
                    rb->lcd_mono_bitmap(num[9], j*8,i*8,8,8);
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                } else if(minefield[i][j].neighbors){
                    rb->lcd_set_drawmode(DRMODE_FG);
                    rb->lcd_mono_bitmap(num[minefield[i][j].neighbors],
                                                        j*8,i*8,8,8);
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                }
            }
        }
    }
    rb->lcd_update();
    
    bool k = true;
    while(k){
        button = rb->button_get_w_tmo(HZ/10);
        if(button !=  BUTTON_NONE && !(button & BUTTON_REL)) k = false;
    }
}


/* the big and ugly function that is the game */
int minesweeper(void)
{
    int i,j;
    int button;
    int lastbutton = BUTTON_NONE;
    
    /* the cursor coordinates */
    int x=0, y=0;

    /* a usefull string for snprintf */
    char str[30];

    /* welcome screen where player can chose mine percentage */
    i = 0;
    while(true){
        rb->lcd_clear_display();

        rb->lcd_puts(0,0,"Mine Sweeper");

        rb->snprintf(str, 20, "%d%% mines", p);
        rb->lcd_puts(0,2,str);
        rb->lcd_puts(0,3,"down / up");
        rb->snprintf(str, 20, "%d cols x %d rows", width, height);
        rb->lcd_puts(0,4,str);
        rb->lcd_puts(0,5,"left x right ");
#if CONFIG_KEYPAD == RECORDER_PAD
        rb->lcd_puts(0,6,"ON to start");
#elif CONFIG_KEYPAD == ONDIO_PAD
        rb->lcd_puts(0,6,"MODE to start");
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
        rb->lcd_puts(0,6,"SELECT to start");
#endif
        rb->lcd_update();

        button = rb->button_get(true);
        switch(button){
            case MINESWP_DOWN:
            case (MINESWP_DOWN | BUTTON_REPEAT):
                p = (p + 98)%100;
                /* Don't let the user play without mines. */
                if(!p) p = 98;
                break;

            case MINESWP_UP:
            case (MINESWP_UP | BUTTON_REPEAT):
                p = (p + 2)%100;
                /* Don't let the user play without mines. */
                if(!p) p = 2;
                break;

            case BUTTON_RIGHT:
            case (BUTTON_RIGHT | BUTTON_REPEAT):
                height = height%(LCD_HEIGHT/8)+1;
                break;

            case BUTTON_LEFT:
            case (BUTTON_LEFT | BUTTON_REPEAT):
                width = width%(LCD_WIDTH/8)+1;
                break;
                
            case MINESWP_RIGHT:
            case (MINESWP_RIGHT | BUTTON_REPEAT):
                height--;
                if(height < 1) height = LCD_HEIGHT/8;
                if(height > LCD_HEIGHT) height = 1;
                break;

            case MINESWP_LEFT:
            case (MINESWP_LEFT | BUTTON_REPEAT):
                width--;
                if(width < 1) width = LCD_WIDTH/8;
                if(width > LCD_WIDTH) width = 1;
                break;

            case MINESWP_START:/* start playing */
                i = 1;
                break;

            case MINESWP_QUIT:/* quit program */
                return MINESWEEPER_QUIT;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return MINESWEEPER_USB;
                break;
        }
        if(i==1)
            break;
    }


    /********************
    *       init        *
    ********************/

    minesweeper_init();
    x = c_width();
    y = c_height();

    /**********************
    *        play         *
    **********************/

    while(true){

        /*clear the screen buffer */
        rb->lcd_clear_display();

        /*display the mine field */
        for(i=c_height();i<c_height() + height;i++){
            for(j=c_width();j<c_width() + width;j++){
#if LCD_DEPTH > 1
                rb->lcd_set_foreground(LCD_DARKGRAY);
                rb->lcd_drawrect(j*8,i*8,8,8);
                rb->lcd_set_foreground(LCD_BLACK);
#else
                rb->lcd_drawrect(j*8,i*8,8,8);
#endif
                if(minefield[i][j].known){
                    if(minefield[i][j].neighbors){
                        rb->lcd_set_drawmode(DRMODE_FG);
                        rb->lcd_mono_bitmap(num[minefield[i][j].neighbors],
                                                              j*8,i*8,8,8);
                        rb->lcd_set_drawmode(DRMODE_SOLID);
                    }
                } else if(minefield[i][j].flag) {
                    rb->lcd_drawline(j*8+2,i*8+2,j*8+5,i*8+5);
                    rb->lcd_drawline(j*8+2,i*8+5,j*8+5,i*8+2);
                } else {
#if LCD_DEPTH > 1
                    rb->lcd_set_foreground(LCD_LIGHTGRAY);
                    rb->lcd_fillrect(j*8+1,i*8+1,6,6);
                    rb->lcd_set_foreground(LCD_BLACK);
#else
                    rb->lcd_fillrect(j*8+2,i*8+2,4,4);
#endif
                }
            }
        }

        /* display the cursor */
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_fillrect(x*8,y*8,8,8);
        rb->lcd_set_drawmode(DRMODE_SOLID);

        /* update the screen */
        rb->lcd_update();

        button = rb->button_get(true);
        switch(button){
            /* quit minesweeper (you really shouldn't use this button ...) */
            case MINESWP_QUIT:
                return MINESWEEPER_QUIT;
                                                
                /* move cursor left */
            case BUTTON_LEFT:
            case (BUTTON_LEFT | BUTTON_REPEAT):
                if(x<=c_width()) x = width + c_width();
                x = x-1;
                break;

                /* move cursor right */
            case BUTTON_RIGHT:
            case (BUTTON_RIGHT | BUTTON_REPEAT):
                if(x>=width + c_width() - 1) x = c_width() - 1;
                x = x+1;
                break;

                /* move cursor down */
            case MINESWP_DOWN:
            case (MINESWP_DOWN | BUTTON_REPEAT):
                if(y>=height + c_height() - 1) y = c_height() - 1;
                y = y+1;
                break;

                /* move cursor up */
            case MINESWP_UP:
            case (MINESWP_UP | BUTTON_REPEAT):
                if(y<=c_height()) y = height + c_height();
                y = y-1;
                break;

                /* discover a tile (and it's neighbors if .neighbors == 0) */
            case MINESWP_DISCOVER:
#ifdef MINESWP_DISCOVER2
            case MINESWP_DISCOVER2:
#endif
                if(minefield[y][x].flag) break;
                /* we put the mines on the first "click" so that you don't */
                /* lose on the first "click" */
                if(tiles_left == width*height && no_mines)
                    minesweeper_putmines(p,x,y);
                                
                discover(y, x);
                
                if(minefield[y][x].mine){
                    return MINESWEEPER_LOSE;
                }
                tiles_left = 0;
                for(i=c_height();i<c_height() + height;i++){
                    for(j=c_width();j<c_width() + width;j++){
                         if(minefield[i][j].known == 0) tiles_left++;
                    }
                }
                if(tiles_left == mine_num){
                    return MINESWEEPER_WIN;
                }
                break;

                /* toggle flag under cursor */
            case MINESWP_TOGGLE:
#ifdef MINESWP_TOGGLE_PRE
                if (lastbutton != MINESWP_TOGGLE_PRE)
                    break;
#endif
#ifdef MINESWP_TOGGLE2
            case MINESWP_TOGGLE2:
#endif
                minefield[y][x].flag = (minefield[y][x].flag + 1)%2;
                break;

                /* show how many mines you think you have found and how many */
                /* there really are on the game */
            case MINESWP_INFO:
                if(no_mines) break;
                tiles_left = 0;
                for(i=c_height();i<c_height() + height;i++){
                    for(j=c_width();j<c_width() + width;j++){
                         if(minefield[i][j].flag && !minefield[i][j].known)
                            tiles_left++;
                    }
                }
                rb->splash(HZ*2, true, "You found %d mines out of %d",
                                                 tiles_left, mine_num);
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return MINESWEEPER_USB;
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }

}

/* plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    bool exit = false;
    /* plugin init */
    (void)parameter;
    rb = api;
    /* end of plugin init */

    while(!exit) {
        switch(minesweeper()){
            case MINESWEEPER_WIN:
                rb->splash(HZ*2, true, "You Won! Press a key");
                rb->lcd_clear_display();
                mine_show();
                break;

            case MINESWEEPER_LOSE:
                rb->splash(HZ*2, true, "You Lost! Press a key");
                rb->lcd_clear_display();
                mine_show();
                break;

            case MINESWEEPER_USB:
                return PLUGIN_USB_CONNECTED;

            case MINESWEEPER_QUIT:
                exit = true;
                break;

            default:
                break;
        }
    }

    return PLUGIN_OK;
}

#endif
