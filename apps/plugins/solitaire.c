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
Solitaire by dionoea

use arrows to move the cursor
use ON to select cards in the columns, move cards inside the columns,
    reveal hidden cards, ...
use PLAY to move a card from the remains' stack to the top of the cursor
use F1 to put card under cursor on one of the 4 final color stacks
use F2 to un-select card if a card was selected, else draw 3 new cards
    out of the remains' stack
use F3 to put card on top of the remains' stack on one of the 4 final color
    stacks

*****************************************************************************/

#include "plugin.h"
#include "button.h"
#include "lcd.h"

#ifdef HAVE_LCD_BITMAP

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;

#define min(a,b) (a<b?a:b)

#define HELP_CASE( key )    case BUTTON_ ## key: \
                                rb->splash(HZ*1, true, # key " : " HELP_BUTTON_ ## key); \
                                break;

#define HELP_BUTTON_UP "Move the cursor up in the column."
#define HELP_BUTTON_DOWN "Move the cursor down in the column."
#define HELP_BUTTON_LEFT "Move the cursor to the previous column."
#define HELP_BUTTON_RIGHT "Move the cursor to the next column."
#define HELP_BUTTON_F1 "Put the card under the cursor on one of the 4 final color stacks."
#define HELP_BUTTON_F2 "Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_BUTTON_F3 "Put the card on top of the remains' stack on one of the 4 final color stacks."
#define HELP_BUTTON_PLAY "Put the card on top of the remains' stack on top of the cursor."
#define HELP_BUTTON_ON "Select cards in the columns, Move cards inside the columns, reveal hidden cards ..."

static unsigned char colors[4][8] = {
/* Spades */
    {0x00, /* ........ */
     0x18, /* ...O.... */
     0x1c, /* ..OOO... */
     0x3e, /* .OOOOO.. */
     0x1c, /* .OOOOO.. */
     0x18, /* ...O.... */
     0x00, /* ........ */
     0x00},/* ........ */
/* Hearts */
    {0x00, /* ........ */
     0x0c, /* ..O.O... */
     0x1e, /* .OOOOO.. */
     0x3c, /* .OOOOO.. */
     0x1e, /* ..OOO... */
     0x0c, /* ...O.... */
     0x00, /* ........ */
     0x00},/* ........ */
/* Clubs */
    {0x00, /* ........ */
     0x18, /* ..OOO... */
     0x0a, /* ...O.... */
     0x3e, /* .OOOOO.. */
     0x0a, /* .O.O.O.. */
     0x18, /* ...O.... */
     0x00, /* ........ */
     0x00},/* ........ */
/* Diamonds */
    {0x00, /* ........ */
     0x08, /* ...O.... */
     0x1c, /* ..OOO... */
     0x3e, /* .OOOOO.. */
     0x1c, /* ..OOO... */
     0x08, /* ...O.... */
     0x00, /* ........ */
     0x00} /* ........ */
};

static unsigned char numbers[13][8] = {
/* Ace */
    {0x00, /* ........ */
     0x38, /* ...O.... */
     0x14, /* ..O.O... */
     0x12, /* .O...O.. */
     0x14, /* .OOOOO.. */
     0x38, /* .O...O.. */
     0x00, /* ........ */
     0x00},/* ........ */
/* 2 */
    {0x00, /* ........ */
     0x24, /* ..OOO... */
     0x32, /* .O...O.. */
     0x32, /* ....O... */
     0x2a, /* ..OO.... */
     0x24, /* .OOOOO.. */
     0x00, /* ........ */
     0x00},/* ........ */
/* 3 */
    {0x00, /* ........ */
     0x22, /* .OOOO... */
     0x2a, /* .....O.. */
     0x2a, /* ..OOO... */
     0x2a, /* .....O.. */
     0x14, /* .OOOO... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 4 */
    {0x00, /* ........ */
     0x10, /* ....O... */
     0x18, /* ...O.... */
     0x34, /* ..O..... */
     0x12, /* .OOOOO.. */
     0x10, /* ...O.... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 5 */
    {0x00, /* ........ */
     0x2e, /* .OOOOO.. */
     0x2a, /* .O...... */
     0x2a, /* .OOOO... */
     0x2a, /* .....O.. */
     0x12, /* .OOOO... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 6 */
    {0x00, /* ........ */
     0x1c, /* ..OOO... */
     0x2a, /* .O...... */
     0x2a, /* .OOOO... */
     0x2a, /* .O...O.. */
     0x10, /* ..OOO... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 7 */
    {0x00, /* ........ */
     0x22, /* .OOOOO.. */
     0x12, /* ....O... */
     0x0a, /* ...O.... */
     0x06, /* ..O..... */
     0x02, /* .O...... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 8 */
    {0x00, /* ........ */
     0x14, /* ..OOO... */
     0x2a, /* .O...O.. */
     0x2a, /* ..OOO... */
     0x2a, /* .O...O.. */
     0x14, /* ..OOO... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 9 */
    {0x00, /* ........ */
     0x04, /* ..OOO... */
     0x2a, /* .O...O.. */
     0x2a, /* ..OOOO.. */
     0x2a, /* .....O.. */
     0x1c, /* ..OOO... */
     0x00, /* ........ */
     0x00},/* ........ */
/* 10 */
    {0x00, /* ........ */
     0x3e, /* .O..O... */
     0x00, /* .O.O.O.. */
     0x1c, /* .O.O.O.. */
     0x22, /* .O.O.O.. */
     0x1c, /* .O..O... */
     0x00, /* ........ */
     0x00},/* ........ */
/* Jack */
    {0x00, /* ........ */
     0x12, /* .OOOOO.. */
     0x22, /* ...O.... */
     0x1e, /* ...O.... */
     0x02, /* .O.O.... */
     0x02, /* ..O..... */
     0x00, /* ........ */
     0x00},/* ........ */
/* Queen */
    {0x00, /* ........ */
     0x1c, /* ..OOO... */
     0x22, /* .O...O.. */
     0x32, /* .O...O.. */
     0x22, /* .O.O.O.. */
     0x1c, /* ..OOO... */
     0x00, /* ........ */
     0x00},/* ........ */
/* King */
    {0x00, /* ........ */
     0x3e, /* .O...O.. */
     0x08, /* .O..O... */
     0x08, /* .OOO.... */
     0x14, /* .O..O... */
     0x22, /* .O...O.. */
     0x00, /* ........ */
     0x00} /* ........ */
};

#define NOT_A_CARD 255

/* number of cards per color */
#define CARDS_PER_COLOR 13

/* number of colors */
#define COLORS 4

/* number of columns */
#define COL_NUM 7

/* number of cards that are drawn on the remains' stack (by pressing F2) */
#define CARDS_PER_DRAW 3

/* size of a card on the screen */
#define CARD_WIDTH 14
#define CARD_HEIGHT 10

typedef struct card {
    unsigned char color : 2;
    unsigned char num : 4;
    unsigned char known : 1;
    unsigned char used : 1;/* this is what is used when dealing cards */
    unsigned char next;
} card;

unsigned char next_random_card(card *deck){
    unsigned char i,r;
    
    r = rb->rand()%(COLORS * CARDS_PER_COLOR)+1;
    i = 0;
    
    while(r>0){
        i = (i + 1)%(COLORS * CARDS_PER_COLOR);
        if(!deck[i].used) r--;
    }

    deck[i].used = 1;

    return i;
}

/* help for the not so intuitive interface */
void solitaire_help(void)
{
    rb->lcd_clear_display();

    rb->lcd_putsxy(0, 0, "Press a key to see");
    rb->lcd_putsxy(0, 7, "it's role.");
    rb->lcd_putsxy(0, 21, "Press OFF to");
    rb->lcd_putsxy(0, 28, "return to menu");

    rb->lcd_update();
    
    while(1){
    
        switch(rb->button_get(true)){
            HELP_CASE( UP );
            HELP_CASE( DOWN );
            HELP_CASE( LEFT );
            HELP_CASE( RIGHT );
            HELP_CASE( F1 );
            HELP_CASE( F2 );
            HELP_CASE( F3 );
            HELP_CASE( PLAY );
            HELP_CASE( ON );

            case BUTTON_OFF:
                return;
        }
    }
}

/* menu return codes */
#define MENU_RESUME 0
#define MENU_RESTART 1
#define MENU_HELP 2
#define MENU_QUIT 3

/* menu item number */
#define MENU_LENGTH 4

/* different menu behaviors */
#define MENU_BEFOREGAME 0
#define MENU_DURINGGAME 1

/* the menu */
/* text displayed changes depending on the 'when' parameter */
int solitaire_menu(unsigned char when) {
    
    static char menu[2][MENU_LENGTH][13] = 
        { { "Start Game",
            "",
            "Help",
            "Quit" },
          { "Resume Game",
            "Restart Game",
            "Help",
            "Quit"}
        };
    
    int i;
    int cursor=0;
    
    if(when!=MENU_BEFOREGAME && when!=MENU_DURINGGAME)
        when = MENU_DURINGGAME;
    
    while(1){
        
        rb->lcd_clear_display();

        rb->lcd_putsxy(20, 1, "Solitaire");
        
        for(i = 0; i<MENU_LENGTH; i++){
            rb->lcd_putsxy(1, 17+9*i, menu[when][i]);
            if(cursor == i)
                rb->lcd_invertrect(0,17-1+9*i, LCD_WIDTH, 9);
        }

        rb->lcd_update();

        switch(rb->button_get(true)){
            case BUTTON_UP:
                cursor = (cursor + MENU_LENGTH - 1)%MENU_LENGTH;
                break;

            case BUTTON_DOWN:
                cursor = (cursor + 1)%MENU_LENGTH;
                break;

            case BUTTON_LEFT:
                return MENU_RESUME;

            case BUTTON_PLAY:
            case BUTTON_RIGHT:
                switch(cursor){
                    case MENU_RESUME:
                    case MENU_RESTART:
                    case MENU_QUIT:
                        return cursor;

                    case MENU_HELP:
                        solitaire_help();
                        break;
                }
            
            case BUTTON_F1:
            case BUTTON_F2:
            case BUTTON_F3:
                rb->splash(HZ, true, "Solitaire for Rockbox by dionoea");
                break;

            case BUTTON_OFF:
                return MENU_QUIT;

            default:
                break;
        }
    }
}

/* player's cursor */
unsigned char cur_card;
/* player's cursor column num */
unsigned char cur_col;

/* selected card */
unsigned char sel_card;

/* the deck */
card deck[COLORS * CARDS_PER_COLOR];

/* the remaining cards */
unsigned char rem;
unsigned char cur_rem;

/* the 7 game columns */
unsigned char cols[COL_NUM];

/* the 4 final color stacks */
unsigned char stacks[COLORS];

/* initialize the game */
void solitaire_init(void){
    unsigned char c;
    int i,j;

    /* init deck */
    for(i=0;i<COLORS;i++){
        for(j=0;j<CARDS_PER_COLOR;j++){
            deck[i*CARDS_PER_COLOR+j].color = i;
            deck[i*CARDS_PER_COLOR+j].num = j;
            deck[i*CARDS_PER_COLOR+j].known = 0;
            deck[i*CARDS_PER_COLOR+j].used = 0;
            deck[i*CARDS_PER_COLOR+j].next = NOT_A_CARD;
        }
    }
    
    /* deal the cards ... */
    /* ... in the columns */
    for(i=0; i<COL_NUM; i++){
        c = NOT_A_CARD;
        for(j=0; j<=i; j++){
            if(c == NOT_A_CARD){
                cols[i] = next_random_card(deck);
                c = cols[i];
            } else {
                deck[c].next = next_random_card(deck);
                c = deck[c].next;
            }
            if(j==i) deck[c].known = 1;
        }
    }

    /* ... shuffle what's left of the deck */
    rem = next_random_card(deck);
    c = rem;
    
    for(i=1; i<COLORS * CARDS_PER_COLOR - COL_NUM * (COL_NUM + 1)/2; i++){
        deck[c].next = next_random_card(deck);
        c = deck[c].next;
    }

    /* we now finished dealing the cards. The game can start ! (at last) */
    
    /* init the stack */
    for(i = 0; i<COL_NUM;i++){
        stacks[i] = NOT_A_CARD;
    }
    
    /* the cursor starts on upper left card */
    cur_card = cols[0];
    cur_col = 0;
    
    /* no card is selected */
    sel_card = NOT_A_CARD;
    
    /* init the remainder */
    cur_rem = NOT_A_CARD;
} 


/* the game */
void solitaire(void){
    
    int i,j;
    unsigned char c;
    int biggest_col_length;
    
    if(solitaire_menu(MENU_BEFOREGAME) == MENU_QUIT) return;
    solitaire_init();
   
    while(true){
        
        rb->lcd_clear_display();

        /* get the biggest column length so that display can be "optimized" */
        biggest_col_length = 0;
        
        for(i=0;i<COL_NUM;i++){
            j = 0;
            c = cols[i];
            while(c != NOT_A_CARD){
                j++;
                c = deck[c].next;
            }
            if(j>biggest_col_length) biggest_col_length = j;
        }
        
        /* check if there are cards remaining in the game. */
        /* if there aren't any, that means you won :) */
        if(biggest_col_length == 0 && rem == NOT_A_CARD){
            rb->splash(HZ*2, true, "You Won :)");
            return;
        }
        
        /* draw the columns */
        for(i=0;i<COL_NUM;i++){
            c = cols[i];
            j = 0;
            while(true){
                if(c==NOT_A_CARD) break;
                /* clear the card's spot */
                rb->lcd_clearrect(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM,
                                  j+1, CARD_WIDTH, CARD_HEIGHT-1);
                /* known card */
                if(deck[c].known) {
                    rb->lcd_bitmap(numbers[deck[c].num],
                                   i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+1,
                                   j, 8, 8, true);
                    rb->lcd_bitmap(colors[deck[c].color],
                                   i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+7,
                                   j, 8, 8, true);
                }
                /* draw top line of the card */
                rb->lcd_drawline(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+1,
                                 j,i*(LCD_WIDTH - CARD_WIDTH)/
                                 COL_NUM+CARD_WIDTH-1,j);
                /* selected card */
                if(c == sel_card && sel_card != NOT_A_CARD){
                    rb->lcd_drawrect(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+1,
                                     j+1, CARD_WIDTH-1, CARD_HEIGHT-1);
                }
                /* cursor (or not) */
                if(c == cur_card){
                    rb->lcd_invertrect(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+1,
                                       j+1, CARD_WIDTH-1, CARD_HEIGHT-1);
                    /* go to the next card */
                    c = deck[c].next;
                    if(c == NOT_A_CARD)
                        break;
                    j += CARD_HEIGHT - 2;
                }
                else {
                    /* go to the next card */
                    c = deck[c].next;
                    if(c == NOT_A_CARD)
                        break;
                    j += min(CARD_HEIGHT - 2,
                             (LCD_HEIGHT - CARD_HEIGHT)/biggest_col_length);
                }
            }
            /* draw line to the left of the column */
            rb->lcd_drawline(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM,
                             1,i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM,
                             j+CARD_HEIGHT-1);
            /* draw line to the right of the column */
            rb->lcd_drawline(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+CARD_WIDTH,
                             1,i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+CARD_WIDTH,
                             j+CARD_HEIGHT-1);
            /* draw bottom of the last card */
            rb->lcd_drawline(i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+1,
                             j+CARD_HEIGHT,i*(LCD_WIDTH - CARD_WIDTH)/COL_NUM+
                             CARD_WIDTH-1,j+CARD_HEIGHT);
        }

        /* draw the stacks */
        for(i=0; i<COLORS; i++){
            c = stacks[i];
            if(c!=NOT_A_CARD){
                while(deck[c].next != NOT_A_CARD){
                    c = deck[c].next;
                }
            }
            if(c != NOT_A_CARD) {
                rb->lcd_bitmap(numbers[deck[c].num], LCD_WIDTH - CARD_WIDTH+1,
                               i*CARD_HEIGHT, 8, 8, true);
            }
            rb->lcd_bitmap(colors[i], LCD_WIDTH - CARD_WIDTH+7,
                           i*CARD_HEIGHT, 8, 8, true);
            rb->lcd_drawline(LCD_WIDTH - CARD_WIDTH+1,
                             i*CARD_HEIGHT,LCD_WIDTH - 1,i*CARD_HEIGHT);
            rb->lcd_drawline(LCD_WIDTH - CARD_WIDTH+1,
                             (i+1)*CARD_HEIGHT,LCD_WIDTH - 1,
                             (i+1)*CARD_HEIGHT);
        }

        /* draw the remains */
        rb->lcd_drawline(LCD_WIDTH - CARD_WIDTH+1,
                         LCD_HEIGHT-CARD_HEIGHT-1,
                         LCD_WIDTH - 1,LCD_HEIGHT-CARD_HEIGHT-1);
        rb->lcd_drawline(LCD_WIDTH - CARD_WIDTH+1,
                         LCD_HEIGHT-1,LCD_WIDTH - 1,LCD_HEIGHT-1);
        if(cur_rem != NOT_A_CARD){
            rb->lcd_bitmap(numbers[deck[cur_rem].num],
                           LCD_WIDTH - CARD_WIDTH+1, LCD_HEIGHT-CARD_HEIGHT,
                           8, 8, true);
             rb->lcd_bitmap(colors[deck[cur_rem].color],
                            LCD_WIDTH - CARD_WIDTH+7,
                            LCD_HEIGHT-CARD_HEIGHT, 8, 8, true);
        }

        rb->lcd_update();
        
        /* what to do when a key is pressed ... */
        switch(rb->button_get(true)){
            
            /* move cursor to the last card of the previous column */
            case BUTTON_RIGHT:
                cur_col = (cur_col+1)%COL_NUM;
                cur_card = cols[cur_col];
                if(cur_card != NOT_A_CARD){
                    while(deck[cur_card].next != NOT_A_CARD){
                        cur_card = deck[cur_card].next;
                    }
                }
                break;
            
                /* move cursor to the last card of the next column */
            case BUTTON_LEFT:
                cur_col = (cur_col + COL_NUM - 1)%COL_NUM;
                cur_card = cols[cur_col];
                if(cur_card != NOT_A_CARD){
                    while(deck[cur_card].next != NOT_A_CARD){
                        cur_card = deck[cur_card].next;
                    }
                }
                break;
            
                /* move cursor to card that's bellow */
            case BUTTON_DOWN:
                if(cur_card == NOT_A_CARD) break;
                if(deck[cur_card].next != NOT_A_CARD){
                    cur_card = deck[cur_card].next;
                } else {
                    cur_card = cols[cur_col];
                }
                break;
            
                /* move cursor to card that's above */
            case BUTTON_UP:
                if(cur_card == NOT_A_CARD) break;
                if(cols[cur_col] == cur_card){
                    while(deck[cur_card].next != NOT_A_CARD){
                        cur_card = deck[cur_card].next;
                    }
                } else {
                    c = cols[cur_col];
                    while(deck[c].next != cur_card){
                        c = deck[c].next;
                    }
                    cur_card = c;
                }
                break;
            
                /* Try to put card under cursor on one of the stacks */
            case BUTTON_F1:
                /* check if a card is selected */
                /* else there would be nothing to move on the stacks ! */
                if(cur_card != NOT_A_CARD){
                    /* find the last card in the color's stack and put it's number in 'c'. */
                    c = stacks[deck[cur_card].color];
                    if(c!=NOT_A_CARD){
                        while(deck[c].next!=NOT_A_CARD){
                            c = deck[c].next;
                        }
                    }
                    /* if 'c' isn't a card, that means that the stack is empty */
                    /* which implies that only an ace can be moved */
                    if(c == NOT_A_CARD){
                        /* check if the selected card is an ace */
                        /* we don't have to check if any card is in the *.next */
                        /* position since the ace is the last possible card */
                        if(deck[cur_card].num == 0){
                            /* remove 'cur_card' from any *.next postition ... */
                            /* ... by looking in the columns */
                            for(i=0;i<COL_NUM;i++){
                                if(cols[i]==cur_card) cols[i] = NOT_A_CARD;
                            }
                            /* ... and in the entire deck */
                            /* TODO : check if looking in the cols is really needed */
                            for(i=0;i<COLORS*CARDS_PER_COLOR;i++){
                                if(deck[i].next==cur_card) deck[i].next = NOT_A_CARD;
                            }
                            /* move cur_card on top of the stack */
                            stacks[deck[cur_card].color] = cur_card;
                            /* assign the card at the bottom of cur_col to cur_card */
                            cur_card = cols[cur_col];
                            if(cur_card != NOT_A_CARD){
                                while(deck[cur_card].next != NOT_A_CARD){
                                    cur_card = deck[cur_card].next;
                                }
                            }
                            /* clear the selection indicator */
                            sel_card = NOT_A_CARD;
                        }
                    }
                    /* the stack is not empty */
                    /* so we can move any card other than an ace */
                    /* we thus check that the card we are moving is the next on the stack and that it isn't under any card */
                    else if(deck[cur_card].num == deck[c].num + 1 &&
                            deck[cur_card].next == NOT_A_CARD) {
                        /* same as above */
                        for(i=0;i<COL_NUM;i++) {
                            if(cols[i]==cur_card)
                                cols[i] = NOT_A_CARD;
                        }
                        /* re same */
                        for(i=0;i<COLORS*CARDS_PER_COLOR;i++){
                            if(deck[i].next==cur_card)
                                deck[i].next = NOT_A_CARD;
                        }
                        /* ... */
                        deck[c].next = cur_card;
                        cur_card = cols[cur_col];
                        if(cur_card != NOT_A_CARD){
                            while(deck[cur_card].next != NOT_A_CARD){
                                cur_card = deck[cur_card].next;
                            }
                        }
                        sel_card = NOT_A_CARD;
                    }
                }
                break;
            
                /* Move cards arround, Uncover cards, ... */
            case BUTTON_ON:
                if(sel_card == NOT_A_CARD) {
                    if((cur_card != NOT_A_CARD?
                        deck[cur_card].next == NOT_A_CARD &&
                        deck[cur_card].known==0:0)) {
                        deck[cur_card].known = 1;
                    } else {
                        sel_card = cur_card;
                    }
                } else if(sel_card == cur_card) {
                    sel_card = NOT_A_CARD;
                } else if(cur_card != NOT_A_CARD){
                    if(deck[cur_card].num == deck[sel_card].num + 1 &&
                       (deck[cur_card].color + deck[sel_card].color)%2 == 1 ){
                        for(i=0;i<COL_NUM;i++){
                            if(cols[i]==sel_card)
                                cols[i] = NOT_A_CARD;
                        }
                        for(i=0;i<COLORS*CARDS_PER_COLOR;i++){
                            if(deck[i].next==sel_card)
                                deck[i].next = NOT_A_CARD;
                        }
                        deck[cur_card].next = sel_card;
                        sel_card = NOT_A_CARD;
                    }
                } else if(cur_card == NOT_A_CARD){
                    if(deck[sel_card].num == CARDS_PER_COLOR - 1){
                        for(i=0;i<COL_NUM;i++){
                            if(cols[i]==sel_card)
                                cols[i] = NOT_A_CARD;
                        }
                        for(i=0;i<COLORS*CARDS_PER_COLOR;i++){
                            if(deck[i].next==sel_card)
                                deck[i].next = NOT_A_CARD;
                        }
                        cols[cur_col] = sel_card;
                        sel_card = NOT_A_CARD;
                    }
                }
                break;
            
                /* If the card on the top of the remains can be put where */
                /* the cursor is, go ahead */
            case BUTTON_PLAY:
                /* check if a card is face up on the remains' stack */
                if(cur_rem != NOT_A_CARD){
                    /* if no card is selected, it means the col is empty */
                    /* thus, only a king can be moved */
                    if(cur_card == NOT_A_CARD){
                        /* check if selcted card is a king */
                        if(deck[cur_rem].num == CARDS_PER_COLOR - 1){
                            /* find the previous card on the remains' stack */
                            c = rem;
                            /* if the current card on the remains' stack */
                            /* is the first card of the stack, then ... */
                            if(c == cur_rem){
                                c = NOT_A_CARD;
                                rem = deck[cur_rem].next;
                            }
                            /* else ... */
                            else {
                                while(deck[c].next != cur_rem){
                                    c = deck[c].next;
                                }
                                deck[c].next = deck[cur_rem].next;
                            }
                            cols[cur_col] = cur_rem;
                            deck[cur_rem].next = NOT_A_CARD;
                            deck[cur_rem].known = 1;
                            cur_rem = c;
                        }
                    } else if(deck[cur_rem].num + 1 == deck[cur_card].num &&
                              (deck[cur_rem].color +
                               deck[cur_card].color)%2==1) {
                        c = rem;
                        if(c == cur_rem){
                            c = NOT_A_CARD;
                            rem = deck[cur_rem].next;
                        } else {
                            while(deck[c].next != cur_rem){
                                c = deck[c].next;
                            }
                            deck[c].next = deck[cur_rem].next;
                        }
                        deck[cur_card].next = cur_rem;
                        deck[cur_rem].next = NOT_A_CARD;
                        deck[cur_rem].known = 1;
                        cur_rem = c;
                    }
                }
                break;
            
                /* If the card on top of the remains can be put on one */
                /* of the stacks, do so */
            case BUTTON_F3:
                if(cur_rem != NOT_A_CARD){
                    if(deck[cur_rem].num == 0){
                        c = rem;
                        if(c == cur_rem){
                            c = NOT_A_CARD;
                            rem = deck[cur_rem].next;
                        } else {
                            while(deck[c].next != cur_rem){
                                c = deck[c].next;
                            }
                            deck[c].next = deck[cur_rem].next;
                        }
                        deck[cur_rem].next = NOT_A_CARD;
                        deck[cur_rem].known = 1;
                        stacks[deck[cur_rem].color] = cur_rem;
                        cur_rem = c;
                    } else {
                        
                        i = stacks[deck[cur_rem].color];
                        if(i==NOT_A_CARD) break;
                        while(deck[i].next != NOT_A_CARD){
                            i = deck[i].next;
                        }
                        if(deck[i].num + 1 == deck[cur_rem].num){
                            c = rem;
                            if(c == cur_rem){
                                c = NOT_A_CARD;
                                rem = deck[cur_rem].next;
                            } else {
                                while(deck[c].next != cur_rem){
                                    c = deck[c].next;
                                }
                                deck[c].next = deck[cur_rem].next;
                            }
                            deck[i].next = cur_rem;
                            deck[cur_rem].next = NOT_A_CARD;
                            deck[cur_rem].known = 1;
                            cur_rem = c;
                        }
                    }
                }
                break;   
            
                /* unselect selected card or ... */
                /* draw new cards from the remains of the deck */
            case BUTTON_F2:
                if(sel_card != NOT_A_CARD){
                    /* unselect selected card */
                    sel_card = NOT_A_CARD;
                } else if(rem != NOT_A_CARD) {
                    /* draw new cards form the remains of the deck */
                    if(cur_rem == NOT_A_CARD){
                        cur_rem = rem;
                        i = CARDS_PER_DRAW - 1;
                    } else {
                        i = CARDS_PER_DRAW;
                    }
                    while(i>0 && deck[cur_rem].next != NOT_A_CARD){
                        cur_rem = deck[cur_rem].next;
                        i--;
                    }
                    /* test if any cards are really left on the remains' stack */
                    if(i == CARDS_PER_DRAW){
                        cur_rem = NOT_A_CARD;
                    }
                }
                break;
            
                /* Show the menu */
            case BUTTON_OFF:
                switch(solitaire_menu(MENU_DURINGGAME)){
                    case MENU_QUIT:
                        return;

                    case MENU_RESTART:
                        solitaire_init();
                        break;
                }
        }
    }
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    /* plugin init */
    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;
    /* end of plugin init */
    
    /* Welcome to Solitaire ! */
    rb->splash(HZ*2, true, "Welcome to Solitaire !");
    
    /* play the game :) */
    solitaire();

    /* Exit the plugin */
    return PLUGIN_OK;
}

#endif
