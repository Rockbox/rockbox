/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner header for the Sanyo LV24020LP
 *
 * Copyright (C) 2007 Michael Sevakis
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

#ifndef _LV24020LP_H_
#define _LV24020LP_H_

/* Define additional tuner messages here */
#define HAVE_RADIO_REGION

#define LV24020LP_CTRL_STAT (RADIO_GET_CHIP_FIRST+0)
#define LV24020LP_REG_STAT  (RADIO_GET_CHIP_FIRST+1)
#define LV24020LP_MSS_FM    (RADIO_GET_CHIP_FIRST+2)
#define LV24020LP_MSS_IF    (RADIO_GET_CHIP_FIRST+3)
#define LV24020LP_MSS_SD    (RADIO_GET_CHIP_FIRST+4)
#define LV24020LP_IF_SET    (RADIO_GET_CHIP_FIRST+5)
#define LV24020LP_SD_SET    (RADIO_GET_CHIP_FIRST+6)

#define LV24020LP_DEBUG_FIRST LV24020LP_CTRL_STAT
#define LV24020LP_DEBUG_LAST  LV24020LP_SD_SET

const unsigned char lv24020lp_region_data[TUNER_NUM_REGIONS];

int lv24020lp_set(int setting, int value);
int lv24020lp_get(int setting);
void lv24020lp_power(bool status);
void lv24020lp_init(void);
void lv24020lp_lock(void);
void lv24020lp_unlock(void);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set lv24020lp_set
#define tuner_get lv24020lp_get
#endif

#endif /* _LV24020LP_H_ */
