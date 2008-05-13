/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004-2006 Antoine Cellerier <dionoea -at- videolan -dot- org>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* what the minesweeper() function can return */
enum minesweeper_status {
    MINESWEEPER_WIN,
    MINESWEEPER_LOSE,
    MINESWEEPER_QUIT,
    MINESWEEPER_USB
};

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_OFF
#   define MINESWP_TOGGLE   BUTTON_ON
#   define MINESWP_TOGGLE2  BUTTON_F1
#   define MINESWP_DISCOVER BUTTON_PLAY
#   define MINESWP_DISCOVER2 BUTTON_F2
#   define MINESWP_INFO     BUTTON_F3

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_OFF
#   define MINESWP_TOGGLE   BUTTON_ON
#   define MINESWP_TOGGLE2  BUTTON_F1
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_DISCOVER2 BUTTON_F2
#   define MINESWP_INFO     BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_OFF
#   define MINESWP_TOGGLE_PRE BUTTON_MENU
#   define MINESWP_TOGGLE   (BUTTON_MENU | BUTTON_REL)
#   define MINESWP_DISCOVER (BUTTON_MENU | BUTTON_REPEAT)
#   define MINESWP_INFO     (BUTTON_MENU | BUTTON_OFF)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_OFF
#   define MINESWP_TOGGLE   BUTTON_ON
#   define MINESWP_TOGGLE2  BUTTON_REC
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_INFO     BUTTON_MODE

#   define MINESWP_RC_QUIT  BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   define MINESWP_SCROLLWHEEL
#   define MINESWP_LEFT       BUTTON_LEFT
#   define MINESWP_RIGHT      BUTTON_RIGHT
#   define MINESWP_UP         BUTTON_MENU
#   define MINESWP_DOWN       BUTTON_PLAY
#   define MINESWP_NEXT       BUTTON_SCROLL_FWD
#   define MINESWP_PREV       BUTTON_SCROLL_BACK
#   define MINESWP_QUIT       (BUTTON_SELECT | BUTTON_MENU)
#   define MINESWP_TOGGLE_PRE BUTTON_SELECT
#   define MINESWP_TOGGLE     (BUTTON_SELECT | BUTTON_REL)
#   define MINESWP_DISCOVER   (BUTTON_SELECT | BUTTON_REPEAT)
#   define MINESWP_INFO       (BUTTON_SELECT | BUTTON_PLAY)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_POWER
#   define MINESWP_TOGGLE   BUTTON_PLAY
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_INFO     BUTTON_REC

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_POWER
#   define MINESWP_TOGGLE   BUTTON_A
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_INFO     BUTTON_MENU

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#   define MINESWP_SCROLLWHEEL
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_POWER
#   define MINESWP_NEXT     BUTTON_SCROLL_FWD
#   define MINESWP_PREV     BUTTON_SCROLL_BACK
#   define MINESWP_TOGGLE   BUTTON_REC
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_INFO     (BUTTON_REC|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#   define MINESWP_LEFT       BUTTON_LEFT
#   define MINESWP_RIGHT      BUTTON_RIGHT
#   define MINESWP_UP         BUTTON_UP
#   define MINESWP_DOWN       BUTTON_DOWN
#   define MINESWP_QUIT       BUTTON_POWER
#   define MINESWP_TOGGLE_PRE BUTTON_SELECT
#   define MINESWP_TOGGLE     (BUTTON_SELECT | BUTTON_REL)
#   define MINESWP_TOGGLE2    BUTTON_VOL_DOWN
#   define MINESWP_DISCOVER   (BUTTON_SELECT | BUTTON_REPEAT)
#   define MINESWP_DISCOVER2  BUTTON_VOL_UP
#   define MINESWP_INFO       BUTTON_REC

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_SCROLL_UP
#   define MINESWP_DOWN     BUTTON_SCROLL_DOWN
#   define MINESWP_QUIT     BUTTON_POWER
#   define MINESWP_TOGGLE   BUTTON_PLAY
#   define MINESWP_DISCOVER BUTTON_REW
#   define MINESWP_INFO     (BUTTON_REW | BUTTON_PLAY)

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_BACK
#   define MINESWP_TOGGLE   BUTTON_PLAY
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_INFO     BUTTON_MENU

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define MINESWP_LEFT     BUTTON_LEFT
#   define MINESWP_RIGHT    BUTTON_RIGHT
#   define MINESWP_UP       BUTTON_UP
#   define MINESWP_DOWN     BUTTON_DOWN
#   define MINESWP_QUIT     BUTTON_POWER
#   define MINESWP_TOGGLE   BUTTON_DISPLAY
#   define MINESWP_DISCOVER BUTTON_SELECT
#   define MINESWP_INFO     BUTTON_MENU

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#   define MINESWP_LEFT     BUTTON_RC_REW
#   define MINESWP_RIGHT    BUTTON_RC_FF
#   define MINESWP_UP       BUTTON_RC_VOL_UP
#   define MINESWP_DOWN     BUTTON_RC_VOL_DOWN
#   define MINESWP_QUIT     BUTTON_RC_REC
#   define MINESWP_TOGGLE   BUTTON_RC_MODE
#   define MINESWP_DISCOVER BUTTON_RC_PLAY
#   define MINESWP_INFO     BUTTON_RC_MENU

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#   define MINESWP_QUIT     BUTTON_POWER

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHPAD
#ifndef MINESWP_QUIT
#   define MINESWP_QUIT     BUTTON_TOPLEFT
#endif
#ifndef MINESWP_LEFT
#   define MINESWP_LEFT     BUTTON_MIDLEFT
#endif
#ifndef MINESWP_RIGHT
#   define MINESWP_RIGHT    BUTTON_MIDRIGHT
#endif
#ifndef MINESWP_UP
#   define MINESWP_UP       BUTTON_TOPMIDDLE
#endif
#ifndef MINESWP_DOWN
#   define MINESWP_DOWN     BUTTON_BOTTOMMIDDLE
#endif
#ifndef MINESWP_TOGGLE
#   define MINESWP_TOGGLE   BUTTON_CENTER
#endif
#ifndef MINESWP_DISCOVER
#   define MINESWP_DISCOVER BUTTON_BOTTOMLEFT
#endif
#ifndef MINESWP_INFO
#   define MINESWP_INFO     BUTTON_BOTTOMRIGHT
#endif
#endif

/* here is a global api struct pointer. while not strictly necessary,
 * it's nice not to have to pass the api pointer in all function calls
 * in the plugin
 */
static const struct plugin_api *rb;

extern const fb_data minesweeper_tiles[];

#ifdef HAVE_LCD_COLOR
#   if ( LCD_HEIGHT * LCD_WIDTH ) / ( 16 * 16 ) >= 130
        /* We want to have at least 130 tiles on the screen */
#       define TileSize 16
#   elif ( LCD_HEIGHT * LCD_WIDTH ) / ( 12 * 12 ) >= 130
#       define TileSize 12
#   else
#       define TileSize 10
#   endif
#   define BackgroundColor LCD_RGBPACK( 128, 128, 128 )
#elif LCD_DEPTH > 1
#   define TileSize 12
#else
#   define TileSize 8
#endif

#define Mine         9
#define Flag         10
#define Unknown      11
#define ExplodedMine 12

#define draw_tile( num, x, y ) \
    rb->lcd_bitmap_part( minesweeper_tiles, 0, num * TileSize, \
                         TileSize, left+x*TileSize, top+y*TileSize, \
                         TileSize, TileSize )

#define invert_tile( x, y ) \
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT); \
    rb->lcd_fillrect( left+x*TileSize, top+y*TileSize, TileSize, TileSize ); \
    rb->lcd_set_drawmode(DRMODE_SOLID);


/* the tile struct
 * if there is a mine, mine is true
 * if tile is known by player, known is true
 * if tile has a flag, flag is true
 * neighbors is the total number of mines arround tile
 */
typedef struct tile
{
    unsigned char mine : 1;
    unsigned char known : 1;
    unsigned char flag : 1;
    unsigned char neighbors : 4;
} tile;

/* the height and width of the field */
#define MAX_HEIGHT (LCD_HEIGHT/TileSize)
#define MAX_WIDTH  (LCD_WIDTH/TileSize)
int height = MAX_HEIGHT;
int width = MAX_WIDTH;
int top;
int left;

/* The Minefield. Caution it is defined as Y, X! Not the opposite. */
tile minefield[MAX_HEIGHT][MAX_WIDTH];

/* total number of mines on the game */
int mine_num = 0;

/* percentage of mines on minefield used during generation */
int p = 16;

/* number of tiles left on the game */
int tiles_left;

/* number of used flags on the game */
int flags_used;

/* Because mines are set after the first move... */
bool no_mines = true;

/* We need a stack (created on discover()) for the cascade algorithm. */
int stack_pos = 0;

/* a usefull string for snprintf */
char str[30];


void push( int *stack, int y, int x )
{
    if( stack_pos <= height*width )
    {
        stack[++stack_pos] = y;
        stack[++stack_pos] = x;
    }
}

/* Unveil tiles and push them to stack if they are empty. */
void unveil( int *stack, int y, int x )
{
    if( x < 0 || y < 0 || x > width - 1 || y > height - 1
       || minefield[y][x].known
       || minefield[y][x].mine || minefield[y][x].flag ) return;

    minefield[y][x].known = 1;

    if( minefield[y][x].neighbors == 0 )
        push( stack, y, x );
}

void discover( int y, int x )
{
    int stack[height*width];

    /* Selected tile. */
    if( x < 0 || y < 0 || x > width - 1 || y > height - 1
       || minefield[y][x].known
       || minefield[y][x].mine || minefield[y][x].flag ) return;

    minefield[y][x].known = 1;
    /* Exit if the tile is not empty. (no mines nearby) */
    if( minefield[y][x].neighbors ) return;

    push( stack, y, x );

    /* Scan all nearby tiles. If we meet a tile with a number we just unveil
     * it. If we meet an empty tile, we push the location in stack. For each
     * location in stack we do the same thing. (scan again all nearby tiles)
     */
    while( stack_pos )
    {
        /* Pop x, y from stack. */
        x = stack[stack_pos--];
        y = stack[stack_pos--];

        unveil( stack, y-1, x-1 );
        unveil( stack, y-1, x   );
        unveil( stack, y-1, x+1 );
        unveil( stack, y,   x+1 );
        unveil( stack, y+1, x+1 );
        unveil( stack, y+1, x   );
        unveil( stack, y+1, x-1 );
        unveil( stack, y,   x-1 );
    }
}

/* Reset the whole board for a new game. */
void minesweeper_init( void )
{
    int i,j;

    for( i = 0; i < MAX_HEIGHT; i++ )
    {
        for( j = 0; j < MAX_WIDTH; j++ )
        {
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
void minesweeper_putmines( int p, int x, int y )
{
    int i,j;

    mine_num = 0;
    for( i = 0; i < height; i++ )
    {
        for( j = 0; j < width; j++ )
        {
            if( rb->rand()%100 < p && !( y==i && x==j ) )
            {
                minefield[i][j].mine = 1;
                mine_num++;
            }
            else
            {
                minefield[i][j].mine = 0;
            }
            minefield[i][j].neighbors = 0;
        }
    }

    /* we need to compute the neighbor element for each tile */
    for( i = 0; i < height; i++ )
    {
        for( j = 0; j < width; j++ )
        {
            if( i > 0 )
            {
                if( j > 0 )
                    minefield[i][j].neighbors += minefield[i-1][j-1].mine;
                minefield[i][j].neighbors += minefield[i-1][j].mine;
                if( j < width - 1 )
                    minefield[i][j].neighbors += minefield[i-1][j+1].mine;
            }
            if( j > 0 )
                minefield[i][j].neighbors += minefield[i][j-1].mine;
            if( j < width - 1 )
                minefield[i][j].neighbors += minefield[i][j+1].mine;
            if( i < height - 1 )
            {
                if( j > 0 )
                    minefield[i][j].neighbors += minefield[i+1][j-1].mine;
                minefield[i][j].neighbors += minefield[i+1][j].mine;
                if( j < width - 1 )
                    minefield[i][j].neighbors += minefield[i+1][j+1].mine;
            }
        }
    }

    no_mines = false;

    /* In case the user is lucky and there are no mines positioned. */
    if( !mine_num && height*width != 1 )
    {
        minesweeper_putmines(p, x, y);
    }
}

/* A function that will uncover all the board, when the user wins or loses.
   can easily be expanded, (just a call assigned to a button) as a solver. */
void mine_show( void )
{
    int i, j, button;

    for( i = 0; i < height; i++ )
    {
        for( j = 0; j < width; j++ )
        {
            if( minefield[i][j].mine )
            {
                if( minefield[i][j].known )
                {
                    draw_tile( ExplodedMine, j, i );
                }
                else
                {
                    draw_tile( Mine, j, i );
                }
            }
            else
            {
                draw_tile( minefield[i][j].neighbors, j, i );
            }
        }
    }
    rb->lcd_update();

    do
        button = rb->button_get(true);
    while( ( button == BUTTON_NONE )
           || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
}

int count_tiles_left( void )
{
    int tiles_left = 0;
    int i, j;
    for( i = 0; i < height; i++ )
        for( j = 0; j < width; j++ )
             if( minefield[i][j].known == 0 )
                tiles_left++;
    return tiles_left;
}

int count_flags( void )
{
    int flags_used = 0;
    int i, j;
    for( i = 0; i < height; i++ )
        for( j = 0; j < width; j++ )
             if( minefield[i][j].flag == 1 )
                flags_used++;
    return flags_used;
}

/* welcome screen where player can chose mine percentage */
enum minesweeper_status menu( void )
{
    int selection, result = MINESWEEPER_QUIT;
    bool menu_quit = false;

    MENUITEM_STRINGLIST( menu, "Minesweeper Menu", NULL, "Play Minesweeper",
                         "Mine Percentage", "Number of Rows",
                         "Number of Columns", "Quit" );

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground( rb->global_settings->fg_color );
    rb->lcd_set_background( rb->global_settings->bg_color );
#endif

    while( !menu_quit )
    {
        switch( rb->do_menu( &menu, &selection, NULL, false ) )
        {
            case 0:
                result = MINESWEEPER_WIN; /* start playing */
                menu_quit = true;
                break;

            case 1:
                rb->set_int( "Mine Percentage", "%", UNIT_INT, &p, NULL,
                             1, 2, 98, NULL );
                break;

            case 2:
                rb->set_int( "Number of Rows", "", UNIT_INT, &height, NULL,
                             1, 1, MAX_HEIGHT, NULL );
                break;

            case 3:
                rb->set_int( "Number of Columns", "", UNIT_INT, &width, NULL,
                             1, 1, MAX_WIDTH, NULL );
                break;

            default:
                result = MINESWEEPER_QUIT; /* quit program */
                menu_quit = true;
                break;
        }
    }

    return result;
}

/* the big and ugly game function */
enum minesweeper_status minesweeper( void )
{
    int i, j;
    int button;
    int lastbutton = BUTTON_NONE;

    /* the cursor coordinates */
    int x=0, y=0;

    /**
     * Show the menu
     */
    if( ( i = menu() ) != MINESWEEPER_WIN ) return i;

    /**
     * Init game
     */
    top = (LCD_HEIGHT-height*TileSize)/2;
    left = (LCD_WIDTH-width*TileSize)/2;

    rb->srand( *rb->current_tick );
    minesweeper_init();
    x = 0;
    y = 0;

    /**
     * Play
     */
    while( true )
    {

        /* clear the screen buffer */
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_background( BackgroundColor );
#endif
        rb->lcd_clear_display();

        /* display the mine field */
        for( i = 0; i < height; i++ )
        {
            for( j = 0; j < width; j++ )
            {
                if( minefield[i][j].known )
                {
                    draw_tile( minefield[i][j].neighbors, j, i );
                }
                else if(minefield[i][j].flag)
                {
                    draw_tile( Flag, j, i );
                }
                else
                {
                    draw_tile( Unknown, j, i );
                }
            }
        }

        /* display the cursor */
        invert_tile( x, y );

        /* update the screen */
        rb->lcd_update();

        switch( button = rb->button_get( true ) )
        {
            /* quit minesweeper (you really shouldn't use this button ...) */
#ifdef MINESWP_RC_QUIT
            case MINESWP_RC_QUIT:
#endif
            case MINESWP_QUIT:
                return MINESWEEPER_QUIT;

            /* move cursor left */
            case MINESWP_LEFT:
            case MINESWP_LEFT|BUTTON_REPEAT:
                x = ( x + width - 1 )%width;
                break;

            /* move cursor right */
            case MINESWP_RIGHT:
            case MINESWP_RIGHT|BUTTON_REPEAT:
                x = ( x + 1 )%width;
                break;

            /* move cursor down */
            case MINESWP_DOWN:
            case MINESWP_DOWN|BUTTON_REPEAT:
                y = ( y + 1 )%height;
                break;

            /* move cursor up */
            case MINESWP_UP:
            case MINESWP_UP|BUTTON_REPEAT:
                y = ( y + height - 1 )%height;
                break;

            /*move cursor though the entire field*/
#ifdef MINESWP_SCROLLWHEEL
            case MINESWP_NEXT:
            case MINESWP_NEXT|BUTTON_REPEAT:
                if (x == width -1 ) {
                    y = ( y + 1 )%height;
                }
                x = ( x + 1 )%width;
                break;

            case MINESWP_PREV:
            case MINESWP_PREV|BUTTON_REPEAT:
                if (x == 0) {
                    y = ( y + height - 1 )%height;
                }
                x = ( x + width - 1 )%width;
                break;
#endif
            /* discover a tile (and it's neighbors if .neighbors == 0) */
            case MINESWP_DISCOVER:
#ifdef MINESWP_DISCOVER2
            case MINESWP_DISCOVER2:
#endif
                if( minefield[y][x].flag ) break;
                /* we put the mines on the first "click" so that you don't
                 * lose on the first "click" */
                if( tiles_left == width*height && no_mines )
                    minesweeper_putmines(p,x,y);

                discover(y, x);

                if( minefield[y][x].mine )
                {
                    minefield[y][x].known = 1;
                    return MINESWEEPER_LOSE;
                }
                tiles_left = count_tiles_left();
                if( tiles_left == mine_num )
                {
                    return MINESWEEPER_WIN;
                }
                break;

            /* toggle flag under cursor */
            case MINESWP_TOGGLE:
#ifdef MINESWP_TOGGLE_PRE
                if( lastbutton != MINESWP_TOGGLE_PRE )
                    break;
#endif
#ifdef MINESWP_TOGGLE2
            case MINESWP_TOGGLE2:
#endif
                minefield[y][x].flag = ( minefield[y][x].flag + 1 )%2;
                break;

            /* show how many mines you think you have found and how many
             * there really are on the game */
            case MINESWP_INFO:
                if( no_mines )
                    break;
                flags_used = count_flags();
                if (flags_used == 1) {
                    rb->splash( HZ*2, "You marked 1 field. There are %d mines.",
                                mine_num );
                }
                else
                {
                    rb->splash( HZ*2, "You marked %d fields. There are %d mines.",
                                flags_used, mine_num );
                }
                break;

            default:
                if( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
                    return MINESWEEPER_USB;
                break;
        }
        if( button != BUTTON_NONE )
            lastbutton = button;
    }

}

/* plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    bool exit = false;

    (void)parameter;
    rb = api;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    while( !exit )
    {
        switch( minesweeper() )
        {
            case MINESWEEPER_WIN:
                rb->splash( HZ, "You Win!" );
                rb->lcd_clear_display();
                mine_show();
                break;

            case MINESWEEPER_LOSE:
                rb->splash( HZ, "You Lose!" );
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
