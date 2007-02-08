
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "recording.h"

#ifdef HAVE_RECORDING
MENUITEM_FUNCTION(rec_menu_recording_screen_item, ID2P(LANG_RECORDING_MENU),
                   (menu_function)recording_screen, NULL);
/* TEMP */
bool recording_menu(bool no_source); /* from apps/sound_menu.h */
MENUITEM_FUNCTION_WPARAM(recording_settings, ID2P(LANG_RECORDING_SETTINGS),
                          (int (*)(void*))recording_menu,0, NULL);

MAKE_MENU(recording_settings_menu,ID2P(LANG_RECORDING),0,
          &rec_menu_recording_screen_item, &recording_settings);
#endif
