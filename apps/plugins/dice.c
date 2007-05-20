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

#define MAX_DICE 12
#define NUM_SIDE_CHOICES 8
#if (CONFIG_KEYPAD == PLAYER_PAD)
    #define START_DICE_ROW 1
    #define ROWS 1
    #define LINE_LENGTH 50
    #define SELECTIONS_ROW 0
#else
    #if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == ONDIO_PAD)
        #define START_DICE_ROW 0
        #define ROWS 3
        #define LINE_LENGTH 18
    #else
        #define START_DICE_ROW 0
        #define ROWS 2
        #define LINE_LENGTH 26
    #endif
#endif

#define min(x,y) (x<y?x:y)
#define max(x,y) (x>y?x:y)

#if CONFIG_KEYPAD == RECORDER_PAD
#define DICE_BUTTON_ON         BUTTON_ON
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_SELECT     BUTTON_PLAY

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define DICE_BUTTON_ON         BUTTON_ON
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_SELECT     BUTTON_SELECT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define DICE_BUTTON_ON        (BUTTON_MENU|BUTTON_OFF)
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_SELECT    (BUTTON_MENU|BUTTON_REL)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define DICE_BUTTON_ON         BUTTON_ON
#define DICE_BUTTON_OFF        BUTTON_OFF
#define DICE_BUTTON_SELECT     BUTTON_SELECT

#define DICE_BUTTON_RC_OFF     BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
#define DICE_BUTTON_ON        (BUTTON_SELECT|BUTTON_PLAY)
#define DICE_BUTTON_OFF       (BUTTON_PLAY|BUTTON_REPEAT)
#define DICE_BUTTON_SELECT     BUTTON_SELECT

#elif CONFIG_KEYPAD == PLAYER_PAD
#define DICE_BUTTON_ON        (BUTTON_ON|BUTTON_REPEAT)
#define DICE_BUTTON_OFF        BUTTON_MENU
#define DICE_BUTTON_SELECT     BUTTON_ON

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define DICE_BUTTON_ON         BUTTON_PLAY
#define DICE_BUTTON_OFF        BUTTON_POWER
#define DICE_BUTTON_SELECT     BUTTON_SELECT
 
#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define DICE_BUTTON_ON         BUTTON_A
#define DICE_BUTTON_OFF        BUTTON_POWER
#define DICE_BUTTON_SELECT     BUTTON_SELECT

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define DICE_BUTTON_ON         BUTTON_UP
#define DICE_BUTTON_OFF        BUTTON_POWER
#define DICE_BUTTON_SELECT     BUTTON_SELECT

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define DICE_BUTTON_ON         BUTTON_PLAY
#define DICE_BUTTON_OFF        BUTTON_POWER
#define DICE_BUTTON_SELECT     BUTTON_REW
 
#else
    #error DICE: Unsupported keypad
#endif




PLUGIN_HEADER
                           /* 0, 1, 2, 3,  4,  5,  6,   7 */
static const int SIDES[] = { 3, 4, 6, 8, 10, 12, 20, 100 };

static struct plugin_api* rb;

static int roll_dice(int dice[], const int num_dice, const int side_index);
static void print_dice(const int dice[], const int total);
static bool dice_menu(int *num_dice, int *side_index);

/* plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    int side_index = 6;
    int num_dice = 1;
    bool exit = false;
    int dice[MAX_DICE];
    int total;

    /* plugin init */
    (void)parameter;
    rb = api;

    rb->lcd_clear_display();
    rb->srand(*rb->current_tick);
    /* end of plugin init */
    
    if(!dice_menu(&num_dice, &side_index))
        exit = true;
    else {
        total = roll_dice(dice, num_dice, side_index);
        print_dice(dice, total);
     }
    while(!exit) {
        switch(rb->button_get(true)) {
            case DICE_BUTTON_ON:
            case DICE_BUTTON_SELECT:
                total = roll_dice(dice, num_dice, side_index);
                print_dice(dice, total);
                break;
#ifdef DICE_BUTTON_RC_OFF
            case DICE_BUTTON_RC_OFF:
#endif
            case DICE_BUTTON_OFF:
                exit=true;
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

/* Prints the dice, and the sum of the dice values */
static void print_dice(const int dice[], const int total) {
    const int dice_per_row = MAX_DICE/ROWS + (MAX_DICE%ROWS?1:0);
    char showdice[LINE_LENGTH];
    int row;
    rb->lcd_clear_display();
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
    rb->lcd_update();
}
static bool dice_menu(int *num_dice, int *side_index) {
    int selection;
    bool menu_quit = false, result = false;
    
    MENUITEM_STRINGLIST(menu,"Dice Menu",NULL,"Roll Dice","Number of Dice",
                        "Number of Sides","Quit");

    static const struct opt_items num_sides_option[8] = {
        { "3", -1 },
        { "4", -1 },
        { "6", -1 },
        { "8", -1 },
        { "10", -1 },
        { "12", -1 },
        { "20", -1 },
        { "100", -1 }
    };
                      
    while (!menu_quit) {
        switch(rb->do_menu(&menu, &selection))
        {
            case 0:
                menu_quit = true;
                result = true;
                break;
                
            case 1:
                rb->set_int("Number of Dice", "", UNIT_INT, num_dice, NULL, 
                            1, 1, MAX_DICE, NULL );
                break;
                
            case 2:
                rb->set_option("Number of Sides", side_index, INT, 
                               num_sides_option, 8, NULL);
                break;
                
            default:
                menu_quit = true;
                result = false;
                break;
         }
    }

    return result;
}
