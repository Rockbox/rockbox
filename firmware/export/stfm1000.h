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
 * Tuner header for the STFM1000
 *
 * Copyright (C) 2012 Amaury Pouly
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

#ifndef __STFM1000__
#define __STFM1000__
#include <stdint.h>

#define HAVE_RADIO_REGION
#define HAVE_RADIO_RSSI

bool stfm1000_detect(void);
void stfm1000_init(void);
int stfm1000_set(int setting, int value);
int stfm1000_get(int setting);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set stfm1000_set
#define tuner_get stfm1000_get
#endif

#endif /* __STFM1000__ */
