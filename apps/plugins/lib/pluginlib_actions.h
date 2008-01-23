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
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __PLUGINLIB_ACTIONS_H__
#define __PLUGINLIB_ACTIONS_H__

#include "config.h"
#include "plugin.h"
#include "action.h"

/* PLA stands for Plugin Lib Action :P */
enum {
    PLA_UP = LAST_ACTION_PLACEHOLDER+1,
    PLA_DOWN,
    PLA_LEFT,
    PLA_RIGHT,
    PLA_UP_REPEAT,
    PLA_DOWN_REPEAT,
    PLA_LEFT_REPEAT,
    PLA_RIGHT_REPEAT,

    PLA_INC,
    PLA_DEC,
    PLA_INC_REPEAT,
    PLA_DEC_REPEAT,

    PLA_QUIT,
    PLA_START,
    PLA_MENU,
    PLA_FIRE,
    PLA_FIRE_REPEAT,

    LAST_PLUGINLIB_ACTION
};

#if defined(HAVE_REMOTE_LCD)
extern const struct button_mapping remote_directions[];
#endif
extern const struct button_mapping generic_directions[];
extern const struct button_mapping generic_left_right_fire[];
extern const struct button_mapping generic_actions[];
extern const struct button_mapping generic_increase_decrease[];

int pluginlib_getaction(struct plugin_api *api,int timeout,
                        const struct button_mapping *plugin_contexts[],
                        int count);

#endif /*  __PLUGINLIB_ACTIONS_H__ */
