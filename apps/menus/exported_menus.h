/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
//        playback_settings,          /* playback_menu.c  */
#ifdef HAVE_RECORDING
        recording_settings_menu,    /* recording_menu.c */
#endif
        sound_settings,             /* sound_menu.c     */
        settings_menu_item,         /* settings_menu.c  */
        playlist_menu_item;         /* playlist_menu.c  */



#endif /* ! PLUGIN */
#endif /*_EXPORTED_MENUS_H */
