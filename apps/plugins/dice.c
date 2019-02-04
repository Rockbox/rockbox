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
#include "lib/pluginlib_actions.h"
#include "lib/configfile.h"
#include "lib/playback_control.h"

#define MAX_DICES 12
#define INITIAL_NB_DICES 1
#define INITIAL_NB_SIDES 2 /* corresponds to 6 sides in the array */

#define DICE_QUIT PLA_CANCEL
#define DICE_ROLL PLA_SELECT


#define CFG_FILE "dice.cfg"

const struct button_mapping* plugin_contexts[]={pla_main_ctx};

struct dices
{
    int values[MAX_DICES];
    int total;
    int nb_dices;
    int nb_sides;
};

#define PRINT_BUFFER_LENGTH MAX_DICES*4


static struct dices dice;
static int sides_index;

static struct opt_items nb_sides_option[8] = {
    { "3", TALK_ID(3, UNIT_INT) },
    { "4", TALK_ID(4, UNIT_INT) },
    { "6", TALK_ID(6, UNIT_INT) },
    { "8", TALK_ID(8, UNIT_INT) },
    { "10", TALK_ID(10, UNIT_INT) },
    { "12", TALK_ID(12, UNIT_INT) },
    { "20", TALK_ID(20, UNIT_INT) },
    { "100", TALK_ID(100, UNIT_INT) }
};
static int nb_sides_values[] = { 3, 4, 6, 8, 10, 12, 20, 100 };
static char *sides_conf[] = {"3", "4", "6", "8", "10", "12", "20", "100" };
static struct configdata config[] =
{
    {TYPE_INT, 0, MAX_DICES, { .int_p = &dice.nb_dices}, "dice count", NULL},
    {TYPE_ENUM, 0, 8, { .int_p = &sides_index }, "side count", sides_conf}
};

static void dice_init(struct dices* dice);
static void dice_roll(struct dices* dice);
static void dice_print(struct dices* dice, struct screen* display);
static bool dice_menu(struct dices* dice);

/* plugin entry point */
enum plugin_status plugin_start(const void* parameter) {
    (void)parameter;
    int action;

    dice_init(&dice);
    rb->srand(*rb->current_tick);

    configfile_load(CFG_FILE, config, 2, 0);
    dice.nb_sides = nb_sides_values[sides_index];
    if(!dice_menu(&dice))
    {
        configfile_save(CFG_FILE, config, 2, 0);
        return PLUGIN_OK;
    }
    configfile_save(CFG_FILE, config, 2, 0);
    dice_roll(&dice);
    FOR_NB_SCREENS(i)
        dice_print( &dice, rb->screens[i] );
    while(true) {
        action = pluginlib_getaction(TIMEOUT_BLOCK,
                                     plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(action) {
            case DICE_ROLL:
                dice_roll(&dice);
                FOR_NB_SCREENS(i)
                    dice_print( &dice, rb->screens[i] );
                break;
            case DICE_QUIT:
                return PLUGIN_OK;
        }
    }
}

static void dice_init(struct dices* dice){
    dice->nb_dices=INITIAL_NB_DICES;
    sides_index=INITIAL_NB_SIDES;

}

static void dice_roll(struct dices* dice) {
    int i;
    dice->total = 0;
    for (i=0; i<dice->nb_dices; i++) {
        dice->values[i] = rb->rand()%dice->nb_sides + 1;
        dice->total+=dice->values[i];
    }
}

static void dice_print_string_buffer(struct dices* dice, char* buffer,
                              int start, int end){
    int i, written;
    for (i=start; i<end; i++) {
        written=rb->snprintf(buffer, PRINT_BUFFER_LENGTH,
                             " %3d", dice->values[i]);
        buffer=&(buffer[written]);
    }
}

static void dice_print(struct dices* dice, struct screen* display){
    char buffer[PRINT_BUFFER_LENGTH];
    /* display characteristics */
    int char_height, char_width;
    display->getstringsize("M", &char_width, &char_height);
    int display_nb_row=display->getheight()/char_height;
    int display_nb_col=display->getwidth()/char_width;

    int nb_dices_per_line=display_nb_col/4;/* 4 char per dice displayed*/
    if(!nb_dices_per_line)
        nb_dices_per_line++;
    
    int nb_lines_required=dice->nb_dices/nb_dices_per_line;
    int current_row=0;
    if(dice->nb_dices%nb_dices_per_line!=0)
        nb_lines_required++;
    display->clear_display();
    if(display_nb_row<nb_lines_required){
        /* Put everything on the same scrolling line */
        dice_print_string_buffer(dice, buffer, 0, dice->nb_dices);
        display->puts_scroll(0, current_row, buffer);
        current_row++;
    }else{
        int start=0;
        int end=0;
        for(;current_row<nb_lines_required;current_row++){
            end=start+nb_dices_per_line;
            if(end>dice->nb_dices)
                end=dice->nb_dices;
            dice_print_string_buffer(dice, buffer, start, end);
            display->puts(0, current_row, buffer);
            start=end;
        }
    }
    rb->snprintf(buffer, PRINT_BUFFER_LENGTH, "Total: %d", dice->total);
    display->puts_scroll(0, current_row, buffer);
    display->update();
}

static bool dice_menu(struct dices * dice) {
    int selection;
    bool menu_quit = false, result = false;

    MENUITEM_STRINGLIST(menu, "Dice Menu", NULL,
                        "Roll Dice",
                        "Number of Dice", "Number of Sides",
                        "Playback Control", "Quit");


    while (!menu_quit) {
        switch(rb->do_menu(&menu, &selection, NULL, false)){
            case 0:
                menu_quit = true;
                result = true;
                break;

            case 1:
                rb->set_int("Number of Dice", "", UNIT_INT, &(dice->nb_dices),
                            NULL, 1, 1, MAX_DICES, NULL );
                break;

            case 2:
                rb->set_option("Number of Sides", &sides_index, INT, 
                               nb_sides_option,
                               sizeof(nb_sides_values)/sizeof(int), NULL);
                dice->nb_sides=nb_sides_values[sides_index];
                break;

            case 3:
                playback_control(NULL);
                break;

            default:
                menu_quit = true;
                result = false;
                break;
        }
    }
    return result;
}
