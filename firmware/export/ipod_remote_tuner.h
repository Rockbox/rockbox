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

extern void rmt_tuner_freq(unsigned int len, const unsigned char *buf);
extern void rmt_tuner_rds_data(unsigned int len, const unsigned char *buf);

int ipod_rmt_tuner_set(int setting, int value);
int ipod_rmt_tuner_get(int setting);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set ipod_rmt_tuner_set
#define tuner_get ipod_rmt_tuner_get
#endif

#endif /* _IPOD_REMOTE_TUNER_H_ */
