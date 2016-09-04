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
#include "lib/configfile.h"
#include "settings.h"

/* supported languages */
const char * picoSupportedLang[] = { "en-US", "en-GB", "de-DE", "es-ES", "fr-FR", "it-IT" };

static struct preferences_t prefs = {.lang_index = en_GB, .pitch = 100, .speed = 100};

static const char config_file[] = "picotts.config";
static struct configdata config[] = {
    { TYPE_ENUM, 0, LANG_CNT, { .int_p = &prefs.lang_index }, "lang", (char **)picoSupportedLang },
    { TYPE_INT, 50, 200, { .int_p = &prefs.pitch }, "pitch", NULL },
    { TYPE_INT, 50, 200, { .int_p = &prefs.speed }, "speed", NULL }
};

void load_settings(void)
{
    configfile_load(config_file, config, ARRAYLEN(config), 0);
}

void save_settings(void)
{
    configfile_save(config_file, config, ARRAYLEN(config), 0);
}

int get_lang(void)
{
    return prefs.lang_index;
}

void set_lang(int lang)
{
    prefs.lang_index = lang;
    save_settings();
}

int get_pitch(void)
{
    return prefs.pitch;
}

void set_pitch(int pitch)
{
    prefs.pitch = pitch;
    save_settings();
}

int get_speed(void)
{
    return prefs.speed;
}

void set_speed(int speed)
{
    prefs.speed = speed;
    save_settings();
}
