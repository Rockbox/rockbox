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
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
   (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define DEBUG_CANCEL  BUTTON_OFF

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#   define DEBUG_CANCEL  BUTTON_REC

#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
#   define DEBUG_CANCEL  BUTTON_RC_REC

#elif (CONFIG_KEYPAD == MPIO_HD200_PAD) || \
    (CONFIG_KEYPAD == MPIO_HD300_PAD)
#   define DEBUG_CANCEL  BUTTON_REC
#endif
bool dbg_ports(void);
bool dbg_hw_info(void);
