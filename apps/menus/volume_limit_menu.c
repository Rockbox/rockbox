/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Purling Nayuki
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

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "sound.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "sound_menu.h"
#include "volume_limit_menu.h"
#include "exported_menus.h"
#include "menu_common.h"
#include "splash.h"
#include "kernel.h"
#include "talk.h"
#include "option_select.h"

static int32_t get_dec_talkid(int value, int unit)
{
    return TALK_ID_DECIMAL(value, 1, unit);
}

int volume_limit_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;

    static struct int_setting volume_limit_int_setting;
        volume_limit_int_setting.option_callback = NULL;
        volume_limit_int_setting.unit = UNIT_DB;
        volume_limit_int_setting.min = sound_min(SOUND_VOLUME);
        volume_limit_int_setting.max = sound_max(SOUND_VOLUME);
        volume_limit_int_setting.step = sound_steps(SOUND_VOLUME);
        volume_limit_int_setting.formatter = NULL;
        volume_limit_int_setting.get_talk_id = get_dec_talkid;

    struct settings_list setting;
        setting.flags = F_BANFROMQS|F_INT_SETTING|F_T_INT|F_NO_WRAP;
        setting.lang_id = LANG_VOLUME_LIMIT;
        setting.default_val.int_ = sound_max(SOUND_VOLUME);
        setting.int_setting = &volume_limit_int_setting;

    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            setting.setting = &global_settings.volume_limit;
            option_screen(&setting, NULL, false, ID2P(LANG_VOLUME_LIMIT));
        case ACTION_EXIT_MENUITEM: /* on exit */
            if (global_settings.volume_limit < global_settings.volume)
            {
                global_settings.volume = global_settings.volume_limit;
                sound_set_volume(global_settings.volume);
            }
            break;
    }
    return action;
}
