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
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "action.h"
#include "pluginlib_actions.h"

#if defined(HAVE_REMOTE_LCD)
/* remote directions */
const struct button_mapping pla_remote_ctx[] =
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
      (CONFIG_KEYPAD == GIGABEAT_PAD) || \
      (CONFIG_KEYPAD == IAUDIO_M3_PAD) || \
      (CONFIG_KEYPAD == GIGABEAT_S_PAD)
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
#elif (CONFIG_REMOTE_KEYPAD == MROBE_REMOTE)
    { PLA_UP,                BUTTON_RC_PLAY,                   BUTTON_NONE},
    { PLA_DOWN,              BUTTON_RC_DOWN,                   BUTTON_NONE},
    { PLA_LEFT,              BUTTON_RC_REW,                    BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_RC_FF,                     BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_RC_PLAY|BUTTON_REPEAT,     BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_RC_DOWN|BUTTON_REPEAT,     BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_RC_REW|BUTTON_REPEAT,      BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_RC_FF|BUTTON_REPEAT,       BUTTON_NONE},
#else
    #error pluginlib_actions: No remote directions
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_PLUGIN),
};
#endif /* HAVE_REMOTE_LCD */

/* these were taken from the bubbles plugin, so may need tweaking */
const struct button_mapping pla_main_ctx[] = 
{
    /* Touchscreens */
#ifdef HAVE_TOUCHSCREEN
    { PLA_CANCEL,           BUTTON_BOTTOMRIGHT,                 BUTTON_NONE},
    { PLA_SELECT,           BUTTON_CENTER,                      BUTTON_NONE},
    { PLA_SELECT_REL,       BUTTON_CENTER|BUTTON_REL,           BUTTON_NONE},
    { PLA_SELECT_REPEAT,    BUTTON_CENTER|BUTTON_REPEAT,        BUTTON_NONE},
    { PLA_UP,               BUTTON_TOPMIDDLE,                   BUTTON_NONE},
    { PLA_DOWN,             BUTTON_BOTTOMMIDDLE,                BUTTON_NONE},
    { PLA_LEFT,             BUTTON_MIDLEFT,                     BUTTON_NONE},
    { PLA_RIGHT,            BUTTON_MIDRIGHT,                    BUTTON_NONE},
    { PLA_UP_REPEAT,        BUTTON_TOPMIDDLE|BUTTON_REPEAT,     BUTTON_NONE},
    { PLA_DOWN_REPEAT,      BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,  BUTTON_NONE},
    { PLA_LEFT_REPEAT,      BUTTON_MIDLEFT|BUTTON_REPEAT,       BUTTON_NONE},
    { PLA_RIGHT_REPEAT,     BUTTON_MIDRIGHT|BUTTON_REPEAT,      BUTTON_NONE},
#endif

    /* Directions */
#if   ((CONFIG_KEYPAD == IRIVER_H100_PAD)   \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)   \
    || (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)     \
    || (CONFIG_KEYPAD == GIGABEAT_PAD)      \
    || (CONFIG_KEYPAD == RECORDER_PAD)      \
    || (CONFIG_KEYPAD == ARCHOS_AV300_PAD)  \
    || (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD) \
    || (CONFIG_KEYPAD == ONDIO_PAD) \
    || (CONFIG_KEYPAD == SANSA_C200_PAD) \
    || (CONFIG_KEYPAD == GIGABEAT_S_PAD) \
    || (CONFIG_KEYPAD == MROBE100_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD) \
    || (CONFIG_KEYPAD == SANSA_CLIP_PAD) \
    || (CONFIG_KEYPAD == CREATIVEZVM_PAD) \
    || (CONFIG_KEYPAD == SANSA_M200_PAD)\
    || (CONFIG_KEYPAD == SANSA_E200_PAD) \
    || (CONFIG_KEYPAD == SANSA_FUZE_PAD) \
    || (CONFIG_KEYPAD == SAMSUNG_YH_PAD) \
    || (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD) \
    || (CONFIG_KEYPAD == SANSA_CONNECT_PAD) \
    || (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD) \
    || (CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD) \
    || (CONFIG_KEYPAD == HM60X_PAD) \
    || (CONFIG_KEYPAD == HM801_PAD))
    { PLA_UP,               BUTTON_UP,                          BUTTON_NONE },
    { PLA_DOWN,             BUTTON_DOWN,                        BUTTON_NONE },
    { PLA_LEFT,             BUTTON_LEFT,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_RIGHT,                       BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_UP|BUTTON_REPEAT,            BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
    /* now the bad ones that don't have standard names for the directional
     * buttons */
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
    || (CONFIG_KEYPAD == IPOD_3G_PAD) \
    || (CONFIG_KEYPAD == IPOD_4G_PAD)
    { PLA_UP,               BUTTON_MENU,                        BUTTON_NONE },
    { PLA_DOWN,             BUTTON_PLAY,                        BUTTON_NONE },
    { PLA_LEFT,             BUTTON_LEFT,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_RIGHT,                       BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_MENU|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
#elif (CONFIG_KEYPAD == PLAYER_PAD)
    { PLA_UP,               BUTTON_PLAY,                        BUTTON_NONE },
    { PLA_DOWN,             BUTTON_STOP,                        BUTTON_NONE },
    { PLA_LEFT,             BUTTON_LEFT,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_RIGHT,                       BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_STOP|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
    { PLA_UP,               BUTTON_SCROLL_UP,                   BUTTON_NONE },
    { PLA_DOWN,             BUTTON_SCROLL_DOWN,                 BUTTON_NONE },
    { PLA_LEFT,             BUTTON_LEFT,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_RIGHT,                       BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_SCROLL_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_SCROLL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
    { PLA_UP,               BUTTON_VOL_UP,                      BUTTON_NONE },
    { PLA_DOWN,             BUTTON_VOL_DOWN,                    BUTTON_NONE },
    { PLA_LEFT,             BUTTON_LEFT,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_RIGHT,                       BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
#elif (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_SA9200_PAD)
    { PLA_UP,               BUTTON_UP,                          BUTTON_NONE },
    { PLA_DOWN,             BUTTON_DOWN,                        BUTTON_NONE },
    { PLA_LEFT,             BUTTON_PREV,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_NEXT,                        BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_UP|BUTTON_REPEAT,            BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_PREV|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_NEXT|BUTTON_REPEAT,          BUTTON_NONE },
#elif (CONFIG_KEYPAD == IAUDIO67_PAD)
    { PLA_UP,               BUTTON_STOP,                        BUTTON_NONE },
    { PLA_DOWN,             BUTTON_PLAY,                        BUTTON_NONE },
    { PLA_LEFT,             BUTTON_LEFT,                        BUTTON_NONE },
    { PLA_RIGHT,            BUTTON_RIGHT,                       BUTTON_NONE },
    { PLA_UP_REPEAT,        BUTTON_STOP|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_DOWN_REPEAT,      BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_LEFT_REPEAT,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE },
    { PLA_RIGHT_REPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE },
#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
    { PLA_UP,                BUTTON_UP,                         BUTTON_NONE},
    { PLA_DOWN,              BUTTON_DOWN,                       BUTTON_NONE},
    { PLA_LEFT,              BUTTON_PREV,                       BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_NEXT,                       BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_UP|BUTTON_REPEAT,           BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_DOWN|BUTTON_REPEAT,         BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_PREV|BUTTON_REPEAT,         BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_NEXT|BUTTON_REPEAT,         BUTTON_NONE},
#elif (CONFIG_KEYPAD == MPIO_HD200_PAD)
    { PLA_UP,                BUTTON_REW,                        BUTTON_NONE},
    { PLA_DOWN,              BUTTON_FF,                         BUTTON_NONE},
    { PLA_LEFT,              BUTTON_VOL_DOWN,                   BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_VOL_UP,                     BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_REW|BUTTON_REPEAT,          BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_FF|BUTTON_REPEAT,           BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_VOL_DOWN|BUTTON_REPEAT,     BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_VOL_UP|BUTTON_REPEAT,       BUTTON_NONE},
#elif (CONFIG_KEYPAD == MPIO_HD300_PAD)
    { PLA_UP,                BUTTON_UP,                         BUTTON_NONE},
    { PLA_DOWN,              BUTTON_DOWN,                       BUTTON_NONE},
    { PLA_LEFT,              BUTTON_REW,                        BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_FF,                         BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_UP|BUTTON_REPEAT,           BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_DOWN|BUTTON_REPEAT,         BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_REW|BUTTON_REPEAT,          BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_FF|BUTTON_REPEAT,           BUTTON_NONE},
#elif (CONFIG_KEYPAD == RK27XX_GENERIC_PAD)
    { PLA_UP,                BUTTON_REW,                        BUTTON_NONE},
    { PLA_DOWN,              BUTTON_FF,                         BUTTON_NONE},
    { PLA_LEFT,              BUTTON_REW|BUTTON_M,               BUTTON_NONE},
    { PLA_RIGHT,             BUTTON_FF|BUTTON_M,                BUTTON_NONE},
    { PLA_UP_REPEAT,         BUTTON_REW|BUTTON_REPEAT,          BUTTON_NONE},
    { PLA_DOWN_REPEAT,       BUTTON_FF|BUTTON_REPEAT,           BUTTON_NONE},
    { PLA_LEFT_REPEAT,       BUTTON_REW|BUTTON_M|BUTTON_REPEAT, BUTTON_NONE},
    { PLA_RIGHT_REPEAT,      BUTTON_FF|BUTTON_M|BUTTON_REPEAT,  BUTTON_NONE},
#else
#   ifndef HAVE_TOUCHSCREEN
#       error pluginlib_actions: No directions defined
#   endif
#endif

    /* Scrollwheels */
#ifdef HAVE_SCROLLWHEEL
    { PLA_SCROLL_BACK,       BUTTON_SCROLL_BACK,                BUTTON_NONE },
    { PLA_SCROLL_FWD,        BUTTON_SCROLL_FWD,                 BUTTON_NONE },
    { PLA_SCROLL_BACK_REPEAT,BUTTON_SCROLL_BACK|BUTTON_REPEAT,  BUTTON_NONE },
    { PLA_SCROLL_FWD_REPEAT, BUTTON_SCROLL_FWD|BUTTON_REPEAT,   BUTTON_NONE },
#endif

    /* Actions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    {PLA_CANCEL,            BUTTON_OFF,                         BUTTON_NONE },
    {PLA_CANCEL,            BUTTON_RC_STOP,                     BUTTON_NONE },
    {PLA_EXIT,              BUTTON_ON,                          BUTTON_NONE },
    {PLA_EXIT,              BUTTON_RC_MENU,                     BUTTON_NONE },
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE },
    {PLA_SELECT,            BUTTON_RC_ON,                       BUTTON_NONE },
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    {PLA_SELECT_REL,        BUTTON_RC_ON|BUTTON_REL,            BUTTON_RC_ON },
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE },
    {PLA_SELECT_REPEAT,     BUTTON_RC_ON|BUTTON_REPEAT,         BUTTON_NONE },
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
   || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {PLA_CANCEL,            BUTTON_MENU|BUTTON_SELECT,          BUTTON_NONE },
    {PLA_EXIT,              BUTTON_PLAY|BUTTON_SELECT,          BUTTON_NONE },
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE },
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE },
#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
    {PLA_CANCEL,            BUTTON_REC,                         BUTTON_NONE },
    {PLA_CANCEL,            BUTTON_RC_REC|BUTTON_REL,           BUTTON_RC_REC},
    {PLA_EXIT,              BUTTON_POWER,                       BUTTON_NONE },
    {PLA_EXIT,              BUTTON_RC_REC|BUTTON_REPEAT,        BUTTON_NONE },
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE },
    {PLA_SELECT,            BUTTON_RC_MODE,                     BUTTON_NONE },
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    {PLA_SELECT_REL,        BUTTON_RC_MODE|BUTTON_REL,          BUTTON_RC_MODE },
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE },
    {PLA_SELECT_REPEAT,     BUTTON_RC_MODE|BUTTON_REPEAT,       BUTTON_NONE },
#elif (CONFIG_KEYPAD == GIGABEAT_PAD \
    || CONFIG_KEYPAD == SANSA_E200_PAD \
    || CONFIG_KEYPAD == SANSA_C200_PAD \
    || CONFIG_KEYPAD == SANSA_CLIP_PAD \
    || CONFIG_KEYPAD == SANSA_M200_PAD \
    || CONFIG_KEYPAD == MROBE100_PAD \
    || CONFIG_KEYPAD == PHILIPS_HDD1630_PAD \
    || CONFIG_KEYPAD == SANSA_CONNECT_PAD \
    || CONFIG_KEYPAD == HM60X_PAD \
    || CONFIG_KEYPAD == HM801_PAD)
    {PLA_CANCEL,            BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {PLA_EXIT,              BUTTON_POWER|BUTTON_REPEAT,         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif  (CONFIG_KEYPAD == GIGABEAT_S_PAD) \
    || (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
    {PLA_CANCEL,            BUTTON_BACK,                        BUTTON_NONE},
    {PLA_EXIT,              BUTTON_MENU,                        BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == RECORDER_PAD)
    {PLA_CANCEL,            BUTTON_ON,                          BUTTON_NONE},
    {PLA_EXIT,              BUTTON_OFF,                         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_PLAY,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {PLA_SELECT_REPEAT,     BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
    {PLA_CANCEL,            BUTTON_OFF|BUTTON_REL,              BUTTON_OFF},
    {PLA_EXIT,              BUTTON_OFF|BUTTON_REPEAT,           BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == ONDIO_PAD)
    {PLA_CANCEL,            BUTTON_OFF|BUTTON_REL,              BUTTON_OFF},
    {PLA_EXIT,              BUTTON_OFF|BUTTON_REPEAT,           BUTTON_NONE},
    {PLA_SELECT,            BUTTON_MENU,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_MENU|BUTTON_REL,             BUTTON_MENU},
    {PLA_SELECT_REPEAT,     BUTTON_MENU|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == PLAYER_PAD)
    {PLA_CANCEL,            BUTTON_MENU|BUTTON_REL,             BUTTON_MENU},
    {PLA_EXIT,              BUTTON_MENU|BUTTON_REPEAT,          BUTTON_NONE},
    {PLA_SELECT,            BUTTON_ON,                          BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_ON|BUTTON_REL,               BUTTON_ON},
    {PLA_SELECT_REPEAT,     BUTTON_ON|BUTTON_REPEAT,            BUTTON_NONE},
#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
    {PLA_CANCEL,            BUTTON_HOME|BUTTON_REL,             BUTTON_HOME},
    {PLA_EXIT,              BUTTON_HOME|BUTTON_REPEAT,          BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
    {PLA_CANCEL,            BUTTON_REW,                         BUTTON_NONE},
    {PLA_EXIT,              BUTTON_POWER,                       BUTTON_NONE},
    {PLA_SELECT,            BUTTON_PLAY,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {PLA_SELECT_REPEAT,     BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
    {PLA_CANCEL,            BUTTON_EQ|BUTOTN_REL,               BUTTON_EQ},
    {PLA_EXIT,              BUTTON_EQ|BUTTON_REPEAT,            BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == MROBE500_PAD)
    {PLA_CANCEL,            BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {PLA_EXIT,              BUTTON_POWER|BUTTON_REPEAT,         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_RC_HEART,                    BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_RC_HEART|BUTTON_REL,         BUTTON_RC_HEART},
    {PLA_SELECT_REPEAT,     BUTTON_RC_HEART|BUTTON_REPEAT,      BUTTON_NONE},
#elif (CONFIG_KEYPAD == COWON_D2_PAD)
    {PLA_CANCEL,            BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {PLA_EXIT,              BUTTON_POWER|BUTTON_REPEAT,         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_MINUS,                       BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_MINUS|BUTTON_REL,            BUTTON_MINUS},
    {PLA_SELECT_REPEAT,     BUTTON_MINUS|BUTTON_REPEAT,         BUTTON_NONE},
#elif (CONFIG_KEYPAD == ANDROID_PAD)
    {PLA_CANCEL,            BUTTON_BACK|BUTTON_REL,             BUTTON_BACK},
    {PLA_EXIT,              BUTTON_BACK|BUTTON_REPEAT,          BUTTON_NONE},
    {PLA_SELECT,            BUTTON_MENU,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_MENU|BUTTON_REL,             BUTTON_MENU},
    {PLA_SELECT_REPEAT,     BUTTON_MENU|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
    {PLA_CANCEL,            BUTTON_RC_REC|BUTTON_REL,           BUTTON_RC_REC},
    {PLA_EXIT,              BUTTON_RC_REC|BUTTON_REPEAT,        BUTTON_NONE},
    {PLA_SELECT,            BUTTON_RC_MODE,                     BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_RC_MODE|BUTTON_REL,          BUTTON_RC_MODE},
    {PLA_SELECT_REPEAT,     BUTTON_RC_MODE|BUTTON_REPEAT,       BUTTON_NONE},
#elif (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_SA9200_PAD)
    {PLA_EXIT,              BUTTON_POWER,                       BUTTON_NONE},
    {PLA_CANCEL,            BUTTON_MENU,                        BUTTON_NONE},
    {PLA_SELECT,            BUTTON_PLAY,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {PLA_SELECT_REPEAT,     BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == IAUDIO67_PAD)
    {PLA_CANCEL,            BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {PLA_EXIT,              BUTTON_POWER|BUTTON_REPEAT,         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_PLAY,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {PLA_SELECT_REPEAT,     BUTTON_PLAY|BUTTON_REPEAT           BUTTON_NONE},
#elif (CONFIG_KEYPAD == CREATIVEZVM_PAD)
    {PLA_CANCEL,            BUTTON_BACK|BUTTON_REL,             BUTTON_BACK},
    {PLA_EXIT,              BUTTON_BACK|BUTTON_REPEAT,          BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
    {PLA_CANCEL,            BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {PLA_EXIT,              BUTTON_POWER|BUTTON_REPEAT,         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_VOL_UP,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_VOL_UP|BUTTON_REL,           BUTTON_VOL_UP},
    {PLA_SELECT_REPEAT,     BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == SAMSUNG_YH_PAD)
    {PLA_CANCEL,            BUTTON_REW,                         BUTTON_NONE},
    {PLA_EXIT,              BUTTON_FFWD,                        BUTTON_NONE},
    {PLA_SELECT,            BUTTON_PLAY,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {PLA_SELECT_REPEAT,     BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
    {PLA_CANCEL,            BUTTON_MENU,                        BUTTON_NONE},
    {PLA_EXIT,              BUTTON_REC,                         BUTTON_NONE},
    {PLA_SELECT,            BUTTON_OK,                          BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_OK|BUTTON_REL,               BUTTON_OK  },
    {PLA_SELECT_REPEAT,     BUTTON_OK|BUTTON_REPEAT,            BUTTON_NONE},
#elif (CONFIG_KEYPAD == MPIO_HD200_PAD)                                      
    {PLA_CANCEL,            BUTTON_REC,                         BUTTON_NONE},
    {PLA_EXIT,              (BUTTON_REC|BUTTON_PLAY),           BUTTON_NONE},
    {PLA_SELECT,            BUTTON_FUNC,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_FUNC|BUTTON_REL,             BUTTON_FUNC},
    {PLA_SELECT_REPEAT,     BUTTON_FUNC|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == MPIO_HD300_PAD)                                      
    {PLA_CANCEL,            BUTTON_MENU,                        BUTTON_NONE},
    {PLA_EXIT,              BUTTON_MENU|BUTTON_REPEAT,          BUTTON_NONE},
    {PLA_SELECT,            BUTTON_ENTER,                       BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_ENTER|BUTTON_REL,            BUTTON_ENTER},
    {PLA_SELECT_REPEAT,     BUTTON_ENTER|BUTTON_REPEAT,         BUTTON_NONE},
#elif (CONFIG_KEYPAD == RK27XX_GENERIC_PAD)                                      
    {PLA_CANCEL,            BUTTON_M,                           BUTTON_NONE},
    {PLA_EXIT,              BUTTON_M|BUTTON_REPEAT,             BUTTON_NONE},
    {PLA_SELECT,            BUTTON_PLAY,                        BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {PLA_SELECT_REPEAT,     BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_NONE},
#elif (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)                                      
    {PLA_CANCEL,            BUTTON_BACK,                        BUTTON_NONE},
    {PLA_EXIT,              BUTTON_POWER,                       BUTTON_NONE},
    {PLA_SELECT,            BUTTON_SELECT,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {PLA_SELECT_REPEAT,     BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_NONE},
#elif (CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD)
    {PLA_CANCEL,            BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {PLA_EXIT,              BUTTON_POWER,                       BUTTON_NONE},
    {PLA_SELECT,            BUTTON_VOL_UP,                      BUTTON_NONE},
    {PLA_SELECT_REL,        BUTTON_VOL_UP|BUTTON_REL,           BUTTON_VOL_UP},
    {PLA_SELECT_REPEAT,     BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
#else
#   ifndef HAVE_TOUCHSCREEN
#       error pluginlib_actions: No actions defined
#   endif
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_PLUGIN),
};

static struct button_mapping **plugin_context_order;
static int plugin_context_count = 0;
static int last_context = 0; /* index into plugin_context_order 
                                of the last context returned */

static const struct button_mapping* get_context_map(int context)
{
    (void)context;
    if (last_context<plugin_context_count)
        return plugin_context_order[last_context++];
    else return NULL;
}

int pluginlib_getaction(int timeout,
                        const struct button_mapping *plugin_contexts[],
                        int count)
{
    plugin_context_order = (struct button_mapping **)plugin_contexts;
    plugin_context_count = count;
    last_context = 0;
    return rb->get_custom_action(CONTEXT_PLUGIN,timeout,get_context_map);
}
