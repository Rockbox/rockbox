/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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
#include "menu.h"
#include "settings.h"

static bool speed_menu(int *speed)
{
    /* Get the limits considering currently used pitch */
    int speed_min = GET_SPEED(PITCH_SPEED_PRECISION * get_pitch(), STRETCH_MIN) / PITCH_SPEED_PRECISION;
    int speed_max = GET_SPEED(PITCH_SPEED_PRECISION * get_pitch(), STRETCH_MAX) / PITCH_SPEED_PRECISION;

    return rb->set_int("Speed", "%", UNIT_INT, speed, NULL, 1, speed_min, speed_max, NULL);
}

static bool pitch_menu(int *pitch)
{
    /* Get the limits considering currently used speed */
    int pitch_min = GET_PITCH(PITCH_SPEED_PRECISION * get_speed(), STRETCH_MAX) / PITCH_SPEED_PRECISION;
    int pitch_max = GET_PITCH(PITCH_SPEED_PRECISION * get_speed(), STRETCH_MIN) / PITCH_SPEED_PRECISION;

    return rb->set_int("Pitch", "%", UNIT_INT, pitch, NULL, 1, pitch_min, pitch_max, NULL);
}

static int lang_menu(void)
{
    int lang = get_lang();
    MENUITEM_STRINGLIST(menu, "Language", NULL, "en-US", "en-GB", "de-DE", "es-ES", "fr-FR", "it-IT");
    return rb->do_menu(&menu, &lang, NULL, false);
}

enum {
    MENU_SPEED,
    MENU_PITCH,
    MENU_LANG,
    MENU_QUIT
};

int display_menu(void)
{
    MENUITEM_STRINGLIST(menu, "PicoTTS Settings", NULL, "Speed", "Pitch", "Language", "Quit");
    int select = 0;
    int ret = PICOTTS_CANCEL_MENU;
    do
    {
        select = rb->do_menu(&menu, &select, NULL, false);
        switch (select)
        {
            case MENU_SPEED:
            {
                int speed = get_speed();
                speed_menu(&speed);
                if (speed != get_speed())
                {
                    set_speed(speed);
                    ret = PICOTTS_SPEED_SETTING_CHANGE;
                }
                break;
            }

            case MENU_PITCH:
            {
                int pitch = get_pitch();
                pitch_menu(&pitch);
                if (pitch != get_pitch())
                {
                    set_pitch(pitch);
                    ret = PICOTTS_PITCH_SETTING_CHANGE;
                }
                break;
            }

            case MENU_LANG:
            {
                DEBUGF("LANG\n");
                int lang = lang_menu();
                if (lang >= 0 && lang != get_lang())
                {
                    set_lang(lang);
                    ret = PICOTTS_TTS_SETTING_CHANGE;
                }
                break;
            }

            case MENU_QUIT:
            {
                DEBUGF("QUIT\n");
                return PICOTTS_EXIT_PLUGIN;
            }

            case MENU_ATTACHED_USB:
            {
                return PICOTTS_USB_CONNECTED;
            }

            default:
                DEBUGF("CANCEL\n");
        }
    } while (select >= 0);

    return ret;
}
