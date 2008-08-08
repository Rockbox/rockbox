/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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
#ifndef _EXPORTED_MENUS_H
#define _EXPORTED_MENUS_H

#include "menu.h"
/* not needed for plugins */
#ifndef PLUGIN 

extern const struct menu_item_ex 
        main_menu_,                 /* main_menu.c      */
        display_menu,               /* display_menu.c   */
        playback_settings,          /* playback_menu.c  */
#ifdef HAVE_RECORDING
        recording_settings,         /* recording_menu.c */
#ifdef HAVE_LCD_BITMAP
        peak_meter_menu,            /* also used from within recording_menu */
#endif
#endif
        sound_settings,             /* sound_menu.c     */
        settings_menu_item,         /* settings_menu.c  */
        playlist_settings,          /* playlist_menu.c  */
        playlist_options,           /* playlist_menu.c  */
        equalizer_menu,             /* eq_menu.c        */
        info_menu,                  /* info_menu.c      */
        theme_menu;                 /* theme_menu.c     */

struct browse_folder_info {
    const char* dir;
    int show_options;
};
int browse_folder(void *param); /* in theme_menu.c as it is mostly used there */

#endif /* ! PLUGIN */
#endif /*_EXPORTED_MENUS_H */

