 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#include "xrick/system/sysmenu_rockbox.h"

#include "xrick/config.h"
#include "xrick/control.h"
#include "xrick/draw.h"
#include "xrick/game.h"
#include "xrick/system/system.h"
#include "xrick/system/syssnd_rockbox.h"

#include "plugin.h"
#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif

#ifdef ENABLE_CHEATS
/*
 * Cheat settings menu
 */
static char * sysmenu_cheatItemText(int selected_item, void *data, char *buffer)
{
    (void)selected_item;
    cheat_t cheat = (cheat_t)data;
    (void)buffer;
    char * messages[] =
    {
        "Disable Unlimited Lives/Ammo Mode",
        "Enable Unlimited Lives/Ammo Mode",
        "Disable Never Die Mode",
        "Enable Never Die Mode",
        "Disable Expose Mode",
        "Enable Expose Mode"
    };

    switch (cheat)
    {
        case Cheat_UNLIMITED_ALL:
        {
            return game_cheat1? messages[0] : messages[1];
        }
        case Cheat_NEVER_DIE:
        {
            return game_cheat2? messages[2] : messages[3];
        }
        case Cheat_EXPOSE:
        {
            return game_cheat3? messages[4] : messages[5];
        }
        default: break;
    }
    return "";
}

/*
 * Callback invoked by cheat menu item
 */
static int sysmenu_doToggleCheat(void *param)
{
    cheat_t cheat = (cheat_t)param;
    game_toggleCheat(cheat);
    return 0;
}

MENUITEM_FUNCTION_DYNTEXT(sysmenu_unlimitedAllItem, MENU_FUNC_USEPARAM,
                          sysmenu_doToggleCheat, (void *)Cheat_UNLIMITED_ALL,
                          sysmenu_cheatItemText, NULL, (void *)Cheat_UNLIMITED_ALL,
                          NULL, Icon_NOICON);

MENUITEM_FUNCTION_DYNTEXT(sysmenu_neverDieItem, MENU_FUNC_USEPARAM,
                          sysmenu_doToggleCheat, (void *)Cheat_NEVER_DIE,
                          sysmenu_cheatItemText, NULL, (void *)Cheat_NEVER_DIE,
                          NULL, Icon_NOICON);

MENUITEM_FUNCTION_DYNTEXT(sysmenu_exposeItem, MENU_FUNC_USEPARAM,
                          sysmenu_doToggleCheat, (void *)Cheat_EXPOSE,
                          sysmenu_cheatItemText, NULL, (void *)Cheat_EXPOSE,
                          NULL, Icon_NOICON);

MAKE_MENU(sysmenu_cheatItems, "Cheat Settings", NULL, Icon_NOICON,
          &sysmenu_unlimitedAllItem, &sysmenu_neverDieItem, &sysmenu_exposeItem);

#endif /* ENABLE_CHEATS */

/*
 * Display main menu
 */
void sysmenu_exec(void)
{
    int result;
    bool done;

    enum
    {
        Menu_RESUME,
        Menu_RESTART,
#ifdef ENABLE_CHEATS
        Menu_CHEAT_SETTINGS,
#endif
        Menu_QUIT
    };

    MENUITEM_STRINGLIST(sysmenu_mainItems, "xrick Menu", NULL,
                    "Resume Game",
                    "Restart Game",
#ifdef ENABLE_CHEATS
                    "Cheat Settings",
#endif
                    "Quit");

#ifdef ENABLE_SOUND
    syssnd_pauseAll(true);
#endif

#ifndef HAVE_LCD_COLOR
    grey_show(false);
#endif

    done = false;
    do
    {
        rb->button_clear_queue();

        result = rb->do_menu(&sysmenu_mainItems, NULL, NULL, false);
        switch(result)
        {
            case Menu_RESUME:
            {
                done = true;
                break;
            }
            case Menu_RESTART:
            {
                control_set(Control_END);
                done = true;
                break;
            }
#ifdef ENABLE_CHEATS
            case Menu_CHEAT_SETTINGS:
            {
                rb->do_menu(&sysmenu_cheatItems, NULL, NULL, false);
                break;
            }
#endif
            case Menu_QUIT:
            {
                control_set(Control_EXIT);
                done = true;
                break;
            }
            default: break;
        }
    } while (!done);

#ifdef HAVE_LCD_COLOR
    if (!(control_test(Control_EXIT)))
    {
        rb->memset(rb->lcd_framebuffer, 0, sizeof(fb_data) * LCD_WIDTH * LCD_HEIGHT);
        sysvid_update(&draw_SCREENRECT);
        rb->lcd_update();
    }
#else
    grey_show(true);
#endif

#ifdef ENABLE_SOUND
    syssnd_pauseAll(false);
#endif
}

/* eof */
