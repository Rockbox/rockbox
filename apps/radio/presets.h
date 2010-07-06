/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Bertrik Sikken
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
 
int  handle_radio_presets(void);
int  handle_radio_add_preset(void);

int  presets_scan(void *viewports);
bool presets_have_changed(void);
void presets_save(void);

int  preset_list_load(void);
int  preset_list_save(void);    // prompts for name of preset file and saves
int  preset_list_clear(void);

void preset_next(int direction);
void preset_set_current(int preset);
int  preset_find(int freq);
void preset_talk(int preset, bool fallback, bool enqueue);

