/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ipod_remote_tuner.h
 * Tuner header for the ipod remote tuner and others remote tuners
 *
 * Copyright (C) 2009 Laurent Gautier
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

#ifndef _IPOD_REMOTE_TUNER_H_
#define _IPOD_REMOTE_TUNER_H_

#define HAVE_RADIO_REGION
#define TIMEOUT_VALUE 20

extern int radio_present;
extern int tuner_frequency;
extern int tuner_signal_power;
extern int radio_tuned;

/* update tuner state: plugged or unplugged when in radio mode */
extern void rmt_tuner_region(int region);
extern void rmt_tuner_set_freq(int curr_freq);
extern void rmt_tuner_freq(void);
extern void rmt_tuner_scan(int direction);

/* tuner mode state: ON or OFF */
extern void rmt_tuner_sleep(int state);

/* parameters are stereo/mono, deemphasis, delta freq... */
extern void rmt_tuner_set_param(unsigned char tuner_param);

extern void rmt_tuner_mute(int value);
extern void rmt_tuner_signal_power(unsigned char value);

extern void rmt_tuner_rds_data(void);

struct rmt_tuner_region_data
{
    /* 0: 50us, 1: 75us */
    unsigned char deemphasis;
    /* 0: europe, 1: japan (BL in TEA spec)*/
    unsigned char band;
    /* 0: us/australia (200kHz), 1: europe/japan (100kHz), 2: (50kHz) */
    unsigned char spacing;
} __attribute__((packed));

extern const struct rmt_tuner_region_data
                                rmt_tuner_region_data[TUNER_NUM_REGIONS];

int ipod_rmt_tuner_set(int setting, int value);
int ipod_rmt_tuner_get(int setting);
char* ipod_get_rds_info(int setting);


#ifndef CONFIG_TUNER_MULTI
#define tuner_set ipod_rmt_tuner_set
#define tuner_get ipod_rmt_tuner_get
#define tuner_get_rds_info ipod_get_rds_info
#endif

#endif /* _IPOD_REMOTE_TUNER_H_ */
