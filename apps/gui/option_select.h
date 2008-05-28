/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_OPTION_SELECT_H_
#define _GUI_OPTION_SELECT_H_
#include "config.h"
#include "screen_access.h"
#include "settings.h"

bool option_screen(const struct settings_list *setting,
                   struct viewport parent[NB_SCREENS],
                   bool use_temp_var, unsigned char* option_title);

#if defined(HAVE_QUICKSCREEN) || defined(HAVE_RECORDING)
void option_select_next_val(const struct settings_list *setting,
                            bool previous, bool apply);
#endif
char *option_get_valuestring(const struct settings_list *setting, 
                             char *buffer, int buf_len,
                             intptr_t temp_var);
void option_talk_value(const struct settings_list *setting, int value, bool enqueue);

/* only use this for int and bool settings */
int option_value_as_int(const struct settings_list *setting);

#endif /* _GUI_OPTION_SELECT_H_ */
