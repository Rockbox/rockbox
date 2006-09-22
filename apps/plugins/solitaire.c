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
#   define SOL_DRAW         BUTTON_F1
#   define SOL_REM2CUR      BUTTON_PLAY
#   define SOL_CUR2STACK    BUTTON_F2
#   define SOL_REM2STACK    BUTTON_F3
#   define HK_MOVE         "ON"
#   define HK_DRAW         "F1"
#   define HK_REM2CUR      "PLAY"
#   define HK_CUR2STACK    "F2"
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
#   define SOL_REM2CUR_PRE  BUTTON_DOWN
#   define SOL_REM2CUR      (BUTTON_DOWN | BUTTON_REPEAT)
#   define SOL_CUR2STACK_PRE BUTTON_UP
#   define SOL_CUR2STACK    (BUTTON_UP | BUTTON_REPEAT)
#   define SOL_REM2STACK_PRE BUTTON_RIGHT
#   define SOL_REM2STACK    (BUTTON_RIGHT | BUTTON_REPEAT)
#   define HK_MOVE         "MODE"
#   define HK_DRAW         "MODE.."
#   define HK_REM2CUR      "DOWN.."
#   define HK_CUR2STACK    "UP.."
#   define HK_REM2STACK    "RIGHT.."

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
#   define SOL_REM          BUTTON_REC
#   define SOL_RC_QUIT      BUTTON_RC_STOP
#   define HK_MOVE         "NAVI"
#   define HK_DRAW         "A-B"
#   define HK_REM2CUR      "PLAY+LEFT"
#   define HK_CUR2STACK    "NAVI.."
#   define HK_REM2STACK    "PLAY+RIGHT"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) ||(CONFIG_KEYPAD == IPOD_3G_PAD)
#   define SOL_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#   define SOL_UP           BUTTON_SCROLL_BACK
#   define SOL_DOWN         BUTTON_SCROLL_FWD
#   define SOL_LEFT_PRE     BUTTON_LEFT
#   define SOL_LEFT         (BUTTON_LEFT | BUTTON_REL)
#   define SOL_RIGHT_PRE    BUTTON_RIGHT
#   define SOL_RIGHT        (BUTTON_RIGHT | BUTTON_REL)
#   define SOL_MOVE_PRE     BUTTON_SELECT
#   define SOL_MOVE         (BUTTON_SELECT | BUTTON_REL)
#   define SOL_DRAW_PRE     BUTTON_MENU
#   define SOL_DRAW         (BUTTON_MENU | BUTTON_REL)
#   define SOL_REM2CUR      BUTTON_PLAY
#   define SOL_CUR2STACK_PRE BUTTON_MENU
#   define SOL_CUR2STACK    (BUTTON_MENU | BUTTON_REPEAT)
#   define SOL_REM2STACK_PRE BUTTON_RIGHT
#   define SOL_REM2STACK    (BUTTON_RIGHT | BUTTON_REPEAT)
#   define HK_UD           "SCROLL U/D"
#   define HK_MOVE         "SELECT"
#   define HK_DRAW         "MENU"
#   define HK_REM2CUR      "PLAY"
#   define HK_CUR2STACK    "MENU.."
#   define HK_REM2STACK    "RIGHT.."

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
#   define HK_MOVE         "MENU"
#   define HK_DRAW         "PLAY"
#   define HK_REM2CUR      "REC+LEFT"
#   define HK_CUR2STACK    "REC+UP"
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
#   define HK_MOVE         "REW"
#   define HK_DRAW         "PLAY"
#   define HK_REM2CUR      "REW+LEFT"
#   define HK_CUR2STACK    "REW+UP.."
#   define HK_REM2STACK    "REW+DOWN"

#else
#   error "Unknown keypad"
#endif

#define HK_LR "LEFT/RIGHT"
#ifndef HK_UD
#   define HK_UD "UP/DOWN"
#endif

/**
 * Help strings
 */

static int helplines;
static int displaylines;

static char helptext[] = 
    /* Use single spaces only! Close each line with one \0. */
    "-- Navigation --\0"
    HK_LR   ": Move the cursor to the previous/ next column.\0"
    HK_UD   ": Move the cursor up/ down in the column.\0"
    HK_MOVE ": Select cards, move cards, reveal hidden cards...\0"
    HK_DRAW ": Deselect a card if it was selected. Else draw new card(s) from the remains stack.\0"
    "-- Shortcuts --\0"
    HK_REM2CUR   ": Put the card on top of the remains stack on top of the cursor.\0"
    HK_CUR2STACK ": Put the card under the cursor on one of the 4 final stacks.\0"
    HK_REM2STACK ": Put the card on top of the remains stack on one of the 4 final stacks.\0"
;

/**
 * Misc constants, graphics and other defines
 */

/* size of a card on the screen */
#if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#   define CARD_WIDTH  28
#   define CARD_HEIGHT 35
#elif LCD_HEIGHT > 64
#   define CARD_WIDTH  20
#   define CARD_HEIGHT 25
#else
#   define CARD_WIDTH  15
#   define CARD_HEIGHT 13
#endif

/* where the cards start */
#if LCD_HEIGHT > 64
#   define MARGIN 2
#   define CARD_START ( CARD_HEIGHT + 3 )
#else
    /* The screen is *small* */
#   define MARGIN 0
#   define CARD_START ( CARD_HEIGHT )
#endif

#include "solitaire_numbers.h"
#include "solitaire_suits.h"
#include "solitaire_suitsi.h"

#define NUMBER_HEIGHT (BMPHEIGHT_solitaire_numbers/13)
#define NUMBER_WIDTH  BMPWIDTH_solitaire_numbers
#define NUMBER_STRIDE BMPWIDTH_solitaire_numbers
#define SUIT_HEIGHT   (BMPHEIGHT_solitaire_suits/4)
#define SUIT_WIDTH    BMPWIDTH_solitaire_suits
#define SUIT_STRIDE   BMPWIDTH_solitaire_suits
#define SUITI_HEIGHT  (BMPHEIGHT_solitaire_suitsi/4)
#define SUITI_WIDTH   BMPWIDTH_solitaire_suitsi
#define SUITI_STRIDE  BMPWIDTH_solitaire_suitsi

#define draw_number( num, x, y ) \
    rb->lcd_mono_bitmap_part( solitaire_numbers, 0, num * NUMBER_HEIGHT, \
                              NUMBER_STRIDE, x, y, NUMBER_WIDTH, NUMBER_HEIGHT );

#define draw_suit( num, x, y ) \
    rb->lcd_mono_bitmap_part( solitaire_suits, 0, num * SUIT_HEIGHT, \
                              SUIT_STRIDE, x, y, SUIT_WIDTH, SUIT_HEIGHT );

#define draw_suiti( num, x, y ) \
    rb->lcd_mono_bitmap_part( solitaire_suitsi, 0, num * SUITI_HEIGHT, \
                              SUITI_STRIDE, x, y, SUITI_WIDTH, SUITI_HEIGHT );

#ifdef HAVE_LCD_COLOR
#include "solitaire_cardback.h"
#define CARDBACK_HEIGHT BMPHEIGHT_solitaire_cardback
#define CARDBACK_WIDTH  BMPWIDTH_solitaire_cardback
#endif

#if HAVE_LCD_COLOR
    static const fb_data colors[4] = {
        LCD_BLACK, LCD_RGBPACK(255, 0, 0), LCD_BLACK, LCD_RGBPACK(255, 0, 0)
    };
#elif LCD_DEPTH > 1
    static const fb_data colors[4] = {
        LCD_BLACK, LCD_BRIGHTNESS(127), LCD_BLACK, LCD_BRIGHTNESS(127)
    };
#endif

#define CONFIG_FILENAME "sol.cfg"

#define NOT_A_CARD -1

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

#define NOT_A_COL -1

/* background color */
#define BACKGROUND_COLOR LCD_RGBPACK(0,157,0)

#if LCD_DEPTH > 1 && !defined( LCD_WHITE )
#   define LCD_WHITE LCD_DEFAULT_BG
#endif

typedef struct
{
    signed char suit;
    signed char num;
    bool known : 1;
    bool used : 1; /* this is what is used when dealing cards */
    signed char next;
} card_t;


/**
 * LCD card drawing routines
 */

static void draw_cursor( int x, int y )
{
    rb->lcd_set_drawmode( DRMODE_COMPLEMENT );
    rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-2, CARD_HEIGHT-2 );
    rb->lcd_set_drawmode( DRMODE_SOLID );
}

/* Draw a card's border, select it if it's selected and draw the cursor
 * if the cursor is currently over the card */
static void draw_card_ext( int x, int y, bool selected, bool cursor )
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground( LCD_BLACK );
#endif
    /* draw a rectangle omiting the corner pixels, which is why we don't
     * use drawrect */
    rb->lcd_hline( x+1, x+CARD_WIDTH-2, y );
    rb->lcd_hline( x+1, x+CARD_WIDTH-2, y+CARD_HEIGHT-1 );
    rb->lcd_vline( x, y+1, y+CARD_HEIGHT-2 );
    rb->lcd_vline( x+CARD_WIDTH-1, y+1, y+CARD_HEIGHT-2 );

    if( selected )
    {
        rb->lcd_drawrect( x+1, y+1, CARD_WIDTH-2, CARD_HEIGHT-2 );
    }
    if( cursor )
    {
        draw_cursor( x, y );
    }
}

/* Draw a card's inner graphics */
static void draw_card( card_t *card, int x, int y,
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
    rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-2, CARD_HEIGHT-2 );
#if LCD_DEPTH == 1
    rb->lcd_set_drawmode( DRMODE_SOLID );
#endif
#endif
    if( card->known )
    {
#ifdef HAVE_LCD_COLOR
        /* On Color LCDs we have a card back so we only need to clear
         * the card area when it's known*/
        rb->lcd_set_foreground( LCD_WHITE );
        rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-2, CARD_HEIGHT-2 );
#endif

#if LCD_DEPTH > 1
        rb->lcd_set_foreground( colors[card->suit] );
#endif
        if( leftstyle )
        {
#if MARGIN > 0
            draw_suit( card->suit, x+1, y+2+NUMBER_HEIGHT );
            draw_number( card->num, x+1, y+1 );
#else
            draw_suit( card->suit, x+1, y+NUMBER_HEIGHT );
            draw_number( card->num, x+1, y );
#endif
        }
        else
        {
#if MARGIN > 0
            draw_suit( card->suit, x+2+NUMBER_WIDTH, y+1 );
#else
            draw_suit( card->suit, x+1+NUMBER_WIDTH, y+1 );
#endif
            draw_number( card->num, x+1, y+1 );
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
    rb->lcd_fillrect( x+1, y+1, CARD_WIDTH-2, CARD_HEIGHT-2 );
#if LCD_DEPTH == 1
    rb->lcd_set_drawmode( DRMODE_SOLID );
#endif

#if LCD_DEPTH > 1
        rb->lcd_set_foreground( colors[s] );
#endif

    draw_suiti( s, x+(CARD_WIDTH-SUITI_WIDTH)/2,
                y+(CARD_HEIGHT-SUITI_HEIGHT)/2 );

    draw_card_ext( x, y, false, cursor );
}

/**
 * Help
 */

static void init_help(void)
{
    int lines = 0;
    int w_space, w, h;
    int para_len;
    char *para = helptext;

    rb->lcd_getstringsize(" ", &w_space, &h);
    displaylines = LCD_HEIGHT / h;
    para_len = rb->strlen(para);

    while (para_len)
    {   
        bool first = true;
        int x = 0;
        char *next, *store;

        next = rb->strtok_r(para, " ", &store);
        while (next)
        {
            rb->lcd_getstringsize(next, &w, NULL);
            if (!first)
            {
                if (x + w > LCD_WIDTH)
                {
                    lines++;
                    x = 0;
                }
                else
                    next[-1] = ' ';  /* re-concatenate string */
            }
            else
                first = false;

            x += w + w_space;
            next = rb->strtok_r(NULL, " ", &store);
        }

        lines++;
        para += para_len + 1;
        para_len = rb->strlen(para);
    }
    helplines = lines;
}

enum help { HELP_QUIT, HELP_USB };

/* help for the not so intuitive interface */
enum help solitaire_help( void )
{
    int start = 0;
    int button;
    int lastbutton = BUTTON_NONE;
    bool fixed = (displaylines >= helplines);

    while( true )
    {
        char *text = helptext;
        int line  = fixed ? (displaylines - helplines) / 2 : 0;
        int i;

        rb->lcd_clear_display();

        for (i = 0; i < start + displaylines; i++)
        {
            if (i >= start)
                rb->lcd_puts(0, line++, text);
            text += rb->strlen(text) + 1;
        }
        rb->lcd_update();

        button = rb->button_get( true );
        switch( button )
        {
            case SOL_UP:
#ifdef SOL_UP_PRE
                if( lastbutton != SOL_UP_PRE )
                    break;
#else
            case SOL_UP|BUTTON_REPEAT:
#endif
                if (!fixed && start > 0)
                    start--;
                break;

            case SOL_DOWN:
#ifdef SOL_DOWN_PRE
                if( lastbutton != SOL_DOWN_PRE )
                    break;
#else
            case SOL_DOWN|BUTTON_REPEAT:
#endif
                if (!fixed && start < helplines - displaylines)
                    start++;
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
 */
 
#define CFGFILE_VERSION 0

/* introduce a struct if there's more than one setting */
int draw_type_disk = 0;
int draw_type;  

static struct configdata config[] = {
   { TYPE_INT, 0, 1, &draw_type_disk, "draw_type", NULL, NULL }
};


char draw_option_string[32];

static void create_draw_option_string(void)
{
    if (draw_type == 0)
        rb->strcpy(draw_option_string, "Draw Three Cards");
    else
        rb->strcpy(draw_option_string, "Draw One Card");
}

void solitaire_init(void);

/* menu return codes */
enum { MENU_RESUME, MENU_QUIT, MENU_USB };

int solitaire_menu(bool in_game)
{
    int m;
    int result = -1;
    int i = 0;

    struct menu_item items[4];

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif

    if (in_game)
    {
        items[i++].desc = "Resume Game";
        items[i++].desc = "Restart Game";
    }
    else
    {
        items[i++].desc = "Start Game";
        items[i++].desc = draw_option_string;
    }
    items[i++].desc = "Help";
    items[i++].desc = "Quit";

    create_draw_option_string();
    m = rb->menu_init(items, i, NULL, NULL, NULL, NULL);
    while (result < 0)
    {
        switch (rb->menu_show(m))
        {
            case MENU_SELECTED_EXIT:
                result = MENU_RESUME;
                break;

            case MENU_ATTACHED_USB:
                result = MENU_USB;
                break;

            case 0:
                result = MENU_RESUME;
                break;

            case 1:
                if (in_game)
                {
                    solitaire_init();
                    result = MENU_RESUME;
                }
                else
                {
                    draw_type = (draw_type + 1) % 2;
                    create_draw_option_string();
                }
                break;

            case 2:
                rb->lcd_setmargins(0, 0);
                if (solitaire_help() == HELP_USB)
                    result = MENU_USB;
                break;

            case 3:
                result = MENU_QUIT;
                break;
        }
    }
    rb->menu_exit(m);
    rb->lcd_setmargins(0, 0);
    return result;
}

/**
 * Global variables
 */

/* player's cursor */
int cur_card;
/* player's cursor column num */
int cur_col;

/* selected card */
int sel_card;

/* the deck */
card_t deck[ NUM_CARDS ];

/* the remaining cards */
/* first card of the remains' stack */
int rem;
/* upper visible card from the remains' stack */
int cur_rem;
/* number of cards drawn from the remains stack - 1 */
int count_rem;
/* number of cards per draw of the remains' stack */
int cards_per_draw;

/* the 7 game columns */
int cols[COL_NUM];
/* the 4 final stacks */
int stacks[SUITS];

/**
 * Card handling routines
 */

int next_random_card( card_t *deck )
{
    int i,r;

    r = rb->rand()%(NUM_CARDS)+1;
    i = 0;

    while( r>0 )
    {
        i = (i + 1)%(NUM_CARDS);
        if( !deck[i].used ) r--;
    }

    deck[i].used = true;

    return i;
}


/* initialize the game */
void solitaire_init( void )
{

    int c;
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
            card.known = true;
            card.used = false;
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
                deck[c].known = false;
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

    count_rem=-1;
}

/* find the column number in which 'card' can be found */
int find_card_col( int card )
{
    int i;
    int c;

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
int find_prev_card( int card ){
    int i;

    for( i=0; i < NUM_CARDS; i++ )
    {
        if( deck[i].next == card ) return i;
    }

    return NOT_A_CARD;
}

/* find the last card of a given column */
int find_last_card( int col )
{
    int c;

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
        c = cur_rem;
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

enum move move_card( int dest_col, int src_card )
{
    /* the column on which to take src_card */
    int src_col;

    /* the last card of dest_col */
    int dest_card;

    /* the card under src_card */
    int src_card_prev;

    /* you can't move no card (at least, it doesn't have any consequence) */
    if( src_card == NOT_A_CARD ) return MOVE_NOT_OK;
    /* you can't put a card back on the remains' stack */
    if( dest_col == REM_COL ) return MOVE_NOT_OK;
    /* you can't move an unknown card */
    if( !deck[src_card].known ) return MOVE_NOT_OK;

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
        /* ... or check if the cards follow one another and have
         * different colorsuit */
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
            /* this is not a winning combination */
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
        }
        /* if src card is not the first card from the stack */
        else
        {
            deck[src_card_prev].next = deck[src_card].next;
        }
        deck[src_card].next = NOT_A_CARD;
        cur_rem = src_card_prev;
        count_rem--;
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

enum { SOLITAIRE_WIN, SOLITAIRE_QUIT, SOLITAIRE_USB };

/**
 * Bouncing cards at the end of the game
 */
 
#define BC_ACCEL   ((1<<16)*LCD_HEIGHT/128)
#define BC_MYSPEED (6*BC_ACCEL)
#define BC_MXSPEED (6*LCD_HEIGHT/128)

int bouncing_cards( void )
{
    int i, j, x, vx, y, fp_y, fp_vy, button;

    /* flush the button queue */
    while( ( button = rb->button_get( false ) ) != BUTTON_NONE )
    {
        if( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
            return SOLITAIRE_USB;
    }

    /* fun stuff :) */
    for( i = CARDS_PER_SUIT-1; i>=0; i-- )
    {
        for( j = 0; j < SUITS; j++ )
        {
            x = LCD_WIDTH-(CARD_WIDTH*4+4+MARGIN)+CARD_WIDTH*j+j+1;
            fp_y = MARGIN<<16;

#if LCD_WIDTH > 200
            vx = rb->rand() % (4*BC_MXSPEED/3-2) - BC_MXSPEED;
            if( vx >= -1 )
                vx += 3;
#else
            vx = rb->rand() % (4*BC_MXSPEED/3) - BC_MXSPEED;
            if( vx >= 0 )
                vx++;
#endif

            fp_vy = -rb->rand() % BC_MYSPEED;

            while( x < LCD_WIDTH && x + CARD_WIDTH > 0 )
            {
                fp_vy += BC_ACCEL;
                x += vx;
                fp_y += fp_vy;
                if( fp_y >= (LCD_HEIGHT-CARD_HEIGHT) << 16 )
                {
                    fp_vy = -fp_vy*4/5;
                    fp_y = (LCD_HEIGHT-CARD_HEIGHT) << 16;
                }
                y = fp_y >> 16;
                draw_card( &deck[j*CARDS_PER_SUIT+i], x, y,
                           false, false, false );
                rb->lcd_update_rect( x<0?0:x, y<0?0:y,
                                     CARD_WIDTH, CARD_HEIGHT );

                button = rb->button_get_w_tmo( 2 );
                if( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
                    return SOLITAIRE_USB;
                if( button == SOL_QUIT || button == SOL_MOVE )
                    return SOLITAIRE_WIN;
            }
        }
    }
    return SOLITAIRE_WIN;
}

/**
 * The main game loop
 */

int solitaire( void )
{

    int i,j;
    int button, lastbutton = 0;
    int c,h,prevcard;
    int biggest_col_length;

    rb->srand( *rb->current_tick );
    switch( solitaire_menu(false) )
    {
        case MENU_QUIT:
            return SOLITAIRE_QUIT;

        case MENU_USB:
            return SOLITAIRE_USB;
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
                if( deck[c].known ) j += 2;
                else j ++;
                c = deck[c].next;
            }
            if( j > biggest_col_length ) biggest_col_length = j;
        }

        /* check if there are cards remaining in the game. */
        /* if there aren't any, that means you won :) */
        if( biggest_col_length == 0 && rem == NOT_A_CARD )
        {
            rb->splash( HZ, true, "You Won :)" );
            return bouncing_cards();
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
                        draw_cursor( MARGIN+i*((LCD_WIDTH-2*MARGIN)/COL_NUM),
                                     j+1 );
                    }
                    break;
                }

                draw_card( &deck[c], MARGIN+i*((LCD_WIDTH-2*MARGIN)/COL_NUM),
                           j+1, c == sel_card, c == cur_card, false );

                h = c;
                c = deck[c].next;
                if( c == NOT_A_CARD ) break;

                /* This is where we change the spacing between cards so that
                 * they don't overflow out of the LCD */
                if( h == cur_card )
                    j += SUIT_HEIGHT+2;
                else if( deck[h].known )
                    j += min( SUIT_HEIGHT+2,
                 2*(LCD_HEIGHT - CARD_START - CARD_HEIGHT)/biggest_col_length );
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
                draw_card( &deck[c],
                           LCD_WIDTH-(CARD_WIDTH*4+4+MARGIN)+CARD_WIDTH*i+i+1,
                           MARGIN,
                           c == sel_card, cur_col == STACKS_COL + i, false );
            }
            else
            {
                draw_empty_stack( i,
                           LCD_WIDTH-(CARD_WIDTH*4+4+MARGIN)+CARD_WIDTH*i+i+1,
                           MARGIN, cur_col == STACKS_COL + i );
            }
        }

        /* draw the remains */
        if( rem != NOT_A_CARD &&
            ( cur_rem == NOT_A_CARD || deck[cur_rem].next != NOT_A_CARD ) )
        {
            /* gruik ! (we want to display a card back) */
            deck[rem].known = false;
            draw_card( &deck[rem], MARGIN, MARGIN, false, false, false );
            deck[rem].known = true;
        }

        if( rem != NOT_A_CARD && cur_rem != NOT_A_CARD )
        {
            if( count_rem < 0 )
            {
                prevcard = rem;
                count_rem = 0;
                while( prevcard != cur_rem && count_rem < cards_per_draw-1 )
                {
                    prevcard = deck[prevcard].next;
                    count_rem++;
                }
            }
            prevcard = cur_rem;
            j = CARD_WIDTH+2*MARGIN+1;
            for( i = 0; i < count_rem; i++ )
                prevcard = find_prev_card(prevcard);
            for( i = 0; i <= count_rem; i++ )
            {
                draw_card( &deck[prevcard], j,
                           MARGIN, sel_card == prevcard,
                           cur_card == prevcard, i < count_rem );
                prevcard = deck[prevcard].next;
                j += NUMBER_WIDTH+2;
            }
        }
        if( ( cur_rem == NOT_A_CARD || rem == NOT_A_CARD )
              && cur_col == REM_COL )
        {
            draw_cursor( CARD_WIDTH+2*MARGIN+1, MARGIN );
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
#else
            case SOL_DOWN|BUTTON_REPEAT:
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
                    while( !deck[ cur_card].known
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
#else
            case SOL_UP|BUTTON_REPEAT:
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
                         && !deck[cur_card].known );
                break;

            /* Try to put card under cursor on one of the stacks */
            case SOL_CUR2STACK:
#ifdef SOL_CUR2STACK_PRE
                if( lastbutton != SOL_CUR2STACK_PRE )
                    break;
#endif
                move_card( deck[cur_card].suit + STACKS_COL, cur_card );
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
                            && !deck[cur_card].known )
                        {
                            /* reveal a hidden card */
                            deck[cur_card].known = true;
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
                    if( move_card( deck[sel_card].suit + COL_NUM, sel_card )
                        == MOVE_OK && cur_col == REM_COL )
                    {
                        cur_card = cur_rem;
                    }
                    sel_card = NOT_A_CARD;
                }
                else
                {
                    /* try moving cards */
                    move_card( cur_col, sel_card );
                }
                break;

            /* If the card on the top of the remains can be put where
             * the cursor is, go ahead */
            case SOL_REM2CUR:
#ifdef SOL_REM2CUR_PRE
                if( lastbutton != SOL_REM2CUR_PRE )
                    break;
#endif
                move_card( cur_col, cur_rem );
                break;

            /* If the card on top of the remains can be put on one
             * of the stacks, do so */
            case SOL_REM2STACK:
#ifdef SOL_REM2STACK_PRE
                if( lastbutton != SOL_REM2STACK_PRE )
                    break;
#endif
                move_card( deck[cur_rem].suit + COL_NUM, cur_rem );
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
                    sel_card = cur_rem;
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
                    count_rem = -1;
                    /* draw new cards form the remains of the deck */
                    if( cur_rem == NOT_A_CARD )
                    {
                        /*if the cursor card is null*/
                        cur_rem = rem;
                        i = cards_per_draw - 1;
                        count_rem++;
                    }
                    else
                    {
                        i = cards_per_draw;
                    }

                    while( i > 0 && deck[cur_rem].next != NOT_A_CARD )
                    {
                        cur_rem = deck[cur_rem].next;
                        i--;
                        count_rem++;
                    }
                    /* test if any cards are really left on
                     * the remains' stack */
                    if( i == cards_per_draw )
                    {
                        cur_rem = NOT_A_CARD;
                        count_rem = -1;
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
                switch( solitaire_menu(true) )
                {
                    case MENU_QUIT:
                        return SOLITAIRE_QUIT;

                    case MENU_USB:
                        return SOLITAIRE_USB;
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

    configfile_init(rb);
    configfile_load(CONFIG_FILENAME, config, 1, 0);
    draw_type = draw_type_disk;
    
    init_help();

    /* play the game :)
     * Keep playing if a game was won (that means display the menu after
     * winning instead of quiting) */
    while( ( result = solitaire() ) == SOLITAIRE_WIN );
    
    if (draw_type != draw_type_disk)
    {
        draw_type_disk = draw_type;
        configfile_save(CONFIG_FILENAME, config, 1, 0);
    }

    /* Exit the plugin */
    return ( result == SOLITAIRE_USB ) ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}

#endif
