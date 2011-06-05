/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Marcin Bukat
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
#if (CONFIG_KEYPAD == IPOD_1G2G_PAD) || \
    (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_4G_PAD)
#   define DEBUG_CANCEL  BUTTON_MENU

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define DEBUG_CANCEL  BUTTON_REW

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define DEBUG_CANCEL  BUTTON_MENU

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD)
#   define DEBUG_CANCEL  BUTTON_LEFT

#elif (CONFIG_KEYPAD == PHILIPS_SA9200_PAD) || \
      (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD)
#   define DEBUG_CANCEL  BUTTON_POWER

#elif (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD)
#   define DEBUG_CANCEL  BUTTON_PREV

#elif (CONFIG_KEYPAD == SAMSUNG_YH_PAD)
#   define DEBUG_CANCEL  BUTTON_PLAY

#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
#   define DEBUG_CANCEL  BUTTON_CANCEL
#endif
bool dbg_ports(void);
bool dbg_hw_info(void);
