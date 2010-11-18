/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Tuner header for the Silicon Labs SI4700
 *
 * Copyright (C) 2008 Dave Chapman
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

#ifndef _SI4700_H_
#define _SI4700_H_
#include <stdint.h>

#define HAVE_RADIO_REGION
#define HAVE_RADIO_RSSI

struct si4700_dbg_info
{
    uint16_t regs[16];  /* Read registers */
};

bool si4700_detect(void);
void si4700_init(void);
int si4700_set(int setting, int value);
int si4700_get(int setting);
void si4700_dbg_info(struct si4700_dbg_info *nfo);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set si4700_set
#define tuner_get si4700_get
#endif

#endif /* _SI4700_H_ */
