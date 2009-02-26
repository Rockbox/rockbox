/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: jackpot.c 14034 2007-07-28 05:42:55Z kevin $
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
#include "lib/playback_control.h"

/* Option structs (possible selections per each option) */
static const struct opt_items noyes_text[] = {
    { "No", -1 },
    { "Yes", -1 }
};

static const struct opt_items backlight_settings_text[] = {
    { "Always Off", -1 },
    { "Use Rockbox Setting", -1 },
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

static const struct opt_items hour_format_text[] = {
    { "24-Hour", -1 },
    { "12-Hour", -1 }
};

/***************
 * Select a mode, returs true when the mode has been selected
 * (we go back to clock display then)
 **************/
bool menu_mode_selector(void){
    MENUITEM_STRINGLIST(menu,"Mode Selector",NULL, "Analog",
                        "Digital", "Binary");
    if(rb->do_menu(&menu, &clock_settings.mode, NULL, false) >=0)
        return(true);
    return(false);
}

/**********************
 * Analog settings menu
 *********************/
void menu_analog_settings(void)
{
    int selection=0, result=0;

    MENUITEM_STRINGLIST(menu,"Analog Mode Settings",NULL,"Show Date",
                        "Show Second Hand","Show Border");

    while(result>=0){
        result=rb->do_menu(&menu, &selection, NULL, false);
        switch(result){
            case 0:
                rb->set_option("Show Date", &clock_settings.analog.show_date,
                               BOOL, noyes_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Show Second Hand",
                               &clock_settings.analog.show_seconds,
                               BOOL, noyes_text, 2, NULL);
                break;
            case 2:
                rb->set_option("Show Border",
                               &clock_settings.analog.show_border,
                               BOOL, noyes_text, 2, NULL);
                break;
        }
    }
}

/***********************
 * Digital settings menu
 **********************/
void menu_digital_settings(void){
    int selection=0, result=0;

    MENUITEM_STRINGLIST(menu,"Digital Mode Settings",NULL,"Show Seconds",
                        "Blinking Colon");

    while(result>=0){
        result=rb->do_menu(&menu, &selection, NULL, false);
        switch(result){
            case 0:
                rb->set_option("Show Seconds",
                               &clock_settings.digital.show_seconds,
                               BOOL, noyes_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Blinking Colon",
                               &clock_settings.digital.blinkcolon,
                               BOOL, noyes_text, 2, NULL);
                break;
        }
    }
}

/***********************************************************
 * Confirm resetting of settings, used in general_settings()
 **********************************************************/
void confirm_reset(void){
    int result=0;

    rb->set_option("Reset all settings?", &result, INT, noyes_text, 2, NULL);

    if(result == 1){ /* reset! */
        clock_settings_reset(&clock_settings);
        rb->splash(HZ, "Settings reset!");
    }
    else
        rb->splash(HZ, "Settings NOT reset.");
}

/************************************
 * General settings. Reset, save, etc
 ***********************************/
void menu_general_settings(void){
    int selection=0, result=0;

    MENUITEM_STRINGLIST(menu,"General Settings",NULL,
                        "Hour format","Date format","Show Counter",
                        "Reset Settings","Save Settings Now",
                        "Save On Exit","Backlight Settings",
                        "Idle Poweroff (temporary)");

    while(result>=0){
        result=rb->do_menu(&menu, &selection, NULL, false);
        switch(result){
            case 0:
                rb->set_option("Hour format",
                               &clock_settings.general.hour_format,
                               INT, hour_format_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Date format",
                               &clock_settings.general.date_format,
                               INT, date_format_text, 4, NULL);
                break;
            case 2:
                rb->set_option("Show Counter", &clock_settings.general.show_counter,
                               BOOL, noyes_text, 2, NULL);
                break;
            case 3:
                confirm_reset();
                break;

            case 4:
                save_settings_wo_gui();
                rb->splash(HZ, "Settings saved");
                break;

            case 5:
                rb->set_option("Save On Exit",
                               &clock_settings.general.save_settings,
                               BOOL, noyes_text, 2, NULL);

                /* if we no longer save on exit,
                   we better save now to remember that */
                if(!clock_settings.general.save_settings)
                    save_settings_wo_gui();
                break;
            case 6:
                rb->set_option("Backlight Settings",
                               &clock_settings.general.backlight,
                               INT, backlight_settings_text, 3, NULL);
                apply_backlight_setting(clock_settings.general.backlight);
                break;

            case 7:
                rb->set_option("Idle Poweroff (temporary)",
                               &clock_settings.general.idle_poweroff,
                               BOOL, idle_poweroff_text, 2, NULL);
                break;
        }
    }
}

/***********
 * Main menu
 **********/
bool main_menu(void){
    int selection=0;
    bool done = false;
    bool exit_clock=false;

    MENUITEM_STRINGLIST(menu,"Clock Menu",NULL,"View Clock","Mode Selector",
                        "Mode Settings","General Settings","Playback Control",
                        "Quit");

    while(!done){
        switch(rb->do_menu(&menu, &selection, NULL, false)){
            case 0:
                done = true;
                break;

            case 1:
                done=menu_mode_selector();
                break;

            case 2:
                switch(clock_settings.mode){
                    case ANALOG: menu_analog_settings();break;
                    case DIGITAL: menu_digital_settings();break;
                    case BINARY: /* no settings */;break;
                }
                break;

            case 3:
                menu_general_settings();
                break;

            case 4:
                playback_control(NULL);
                break;

            case 5:
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
