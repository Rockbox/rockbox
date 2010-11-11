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
 * Tuner header for the RDA Microelectronics RDA5802 FM tuner chip
 *
 * Copyright (C) 2010 Bertrik Sikken
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

#ifndef _RDA5802_H_
#define _RDA5802_H_

#define HAVE_RADIO_REGION
#define HAVE_RADIO_RSSI

struct rda5802_dbg_info
{
    uint16_t regs[16];  /* Read registers */
};

bool rda5802_detect(void);
void rda5802_init(void);
int rda5802_set(int setting, int value);
int rda5802_get(int setting);
void rda5802_dbg_info(struct rda5802_dbg_info *nfo);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set rda5802_set
#define tuner_get rda5802_get
#endif

#endif /* _RDA5802_H_ */
