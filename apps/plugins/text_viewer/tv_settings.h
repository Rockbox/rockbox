/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#ifndef PLUGIN_TEXT_VIEWER_SETTINGS_H
#define PLUGIN_TEXT_VIEWER_SETTINGS_H

#include "tv_preferences.h"

/*
 * load from the global settings file
 *
 * [Out] prefs
 *          the structure which read settings is stored
 *
 * return
 *     true  success
 *     false failure
 */
bool tv_load_global_settings(struct tv_preferences *prefs);

/*
 * save to the global settings file
 *
 * [In] prefs
 *          the structure with settings to save
 *
 * return
 *     true  success
 *     false failure
 */
bool tv_save_global_settings(const struct tv_preferences *prefs);

/*
 * load the settings at each file
 *
 * [In] file_name
 *          the file name of file that wants to read settings
 *
 * return
 *     true  success
 *     false failure
 */
bool tv_load_settings(const unsigned char *file_name);

/*
 * save the settings at each file
 * supposed to be called only once at plugin exit
 *
 * return
 *     true  success
 *     false failure
 */
bool tv_save_settings(void);

#endif
