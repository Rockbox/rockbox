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
    
    PLA_QUIT,
    PLA_START,
    PLA_MENU,
    PLA_FIRE,
    PLA_FIRE_REPEAT,

    LAST_PLUGINLIB_ACTION
};

static const struct button_mapping generic_directions[] = 
{
#if    (CONFIG_KEYPAD == IRIVER_H100_PAD)   \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)   \
    || (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)     \
    || (CONFIG_KEYPAD == GIGABEAT_PAD)      \
    || (CONFIG_KEYPAD == RECORDER_PAD)      \
    || (CONFIG_KEYPAD == ARCHOS_AV300_PAD)  \
    || (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
    { PLA_UP,                BUTTON_UP,                  BUTTON_NONE},
    { PLA_DOWN,              BUTTON_DOWN,                BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
    { PLA_UP,                BUTTON_SCROLL_FWD,      BUTTON_NONE},
    { PLA_DOWN,              BUTTON_SCROLL_BACK,     BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,            BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,           BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_NONE},
#elif CONFIG_KEYPAD == ONDIO_PAD
    { PLA_UP,                BUTTON_UP,                  BUTTON_NONE},
    { PLA_DOWN,              BUTTON_DOWN,                BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
#elif CONFIG_KEYPAD == PLAYER_PAD
    {PLA_UP,                BUTTON_STOP,  BUTTON_NONE},
    {PLA_DOWN,              BUTTON_PLAY,  BUTTON_NONE},
    {PLA_LEFT,              BUTTON_LEFT,  BUTTON_NONE},
    {PLA_RIGHT,             BUTTON_RIGHT, BUTTON_NONE},
    {PLA_UP_REPEAT,         BUTTON_STOP|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_DOWN_REPEAT,       BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) \
   || (CONFIG_KEYPAD == IRIVER_H10_PAD)
    { PLA_UP,                BUTTON_SCROLL_UP,           BUTTON_NONE},
    { PLA_DOWN,              BUTTON_SCROLL_DOWN,         BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_SCROLL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_SCROLL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

static const struct button_mapping generic_left_right_fire[] = 
{
#if    (CONFIG_KEYPAD == IRIVER_H100_PAD)   \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)   \
    || (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)     \
    || (CONFIG_KEYPAD == GIGABEAT_PAD)      \
    || (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_SELECT,              BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
    { PLA_LEFT,              BUTTON_SCROLL_BACK,         BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_SCROLL_FWD,          BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_FIRE,              BUTTON_SELECT,              BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_NONE},
#elif CONFIG_KEYPAD == ONDIO_PAD
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_UP,                  BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == PLAYER_PAD
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_ON,                  BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_ON|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == RECORDER_PAD
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_PLAY,                BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_NONE},
#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_SELECT,              BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_NONE},
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) 
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_SELECT,              BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_NONE},
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_REW,                 BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

/* these were taken from the bubbles plugin, so may need tweaking */
static const struct button_mapping generic_actions[] = 
{
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    {PLA_QUIT,          BUTTON_OFF,     BUTTON_NONE},
    {PLA_QUIT,          BUTTON_RC_STOP,  BUTTON_NONE},
    {PLA_START,         BUTTON_ON,      BUTTON_NONE},
    {PLA_MENU,          BUTTON_MODE,    BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,  BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {PLA_QUIT,          BUTTON_MENU|BUTTON_SELECT,      BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY},
    {PLA_MENU,          BUTTON_MENU|BUTTON_REL,         BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
    {PLA_QUIT,          BUTTON_POWER,       BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY,        BUTTON_NONE},
    {PLA_MENU,          BUTTON_REC,         BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,      BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == GIGABEAT_PAD
    {PLA_QUIT,          BUTTON_A,           BUTTON_NONE},
    {PLA_START,         BUTTON_POWER,       BUTTON_NONE},
    {PLA_MENU,          BUTTON_MENU,        BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,      BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == RECORDER_PAD
    {PLA_QUIT,          BUTTON_OFF,         BUTTON_NONE},
    {PLA_START,         BUTTON_ON,          BUTTON_NONE},
    {PLA_MENU,          BUTTON_F1,          BUTTON_NONE},
    {PLA_FIRE,          BUTTON_PLAY,        BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_PLAY|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
    {PLA_QUIT,          BUTTON_OFF,         BUTTON_NONE},
    {PLA_START,         BUTTON_ON,          BUTTON_NONE},
    {PLA_MENU,          BUTTON_F1,          BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,      BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == ONDIO_PAD
    {PLA_QUIT,          BUTTON_OFF,         BUTTON_NONE},
    {PLA_START,         BUTTON_MENU,        BUTTON_NONE},
    {PLA_MENU,          BUTTON_DOWN,        BUTTON_NONE},
    {PLA_FIRE,          BUTTON_UP,          BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == PLAYER_PAD
    {PLA_QUIT,          BUTTON_STOP|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_MENU,          BUTTON_MENU,        BUTTON_NONE},
    {PLA_FIRE,          BUTTON_ON,          BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_ON|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == SANSA_E200_PAD
    {PLA_QUIT,          BUTTON_POWER,       BUTTON_NONE},
    {PLA_START,         BUTTON_UP,        BUTTON_NONE},
    {PLA_MENU,          BUTTON_DOWN,        BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,      BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
    {PLA_QUIT,          BUTTON_POWER,       BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY,        BUTTON_NONE},
    {PLA_MENU,          BUTTON_FF,          BUTTON_NONE},
    {PLA_FIRE,          BUTTON_REW,         BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_REW|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
    {PLA_QUIT,          BUTTON_EQ,       BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY,     BUTTON_NONE},
    {PLA_MENU,          BUTTON_MODE,     BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,   BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

int pluginlib_getaction(struct plugin_api *api,int timeout,
                        const struct button_mapping *plugin_contexts[],
                        int count);

#endif /*  __PLUGINLIB_ACTIONS_H__ */
