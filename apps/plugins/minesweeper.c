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

use arrow keys to move cursor
use ON or F2 to clear a tile
use PLAY or F1 to put a flag on a tile
use F3 to see how many mines are left (supposing all your flags are correct)

*****************************************************************************/

#include "plugin.h"
#include "button.h"
#include "lcd.h"

#ifdef HAVE_LCD_BITMAP

//what the minesweeper() function can return
#define MINESWEEPER_USB  3
#define MINESWEEPER_QUIT 2
#define MINESWEEPER_LOSE 1
#define MINESWEEPER_WIN  0

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define MINESWP_QUIT BUTTON_OFF
#define MINESWP_START BUTTON_ON
#define MINESWP_TOGGLE BUTTON_PLAY
#define MINESWP_TOGGLE2 BUTTON_F1
#define MINESWP_DISCOVER BUTTON_ON
#define MINESWP_DISCOVER2 BUTTON_F2
#define MINESWP_INFO BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MINESWP_QUIT BUTTON_OFF
#define MINESWP_START BUTTON_MENU
#define MINESWP_TOGGLE_PRE BUTTON_MENU
#define MINESWP_TOGGLE (BUTTON_MENU | BUTTON_REL)
#define MINESWP_DISCOVER (BUTTON_MENU | BUTTON_REPEAT)
#define MINESWP_INFO (BUTTON_MENU | BUTTON_OFF)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MINESWP_QUIT BUTTON_OFF
#define MINESWP_START BUTTON_SELECT
#define MINESWP_TOGGLE BUTTON_SELECT
#define MINESWP_DISCOVER BUTTON_ON
#define MINESWP_INFO BUTTON_MODE

#endif

#if LCD_DEPTH > 1
#if HAVE_LCD_COLOR
#define LIGHT_GRAY ((struct rgb){2*LCD_MAX_RED/3, 2*LCD_MAX_GREEN/3, 2*LCD_MAX_BLUE/3})
#define DARK_GRAY  ((struct rgb){LCD_MAX_RED/3, LCD_MAX_GREEN/3, LCD_MAX_BLUE/3})
#else
#define LIGHT_GRAY (2*LCD_MAX_LEVEL/3)
#define DARK_GRAY  (LCD_MAX_LEVEL/3)
#endif
#endif

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;


/* define how numbers are displayed (that way we don't have to */
/* worry about fonts) */
static unsigned char num[9][8] = {
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

/* the minefield */
tile minefield[LCD_HEIGHT/8][LCD_WIDTH/8];

/* total number of mines on the game */
int mine_num = 0;

/* discovers the tile when player clears one of them */
/* a chain reaction (of discovery) occurs if tile has no mines */
/* as neighbors */
void discover(int, int);
void discover(int x, int y){

    if(x<0) return;
    if(y<0) return;
    if(x>width-1) return;
    if(y>height-1) return;
    if(minefield[y][x].known) return;

    minefield[y][x].known = 1;
    if(minefield[y][x].neighbors == 0){
        discover(x-1,y-1);
        discover(x,y-1);
        discover(x+1,y-1);
        discover(x+1,y);
        discover(x+1,y+1);
        discover(x,y+1);
        discover(x-1,y+1);
        discover(x-1,y);
    }
    return;
}


/* init not mine related elements of the mine field */
void minesweeper_init(void){
    int i,j;

    for(i=0;i<height;i++){
        for(j=0;j<width;j++){
            minefield[i][j].known = 0;
            minefield[i][j].flag = 0;
        }
    }
}


/* put mines on the mine field */
/* there is p% chance that a tile is a mine */
/* if the tile has coordinates (x,y), then it can't be a mine */
void minesweeper_putmines(int p, int x, int y){
    int i,j;

    mine_num = 0;
    for(i=0;i<height;i++){
        for(j=0;j<width;j++){
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
    for(i=0;i<height;i++){
        for(j=0;j<width;j++){
            if(i>0){
                if(j>0)
                    minefield[i][j].neighbors += minefield[i-1][j-1].mine;
                minefield[i][j].neighbors += minefield[i-1][j].mine;
                if(j<width-1)
                    minefield[i][j].neighbors += minefield[i-1][j+1].mine;
            }
            if(j>0)
                minefield[i][j].neighbors += minefield[i][j-1].mine;
            if(j<width-1)
                minefield[i][j].neighbors += minefield[i][j+1].mine;
            if(i<height-1){
                if(j>0)
                    minefield[i][j].neighbors += minefield[i+1][j-1].mine;
                minefield[i][j].neighbors += minefield[i+1][j].mine;
                if(j<width-1)
                    minefield[i][j].neighbors += minefield[i+1][j+1].mine;
            }
        }
    }
}

/* the big and ugly function that is the game */
int minesweeper(void)
{
    int i,j;
    int button;
    int lastbutton = BUTTON_NONE;

    /* the cursor coordinates */
    int x=0,y=0;

    /* number of tiles left on the game */
    int tiles_left=width*height;

    /* percentage of mines on minefield used durring generation */
    int p=16;

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
            case BUTTON_DOWN:
                p = (p + 98)%100;
                break;

            case BUTTON_UP:
                p = (p + 2)%100;
                break;

            case BUTTON_RIGHT:
                height = height%(LCD_HEIGHT/8)+1;
                break;

            case BUTTON_LEFT:
                width = width%(LCD_WIDTH/8)+1;
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

    /**********************
    *        play         *
    **********************/

    while(true){

        //clear the screen buffer
        rb->lcd_clear_display();

        //display the mine field
        for(i=0;i<height;i++){
            for(j=0;j<width;j++){
#if LCD_DEPTH > 1
                rb->lcd_set_foreground(DARK_GRAY);
                rb->lcd_drawrect(j*8,i*8,8,8);
                rb->lcd_set_foreground(LCD_BLACK);
#else
                rb->lcd_drawrect(j*8,i*8,8,8);
#endif
                if(minefield[i][j].known){
                    if(minefield[i][j].mine){
                        rb->lcd_putsxy(j*8+1,i*8+1,"b");
                    } else if(minefield[i][j].neighbors){
                        rb->lcd_set_drawmode(DRMODE_FG);
                        rb->lcd_mono_bitmap(num[minefield[i][j].neighbors],j*8,i*8,8,8);
                        rb->lcd_set_drawmode(DRMODE_SOLID);
                    }
                } else if(minefield[i][j].flag) {
                    rb->lcd_drawline(j*8+2,i*8+2,j*8+5,i*8+5);
                    rb->lcd_drawline(j*8+2,i*8+5,j*8+5,i*8+2);
                } else {
#if LCD_DEPTH > 1
                    rb->lcd_set_foreground(LIGHT_GRAY);
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
                x = (x + width - 1)%width;
                break;

                /* move cursor right */
            case BUTTON_RIGHT:
            case (BUTTON_RIGHT | BUTTON_REPEAT):
                x = (x + 1)%width;
                break;

                /* move cursor down */
            case BUTTON_DOWN:
            case (BUTTON_DOWN | BUTTON_REPEAT):
                y = (y + 1)%height;
                break;

                /* move cursor up */
            case BUTTON_UP:
            case (BUTTON_UP | BUTTON_REPEAT):
                y = (y + height - 1)%height;
                break;

                /* discover a tile (and it's neighbors if .neighbors == 0) */
            case MINESWP_DISCOVER:
#ifdef MINESWP_DISCOVER2
            case MINESWP_DISCOVER2:
#endif
                if(minefield[y][x].flag) break;
                /* we put the mines on the first "click" so that you don't */
                /* lose on the first "click" */
                if(tiles_left == width*height) minesweeper_putmines(p,x,y);
                discover(x,y);
                if(minefield[y][x].mine){
                    return MINESWEEPER_LOSE;
                }
                tiles_left = 0;
                for(i=0;i<height;i++){
                    for(j=0;j<width;j++){
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
                tiles_left = 0;
                for(i=0;i<height;i++){
                    for(j=0;j<width;j++){
                         if(minefield[i][j].flag) tiles_left++;
                    }
                }
                rb->splash(HZ*2, true, "You found %d mines out of %d", tiles_left, mine_num);
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
    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;
    /* end of plugin init */

    while(!exit) {
        switch(minesweeper()){
            case MINESWEEPER_WIN:
                rb->splash(HZ*2, true, "You Win :)");
                break;

            case MINESWEEPER_LOSE:
                rb->splash(HZ*2, true, "You Lost :(");
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
