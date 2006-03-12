/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Brandon Low
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************************************************************
Dice by lostlogic

use left and right to pick between sides and number of dice
use up and down to select number of sides or number of dice
use select or play to roll the dice
use stop to exit

*****************************************************************************/

#include "plugin.h"
#include "button.h"
#include "lcd.h"

#define MAX_DICE 12
#define NUM_SIDE_CHOICES 8
#if (CONFIG_KEYPAD == PLAYER_PAD)
    #define SELECTIONS_SIZE 9
    #define START_DICE_ROW 1
    #define ROWS 1
    #define LINE_LENGTH 50
    #define SELECTIONS_ROW 0
#else
    #define SELECTIONS_SIZE 7
    #define SELECTIONS_ROW 4
    #if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == ONDIO_PAD)
        #define START_DICE_ROW 0
        #define ROWS 3
        #define LINE_LENGTH 18
    #else
        #define START_DICE_ROW 7
        #define ROWS 2
        #define LINE_LENGTH 26
    #endif
#endif

/* Values for selected */
#define CHANGE_DICE 0
#define CHANGE_SIDES 1
#define EXIT 2



#if CONFIG_KEYPAD == RECORDER_PAD
#define DICE_BUTTON_UP         BUTTON_UP
#define DICE_BUTTON_DOWN       BUTTON_DOWN
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_ON         BUTTON_ON
#define DICE_BUTTON_SELECT     BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define DICE_BUTTON_UP         BUTTON_UP
#define DICE_BUTTON_DOWN       BUTTON_DOWN
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_SELECT    (BUTTON_MENU|BUTTON_REL)
#define DICE_BUTTON_ON        (BUTTON_MENU|BUTTON_OFF)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define DICE_BUTTON_UP         BUTTON_UP
#define DICE_BUTTON_DOWN       BUTTON_DOWN
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_ON         BUTTON_ON
#define DICE_BUTTON_SELECT     BUTTON_SELECT
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
#define DICE_BUTTON_UP         BUTTON_SCROLL_FWD
#define DICE_BUTTON_DOWN       BUTTON_SCROLL_BACK
#define DICE_BUTTON_OFF       (BUTTON_PLAY|BUTTON_REPEAT)
#define DICE_BUTTON_ON        (BUTTON_SELECT|BUTTON_PLAY)
#define DICE_BUTTON_SELECT     BUTTON_SELECT
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT

#elif (CONFIG_KEYPAD == PLAYER_PAD)
#define DICE_BUTTON_UP         BUTTON_PLAY
#define DICE_BUTTON_DOWN       BUTTON_STOP
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT
#define DICE_BUTTON_SELECT     BUTTON_ON
#define DICE_BUTTON_ON        (BUTTON_ON|BUTTON_REPEAT)
#define DICE_BUTTON_OFF        BUTTON_MENU

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define DICE_BUTTON_UP         BUTTON_UP
#define DICE_BUTTON_DOWN       BUTTON_DOWN
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT
#define DICE_BUTTON_SELECT     BUTTON_SELECT
#define DICE_BUTTON_ON         BUTTON_PLAY
#define DICE_BUTTON_OFF        BUTTON_POWER
 
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define DICE_BUTTON_UP         BUTTON_UP
#define DICE_BUTTON_DOWN       BUTTON_DOWN
#define DICE_BUTTON_LEFT       BUTTON_LEFT
#define DICE_BUTTON_RIGHT      BUTTON_RIGHT
#define DICE_BUTTON_SELECT     BUTTON_SELECT
#define DICE_BUTTON_ON         BUTTON_POWER
#define DICE_BUTTON_OFF        BUTTON_A

#else
    #error DICE: Unsupported keypad
#endif




PLUGIN_HEADER
                           /* 0, 1, 2, 3,  4,  5,  6,   7 */
static const int SIDES[] = { 3, 4, 6, 8, 10, 12, 20, 100 };

static struct plugin_api* rb;

static int roll_dice(int dice[], const int num_dice, const int side_index);
static void print_dice(const int dice[], const int total);
static void print_selections(const int selected,
        const int num_dice,
        const int side_index);
static void print_help(void);

/* plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    int side_index = 6;
    int num_dice = 1;
    int selected = CHANGE_DICE;

    /* plugin init */
    (void)parameter;
    rb = api;

    rb->lcd_clear_display();
    rb->srand(*rb->current_tick);
    /* end of plugin init */

    print_selections(selected, num_dice, side_index);
    print_help();
    while(selected!=EXIT) {
        switch(rb->button_get(true)) {
            case DICE_BUTTON_UP:
                if (selected==CHANGE_DICE) {
                    num_dice%=MAX_DICE;
                    num_dice++;
                } else if (selected==CHANGE_SIDES) {
                    side_index++;
                    side_index%=NUM_SIDE_CHOICES;
                }
                print_selections(selected, num_dice, side_index);
                break;

            case DICE_BUTTON_DOWN:
                if (selected==CHANGE_DICE) {
                    num_dice--;
                    if (num_dice==0) {
                        num_dice=MAX_DICE;
                    }
                } else if (selected==CHANGE_SIDES) {
                    if (side_index==0) {
                        side_index=NUM_SIDE_CHOICES;
                    }
                    side_index--;
                }
                print_selections(selected, num_dice, side_index);
                break;

            case DICE_BUTTON_LEFT:
                selected = CHANGE_DICE;
                print_selections(selected, num_dice, side_index);
                break;

            case DICE_BUTTON_RIGHT:
                selected = CHANGE_SIDES;
                print_selections(selected, num_dice, side_index);
                break;

            case DICE_BUTTON_ON:
            case DICE_BUTTON_SELECT:
                {
                    int dice[MAX_DICE];
                    int total = roll_dice(dice, num_dice, side_index);
                    print_dice(dice, total);
                }
                break;

            case DICE_BUTTON_OFF:
                selected = EXIT;
                break;

            default:
                break;
        } /* switch */
    } /* while */

    return PLUGIN_OK;
}

/* Clears the dice array, rolls the dice and returns the sum */
static int roll_dice(int dice[], const int num_dice, const int side_index) {
    int i;
    int total = 0;
    rb->memset((void*)dice,0,MAX_DICE*sizeof(int));
    for (i=0; i<num_dice; i++) {
        dice[i] = rb->rand()%SIDES[side_index] + 1;
        total+=dice[i];
    }
    return total;
}

#define min(x,y) (x<y?x:y)
#define max(x,y) (x>y?x:y)

/* Prints the dice, and the sum of the dice values */
static void print_dice(const int dice[], const int total) {
    const int dice_per_row = MAX_DICE/ROWS + (MAX_DICE%ROWS?1:0);
    char showdice[LINE_LENGTH];
    int row;
    for (row=0; /*break;*/; row++) {
        const int start = row*dice_per_row;
        const int end = min(MAX_DICE,start+dice_per_row);
        char *pointer = showdice;
        int space = LINE_LENGTH;
        int i;
        rb->memset((void*)showdice,0,LINE_LENGTH*sizeof(char));
        for (i=start; i<end && dice[i]>0; i++) {
            int count = rb->snprintf(pointer,space," %3d",dice[i]);
            pointer = &pointer[count];
            space -= count;
        }
        if (i > start) {
            rb->lcd_puts_scroll(0,START_DICE_ROW+row,showdice);
        } else {
            row--;
            break;
        }
        if (i < end || end == MAX_DICE) break;
    }
#if (CONFIG_KEYPAD == PLAYER_PAD)
    (void)total;
#else
    rb->lcd_puts(0,START_DICE_ROW+(++row)," ");
    if (total > 0) {
        rb->snprintf(showdice,LINE_LENGTH,"Total: %4d",total);
        rb->lcd_puts(1,START_DICE_ROW+(++row),showdice);
    }
    while (row < ROWS+2) {
        rb->lcd_puts(0,START_DICE_ROW+(++row)," ");
    }
#endif
#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif
}

/* Print the current user input choices */
static void print_selections(const int selected,
        const int num_dice,
        const int side_index) {
    char buffer[SELECTIONS_SIZE];
#if (CONFIG_KEYPAD == PLAYER_PAD)
    rb->snprintf(buffer, SELECTIONS_SIZE, "%c%2dd%c%3d",
            selected==CHANGE_DICE?'*':' ', num_dice,
            selected==CHANGE_SIDES?'*':' ', SIDES[side_index]);
    rb->lcd_puts(1,SELECTIONS_ROW,buffer);
#else
    rb->snprintf(
            buffer,SELECTIONS_SIZE,"%2dd%3d",num_dice,SIDES[side_index]);
    rb->lcd_puts(1,SELECTIONS_ROW,buffer);
    if (selected==CHANGE_DICE) {
        rb->lcd_puts(1,SELECTIONS_ROW+1,"--");
    } else if (selected==CHANGE_SIDES) {
        rb->lcd_puts(1,SELECTIONS_ROW+1,"   ---");
    }
#endif
#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif
}

static void print_help() {
#if (CONFIG_KEYPAD == PLAYER_PAD)
    rb->lcd_puts_scroll(1,SELECTIONS_ROW, "</>, +/- setup");
    rb->lcd_puts_scroll(1,SELECTIONS_ROW+1, "on roll, menu exit");
#else
    rb->lcd_puts(1,START_DICE_ROW,"</> pick dice/sides");
    rb->lcd_puts(1,START_DICE_ROW+1,"+/- change");
    rb->lcd_puts(1,START_DICE_ROW+2,"on/select roll");
    rb->lcd_puts(1,START_DICE_ROW+3,"off exit");
#endif
#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif
}
