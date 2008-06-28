/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner header for the Philips TEA5767
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

#ifndef _TEA5767_H_
#define _TEA5767_H_

#define HAVE_RADIO_REGION
#define HAVE_RADIO_MUTE_TIMEOUT

struct tea5767_region_data
{
    unsigned char deemphasis; /* 0: 50us, 1: 75us */
    unsigned char band; /* 0: europe, 1: japan (BL in TEA spec)*/
} __attribute__((packed));

const struct tea5767_region_data tea5767_region_data[TUNER_NUM_REGIONS];

struct tea5767_dbg_info
{
    unsigned char read_regs[5];
    unsigned char write_regs[5];
};

int tea5767_set(int setting, int value);
int tea5767_get(int setting);
void tea5767_dbg_info(struct tea5767_dbg_info *info);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set tea5767_set
#define tuner_get tea5767_get
#endif

#endif /* _TEA5767_H_ */
