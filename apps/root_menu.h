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
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#include "config.h"
#ifndef __ROOT_MENU_H__
#define __ROOT_MENU_H__

void root_menu(void);

enum {
    /* from old menu api, but still required*/
    MENU_ATTACHED_USB = -10,
    MENU_SELECTED_EXIT = -9,

    GO_TO_ROOTITEM_CONTEXT = -5,
    GO_TO_PREVIOUS_MUSIC = -4,
    GO_TO_PREVIOUS_BROWSER = -3,
    GO_TO_PREVIOUS = -2,
    GO_TO_ROOT = -1,
    GO_TO_FILEBROWSER = 0,
    GO_TO_DBBROWSER,
    GO_TO_WPS,
    GO_TO_MAINMENU,
#ifdef HAVE_RECORDING
    GO_TO_RECSCREEN,
#endif
#if CONFIG_TUNER
    GO_TO_FM,
#endif
    GO_TO_RECENTBMARKS,
    /* Do Not add any items above here unless you want it to be able to 
       be the "start screen" after a boot up. The setting in settings_list.c
       will need editing if this is the case. */
    GO_TO_BROWSEPLUGINS,
};

extern const struct menu_item_ex root_menu_;

extern void previous_music_is_wps(void);

#endif /* __ROOT_MENU_H__ */
