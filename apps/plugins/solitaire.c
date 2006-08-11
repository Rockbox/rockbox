/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004-2006 Antoine Cellerier <dionoea @t videolan d.t org>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************************************************************
Solitaire by dionoea
Graphics & Fix Bugs by Ben Basha

use arrows to move the cursor
use ON to select cards, move cards, reveal hidden cards, ...
use PLAY to move a card from the remains' stack to the top of the cursor
use F1 to put card under cursor on one of the 4 final stacks
use F2 to un-select card if a card was selected, else draw 3 new cards
    out of the remains' stack
use F3 to put card on top of the remains' stack on one of the 4 final stacks

*****************************************************************************/

#include "plugin.h"
#include "configfile.h"
#include "button.h"
#include "lcd.h"

#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

static struct plugin_api* rb;
#define min(a,b) (a<b?a:b)

/**
 * Key definitions
 */

#if CONFIG_KEYPAD == RECORDER_PAD
#   define SOL_QUIT         BUTTON_OFF
#   define SOL_UP           BUTTON_UP
#   define SOL_DOWN         BUTTON_DOWN
#   define SOL_LEFT         BUTTON_LEFT
#   define SOL_RIGHT        BUTTON_RIGHT
#   define SOL_MOVE         BUTTON_ON
#   define SOL_DRAW         BUTTON_F2
#   define SOL_REM2CUR      BUTTON_PLAY
#   define SOL_CUR2STACK    BUTTON_F1
#   define SOL_REM2STACK    BUTTON_F3
#   define SOL_MENU_RUN     BUTTON_RIGHT
#   define SOL_MENU_RUN2    BUTTON_PLAY
#   define HK_MOVE         "ON"
#   define HK_DRAW         "F2"
#   define HK_REM2CUR      "PLAY"
#   define HK_CUR2STACK    "F1"
#   define HK_REM2STACK    "F3"

#elif CONFIG_KEYPAD == ONDIO_PAD
#   define SOL_QUIT         BUTTON_OFF
#   define SOL_UP_PRE       BUTTON_UP
#   define SOL_UP           (BUTTON_UP | BUTTON_REL)
#   define SOL_DOWN_PRE     BUTTON_DOWN
#   define SOL_DOWN         (BUTTON_DOWN | BUTTON_REL)
#   define SOL_LEFT_PRE     BUTTON_LEFT
#   define SOL_LEFT         (BUTTON_LEFT | BUTTON_REL)
#   define SOL_RIGHT_PRE    BUTTON_RIGHT
#   define SOL_RIGHT        (BUTTON_RIGHT | BUTTON_REL)
#   define SOL_MOVE_PRE     BUTTON_MENU
#   define SOL_MOVE         (BUTTON_MENU | BUTTON_REL)
#   define SOL_DRAW_PRE     BUTTON_MENU
#   define SOL_DRAW         (BUTTON_MENU | BUTTON_REPEAT)
#   define SOL_REM2CUR_PRE  BUTTON_LEFT
#   define SOL_REM2CUR      (BUTTON_LEFT | BUTTON_REPEAT)
#   define SOL_CUR2STACK_PRE BUTTON_RIGHT
#   define SOL_CUR2STACK    (BUTTON_RIGHT | BUTTON_REPEAT)
#   define SOL_REM2STACK_PRE BUTTON_UP
#   define SOL_REM2STACK    (BUTTON_UP | BUTTON_REPEAT)
#   define SOL_MENU_RUN     BUTTON_RIGHT
#   define HK_MOVE         "MODE"
#   define HK_DRAW         "MODE.."
#   define HK_REM2CUR      "LEFT.."
#   define HK_CUR2STACK    "RIGHT.."
#   define HK_REM2STACK    "UP.."

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define SOL_QUIT         BUTTON_OFF
#   define SOL_UP           BUTTON_UP
#   define SOL_DOWN         BUTTON_DOWN
#   define SOL_LEFT         BUTTON_LEFT
#   define SOL_RIGHT        BUTTON_RIGHT
#   define SOL_MOVE_PRE     BUTTON_SELECT
#   define SOL_MOVE         (BUTTON_SELECT | BUTTON_REL)
#   define SOL_DRAW         BUTTON_MODE
#   define SOL_REM2CUR      (BUTTON_LEFT | BUTTON_ON)
#   define SOL_CUR2STACK    (BUTTON_SELECT | BUTTON_REPEAT)
#   define SOL_REM2STACK    (BUTTON_RIGHT | BUTTON_ON)
#   define SOL_MENU_RUN     BUTTON_SELECT
#   define SOL_MENU_RUN2    BUTTON_RIGHT
#   define SOL_OPT          BUTTON_ON
#   define SOL_REM          BUTTON_REC
#   define SOL_RC_QUIT      BUTTON_RC_STOP
#   define HK_MOVE         "SELECT"
#   define HK_DRAW         "REC"
#   define HK_REM2CUR      "PLAY+LEFT"
#   define HK_CUR2STACK    "SELECT"
#   define HK_REM2STACK    "PLAY+RIGHT"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) ||(CONFIG_KEYPAD == IPOD_3G_PAD)
#   define SOL_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#   define SOL_UP           BUTTON_MENU
#   define SOL_DOWN         BUTTON_PLAY
#   define SOL_LEFT         BUTTON_LEFT
#   define SOL_RIGHT        BUTTON_RIGHT
#   define SOL_MOVE         BUTTON_SELECT
#   define SOL_DRAW         (BUTTON_SELECT | BUTTON_PLAY)
#   define SOL_REM2CUR      (BUTTON_SELECT | BUTTON_LEFT)
#   define SOL_CUR2STACK    (BUTTON_SELECT | BUTTON_RIGHT)
#   define SOL_REM2STACK    (BUTTON_LEFT | BUTTON_RIGHT)
#   define SOL_MENU_RUN     BUTTON_SELECT
#   define HK_MOVE         "SELECT"
#   define HK_DRAW         "SELECT+PLAY"
#   define HK_REM2CUR      "SELECT+LEFT"
#   define HK_CUR2STACK    "SELECT+RIGHT.."
#   define HK_REM2STACK    "LEFT+RIGHT"

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#   define SOL_QUIT         BUTTON_POWER
#   define SOL_UP           BUTTON_UP
#   define SOL_DOWN         BUTTON_DOWN
#   define SOL_LEFT         BUTTON_LEFT
#   define SOL_RIGHT        BUTTON_RIGHT
#   define SOL_MOVE         BUTTON_SELECT
#   define SOL_DRAW         BUTTON_PLAY
#   define SOL_REM2CUR      (BUTTON_REC | BUTTON_LEFT)
#   define SOL_CUR2STACK    (BUTTON_REC | BUTTON_UP)
#   define SOL_REM2STACK    (BUTTON_REC | BUTTON_DOWN)
#   define SOL_MENU_RUN     BUTTON_SELECT
#   define HK_MOVE         "MENU"
#   define HK_DRAW         "PLAY"
#   define HK_REM2CUR      "REC+LEFT"
#   define HK_CUR2STACK    "REC+UP.."
#   define HK_REM2STACK    "REC+DOWN"

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define SOL_QUIT         BUTTON_A
#   define SOL_UP           BUTTON_UP
#   define SOL_DOWN         BUTTON_DOWN
#   define SOL_LEFT         BUTTON_LEFT
#   define SOL_RIGHT        BUTTON_RIGHT
#   define SOL_MOVE_PRE     BUTTON_SELECT
#   define SOL_MOVE         (BUTTON_SELECT | BUTTON_REL)
#   define SOL_DRAW         BUTTON_MENU
#   define SOL_REM2CUR      (BUTTON_LEFT | BUTTON_POWER)
#   define SOL_CUR2STACK    (BUTTON_SELECT | BUTTON_REPEAT)
#   define SOL_REM2STACK    (BUTTON_RIGHT | BUTTON_POWER)
#   define SOL_MENU_RUN     BUTTON_SELECT
#   define SOL_MENU_RUN2    BUTTON_RIGHT
#   define HK_MOVE         "SELECT"
#   define HK_DRAW         "MENU"
#   define HK_REM2CUR      "POWER+LEFT"
#   define HK_CUR2STACK    "SELECT.."
#   define HK_REM2STACK    "POWER+RIGHT"

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define SOL_QUIT        BUTTON_POWER
#   define SOL_UP          BUTTON_SCROLL_UP
#   define SOL_DOWN        BUTTON_SCROLL_DOWN
#   define SOL_LEFT        BUTTON_LEFT
#   define SOL_RIGHT       BUTTON_RIGHT
#   define SOL_MOVE        BUTTON_REW
#   define SOL_DRAW        BUTTON_PLAY
#   define SOL_REM2CUR     (BUTTON_FF | BUTTON_LEFT)
#   define SOL_CUR2STACK   (BUTTON_FF | BUTTON_SCROLL_UP)
#   define SOL_REM2STACK   (BUTTON_FF | BUTTON_SCROLL_DOWN)
#   define SOL_MENU_RUN    BUTTON_REW
#   define SOL_MENU_INFO   BUTTON_PLAY
#   define HK_MOVE         "REW"
#   define HK_DRAW         "PLAY"
#   define HK_REM2CUR      "REW+LEFT"
#   define HK_CUR2STACK    "REW+UP.."
#   define HK_REM2STACK    "REW+DOWN"

#else
#   error "Unknown keypad"
#endif

/**
 * Help strings
 */

#define HELP_SOL_UP     "UP: Move the cursor up in the column."
#define HELP_SOL_DOWN   "DOWN: Move the cursor down in the column."
#define HELP_SOL_LEFT   "LEFT: Move the cursor to the previous column."
#define HELP_SOL_RIGHT  "RIGHT: Move the cursor to the next column."
#define HELP_SOL_MOVE HK_MOVE \
    ": Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW HK_DRAW \
    ": Un-select a card if it was selected. " \
    "Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR HK_REM2CUR \
    ": Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK  HK_CUR2STACK \
    ": Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK HK_REM2STACK \
    ": Put the card on top of the remains' stack on one of the 4 final stacks."

/**
 * Misc constants, graphics and other defines
 */

/* size of a card on the screen */
#if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#   define CARD_WIDTH  27
#   define CARD_HEIGHT 34
#elif LCD_HEIGHT > 64
#   define CARD_WIDTH  19
#   define CARD_HEIGHT 24
#else
#   define CARD_WIDTH  15
#   define CARD_HEIGHT 12
#endif

/* where the cards start */
#if LCD_HEIGHT > 64
#   define UPPER_ROW_MARGIN 2
#   define CARD_START ( CARD_HEIGHT + 4 )
#else
    /* The screen is *small* */
#   define UPPER_ROW_MARGIN 0
#   define CARD_START ( CARD_HEIGHT + 1 )
#endif


#if LCD_HEIGHT > 64
#   define NUMBER_HEIGHT 10
#   define NUMBER_WIDTH  8
#   define NUMBER_STRIDE 8
#   define SUIT_HEIGHT 10
#   define SUIT_WIDTH  8
#   define SUIT_STRIDE 8
#else
#   define NUMBER_HEIGHT 6
#   define NUMBER_WIDTH  6
#   define NUMBER_STRIDE 6
#   define SUIT_HEIGHT 6
#   define SUIT_WIDTH  6
#   define SUIT_STRIDE 6
#endif

#define SUITI_HEIGHT 16
#define SUITI_WIDTH  15
#define SUITI_STRIDE 15


#define draw_number( num, x, y ) \
    rb->lcd_bitmap_part( numbers, 0, num * NUMBER_HEIGHT, NUMBER_STRIDE, \
                         x, y, NUMBER_WIDTH, NUMBER_HEIGHT );
extern const fb_data solitaire_numbers[];
#define numbers solitaire_numbers

#define draw_suit( num, x, y ) \
    rb->lcd_bitmap_part( suits, 0, num * SUIT_HEIGHT, SUIT_STRIDE, \
                         x, y, SUIT_WIDTH, SUIT_HEIGHT );
extern const fb_data solitaire_suits[];
#define suits   solitaire_suits

#if ( CARD_HEIGHT < SUITI_HEIGHT + 1 ) || ( CARD_WIDTH < SUITI_WIDTH + 1 )
#   undef  SUITI_HEIGHT
#   undef  SUITI_WIDTH
#   define SUITI_HEIGHT SUIT_HEIGHT
#   define SUITI_WIDTH  SUIT_WIDTH
#   define draw_suiti( num, x, y ) draw_suit( num, x, y )
#else
#   define draw_suiti( num, x, y ) \
    rb->lcd_bitmap_part( suitsi, 0, num * SUITI_HEIGHT, SUITI_STRIDE, \
                         x, y, SUITI_WIDTH, SUITI_HEIGHT );
    extern const fb_data solitaire_suitsi[];
#   define suitsi  solitaire_suitsi
#endif

#ifdef HAVE_LCD_COLOR
#   if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#       define CARDBACK_HEIGHT 33
#       define CARDBACK_WIDTH  26
#   else
#       define CARDBACK_HEIGHT 24
#       define CARDBACK_WIDTH  18
#   endif

    extern const fb_data solitaire_cardback[];
#endif

#define CONFIG_FILENAME "sol.cfg"

#define NOT_A_CARD 255

/* number of cards per suit */
#define CARDS_PER_SUIT 13

/* number of suits */
#define SUITS 4

#define NUM_CARDS ( CARDS_PER_SUIT * SUITS )

/* number of columns */
#define COL_NUM 7

/* pseudo column numbers to be used for cursor coordinates */
/* columns COL_NUM to COL_NUM + SUITS - 1 correspond to the final stacks */
#define STACKS_COL COL_NUM
/* column COL_NUM + SUITS corresponds to the remains' stack */
#define REM_COL (STACKS_COL + SUITS)

#define NOT_A_COL 255

/* background color */
#define BACKGROUND_COLOR LCD_RGBPACK(0,157,0)

#if LCD_DEPTH > 1 && !defined( LCD_WHITE )
#   define LCD_WHITE LCD_DEFAULT_BG
#endif

typedef struct
{
    unsigned char suit : 2;
    unsigned char num : 4;
    unsigned char known : 1;
    unsigned char used : 1;/* this is what is used when dealing cards */
    unsigned char next;
} card_t;


/**
 * LCD card drawing routines
 */

static void draw_cursor( int x, int y )
{
    rb->lcd_set_drawmode( DRMODE_COMPLEMENT );
    rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-1, CARD_HEIGHT-1 );
    rb->lcd_set_drawmode( DRMODE_SOLID );
}

/* Draw a card's border, select it if it's selected and draw the cursor
 * is the cursor is currently over the card */
static void draw_card_ext( int x, int y, bool selected, bool cursor )
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground( LCD_BLACK );
#endif
    /* draw a rectangle omiting the corner pixels, which is why we don't
     * use drawrect */
    rb->lcd_drawline( x+1, y, x+CARD_WIDTH-1, y );
    rb->lcd_drawline( x+1, y+CARD_HEIGHT, x+CARD_WIDTH-1, y+CARD_HEIGHT );
    rb->lcd_drawline( x, y+1, x, y+CARD_HEIGHT-1 );
    rb->lcd_drawline( x+CARD_WIDTH, y+1, x+CARD_WIDTH, y+CARD_HEIGHT-1 );

    if( selected )
    {
        rb->lcd_drawrect( x+1, y+1, CARD_WIDTH-1, CARD_HEIGHT-1 );
    }
    if( cursor )
    {
        draw_cursor( x, y );
    }
}

/* Draw a card's inner graphics */
static void draw_card( card_t card, int x, int y,
                       bool selected, bool cursor, bool leftstyle )
{
#ifndef HAVE_LCD_COLOR
    /* On Black&White or Greyscale LCDs we don't have a card back.
     * We thus need to clear the card area even if the card isn't
     * known. */
#if LCD_DEPTH > 1
    rb->lcd_set_foreground( LCD_WHITE );
#else
    rb->lcd_set_drawmode( DRMODE_SOLID|DRMODE_INVERSEVID );
#endif
    rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-1, CARD_HEIGHT-1 );
#if LCD_DEPTH == 1
    rb->lcd_set_drawmode( DRMODE_SOLID );
#endif
#endif
    if( card.known )
    {
#ifdef HAVE_LCD_COLOR
        /* On Color LCDs we have a card back so we only need to clear
         * the card area when it's known*/
        rb->lcd_set_foreground( LCD_WHITE );
        rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-1, CARD_HEIGHT-1 );
#endif
        if( leftstyle )
        {
#if UPPER_ROW_MARGIN > 0
            draw_suit( card.suit, x+1, y+2+NUMBER_HEIGHT );
            draw_number( card.num, x+1, y+1 );
#else
            draw_suit( card.suit, x+1, y+NUMBER_HEIGHT );
            draw_number( card.num, x+1, y );
#endif
        }
        else
        {
            draw_suit( card.suit, x+2+NUMBER_WIDTH, y+1 );
            draw_number( card.num, x+1, y+1 );
        }
    }
#ifdef HAVE_LCD_COLOR
    else
    {
        rb->lcd_bitmap( solitaire_cardback, x+1, y+1,
                        CARDBACK_WIDTH, CARDBACK_HEIGHT );
    }
#endif

    draw_card_ext( x, y, selected, cursor );
}

/* Draw an empty stack */
static void draw_empty_stack( int s, int x, int y, bool cursor )
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground( LCD_WHITE );
#else
    rb->lcd_set_drawmode( DRMODE_SOLID|DRMODE_INVERSEVID );
#endif
    rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-1, CARD_HEIGHT-1 );
#if LCD_DEPTH == 1
    rb->lcd_set_drawmode( DRMODE_SOLID );
#endif
    draw_suiti( s, x+(CARD_WIDTH-SUITI_WIDTH)/2,
                y+(CARD_HEIGHT-SUITI_HEIGHT)/2 );

    draw_card_ext( x, y, false, cursor );
}

/**
 * Help
 *
 * TODO: the help menu should just list the key definitions. Asking the
 *       user to try all possible keys/key combos is just counter
 *       productive.
 */

enum help { HELP_QUIT, HELP_USB };

/* help for the not so intuitive interface */
enum help solitaire_help( void )
{

    int button;
    int lastbutton = BUTTON_NONE;

    while( true )
    {
        rb->lcd_clear_display();

#if CONFIG_KEYPAD == RECORDER_PAD
        rb->lcd_putsxy(0, 0, "Press a key to see");
        rb->lcd_putsxy(0, 7, "it's role.");
        rb->lcd_putsxy(0, 21, "Press OFF to");
        rb->lcd_putsxy(0, 28, "return to menu.");
        rb->lcd_putsxy(0, 42, "All actions can");
        rb->lcd_putsxy(0, 49, "be done using");
        rb->lcd_putsxy(0, 56, "arrows, ON and F2.");
#elif CONFIG_KEYPAD == ONDIO_PAD
        rb->lcd_putsxy(0, 0, "Press a key short");
        rb->lcd_putsxy(0, 7, "or long to see it's");
        rb->lcd_putsxy(0, 21, "role. Press OFF to");
        rb->lcd_putsxy(0, 28, "return to menu.");
        rb->lcd_putsxy(0, 42, "All actions can be");
        rb->lcd_putsxy(0, 49, "done using arrows,");
        rb->lcd_putsxy(0, 56, "short & long MODE.");
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
        rb->lcd_putsxy(20, 8, "Press a key or key");
        rb->lcd_putsxy(20, 16, "combo to see it's");
        rb->lcd_putsxy(20, 24, "role. Press STOP to");
        rb->lcd_putsxy(20, 32, "return to menu.");
        rb->lcd_putsxy(20, 48, "All actions can be");
        rb->lcd_putsxy(20, 56, "done using the");
        rb->lcd_putsxy(20, 64, "joystick and RECORD.");
#else
//#   warning "Add help strings for other keypads"
#endif

        rb->lcd_update();

        button = rb->button_get( true );
        switch( button )
        {
            case SOL_UP:
#ifdef SOL_UP_PRE
                if( lastbutton != SOL_UP_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_UP );
                break;

            case SOL_DOWN:
#ifdef SOL_DOWN_PRE
                if( lastbutton != SOL_DOWN_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_DOWN );
                break;

            case SOL_LEFT:
#ifdef SOL_LEFT_PRE
                if( lastbutton != SOL_LEFT_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_LEFT );
                break;

            case SOL_RIGHT:
#ifdef SOL_RIGHT_PRE
                if( lastbutton != SOL_RIGHT_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_RIGHT );
                break;

            case SOL_MOVE:
#ifdef SOL_MOVE_PRE
                if( lastbutton != SOL_MOVE_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_MOVE );
                break;

            case SOL_DRAW:
#ifdef SOL_DRAW_PRE
                if( lastbutton != SOL_DRAW_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_DRAW );
                break;

            case SOL_CUR2STACK:
#ifdef SOL_CUR2STACK_PRE
                if( lastbutton != SOL_CUR2STACK_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_CUR2STACK );
                break;

            case SOL_REM2STACK:
#ifdef SOL_REM2STACK_PRE
                if( lastbutton != SOL_REM2STACK_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_REM2STACK );
                break;

            case SOL_REM2CUR:
#ifdef SOL_REM2CUR_PRE
                if( lastbutton != SOL_REM2CUR_PRE )
                    break;
#endif
                rb->splash( HZ*2, true, HELP_SOL_REM2CUR );
                break;
#ifdef SOL_RC_QUIT
            case SOL_RC_QUIT:
#endif
            case SOL_QUIT:
                return HELP_QUIT;

            default:
                if( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
                    return HELP_USB;
                break;
        }
        if( button != BUTTON_NONE )
            lastbutton = button;
    }
}

/**
 * Custom menu / options
 *
 * TODO: use rockbox api menus instead
 */

#define CFGFILE_VERSION 0
int draw_type;

static struct configdata config[] = {
   { TYPE_INT, 0, 1, &draw_type, "draw_type", NULL, NULL }
};

/* menu return codes */
enum { MENU_RESUME, MENU_RESTART, MENU_OPT,
       MENU_HELP,   MENU_QUIT,    MENU_USB };
#define MENU_LENGTH MENU_USB

/* different menu behaviors */
enum { MENU_BEFOREGAME, MENU_BEFOREGAMEOP, MENU_DURINGGAME };

/**
 * The menu
 * text displayed changes depending on the context */
int solitaire_menu( unsigned char context )
{
    static char menu[3][MENU_LENGTH][17] =
        { { "Start Game",
            "",
            "Draw Three Cards",
            "Help",
            "Quit" },
          { "Start Game",
            "",
            "Draw One Card",
            "Help",
            "Quit" },
          { "Resume Game",
            "Restart Game",
            "",
            "Help",
            "Quit"},
        };


    int i;
    int cursor = 0;
    int button;

    int fh;
    rb->lcd_getstringsize( menu[0][0], NULL, &fh );
    fh++;

    if(    context != MENU_BEFOREGAMEOP
        && context != MENU_BEFOREGAME
        && context != MENU_DURINGGAME )
    {
        context = MENU_DURINGGAME;
    }

    while( true )
    {

        rb->lcd_clear_display();
        rb->lcd_putsxy( 20, 1, "Solitaire" );

        for( i = 0; i<MENU_LENGTH; i++ )
        {
            rb->lcd_putsxy( 1, 17+fh*i, menu[context][i] );
            if( cursor == i )
            {
                rb->lcd_set_drawmode( DRMODE_COMPLEMENT );
                rb->lcd_fillrect( 0, 17+fh*i, LCD_WIDTH, fh );
                rb->lcd_set_drawmode( DRMODE_SOLID );
            }
        }

        rb->lcd_update();

        button = rb->button_get( true );
        switch( button )
        {
            case SOL_UP:
                cursor = (cursor + MENU_LENGTH - 1)%MENU_LENGTH;
                break;

            case SOL_DOWN:
                cursor = (cursor + 1)%MENU_LENGTH;
                break;

            case SOL_LEFT:
                return MENU_RESUME;

            case SOL_MENU_RUN:
#ifdef SOL_MENU_RUN2
            case SOL_MENU_RUN2:
#endif
                switch( cursor )
                {
                    case MENU_RESUME:
                    case MENU_RESTART:
                    case MENU_OPT:
                    case MENU_QUIT:
                        return cursor;

                    case MENU_HELP:
                        if( solitaire_help() == HELP_USB )
                            return MENU_USB;
                        break;
                }
                break;

#ifdef SOL_OPT
            case SOL_OPT:
                return MENU_OPT;
#endif

#ifdef SOL_RC_QUIT
            case SOL_RC_QUIT:
#endif
            case SOL_QUIT:
                return MENU_QUIT;

            default:
                if( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
                    return MENU_USB;
                break;
        }
    }
}

/**
 * Global variables
 */

/* player's cursor */
unsigned char cur_card;
/* player's cursor column num */
unsigned char cur_col;

/* selected card */
unsigned char sel_card;

/* the deck */
card_t deck[ NUM_CARDS ];

/* the remaining cards */
unsigned char rem;
unsigned char cur_rem;
unsigned char coun_rem;

/* the 7 game columns */
unsigned char cols[COL_NUM];

int cards_per_draw;
/* the 4 final stacks */
unsigned char stacks[SUITS];

/**
 * Card handling routines
 */

unsigned char next_random_card( card_t *deck )
{
    unsigned char i,r;

    r = rb->rand()%(NUM_CARDS)+1;
    i = 0;

    while( r>0 )
    {
        i = (i + 1)%(NUM_CARDS);
        if( !deck[i].used ) r--;
    }

    deck[i].used = 1;

    return i;
}


/* initialize the game */
void solitaire_init( void )
{

    unsigned char c;
    int i, j;

    /* number of cards that are drawn on the remains' stack (by pressing F2) */
    if( draw_type == 0 )
    {
      cards_per_draw = 3;
    }
    else
    {
      cards_per_draw = 1;
    }

    /* init deck */
    for( i=0; i<SUITS; i++ )
    {
        for( j=0; j<CARDS_PER_SUIT; j++ )
        {
#define card deck[i*CARDS_PER_SUIT+j]
            card.suit = i;
            card.num = j;
            card.known = 1;
            card.used = 0;
            card.next = NOT_A_CARD;
#undef card
        }
    }

    /* deal the cards ... */
    /* ... in the columns */
    for( i=0; i<COL_NUM; i++ )
    {
        c = NOT_A_CARD;
        for( j=0; j<=i; j++ )
        {
            if( c == NOT_A_CARD )
            {
                cols[i] = next_random_card( deck );
                c = cols[i];
            }
            else
            {
                deck[c].next = next_random_card( deck );
                c = deck[c].next;
            }
            if( j < i )
                deck[c].known = 0;
        }
    }

    /* ... shuffle what's left of the deck */
    rem = next_random_card(deck);
    c = rem;

    for( i=1; i < NUM_CARDS - COL_NUM * (COL_NUM + 1)/2; i++ )
    {
        deck[c].next = next_random_card( deck );
        c = deck[c].next;
    }

    /* we now finished dealing the cards. The game can start ! (at last) */

    /* init the stack */
    for( i = 0; i<SUITS; i++ )
    {
        stacks[i] = NOT_A_CARD;
    }

    /* the cursor starts on upper left card */
    cur_card = cols[0];
    cur_col = 0;

    /* no card is selected */
    sel_card = NOT_A_CARD;

    /* init the remainder */
    cur_rem = NOT_A_CARD;

    coun_rem=0;
}

/* find the column number in which 'card' can be found */
unsigned char find_card_col( unsigned char card )
{
    int i;
    unsigned char c;

    if( card == NOT_A_CARD ) return NOT_A_COL;

    for( i=0; i<COL_NUM; i++ )
    {
        c = cols[i];
        while( c != NOT_A_CARD )
        {
            if( c == card ) return i;
            c = deck[c].next;
        }
    }

    for( i=0; i<SUITS; i++ )
    {
        c = stacks[i];
        while( c != NOT_A_CARD )
        {
            if( c == card ) return STACKS_COL + i;
            c = deck[c].next;
        }
    }

    return REM_COL;
}

/* find the card preceding 'card' */
/* if it doesn't exist, return NOT_A_CARD */
unsigned char find_prev_card( unsigned char card ){
    int i;

    for( i=0; i < NUM_CARDS; i++ )
    {
        if( deck[i].next == card ) return i;
    }

    return NOT_A_CARD;
}

/* find the last card of a given column */
unsigned char find_last_card( unsigned char col )
{
    unsigned char c;

    if( col < COL_NUM )
    {
        c = cols[col];
    }
    else if( col < REM_COL )
    {
        c = stacks[col - STACKS_COL];
    }
    else
    {
        c = rem;
    }

    if(c == NOT_A_CARD)
        return c;
    else
    {
        while(deck[c].next != NOT_A_CARD)
            c = deck[c].next;
        return c;
    }
}

enum move { MOVE_OK, MOVE_NOT_OK };

enum move move_card( unsigned char dest_col, unsigned char src_card )
{
    /* the column on which to take src_card */
    unsigned char src_col;

    /* the last card of dest_col */
    unsigned char dest_card;

    /* the card under src_card */
    unsigned char src_card_prev;

    /* you can't move no card (at least, it doesn't have any consequence) */
    if( src_card == NOT_A_CARD ) return MOVE_NOT_OK;
    /* you can't put a card back on the remains' stack */
    if( dest_col == REM_COL ) return MOVE_NOT_OK;

    src_col = find_card_col( src_card );
    dest_card = find_last_card( dest_col );
    src_card_prev = find_prev_card( src_card );

    /* you can't move more than one card at a time from the final stack */
    /* to the rest of the game */
    if( src_col >= COL_NUM && src_col < REM_COL
       && deck[src_card].next != NOT_A_CARD )
    {
        return MOVE_NOT_OK;
    }

    /* if we (that means dest) are on one of the 7 columns ... */
    if( dest_col < COL_NUM )
    {
        /* ... check is we are on an empty color and that the src is a king */
        if( dest_card == NOT_A_CARD
            && deck[src_card].num == CARDS_PER_SUIT - 1 )
        {
            /* this is a winning combination */
            cols[dest_col] = src_card;
        }
        /* ... or check if the cards follow one another and have same suit */
        else if(( deck[dest_card].suit + deck[src_card].suit)%2==1
                  && deck[dest_card].num == deck[src_card].num + 1 )
        {
            /* this is a winning combination */
            deck[dest_card].next = src_card;
        }
        /* ... or, humpf, well that's not good news */
        else
        {
            /* this is not a winning combination */
            return MOVE_NOT_OK;
        }
    }
    /* if we are on one of the 4 final stacks ... */
    else if( dest_col < REM_COL )
    {
        /* ... check if we are on an empty stack, that the src is an
         * ace and that this is the good final stack */
        if( dest_card == NOT_A_CARD
            && deck[src_card].num == 0
            && deck[src_card].suit == dest_col - STACKS_COL )
        {
            /* this is a winning combination */
            stacks[dest_col - STACKS_COL] = src_card;
        }
        /* ... or check if the cards follow one another, have the same
         * suit and {that src has no .next element or is from the remains'
         * stack} */
        else if( deck[dest_card].suit == deck[src_card].suit
                 && deck[dest_card].num + 1 == deck[src_card].num
                 && (deck[src_card].next == NOT_A_CARD || src_col == REM_COL) )
        {
            /* this is a winning combination */
            deck[dest_card].next = src_card;
        }
        /* ... or, well that's not good news */
        else
        {
            /* this is not a winnong combination */
            return MOVE_NOT_OK;
        }
    }
    /* if we are on the remains' stack */
    else
    {
        /* you can't move a card back to the remains' stack */
        return MOVE_NOT_OK;
    }

    /* if the src card is from the remains' stack, we don't want to take
     * the following cards */
    if( src_col == REM_COL )
    {
        /* if src card is the first card from the stack */
        if( src_card_prev == NOT_A_CARD )
        {
            rem = deck[src_card].next;
            coun_rem = coun_rem-1;
        }
        /* if src card is not the first card from the stack */
        else
        {
            deck[src_card_prev].next = deck[src_card].next;
        }
        deck[src_card].next = NOT_A_CARD;
        cur_rem = src_card_prev;
        coun_rem = coun_rem-1;
    }
    /* if the src card is from somewhere else, just take everything */
    else
    {
        if( src_card_prev == NOT_A_CARD )
        {
            if( src_col < COL_NUM )
            {
                cols[src_col] = NOT_A_CARD;
            }
            else
            {
                stacks[src_col - STACKS_COL] = NOT_A_CARD;
            }
        }
        else
        {
            deck[src_card_prev].next = NOT_A_CARD;
        }
    }
    sel_card = NOT_A_CARD;
    /* tada ! */
    return MOVE_OK;
}

/**
 * The main game loop
 */

enum { SOLITAIRE_WIN, SOLITAIRE_QUIT, SOLITAIRE_USB };

int solitaire( void )
{

    int i,j;
    int button, lastbutton = 0;
    unsigned char c,h,prevcard;
    int biggest_col_length;

    configfile_init(rb);
    configfile_load(CONFIG_FILENAME, config, 1, 0);

    rb->srand( *rb->current_tick );
    switch( solitaire_menu( draw_type == 0 ? MENU_BEFOREGAME
                                           : MENU_BEFOREGAMEOP ) )
    {
        case MENU_QUIT:
            return SOLITAIRE_QUIT;

        case MENU_USB:
            return SOLITAIRE_USB;

        case MENU_OPT:
            draw_type = (draw_type+1)%2;
            configfile_save(CONFIG_FILENAME, config, 1, 0);
            return 0;
    }
    solitaire_init();

    while( true )
    {

#if LCD_DEPTH>1
        rb->lcd_set_foreground(LCD_BLACK);
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_background(BACKGROUND_COLOR);
#endif
#endif
        rb->lcd_clear_display();

#if LCD_DEPTH > 1
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_set_background(LCD_WHITE);
#endif

        /* get the biggest column length so that display can be "optimized" */
        biggest_col_length = 0;

        for(i=0;i<COL_NUM;i++)
        {
            j = 0;
            c = cols[i];
            while( c != NOT_A_CARD )
            {
                j++;
                c = deck[c].next;
            }
            if( j > biggest_col_length ) biggest_col_length = j;
        }

        /* check if there are cards remaining in the game. */
        /* if there aren't any, that means you won :) */
        if( biggest_col_length == 0 && rem == NOT_A_CARD )
        {
            rb->splash( HZ*2, true, "You Won :)" );
            return SOLITAIRE_WIN;
        }

        /* draw the columns */
        for( i = 0; i < COL_NUM; i++ )
        {
            c = cols[i];
            j = CARD_START;
            while( true )
            {
                if( c == NOT_A_CARD )
                {
                    /* draw the cursor on empty columns */
                    if( cur_col == i )
                    {
                        draw_cursor( 1+i*((LCD_WIDTH - 2)/COL_NUM), j+1 );
                    }
                    break;
                }

                draw_card( deck[c], 1+i*((LCD_WIDTH - 2)/COL_NUM), j+1,
                           c == sel_card, c == cur_card, false );

                h = c;
                c = deck[c].next;
                if( c == NOT_A_CARD ) break;

                /* This is where we change the spacing between cards so that
                 * they don't overflow out of the LCD */
                if( h == cur_card )
                    j += SUIT_HEIGHT+2;
                else
                    j += min( SUIT_HEIGHT+2,
                   (LCD_HEIGHT - CARD_START - CARD_HEIGHT)/biggest_col_length );
            }
        }

        /* draw the stacks */
        for( i=0; i<SUITS; i++ )
        {
            c = find_last_card( STACKS_COL + i );

            if( c != NOT_A_CARD )
            {
                draw_card( deck[c],
                           LCD_WIDTH - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1,
                           UPPER_ROW_MARGIN,
                           c == sel_card, cur_col == STACKS_COL + i, false );
            }
            else
            {
                draw_empty_stack( i,
                           LCD_WIDTH - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1,
                           UPPER_ROW_MARGIN,
                           cur_col == STACKS_COL + i );
            }
        }

        /* draw the remains */
        if( ( cur_rem == NOT_A_CARD && rem != NOT_A_CARD )
            || deck[cur_rem].next != NOT_A_CARD )
        {
            /* gruik ! (we want to display a card back) */
            deck[rem].known = false;
            draw_card( deck[rem], UPPER_ROW_MARGIN, UPPER_ROW_MARGIN,
                       false, false, false );
            deck[rem].known = true;
        }

        if( rem != NOT_A_CARD )
        {
            if( coun_rem >= cards_per_draw )
                coun_rem = cards_per_draw-1;
            if(    cur_rem != NOT_A_CARD
                && find_prev_card(cur_rem) != NOT_A_CARD )
            {
                prevcard = cur_rem;
#if UPPER_ROW_MARGIN > 0
                j = CARD_WIDTH+2*UPPER_ROW_MARGIN+1;
#else
                j = CARD_WIDTH/2+2*UPPER_ROW_MARGIN+1;
#endif
                for( i = 0; i < coun_rem; i++ )
                    prevcard = find_prev_card(prevcard);
                for( i = 0; i <= coun_rem; i++ )
                {
                    draw_card( deck[prevcard], j,
                               UPPER_ROW_MARGIN, sel_card == prevcard,
                               cur_card == prevcard, i < coun_rem );
                    prevcard = deck[prevcard].next;
                    j += NUMBER_WIDTH+2;
                }
            }
            else if( cur_rem == NOT_A_CARD && cur_col == REM_COL )
            {
                draw_cursor( CARD_WIDTH+2*UPPER_ROW_MARGIN+1,
                             UPPER_ROW_MARGIN );
            }
        }

        rb->lcd_update();

        /* what to do when a key is pressed ... */
        button = rb->button_get( true );
        switch( button )
        {
            /* move cursor to the last card of the previous column
             * or to the previous final stack
             * or to the remains stack */
            case SOL_RIGHT:
#ifdef SOL_RIGHT_PRE
                if( lastbutton != SOL_RIGHT_PRE )
                    break;
#endif
                if( cur_col >= COL_NUM )
                {
                    cur_col = 0;
                }
                else if( cur_col == COL_NUM - 1 )
                {
                    cur_col = REM_COL;
                }
                else
                {
                    cur_col = (cur_col+1)%(REM_COL+1);
                }
                if(cur_col == REM_COL)
                {
                    cur_card = cur_rem;
                    break;
                }
                cur_card  = find_last_card( cur_col );
                break;

            /* move cursor to the last card of the next column
             * or to the next final stack
             * or to the remains stack */
            case SOL_LEFT:
#ifdef SOL_LEFT_PRE
                if( lastbutton != SOL_LEFT_PRE )
                    break;
#endif
                if( cur_col == 0 )
                {
                    cur_col = REM_COL;
                }
                else if( cur_col >= COL_NUM )
                {
                    cur_col = COL_NUM - 1;
                }
                else
                {
                    cur_col = (cur_col + REM_COL)%(REM_COL+1);
                }
                if( cur_col == REM_COL )
                {
                    cur_card = cur_rem;
                    break;
                }
                cur_card = find_last_card( cur_col );
                break;

            /* move cursor to card that's bellow */
            case SOL_DOWN:
#ifdef SOL_DOWN_PRE
                if( lastbutton != SOL_DOWN_PRE )
                    break;
#endif
                if( cur_col >= COL_NUM )
                {
                    cur_col = (cur_col - COL_NUM + 1)%(SUITS + 1) + COL_NUM;
                    if( cur_col == REM_COL )
                    {
                        cur_card = cur_rem;
                    }
                    else
                    {
                        cur_card = find_last_card( cur_col );
                    }
                    break;
                }
                if( cur_card == NOT_A_CARD ) break;
                if( deck[cur_card].next != NOT_A_CARD )
                {
                    cur_card = deck[cur_card].next;
                }
                else
                {
                    cur_card = cols[cur_col];
                    while( deck[ cur_card].known == 0
                           && deck[cur_card].next != NOT_A_CARD )
                    {
                        cur_card = deck[cur_card].next;
                    }
                }
                break;

            /* move cursor to card that's above */
            case SOL_UP:
#ifdef SOL_UP_PRE
                if( lastbutton != SOL_UP_PRE )
                    break;
#endif
                if( cur_col >= COL_NUM )
                {
                    cur_col = (cur_col - COL_NUM + SUITS)%(SUITS + 1) + COL_NUM;
                    if( cur_col == REM_COL )
                    {
                        cur_card = cur_rem;
                    }
                    else
                    {
                        cur_card = find_last_card( cur_col );
                    }
                    break;
                }
                if( cur_card == NOT_A_CARD ) break;
                do {
                    cur_card = find_prev_card( cur_card );
                    if( cur_card == NOT_A_CARD )
                    {
                        cur_card = find_last_card( cur_col );
                    }
                } while(    deck[cur_card].next != NOT_A_CARD
                         && deck[cur_card].known == 0 );
                break;

            /* Try to put card under cursor on one of the stacks */
            case SOL_CUR2STACK:
#ifdef SOL_CUR2STACK_PRE
                if( lastbutton != SOL_CUR2STACK_PRE )
                    break;
#endif
                if( cur_card != NOT_A_CARD )
                {
                    move_card( deck[cur_card].suit + STACKS_COL, cur_card );
                    sel_card = NOT_A_CARD;
                }
                break;

            /* Move cards arround, Uncover cards, ... */
            case SOL_MOVE:
#ifdef SOL_MOVE_PRE
                if( lastbutton != SOL_MOVE_PRE )
                    break;
#endif

                if( sel_card == NOT_A_CARD )
                {
                    if( cur_card != NOT_A_CARD )
                    {
                        if(    deck[cur_card].next == NOT_A_CARD
                            && deck[cur_card].known == 0 )
                        {
                            /* reveal a hidden card */
                            deck[cur_card].known = 1;
                        }
                        else if( cur_col == REM_COL && cur_rem == NOT_A_CARD )
                        {
                               break;
                        }
                        else
                        {
                            /* select a card */
                            sel_card = cur_card;
                        }
                    }
                }
                else if( sel_card == cur_card )
                {
                    /* unselect card or try putting card on
                     * one of the 4 stacks */
                    move_card( deck[sel_card].suit + COL_NUM, sel_card );
                    sel_card = NOT_A_CARD;
                    if( cur_col == REM_COL )
                    {
                        cur_card = cur_rem;
                    }
                }
                else
                {
                    /* try moving cards */
                    if( move_card( cur_col, sel_card ) == MOVE_OK )
                    {
                        sel_card = NOT_A_CARD;
                    }
                }
                break;

            /* If the card on the top of the remains can be put where
             * the cursor is, go ahead */
            case SOL_REM2CUR:
#ifdef SOL_REM2CUR_PRE
                if( lastbutton != SOL_REM2CUR_PRE )
                    break;
#endif
                coun_rem = coun_rem-1;
                move_card( cur_col, cur_rem );
                sel_card = NOT_A_CARD;
                break;

            /* If the card on top of the remains can be put on one
             * of the stacks, do so */
            case SOL_REM2STACK:
#ifdef SOL_REM2STACK_PRE
                if( lastbutton != SOL_REM2STACK_PRE )
                    break;
#endif
                if( cur_rem != NOT_A_CARD )
                {
                    move_card( deck[cur_rem].suit + COL_NUM, cur_rem );
                    sel_card = NOT_A_CARD;
                    coun_rem = coun_rem-1;
                }
                break;

#ifdef SOL_REM
            case SOL_REM:
                if( sel_card != NOT_A_CARD )
                {
                    /* unselect selected card */
                    sel_card = NOT_A_CARD;
                    break;
                }
                if( rem != NOT_A_CARD && cur_rem != NOT_A_CARD )
                {
                    sel_card=cur_rem;
                    break;
                }
                break;
#endif

            /* unselect selected card or ...
             * draw new cards from the remains of the deck */
            case SOL_DRAW:
#ifdef SOL_DRAW_PRE
                if( lastbutton != SOL_DRAW_PRE )
                    break;
#endif
                if( sel_card != NOT_A_CARD )
                {
                    /* unselect selected card */
                    sel_card = NOT_A_CARD;
                    break;
                }
                if( rem != NOT_A_CARD )
                {
                    int cur_rem_old = cur_rem;
                    coun_rem = 0;
                    /* draw new cards form the remains of the deck */
                    if( cur_rem == NOT_A_CARD )
                    {
                        /*if the cursor card is null*/
                        cur_rem = rem;
                        i = cards_per_draw - 1;
                    }
                    else
                    {
                        i = cards_per_draw;
                    }

                    while( i > 0 && deck[cur_rem].next != NOT_A_CARD )
                    {
                        cur_rem = deck[cur_rem].next;
                        i--;
                        coun_rem = coun_rem +1;
                    }
                    /* test if any cards are really left on
                     * the remains' stack */
                    if( i == cards_per_draw )
                    {
                        cur_rem = NOT_A_CARD;
                        coun_rem = 0;
                    }
                    /* if cursor was on remains' stack when new cards were
                     * drawn, put cursor on top of remains' stack */
                    if( cur_col == REM_COL && cur_card == cur_rem_old )
                    {
                        cur_card = cur_rem;
                        sel_card = NOT_A_CARD;
                    }
                }
                break;

            /* Show the menu */
#ifdef SOL_RC_QUIT
            case SOL_RC_QUIT:
#endif
            case SOL_QUIT:
#if LCD_DEPTH > 1
                rb->lcd_set_background( LCD_DEFAULT_BG );
#endif
                switch( solitaire_menu( MENU_DURINGGAME ) )
                {
                    case MENU_QUIT:
                        return SOLITAIRE_QUIT;

                    case MENU_USB:
                        return SOLITAIRE_USB;

                    case MENU_RESTART:
                        solitaire_init();
                        break;
                }
                break;

            default:
                if( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
                    return SOLITAIRE_USB;
                break;
        }

        if( button != BUTTON_NONE )
            lastbutton = button;

        /* fix incoherences concerning cur_col and cur_card */
        c = find_card_col( cur_card );
        if( c != NOT_A_COL && c != cur_col )
            cur_card = find_last_card( cur_col );

        if(    cur_card == NOT_A_CARD
            && find_last_card( cur_col ) != NOT_A_CARD )
            cur_card = find_last_card( cur_col );
    }
}

/**
 * Plugin entry point
 */

enum plugin_status plugin_start( struct plugin_api* api, void* parameter )
{
    int result;

    /* plugin init */
    (void)parameter;
    rb = api;

    rb->splash( HZ, true, "Welcome to Solitaire!" );

    /* play the game :)
     * Keep playing if a game was won (that means display the menu after
     * winning instead of quiting) */
    while( ( result = solitaire() ) == SOLITAIRE_WIN );

    /* Exit the plugin */
    return ( result == SOLITAIRE_USB ) ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}

#endif
