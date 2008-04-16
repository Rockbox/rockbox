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

#include "plugin.h"
#include "action.h"
#include "pluginlib_actions.h"

#if defined(HAVE_REMOTE_LCD)
const struct button_mapping remote_directions[] =
{
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
    { PLA_UP,                BUTTON_RC_BITRATE,                BUTTON_NONE},
    { PLA_DOWN,              BUTTON_RC_SOURCE,                 BUTTON_NONE},
    { PLA_LEFT,              BUTTON_RC_VOL_DOWN,               BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_VOL_UP,                 BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_RC_BITRATE|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_RC_SOURCE|BUTTON_REPEAT,   BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE},
#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H10_PAD) || \
      (CONFIG_KEYPAD == GIGABEAT_PAD)
    { PLA_UP,                BUTTON_RC_FF,                     BUTTON_NONE},
    { PLA_DOWN,              BUTTON_RC_REW,                    BUTTON_NONE},
    { PLA_LEFT,              BUTTON_RC_VOL_DOWN,               BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_VOL_UP,                 BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_RC_FF|BUTTON_REPEAT,       BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_RC_REW|BUTTON_REPEAT,      BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE},
#elif (CONFIG_KEYPAD == PLAYER_PAD) || \
      (CONFIG_KEYPAD == RECORDER_PAD)
    { PLA_UP,                BUTTON_RC_VOL_UP,                 BUTTON_NONE},
    { PLA_DOWN,              BUTTON_RC_VOL_DOWN,               BUTTON_NONE},
    { PLA_LEFT,              BUTTON_RC_LEFT,                   BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_RIGHT,                  BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_LEFT|BUTTON_REPEAT,     BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_RIGHT|BUTTON_REPEAT,    BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported remote keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};
#endif /* HAVE_REMOTE_LCD */

const struct button_mapping generic_directions[] =
{
#ifdef HAVE_TOUCHPAD
    { PLA_UP,                BUTTON_TOPMIDDLE,                  BUTTON_NONE},
    { PLA_DOWN,              BUTTON_BOTTOMMIDDLE,               BUTTON_NONE},
    { PLA_LEFT,              BUTTON_MIDLEFT,                    BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_MIDRIGHT,                   BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_MIDLEFT|BUTTON_REPEAT,      BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_NONE},
#endif
#if    (CONFIG_KEYPAD == IRIVER_H100_PAD)   \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)   \
    || (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)     \
    || (CONFIG_KEYPAD == GIGABEAT_PAD)      \
    || (CONFIG_KEYPAD == RECORDER_PAD)      \
    || (CONFIG_KEYPAD == ARCHOS_AV300_PAD)  \
    || (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD) \
    || (CONFIG_KEYPAD == SANSA_C200_PAD) \
    || (CONFIG_KEYPAD == GIGABEAT_S_PAD) \
    || (CONFIG_KEYPAD == MROBE100_PAD)
    { PLA_UP,                BUTTON_UP,                  BUTTON_NONE},
    { PLA_DOWN,              BUTTON_DOWN,                BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
   || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == SANSA_E200_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
    { PLA_UP,                BUTTON_SCROLL_BACK,      BUTTON_NONE},
    { PLA_DOWN,              BUTTON_SCROLL_FWD,     BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,            BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,           BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_SCROLL_BACK|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_SCROLL_FWD|BUTTON_REPEAT, BUTTON_NONE},
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
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
    { PLA_UP,                BUTTON_SCROLL_UP,           BUTTON_NONE},
    { PLA_DOWN,              BUTTON_SCROLL_DOWN,         BUTTON_NONE},
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_SCROLL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_SCROLL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
#elif (CONFIG_KEYPAD == MROBE500_PAD)
    { PLA_UP,                BUTTON_RC_PLAY,                    BUTTON_NONE},
    { PLA_DOWN,              BUTTON_RC_DOWN,                    BUTTON_NONE},
    { PLA_LEFT,              BUTTON_RC_REW,                     BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_FF,                      BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_RC_PLAY|BUTTON_REPEAT,      BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_RC_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_REW|BUTTON_REPEAT,       BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_FF|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == COWOND2_PAD)
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    { PLA_UP,                BUTTON_RC_VOL_UP,                  BUTTON_NONE},
    { PLA_DOWN,              BUTTON_RC_VOL_DOWN,                BUTTON_NONE},
    { PLA_LEFT,              BUTTON_RC_REW,                     BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_FF,                      BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_RC_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_REW|BUTTON_REPEAT,       BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_FF|BUTTON_REPEAT,        BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

const struct button_mapping generic_left_right_fire[] =
{
#ifdef HAVE_TOUCHPAD
    { PLA_LEFT,              BUTTON_MIDLEFT,                BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_MIDLEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_MIDRIGHT,               BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_MIDRIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_CENTER,                 BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_CENTER|BUTTON_REPEAT,   BUTTON_NONE},
#endif
#if    (CONFIG_KEYPAD == IRIVER_H100_PAD)   \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)   \
    || (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)   \
    || (CONFIG_KEYPAD == GIGABEAT_PAD)      \
    || (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD) \
    || (CONFIG_KEYPAD == GIGABEAT_S_PAD)    \
    || (CONFIG_KEYPAD == MROBE100_PAD)
    { PLA_LEFT,              BUTTON_LEFT,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RIGHT,               BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_FIRE,              BUTTON_SELECT,              BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
   || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
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
#elif (CONFIG_KEYPAD == SANSA_C200_PAD) \
    || (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
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
#elif (CONFIG_KEYPAD == MROBE500_PAD)
    { PLA_LEFT,              BUTTON_RC_REW,                     BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_FF,                      BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_REW|BUTTON_REPEAT,       BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_FF|BUTTON_REPEAT,        BUTTON_NONE},
    { PLA_FIRE,              BUTTON_RC_HEART,                   BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_RC_HEART|BUTTON_REPEAT,     BUTTON_NONE},
#elif (CONFIG_KEYPAD == COWOND2_PAD)
    { PLA_LEFT,              BUTTON_MINUS,                  BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_MINUS|BUTTON_REPEAT,    BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_PLUS,                   BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_PLUS|BUTTON_REPEAT,     BUTTON_NONE},
    { PLA_FIRE,              BUTTON_MENU,                   BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE},
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    { PLA_LEFT,              BUTTON_RC_REW,                BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_FF,                 BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_REW|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_FF|BUTTON_REPEAT,   BUTTON_NONE},
    { PLA_FIRE,              BUTTON_RC_MODE,               BUTTON_NONE},
    { PLA_FIRE_REPEAT,       BUTTON_RC_MODE|BUTTON_REPEAT, BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

/* these were taken from the bubbles plugin, so may need tweaking */
const struct button_mapping generic_actions[] = 
{
#ifdef HAVE_TOUCHPAD
    {PLA_QUIT,          BUTTON_BOTTOMRIGHT,                 BUTTON_NONE},
    {PLA_START,         BUTTON_CENTER,                      BUTTON_NONE},
    {PLA_MENU,          BUTTON_TOPLEFT,                     BUTTON_NONE},
    {PLA_FIRE,          BUTTON_BOTTOMMIDDLE,                BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,  BUTTON_NONE},
#endif
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    {PLA_QUIT,          BUTTON_OFF,                   BUTTON_NONE},
    {PLA_QUIT,          BUTTON_RC_STOP,               BUTTON_NONE},
    {PLA_START,         BUTTON_ON,                    BUTTON_NONE},
    {PLA_START,         BUTTON_RC_ON,                 BUTTON_NONE},
    {PLA_MENU,          BUTTON_MODE,                  BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,                BUTTON_NONE},
    {PLA_FIRE,          BUTTON_RC_MENU,               BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_RC_MENU|BUTTON_REPEAT, BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
   || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {PLA_QUIT,          BUTTON_MENU|BUTTON_SELECT,      BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY},
    {PLA_MENU,          BUTTON_MENU|BUTTON_REL,         BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
    {PLA_QUIT,          BUTTON_POWER,                   BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY,                    BUTTON_NONE},
    {PLA_START,         BUTTON_RC_PLAY,                 BUTTON_NONE},
    {PLA_MENU,          BUTTON_REC,                     BUTTON_NONE},
    {PLA_MENU,          BUTTON_RC_MENU,                 BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,                  BUTTON_NONE},
    {PLA_FIRE,          BUTTON_RC_MODE,                 BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_RC_MODE|BUTTON_REPEAT,   BUTTON_NONE},
#elif CONFIG_KEYPAD == GIGABEAT_PAD
    {PLA_QUIT,          BUTTON_POWER,       BUTTON_NONE},
    {PLA_START,         BUTTON_A,           BUTTON_NONE},
    {PLA_MENU,          BUTTON_MENU,        BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,      BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
    {PLA_QUIT,          BUTTON_BACK,        BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY,        BUTTON_NONE},
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
#elif CONFIG_KEYPAD == SANSA_C200_PAD
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
#elif (CONFIG_KEYPAD == MROBE500_PAD)
    {PLA_QUIT,          BUTTON_POWER,                       BUTTON_NONE},
    {PLA_START,         BUTTON_RC_PLAY,                     BUTTON_NONE},
    {PLA_MENU,          BUTTON_RC_MODE,                     BUTTON_NONE},
    {PLA_FIRE,          BUTTON_RC_HEART,                    BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_RC_HEART|BUTTON_REPEAT,      BUTTON_NONE},
#elif CONFIG_KEYPAD == MROBE100_PAD
    {PLA_QUIT,          BUTTON_POWER,                   BUTTON_NONE},
    {PLA_START,         BUTTON_PLAY,                    BUTTON_NONE},
    {PLA_MENU,          BUTTON_MENU,                    BUTTON_NONE},
    {PLA_FIRE,          BUTTON_SELECT,                  BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_NONE},
#elif (CONFIG_KEYPAD == COWOND2_PAD)
    {PLA_QUIT,          BUTTON_POWER,                   BUTTON_NONE},
    {PLA_START,         BUTTON_MINUS,                   BUTTON_NONE},
    {PLA_MENU,          BUTTON_MENU,                    BUTTON_NONE},
    {PLA_FIRE,          BUTTON_CENTER,                  BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_CENTER|BUTTON_REPEAT,    BUTTON_NONE},
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    {PLA_QUIT,          BUTTON_RC_REC,                  BUTTON_NONE},
    {PLA_START,         BUTTON_RC_PLAY,                 BUTTON_NONE},
    {PLA_MENU,          BUTTON_RC_MENU,                 BUTTON_NONE},
    {PLA_FIRE,          BUTTON_RC_MODE,                 BUTTON_NONE},
    {PLA_FIRE_REPEAT,   BUTTON_RC_MODE|BUTTON_REPEAT,   BUTTON_NONE},
#else
    #error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

const struct button_mapping generic_increase_decrease[] =
{
#ifdef HAVE_TOUCHPAD
    {PLA_INC,             BUTTON_TOPMIDDLE,                  BUTTON_NONE},
    {PLA_DEC,             BUTTON_BOTTOMMIDDLE,               BUTTON_NONE},
    {PLA_INC_REPEAT,      BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE},
    {PLA_DEC_REPEAT,      BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE},
#endif
#if    (CONFIG_KEYPAD == IRIVER_H100_PAD)   \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)   \
    || (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)   \
    || (CONFIG_KEYPAD == GIGABEAT_PAD)      \
    || (CONFIG_KEYPAD == RECORDER_PAD)      \
    || (CONFIG_KEYPAD == ARCHOS_AV300_PAD)  \
    || (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD) \
    || (CONFIG_KEYPAD == ONDIO_PAD)         \
    || (CONFIG_KEYPAD == GIGABEAT_S_PAD)    \
    || (CONFIG_KEYPAD == MROBE100_PAD)
    {PLA_INC,              BUTTON_UP,                  BUTTON_NONE},
    {PLA_DEC,              BUTTON_DOWN,                BUTTON_NONE},
    {PLA_INC_REPEAT,       BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE},
    {PLA_DEC_REPEAT,       BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
    {PLA_INC,              BUTTON_VOL_UP,                  BUTTON_NONE},
    {PLA_DEC,              BUTTON_VOL_DOWN,                BUTTON_NONE},
    {PLA_INC_REPEAT,       BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    {PLA_DEC_REPEAT,       BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
    || (CONFIG_KEYPAD == IPOD_3G_PAD) \
    || (CONFIG_KEYPAD == SANSA_E200_PAD) \
    || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {PLA_INC,              BUTTON_SCROLL_FWD,      BUTTON_NONE},
    {PLA_DEC,              BUTTON_SCROLL_BACK,     BUTTON_NONE},
    {PLA_INC_REPEAT,       BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_NONE},
    {PLA_DEC_REPEAT,       BUTTON_SCROLL_BACK|BUTTON_REPEAT, BUTTON_NONE},
#elif CONFIG_KEYPAD == PLAYER_PAD
    {PLA_INC,              BUTTON_STOP,  BUTTON_NONE},
    {PLA_DEC,              BUTTON_PLAY,  BUTTON_NONE},
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
    {PLA_INC,              BUTTON_SCROLL_UP,           BUTTON_NONE},
    {PLA_DEC,              BUTTON_SCROLL_DOWN,         BUTTON_NONE},
    {PLA_INC_REPEAT,       BUTTON_SCROLL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    {PLA_DEC_REPEAT,       BUTTON_SCROLL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
#elif (CONFIG_KEYPAD == MROBE500_PAD)
    {PLA_INC,             BUTTON_RC_PLAY,                    BUTTON_NONE},
    {PLA_DEC,             BUTTON_RC_DOWN,                    BUTTON_NONE},
    {PLA_INC_REPEAT,      BUTTON_RC_PLAY|BUTTON_REPEAT,      BUTTON_NONE},
    {PLA_DEC_REPEAT,      BUTTON_RC_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    {PLA_INC,             BUTTON_RC_VOL_UP,                  BUTTON_NONE},
    {PLA_DEC,             BUTTON_RC_VOL_DOWN,                BUTTON_NONE},
    {PLA_INC_REPEAT,      BUTTON_RC_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE},
    {PLA_DEC_REPEAT,      BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
#elif CONFIG_KEYPAD == COWOND2_PAD
    {PLA_INC,             BUTTON_PLUS,                       BUTTON_NONE},
    {PLA_DEC,             BUTTON_MINUS,                      BUTTON_NONE},
    {PLA_INC_REPEAT,      BUTTON_PLUS|BUTTON_REPEAT,         BUTTON_NONE},
    {PLA_DEC_REPEAT,      BUTTON_MINUS|BUTTON_REPEAT,        BUTTON_NONE},
#else
#error pluginlib_actions: Unsupported keypad
#endif
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};

static struct button_mapping **plugin_context_order;
static int plugin_context_count = 0;
static int last_context = 0; /* index into plugin_context_order 
                                of the last context returned */

const struct button_mapping* get_context_map(int context)
{
    (void)context;
    if (last_context<plugin_context_count)
        return plugin_context_order[last_context++];
    else return NULL;
}

int pluginlib_getaction(struct plugin_api *api,int timeout,
                        const struct button_mapping *plugin_contexts[],
                        int count)
{
    plugin_context_order = (struct button_mapping **)plugin_contexts;
    plugin_context_count = count;
    last_context = 0;
    return api->get_custom_action(CONTEXT_CUSTOM,timeout,get_context_map);
}
