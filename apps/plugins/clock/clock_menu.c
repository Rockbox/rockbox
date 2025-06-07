/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Zakk Roberts
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

#include "clock.h"
#include "clock_bitmaps.h"
#include "clock_settings.h"
#include "clock_menu.h"
#include "lib/playback_control.h"

/* Option structs (possible selections per each option) */
static const struct opt_items noyes_text[] = {
    { "No", -1 },
    { "Yes", -1 }
};

static const struct opt_items backlight_settings_text[] = {
    { "Always Off", -1 },
    { "Use System Setting", -1 },
    { "Always On", -1 }
};

static const struct opt_items idle_poweroff_text[] = {
    { "Disabled", -1 },
    { "Enabled", -1 }
};

static const struct opt_items date_format_text[] = {
    { "No date", -1 },
    { "US (M-D-Y)", -1 },
    { "European (D-M-Y)", -1 },
    { "Japanese (Y-M-D)", -1 },
};

/***************
 * Select a mode, returs true when the mode has been selected
 * (we go back to clock display then)
 **************/
static bool menu_mode_selector(void){
    MENUITEM_STRINGLIST(menu,"Mode",NULL, "Analog",
                        "Digital", "Binary");
    if(rb->do_menu(&menu, &clock_settings.mode, NULL, false) >=0)
        return(true);
    return(false);
}

/**********************
 * Analog settings menu
 *********************/
static void menu_analog_settings(void)
{
    int selection=0, result=0;

    MENUITEM_STRINGLIST(menu,"Analog",NULL,"Show Date",
                        "Show Second Hand","Show Border");

    while(result>=0){
        result=rb->do_menu(&menu, &selection, NULL, false);
        switch(result){
            case 0:
                rb->set_option("Show Date", &clock_settings.analog.show_date,
                               RB_BOOL, noyes_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Show Second Hand",
                               &clock_settings.analog.show_seconds,
                               RB_BOOL, noyes_text, 2, NULL);
                break;
            case 2:
                rb->set_option("Show Border",
                               &clock_settings.analog.show_border,
                               RB_BOOL, noyes_text, 2, NULL);
                break;
        }
    }
}

/***********************
 * Digital settings menu
 **********************/
static void menu_digital_settings(void){
    int selection=0, result=0;

    MENUITEM_STRINGLIST(menu,"Digital",NULL,"Show Seconds",
                        "Blinking Colon");

    while(result>=0){
        result=rb->do_menu(&menu, &selection, NULL, false);
        switch(result){
            case 0:
                rb->set_option("Show Seconds",
                               &clock_settings.digital.show_seconds,
                               RB_BOOL, noyes_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Blinking Colon",
                               &clock_settings.digital.blinkcolon,
                               RB_BOOL, noyes_text, 2, NULL);
                break;
        }
    }
}

/************************************
 * General settings. Reset, save, etc
 ***********************************/
static int menu_general_settings(void){
    int selection=0, result=0;

    MENUITEM_STRINGLIST(menu, "Clock Settings", NULL,
                        "Analog", "Digital",
                        "Date Format", "Backlight", "Idle Poweroff",
                        "Save on Exit", "Save", "Reset");

    while(result>=0){
        result=rb->do_menu(&menu, &selection, NULL, false);
        switch(result){
            case 0:
                menu_analog_settings();
                break;
            case 1:
                menu_digital_settings();
                break;
            case 2:
                rb->set_option("Date Format",
                               &clock_settings.general.date_format,
                               RB_INT, date_format_text, 4, NULL);
                break;
            case 3:
                rb->set_option("Backlight",
                               &clock_settings.general.backlight,
                               RB_INT, backlight_settings_text, 3, NULL);
                apply_backlight_setting(clock_settings.general.backlight);
                break;

            case 4:
                rb->set_option("Idle Poweroff",
                               &clock_settings.general.idle_poweroff,
                               RB_BOOL, idle_poweroff_text, 2, NULL);
                break;
            case 5:
                rb->set_option("Save on Exit",
                               &clock_settings.general.save_settings,
                               RB_BOOL, noyes_text, 2, NULL);

                /* if we no longer save on exit,
                   we better save now to remember that */
                if(!clock_settings.general.save_settings)
                    save_settings_wo_gui();
                break;
            case 6:
                save_settings_wo_gui();
                rb->splash(HZ, ID2P(LANG_SETTINGS_SAVED));
                break;
            case 7:
                if (rb->yesno_pop_confirm(ID2P(LANG_RESET)))
                {
                    clock_settings_reset(&clock_settings);
                    return 1;
                }
                break;
        }
    }
    return 0;
}

/***********
 * Main menu
 **********/
bool main_menu(void)
{
    int selection = 0;
    bool done = false;
    bool exit_clock=false;

    MENUITEM_STRINGLIST(menu, "Clock", NULL, "Mode",
                        "Settings","Playback Control",
                        "Quit");
    while (!done)
    {
        switch(rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0:
                done=menu_mode_selector();
                break;
            case 1:
                if (menu_general_settings())
                    done = true;
                break;
            case 2:
                playback_control(NULL);
                break;
            case 3:
                exit_clock = true;
                done = true;
                break;
            default:
                done=true;
                break;
        }
    }
    return(exit_clock);
}
