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

//void put_image_native(void);
//void tv_display(void);

// contains lot of testing code
static void do_new_palette(void)
{
    int saturation = palette.saturation;
    //int min_lightness = 0;
    //int max_lightness = 100;
    int contrast = palette.contrast;
    int brightness = palette.brightness;
    int colour_shift = palette.c_shift;
    int select = 0;
    int pal_pic = 0; /* display palette or picture */
    int param = 0;
    int yuv_mode = 0;
    int pr_displacement = 0;
    int pb_displacement = 0;
    int col_step = 0;

    for (;;) {
        make_palette(yuv_mode, colour_shift, saturation, contrast, brightness,
                    pr_displacement, pb_displacement, col_step);
        if (pal_pic==0) {
            show_palette();
        }
        else {
            tv_display();
        }
        switch (param) {
            case 0:
                rb->splashf(0,"Contrast=%d, Brightness=%d",
                    contrast, brightness);
                break;
            case 1:
                rb->splashf(0,"yuv-mode=%d, Satur=%d",
                     yuv_mode, saturation);
                break;
            case 2:
                //rb->splashf(0,"YUV-Mode=%d, pr_displacement=%d", yuv_mode, pr_displacement);
                rb->splashf(0,"pr_displacement=%d, pb=%d", pr_displacement, pb_displacement);
                break;
            case 3:
                rb->splashf(0,"C-shift=%d, col_step=%d", colour_shift, col_step);
                break;
            case 4:
                rb->splashf(0,"...=%d, pb=%d", pr_displacement, pb_displacement);
                break;
        }
        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button) {
            case PLA_CANCEL:
            case PLA_EXIT:
                palette.saturation = saturation;
                palette.contrast = contrast;
                palette.brightness = brightness;
                palette.c_shift = colour_shift;
                return;
            case PLA_SELECT:
                ++select;
                if (select >= 8) select = 0;
                pal_pic = select & 0x01;
                param = select >> 1;
                break;
            case PLA_LEFT:
            case PLA_LEFT_REPEAT:
                switch (param) {
                    case 0:
                        contrast -= 1;
                        break;
                    case 1:
                        yuv_mode--;
                        break;
                    case 2:
                        pb_displacement--;
                        break;
                    case 3:
                        colour_shift -= 1;
                        break;
                }
                break;
            case PLA_RIGHT:
            case PLA_RIGHT_REPEAT:
                switch (param) {
                    case 0:
                        contrast += 1;
                        break;
                    case 1:
                        yuv_mode++;
                        break;
                    case 2:
                        pb_displacement++;
                        break;
                    case 3:
                        colour_shift += 1;
                        break;
                }
                break;
            case PLA_UP:
            case PLA_UP_REPEAT:
                switch (param) {
                    case 0:
                        brightness -= 1;
                        break;
                    case 1:
                        saturation -= 1;
                        //if (saturation < 0) saturation = 0;
                        break;
                    case 2:
                        pr_displacement--;
                        break;
                    case 3:
                        col_step--;
                        break;
                }
                break;
            case PLA_DOWN:
            case PLA_DOWN_REPEAT:
                switch (param) {
                    case 0:
                        brightness += 1;
                        break;
                    case 1:
                        saturation += 1;
                        //if (saturation > 100) saturation = 100;
                        break;
                    case 2:
                        pr_displacement++;
                        break;
                    case 3:
                        col_step++;
                        break;
                }
                break;
        }
    }

    palette.saturation = saturation;
    palette.contrast = contrast;
    palette.brightness = brightness;
    palette.c_shift = colour_shift;
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
                set_picture_size(0);
                break;
            case MENU_SET_PAL:
                rb->splash(HZ, "PAL defaults loaded");
                init_screen(TV_PAL);
                set_picture_size(0);
                break;
            case MENU_SET_SECAM:
                rb->splash(HZ, "SECAM defaults loaded");
                init_screen(TV_SECAM);
                set_picture_size(0);
                break;
            default:
                exit_menu=true;
                break;
        }
    }
    return ret;
}


int do_screen_menu(void)
{
    bool exit_menu=false;
    int  new_setting;
    int selected=0;
    int result;
    int ret = 0;

    static const struct opt_items max_frameskip[]= {
        { "0", -1 },
        { "1", -1 },
        { "2", -1 },
        { "3", -1 },
        { "4", -1 },
        { "5", -1 },
        { "6", -1 },
        { "7", -1 },
        { "8", -1 },
        { "9", -1 },
        { "10", -1 },
    };

    static const struct opt_items tv_refresh_rate[] = {
        { "50 Hz (PAL, SECAM)", -1 },
        { "60 Hz (NTSC)", -1 },
        { "Unlimited", -1 },
    };

    static const struct opt_items tv_zoom[] = {
        { "Native Size", -1 },
        { "Full Screen", -1 },
        { "Custom Zoom", -1 },
        { "Set Custom Zoom Size", -1 },
    };

    MENUITEM_STRINGLIST(menu, "Screen Menu", NULL,
        "Set TV System",
        "Toggle Show FPS",
        "Max. Frameskip",
        "Make new palette",
        "Screen size (Native/Full)",
        "TV Refresh Rate",
        "Render All/200 lines",
    );
    enum {
        MENU_SET_TV_SYSTEM = 0,
        MENU_SHOW_FPS,
        MENU_MAX_FRAMESKIP,
        MENU_MAKEPALETTE,
        MENU_SCREENSIZE,
        MENU_TV_REFRESH_RATE,
        MENU_RENDER_ALL,
    };

    while(!exit_menu)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);
        switch (result) {
            case MENU_SET_TV_SYSTEM:
                menu_set_tv_system();
                break;
            case MENU_SHOW_FPS:
                lcd.show_fps = !lcd.show_fps;
                exit_menu=true;
                ret = 1;    /* go back to game */
                break;
            case MENU_MAX_FRAMESKIP:
                new_setting = lcd.max_frameskip;
                rb->set_option("Frameskip", &new_setting, INT, max_frameskip,
                    ARRAY_LEN(max_frameskip), NULL );
                //lcd.max_frameskip = new_setting;
                set_frameskip(new_setting);
                ret = 1;    /* go back to game */
                break;
            case MENU_MAKEPALETTE:
                do_new_palette();
                break;
            case MENU_SCREENSIZE:
                new_setting = lcd.show_fullscreen;
                rb->set_option("Picture Size", &new_setting, INT, tv_zoom,
                    ARRAY_LEN(tv_zoom), NULL );
                lcd.show_fullscreen = new_setting;
#if 1
                set_picture_size(new_setting);
#else
                lcd.y_start = (tv.vblank-tv.vsync+tv.height/2)-LCD_HEIGHT/2;
                if (lcd.y_start < 0) lcd.y_start = 0;
                lcd.y_end = lcd.y_start + LCD_HEIGHT;
#endif
                exit_menu=true;
                ret = 1;    /* go back to game */
                break;
            case MENU_TV_REFRESH_RATE:
                new_setting = lcd.lockfps==0 ? 2 : lcd.lockfps/10-5;
                rb->set_option("TV Refresh Rate", &new_setting, INT, tv_refresh_rate,
                    ARRAY_LEN(tv_refresh_rate), NULL );
                lcd.lockfps = new_setting==2 ? 0 : 50+new_setting*10;
                break;
            case MENU_RENDER_ALL:
                if (lcd.render_start) {
                    lcd.render_start = 0;
                    lcd.render_end = 320;
                }
                else {
                    // TODO: take numbers from lcd. or tv.-setting
                    lcd.render_start = 35;
                    lcd.render_end = 232;
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
