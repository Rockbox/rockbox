/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
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


#include "rbconfig.h"
#include "rb_test.h"

#include "keyboard.h"
#include "files.h"
#include "config.h"
#include "vmachine.h"
#include "profile.h"
#include "options.h"

#include "memory.h"

#include "rb_lcd.h"
#include "palette.h"
#include "lib/helper.h"

#include "address.h"
#include "riot.h"
#include "rb_keymap.h"
#include "rb_menu.h"


struct rb_button input =
    { .joy1_left = JOY_LEFT,
      .joy1_right = JOY_RIGHT,
      .joy1_up = JOY_UP,
      .joy1_down = JOY_DOWN,
      .joy1_trigger = JOY_TRIGGER,
#if defined(CONSOLE_SELECT)
      .console_select = CONSOLE_SELECT,
#endif
#if defined(CONSOLE_RESET)
      .console_reset = CONSOLE_RESET,
#endif
    };


/*
 * Rockbox Menus
 */

/* =========================================================================
 * Palette stuff
 * Note: this isn't production quality yet.
 */

#include "lib/pluginlib_actions.h"
/* PLA data */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };


static void do_new_palette(void)
{
    int pal_pic = 0; /* display palette or picture */

    palette_chg_parameter(-1, 0, 0);
    for (;;) {
        setup_palette(TV_UNCHANGED);
        if (pal_pic==0) {
            show_palette();
        }
        else {
            screen.put_image();
        }
        rb->lcd_update();
        rb->splashf(0, palette_chg_parameter(0, 0, 0));

        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button) {
            case PLA_CANCEL:
            case PLA_EXIT:
                // TODO: leave old parameters if EXIT/CANCEL and apply new ones
                // on long select?
                return;
            case PLA_SELECT:
                if (pal_pic)
                    palette_chg_parameter(1, 0, 0);
                pal_pic = !pal_pic;
                break;
            case PLA_LEFT:
            case PLA_LEFT_REPEAT:
                palette_chg_parameter(0, -1, 0);
                break;
            case PLA_RIGHT:
            case PLA_RIGHT_REPEAT:
                palette_chg_parameter(0, +1, 0);
                break;
            case PLA_UP:
            case PLA_UP_REPEAT:
                palette_chg_parameter(0, 0, -1);
                break;
            case PLA_DOWN:
            case PLA_DOWN_REPEAT:
                palette_chg_parameter(0, 0, +1);
                break;
        }
    }
}

/* -------------------------------------------------------------------------
 */

static void set_custom_zoom(void)
{
    set_picture_size(DISPLAY_MODE_ZOOM);

    zoom_chg_parameter(-1, 0, 0);
    for (;;) {
        rb->lcd_clear_display();
        screen.put_image();
        rb->lcd_update();
        rb->splashf(0, zoom_chg_parameter(0, 0, 0));

        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button) {
            case PLA_CANCEL:
            case PLA_EXIT:
                // TODO: leave old parameters if EXIT/CANCEL and apply new ones
                // on long select?
                return;
            case PLA_SELECT:
                zoom_chg_parameter(1, 0, 0);
                break;
            case PLA_LEFT:
            case PLA_LEFT_REPEAT:
                zoom_chg_parameter(0, -1, 0);
                break;
            case PLA_RIGHT:
            case PLA_RIGHT_REPEAT:
                zoom_chg_parameter(0, +1, 0);
                break;
            case PLA_UP:
            case PLA_UP_REPEAT:
                zoom_chg_parameter(0, 0, -1);
                break;
            case PLA_DOWN:
            case PLA_DOWN_REPEAT:
                zoom_chg_parameter(0, 0, +1);
                break;
        }
    }
}

/* -------------------------------------------------------------------------
 * using the atari switches
 */

// TODO: how to press select / reset if not defined in the button map?

static int menu_vcs_controls(void)
{
    bool exit_menu=false;
    int  new_setting;
    int selected=0;
    int result;
    int ret = 0;

    static const struct opt_items switch_diff[]= {
        { "B", -1 },
        { "A", -1 },
    };

    static const struct opt_items switch_colour[]= {
        { "Colour", -1 },
        { "B/W", -1 },
    };

    MENUITEM_STRINGLIST(menu, "VCS Controls", NULL,
        "Player Left Difficulty",
        "Player Right Difficulty",
        "Colour Switch",
    );
    enum {
        MENU_LEFT_DIFF = 0,
        MENU_RIGHT_DIFF,
        MENU_COLOUR_SWITCH,
    };

    while(!exit_menu)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);
        switch (result) {
            case MENU_LEFT_DIFF:
                new_setting = (riot.read[SWCHB] & 0x40) == 0 ? 0 : 1;
                rb->set_option("Left Player Difficulty", &new_setting, INT,
                    switch_diff, ARRAY_LEN(switch_diff), NULL );
                if (new_setting == 0)
                    riot.read[SWCHB] &= ~0x40;
                else
                    riot.read[SWCHB] |= 0x40; /* setting A (hard) */
                break;
            case MENU_RIGHT_DIFF:
                new_setting = (riot.read[SWCHB] & 0x80) == 0 ? 0 : 1;
                new_setting = 0; //TODO: global setting var!
                rb->set_option("Right Player Difficulty", &new_setting, INT,
                    switch_diff, ARRAY_LEN(switch_diff), NULL );
                if (new_setting == 0)
                    riot.read[SWCHB] &= ~0x80;
                else
                    riot.read[SWCHB] |= 0x80;
                break;
            case MENU_COLOUR_SWITCH:
                new_setting = (riot.read[SWCHB] & 0x08) == 0 ? 1 : 0;
                rb->set_option("VCS Colour switch", &new_setting, INT,
                    switch_colour, ARRAY_LEN(switch_colour), NULL );
                if (new_setting == 0)
                    riot.read[SWCHB] |= 0x08;
                else
                    riot.read[SWCHB] &= ~0x08;
                break;
            default:
                exit_menu = true;
                break;
        }
    }
    return ret;
}


/* -------------------------------------------------------------------------
 * Button stuff
 */

/* stolen from rockboy */
static int getbutton(char *text)
{
    int fw, fh;
    int button;
    rb->lcd_clear_display();
    rb->font_getstringsize(text, &fw, &fh,0);
    rb->lcd_putsxy(LCD_WIDTH/2-fw/2, LCD_HEIGHT/2-fh/2, text);
    rb->lcd_update();
    rb->sleep(30);

    int timeout = *rb->current_tick;
    while (rb->button_get(false) != BUTTON_NONE) {
        rb->yield();
        if (*rb->current_tick - timeout > HZ*5) {
            rb->splash(HZ/2, "BUTTON_NONE");
            break;
        }
    }

    while(true)
    {
        button = rb->button_get(true);
        button = button&(BUTTON_MAIN|BUTTON_REMOTE);

        return button;
    }
}


/* stolen from rockboy */
static void setupkeys(void)
{
    input.joy1_up      = getbutton("Press Up");
    input.joy1_down    = getbutton("Press Down");
    input.joy1_left    = getbutton("Press Left");
    input.joy1_right   = getbutton("Press Right");
    input.joy1_trigger = getbutton("Press Fire");

    input.console_select  = getbutton("Press Select");
    input.console_reset   = getbutton("Press Reset");
}


/* =========================================================================
 * Screen Menu
 */

int menu_set_tv_system(void)
{
    bool exit_menu=false;
    //int  new_setting;
    int selected=0;
    int result;
    int ret = 0;

    MENUITEM_STRINGLIST(menu, "Set TV System", NULL,
        "NTSC",
        "PAL",
        "SECAM",
    );
    enum {
        MENU_SET_NTSC = 0,
        MENU_SET_PAL,
        MENU_SET_SECAM,
    };

    while(!exit_menu) {
        result = rb->do_menu(&menu, &selected, NULL, false);
        switch (result) {
            case MENU_SET_NTSC:
                rb->splash(HZ, "NTSC defaults loaded");
                init_screen(TV_NTSC);
                setup_palette(TV_NTSC);
                set_picture_size(DISPLAY_MODE_NATIVE); // ?? better keep what was before
                break;
            case MENU_SET_PAL:
                rb->splash(HZ, "PAL defaults loaded");
                init_screen(TV_PAL);
                setup_palette(TV_PAL);
                set_picture_size(DISPLAY_MODE_NATIVE); // ??
                break;
            case MENU_SET_SECAM:
                rb->splash(HZ, "SECAM defaults loaded");
                init_screen(TV_SECAM);
                setup_palette(TV_SECAM);
                set_picture_size(DISPLAY_MODE_NATIVE); // ??
                break;
            default:
                exit_menu=true;
                break;
        }
    }
    return ret;
}

static const char* set_fps_formatter(char *buf, size_t length,
                                     int value, const char *input)
{
    (void)input;

    if (value < 0)
        return "Unlimited";
    else {
        int fps = FPS_MIN_SETTING + value * FPS_STEP_SETTING;
        char *note = "";
        if (fps == 60)
            note = "(NTSC)";
        else if (fps == 50)
            note = "(PAL, SECAM)";
        rb->snprintf(buf, length, "%d %s", fps, note);
    }
    return buf;
}


int do_screen_menu(void)
{
    bool exit_menu=false;
    int  new_setting;
    int selected=0;
    int result;
    int ret = 0;

    static const struct opt_items palette_type[] = {
        { "NTSC palette", -1 },
        { "PAL palette", -1 },
        { "SECAM palette", -1 },
    };
    enum {
        MENU_SET_NTSC = 0,
        MENU_SET_PAL,
        MENU_SET_SECAM,
    };

    static const struct opt_items tv_zoom[] = {
        { "Native Size", -1 },
        { "Full Screen", -1 },
        { "Custom Zoom", -1 },
        { "Generic display function (for testing)", -1 },
    };

    MENUITEM_STRINGLIST(menu, "Screen Menu", NULL,
        "Show FPS",
        "Max. Frameskip",
        "Palette Type",
        "Make new palette",
        "Screen size (Native/Full)",
        "Set custom zoom size",
        "Frame Rate",
        "Render All/200 lines",
    );
    enum {
        MENU_SHOW_FPS = 0,
        MENU_MAX_FRAMESKIP,
        MENU_PALETTETYPE,
        MENU_MAKEPALETTE,
        MENU_SCREENSIZE,
        MENU_SET_ZOOM,
        MENU_TV_REFRESH_RATE,
        MENU_RENDER_ALL,
    };

    while(!exit_menu)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);
        switch (result) {
            case MENU_SHOW_FPS:
                rb->set_bool("Show FPS", &screen.show_fps);
                break;
            case MENU_MAX_FRAMESKIP:
                new_setting = screen.max_frameskip;
                rb->set_int("Max. Frameskip", "", UNIT_INT, &new_setting,
                            NULL, 1, 0, FRAMESKIP_MAX_COUNT, NULL);
                set_frameskip(new_setting);
                ret = 1;    /* go back to game */
                break;
            case MENU_PALETTETYPE:
                new_setting = get_palette_type();;
                rb->set_option("Palette Type", &new_setting, INT, palette_type,
                    ARRAY_LEN(palette_type), NULL );
                setup_palette(new_setting);
                break;
            case MENU_MAKEPALETTE:
                do_new_palette();
                break;
            case MENU_SCREENSIZE:
                new_setting = get_screen_mode();
                rb->set_option("Picture Size", &new_setting, INT, tv_zoom,
                    ARRAY_LEN(tv_zoom), NULL );
                set_picture_size(new_setting);
                exit_menu=true;
                ret = 1;    /* go back to game */
                break;
            case MENU_SET_ZOOM:
                set_custom_zoom();
                set_picture_size(DISPLAY_MODE_ZOOM); // hm, must already be done BEFORE!
                break;
            case MENU_TV_REFRESH_RATE:
                new_setting = screen.lockfps == 0 ? -1 :
                              (screen.lockfps - FPS_MIN_SETTING) / FPS_STEP_SETTING;
                rb->set_int("TV Refresh Rate", "", UNIT_INT, &new_setting,
                            NULL, 1, -1,
                            (FPS_MAX_SETTING-FPS_MIN_SETTING)/FPS_STEP_SETTING,
                            set_fps_formatter);
                new_setting = new_setting < 0 ? 0 :
                              FPS_MIN_SETTING + new_setting * FPS_STEP_SETTING;
                set_framerate(new_setting);
                break;
            case MENU_RENDER_ALL:
                if (screen.render_start) {
                    screen.render_start = 0;
                    screen.render_end = 320;
                }
                else {
                    // TODO: take numbers from screen. or tv.-setting
                    screen.render_start = 35;
                    screen.render_end = 232;
                }
                break;
            default:
                exit_menu=true;
                break;
        }
    }
    return ret;
}


/* =========================================================================
 * MAIN MENU
 */

int do_a2600_menu(void)
{
    bool exit_menu = false;
    int selected=0, ret=0;
    int result;
    int  new_setting;


    static const struct opt_items bank_sw_scheme[] = {
        BSW_SCHEME_LIST_NAMES
    };

    backlight_use_settings();

    /* Clean out the button Queue */
    while (rb->button_get(false) != BUTTON_NONE)
        rb->yield();

    MENUITEM_STRINGLIST(menu, "a2600 Menu", NULL,
        "Quit",
        "Screen options",
        "Set Bank Switching Scheme",
        "Set TV System",
        "Set up keys",
        "VCS Controls",
        "Test",
        "Restart Console",
        "Quit",
    );
    enum {
        MENU_QUIT0 = 0,
        MENU_SCREEN_OPTS,
        MENU_BANK_SW,
        MENU_SET_TV_SYSTEM,
        MENU_KEYMAP,
        MENU_CONTROLS,
        MENU_TEST,
        MENU_RESTART,
        MENU_QUIT,
    };

    while(!exit_menu) {
        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result) {
            case MENU_SCREEN_OPTS:
                if (do_screen_menu())
                    exit_menu = true;
                break;
            case MENU_KEYMAP:
                setupkeys();
                break;
            case MENU_BANK_SW:
                new_setting = base_opts.bank;
                rb->set_option("Bank Switching Scheme", &new_setting, INT,
                    bank_sw_scheme, ARRAY_LEN(bank_sw_scheme), NULL );
                if (BANK_SW_ROM_SZ(new_setting) != rom_size/1024) {
                    rb->splashf(2*HZ, "WARNING: Rom size (%d k) doesn't match "
                        "switching scheme (%d k)", rom_size/1024,
                        BANK_SW_ROM_SZ(new_setting));
                }
                if (base_opts.bank != new_setting) {
                    base_opts.bank = new_setting;
                    init_memory_map();
                    rb->splash(HZ/5, "Bank Switching has changed."
                        "Press SELECT to restart console.");
                    int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
                    if (button == PLA_SELECT) {
                        init_hardware();
                    }
                }
                break;
            case MENU_SET_TV_SYSTEM:
                menu_set_tv_system();
                break;
            case MENU_CONTROLS:
                if (menu_vcs_controls())
                    exit_menu = true;
                break;
            case MENU_TEST:
                if (menu_test())
                    exit_menu = true;
                break;
            case MENU_RESTART:
                init_hardware();
                rb->splash(HZ, "Console resetted");
                break;
            case MENU_QUIT0:
            case MENU_QUIT:
                ret = USER_MENU_QUIT;
                exit_menu=true;
                break;
            default:
                exit_menu=true;
                break;
        }
    }

//  rb->lcd_setfont(FONT_SYSFIXED); /* Reset the font */
    rb->lcd_clear_display(); /* Clear display for screen size changes */

    /* ignore backlight time out */
    backlight_ignore_timeout();

    /* wait until all buttons are released */
    while (rb->button_status())
        ;

    return ret;
}
