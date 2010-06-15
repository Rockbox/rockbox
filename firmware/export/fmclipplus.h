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
 * Tuner header for the Silicon Labs Mystery radio chip in some Sansa Clip+
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

#ifndef _FMCLIPPLUS_H_
#define _FMCLIPPLUS_H_

#define HAVE_RADIO_REGION

struct fmclipplus_region_data
{
    unsigned char deemphasis; /* 0: 75us, 1: 50us */
    unsigned char band; /* 0: us/europe, 1: japan */
} __attribute__((packed));

extern const struct fmclipplus_region_data fmclipplus_region_data[TUNER_NUM_REGIONS];

struct fmclipplus_dbg_info
{
    uint16_t regs[32];  /* Read registers */
};

bool fmclipplus_detect(void);
void fmclipplus_init(void);
int fmclipplus_set(int setting, int value);
int fmclipplus_get(int setting);
void fmclipplus_dbg_info(struct fmclipplus_dbg_info *nfo);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set fmclipplus_set
#define tuner_get fmclipplus_get
#endif

#endif /* _FMCLIPPLUS_H_ */
