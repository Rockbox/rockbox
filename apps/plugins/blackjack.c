/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2006 Tom Ross
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/            

#include "plugin.h"
#include "card_deck.h"
#include "card_back.h"

PLUGIN_HEADER

/* save files */
#define SCORE_FILE PLUGIN_GAMES_DIR "/blackjack.score"
#define SAVE_FILE  PLUGIN_GAMES_DIR "/blackjack.save"

#define NUM_SCORES LCD_HEIGHT/8-2

/* final game return status */
#define BJ_END  3
#define BJ_USB  2
#define BJ_QUIT 1
#define BJ_LOSE 0

#if CONFIG_KEYPAD == RECORDER_PAD
#define BJACK_START      BUTTON_ON
#define BJACK_QUIT       BUTTON_OFF
#define BJACK_MAX        (BUTTON_ON|BUTTON_UP)
#define BJACK_MIN        (BUTTON_ON|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_F1
#define BJACK_STAY       BUTTON_F2
#define BJACK_DOUBLEDOWN BUTTON_F3
#define BJACK_SCORES     BUTTON_RIGHT
#define BJACK_RESUME     BUTTON_PLAY
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define BJACK_START      BUTTON_MENU
#define BJACK_QUIT       BUTTON_OFF
#define BJACK_MAX        (BUTTON_MENU|BUTTON_UP)
#define BJACK_MIN        (BUTTON_MENU|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_LEFT
#define BJACK_STAY       BUTTON_RIGHT
#define BJACK_DOUBLEDOWN BUTTON_UP
#define BJACK_SCORES     BUTTON_UP
#define BJACK_RESUME     BUTTON_DOWN
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define BJACK_START      BUTTON_PLAY
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_MAX        (BUTTON_PLAY|BUTTON_SCROLL_UP)
#define BJACK_MIN        (BUTTON_PLAY|BUTTON_SCROLL_DOWN)
#define BJACK_HIT        BUTTON_PLAY
#define BJACK_STAY       BUTTON_FF
#define BJACK_DOUBLEDOWN BUTTON_REW
#define BJACK_SCORES     BUTTON_LEFT
#define BJACK_RESUME     BUTTON_RIGHT
#define BJACK_UP         BUTTON_SCROLL_UP
#define BJACK_DOWN       BUTTON_SCROLL_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BJACK_START      BUTTON_ON
#define BJACK_QUIT       BUTTON_OFF
#define BJACK_MAX        (BUTTON_ON|BUTTON_UP)
#define BJACK_MIN        (BUTTON_ON|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_ON
#define BJACK_STAY       BUTTON_REC
#define BJACK_DOUBLEDOWN BUTTON_SELECT
#define BJACK_SCORES     BUTTON_SELECT
#define BJACK_RESUME     BUTTON_MODE
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BJACK_START      BUTTON_SELECT
#define BJACK_QUIT       BUTTON_MENU
#define BJACK_MAX        (BUTTON_SELECT|BUTTON_SCROLL_FWD)
#define BJACK_MIN        (BUTTON_SELECT|BUTTON_SCROLL_BACK)
#define BJACK_HIT        BUTTON_SELECT
#define BJACK_STAY       BUTTON_RIGHT
#define BJACK_DOUBLEDOWN BUTTON_LEFT
#define BJACK_SCORES     BUTTON_RIGHT
#define BJACK_RESUME     BUTTON_PLAY
#define BJACK_UP         BUTTON_SCROLL_FWD
#define BJACK_DOWN       BUTTON_SCROLL_BACK
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define BJACK_START      BUTTON_PLAY
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_MAX        (BUTTON_PLAY|BUTTON_UP)
#define BJACK_MIN        (BUTTON_PLAY|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_SELECT
#define BJACK_STAY       BUTTON_REC
#define BJACK_DOUBLEDOWN BUTTON_PLAY
#define BJACK_SCORES     BUTTON_RIGHT
#define BJACK_RESUME     BUTTON_DOWN
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define BJACK_START      BUTTON_MODE
#define BJACK_QUIT       BUTTON_PLAY
#define BJACK_MAX        (BUTTON_EQ|BUTTON_UP)
#define BJACK_MIN        (BUTTON_EQ|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_EQ
#define BJACK_STAY       BUTTON_MODE
#define BJACK_DOUBLEDOWN BUTTON_SELECT
#define BJACK_SCORES     BUTTON_SELECT
#define BJACK_RESUME     (BUTTON_EQ|BUTTON_MODE)
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define BJACK_START      BUTTON_POWER
#define BJACK_QUIT       BUTTON_A
#define BJACK_MAX        BUTTON_VOL_UP
#define BJACK_MIN        BUTTON_VOL_DOWN
#define BJACK_HIT        BUTTON_VOL_UP
#define BJACK_STAY       BUTTON_VOL_DOWN
#define BJACK_DOUBLEDOWN BUTTON_SELECT
#define BJACK_SCORES     BUTTON_RIGHT
#define BJACK_RESUME     BUTTON_MENU
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define BJACK_START      BUTTON_SELECT
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_MAX        (BUTTON_REC|BUTTON_UP)
#define BJACK_MIN        (BUTTON_REC|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_SELECT
#define BJACK_STAY       BUTTON_RIGHT
#define BJACK_DOUBLEDOWN BUTTON_LEFT
#define BJACK_SCORES     BUTTON_UP
#define BJACK_RESUME     BUTTON_REC
#define BJACK_UP         BUTTON_SCROLL_FWD
#define BJACK_DOWN       BUTTON_SCROLL_BACK
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define BJACK_START      BUTTON_SELECT
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_MAX        BUTTON_VOL_UP
#define BJACK_MIN        BUTTON_VOL_DOWN
#define BJACK_HIT        BUTTON_SELECT
#define BJACK_STAY       BUTTON_RIGHT
#define BJACK_DOUBLEDOWN BUTTON_LEFT
#define BJACK_SCORES     BUTTON_REC
#define BJACK_RESUME     BUTTON_DOWN
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == ELIO_TPJ1022_PAD
#define BJACK_START      BUTTON_MAIN
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_MAX        (BUTTON_REC|BUTTON_UP)
#define BJACK_MIN        (BUTTON_REC|BUTTON_DOWN)
#define BJACK_HIT        BUTTON_MAIN
#define BJACK_STAY       BUTTON_MENU
#define BJACK_DOUBLEDOWN BUTTON_DOWN
#define BJACK_SCORES     BUTTON_UP
#define BJACK_RESUME     BUTTON_FF
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define BJACK_START      BUTTON_PLAY
#define BJACK_QUIT       BUTTON_BACK
#define BJACK_MAX        BUTTON_VOL_UP
#define BJACK_MIN        BUTTON_VOL_DOWN
#define BJACK_HIT        BUTTON_VOL_UP
#define BJACK_STAY       BUTTON_VOL_DOWN
#define BJACK_DOUBLEDOWN BUTTON_SELECT
#define BJACK_SCORES     BUTTON_RIGHT
#define BJACK_RESUME     BUTTON_MENU
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == MROBE100_PAD

#define BJACK_START      BUTTON_SELECT
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_MAX        BUTTON_MENU
#define BJACK_MIN        BUTTON_DISPLAY
#define BJACK_HIT        BUTTON_MENU
#define BJACK_STAY       BUTTON_DISPLAY
#define BJACK_DOUBLEDOWN BUTTON_SELECT
#define BJACK_SCORES     BUTTON_RIGHT
#define BJACK_RESUME     BUTTON_PLAY
#define BJACK_UP         BUTTON_UP
#define BJACK_DOWN       BUTTON_DOWN
#define BJACK_RIGHT      BUTTON_RIGHT
#define BJACK_LEFT       BUTTON_LEFT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define BJACK_START      BUTTON_RC_PLAY
#define BJACK_QUIT       BUTTON_RC_REC
#define BJACK_MAX        (BUTTON_RC_PLAY|BUTTON_RC_VOL_UP)
#define BJACK_MIN        (BUTTON_RC_PLAY|BUTTON_RC_VOL_DOWN)
#define BJACK_HIT        BUTTON_RC_PLAY
#define BJACK_STAY       BUTTON_RC_FF
#define BJACK_DOUBLEDOWN BUTTON_RC_REW
#define BJACK_SCORES     BUTTON_RC_MENU
#define BJACK_RESUME     BUTTON_RC_MODE
#define BJACK_UP         BUTTON_RC_VOL_UP
#define BJACK_DOWN       BUTTON_RC_VOL_DOWN
#define BJACK_RIGHT      BUTTON_RC_FF
#define BJACK_LEFT       BUTTON_RC_REW

#elif CONFIG_KEYPAD == COWOND2_PAD
#define BJACK_QUIT       BUTTON_POWER
#define BJACK_DOUBLEDOWN BUTTON_MINUS
#define BJACK_SCORES     BUTTON_MENU

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHPAD
#ifndef BJACK_START
#define BJACK_START      BUTTON_CENTER
#endif
#ifndef BJACK_HIT
#define BJACK_HIT        BUTTON_CENTER
#endif
#ifndef BJACK_MAX
#define BJACK_MAX        BUTTON_TOPRIGHT
#endif
#ifndef BJACK_MIN
#define BJACK_MIN        BUTTON_TOPLEFT
#endif
#ifndef BJACK_RESUME
#define BJACK_RESUME     BUTTON_BOTTOMRIGHT
#endif
#ifndef BJACK_STAY
#define BJACK_STAY       BUTTON_BOTTOMLEFT
#endif
#ifndef BJACK_UP
#define BJACK_UP         BUTTON_TOPMIDDLE
#endif
#ifndef BJACK_DOWN
#define BJACK_DOWN       BUTTON_BOTTOMMIDDLE
#endif
#ifndef BJACK_RIGHT
#define BJACK_RIGHT      BUTTON_MIDRIGHT
#endif
#ifndef BJACK_LEFT
#define BJACK_LEFT       BUTTON_MIDLEFT
#endif

#endif

#ifdef HAVE_LCD_COLOR
#define BG_COLOR LCD_RGBPACK(0,157,0)
#define FG_COLOR LCD_WHITE
#elif LCD_DEPTH > 1
#define BG_COLOR LCD_WHITE
#define FG_COLOR LCD_BLACK
#endif

#define CARD_WIDTH  BMPWIDTH_card_back
#define CARD_HEIGHT BMPHEIGHT_card_back

/* This is the max amount of cards onscreen before condensing */
#define MAX_CARDS LCD_WIDTH/(CARD_WIDTH+4)

extern const fb_data card_deck[];
extern const fb_data card_back[];

#define NEXT_CARD bj->player_cards[done][bj->num_player_cards[done]]

/* global rockbox api */
static const struct plugin_api* rb;

MEM_FUNCTION_WRAPPERS(rb);

/* dealer and player card positions */
unsigned int dealer_x, dealer_y, player_x, player_y;

typedef struct card {
    unsigned int value; /* Card's value in Blackjack */
    unsigned int num;        /* Value on card face 0-12 (0=Ace, 1=2, 11=Q) */
    unsigned int suit;       /* 0:Spades, 1:Hearts, 2: Clubs; 3: Diamonds */
    bool is_soft_ace;
} card;

typedef struct game_context {
    struct card player_cards[2][22]; /* 22 Cards means the deal was all aces */
    struct card dealer_cards[22];    /* That is the worst-case scenario */
    unsigned int player_total;              
    unsigned int dealer_total;              
    signed int player_money;                
    unsigned int num_player_cards[2]; 
    unsigned int num_dealer_cards;    
    unsigned int current_bet;               
    unsigned int split_status; /* 0 = split hasn't been asked,         *
                                * 1 = split did not occur              *
                                * 2 = split occurred                   *
                                * 3 = split occurred and 1st hand done */                                            
    bool is_blackjack;                      
    bool end_hand;                          
    bool asked_insurance;               
    signed short highscores[NUM_SCORES];
    bool resume;
    bool dirty;
} game_context;

/*****************************************************************************
* blackjack_init() initializes blackjack data structures.
******************************************************************************/
static void blackjack_init(struct game_context* bj) {  
    /* seed the rand generator */
    rb->srand(*rb->current_tick);

    /* reset card positions */
    dealer_x = 4;
    dealer_y = LCD_HEIGHT/4 - CARD_HEIGHT/2;
    player_x = 4;
    player_y = LCD_HEIGHT - LCD_HEIGHT/4 - CARD_HEIGHT/2;
    
    /* check for resumed game */
    if(bj->resume) return;

    /* reset scoring */
    bj->player_total = 0;
    bj->dealer_total = 0;
    bj->num_player_cards[0] = 2;
    bj->num_player_cards[1] = 0;
    bj->num_dealer_cards = 2;
    bj->end_hand = false;
    bj->split_status = 0;
    bj->is_blackjack = false;
    bj->asked_insurance = false;   
}

/*****************************************************************************
* blackjack_drawtable() draws the table and some text.
******************************************************************************/
static void blackjack_drawtable(struct game_context* bj) {
    unsigned int w, h, y_loc;
    char str[10];
    
#if LCD_HEIGHT <= 64
    rb->lcd_getstringsize("Bet", &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, 2*h + 1, "Bet");
    rb->snprintf(str, 9, "$%d", bj->current_bet);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, 3*h + 1, str);
    y_loc = LCD_HEIGHT/2;
#else
    rb->lcd_getstringsize("Bet", &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, 5*h / 2, "Bet");
    rb->snprintf(str, 9, "$%d", bj->current_bet);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, 7*h / 2, str);
    rb->lcd_hline(0, LCD_WIDTH, LCD_HEIGHT/2);
    y_loc = LCD_HEIGHT/2 + h;
#endif

    rb->lcd_putsxy(0,0, "Dealer");
    rb->lcd_getstringsize("Player", &w, &h);
    rb->lcd_putsxy(0, y_loc, "Player");
    rb->lcd_getstringsize("Total", &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, y_loc, "Total");
    rb->lcd_getstringsize("Money", &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, 0, "Money");
    rb->snprintf(str, 9, "$%d", bj->player_money - bj->current_bet);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, h + 1, str);
    rb->snprintf(str, 3, "%d", bj->player_total);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH - w, y_loc + h, str);
}

/*****************************************************************************
* find_value() is passed a card and returns its blackjack value.
******************************************************************************/
static unsigned int find_value(unsigned int number) {
    unsigned int thisValue;
    if (number == 0)
        thisValue = 11;   /* Aces get a value of 11 at first */
    else if (number < 10)
        thisValue = number + 1;
    else
        thisValue = 10;   /* Anything 10 or higher gets a value of 10 */

    return thisValue;
}

/*****************************************************************************
* draw_card() draws a card to the screen.
******************************************************************************/
static void draw_card(struct card temp_card, bool shown, unsigned int x, 
                      unsigned int y) {
    if(shown)
        rb->lcd_bitmap_part(card_deck, CARD_WIDTH*temp_card.num, 
                            CARD_HEIGHT*temp_card.suit, BMPWIDTH_card_deck,
                            x+1, y+1, CARD_WIDTH, CARD_HEIGHT);
    else
        rb->lcd_bitmap(card_back, x+1, y+1,CARD_WIDTH, CARD_HEIGHT);          
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    /* Print outlines */ 
#if CARD_WIDTH >= 26
    rb->lcd_hline(x+2, x+CARD_WIDTH-1, y);
    rb->lcd_hline(x+2, x+CARD_WIDTH-1, y+CARD_HEIGHT+1);
    rb->lcd_vline(x, y+2, y+CARD_HEIGHT-3);
    rb->lcd_vline(x+CARD_WIDTH+1, y+2, y+CARD_HEIGHT-1);
    rb->lcd_drawpixel(x+1, y+1);
    rb->lcd_drawpixel(x+1, y+CARD_HEIGHT);
    rb->lcd_drawpixel(x+CARD_WIDTH, y+1);
    rb->lcd_drawpixel(x+CARD_WIDTH, y+CARD_HEIGHT);
#else
    rb->lcd_hline(x+1, x+CARD_WIDTH, y);
    rb->lcd_hline(x+1, x+CARD_WIDTH, y+CARD_HEIGHT+1);
    rb->lcd_vline(x, y+1, y+CARD_HEIGHT);
    rb->lcd_vline(x+CARD_WIDTH+1, y+1, y+CARD_HEIGHT);
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(FG_COLOR);
#endif
}

/*****************************************************************************
* new_card() initializes a new card and gives it values.
******************************************************************************/
static struct card new_card(void) {
    struct card new_card;
    new_card.suit = rb->rand()%4; /* Random number 0-3 */
    new_card.num = rb->rand()%13; /* Random number 0-12 */
    new_card.value = find_value(new_card.num);      
    new_card.is_soft_ace = new_card.num == 0 ? true : false;
    return new_card;
}

/*****************************************************************************
* deal_init_card() deals and draws to the screen the player's and dealer's
* initial cards.
******************************************************************************/
static void deal_init_cards(struct game_context* bj) {
    bj->dealer_cards[0] = new_card();
    bj->dealer_total += bj->dealer_cards[0].value;

    draw_card(bj->dealer_cards[0], false, dealer_x, dealer_y);

    bj->dealer_cards[1] = new_card();
    bj->dealer_total += bj->dealer_cards[1].value;
    draw_card(bj->dealer_cards[1], true, dealer_x + CARD_WIDTH + 4, dealer_y);

    bj->player_cards[0][0] = new_card();
    bj->player_total += bj->player_cards[0][0].value;
    draw_card(bj->player_cards[0][0], true, player_x, player_y);
    player_x += CARD_WIDTH + 4;

    bj->player_cards[0][1] = new_card();
    bj->player_total += bj->player_cards[0][1].value;
    draw_card(bj->player_cards[0][1], true, player_x, player_y);
    player_x += CARD_WIDTH + 4;    
}

/*****************************************************************************
* redraw_board() redraws all the cards and the board
******************************************************************************/
static void redraw_board(struct game_context* bj) {
    unsigned int i, n, upper_bound;
    rb->lcd_clear_display();

    blackjack_drawtable(bj);
    player_x = 4;
    dealer_x = 4;
    upper_bound = bj->split_status > 1 ? 2 : 1;

    for (i = 0; i < bj->num_dealer_cards; i++) {
        if (!bj->end_hand) {
            draw_card(bj->dealer_cards[0], false, dealer_x, dealer_y);

            /* increment i so the dealer's first card isn't displayed */
            i++;
            dealer_x += CARD_WIDTH + 4;
        }
        draw_card(bj->dealer_cards[i], true, dealer_x, dealer_y);
        
        if (bj->num_dealer_cards > MAX_CARDS-1)
            dealer_x += 10;
        else
            dealer_x += CARD_WIDTH + 4;
    }

    for (n = 0; n < upper_bound; n++) {
        for (i = 0; i < bj->num_player_cards[n]; i++) {
            draw_card(bj->player_cards[n][i], true, player_x, player_y);
            if (bj->split_status>1 || bj->num_player_cards[n]>MAX_CARDS)
                player_x += 10;
            else
                player_x += CARD_WIDTH + 4;                
        }
        if (bj->split_status > 1)
            player_x = LCD_WIDTH/2 + 4;
    }
}

/*****************************************************************************
* update_total updates the player's total
******************************************************************************/
static void update_total(struct game_context* bj) {
    char total[3];
    unsigned int w, h;
    rb->snprintf(total, 3, "%d", bj->player_total);
    rb->lcd_getstringsize(total, &w, &h);
#if LCD_HEIGHT > 64
    h *= 2;
#endif
    rb->lcd_putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 + h, total);
    rb->lcd_update_rect(LCD_WIDTH - w, LCD_HEIGHT/2 + h, w, h);
}


/*****************************************************************************
* check_for_aces() is passed an array of cards and returns where an ace is
* located. Otherwise, returns -1.
******************************************************************************/
static signed int check_for_aces(struct card temp_cards[], 
                                 unsigned int size) {
    unsigned int i;
    for(i = 0; i < size; i++) {
        if (temp_cards[i].is_soft_ace == true)
          return i;
    }
    return -1;
}

/*****************************************************************************
* check_totals() compares player and dealer totals.
* 0: bust 1: loss, 2: push, 3: win, 4: blackjack, 5: something's not right...
******************************************************************************/
static unsigned int check_totals(struct game_context* bj)
{
    unsigned int temp;
    if (bj->player_total > 21)
      temp = 0;
    else if (bj->player_total == 21 && bj->is_blackjack)
      if (bj->dealer_total == 21 && bj->num_dealer_cards == 2)
        temp = 2;
      else
        temp = 4;
    else if (bj->player_total == bj->dealer_total)
      temp = 2;
    else if (bj->dealer_total > 21 && bj->player_total < 22)
      temp = 3;
    else if (bj->dealer_total > bj->player_total)
      temp = 1;
    else if (bj->player_total > bj->dealer_total)
      temp = 3;
    else
      temp = 5;

    return temp;
}

/*****************************************************************************
* finish_dealer() draws cards for the dealer until he has 17 or more.
******************************************************************************/
static void finish_dealer(struct game_context* bj) {
    signed int temp = 0;
    
    if (bj->dealer_total > 16 && bj->dealer_total < 22)
        return;
    
    while (bj->dealer_total < 17) {
      bj->dealer_cards[bj->num_dealer_cards] = new_card();
      bj->dealer_total += bj->dealer_cards[bj->num_dealer_cards].value;
      bj->num_dealer_cards++;
    }
    
    while (bj->dealer_total > 21) {
      temp = check_for_aces(bj->dealer_cards, bj->num_dealer_cards);
      if(temp != -1) {
        bj->dealer_cards[temp].is_soft_ace = false;
        bj->dealer_total -= 10;
      }
      else
        return;
    }
}

/*****************************************************************************
* finish_game() completes the game once player's turn is over.
******************************************************************************/
static void finish_game(struct game_context* bj) {
    unsigned int rValue, w, h;
    char str[19];

    do {
        finish_dealer(bj);
    } while (bj->dealer_total < 17);
    
    redraw_board(bj);
    rValue = check_totals(bj);

    if (rValue == 0) {
      rb->snprintf(str, sizeof(str), " Bust! ");
      bj->player_money -= bj->current_bet;
    }
    else if (rValue == 1) {
      rb->snprintf(str, sizeof(str), " Sorry, you lost. ");
      bj->player_money -= bj->current_bet;
    }
    else if (rValue == 2) {
      rb->snprintf(str, sizeof(str), " Push ");
    }
    else if (rValue == 3) {
      rb->snprintf(str, sizeof(str), " You won! ");
      bj->player_money+= bj->current_bet;
    }
    else {
      rb->snprintf(str, sizeof(str), " Blackjack! ");
      bj->player_money += bj->current_bet * 3 / 2;
    }
    rb->lcd_getstringsize(str, &w, &h);
    
#if LCD_HEIGHT <= 64
    rb->lcd_set_drawmode(DRMODE_BG+DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, LCD_HEIGHT/2, LCD_WIDTH, LCD_HEIGHT/2);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + h, str);
    rb->snprintf(str, 12, "You have %d", bj->player_total);
    rb->lcd_getstringsize(str, &w, &h);
    rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2, str);
#else
    rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 - h/2, str);
#endif
    rb->lcd_update();
}

/*****************************************************************************
* blackjack_recordscore() inserts a high score into the high scores list and
*     returns the high score position.
******************************************************************************/
static unsigned int blackjack_recordscore(struct game_context* bj) {
    unsigned int i;
    unsigned int position = 0;
    signed short current, temp;

    /* calculate total score */
    current = bj->player_money;
    if(current <= 10) return 0;

    /* insert the current score into the high scores */
    for(i=0; i<NUM_SCORES; i++) {
        if(current >= bj->highscores[i]) {
            if(!position) {
                position = i+1;
                bj->dirty = true;
            }
            temp = bj->highscores[i];
            bj->highscores[i] = current;
            current = temp;
        }
    }

    return position;
}

/*****************************************************************************
* blackjack_loadscores() loads the high scores saved file.
******************************************************************************/
static void blackjack_loadscores(struct game_context* bj) {
    signed int fd;

    bj->dirty = false;

    /* clear high scores */
    rb->memset(bj->highscores, 0, sizeof(bj->highscores));

    /* open scores file */
    fd = rb->open(SCORE_FILE, O_RDONLY);
    if(fd < 0) return;

    /* read in high scores */
    if(rb->read(fd, bj->highscores, sizeof(bj->highscores)) <= 0) {
        /* scores are bad, reset */
        rb->memset(bj->highscores, 0, sizeof(bj->highscores));
    }

    rb->close(fd);
}

/*****************************************************************************
* blackjack_savescores() saves the high scores saved file.
******************************************************************************/
static void blackjack_savescores(struct game_context* bj) {
    unsigned int fd;

    /* write out the high scores to the save file */
    fd = rb->open(SCORE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, bj->highscores, sizeof(bj->highscores));
    rb->close(fd);
    bj->dirty = false;
}

/*****************************************************************************
* blackjack_loadgame() loads the saved game and returns load success.
******************************************************************************/
static bool blackjack_loadgame(struct game_context* bj) {
    signed int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return loaded;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &bj->player_money, sizeof(bj->player_money)) <= 0) break;
        if(rb->read(fd, &bj->player_total, sizeof(bj->player_total)) <= 0) break;
        if(rb->read(fd, &bj->dealer_total, sizeof(bj->dealer_total)) <= 0) break;
        if(rb->read(fd, &bj->num_player_cards, sizeof(bj->num_player_cards))<=0)
            break;
        if(rb->read(fd, &bj->num_dealer_cards, sizeof(bj->num_dealer_cards))<=0)
            break;
        if(rb->read(fd, &bj->current_bet, sizeof(bj->current_bet)) <= 0) break;
        if(rb->read(fd, &bj->is_blackjack, sizeof(bj->is_blackjack)) <= 0) break;
        if(rb->read(fd, &bj->split_status, sizeof(bj->split_status)) <= 0) break;
        if(rb->read(fd, &bj->asked_insurance, sizeof(bj->asked_insurance)) <= 0)
            break;
        if(rb->read(fd, &bj->end_hand, sizeof(bj->end_hand)) <= 0) break;
        if(rb->read(fd, &bj->player_cards, sizeof(bj->player_cards)) <= 0) break;
        if(rb->read(fd, &bj->dealer_cards, sizeof(bj->dealer_cards)) <= 0) break;
        bj->resume = true;
        loaded = true;
        break;
    }

    rb->close(fd);

    /* delete saved file */
    rb->remove(SAVE_FILE);
    return loaded;
}

/*****************************************************************************
* blackjack_savegame() saves the current game state.
******************************************************************************/
static void blackjack_savegame(struct game_context* bj) {
    unsigned int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &bj->player_money, sizeof(bj->player_money));
    rb->write(fd, &bj->player_total, sizeof(bj->player_total));
    rb->write(fd, &bj->dealer_total, sizeof(bj->dealer_total));
    rb->write(fd, &bj->num_player_cards, sizeof(bj->num_player_cards));
    rb->write(fd, &bj->num_dealer_cards, sizeof(bj->num_dealer_cards));
    rb->write(fd, &bj->current_bet, sizeof(bj->current_bet));
    rb->write(fd, &bj->is_blackjack, sizeof(bj->is_blackjack));
    rb->write(fd, &bj->split_status, sizeof(bj->split_status));
    rb->write(fd, &bj->asked_insurance, sizeof(bj->asked_insurance));
    rb->write(fd, &bj->end_hand, sizeof(bj->end_hand));
    rb->write(fd, &bj->player_cards, sizeof(bj->player_cards));
    rb->write(fd, &bj->dealer_cards, sizeof(bj->dealer_cards));
    rb->close(fd);

    bj->resume = true;
}

/*****************************************************************************
* blackjack_callback() is the default event handler callback which is called
*     on usb connect and shutdown.
******************************************************************************/
static void blackjack_callback(void* param) {
    struct game_context* bj = (struct game_context*) param;
    if(bj->dirty) {
        rb->splash(HZ, "Saving high scores...");
        blackjack_savescores(bj);
    }
}

/*****************************************************************************
* blackjack_get_yes_no() gets a yes/no answer from the user
******************************************************************************/
static unsigned int blackjack_get_yes_no(char message[20]) {
    int button;
    unsigned int w, h, b, choice = 0;
    bool breakout = false;
    char message_yes[24], message_no[24];

    rb->strcpy(message_yes, message);
    rb->strcpy(message_no, message);
    rb->strcat(message_yes, " Yes");
    rb->strcat(message_no, "  No");
    rb->lcd_getstringsize(message_yes, &w, &h);
    const char *stg[] = {message_yes, message_no};
    
#if LCD_HEIGHT <= 64
    b = 2*h+1;
#else
    b = h-1;
#endif

#ifdef HAVE_LCD_COLOR
    rb->lcd_fillrect(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + b, w+1, h+3);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#else
    rb->lcd_set_drawmode(DRMODE_BG+DRMODE_INVERSEVID);
    rb->lcd_fillrect(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + b, w+1, h+3);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
    rb->lcd_drawrect(LCD_WIDTH/2 - w/2 - 1, LCD_HEIGHT/2 + b - 1, w+3, h+4);
    
    while(!breakout) {
        rb->lcd_putsxy(LCD_WIDTH/2 - w/2, LCD_HEIGHT/2 + b +1, stg[choice]);
        rb->lcd_update_rect(LCD_WIDTH/2 - w/2 - 1, LCD_HEIGHT/2 + b -1, 
                            w+3, h+4);
        button = rb->button_get(true);

        switch(button) {
            case BJACK_LEFT:
            case (BJACK_LEFT|BUTTON_REPEAT):
            case BJACK_RIGHT:
            case (BJACK_RIGHT|BUTTON_REPEAT):
                choice ^= 1;
                break;
            case BJACK_START: breakout = true;
                break;
            case BJACK_QUIT: breakout = true;
                choice = BJ_QUIT;
                break;
        }
    }
    
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(FG_COLOR);
    rb->lcd_set_background(BG_COLOR);
#endif
    return choice;
}

/*****************************************************************************
* blackjack_get_amount() gets an amount from the player to be used
******************************************************************************/
static signed int blackjack_get_amount(char message[20], signed int lower_limit, 
                                       signed int upper_limit, 
                                       signed int start) {
    int button;
    char str[6];
    bool changed = false;
    unsigned int w, h;
    signed int amount;
    
    rb->lcd_getstringsize("A", &w, &h); /* find the size of one character */
    
    if (start > upper_limit)
        amount = upper_limit;
    else if (start < lower_limit)
        amount = lower_limit;
    else
        amount = start;

#if LCD_DEPTH > 1
    rb->lcd_set_background(LCD_WHITE);
    rb->lcd_set_foreground(LCD_BLACK);
#endif

#if LCD_HEIGHT <= 64
    rb->lcd_clear_display();
    rb->lcd_puts(0, 1, message);
    rb->snprintf(str, 9, "$%d", amount);
    rb->lcd_puts(0, 2, str);
    rb->lcd_puts(0, 3, "RIGHT: +1");
    rb->lcd_puts(0, 4, "LEFT:  -1");
    rb->lcd_puts(0, 5, "UP:   +10");
    rb->lcd_puts(0, 6, "DOWN: -10");
    rb->lcd_update();
#else
    rb->lcd_set_drawmode(DRMODE_BG+DRMODE_INVERSEVID);
    rb->lcd_fillrect(LCD_WIDTH/2 - 9*w - 1, LCD_HEIGHT/2 - 4*h - 3, 37*w / 2, 
                     8*h -3);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_drawrect(LCD_WIDTH/2 - 9*w - 1, LCD_HEIGHT/2 - 4*h - 3, 37*w / 2, 
                     8*h -3);
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 4*h - 1, message);
    rb->snprintf(str, 9, "$%d", amount);
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 3*h, str);
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - h-2, " >>|:     +1");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 1, " |<<:     -1");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 + h, "SCROLL+: +10");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 + 2*h + 1, "SCROLL-: -10");
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - h-2, "RIGHT:    +1");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 1, "LEFT:     -1");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 + h, "SCROLL+: +10");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 + 2*h + 1, "SCROLL-: -10");
#else
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - h-2, "RIGHT: +1");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 1, "LEFT:  -1");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 + h, "UP:   +10");
    rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 + 2*h + 1, "DOWN: -10");
#endif
    rb->lcd_update_rect(LCD_WIDTH/2 - 9*w - 2, LCD_HEIGHT/2 - 9*h/2, 37*w/2 + 1,
                        8*h-2);
#endif
    
    while(true) {
        button = rb->button_get(true);

        switch(button) {
            case BJACK_UP:
            case (BJACK_UP|BUTTON_REPEAT):
            if (amount + 10 < upper_limit + 1) {
                    amount += 10;
                    changed = true;
            }
            break;
            case BJACK_DOWN:
            case (BJACK_DOWN|BUTTON_REPEAT):
                if (amount - 10 > lower_limit - 1) {
                    amount -= 10;
                    changed = true;
                }
                break;
            case BJACK_RIGHT:
            case (BJACK_RIGHT|BUTTON_REPEAT):
                if (amount + 1 < upper_limit + 1) {
                    amount++;
                    changed = true;
                }
                break;
            case BJACK_LEFT:
            case (BJACK_LEFT|BUTTON_REPEAT):
                if (amount - 1 > lower_limit - 1) {
                    amount--;
                    changed = true;
                }
                break;
            case BJACK_MAX :
                amount = upper_limit;
                changed = true;
                break;
            case BJACK_MIN :
                amount = lower_limit;
                changed = true;
                break;
            case BJACK_QUIT:
                return 0;
            case BJACK_START:
#if LCD_DEPTH > 1
                rb->lcd_set_foreground(FG_COLOR);
                rb->lcd_set_background(BG_COLOR);
#endif
                rb->lcd_clear_display();
                return amount;
       }

       if(changed) {
            rb->snprintf(str, 9, "$%d", amount);
#if LCD_HEIGHT <= 64
            rb->lcd_puts(0, 2, str);
            rb->lcd_update();
#else
            rb->lcd_set_drawmode(DRMODE_BG+DRMODE_INVERSEVID);
            rb->lcd_fillrect(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 3*h, 5*w, h);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 3*h, str);
            rb->lcd_update_rect(LCD_WIDTH/2 - 9*w, LCD_HEIGHT/2 - 3*h, 5*w, h);
#endif
            changed = false;
        }
    }
}

/*****************************************************************************
* blackjack_get_bet() gets the player's bet.
******************************************************************************/
static void blackjack_get_bet(struct game_context* bj) {
    bj->current_bet = blackjack_get_amount("Please enter a bet", 10, 
                                           bj->player_money, bj->current_bet);
}

/*****************************************************************************
* double_down() returns one final card then finishes the game
******************************************************************************/
static void double_down(struct game_context* bj) {
    bj->current_bet *= 2;
    bj->player_cards[0][bj->num_player_cards[0]] = new_card();
    bj->player_total += bj->player_cards[0][bj->num_player_cards[0]].value;
    bj->num_player_cards[0]++;
}

/*****************************************************************************
* split() checks if the player wants to split and acts accordingly.
* When bj->split_status is 1, no split occurred. 2 means the player split and 3
* means a split has already occurred and the first hand is done.
******************************************************************************/
static void split(struct game_context* bj) {
    if (blackjack_get_yes_no("Split?") == 1)
        bj->split_status = 1;
    else {
        bj->split_status = 2;
        bj->current_bet *= 2;
        bj->num_player_cards[0] = 1;
        bj->num_player_cards[1] = 1;
        bj->player_cards[1][0] = bj->player_cards[0][1];
        bj->player_total = bj->player_cards[0][0].value;
    }
}

/*****************************************************************************
* insurance() see if the player wants to buy insurance and how much.
******************************************************************************/
static unsigned int insurance(struct game_context* bj) {
    unsigned int insurance, max_amount; 
    
    insurance = blackjack_get_yes_no("Buy Insurance?");
    bj->asked_insurance = true;
    max_amount = bj->current_bet < (unsigned int)bj->player_money ? 
                            bj->current_bet/2 : (unsigned int)bj->player_money;
    if (insurance == 1) return 0;

    insurance = blackjack_get_amount("How much?", 0, max_amount, 0);
    redraw_board(bj);
    return insurance;
}

/*****************************************************************************
* play_again() checks to see if the player wants to keep playing.
******************************************************************************/
static unsigned int play_again(void) {
    return blackjack_get_yes_no("Play Again?");
}

/*****************************************************************************
* blackjack_menu() is the initial menu at the start of the game.
******************************************************************************/
static unsigned int blackjack_menu(struct game_context* bj) {
    int button;
    char *title = "Blackjack";
    char str[18];
    unsigned int i, w, h;
    bool breakout = false;
    bool showscores = false;
    
    while(true){
#if LCD_DEPTH > 1
        rb->lcd_set_background(BG_COLOR);
        rb->lcd_set_foreground(FG_COLOR);
#endif
        rb->lcd_clear_display();

        if(!showscores) {
             /* welcome screen to display key bindings */
            rb->lcd_getstringsize(title, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, title);

#if CONFIG_KEYPAD == RECORDER_PAD
            rb->lcd_puts(0, 1, "ON: start");
            rb->lcd_puts(0, 2, "OFF: exit");
            rb->lcd_puts(0, 3, "F1: hit");
            rb->lcd_puts(0, 4, "F2: stay");
            rb->lcd_puts(0, 5, "F3: double down");
            rb->lcd_puts(0, 6, "PLAY: save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 7, str);
#elif CONFIG_KEYPAD == ONDIO_PAD
            rb->lcd_puts(0, 1, "MENU: start");
            rb->lcd_puts(0, 2, "OFF: exit");
            rb->lcd_puts(0, 3, "LEFT: hit");
            rb->lcd_puts(0, 4, "RIGHT: stay");
            rb->lcd_puts(0, 5, "UP: double down");
            rb->lcd_puts(0, 6, "DOWN: save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 7, str);
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
            rb->lcd_puts(0, 2, "PLAY to start & to hit");
            rb->lcd_puts(0, 3, "STOP to exit");
            rb->lcd_puts(0, 4, "REC to stay");
            rb->lcd_puts(0, 5, "NAVI to double down ");
            rb->lcd_puts(0, 6, "  & to view highscores");
            rb->lcd_puts(0, 7, "AB to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 8, str);
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
            rb->lcd_puts(0, 2, "PLAY to start & hit");
            rb->lcd_puts(0, 3, "POWER to exit");
            rb->lcd_puts(0, 4, ">>| to stay");
            rb->lcd_puts(0, 5, "|<< to double down");
            rb->lcd_puts(0, 6, "LEFT to view scores");
            rb->lcd_puts(0, 7, "RIGHT to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 8, str);

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#if LCD_WIDTH >=176
            rb->lcd_puts(0, 2, "SELECT to start & to hit");
            rb->lcd_puts(0, 3, "MENU to exit");
            rb->lcd_puts(0, 4, ">>| to stay & to view highscores");
            rb->lcd_puts(0, 5, "|<< to double down");
            rb->lcd_puts(0, 6, "PLAY to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 7, str);
#else
            rb->lcd_puts(0, 2, "SELECT to start & to ");
            rb->lcd_puts(0, 3, "  hit");
            rb->lcd_puts(0, 4, "MENU to exit");
            rb->lcd_puts(0, 5, ">>| to stay & to view ");
            rb->lcd_puts(0, 6, "  highscores");
            rb->lcd_puts(0, 7, "|<< to double down");
            rb->lcd_puts(0, 8, "PLAY to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 9, str);
#endif
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
            rb->lcd_puts(0, 2, "PLAY to start to hit");
            rb->lcd_puts(0, 3, "POWER to exit");
            rb->lcd_puts(0, 4, "SELECT to hit");
            rb->lcd_puts(0, 5, "REC to stay");
            rb->lcd_puts(0, 6, "PLAY to double down");
            rb->lcd_puts(0, 7, "RIGHT to view highscores ");
            rb->lcd_puts(0, 8, "DOWN to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 9, str);
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
            rb->lcd_puts(0, 2, "AB to start & to");
            rb->lcd_puts(0, 3, "  stay");
            rb->lcd_puts(0, 4, "EQ to hit");
            rb->lcd_puts(0, 5, "PLAY to exit");
            rb->lcd_puts(0, 6, "CLICK to double down");
            rb->lcd_puts(0, 7, "& to view highscores");
            rb->lcd_puts(0, 8, "AB+EQ to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 9, str);
#elif CONFIG_KEYPAD == GIGABEAT_PAD
            rb->lcd_puts(0, 2, "POWER to start");
            rb->lcd_puts(0, 3, "A to exit");
            rb->lcd_puts(0, 4, "VOL+ to hit");
            rb->lcd_puts(0, 5, "VOL- to stay");
            rb->lcd_puts(0, 6, "CENTER to double down");
            rb->lcd_puts(0, 6, "RIGHT to view highscores ");
            rb->lcd_puts(0, 8, "MENU to save/resume");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 9, str);
#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
            rb->lcd_puts(0, 2, "SELECT to start & to hit");
            rb->lcd_puts(0, 3, "POWER to exit");
            rb->lcd_puts(0, 4, "RIGHT to stay");
            rb->lcd_puts(0, 5, "LEFT to double down");
            rb->lcd_puts(0, 6, "REC to save/resume");
            rb->lcd_puts(0, 7, "UP to view scores");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 8, str);
#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
            rb->lcd_puts(0, 2, "SELECT to start & to hit");
            rb->lcd_puts(0, 3, "POWER to exit");
            rb->lcd_puts(0, 4, "RIGHT to stay");
            rb->lcd_puts(0, 5, "LEFT to double down");
            rb->lcd_puts(0, 6, "DOWN to save/resume");
            rb->lcd_puts(0, 7, "REC to view scores");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 9, str);
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
            rb->lcd_puts(0, 2, "PLAY to start & to");
            rb->lcd_puts(0, 3, "  hit");
            rb->lcd_puts(0, 4, "REC to exit");
            rb->lcd_puts(0, 5, "FF to stay");
            rb->lcd_puts(0, 6, "REW to double down");
            rb->lcd_puts(0, 7, "MODE to save/resume");
            rb->lcd_puts(0, 8, "MENU to view scores");
            rb->snprintf(str, 21, "High Score: $%d", bj->highscores[0]);
            rb->lcd_puts(0, 10, str);
#elif CONFIG_KEYPAD == COWOND2_PAD
            rb->lcd_puts(0, 6, "POWER           to exit");
            rb->lcd_puts(0, 7, "MINUS           to double down");
            rb->lcd_puts(0, 8, "MENU            to view scores");
            rb->snprintf(str, 21, "High Score:     $%d", bj->highscores[0]);
            rb->lcd_puts(0, 10, str);
#endif

#ifdef HAVE_TOUCHPAD
            rb->lcd_puts(0, 2, "LCD CENTRE      to start & to hit");
            rb->lcd_puts(0, 3, "LCD BOTTOMLEFT  to stay");
            rb->lcd_puts(0, 4, "LCD BOTTOMRIGHT to save/resume");
#endif
        } else {
            rb->snprintf(str, 12, "%s", "High Scores");
            rb->lcd_getstringsize(str, &w, &h);
            rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, str);

            /* print high scores */
            for(i=0; i<NUM_SCORES; i++) {
                rb->snprintf(str, 14, "#%02d: $%d", i+1, bj->highscores[i]);
                rb->lcd_puts(0, i+1, str);
            }
        }

        rb->lcd_update();

        /* handle menu button presses */
        button = rb->button_get(true);

        switch(button) {
            case BJACK_START: /* start playing */
                breakout = true;
                break;

            case BJACK_QUIT:  /* quit program */
                if(showscores) {
                    showscores = 0;
                    break;
                }
                return BJ_QUIT;

            case BJACK_RESUME:/* resume game */
                if(!blackjack_loadgame(bj)) {
                    rb->splash(HZ*2, "Nothing to resume");
                } else {
                    rb->splash(HZ*2, "Loading...");
                    breakout = true;
                }
                break;

            case BJACK_SCORES:/* toggle high scores */
                showscores = !showscores;
                break;

            default:
                if(rb->default_event_handler_ex(button, blackjack_callback,
                   (void*) bj) == SYS_USB_CONNECTED)
                    return BJ_USB;
                break;
        }

        if(breakout) break;
    }

    return(0);
}

/*****************************************************************************
* blackjack() is the main game subroutine, it returns the final game status.
******************************************************************************/
static int blackjack(struct game_context* bj) {
    int button;
    unsigned int w, h, temp_var, done = 0, todo = 1;
    signed int temp;
    bool breakout = false;
    bool dbl_down = false;

    /* don't resume by default */
    bj->resume = false;

    /********************
    *       menu        *
    ********************/
    temp_var = blackjack_menu(bj);
    if (temp_var == BJ_QUIT || temp_var == BJ_USB)
        return temp_var;


    /********************
    *       init        *
    ********************/
    blackjack_init(bj);
    bj->current_bet=10;

    /********************
    *       play        *
    ********************/

    /* check for resumed game */
    if(bj->resume) {
        bj->resume = false;
        redraw_board(bj);
        if (bj->split_status == 2) {
          todo=2;
          player_x = bj->num_player_cards[0] * 10 + 4;
        }
        else if (bj->split_status == 3) {
          player_x = bj->num_player_cards[1] * 10 + LCD_WIDTH/2 + 4;
          todo=2;
          done=1;
        }

    }
    else {
        bj->player_money = 1000;
        blackjack_get_bet(bj);
        if (bj->current_bet == 0)
            return BJ_QUIT;
        rb->lcd_clear_display();
        deal_init_cards(bj);
        blackjack_drawtable(bj);
    }

    rb->lcd_update();

    breakout = false;

    while(true){
        if(bj->player_total == 21 && bj->num_player_cards[0] == 2) {
            bj->is_blackjack = true;
            bj->end_hand = true;
            finish_game(bj);
        }
        else if(bj->dealer_cards[1].is_soft_ace && !breakout && 
                !bj->asked_insurance) {
            temp_var = insurance(bj);
            if (bj->dealer_total == 21) {
                rb->splash(HZ, "Dealer has blackjack");
                bj->player_money += temp_var;
                bj->end_hand = true;
                breakout = true;
                redraw_board(bj);
                finish_game(bj);
            }
            else {
                rb->splash(HZ, "Dealer does not have blackjack");
                bj->player_money -= temp_var;
                breakout = true;
                redraw_board(bj);
                rb->lcd_update();
            }
        }
        if(bj->split_status == 0 && 
           bj->player_cards[0][0].num == bj->player_cards[0][1].num) {
            split(bj);
            redraw_board(bj);
            rb->lcd_update_rect(0, LCD_HEIGHT/2, LCD_WIDTH, LCD_HEIGHT/2);
            if (bj->split_status == 2) {
                todo++;
                player_x = bj->num_player_cards[0] * 10 + 4;
            }
        }

        while(done < todo) {
            button = rb->button_get(true);

            switch(button) {
                case BJACK_HIT:
                    NEXT_CARD = new_card();
                    bj->player_total += NEXT_CARD.value;
                    draw_card(NEXT_CARD, true, player_x, player_y);
                    bj->num_player_cards[done]++;
                    if (bj->num_player_cards[done] == MAX_CARDS + 1) {
                        redraw_board(bj);
                        rb->lcd_update_rect(0, LCD_HEIGHT/2, LCD_WIDTH, 
                                            LCD_HEIGHT/2);
                    }
                    else if (bj->num_player_cards[done]>MAX_CARDS || todo > 1) {
                        rb->lcd_update_rect(player_x, player_y, CARD_WIDTH+2, 
                                            CARD_HEIGHT+2);
                        player_x += 10;
                    }
                    else {
                        rb->lcd_update_rect(player_x, player_y, CARD_WIDTH+2, 
                                            CARD_HEIGHT+2);
                        player_x += CARD_WIDTH + 4;
                    }
                    update_total(bj);

                    break;
                case BJACK_STAY:
                    bj->end_hand = true;
                    break;
                case BJACK_DOUBLEDOWN:
                    if ((signed int)bj->current_bet * 2 < bj->player_money + 1 && 
                         bj->num_player_cards[0]==2 && todo==1) {
                        double_down(bj);
                        dbl_down = true;
                        if (bj->player_total < 22) {
                            bj->end_hand = true;
                            finish_game(bj);
                        }
                    }
                    else if((signed int)bj->current_bet * 2 > bj->player_money) {
                        rb->splash(HZ, "Not enough money to double down.");
                        redraw_board(bj);
                        rb->lcd_update();
                    }
                    break;
                case BJACK_RESUME:           /* save and end game */
                    rb->splash(HZ, "Saving game...");
                    blackjack_savegame(bj);
                    /* fall through to BJACK_QUIT */

                case BJACK_QUIT:
                    return BJ_END;
            }

            while (bj->player_total > 21 && !bj->end_hand) {
                temp = check_for_aces(bj->player_cards[done], 
                                      bj->num_player_cards[done]);
                if(temp != -1) {
                    bj->player_cards[done][temp].is_soft_ace = false;
                    bj->player_total -= 10;
                    update_total(bj);
                    if (dbl_down) {
                        bj->end_hand = true;
                        finish_game(bj);
                    }
                }
                else
                    bj->end_hand = true;
            }
            
            if (bj->end_hand) {
                done++;
                if(todo > 1) {
                    if (done == 2) {
                        temp = bj->player_total;
                        bj->player_total = temp_var;
                        temp_var = temp;
                        finish_game(bj);
                        rb->lcd_getstringsize(" Split 1 ", &w, &h);
                        rb->lcd_putsxy(LCD_WIDTH/2-w/2, LCD_HEIGHT/2-3*h/2,
                                       " Split 1 ");
                        rb->lcd_update_rect(LCD_WIDTH/2-w/2, LCD_HEIGHT/2-3*h/2,
                                            w,h);
                        bj->current_bet /= 2;
                        rb->lcd_update_rect(LCD_WIDTH/2-w/2, LCD_HEIGHT/2-3*h/2,
                                            w,h);
                        rb->sleep(HZ*2);                      
                        bj->player_total = temp_var;
                        finish_game(bj);
                        rb->lcd_getstringsize(" Split 2 ", &w, &h);
                        rb->lcd_putsxy(LCD_WIDTH/2-w/2, LCD_HEIGHT/2-3*h/2,
                                       " Split 2 ");
                        rb->lcd_update_rect(LCD_WIDTH/2-w/2, LCD_HEIGHT/2-3*h/2,
                                            w,h);
                        rb->sleep(HZ*2);
                    }
                    else {
                        bj->end_hand = false;
                        bj->split_status = 3;
                        temp_var = bj->player_total;
                        bj->player_total = bj->player_cards[1][0].value;
                        update_total(bj);
                        redraw_board(bj);
                        player_x += 10;
                        rb->lcd_update();
                    }
                }                
                else
                    finish_game(bj);
            }
        }

        if (bj->player_money < 10) {
            rb->sleep(HZ);
            return BJ_LOSE;
        }

        if (bj->end_hand) {                     /* If hand is over */
            if (play_again() != 0)              /* User wants to quit */
                return BJ_END;
            else {                              /* User keeps playing */
                breakout = false;
                redraw_board(bj);
                if(dbl_down) {
                    bj->current_bet /= 2;
                    dbl_down = false;
                }
                done = 0;
                todo = 1;
                blackjack_init(bj);
                blackjack_get_bet(bj);
                if (bj->current_bet == 0)
                    return BJ_END;
                deal_init_cards(bj);
                blackjack_drawtable(bj);
                rb->lcd_update();
            }
        }        
    }
    /* Never reached */
    return PLUGIN_OK;
}

/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    struct game_context bj;
    bool exit = false;
    unsigned int position;
    char str[19];

    (void)parameter;
    rb = api;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    /* load high scores */
    blackjack_loadscores(&bj);

    rb->lcd_setfont(FONT_SYSFIXED);

    while(!exit) {
        switch(blackjack(&bj)){
            case BJ_LOSE:
                rb->splash(HZ, "Not enough money to continue");
                /* fall through to BJ_END */

            case BJ_END:
                if(!bj.resume) {
                    if((position = blackjack_recordscore(&bj))) {
                        rb->snprintf(str, 19, "New high score #%d!", position);
                        rb->splash(HZ*2, str);
                    }
                }
                break;

            case BJ_USB:
                rb->lcd_setfont(FONT_UI);
                return PLUGIN_USB_CONNECTED;

            case BJ_QUIT:
                if(bj.dirty) {
                    rb->splash(HZ, "Saving high scores...");
                    blackjack_savescores(&bj);
                }
                exit = true;
                break;

            default:
                break;
        }
    }

    rb->lcd_setfont(FONT_UI);
    return PLUGIN_OK;
}
