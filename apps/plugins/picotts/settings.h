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
#ifndef PLUGIN_PICOTTS_SETTINGS_H
#define PLUGIN_PICOTTS_SETTINGS_H

struct preferences_t {
    int lang_index;
    int pitch;
    int speed;
};

enum {
    en_US,
    en_GB,
    de_DE,
    es_ES,
    fr_FR,
    it_IT,
    LANG_CNT
};

void load_settings(void);
void save_settings(void);

int get_lang(void);
void set_lang(int lang);
int get_pitch(void);
void set_pitch(int pitch);
int get_speed(void);
void set_speed(int speed);
#endif
